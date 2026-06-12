#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include "solo/math/S_Math.h"

namespace solo
{

class S_VulkanRendererAPI;

class S_VulkanBindless
{
public:
    static constexpr uint32_t kMaxInstances = 4096;
    static constexpr uint32_t kMaxJoints    = 4096; // palette mat4 capacity per frame
    static constexpr uint32_t kMaxFrames    = 4;

    S_VulkanBindless(S_VulkanRendererAPI* api, uint32_t frameCount);
    ~S_VulkanBindless();

    void uploadTransforms(const glm::mat4* transforms, uint32_t count, uint32_t frameIndex);
    void uploadPalettes(const glm::mat4* palettes, uint32_t count, uint32_t frameIndex);
    void setTlas(const VkAccelerationStructureKHR* tlasPerFrame); // one-time write of binding 2
    void bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex) const;

    VkDescriptorSetLayout setLayout() const { return m_setLayout; }

private:
    S_VulkanRendererAPI* m_api;
    uint32_t             m_frameCount;

    VkBuffer              m_transformBuffers[kMaxFrames] = {};
    VmaAllocation         m_transformAllocs[kMaxFrames]  = {};
    void*                 m_transformMapped[kMaxFrames]  = {};

    VkBuffer              m_paletteBuffers[kMaxFrames] = {};
    VmaAllocation         m_paletteAllocs[kMaxFrames]  = {};
    void*                 m_paletteMapped[kMaxFrames]  = {};

    VkDescriptorPool      m_pool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
    VkDescriptorSet       m_sets[kMaxFrames] = {};
};

}
