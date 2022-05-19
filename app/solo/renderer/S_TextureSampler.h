#pragma once
#include <stdint.h>

namespace solo
{

enum class S_SamplerFilter
{
    NEAREST = 0,
    LINEAR = 1,
};

enum class S_SamplerMipmapMode
{
    NEAREST = 0,
    LINEAR = 1,
};

enum class S_SamplerAddressMode
{
    REPEAT = 0,
    MIRRORED_REPEAT = 1,
    CLAMP_TO_EDGE = 2,
    CLAMP_TO_BORDER = 3,
    MIRROR_CLAMP_TO_EDGE = 4,
};

enum class S_SamplerBorderColor
{
    FLOAT_TRANSPARENT_BLACK = 0,
    INT_TRANSPARENT_BLACK = 1,
    FLOAT_OPAQUE_BLACK = 2,
    INT_OPAQUE_BLACK = 3,
    FLOAT_OPAQUE_WHITE = 4,
    INT_OPAQUE_WHITE = 5,
};

struct S_TextureSamplerDescriptor
{
    S_SamplerFilter MagFilter;
    S_SamplerFilter MinFilter;
    S_SamplerMipmapMode MipmapMode;
    S_SamplerAddressMode AddressModeU;
    S_SamplerAddressMode AddressModeV;
    S_SamplerAddressMode AddressModeW;
    bool AnisotropyEnable;
    uint32_t MaxAnisotropy;
    S_SamplerBorderColor BorderColor;
    float MipLodBias;
    float MinLod;
    float MaxLod;

    S_TextureSamplerDescriptor()
    {
        MagFilter = S_SamplerFilter::LINEAR;
        MinFilter = S_SamplerFilter::LINEAR;
        MipmapMode = S_SamplerMipmapMode::LINEAR;
        AddressModeU = S_SamplerAddressMode::REPEAT;
        AddressModeV = S_SamplerAddressMode::REPEAT;
        AddressModeW = S_SamplerAddressMode::REPEAT;
        AnisotropyEnable = false;
        MaxAnisotropy = 16;
        BorderColor = S_SamplerBorderColor::FLOAT_TRANSPARENT_BLACK;
        MipLodBias = 0.0f;
        MinLod = 0.0f;
        MaxLod = 10.0f;
    }
};

class S_TextureSampler
{
public:
    S_TextureSampler();
    virtual ~S_TextureSampler();
private:

};

}

