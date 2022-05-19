#pragma once

#include "solo/renderer/S_TextureSampler.h"
#include "solo/stl/S_Vector.h"
#include "solo/renderer/vulkan/S_VulkanDeviceAllocator.h"
#include <vulkan/vulkan.h>

namespace solo
{

class S_VulkanRendererAPI;

class S_VulkanTextureSampler: public S_TextureSampler
{
    friend class S_VulkanTextureManager;
public:
    S_VulkanTextureSampler(S_VulkanRendererAPI *api, const S_TextureSamplerDescriptor &descriptor);
    virtual ~S_VulkanTextureSampler();
    VkSampler sampler() const;

private:
    S_VulkanRendererAPI *m_api;
    VkSampler m_sampler;
};

}

