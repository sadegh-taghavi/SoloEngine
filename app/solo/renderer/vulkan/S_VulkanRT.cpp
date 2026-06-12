#include "S_VulkanRT.h"
#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include "solo/mesh/S_MeshBin.h"
#include "solo/debug/S_Debug.h"
#include <cstring>

using namespace solo;

VkDeviceAddress S_VulkanRT::bufferAddress(VkBuffer buffer) const
{
    VkBufferDeviceAddressInfo info{};
    info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = buffer;
    return vkGetBufferDeviceAddress(m_api->device(), &info);
}

S_VulkanRT::S_VulkanRT(S_VulkanRendererAPI* api, uint32_t frameCount)
    : m_api(api), m_frameCount(frameCount)
{
    VkDevice device = m_api->device();

    m_createAS      = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    m_destroyAS     = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    m_getBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    m_getASAddress  = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    m_cmdBuildAS    = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));

    if (!m_createAS || !m_destroyAS || !m_getBuildSizes || !m_getASAddress || !m_cmdBuildAS)
    {
        s_debugLayer("S_VulkanRT: acceleration structure entry points unavailable — RT disabled");
        return;
    }

    VkCommandPoolCreateInfo poolCI{};
    poolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCI.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCI.queueFamilyIndex = m_api->graphicsQueueFamilyIndex();
    VK_RESULT_CHECK( vkCreateCommandPool(device, &poolCI, S_VulkanAllocator(), &m_buildPool) )

    // per-frame TLAS at fixed capacity so handles (and descriptor writes) stay stable
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geometry;

    uint32_t maxInstances = kMaxRtInstances;
    VkAccelerationStructureBuildSizesInfoKHR sizes{};
    sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    m_getBuildSizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &maxInstances, &sizes);

    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        TlasFrame& f = m_tlas[i];

        VkBufferCreateInfo bufCI{};
        bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo gpuAlloc{};
        gpuAlloc.usage = VMA_MEMORY_USAGE_AUTO;

        // TLAS storage
        bufCI.size  = sizes.accelerationStructureSize;
        bufCI.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &gpuAlloc, &f.buffer, &f.alloc, nullptr) )

        VkAccelerationStructureCreateInfoKHR asCI{};
        asCI.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        asCI.buffer = f.buffer;
        asCI.size   = sizes.accelerationStructureSize;
        asCI.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        VK_RESULT_CHECK( m_createAS(device, &asCI, S_VulkanAllocator(), &f.as) )

        // scratch (AS builds need 256-aligned scratch addresses)
        bufCI.size  = sizes.buildScratchSize;
        bufCI.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VK_RESULT_CHECK( vmaCreateBufferWithAlignment(m_api->vmaAllocator(), &bufCI, &gpuAlloc, 256,
                                                      &f.scratch, &f.scratchAlloc, nullptr) )
        f.scratchAddress = bufferAddress(f.scratch);

        // host-written instance array
        bufCI.size  = kMaxRtInstances * sizeof(VkAccelerationStructureInstanceKHR);
        bufCI.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VmaAllocationCreateInfo hostAlloc{};
        hostAlloc.usage = VMA_MEMORY_USAGE_AUTO;
        hostAlloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo info;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &hostAlloc, &f.instanceBuffer, &f.instanceAlloc, &info) )
        f.instanceMapped  = info.pMappedData;
        f.instanceAddress = bufferAddress(f.instanceBuffer);
    }

    m_available = true;
    s_debugLayer("S_VulkanRT: ready (TLAS capacity", kMaxRtInstances, "instances)");
}

S_VulkanRT::~S_VulkanRT()
{
    VkDevice device = m_api->device();
    for (auto& kv : m_dynBlas)
    {
        DynBlas& d = kv.second;
        if (d.as)      m_destroyAS(device, d.as, S_VulkanAllocator());
        if (d.buffer)  vmaDestroyBuffer(m_api->vmaAllocator(), d.buffer, d.alloc);
        if (d.scratch) vmaDestroyBuffer(m_api->vmaAllocator(), d.scratch, d.scratchAlloc);
    }
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        TlasFrame& f = m_tlas[i];
        if (f.as)             m_destroyAS(device, f.as, S_VulkanAllocator());
        if (f.buffer)         vmaDestroyBuffer(m_api->vmaAllocator(), f.buffer, f.alloc);
        if (f.scratch)        vmaDestroyBuffer(m_api->vmaAllocator(), f.scratch, f.scratchAlloc);
        if (f.instanceBuffer) vmaDestroyBuffer(m_api->vmaAllocator(), f.instanceBuffer, f.instanceAlloc);
    }
    if (m_buildPool)
        vkDestroyCommandPool(device, m_buildPool, S_VulkanAllocator());
}

