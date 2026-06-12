#include "S_Physics.h"
#include "solo/debug/S_Debug.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <thread>
#include <mutex>

using namespace solo;

namespace
{

// object layers
namespace Layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING     = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

// broad phase layers
namespace BPLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS = 2;
}

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    virtual JPH::uint GetNumBroadPhaseLayers() const override { return BPLayers::NUM_LAYERS; }
    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return layer == Layers::NON_MOVING ? BPLayers::NON_MOVING : BPLayers::MOVING;
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        return layer == BPLayers::NON_MOVING ? "NON_MOVING" : "MOVING";
    }
#endif
};

class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override
    {
        if (layer1 == Layers::NON_MOVING)
            return layer2 == BPLayers::MOVING;
        return true;
    }
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const override
    {
        if (layer1 == Layers::NON_MOVING)
            return layer2 == Layers::MOVING;
        return true;
    }
};

JPH::Vec3 toJolt(const glm::vec3& v)  { return { v.x, v.y, v.z }; }
JPH::Quat toJolt(const glm::quat& q)  { return { q.x, q.y, q.z, q.w }; }

JPH::EMotionType toJolt(S_BodyType t)
{
    switch (t)
    {
    case S_BodyType::Static:    return JPH::EMotionType::Static;
    case S_BodyType::Kinematic: return JPH::EMotionType::Kinematic;
    default:                    return JPH::EMotionType::Dynamic;
    }
}

// collects new-contact events; Jolt calls this from its job threads during
// Update, so writes go through a mutex and are drained once per S_Physics::update
class ContactListenerImpl final : public JPH::ContactListener
{
public:
    virtual void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2,
                                const JPH::ContactManifold& manifold,
                                JPH::ContactSettings&) override
    {
        S_ContactEvent e;
        e.bodyA = { body1.GetID().GetIndexAndSequenceNumber() };
        e.bodyB = { body2.GetID().GetIndexAndSequenceNumber() };

        JPH::RVec3 p = manifold.GetWorldSpaceContactPointOn1(0);
        e.position = { static_cast<float>(p.GetX()), static_cast<float>(p.GetY()), static_cast<float>(p.GetZ()) };
        e.normal   = { manifold.mWorldSpaceNormal.GetX(), manifold.mWorldSpaceNormal.GetY(),
                       manifold.mWorldSpaceNormal.GetZ() };

        JPH::Vec3 relVel = body2.GetLinearVelocity() - body1.GetLinearVelocity();
        e.impactSpeed = std::abs(relVel.Dot(manifold.mWorldSpaceNormal));

        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending.push_back(e);
    }

    void drain(std::vector<S_ContactEvent>& out)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        out.insert(out.end(), m_pending.begin(), m_pending.end());
        m_pending.clear();
    }

private:
    std::mutex                  m_mutex;
    std::vector<S_ContactEvent> m_pending;
};

}

struct S_Physics::Impl
{
    BPLayerInterfaceImpl              bpLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBpFilter;
    ObjectLayerPairFilterImpl         objectPairFilter;
    ContactListenerImpl               contactListener;

    std::unique_ptr<JPH::TempAllocatorImpl>    tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool>  jobSystem;
    std::unique_ptr<JPH::PhysicsSystem>        system;

    float                       accumulator = 0.0f;
    std::vector<S_ContactEvent> contactEvents;
};

S_Physics::S_Physics() : m_impl(std::make_unique<Impl>())
{
    JPH::RegisterDefaultAllocator();
    if (JPH::Factory::sInstance == nullptr)
    {
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }

    m_impl->tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    m_impl->jobSystem     = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        static_cast<int>(std::max(1u, std::thread::hardware_concurrency() - 1)));

    constexpr JPH::uint cMaxBodies             = 4096;
    constexpr JPH::uint cNumBodyMutexes        = 0;
    constexpr JPH::uint cMaxBodyPairs          = 4096;
    constexpr JPH::uint cMaxContactConstraints = 2048;

    m_impl->system = std::make_unique<JPH::PhysicsSystem>();
    m_impl->system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
                         m_impl->bpLayerInterface, m_impl->objectVsBpFilter, m_impl->objectPairFilter);
    m_impl->system->SetContactListener(&m_impl->contactListener);

    s_debugLayer("S_Physics: Jolt initialized");
}

