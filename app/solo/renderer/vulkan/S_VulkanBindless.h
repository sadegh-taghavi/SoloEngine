#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <vector>
#include "solo/math/S_Math.h"
#include "solo/renderer/S_Renderer.h"

namespace solo
{

class S_VulkanRendererAPI;
class S_Texture;

class S_VulkanBindless
{
public:
    static constexpr uint32_t kMaxInstances = 4096;
    static constexpr uint32_t kMaxJoints    = 4096; // palette mat4 capacity per frame
    static constexpr uint32_t kMaxFrames    = 4;
    static constexpr uint32_t kMaxMaterials = 256;
    static constexpr uint32_t kMaxTextures  = 64;

    S_VulkanBindless(S_VulkanRendererAPI* api, uint32_t frameCount);
    ~S_VulkanBindless();

    void uploadTransforms(const glm::mat4* transforms, uint32_t count, uint32_t frameIndex);
    void uploadPalettes(const glm::mat4* palettes, uint32_t count, uint32_t frameIndex);
    void setTlas(const VkAccelerationStructureKHR* tlasPerFrame); // one-time write of binding 2
    void setRtShadeBuffers(const VkBuffer* buffersPerFrame);      // one-time write of binding 5

    // material table + texture array; descriptors refresh lazily per frame slot
    void setMaterials(const std::vector<S_MaterialRecord>& records,
                      const std::vector<S_Texture*>& textures);

    void bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex);

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

    VkBuffer      m_materialBuffers[kMaxFrames] = {};
    VmaAllocation m_materialAllocs[kMaxFrames]  = {};
    void*         m_materialMapped[kMaxFrames]  = {};
    VkSampler     m_textureSampler              = VK_NULL_HANDLE;

    std::vector<S_MaterialRecord> m_materialData;
    std::vector<S_Texture*>       m_textureData;
    uint64_t m_materialVersion = 0;
    uint64_t m_setVersion[kMaxFrames] = {};
};

}
