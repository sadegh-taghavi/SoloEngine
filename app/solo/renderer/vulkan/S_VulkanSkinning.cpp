#include "S_VulkanSkinning.h"
#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include "S_VulkanMesh.h"
#include "solo/application/S_Application.h"
#include "solo/pack/S_Pack.h"
#include "solo/debug/S_Debug.h"
#include <cstring>

using namespace solo;

S_VulkanSkinning::S_VulkanSkinning(S_VulkanRendererAPI* api, uint32_t frameCount)
    : m_api(api), m_frameCount(frameCount)
{
    VkDevice device = m_api->device();

    auto code = S_Application::executingApplication()->pack()->load("shaders/skin.compc");
    if (code.empty())
    {
        s_debugLayer("S_VulkanSkinning: shaders/skin.compc missing — skinned RT disabled");
        return;
    }

    VkShaderModuleCreateInfo moduleCI{};
    moduleCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCI.codeSize = code.size();
    moduleCI.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    VK_RESULT_CHECK( vkCreateShaderModule(device, &moduleCI, S_VulkanAllocator(), &m_module) )

    VkDescriptorSetLayoutBinding bindings[4]{};
    for (uint32_t b = 0; b < 4; ++b)
    {
        bindings[b].binding         = b;
        bindings[b].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[b].descriptorCount = 1;
        bindings[b].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = 4;
    layoutCI.pBindings    = bindings;
    VK_RESULT_CHECK( vkCreateDescriptorSetLayout(device, &layoutCI, S_VulkanAllocator(), &m_setLayout) )

    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcRange.size       = sizeof(uint32_t) * 2; // vertexCount, paletteBase

    VkPipelineLayoutCreateInfo plCI{};
    plCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCI.setLayoutCount         = 1;
    plCI.pSetLayouts            = &m_setLayout;
    plCI.pushConstantRangeCount = 1;
    plCI.pPushConstantRanges    = &pcRange;
    VK_RESULT_CHECK( vkCreatePipelineLayout(device, &plCI, S_VulkanAllocator(), &m_layout) )

    VkComputePipelineCreateInfo cpCI{};
    cpCI.sType        = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpCI.stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cpCI.stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    cpCI.stage.module = m_module;
    cpCI.stage.pName  = "main";
    cpCI.layout       = m_layout;
    VK_RESULT_CHECK( vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &cpCI, S_VulkanAllocator(), &m_pipeline) )

    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 64 * 4;

    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.poolSizeCount = 1;
    poolCI.pPoolSizes    = &poolSize;
    poolCI.maxSets       = 64;
    VK_RESULT_CHECK( vkCreateDescriptorPool(device, &poolCI, S_VulkanAllocator(), &m_pool) )

    VkBufferCreateInfo bufCI{};
    bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size        = kMaxPaletteJoints * sizeof(glm::mat4);
    bufCI.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo hostAlloc{};
    hostAlloc.usage = VMA_MEMORY_USAGE_AUTO;
    hostAlloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        VmaAllocationInfo info;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &hostAlloc,
                                         &m_paletteBuffers[i], &m_paletteAllocs[i], &info) )
        m_paletteMapped[i] = info.pMappedData;
    }

    m_available = true;
    s_debugLayer("S_VulkanSkinning: compute skinning ready");
}

S_VulkanSkinning::~S_VulkanSkinning()
{
    VkDevice device = m_api->device();
    for (auto& kv : m_meshFrames)
        if (kv.second.dst)
            vmaDestroyBuffer(m_api->vmaAllocator(), kv.second.dst, kv.second.dstAlloc);
    for (uint32_t i = 0; i < m_frameCount; ++i)
        if (m_paletteBuffers[i])
            vmaDestroyBuffer(m_api->vmaAllocator(), m_paletteBuffers[i], m_paletteAllocs[i]);
    if (m_pool)      vkDestroyDescriptorPool(device, m_pool, S_VulkanAllocator());
    if (m_pipeline)  vkDestroyPipeline(device, m_pipeline, S_VulkanAllocator());
    if (m_layout)    vkDestroyPipelineLayout(device, m_layout, S_VulkanAllocator());
    if (m_setLayout) vkDestroyDescriptorSetLayout(device, m_setLayout, S_VulkanAllocator());
    if (m_module)    vkDestroyShaderModule(device, m_module, S_VulkanAllocator());
}

