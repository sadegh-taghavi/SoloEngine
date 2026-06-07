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

    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_ALL_GRAPHICS;

    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = 1;
    layoutCI.pBindings    = &binding;
    VK_RESULT_CHECK( vkCreateDescriptorSetLayout(device, &layoutCI, S_VulkanAllocator(), &m_setLayout) )

    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = m_frameCount;

    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.poolSizeCount = 1;
    poolCI.pPoolSizes    = &poolSize;
    poolCI.maxSets       = m_frameCount;
    VK_RESULT_CHECK( vkCreateDescriptorPool(device, &poolCI, S_VulkanAllocator(), &m_pool) )

    std::vector<VkDescriptorSetLayout> layouts(m_frameCount, m_setLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_pool;
    allocInfo.descriptorSetCount = m_frameCount;
    allocInfo.pSetLayouts        = layouts.data();
    VK_RESULT_CHECK( vkAllocateDescriptorSets(device, &allocInfo, m_sets) )

    const VkDeviceSize bufSize = kMaxInstances * sizeof(glm::mat4);

    VkBufferCreateInfo bufCI{};
    bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size        = bufSize;
    bufCI.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        VmaAllocationInfo info;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &allocCI,
                                         &m_transformBuffers[i], &m_transformAllocs[i], &info) )
        m_transformMapped[i] = info.pMappedData;

        VkDescriptorBufferInfo dbi{};
        dbi.buffer = m_transformBuffers[i];
        dbi.offset = 0;
        dbi.range  = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_sets[i];
        write.dstBinding      = 0;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo     = &dbi;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

S_VulkanBindless::~S_VulkanBindless()
{
    for (uint32_t i = 0; i < m_frameCount; ++i)
        vmaDestroyBuffer(m_api->vmaAllocator(), m_transformBuffers[i], m_transformAllocs[i]);
    vkDestroyDescriptorPool(m_api->device(), m_pool,      S_VulkanAllocator());
    vkDestroyDescriptorSetLayout(m_api->device(), m_setLayout, S_VulkanAllocator());
}

void S_VulkanBindless::uploadTransforms(const glm::mat4* transforms, uint32_t count, uint32_t frameIndex)
{
    if (!count) return;
    uint32_t safeCount = count < kMaxInstances ? count : kMaxInstances;
    memcpy(m_transformMapped[frameIndex], transforms, safeCount * sizeof(glm::mat4));
    vmaFlushAllocation(m_api->vmaAllocator(), m_transformAllocs[frameIndex], 0, safeCount * sizeof(glm::mat4));
}

void S_VulkanBindless::bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex) const
{
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                            1, 1, &m_sets[frameIndex], 0, nullptr);
}
