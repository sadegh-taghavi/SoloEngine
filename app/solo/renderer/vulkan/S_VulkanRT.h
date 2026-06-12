#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <map>
#include <utility>
#include <cstdint>
#include "solo/math/S_Math.h"

namespace solo
{

class S_VulkanRendererAPI;

// Ray tracing acceleration structures (VK_KHR_acceleration_structure + ray_query).
// One BLAS per mesh built at load; one TLAS per frame-in-flight rebuilt every
// frame (pre render pass) from the previous frame's static draw list. The TLAS
// handles are stable (created at max capacity), so descriptor writes happen once.
class S_VulkanRT
{
public:
    static constexpr uint32_t kMaxRtInstances = 1024;

    struct Blas
    {
        VkAccelerationStructureKHR as      = VK_NULL_HANDLE;
        VkBuffer                   buffer  = VK_NULL_HANDLE;
        VmaAllocation              alloc   = VK_NULL_HANDLE;
        VkDeviceAddress            address = 0;
    };

    struct Instance
    {
        VkDeviceAddress blasAddress = 0;
        glm::mat4       transform   = glm::mat4(1.0f);
        // hit-shading data, written to the shade SSBO at the instance's custom index
        uint64_t indexAddress   = 0;
        uint64_t hitDataAddress = 0;
        uint32_t materialBase   = 0; // first global material ID, or the fallback ID
        uint32_t useLocalMaterial = 0;
    };

    // GPU layout of the per-instance shade record (std430, 32 bytes)
    struct ShadeRecord
    {
        uint64_t indexAddress;
        uint64_t hitDataAddress;
        uint32_t materialBase;
        uint32_t useLocalMaterial;
        uint32_t pad0, pad1;
    };

    // skinned draw captured for the next frame's pre-pass (compute skin + BLAS rebuild)
    struct SkinnedInstance
    {
        class S_Mesh*          mesh = nullptr;
        glm::mat4              transform = glm::mat4(1.0f);
        std::vector<glm::mat4> palette;
        uint64_t indexAddress     = 0;
        uint64_t hitDataAddress   = 0;
        uint32_t materialBase     = 0;
        uint32_t useLocalMaterial = 0;
    };

    S_VulkanRT(S_VulkanRendererAPI* api, uint32_t frameCount);
    ~S_VulkanRT();

    bool available() const { return m_available; }

    Blas buildBlas(VkBuffer positions, uint32_t vertexCount, VkBuffer indices, uint32_t indexCount);
    void destroyBlas(Blas& blas);

    // BLAS for animated geometry: persistent per (key, frame), rebuild recorded
    // into the frame command buffer from compute-skinned positions
    VkDeviceAddress buildDynamicBlas(VkCommandBuffer cmd, const void* key,
                                     VkBuffer skinnedPositions, uint32_t vertexCount,
                                     VkBuffer indices, uint32_t indexCount, uint32_t frameIndex);
    void barrierBlasToTlas(VkCommandBuffer cmd);

    // records the TLAS build into cmd; must be outside a render pass
    void buildTlas(VkCommandBuffer cmd, const std::vector<Instance>& instances, uint32_t frameIndex);

    VkAccelerationStructureKHR tlas(uint32_t frameIndex) const { return m_tlas[frameIndex].as; }
    VkBuffer shadeBuffer(uint32_t frameIndex) const { return m_tlas[frameIndex].shadeBuffer; }

private:
    static constexpr uint32_t kMaxFrames = 4;

    struct TlasFrame
    {
        VkAccelerationStructureKHR as            = VK_NULL_HANDLE;
        VkBuffer       buffer                    = VK_NULL_HANDLE;
        VmaAllocation  alloc                     = VK_NULL_HANDLE;
        VkBuffer       instanceBuffer            = VK_NULL_HANDLE;
        VmaAllocation  instanceAlloc             = VK_NULL_HANDLE;
        void*          instanceMapped            = nullptr;
        VkDeviceAddress instanceAddress          = 0;
        VkBuffer       scratch                   = VK_NULL_HANDLE;
        VmaAllocation  scratchAlloc              = VK_NULL_HANDLE;
        VkDeviceAddress scratchAddress           = 0;
        VkBuffer       shadeBuffer               = VK_NULL_HANDLE;
        VmaAllocation  shadeAlloc                = VK_NULL_HANDLE;
        void*          shadeMapped               = nullptr;
    };

    VkDeviceAddress bufferAddress(VkBuffer buffer) const;

    struct DynBlas
    {
        VkAccelerationStructureKHR as      = VK_NULL_HANDLE;
        VkBuffer        buffer             = VK_NULL_HANDLE;
        VmaAllocation   alloc              = VK_NULL_HANDLE;
        VkBuffer        scratch            = VK_NULL_HANDLE;
        VmaAllocation   scratchAlloc       = VK_NULL_HANDLE;
        VkDeviceAddress scratchAddress     = 0;
        VkDeviceAddress address            = 0;
    };

    std::map<std::pair<const void*, uint32_t>, DynBlas> m_dynBlas;

    S_VulkanRendererAPI* m_api;
    uint32_t             m_frameCount;
    bool                 m_available = false;

    VkCommandPool m_buildPool = VK_NULL_HANDLE; // graphics-family pool for BLAS builds

    TlasFrame m_tlas[kMaxFrames];

    // extension entry points
    PFN_vkCreateAccelerationStructureKHR        m_createAS        = nullptr;
    PFN_vkDestroyAccelerationStructureKHR       m_destroyAS       = nullptr;
    PFN_vkGetAccelerationStructureBuildSizesKHR m_getBuildSizes   = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR m_getASAddress = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR     m_cmdBuildAS      = nullptr;
};

}
