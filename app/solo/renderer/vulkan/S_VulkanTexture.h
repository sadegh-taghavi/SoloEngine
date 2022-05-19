#pragma once

#include "solo/renderer/S_Texture.h"
#include "solo/stl/S_Vector.h"
#include "solo/renderer/vulkan/S_VulkanDeviceAllocator.h"
#include <vulkan/vulkan.h>

namespace solo
{

class S_VulkanRendererAPI;

class S_VulkanTexture: public S_Texture
{
public:
    S_VulkanTexture(S_VulkanRendererAPI *api, const S_String &texture);
    virtual ~S_VulkanTexture();
    VkImage image() const;
    VkImageView view() const;
    VkImageLayout layout() const;

private:
    VkImageLayout m_layout;
    S_VulkanRendererAPI *m_api;
    VkImage m_image;
    VkImageView m_view;
};

}

