#include "S_VulkanBindless.h"
#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include "S_VulkanTexture.h"
#include <vector>
#include <cstring>

using namespace solo;

S_VulkanBindless::S_VulkanBindless(S_VulkanRendererAPI* api, uint32_t frameCount)
    : m_api(api), m_frameCount(frameCount)
{
    VkDevice device = m_api->device();

    VkDescriptorSetLayoutBinding bindings[7]{};
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
    bindings[3].binding         = 3; // material table
    bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[4].binding         = 4; // material textures
    bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].descriptorCount = kMaxTextures;
    bindings[4].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[5].binding         = 5; // RT per-instance shade records
    bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[6].binding         = 6; // unified HDR environment probe cube (IBL + skybox)
    bindings[6].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[6].descriptorCount = 1;
    bindings[6].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = 7;
    layoutCI.pBindings    = bindings;
    VK_RESULT_CHECK( vkCreateDescriptorSetLayout(device, &layoutCI, S_VulkanAllocator(), &m_setLayout) )

    VkDescriptorPoolSize poolSizes[3]{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = m_frameCount * 4;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[1].descriptorCount = m_frameCount;
    poolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = m_frameCount * (kMaxTextures + 1); // +1 env probe cube (binding 6)

    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.poolSizeCount = 3;
    poolCI.pPoolSizes    = poolSizes;
    poolCI.maxSets       = m_frameCount;
    VK_RESULT_CHECK( vkCreateDescriptorPool(device, &poolCI, S_VulkanAllocator(), &m_pool) )

    VkSamplerCreateInfo samplerCI{};
    samplerCI.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.magFilter    = VK_FILTER_LINEAR;
    samplerCI.minFilter    = VK_FILTER_LINEAR;
    samplerCI.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.maxLod       = VK_LOD_CLAMP_NONE;
    VK_RESULT_CHECK( vkCreateSampler(device, &samplerCI, S_VulkanAllocator(), &m_textureSampler) )

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

        bufCI.size = kMaxMaterials * sizeof(S_MaterialRecord);
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &allocCI,
                                         &m_materialBuffers[i], &m_materialAllocs[i], &info) )
        m_materialMapped[i] = info.pMappedData;

        VkDescriptorBufferInfo dbis[3]{};
        dbis[0].buffer = m_transformBuffers[i];
        dbis[0].offset = 0;
        dbis[0].range  = VK_WHOLE_SIZE;
        dbis[1].buffer = m_paletteBuffers[i];
        dbis[1].offset = 0;
        dbis[1].range  = VK_WHOLE_SIZE;
        dbis[2].buffer = m_materialBuffers[i];
        dbis[2].offset = 0;
        dbis[2].range  = VK_WHOLE_SIZE;

        VkWriteDescriptorSet writes[3]{};
        for (uint32_t b = 0; b < 3; ++b)
        {
            writes[b].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[b].dstSet          = m_sets[i];
            writes[b].dstBinding      = b == 2 ? 3 : b; // materials live at binding 3
            writes[b].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[b].descriptorCount = 1;
            writes[b].pBufferInfo     = &dbis[b];
        }
        vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
    }
}

S_VulkanBindless::~S_VulkanBindless()
{
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        vmaDestroyBuffer(m_api->vmaAllocator(), m_transformBuffers[i], m_transformAllocs[i]);
        vmaDestroyBuffer(m_api->vmaAllocator(), m_paletteBuffers[i],   m_paletteAllocs[i]);
        vmaDestroyBuffer(m_api->vmaAllocator(), m_materialBuffers[i],  m_materialAllocs[i]);
    }
    vkDestroySampler(m_api->device(), m_textureSampler, S_VulkanAllocator());
    vkDestroyDescriptorPool(m_api->device(), m_pool,      S_VulkanAllocator());
    vkDestroyDescriptorSetLayout(m_api->device(), m_setLayout, S_VulkanAllocator());
}

void S_VulkanBindless::setRtShadeBuffers(const VkBuffer* buffersPerFrame)
{
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        VkDescriptorBufferInfo dbi{ buffersPerFrame[i], 0, VK_WHOLE_SIZE };
        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_sets[i];
        write.dstBinding      = 5;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo     = &dbi;
        vkUpdateDescriptorSets(m_api->device(), 1, &write, 0, nullptr);
    }
}

void S_VulkanBindless::setMaterials(const std::vector<S_MaterialRecord>& records,
                                    const std::vector<S_Texture*>& textures)
{
    m_materialData = records;
    m_textureData  = textures;
    ++m_materialVersion;
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

void S_VulkanBindless::setEnvCube(S_Texture* env)
{
    if (!env) return;
    auto* vt = static_cast<S_VulkanTexture*>(env);
    for (uint32_t i = 0; i < m_frameCount; ++i)
    {
        VkDescriptorImageInfo info{};
        info.sampler     = m_textureSampler;
        info.imageView   = vt->view();
        info.imageLayout = vt->layout();
        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_sets[i];
        write.dstBinding      = 6;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo      = &info;
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

void S_VulkanBindless::bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex)
{
    // lazy material/texture refresh: this frame slot's fence has been waited,
    // so its set and buffer are safe to rewrite here
    if (m_setVersion[frameIndex] != m_materialVersion && !m_textureData.empty())
    {
        const uint32_t count = m_materialData.size() < kMaxMaterials
                             ? static_cast<uint32_t>(m_materialData.size()) : kMaxMaterials;
        if (count)
        {
            memcpy(m_materialMapped[frameIndex], m_materialData.data(), count * sizeof(S_MaterialRecord));
            vmaFlushAllocation(m_api->vmaAllocator(), m_materialAllocs[frameIndex],
                               0, count * sizeof(S_MaterialRecord));
        }

        VkDescriptorImageInfo imageInfos[kMaxTextures];
        for (uint32_t t = 0; t < kMaxTextures; ++t)
        {
            auto* tex = static_cast<S_VulkanTexture*>(
                t < m_textureData.size() ? m_textureData[t] : m_textureData[0]);
            imageInfos[t].sampler     = m_textureSampler;
            imageInfos[t].imageView   = tex->view();
            imageInfos[t].imageLayout = tex->layout();
        }

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_sets[frameIndex];
        write.dstBinding      = 4;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = kMaxTextures;
        write.pImageInfo      = imageInfos;
        vkUpdateDescriptorSets(m_api->device(), 1, &write, 0, nullptr);

        m_setVersion[frameIndex] = m_materialVersion;
    }

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                            1, 1, &m_sets[frameIndex], 0, nullptr);
}
