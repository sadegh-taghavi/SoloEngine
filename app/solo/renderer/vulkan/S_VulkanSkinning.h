#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <map>
#include <cstdint>
#include "solo/math/S_Math.h"

namespace solo
{

class S_VulkanRendererAPI;
class S_VulkanMesh;

// Compute pre-skinning for ray tracing: runs the palette skinning of
// skinned.vert in a compute pass, writing mesh-space skinned positions into a
// per-(mesh, frame) buffer that the dynamic BLAS rebuild consumes.
class S_VulkanSkinning
{
public:
    S_VulkanSkinning(S_VulkanRendererAPI* api, uint32_t frameCount);
    ~S_VulkanSkinning();

    bool available() const { return m_available; }

    void beginFrame(uint32_t frameIndex); // resets the frame's palette cursor

    // records the skinning dispatch; returns the skinned position buffer
    // (VK_NULL_HANDLE on failure/capacity)
    VkBuffer dispatch(VkCommandBuffer cmd, S_VulkanMesh* mesh,
                      const glm::mat4* palette, uint32_t jointCount, uint32_t frameIndex);

    // compute writes -> acceleration structure build reads
    void barrierToAsBuild(VkCommandBuffer cmd);

private:
    static constexpr uint32_t kMaxFrames        = 4;
    static constexpr uint32_t kMaxPaletteJoints = 1024; // per frame, across all skinned draws

    struct MeshFrame
    {
        VkBuffer        dst      = VK_NULL_HANDLE;
        VmaAllocation   dstAlloc = VK_NULL_HANDLE;
        VkDescriptorSet set      = VK_NULL_HANDLE;
    };

    MeshFrame& meshFrame(S_VulkanMesh* mesh, uint32_t frameIndex);

    S_VulkanRendererAPI* m_api;
    uint32_t             m_frameCount;
    bool                 m_available = false;

    VkShaderModule        m_module    = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_layout    = VK_NULL_HANDLE;
    VkPipeline            m_pipeline  = VK_NULL_HANDLE;
    VkDescriptorPool      m_pool      = VK_NULL_HANDLE;

    VkBuffer      m_paletteBuffers[kMaxFrames] = {};
    VmaAllocation m_paletteAllocs[kMaxFrames]  = {};
    void*         m_paletteMapped[kMaxFrames]  = {};
    uint32_t      m_paletteCursor[kMaxFrames]  = {};

    std::map<std::pair<S_VulkanMesh*, uint32_t>, MeshFrame> m_meshFrames;
};

}
