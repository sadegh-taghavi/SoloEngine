#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstddef>

namespace solo
{

class S_VulkanRendererAPI;

class S_VulkanPerFrame
{
public:
    S_VulkanPerFrame(S_VulkanRendererAPI* api, uint32_t frameCount, size_t dataSize);
    ~S_VulkanPerFrame();

    void            update(const void* data, uint32_t frameIndex);
    void            bind(VkCommandBuffer cmd, uint32_t frameIndex) const;
    VkDescriptorSet set(uint32_t frameIndex) const { return m_sets[frameIndex]; }

    VkDescriptorSetLayout setLayout() const { return m_setLayout; }

private:
    static constexpr uint32_t kMaxFrames = 4;

    S_VulkanRendererAPI* m_api;
    uint32_t             m_frameCount;
    size_t               m_dataSize;

    VkBuffer              m_buffers[kMaxFrames]  = {};
    VmaAllocation         m_allocs[kMaxFrames]   = {};
    void*                 m_mapped[kMaxFrames]   = {};

    VkDescriptorPool      m_pool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_layout    = VK_NULL_HANDLE;
    VkDescriptorSet       m_sets[kMaxFrames] = {};
};

}
