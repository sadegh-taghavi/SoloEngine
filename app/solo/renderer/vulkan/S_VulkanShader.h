#pragma once

#include "S_VulkanShaderReflection.h"
#include "solo/renderer/S_Shader.h"
#include "solo/stl/S_Vector.h"
#include "solo/renderer/vulkan/S_VulkanDeviceAllocator.h"
#include <vulkan/vulkan.h>

namespace solo
{

class S_VulkanRendererAPI;

struct S_ShaderReflectionData
{
    S_VulkanShaderReflection Reflection;
    S_Vector<S_VulkanShaderReflectionUniformBuffer> UniformBuffers;
    S_Map<S_String, uint32_t> UniformBuffersMap;
    S_Vector<S_VulkanShaderReflectionTexture> Textures;
    S_Map<S_String, uint32_t> TextureMap;
};

class S_VulkanShader: public S_Shader
{
public:
    S_VulkanShader(S_VulkanRendererAPI *api, const S_String &vertexShader, const S_String &fragmentShader,
                   const S_String &geometryShader, const S_String &computeShader);
    virtual ~S_VulkanShader();
    void updateUniformValue(const S_String &name, S_ShaderStage stage, const void *value );
    void updateTextureValue( const S_String &name, S_ShaderStage stage, const S_Texture &texture, uint32_t arrayIndex = 0 );
    void bind();
    void commit();
    VkShaderModule shaderModule( S_ShaderStage type );
    const S_ShaderReflectionData *shaderReflection( S_ShaderStage type );
    const S_Vector<VkDescriptorSetLayout> *descriptorSetLayouts();

private:
    void setShader(S_ShaderStage stage, const S_String &name );
    VkDescriptorPool m_descriptorsPool;
    uint32_t m_maxUniformSetInStages;
    uint32_t m_maxTextureSetInStages;
    uint32_t m_commitsCount;
    uint64_t m_bufferAlignment;
    S_Vector<VkMappedMemoryRange> m_aboutToWriteMemoryRanges;
    S_Vector<VkWriteDescriptorSet> m_aboutToWriteDescriptorSets;
    S_Vector<VkDescriptorSet> m_aboutToUseDescriptorSets;
    S_Vector<std::unique_ptr<VkDescriptorBufferInfo>> m_descriptorBufferInfos;
    S_Vector<std::unique_ptr<VkDescriptorImageInfo>> m_descriptorImageInfos;
    uint32_t m_uniformsMemorySize;
    VkBuffer m_uniformBuffers;
    S_VulkanDeviceMemory m_uniformBuffersMemory;
    S_Vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
    S_Vector<VkDescriptorSet> m_descriptorSets;
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

