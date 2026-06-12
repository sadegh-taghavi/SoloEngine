#pragma once
#include <vector>
#include <cstdint>
#include "solo/renderer/S_Handle.h"
#include "solo/math/S_Math.h"

namespace solo
{

static constexpr uint32_t S_NO_PALETTE = 0xFFFFFFFFu;

struct S_DrawCall
{
    S_MeshHandle mesh;
    uint32_t     instanceIndex;
    uint32_t     materialID;
    uint32_t     paletteOffset; // index into palettes(), S_NO_PALETTE for static draws
};

class S_RenderQueue
{
public:
    void submit(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID = 0,
                const glm::mat4* palette = nullptr, uint32_t paletteJointCount = 0);
    void clear();
    bool empty() const { return m_draws.empty(); }

    const std::vector<S_DrawCall>&  draws()      const { return m_draws; }
    const std::vector<glm::mat4>&   transforms() const { return m_transforms; }
    const std::vector<glm::mat4>&   palettes()   const { return m_palettes; }

private:
    std::vector<S_DrawCall>  m_draws;
    std::vector<glm::mat4>   m_transforms;
    std::vector<glm::mat4>   m_palettes;
};

}
