#pragma once
#include <cstdint>
#include <limits>

namespace solo
{

template<uint32_t Tag>
struct S_Handle
{
    static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();
    uint32_t index      = INVALID;
    uint32_t generation = 0;
    bool valid() const { return index != INVALID; }
};

using S_TextureHandle      = S_Handle<0>;
using S_ShaderHandle       = S_Handle<1>;
using S_VertexBufferHandle = S_Handle<2>;
using S_SamplerHandle      = S_Handle<3>;
using S_MeshHandle         = S_Handle<4>;

}
