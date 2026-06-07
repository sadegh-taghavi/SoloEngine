#include "S_RenderQueue.h"

using namespace solo;

void S_RenderQueue::submit(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID)
{
    uint32_t idx = static_cast<uint32_t>(m_transforms.size());
    m_transforms.push_back(transform);
    m_draws.push_back({ mesh, idx, materialID });
}

void S_RenderQueue::clear()
{
    m_draws.clear();
    m_transforms.clear();
}
