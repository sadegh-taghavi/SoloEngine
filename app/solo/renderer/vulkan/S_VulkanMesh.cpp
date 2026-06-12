#include "S_VulkanMesh.h"
#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include "solo/application/S_Application.h"
#include "solo/debug/S_Debug.h"
#include <cstring>

using namespace solo;

VkBuffer S_VulkanMesh::uploadBuffer(S_VulkanRendererAPI* api, const void* data, VkDeviceSize size,
                                    VkBufferUsageFlags usage, VmaAllocation& outAlloc)
{
    VkBuffer      stageBuf;
    VmaAllocation stageAlloc;

    VkBufferCreateInfo stageBufInfo = {};
    stageBufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stageBufInfo.size        = size;
    stageBufInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stageBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stageAllocCI = {};
    stageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
    stageAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stageAllocInfo;
    VK_RESULT_CHECK( vmaCreateBuffer(api->vmaAllocator(), &stageBufInfo, &stageAllocCI,
                                     &stageBuf, &stageAlloc, &stageAllocInfo) )
    memcpy(stageAllocInfo.pMappedData, data, size);
    VK_RESULT_CHECK( vmaFlushAllocation(api->vmaAllocator(), stageAlloc, 0, VK_WHOLE_SIZE) )

    VkBuffer gpuBuf;

    VkBufferCreateInfo gpuBufInfo = {};
    gpuBufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    gpuBufInfo.size        = size;
    gpuBufInfo.usage       = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    gpuBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo gpuAllocCI = {};
    gpuAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

    VK_RESULT_CHECK( vmaCreateBuffer(api->vmaAllocator(), &gpuBufInfo, &gpuAllocCI,
                                     &gpuBuf, &outAlloc, nullptr) )

    VkCommandBuffer cmd = api->beginSingleTimeTransferCommands();
    VkBufferCopy region = { 0, 0, size };
    vkCmdCopyBuffer(cmd, stageBuf, gpuBuf, 1, &region);
    api->endSingleTimeTransferCommands(cmd);

    vmaDestroyBuffer(api->vmaAllocator(), stageBuf, stageAlloc);
    return gpuBuf;
}

S_VulkanMesh::S_VulkanMesh(S_VulkanRendererAPI* api, const std::string& path)
    : S_Mesh(), m_api(api)
{
    auto fileData = S_Application::executingApplication()->pack()->load(path);
    if (fileData.empty())
    {
        s_debugLayer("S_VulkanMesh: not found in pack!", path);
        return;
    }

    if (fileData.size() < sizeof(MeshBinHeader))
    {
        s_debugLayer("S_VulkanMesh: truncated header", path);
        return;
    }

    MeshBinHeader header;
    memcpy(&header, fileData.data(), sizeof(header));

    if (header.magic != MESH_BIN_MAGIC)
    {
        s_debugLayer("S_VulkanMesh: invalid magic", path);
        return;
    }

    if (header.version != MESH_BIN_VERSION)
    {
        s_debugLayer("S_VulkanMesh: unsupported version", path, header.version);
        return;
    }

    m_vertexCount = header.vertexCount;
    m_indexCount  = header.indexCount;
    m_flags       = header.flags;

    m_primitives.resize(header.primitiveCount);
    memcpy(m_primitives.data(), fileData.data() + header.primitiveOffset,
           header.primitiveCount * sizeof(MeshBinPrimitive));

    const uint8_t* d = fileData.data();

    constexpr VkBufferUsageFlags kAsInputUsage =
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    m_indexBuffer = uploadBuffer(api, d + header.indexOffset,
        header.indexCount * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | kAsInputUsage,
        m_indexAlloc);

    m_positionBuffer = uploadBuffer(api, d + header.positionOffset,
        header.vertexCount * sizeof(MeshBinPosition),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | kAsInputUsage,
        m_positionAlloc);

    m_rasterAttribBuffer = uploadBuffer(api, d + header.rasterAttribOffset,
        header.vertexCount * sizeof(MeshBinRasterAttrib),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        m_rasterAttribAlloc);

    m_rtHitDataBuffer = uploadBuffer(api, d + header.rtHitDataOffset,
        header.vertexCount * sizeof(MeshBinRTHitData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        m_rtHitDataAlloc);

    if ((header.flags & MESH_BIN_FLAG_SKINNED) && header.skinOffset != 0)
    {
        m_skinBuffer = uploadBuffer(api, d + header.skinOffset,
            header.vertexCount * sizeof(MeshBinSkinVertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            m_skinAlloc);
    }

    if (header.materialCount && header.materialOffset)
    {
        m_pendingMaterials.resize(header.materialCount);
        memcpy(m_pendingMaterials.data(), d + header.materialOffset,
               header.materialCount * sizeof(MeshBinMaterial));
    }

    loadAnimationData(d, header);

    if (api->rt() && api->rt()->available())
    {
        m_blas = api->rt()->buildBlas(m_positionBuffer, header.vertexCount,
                                      m_indexBuffer, header.indexCount);

        VkBufferDeviceAddressInfo addrInfo{};
        addrInfo.sType   = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addrInfo.buffer  = m_indexBuffer;
        m_indexAddress   = vkGetBufferDeviceAddress(api->device(), &addrInfo);
        addrInfo.buffer  = m_rtHitDataBuffer;
        m_hitDataAddress = vkGetBufferDeviceAddress(api->device(), &addrInfo);
    }
}

void S_VulkanMesh::bindBuffers(VkCommandBuffer cmd)
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_positionBuffer, &offset);
    if (m_skinBuffer) // skinned layout: positions@0, skin@1, attribs@2
    {
        vkCmdBindVertexBuffers(cmd, 1, 1, &m_skinBuffer, &offset);
        vkCmdBindVertexBuffers(cmd, 2, 1, &m_rasterAttribBuffer, &offset);
    }
    else              // static layout: positions@0, attribs@1
        vkCmdBindVertexBuffers(cmd, 1, 1, &m_rasterAttribBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void S_VulkanMesh::draw()
{
    VkCommandBuffer cmd = m_api->nextFrameRenderCommandBuffer();
    bindBuffers(cmd);
    for (const auto& prim : m_primitives)
        vkCmdDrawIndexed(cmd, prim.indexCount, 1, prim.indexOffset,
                         static_cast<int32_t>(prim.vertexOffset), 0);
}

S_VulkanMesh::~S_VulkanMesh()
{
    auto vma = m_api->vmaAllocator();
    if (m_blas.as && m_api->rt()) m_api->rt()->destroyBlas(m_blas);
    if (m_indexBuffer)        vmaDestroyBuffer(vma, m_indexBuffer,        m_indexAlloc);
    if (m_positionBuffer)     vmaDestroyBuffer(vma, m_positionBuffer,     m_positionAlloc);
    if (m_rasterAttribBuffer) vmaDestroyBuffer(vma, m_rasterAttribBuffer, m_rasterAttribAlloc);
    if (m_rtHitDataBuffer)    vmaDestroyBuffer(vma, m_rtHitDataBuffer,    m_rtHitDataAlloc);
    if (m_skinBuffer)         vmaDestroyBuffer(vma, m_skinBuffer,         m_skinAlloc);
}
