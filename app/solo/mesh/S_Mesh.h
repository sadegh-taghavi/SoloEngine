#pragma once
#include <vector>
#include <cstdint>
#include "S_MeshBin.h"

namespace solo
{

class S_Mesh
{
public:
    S_Mesh();
    virtual ~S_Mesh();

    uint32_t                         vertexCount()    const;
    uint32_t                         indexCount()     const;
    uint32_t                         primitiveCount() const;
    const std::vector<MeshBinPrimitive>& primitives() const;
    bool                             isSkinned()      const;

protected:
    uint32_t                      m_vertexCount    = 0;
    uint32_t                      m_indexCount     = 0;
    uint32_t                      m_flags          = 0;
    std::vector<MeshBinPrimitive> m_primitives;
};

}
