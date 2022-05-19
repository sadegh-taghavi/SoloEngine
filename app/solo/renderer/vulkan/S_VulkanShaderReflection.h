#pragma once
#include <stdint.h>
#include "solo/renderer/S_Shader.h"
namespace solo
{

#define S_VulkanShaderReflectionMaxNameSize 64

struct S_VulkanShaderReflection
{
    S_ShaderStage Stage;
    uint32_t NumberOfUniformBuffers;
    uint32_t MaxUniformBuffersSet;
    uint32_t NumberOfTextures;
    uint32_t MaxTextureSet;
    char EntryPointName[S_VulkanShaderReflectionMaxNameSize];
};

struct S_VulkanShaderReflectionUniformBuffer
{
    uint32_t BlockSize;
    uint32_t ArraySize;
    uint32_t Offset;
    uint32_t Binding;
    uint32_t Set;
    char Name[S_VulkanShaderReflectionMaxNameSize];
};

struct S_VulkanShaderReflectionTexture
{
    uint32_t ArraySize;
    uint32_t Offset;
    uint32_t Binding;
    uint32_t Set;
    char Name[S_VulkanShaderReflectionMaxNameSize];
};

}

