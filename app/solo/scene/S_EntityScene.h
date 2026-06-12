#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "solo/renderer/S_Handle.h"
#include "solo/physics/S_Physics.h"
#include "solo/math/S_Math.h"

namespace solo
{

class S_Renderer;
class S_Animator;

using S_EntityID = uint32_t;
static constexpr S_EntityID S_NO_ENTITY = 0xFFFFFFFFu;

struct S_TransformTRS
{
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale       = glm::vec3(1.0f);
    glm::mat4 matrix() const;
};

// Lean object/component scene: named entities with TRS + parent hierarchy and
// optional mesh, physics-body, and animator components. update() syncs bodies
// and animators and resolves world transforms; render() submits draws.
class S_EntityScene
{
public:
    S_EntityID create(const std::string& name, S_EntityID parent = S_NO_ENTITY);
    void       destroy(S_EntityID id);
    S_EntityID find(const std::string& name) const;
    bool       valid(S_EntityID id) const;

    void setLocal(S_EntityID id, const S_TransformTRS& trs);
    void setPosition(S_EntityID id, const glm::vec3& p);
    void setRotation(S_EntityID id, const glm::quat& r);
    void setScale(S_EntityID id, const glm::vec3& s);
    const S_TransformTRS& local(S_EntityID id) const;
    const glm::mat4&      world(S_EntityID id);

    void attachMesh(S_EntityID id, S_MeshHandle mesh, uint32_t materialID = 0);
    void attachBody(S_EntityID id, S_BodyHandle body);   // world follows the body (keeps local scale)
    void attachAnimator(S_EntityID id, S_Animator* animator); // not owned; palette source for skinned mesh

    // body sync + animator advance + world transform resolve
    void update(S_Physics* physics, float dtSeconds);
    void render(S_Renderer& renderer) const;

    size_t entityCount() const { return m_aliveCount; }

private:
    struct Entity
    {
        std::string    name;
        S_EntityID     parent = S_NO_ENTITY;
        bool           alive  = false;
        S_TransformTRS local;
        glm::mat4      world = glm::mat4(1.0f);
        bool           worldDirty = true;

        S_MeshHandle mesh;
        uint32_t     materialID = 0;
        S_BodyHandle body;
        S_Animator*  animator = nullptr;
    };

    const glm::mat4& resolveWorld(S_EntityID id);

    std::vector<Entity> m_entities;
    size_t              m_aliveCount = 0;
};

}
