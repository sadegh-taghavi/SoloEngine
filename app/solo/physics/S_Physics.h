#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "solo/math/S_Math.h"

namespace solo
{

enum class S_BodyType
{
    Static,
    Dynamic,
    Kinematic,
};

struct S_BodyHandle
{
    uint32_t id = 0xFFFFFFFFu; // JPH::BodyID raw value
    bool valid() const { return id != 0xFFFFFFFFu; }
};

// emitted when two bodies first touch (new contact manifold)
struct S_ContactEvent
{
    S_BodyHandle bodyA;
    S_BodyHandle bodyB;
    glm::vec3    position;    // world-space contact point
    glm::vec3    normal;      // from bodyA toward bodyB
    float        impactSpeed; // relative velocity along the normal, m/s
};

// Kinematic character (Jolt CharacterVirtual): capsule with feet-origin
// position, slope/step handling, gravity and jumping handled in update().
// Create through S_Physics::createCharacter.
class S_Character
{
public:
    ~S_Character();

    // desiredVelocity: world-space horizontal movement (y ignored);
    // jump fires only when grounded
    void update(float dtSeconds, const glm::vec3& desiredVelocity, bool jump = false);

    glm::vec3 position() const;   // feet position
    glm::vec3 velocity() const;
    bool      isOnGround() const;
    bool      justLanded() const; // true for the frame the character touches down

    void setJumpSpeed(float speed) { m_jumpSpeed = speed; }
    void teleport(const glm::vec3& position);

private:
    friend class S_Physics;
    S_Character();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    float m_jumpSpeed = 8.0f;
};

// Jolt Physics wrapper: fixed-timestep simulation with the standard two-layer
// (static / moving) broad phase setup. Positions/rotations cross the boundary
// as glm types; nothing outside solo/physics includes Jolt headers.
class S_Physics
{
public:
    S_Physics();
    ~S_Physics();

    void update(float dtSeconds); // accumulates and steps at a fixed 60 Hz

    // new contacts generated during the most recent update() call
    const std::vector<S_ContactEvent>& contactEvents() const;

    S_BodyHandle createBox(const glm::vec3& halfExtents, const glm::vec3& position,
                           S_BodyType type = S_BodyType::Dynamic,
                           const glm::quat& rotation = glm::quat(1, 0, 0, 0));
    S_BodyHandle createSphere(float radius, const glm::vec3& position,
                              S_BodyType type = S_BodyType::Dynamic);
    void removeBody(S_BodyHandle body);

    // capsule character; position = feet. Total height = 2 * (halfHeight + radius)
    std::unique_ptr<S_Character> createCharacter(float radius, float halfHeight,
                                                 const glm::vec3& position);

    glm::mat4 bodyTransform(S_BodyHandle body) const; // rotation + translation
    glm::vec3 bodyPosition(S_BodyHandle body) const;
    bool      bodyActive(S_BodyHandle body) const;

    glm::vec3 linearVelocity(S_BodyHandle body) const;
    void setLinearVelocity(S_BodyHandle body, const glm::vec3& velocity);
    void addImpulse(S_BodyHandle body, const glm::vec3& impulse);

    void setGravity(const glm::vec3& gravity);

private:
    struct Impl; // hides all Jolt types from engine headers
    std::unique_ptr<Impl> m_impl;
};

}
