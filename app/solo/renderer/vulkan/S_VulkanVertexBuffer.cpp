#include "S_VulkanVertexBuffer.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"

using namespace solo;

S_VulkanVertexBuffer::S_VulkanVertexBuffer(S_VulkanRendererAPI *api, uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                           std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                           std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray):
    S_VertexBuffer( verticesCount, indicesCount, instancesCount, std::move( verticesDescriptorArray ), std::move( instancesDescriptorArray ) ), m_api(api)

{
    m_verticesBufferSize = m_api->deviceAllocator()->getAlign( m_verticesDescriptorArray->stride() * verticesCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
    m_indicesBufferSize = m_api->deviceAllocator()->getAlign( sizeof(uint32_t) * indicesCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
    m_instancesBufferSize = m_api->deviceAllocator()->getAlign( m_instancesDescriptorArray->stride() * instancesCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
    m_api->deviceAllocator()->createBuffer( m_verticesBufferSize + m_indicesBufferSize + m_instancesBufferSize,
                                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_verticesStagingBuffer, m_verticesStagingMemory );
    //    m_api->deviceAllocator()->createBuffer( vbSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    //                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_verticesBuffer, m_verticesMemory );

    m_vertices = m_verticesStagingMemory.MappedPtr;
    m_indices = reinterpret_cast<void *>(reinterpret_cast<uint64_t>( m_vertices ) + m_verticesBufferSize );
    m_instances = reinterpret_cast<void *>(reinterpret_cast<uint64_t>( m_indices ) + m_indicesBufferSize );

    m_verticesMemoryRanges.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    m_verticesMemoryRanges.size = m_verticesBufferSize + m_indicesBufferSize;
    m_verticesMemoryRanges.offset = m_verticesStagingMemory.OffsetFromBufferBase;
    m_verticesMemoryRanges.pNext = nullptr;
    m_verticesMemoryRanges.memory = m_verticesStagingMemory.Memory;

    m_instancesMemoryRanges.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    m_instancesMemoryRanges.size = m_instancesBufferSize;
    m_instancesMemoryRanges.offset = m_verticesStagingMemory.OffsetFromBufferBase + m_verticesMemoryRanges.size;
    m_instancesMemoryRanges.pNext = nullptr;
    m_instancesMemoryRanges.memory = m_verticesStagingMemory.Memory;
}

S_VulkanVertexBuffer::~S_VulkanVertexBuffer()
{
    m_api->deviceAllocator()->destroy( m_verticesStagingBuffer );
}

std::pair<void *, void *> S_VulkanVertexBuffer::beginVerticesData()
{
    return std::make_pair( m_vertices, m_indices );
}

void S_VulkanVertexBuffer::endVerticesData()
{
    VK_RESULT_CHECK( vkFlushMappedMemoryRanges( m_api->device(), 1, &m_verticesMemoryRanges ) );
}

void *S_VulkanVertexBuffer::beginInstancesData()
{
    return m_instances;
}

void S_VulkanVertexBuffer::endInstancesData()
{
    VK_RESULT_CHECK( vkFlushMappedMemoryRanges( m_api->device(), 1, &m_instancesMemoryRanges ) );
}

void S_VulkanVertexBuffer::draw()
{
    VkBuffer vertexBuffers[] = {m_verticesStagingBuffer, m_verticesStagingBuffer};
    VkDeviceSize vbOffsets[] = {0, m_verticesBufferSize + m_indicesBufferSize};

    vkCmdBindVertexBuffers( m_api->nextFrameRenderCommandBuffer(), 0, 2, vertexBuffers, vbOffsets);
    vkCmdBindIndexBuffer( m_api->nextFrameRenderCommandBuffer(), m_verticesStagingBuffer, m_verticesBufferSize, VK_INDEX_TYPE_UINT32 );
    vkCmdDrawIndexed( m_api->nextFrameRenderCommandBuffer(), m_drawIndicesCount, m_drawInstancesCount, 0, 0, 0 );
}
