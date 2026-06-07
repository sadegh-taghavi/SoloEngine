#include "S_VulkanPerFrame.h"
#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include "solo/debug/S_Debug.h"
#include <cstring>

using namespace solo;

S_VulkanPerFrame::S_VulkanPerFrame(S_VulkanRendererAPI* api, uint32_t frameCount, size_t dataSize)
    : m_api(api), m_frameCount(frameCount), m_dataSize(dataSize)
{
    VkDevice device = m_api->device();

    VkDescriptorSetLayoutBinding binding = {};
    binding.binding            = 0;
    binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount    = 1;
    binding.stageFlags         = VK_SHADER_STAGE_ALL_GRAPHICS;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &binding;
    VK_RESULT_CHECK( vkCreateDescriptorSetLayout(device, &layoutInfo, S_VulkanAllocator(), &m_setLayout) )

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts    = &m_setLayout;
    VK_RESULT_CHECK( vkCreatePipelineLayout(device, &pipelineLayoutInfo, S_VulkanAllocator(), &m_layout) )

    VkDescriptorPoolSize poolSize = {};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = m_frameCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = m_frameCount;
    VK_RESULT_CHECK( vkCreateDescriptorPool(device, &poolInfo, S_VulkanAllocator(), &m_pool) )

    std::vector<VkDescriptorSetLayout> layouts(m_frameCount, m_setLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_pool;
    allocInfo.descriptorSetCount = m_frameCount;
    allocInfo.pSetLayouts        = layouts.data();
    VK_RESULT_CHECK( vkAllocateDescriptorSets(device, &allocInfo, m_sets) )

    VkBufferCreateInfo bufInfo = {};
    bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size        = m_dataSize;
    bufInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI = {};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        VmaAllocationInfo info;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufInfo, &allocCI,
                                         &m_buffers[i], &m_allocs[i], &info) )
        m_mapped[i] = info.pMappedData;

        VkDescriptorBufferInfo dbi = {};
        dbi.buffer = m_buffers[i];
        dbi.offset = 0;
        dbi.range  = m_dataSize;

        VkWriteDescriptorSet write = {};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_sets[i];
        write.dstBinding      = 0;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo     = &dbi;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

S_VulkanPerFrame::~S_VulkanPerFrame()
{
    for (uint32_t i = 0; i < m_frameCount; ++i)
        vmaDestroyBuffer(m_api->vmaAllocator(), m_buffers[i], m_allocs[i]);
    vkDestroyDescriptorPool(m_api->device(), m_pool,      S_VulkanAllocator());
    vkDestroyPipelineLayout(m_api->device(), m_layout,    S_VulkanAllocator());
    vkDestroyDescriptorSetLayout(m_api->device(), m_setLayout, S_VulkanAllocator());
}

void S_VulkanPerFrame::update(const void* data, uint32_t frameIndex)
{
    memcpy(m_mapped[frameIndex], data, m_dataSize);
    vmaFlushAllocation(m_api->vmaAllocator(), m_allocs[frameIndex], 0, VK_WHOLE_SIZE);
}

void S_VulkanPerFrame::bind(VkCommandBuffer cmd, uint32_t frameIndex) const
{
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout,
                            0, 1, &m_sets[frameIndex], 0, nullptr);
}
