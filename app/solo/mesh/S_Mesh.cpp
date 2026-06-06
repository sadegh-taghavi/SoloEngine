#include "S_Mesh.h"

using namespace solo;

S_Mesh::S_Mesh() {}
S_Mesh::~S_Mesh() {}

uint32_t S_Mesh::vertexCount()    const { return m_vertexCount; }
uint32_t S_Mesh::indexCount()     const { return m_indexCount; }
uint32_t S_Mesh::primitiveCount() const { return static_cast<uint32_t>(m_primitives.size()); }
const std::vector<MeshBinPrimitive>& S_Mesh::primitives() const { return m_primitives; }
bool S_Mesh::isSkinned()          const { return (m_flags & MESH_BIN_FLAG_SKINNED) != 0; }
