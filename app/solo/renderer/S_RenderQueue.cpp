#include "S_RenderQueue.h"

using namespace solo;

void S_RenderQueue::submit(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID,
                           const glm::mat4* palette, uint32_t paletteJointCount)
{
    uint32_t idx = static_cast<uint32_t>(m_transforms.size());
    m_transforms.push_back(transform);

    uint32_t paletteOffset = S_NO_PALETTE;
    if (palette && paletteJointCount)
    {
        paletteOffset = static_cast<uint32_t>(m_palettes.size());
        m_palettes.insert(m_palettes.end(), palette, palette + paletteJointCount);
    }

    m_draws.push_back({ mesh, idx, materialID, paletteOffset });
}

void S_RenderQueue::clear()
{
    m_draws.clear();
    m_transforms.clear();
    m_palettes.clear();
}
