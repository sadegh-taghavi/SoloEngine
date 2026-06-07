#pragma once
#include <vector>
#include "solo/renderer/S_Handle.h"
#include "solo/math/S_Math.h"

namespace solo
{

struct S_DrawCall
{
    S_MeshHandle mesh;
    uint32_t     instanceIndex;
    uint32_t     materialID;
};

class S_RenderQueue
{
public:
    void submit(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID = 0);
    void clear();
    bool empty() const { return m_draws.empty(); }

    const std::vector<S_DrawCall>&  draws()      const { return m_draws; }
    const std::vector<glm::mat4>&   transforms() const { return m_transforms; }

private:
    std::vector<S_DrawCall>  m_draws;
    std::vector<glm::mat4>   m_transforms;
};

}