void S_VulkanSkinning::beginFrame(uint32_t frameIndex)
{
    m_paletteCursor[frameIndex] = 0;
}

S_VulkanSkinning::MeshFrame& S_VulkanSkinning::meshFrame(S_VulkanMesh* mesh, uint32_t frameIndex)
{
    auto key = std::make_pair(mesh, frameIndex);
    auto it  = m_meshFrames.find(key);
    if (it != m_meshFrames.end())
        return it->second;

    MeshFrame mf;

    VkBufferCreateInfo bufCI{};
    bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size        = static_cast<VkDeviceSize>(mesh->vertexCount()) * 4 * sizeof(float);
    bufCI.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo gpuAlloc{};
    gpuAlloc.usage = VMA_MEMORY_USAGE_AUTO;
    VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &gpuAlloc, &mf.dst, &mf.dstAlloc, nullptr) )

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &m_setLayout;
    VK_RESULT_CHECK( vkAllocateDescriptorSets(m_api->device(), &allocInfo, &mf.set) )

    VkDescriptorBufferInfo dbis[4]{};
    dbis[0] = { mesh->positionBuffer(),       0, VK_WHOLE_SIZE };
    dbis[1] = { mesh->skinBuffer(),           0, VK_WHOLE_SIZE };
    dbis[2] = { m_paletteBuffers[frameIndex], 0, VK_WHOLE_SIZE };
    dbis[3] = { mf.dst,                       0, VK_WHOLE_SIZE };

    VkWriteDescriptorSet writes[4]{};
    for (uint32_t b = 0; b < 4; ++b)
    {
        writes[b].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[b].dstSet          = mf.set;
        writes[b].dstBinding      = b;
        writes[b].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[b].descriptorCount = 1;
        writes[b].pBufferInfo     = &dbis[b];
    }
    vkUpdateDescriptorSets(m_api->device(), 4, writes, 0, nullptr);

    return m_meshFrames.emplace(key, mf).first->second;
}

VkBuffer S_VulkanSkinning::dispatch(VkCommandBuffer cmd, S_VulkanMesh* mesh,
                                    const glm::mat4* palette, uint32_t jointCount, uint32_t frameIndex)
{
    if (!m_available || !mesh || !mesh->skinBuffer() || !jointCount)
        return VK_NULL_HANDLE;

    uint32_t base = m_paletteCursor[frameIndex];
    if (base + jointCount > kMaxPaletteJoints)
        return VK_NULL_HANDLE; // palette capacity exhausted this frame

    memcpy(static_cast<glm::mat4*>(m_paletteMapped[frameIndex]) + base,
           palette, jointCount * sizeof(glm::mat4));
    vmaFlushAllocation(m_api->vmaAllocator(), m_paletteAllocs[frameIndex],
                       base * sizeof(glm::mat4), jointCount * sizeof(glm::mat4));
    m_paletteCursor[frameIndex] = base + jointCount;

    MeshFrame& mf = meshFrame(mesh, frameIndex);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_layout, 0, 1, &mf.set, 0, nullptr);

    struct { uint32_t vertexCount, paletteBase; } pc{ mesh->vertexCount(), base };
    vkCmdPushConstants(cmd, m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
    vkCmdDispatch(cmd, (mesh->vertexCount() + 63) / 64, 1, 1);

    return mf.dst;
}

void S_VulkanSkinning::barrierToAsBuild(VkCommandBuffer cmd)
{
    VkMemoryBarrier barrier{};
    barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);
}
