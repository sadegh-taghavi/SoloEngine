#pragma once
#include <memory>
#include <vector>
#include "solo/renderer/S_Handle.h"
#include "solo/math/S_Math.h"

namespace solo
{

class S_Camera;

struct S_Node
{
    S_MeshHandle mesh;
    glm::mat4    transform  = glm::mat4(1.0f);
    uint32_t     materialID = 0;
};

class S_Scene
{
public:
    S_Scene();
    virtual ~S_Scene();

    std::shared_ptr<S_Camera> camera() const;
    void setCamera(const std::shared_ptr<S_Camera>& camera);

    uint32_t                   addNode(S_MeshHandle mesh, const glm::mat4& transform = glm::mat4(1.0f), uint32_t materialID = 0);
    S_Node&                    node(uint32_t index);
    const std::vector<S_Node>& nodes() const;
    void                       clear();

private:
    std::shared_ptr<S_Camera> m_camera;
    std::vector<S_Node>       m_nodes;
};

}