S_VulkanRT::Blas S_VulkanRT::buildBlas(VkBuffer positions, uint32_t vertexCount,
                                       VkBuffer indices, uint32_t indexCount)
{
    Blas out;
    if (!m_available || !vertexCount || !indexCount)
        return out;

    VkDevice device = m_api->device();
    const uint32_t triangleCount = indexCount / 3;

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    auto& tri = geometry.geometry.triangles;
    tri.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    tri.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
    tri.vertexData.deviceAddress = bufferAddress(positions);
    tri.vertexStride             = sizeof(MeshBinPosition); // 16B — padded position stream
    tri.maxVertex                = vertexCount - 1;
    tri.indexType                = VK_INDEX_TYPE_UINT32;
    tri.indexData.deviceAddress  = bufferAddress(indices);

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR sizes{};
    sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    m_getBuildSizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &triangleCount, &sizes);

    VkBufferCreateInfo bufCI{};
    bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo gpuAlloc{};
    gpuAlloc.usage = VMA_MEMORY_USAGE_AUTO;

    bufCI.size  = sizes.accelerationStructureSize;
    bufCI.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &gpuAlloc, &out.buffer, &out.alloc, nullptr) )

    VkAccelerationStructureCreateInfoKHR asCI{};
    asCI.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    asCI.buffer = out.buffer;
    asCI.size   = sizes.accelerationStructureSize;
    asCI.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    VK_RESULT_CHECK( m_createAS(device, &asCI, S_VulkanAllocator(), &out.as) )

    VkBuffer      scratch      = VK_NULL_HANDLE;
    VmaAllocation scratchAlloc = VK_NULL_HANDLE;
    bufCI.size  = sizes.buildScratchSize;
    bufCI.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VK_RESULT_CHECK( vmaCreateBufferWithAlignment(m_api->vmaAllocator(), &bufCI, &gpuAlloc, 256,
                                                  &scratch, &scratchAlloc, nullptr) )

    buildInfo.dstAccelerationStructure  = out.as;
    buildInfo.scratchData.deviceAddress = bufferAddress(scratch);

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = triangleCount;
    const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;

    // one-shot build on the graphics queue
    VkCommandBufferAllocateInfo cbAI{};
    cbAI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAI.commandPool        = m_buildPool;
    cbAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAI.commandBufferCount = 1;
    VkCommandBuffer cmd;
    VK_RESULT_CHECK( vkAllocateCommandBuffers(device, &cbAI, &cmd) )

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    m_cmdBuildAS(cmd, 1, &buildInfo, &pRange);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &cmd;
    VK_RESULT_CHECK( vkQueueSubmit(m_api->graphicsQueue(), 1, &submit, VK_NULL_HANDLE) )
    vkQueueWaitIdle(m_api->graphicsQueue());
    vkFreeCommandBuffers(device, m_buildPool, 1, &cmd);

    vmaDestroyBuffer(m_api->vmaAllocator(), scratch, scratchAlloc);

    VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
    addrInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addrInfo.accelerationStructure = out.as;
    out.address = m_getASAddress(device, &addrInfo);
    return out;
}

void S_VulkanRT::destroyBlas(Blas& blas)
{
    if (blas.as)     m_destroyAS(m_api->device(), blas.as, S_VulkanAllocator());
    if (blas.buffer) vmaDestroyBuffer(m_api->vmaAllocator(), blas.buffer, blas.alloc);
    blas = Blas{};
}

