#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "solo/stl/S_String.h"
#include "solo/stl/S_List.h"
#include "solo/renderer/S_TextureSampler.h"

namespace solo
{

class S_VulkanVertexBuffer;
class S_VulkanShader;
class S_VulkanTexture;
class S_VulkanTextureSampler;
class S_VertexBufferDescriptorArray;
class S_VulkanRendererAPI;

class S_VulkanItemsManager
{
public:
    S_VulkanItemsManager(S_VulkanRendererAPI *api);
    virtual ~S_VulkanItemsManager();
    S_VulkanVertexBuffer *createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                 std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                 std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray);
    S_VulkanShader *createShader(const S_String &vertexShader, const S_String &fragmentShader,
                           const S_String &geometryShader, const S_String &computeShader);

    S_VulkanTexture *createTexture(const S_String &texture);

    S_VulkanTextureSampler *createTextureSampler(const S_TextureSamplerDescriptor &descriptor);


    //    virtual void destroy(S_VulkanVertexBuffer *buffer);
    virtual void destroy();

private:
    S_VulkanRendererAPI *m_api;  
    S_List<std::unique_ptr<S_VulkanVertexBuffer>> m_vertexBuffers;
    S_List<std::unique_ptr<S_VulkanShader>> m_shaders;
    S_List<std::unique_ptr<S_VulkanTexture>> m_textures;
    S_List<std::unique_ptr<S_VulkanTextureSampler>> m_textureSamplers;

};

}

