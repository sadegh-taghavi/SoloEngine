#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include <string>
#include <list>
#include "solo/renderer/S_TextureSampler.h"

namespace solo
{

class S_VulkanVertexBuffer;
class S_VulkanShader;
class S_VulkanTexture;
class S_VulkanTextureSampler;
class S_VulkanMesh;
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
    S_VulkanShader *createShader(const std::string &vertexShader, const std::string &fragmentShader,
                           const std::string &geometryShader, const std::string &computeShader);
    S_VulkanTexture *createTexture(const std::string &texture);
    S_VulkanTextureSampler *createTextureSampler(const S_TextureSamplerDescriptor &descriptor);
    S_VulkanMesh *createMesh(const std::string &path);

    virtual void destroy();

private:
    S_VulkanRendererAPI *m_api;
    std::list<std::unique_ptr<S_VulkanVertexBuffer>>   m_vertexBuffers;
    std::list<std::unique_ptr<S_VulkanShader>>         m_shaders;
    std::list<std::unique_ptr<S_VulkanTexture>>        m_textures;
    std::list<std::unique_ptr<S_VulkanTextureSampler>> m_textureSamplers;
    std::list<std::unique_ptr<S_VulkanMesh>>           m_meshes;
};

}
