#include "S_Scene.h"
#include "S_Camera.h"

using namespace solo;

S_Scene::S_Scene() {}
S_Scene::~S_Scene() {}

std::shared_ptr<S_Camera> S_Scene::camera() const { return m_camera; }
void S_Scene::setCamera(const std::shared_ptr<S_Camera>& camera) { m_camera = camera; }

uint32_t S_Scene::addNode(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID)
{
    m_nodes.push_back({ mesh, transform, materialID });
    return static_cast<uint32_t>(m_nodes.size() - 1);
}

S_Node& S_Scene::node(uint32_t index) { return m_nodes[index]; }
const std::vector<S_Node>& S_Scene::nodes() const { return m_nodes; }
void S_Scene::clear() { m_nodes.clear(); }
