#include "S_VulkanVertexBuffer.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"

using namespace solo;

S_VulkanVertexBuffer::S_VulkanVertexBuffer(S_VulkanRendererAPI *api, uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                           std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                           std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray):
    S_VertexBuffer( verticesCount, indicesCount, instancesCount, std::move( verticesDescriptorArray ), std::move( instancesDescriptorArray ) ), m_api(api)

{
    m_verticesBufferSize = (m_verticesDescriptorArray->stride() * verticesCount + 3) & ~uint64_t(3);
    m_indicesBufferSize = (sizeof(uint32_t) * indicesCount + 3) & ~uint64_t(3);
    m_instancesBufferSize = (m_instancesDescriptorArray->stride() * instancesCount + 3) & ~uint64_t(3);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_verticesBufferSize + m_indicesBufferSize + m_instancesBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocInfo;
    VK_RESULT_CHECK( vmaCreateBuffer( m_api->vmaAllocator(), &bufferInfo, &allocCreateInfo, &m_verticesStagingBuffer, &m_verticesStagingAllocation, &allocInfo ) )

    m_vertices = allocInfo.pMappedData;
    m_indices = reinterpret_cast<void *>( reinterpret_cast<uint64_t>( m_vertices ) + m_verticesBufferSize );
    m_instances = reinterpret_cast<void *>( reinterpret_cast<uint64_t>( m_indices ) + m_indicesBufferSize );
}

S_VulkanVertexBuffer::~S_VulkanVertexBuffer()
{
    vmaDestroyBuffer( m_api->vmaAllocator(), m_verticesStagingBuffer, m_verticesStagingAllocation );
}

std::pair<void *, void *> S_VulkanVertexBuffer::beginVerticesData()
{
    return std::make_pair( m_vertices, m_indices );
}

void S_VulkanVertexBuffer::endVerticesData()
{
    VK_RESULT_CHECK( vmaFlushAllocation( m_api->vmaAllocator(), m_verticesStagingAllocation, 0, m_verticesBufferSize + m_indicesBufferSize ) )
}

void *S_VulkanVertexBuffer::beginInstancesData()
{
    return m_instances;
}

void S_VulkanVertexBuffer::endInstancesData()
{
    VK_RESULT_CHECK( vmaFlushAllocation( m_api->vmaAllocator(), m_verticesStagingAllocation, m_verticesBufferSize + m_indicesBufferSize, m_instancesBufferSize ) )
}

void S_VulkanVertexBuffer::draw()
{
    VkBuffer vertexBuffers[] = {m_verticesStagingBuffer, m_verticesStagingBuffer};
    VkDeviceSize vbOffsets[] = {0, m_verticesBufferSize + m_indicesBufferSize};

    vkCmdBindVertexBuffers( m_api->nextFrameRenderCommandBuffer(), 0, 2, vertexBuffers, vbOffsets);
    vkCmdBindIndexBuffer( m_api->nextFrameRenderCommandBuffer(), m_verticesStagingBuffer, m_verticesBufferSize, VK_INDEX_TYPE_UINT32 );
    vkCmdDrawIndexed( m_api->nextFrameRenderCommandBuffer(), m_drawIndicesCount, m_drawInstancesCount, 0, 0, 0 );
}
