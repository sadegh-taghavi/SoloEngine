#include "S_EntityScene.h"
#include "solo/renderer/S_Renderer.h"
#include "solo/renderer/S_Animator.h"

using namespace solo;

glm::mat4 S_TransformTRS::matrix() const
{
    return glm::translate(glm::mat4(1.0f), translation)
         * glm::mat4_cast(rotation)
         * glm::scale(glm::mat4(1.0f), scale);
}

S_EntityID S_EntityScene::create(const std::string& name, S_EntityID parent)
{
    Entity e;
    e.name   = name;
    e.parent = parent;
    e.alive  = true;

    for (size_t i = 0; i < m_entities.size(); ++i)
        if (!m_entities[i].alive)
        {
            m_entities[i] = std::move(e);
            ++m_aliveCount;
            return static_cast<S_EntityID>(i);
        }
    m_entities.push_back(std::move(e));
    ++m_aliveCount;
    return static_cast<S_EntityID>(m_entities.size() - 1);
}

void S_EntityScene::destroy(S_EntityID id)
{
    if (!valid(id)) return;
    // orphan children rather than cascading
    for (auto& e : m_entities)
        if (e.alive && e.parent == id)
            e.parent = S_NO_ENTITY;
    m_entities[id] = Entity{};
    --m_aliveCount;
}

S_EntityID S_EntityScene::find(const std::string& name) const
{
    for (size_t i = 0; i < m_entities.size(); ++i)
        if (m_entities[i].alive && m_entities[i].name == name)
            return static_cast<S_EntityID>(i);
    return S_NO_ENTITY;
}

bool S_EntityScene::valid(S_EntityID id) const
{
    return id < m_entities.size() && m_entities[id].alive;
}

void S_EntityScene::setLocal(S_EntityID id, const S_TransformTRS& trs)
{
    if (!valid(id)) return;
    m_entities[id].local = trs;
    m_entities[id].worldDirty = true;
}

void S_EntityScene::setPosition(S_EntityID id, const glm::vec3& p)
{
    if (!valid(id)) return;
    m_entities[id].local.translation = p;
    m_entities[id].worldDirty = true;
}

void S_EntityScene::setRotation(S_EntityID id, const glm::quat& r)
{
    if (!valid(id)) return;
    m_entities[id].local.rotation = r;
    m_entities[id].worldDirty = true;
}

void S_EntityScene::setScale(S_EntityID id, const glm::vec3& s)
{
    if (!valid(id)) return;
    m_entities[id].local.scale = s;
    m_entities[id].worldDirty = true;
}

const S_TransformTRS& S_EntityScene::local(S_EntityID id) const
{
    static const S_TransformTRS kIdentity;
    return valid(id) ? m_entities[id].local : kIdentity;
}

const glm::mat4& S_EntityScene::world(S_EntityID id)
{
    static const glm::mat4 kIdentity(1.0f);
    return valid(id) ? resolveWorld(id) : kIdentity;
}

void S_EntityScene::attachMesh(S_EntityID id, S_MeshHandle mesh, uint32_t materialID)
{
    if (!valid(id)) return;
    m_entities[id].mesh       = mesh;
    m_entities[id].materialID = materialID;
}

void S_EntityScene::attachBody(S_EntityID id, S_BodyHandle body)
{
    if (!valid(id)) return;
    m_entities[id].body = body;
}

void S_EntityScene::attachAnimator(S_EntityID id, S_Animator* animator)
{
    if (!valid(id)) return;
    m_entities[id].animator = animator;
}

const glm::mat4& S_EntityScene::resolveWorld(S_EntityID id)
{
    Entity& e = m_entities[id];
    if (e.worldDirty)
    {
        const glm::mat4 parentWorld =
            (e.parent != S_NO_ENTITY && valid(e.parent)) ? resolveWorld(e.parent) : glm::mat4(1.0f);
        e.world      = parentWorld * e.local.matrix();
        e.worldDirty = false;
    }
    return e.world;
}

void S_EntityScene::update(S_Physics* physics, float dtSeconds)
{
    for (auto& e : m_entities)
    {
        if (!e.alive) continue;

        if (e.animator)
            e.animator->update(dtSeconds);

        if (e.body.valid() && physics)
        {
            // body-driven entities bypass the hierarchy: world = body pose * scale
            e.world = physics->bodyTransform(e.body)
                    * glm::scale(glm::mat4(1.0f), e.local.scale);
            e.worldDirty = false;
            continue;
        }
        e.worldDirty = true;
    }

    for (S_EntityID i = 0; i < static_cast<S_EntityID>(m_entities.size()); ++i)
        if (m_entities[i].alive && !m_entities[i].body.valid())
            resolveWorld(i);
}

void S_EntityScene::render(S_Renderer& renderer) const
{
    for (const auto& e : m_entities)
    {
        if (!e.alive || !e.mesh.valid()) continue;
        if (e.animator)
            renderer.submitDraw(e.mesh, e.world, e.materialID, e.animator->palette());
        else
            renderer.submitDraw(e.mesh, e.world, e.materialID);
    }
}
