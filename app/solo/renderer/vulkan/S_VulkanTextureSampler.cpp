#include "S_VulkanTextureSampler.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include "solo/resource/S_ResourceManager.h"
#include "solo/application/S_Application.h"
#include "solo/renderer/vulkan/S_VulkanAllocator.h"

using namespace solo;

S_VulkanTextureSampler::S_VulkanTextureSampler(S_VulkanRendererAPI *api, const S_TextureSamplerDescriptor &descriptor) : S_TextureSampler(), m_api( api )

{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = static_cast<VkFilter>(descriptor.MagFilter);
    samplerInfo.minFilter = static_cast<VkFilter>(descriptor.MinFilter);
    samplerInfo.mipmapMode = static_cast<VkSamplerMipmapMode>(descriptor.MipmapMode);
    samplerInfo.mipLodBias = descriptor.MipLodBias;
    samplerInfo.minLod = descriptor.MinLod;
    samplerInfo.maxLod = descriptor.MaxLod;
    samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(descriptor.AddressModeU);
    samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(descriptor.AddressModeV);
    samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(descriptor.AddressModeW);
    samplerInfo.anisotropyEnable = descriptor.AnisotropyEnable;
    samplerInfo.maxAnisotropy = descriptor.MaxAnisotropy;
    samplerInfo.borderColor = static_cast<VkBorderColor>(descriptor.BorderColor);
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    VK_RESULT_CHECK( vkCreateSampler(m_api->device(), &samplerInfo, S_VulkanAllocator(), &m_sampler) );
}

S_VulkanTextureSampler::~S_VulkanTextureSampler()
{
    vkDestroySampler( m_api->device(), m_sampler, S_VulkanAllocator() );
}

VkSampler S_VulkanTextureSampler::sampler() const
{
    return m_sampler;
}
