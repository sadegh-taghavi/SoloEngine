#pragma once

#include "S_VulkanShaderReflection.h"
#include "solo/renderer/S_Shader.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <vk_mem_alloc.h>

namespace solo
{

class S_VulkanRendererAPI;

struct S_ShaderReflectionData
{
    S_VulkanShaderReflection Reflection;
    std::vector<S_VulkanShaderReflectionUniformBuffer> UniformBuffers;
    std::map<std::string, uint32_t> UniformBuffersMap;
    std::vector<S_VulkanShaderReflectionTexture> Textures;
    std::map<std::string, uint32_t> TextureMap;
};

class S_VulkanShader: public S_Shader
{
public:
    S_VulkanShader(S_VulkanRendererAPI *api, const std::string &vertexShader, const std::string &fragmentShader,
                   const std::string &geometryShader, const std::string &computeShader);
    virtual ~S_VulkanShader();
    void updateUniformValue(const std::string &name, S_ShaderStage stage, const void *value );
    void updateTextureValue( const std::string &name, S_ShaderStage stage, const S_Texture &texture, uint32_t arrayIndex = 0 );
    void bind();
    void commit();
    void setPipelineLayout(VkPipelineLayout layout);
    VkShaderModule shaderModule( S_ShaderStage type );
    const S_ShaderReflectionData *shaderReflection( S_ShaderStage type );
    const std::vector<VkDescriptorSetLayout> *descriptorSetLayouts();

private:
    void setShader(S_ShaderStage stage, const std::string &name );
    VkDescriptorPool m_descriptorsPool;
    uint32_t m_maxUniformSetInStages;
    uint32_t m_maxTextureSetInStages;
    uint32_t m_commitsCount;
    uint64_t m_bufferAlignment;
    std::vector<VkWriteDescriptorSet> m_aboutToWriteDescriptorSets;
    std::vector<VkDescriptorSet> m_aboutToUseDescriptorSets;
    std::vector<std::unique_ptr<VkDescriptorBufferInfo>> m_descriptorBufferInfos;
    std::vector<std::unique_ptr<VkDescriptorImageInfo>> m_descriptorImageInfos;
    uint32_t m_uniformsMemorySize;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    size_t m_dirtyMin = SIZE_MAX;
    size_t m_dirtyMax = 0;
    VkBuffer m_uniformBuffers;
    VmaAllocation m_uniformBuffersAllocation;
    void *m_uniformBuffersMappedData;
    std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
    std::vector<VkDescriptorSet> m_descriptorSets;
    S_VulkanRendererAPI *m_api;
    S_ShaderReflectionData m_shaderReflections[static_cast<int>(S_ShaderStage::Count)];
    VkShaderModule m_shaderModules[static_cast<int>(S_ShaderStage::Count)];
    constexpr static const char* const m_EXTENSIONS[] = { ".vert", ".frag", ".geom", ".comp" };
    constexpr static const VkShaderStageFlagBits m_STAGEMAP[] = { VK_SHADER_STAGE_VERTEX_BIT,
                                                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                  VK_SHADER_STAGE_GEOMETRY_BIT,
                                                                  VK_SHADER_STAGE_COMPUTE_BIT };

};

}

