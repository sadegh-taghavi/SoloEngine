#include "S_VulkanItemsManager.h"
#include "solo/renderer/vulkan/S_VulkanVertexBuffer.h"
#include "solo/renderer/vulkan/S_VulkanShader.h"
#include "solo/renderer/vulkan/S_VulkanTexture.h"
#include "solo/renderer/vulkan/S_VulkanTextureSampler.h"
#include "S_VulkanRendererAPI.h"
#include "S_VulkanPipeline.h"
#include "S_VulkanAllocator.h"
#include "solo/stl/S_Map.h"
#include "solo/application/S_Application.h"
#include "solo/debug/S_Debug.h"
#include <stdint.h>
#include <algorithm>
#include <set>

using namespace solo;

S_VulkanItemsManager::S_VulkanItemsManager(S_VulkanRendererAPI *api): m_api( api )
{

}

S_VulkanItemsManager::~S_VulkanItemsManager()
{

}

S_VulkanVertexBuffer *S_VulkanItemsManager::createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                                          std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                                          std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray )
{
    m_vertexBuffers.push_back( std::make_unique<S_VulkanVertexBuffer>( m_api, verticesCount, indicesCount, instancesCount,
                                                                       std::move( verticesDescriptorArray ), std::move( instancesDescriptorArray ) ) );
    auto it = m_vertexBuffers.end();
    return (--it)->get();
}

S_VulkanShader *S_VulkanItemsManager::createShader(const S_String &vertexShader, const S_String &fragmentShader,
                                                    const S_String &geometryShader, const S_String &computeShader )
{
    m_shaders.push_back( std::make_unique<S_VulkanShader>( m_api, vertexShader, fragmentShader, geometryShader, computeShader ) );
    auto it = m_shaders.end();
    return (--it)->get();
}

S_VulkanTexture *S_VulkanItemsManager::createTexture(const S_String &texture )
{
    m_textures.push_back( std::make_unique<S_VulkanTexture>( m_api, texture ) );
    auto it = m_textures.end();
    return (--it)->get();
}

S_VulkanTextureSampler *S_VulkanItemsManager::createTextureSampler(const S_TextureSamplerDescriptor &descriptor)
{
    m_textureSamplers.push_back( std::make_unique<S_VulkanTextureSampler>( m_api, descriptor ) );
    auto it = m_textureSamplers.end();
    return (--it)->get();
}

void S_VulkanItemsManager::destroy()
{
    m_vertexBuffers.clear();
    m_shaders.clear();
    m_textures.clear();
    m_textureSamplers.clear();
}

