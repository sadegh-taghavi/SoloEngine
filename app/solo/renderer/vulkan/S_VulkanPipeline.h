#pragma once
#include "solo/renderer/S_RendererAPI.h"
#include "solo/renderer/S_VertexBuffer.h"
#include "solo/stl/S_Vector.h"
#include "solo/stl/S_Array.h"
#include "solo/debug/S_Debug.h"
#include "solo/stl/S_Map.h"
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
    void create(const S_Vector<S_PipelineDescriptor> *descriptors = nullptr );
    void recreate();
    S_Vector<VkPipeline> *pipelines();
    S_Vector<VkPipelineLayout> *layouts();
protected:
    S_VulkanRendererAPI *m_api;
    S_Vector<VkPipeline> m_pipelines;
    S_Vector<VkPipelineLayout> m_layouts;
    S_Vector<S_PipelineDescriptor> m_descriptors;
    VkShaderModule createShaderModule(const S_String &shaderName);
private:
};

}