VkDeviceAddress S_VulkanRT::buildDynamicBlas(VkCommandBuffer cmd, const void* key,
                                             VkBuffer skinnedPositions, uint32_t vertexCount,
                                             VkBuffer indices, uint32_t indexCount, uint32_t frameIndex)
{
    if (!m_available || !skinnedPositions || !vertexCount || !indexCount)
        return 0;

    VkDevice device = m_api->device();
    const uint32_t triangleCount = indexCount / 3;

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    auto& tri = geometry.geometry.triangles;
    tri.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    tri.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
    tri.vertexData.deviceAddress = bufferAddress(skinnedPositions);
    tri.vertexStride             = sizeof(MeshBinPosition); // skinned output keeps the padded layout
    tri.maxVertex                = vertexCount - 1;
    tri.indexType                = VK_INDEX_TYPE_UINT32;
    tri.indexData.deviceAddress  = bufferAddress(indices);

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    buildInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geometry;

    auto mapKey = std::make_pair(key, frameIndex);
    auto it     = m_dynBlas.find(mapKey);
    if (it == m_dynBlas.end())
    {
        VkAccelerationStructureBuildSizesInfoKHR sizes{};
        sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        m_getBuildSizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                        &buildInfo, &triangleCount, &sizes);

        DynBlas d;
        VkBufferCreateInfo bufCI{};
        bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo gpuAlloc{};
        gpuAlloc.usage = VMA_MEMORY_USAGE_AUTO;

        bufCI.size  = sizes.accelerationStructureSize;
        bufCI.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &gpuAlloc, &d.buffer, &d.alloc, nullptr) )

        VkAccelerationStructureCreateInfoKHR asCI{};
        asCI.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        asCI.buffer = d.buffer;
        asCI.size   = sizes.accelerationStructureSize;
        asCI.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        VK_RESULT_CHECK( m_createAS(device, &asCI, S_VulkanAllocator(), &d.as) )

        bufCI.size  = sizes.buildScratchSize;
        bufCI.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VK_RESULT_CHECK( vmaCreateBufferWithAlignment(m_api->vmaAllocator(), &bufCI, &gpuAlloc, 256,
                                                      &d.scratch, &d.scratchAlloc, nullptr) )
        d.scratchAddress = bufferAddress(d.scratch);

        VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
        addrInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addrInfo.accelerationStructure = d.as;
        d.address = m_getASAddress(device, &addrInfo);

        it = m_dynBlas.emplace(mapKey, d).first;
    }

    DynBlas& d = it->second;
    buildInfo.dstAccelerationStructure  = d.as;
    buildInfo.scratchData.deviceAddress = d.scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = triangleCount;
    const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;
    m_cmdBuildAS(cmd, 1, &buildInfo, &pRange);

    return d.address;
}

void S_VulkanRT::barrierBlasToTlas(VkCommandBuffer cmd)
{
    VkMemoryBarrier barrier{};
    barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void S_VulkanRT::buildTlas(VkCommandBuffer cmd, const std::vector<Instance>& instances, uint32_t frameIndex)
{
    if (!m_available)
        return;

    TlasFrame& f = m_tlas[frameIndex];

    uint32_t count = static_cast<uint32_t>(instances.size());
    if (count > kMaxRtInstances) count = kMaxRtInstances;

    auto* dst = static_cast<VkAccelerationStructureInstanceKHR*>(f.instanceMapped);
    for (uint32_t i = 0; i < count; ++i)
    {
        const Instance& in = instances[i];
        VkAccelerationStructureInstanceKHR& vi = dst[i];
        // glm is column-major; VkTransformMatrixKHR wants a 3x4 row-major matrix
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
                vi.transform.matrix[row][col] = in.transform[col][row];
        vi.instanceCustomIndex                    = i;
        vi.mask                                   = 0xFF;
        vi.instanceShaderBindingTableRecordOffset = 0;
        vi.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        vi.accelerationStructureReference         = in.blasAddress;
    }
    if (count)
        vmaFlushAllocation(m_api->vmaAllocator(), f.instanceAlloc, 0,
                           count * sizeof(VkAccelerationStructureInstanceKHR));

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = f.instanceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount             = 1;
    buildInfo.pGeometries               = &geometry;
    buildInfo.dstAccelerationStructure  = f.as;
    buildInfo.scratchData.deviceAddress = f.scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = count;
    const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;

    m_cmdBuildAS(cmd, 1, &buildInfo, &pRange);

    // TLAS write must be visible to fragment-shader ray queries
    VkMemoryBarrier barrier{};
    barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);
}
