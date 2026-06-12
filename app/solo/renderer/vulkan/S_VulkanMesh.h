#pragma once
#include "solo/mesh/S_Mesh.h"
#include "S_VulkanRT.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <string>

namespace solo
{

class S_VulkanRendererAPI;

class S_VulkanMesh : public S_Mesh
{
public:
    S_VulkanMesh(S_VulkanRendererAPI* api, const std::string& path);
    ~S_VulkanMesh();

    VkBuffer indexBuffer()        const { return m_indexBuffer; }
    VkBuffer positionBuffer()     const { return m_positionBuffer; }
    VkBuffer rasterAttribBuffer() const { return m_rasterAttribBuffer; }
    VkBuffer rtHitDataBuffer()    const { return m_rtHitDataBuffer; }
    VkBuffer skinBuffer()         const { return m_skinBuffer; }
    void     bindBuffers(VkCommandBuffer cmd);
    void     draw()               override;
    uint64_t blasAddress()        const override { return m_blas.address; }
    uint64_t indexAddress()       const override { return m_indexAddress; }
    uint64_t hitDataAddress()     const override { return m_hitDataAddress; }

private:
    static VkBuffer uploadBuffer(S_VulkanRendererAPI* api, const void* data, VkDeviceSize size,
                                 VkBufferUsageFlags usage, VmaAllocation& outAlloc);

    S_VulkanRendererAPI* m_api;

    VkBuffer      m_indexBuffer        = VK_NULL_HANDLE;
    VmaAllocation m_indexAlloc         = VK_NULL_HANDLE;
    VkBuffer      m_positionBuffer     = VK_NULL_HANDLE;
    VmaAllocation m_positionAlloc      = VK_NULL_HANDLE;
    VkBuffer      m_rasterAttribBuffer = VK_NULL_HANDLE;
    VmaAllocation m_rasterAttribAlloc  = VK_NULL_HANDLE;
    VkBuffer      m_rtHitDataBuffer    = VK_NULL_HANDLE;
    VmaAllocation m_rtHitDataAlloc     = VK_NULL_HANDLE;
    VkBuffer      m_skinBuffer         = VK_NULL_HANDLE;
    VmaAllocation m_skinAlloc          = VK_NULL_HANDLE;
    S_VulkanRT::Blas m_blas;
    uint64_t         m_indexAddress   = 0;
    uint64_t         m_hitDataAddress = 0;
};

}
