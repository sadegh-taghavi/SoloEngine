#pragma once
#include "solo/renderer/S_RendererAPI.h"
#include "solo/renderer/S_VertexBuffer.h"
#include "solo/debug/S_Debug.h"
#include <vector>
#include <array>
#include <map>
#include <string>
#include <vulkan/vulkan.h>
#include <memory>

namespace solo
{

class S_VulkanRendererAPI;
class S_Shader;

struct S_PipelineDescriptor
{
    S_VertexBufferDescriptorArray VertexBufferDescriptorArray;
    S_VertexBufferDescriptorArray InstanceBufferDescriptorArray;
    S_Shader *Shader;
};

class S_VulkanPipeline
{
public:
    struct Pipeline
    {
         VkPipeline Pipeline;
         VkPipelineLayout Layout;
    };
    S_VulkanPipeline(S_VulkanRendererAPI *api);
    virtual ~S_VulkanPipeline();
    virtual void destroy();
    void create(const std::vector<S_PipelineDescriptor> *descriptors = nullptr );
    void recreate();
    std::vector<VkPipeline> *pipelines();
    std::vector<VkPipelineLayout> *layouts();
protected:
    S_VulkanRendererAPI *m_api;
    std::vector<VkPipeline> m_pipelines;
    std::vector<VkPipelineLayout> m_layouts;
    std::vector<S_PipelineDescriptor> m_descriptors;
    VkShaderModule createShaderModule(const std::string &shaderName);
private:
};

}