S_Physics::~S_Physics()
{
    m_impl->system.reset();
    m_impl->jobSystem.reset();
    m_impl->tempAllocator.reset();
    // Factory/types stay registered for the process lifetime (cheap, avoids
    // unregister ordering issues if multiple systems ever coexist)
}

void S_Physics::update(float dtSeconds)
{
    m_impl->contactEvents.clear();

    constexpr float kFixedStep = 1.0f / 60.0f;
    m_impl->accumulator += glm::min(dtSeconds, 0.25f); // clamp huge hitches
    while (m_impl->accumulator >= kFixedStep)
    {
        m_impl->system->Update(kFixedStep, 1, m_impl->tempAllocator.get(), m_impl->jobSystem.get());
        m_impl->accumulator -= kFixedStep;
    }

    m_impl->contactListener.drain(m_impl->contactEvents);
}

const std::vector<S_ContactEvent>& S_Physics::contactEvents() const
{
    return m_impl->contactEvents;
}

S_BodyHandle S_Physics::createBox(const glm::vec3& halfExtents, const glm::vec3& position,
                                  S_BodyType type, const glm::quat& rotation)
{
    JPH::BodyInterface& bi = m_impl->system->GetBodyInterface();
    JPH::BodyCreationSettings settings(
        new JPH::BoxShape(toJolt(halfExtents)),
        toJolt(position), toJolt(rotation), toJolt(type),
        type == S_BodyType::Static ? Layers::NON_MOVING : Layers::MOVING);
    JPH::BodyID id = bi.CreateAndAddBody(settings,
        type == S_BodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
    return { id.GetIndexAndSequenceNumber() };
}

S_BodyHandle S_Physics::createSphere(float radius, const glm::vec3& position, S_BodyType type)
{
    JPH::BodyInterface& bi = m_impl->system->GetBodyInterface();
    JPH::BodyCreationSettings settings(
        new JPH::SphereShape(radius),
        toJolt(position), JPH::Quat::sIdentity(), toJolt(type),
        type == S_BodyType::Static ? Layers::NON_MOVING : Layers::MOVING);
    JPH::BodyID id = bi.CreateAndAddBody(settings,
        type == S_BodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
    return { id.GetIndexAndSequenceNumber() };
}

void S_Physics::removeBody(S_BodyHandle body)
{
    if (!body.valid()) return;
    JPH::BodyInterface& bi = m_impl->system->GetBodyInterface();
    JPH::BodyID id(body.id);
    bi.RemoveBody(id);
    bi.DestroyBody(id);
}

glm::mat4 S_Physics::bodyTransform(S_BodyHandle body) const
{
    if (!body.valid()) return glm::mat4(1.0f);
    JPH::RMat44 m = m_impl->system->GetBodyInterface().GetWorldTransform(JPH::BodyID(body.id));
    glm::mat4 out;
    for (int c = 0; c < 4; ++c)
    {
        JPH::Vec4 col = m.GetColumn4(c);
        out[c] = glm::vec4(col.GetX(), col.GetY(), col.GetZ(), col.GetW());
    }
    return out;
}

glm::vec3 S_Physics::bodyPosition(S_BodyHandle body) const
{
    if (!body.valid()) return glm::vec3(0.0f);
    JPH::RVec3 p = m_impl->system->GetBodyInterface().GetCenterOfMassPosition(JPH::BodyID(body.id));
    return { p.GetX(), p.GetY(), p.GetZ() };
}

bool S_Physics::bodyActive(S_BodyHandle body) const
{
    return body.valid() && m_impl->system->GetBodyInterface().IsActive(JPH::BodyID(body.id));
}

glm::vec3 S_Physics::linearVelocity(S_BodyHandle body) const
{
    if (!body.valid()) return glm::vec3(0.0f);
    JPH::Vec3 v = m_impl->system->GetBodyInterface().GetLinearVelocity(JPH::BodyID(body.id));
    return { v.GetX(), v.GetY(), v.GetZ() };
}

void S_Physics::setLinearVelocity(S_BodyHandle body, const glm::vec3& velocity)
{
    if (!body.valid()) return;
    m_impl->system->GetBodyInterface().SetLinearVelocity(JPH::BodyID(body.id), toJolt(velocity));
}

void S_Physics::addImpulse(S_BodyHandle body, const glm::vec3& impulse)
{
    if (!body.valid()) return;
    m_impl->system->GetBodyInterface().AddImpulse(JPH::BodyID(body.id), toJolt(impulse));
}

void S_Physics::setGravity(const glm::vec3& gravity)
{
    m_impl->system->SetGravity(toJolt(gravity));
}

// ---- S_Character ----

struct S_Character::Impl
{
    JPH::Ref<JPH::CharacterVirtual> character;
    JPH::PhysicsSystem*             system        = nullptr;
    JPH::TempAllocator*             tempAllocator = nullptr;
    bool wasOnGround = false;
    bool landed      = false;
};

S_Character::S_Character() : m_impl(std::make_unique<Impl>()) {}
S_Character::~S_Character() = default;

std::unique_ptr<S_Character> S_Physics::createCharacter(float radius, float halfHeight,
                                                        const glm::vec3& position)
{
    auto out = std::unique_ptr<S_Character>(new S_Character());
    S_Character::Impl& impl = *out->m_impl;
    impl.system        = m_impl->system.get();
    impl.tempAllocator = m_impl->tempAllocator.get();

    // capsule offset upward so the character's position is at its feet
    const float centerY = halfHeight + radius;
    JPH::Ref<JPH::Shape> shape =
        JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, centerY, 0), JPH::Quat::sIdentity(),
                                            new JPH::CapsuleShape(halfHeight, radius))
            .Create().Get();

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mShape            = shape;
    settings->mMaxSlopeAngle    = JPH::DegreesToRadians(50.0f);
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -radius); // accept ground contact on the lower sphere

    impl.character = new JPH::CharacterVirtual(settings, toJolt(position) , JPH::Quat::sIdentity(),
                                               0, m_impl->system.get());
    return out;
}

