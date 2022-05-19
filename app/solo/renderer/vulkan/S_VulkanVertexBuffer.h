#pragma once

#include "solo/renderer/S_VertexBuffer.h"
#include "solo/renderer/vulkan/S_VulkanDeviceAllocator.h"
#include <vulkan/vulkan.h>

namespace solo
{

class S_VulkanRendererAPI;
class S_VulkanVertexBuffer: public S_VertexBuffer
{
public:
    S_VulkanVertexBuffer(S_VulkanRendererAPI *api, uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                   std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray, std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray);
    virtual ~S_VulkanVertexBuffer();
    std::pair<void *, void *> beginVerticesData();
    void endVerticesData();
    void * beginInstancesData();
    void endInstancesData();
    void draw();
private:
    S_VulkanRendererAPI *m_api;
    VkBuffer m_verticesStagingBuffer;
    S_VulkanDeviceMemory m_verticesStagingMemory;
    S_VulkanDeviceMemory m_verticesMemory;
    VkMappedMemoryRange m_verticesMemoryRanges;
    VkMappedMemoryRange m_instancesMemoryRanges;
    void *m_vertices;
    void *m_indices;
    void *m_instances;
    uint64_t m_verticesBufferSize;
    uint64_t m_indicesBufferSize;
    uint64_t m_instancesBufferSize;
};

}

