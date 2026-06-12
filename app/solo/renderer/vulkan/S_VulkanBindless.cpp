#include "S_VulkanBindless.h"
#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include <vector>
#include <cstring>

using namespace solo;

S_VulkanBindless::S_VulkanBindless(S_VulkanRendererAPI* api, uint32_t frameCount)
    : m_api(api), m_frameCount(frameCount)
{
    VkDevice device = m_api->device();

    VkDescriptorSetLayoutBinding bindings[3]{};
    bindings[0].binding         = 0; // instance transforms
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags      = VK_SHADER_STAGE_ALL_GRAPHICS;
    bindings[1].binding         = 1; // skinning joint palettes
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags      = VK_SHADER_STAGE_ALL_GRAPHICS;
    bindings[2].binding         = 2; // scene TLAS for fragment ray queries
    bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = 3;
    layoutCI.pBindings    = bindings;
    VK_RESULT_CHECK( vkCreateDescriptorSetLayout(device, &layoutCI, S_VulkanAllocator(), &m_setLayout) )

    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = m_frameCount * 2;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[1].descriptorCount = m_frameCount;

    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.poolSizeCount = 2;
    poolCI.pPoolSizes    = poolSizes;
    poolCI.maxSets       = m_frameCount;
    VK_RESULT_CHECK( vkCreateDescriptorPool(device, &poolCI, S_VulkanAllocator(), &m_pool) )

    std::vector<VkDescriptorSetLayout> layouts(m_frameCount, m_setLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_pool;
    allocInfo.descriptorSetCount = m_frameCount;
    allocInfo.pSetLayouts        = layouts.data();
    VK_RESULT_CHECK( vkAllocateDescriptorSets(device, &allocInfo, m_sets) )

    VkBufferCreateInfo bufCI{};
    bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        VmaAllocationInfo info;

        bufCI.size = kMaxInstances * sizeof(glm::mat4);
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &allocCI,
                                         &m_transformBuffers[i], &m_transformAllocs[i], &info) )
        m_transformMapped[i] = info.pMappedData;

        bufCI.size = kMaxJoints * sizeof(glm::mat4);
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &allocCI,
                                         &m_paletteBuffers[i], &m_paletteAllocs[i], &info) )
        m_paletteMapped[i] = info.pMappedData;

        VkDescriptorBufferInfo dbis[2]{};
        dbis[0].buffer = m_transformBuffers[i];
        dbis[0].offset = 0;
        dbis[0].range  = VK_WHOLE_SIZE;
        dbis[1].buffer = m_paletteBuffers[i];
        dbis[1].offset = 0;
        dbis[1].range  = VK_WHOLE_SIZE;

        VkWriteDescriptorSet writes[2]{};
        for (uint32_t b = 0; b < 2; ++b)
        {
            writes[b].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[b].dstSet          = m_sets[i];
            writes[b].dstBinding      = b;
            writes[b].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[b].descriptorCount = 1;
            writes[b].pBufferInfo     = &dbis[b];
        }
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    }
}

S_VulkanBindless::~S_VulkanBindless()
{
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        vmaDestroyBuffer(m_api->vmaAllocator(), m_transformBuffers[i], m_transformAllocs[i]);
        vmaDestroyBuffer(m_api->vmaAllocator(), m_paletteBuffers[i],   m_paletteAllocs[i]);
    }
    vkDestroyDescriptorPool(m_api->device(), m_pool,      S_VulkanAllocator());
    vkDestroyDescriptorSetLayout(m_api->device(), m_setLayout, S_VulkanAllocator());
}

void S_VulkanBindless::setTlas(const VkAccelerationStructureKHR* tlasPerFrame)
{
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        VkWriteDescriptorSetAccelerationStructureKHR asWrite{};
        asWrite.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        asWrite.accelerationStructureCount = 1;
        asWrite.pAccelerationStructures    = &tlasPerFrame[i];

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext           = &asWrite;
        write.dstSet          = m_sets[i];
        write.dstBinding      = 2;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        write.descriptorCount = 1;
        vkUpdateDescriptorSets(m_api->device(), 1, &write, 0, nullptr);
    }
}

void S_VulkanBindless::uploadTransforms(const glm::mat4* transforms, uint32_t count, uint32_t frameIndex)
{
    if (!count) return;
    uint32_t safeCount = count < kMaxInstances ? count : kMaxInstances;
    memcpy(m_transformMapped[frameIndex], transforms, safeCount * sizeof(glm::mat4));
    vmaFlushAllocation(m_api->vmaAllocator(), m_transformAllocs[frameIndex], 0, safeCount * sizeof(glm::mat4));
}

void S_VulkanBindless::uploadPalettes(const glm::mat4* palettes, uint32_t count, uint32_t frameIndex)
{
    if (!count) return;
    uint32_t safeCount = count < kMaxJoints ? count : kMaxJoints;
    memcpy(m_paletteMapped[frameIndex], palettes, safeCount * sizeof(glm::mat4));
    vmaFlushAllocation(m_api->vmaAllocator(), m_paletteAllocs[frameIndex], 0, safeCount * sizeof(glm::mat4));
}

void S_VulkanBindless::bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex) const
{
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                            1, 1, &m_sets[frameIndex], 0, nullptr);
}