void S_Character::update(float dtSeconds, const glm::vec3& desiredVelocity, bool jump)
{
    Impl& impl = *m_impl;
    if (!impl.character || dtSeconds <= 0.0f)
        return;

    const bool grounded =
        impl.character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

    float vy = impl.character->GetLinearVelocity().GetY();
    if (grounded)
    {
        vy = 0.0f;
        if (jump)
            vy = m_jumpSpeed;
    }
    else
        vy += impl.system->GetGravity().GetY() * dtSeconds;

    impl.character->SetLinearVelocity(JPH::Vec3(desiredVelocity.x, vy, desiredVelocity.z));

    impl.character->Update(dtSeconds, impl.system->GetGravity(),
                           impl.system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                           impl.system->GetDefaultLayerFilter(Layers::MOVING),
                           {}, {}, *impl.tempAllocator);

    const bool nowGrounded =
        impl.character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    impl.landed      = nowGrounded && !impl.wasOnGround;
    impl.wasOnGround = nowGrounded;
}

glm::vec3 S_Character::position() const
{
    JPH::RVec3 p = m_impl->character->GetPosition();
    return { static_cast<float>(p.GetX()), static_cast<float>(p.GetY()), static_cast<float>(p.GetZ()) };
}

glm::vec3 S_Character::velocity() const
{
    JPH::Vec3 v = m_impl->character->GetLinearVelocity();
    return { v.GetX(), v.GetY(), v.GetZ() };
}

bool S_Character::isOnGround() const
{
    return m_impl->character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
}

bool S_Character::justLanded() const
{
    return m_impl->landed;
}

void S_Character::teleport(const glm::vec3& position)
{
    m_impl->character->SetPosition(toJolt(position));
}
