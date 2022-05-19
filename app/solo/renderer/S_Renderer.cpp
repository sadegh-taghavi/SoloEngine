#include "S_Renderer.h"
#include "S_RendererAPI.h"
#include "S_Scene.h"
#include "solo/platforms/S_BaseApplication.h"
#include "solo/debug/S_Debug.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"

using namespace solo;

S_Renderer::S_Renderer()
{
    m_api = std::make_unique<S_VulkanRendererAPI>();
}

S_RendererAPI *S_Renderer::api() const
{
    return m_api.get();
}

S_VertexBuffer *S_Renderer::createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                               std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                               std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray)
{
    return m_api->createVertexBuffer(verticesCount, indicesCount, instancesCount,
                                     std::move(verticesDescriptorArray),
                                     std::move(instancesDescriptorArray) );
}

S_Shader *S_Renderer::createShader(const S_String &vertexShader, const S_String &fragmentShader, const S_String &geometryShader, const S_String &computeShader)
{
    return m_api->createShader( vertexShader, fragmentShader, geometryShader, computeShader );
}

S_Texture *S_Renderer::createTexture(const S_String &texture)
{
    return m_api->createTexture( texture );
}

S_TextureSampler *S_Renderer::createTextureSampler(const S_TextureSamplerDescriptor &descriptor)
{
    return m_api->createTextureSampler(descriptor);
}

void S_Renderer::beginScene(std::shared_ptr<S_Scene> scene)
{

}

void S_Renderer::endScene()
{

}

void S_Renderer::drawFrame()
{
    m_elapsedTimeUs = m_elapsedTime.restart();
    m_api->drawFrame();
}

void S_Renderer::resize(uint32_t width, uint32_t height)
{
    m_api->resize(width, height);
}

void S_Renderer::active(bool active)
{
    m_api->active( active );
}

uint64_t S_Renderer::elapsedTimeUs()
{
    return m_elapsedTimeUs;
}

S_Renderer::~S_Renderer()
{

}
