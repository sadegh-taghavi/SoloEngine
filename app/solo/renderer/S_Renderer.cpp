#include "S_Renderer.h"
#include "S_RendererAPI.h"
#include "S_Shader.h"
#include "solo/mesh/S_Mesh.h"
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

S_VertexBufferHandle S_Renderer::createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                                     std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                                     std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray)
{
    S_VertexBuffer* raw = m_api->createVertexBuffer(verticesCount, indicesCount, instancesCount,
                                                     std::move(verticesDescriptorArray),
                                                     std::move(instancesDescriptorArray));
    return poolInsert<S_VertexBuffer, 2>(m_vbSlots, m_freeVB, raw);
}

S_ShaderHandle S_Renderer::createShader(const std::string &vertexShader, const std::string &fragmentShader,
                                         const std::string &geometryShader, const std::string &computeShader)
{
    std::string key = vertexShader + '\n' + fragmentShader + '\n' + geometryShader + '\n' + computeShader;
    auto it = m_shaderCache.find(key);
    if (it != m_shaderCache.end()) return it->second;
    S_Shader* raw = m_api->createShader(vertexShader, fragmentShader, geometryShader, computeShader);
    if (!raw) return {};
    S_ShaderHandle h = poolInsert<S_Shader, 1>(m_shaderSlots, m_freeShader, raw);
    m_shaderCache[key] = h;
    return h;
}

S_TextureHandle S_Renderer::createTexture(const std::string &texture)
{
    auto it = m_textureCache.find(texture);
    if (it != m_textureCache.end()) return it->second;
    S_Texture* raw = m_api->createTexture(texture);
    if (!raw) return {};
    S_TextureHandle h = poolInsert<S_Texture, 0>(m_textureSlots, m_freeTexture, raw);
    m_textureCache[texture] = h;
    return h;
}

S_SamplerHandle S_Renderer::createTextureSampler(const S_TextureSamplerDescriptor &descriptor)
{
    S_TextureSampler* raw = m_api->createTextureSampler(descriptor);
    return poolInsert<S_TextureSampler, 3>(m_samplerSlots, m_freeSampler, raw);
}

S_VertexBuffer* S_Renderer::getVertexBuffer(S_VertexBufferHandle h) const
{
    return poolGet<S_VertexBuffer, 2>(m_vbSlots, h);
}

S_Shader* S_Renderer::getShader(S_ShaderHandle h) const
{
    return poolGet<S_Shader, 1>(m_shaderSlots, h);
}

S_Texture* S_Renderer::getTexture(S_TextureHandle h) const
{
    return poolGet<S_Texture, 0>(m_textureSlots, h);
}

S_TextureSampler* S_Renderer::getSampler(S_SamplerHandle h) const
{
    return poolGet<S_TextureSampler, 3>(m_samplerSlots, h);
}

S_MeshHandle S_Renderer::createMesh(const std::string &path)
{
    auto it = m_meshCache.find(path);
    if (it != m_meshCache.end()) return it->second;
    S_Mesh* raw = m_api->createMesh(path);
    if (!raw) return {};
    S_MeshHandle h = poolInsert<S_Mesh, 4>(m_meshSlots, m_freeMesh, raw);
    m_meshCache[path] = h;
    return h;
}

S_Mesh* S_Renderer::getMesh(S_MeshHandle h) const
{
    return poolGet<S_Mesh, 4>(m_meshSlots, h);
}

void S_Renderer::updatePerFrame(const S_PerFrameData& data)
{
    m_api->updatePerFrame(&data, sizeof(data));
}

uint32_t S_Renderer::createMaterial()
{
    return m_materialPool.allocate();
}

void S_Renderer::submitDraw(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID)
{
    m_queue.submit(mesh, transform, materialID);
}

void S_Renderer::submitDraw(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID,
                            const std::vector<glm::mat4>& jointPalette)
{
    m_queue.submit(mesh, transform, materialID,
                   jointPalette.data(), static_cast<uint32_t>(jointPalette.size()));
}

void S_Renderer::flushDraws(S_ShaderHandle shaderH, S_ShaderHandle skinnedShaderH)
{
    if( m_queue.empty() ) return;
    auto* shader        = getShader(shaderH);
    auto* skinnedShader = getShader(skinnedShaderH);
    if( !shader ) return;

    std::vector<S_ResolvedDraw> staticDraws, skinnedDraws;
    staticDraws.reserve(m_queue.draws().size());
    for( const auto& draw : m_queue.draws() )
    {
        auto* mesh = getMesh(draw.mesh);
        if( !mesh ) continue;
        // skinned draws fall back to the static shader (bind pose) when no skinned shader given
        if( skinnedShader && draw.paletteOffset != S_NO_PALETTE )
            skinnedDraws.push_back({ mesh, draw.instanceIndex, draw.materialID, draw.paletteOffset });
        else
            staticDraws.push_back({ mesh, draw.instanceIndex, draw.materialID, S_NO_PALETTE });
    }

    const auto& transforms = m_queue.transforms();
    const auto& palettes   = m_queue.palettes();

    if( !staticDraws.empty() )
    {
        shader->bind();
        shader->commit();
        m_api->flushRenderQueue(shader, staticDraws,
                                 transforms.data(), static_cast<uint32_t>(transforms.size()));
    }

    if( skinnedShader && !skinnedDraws.empty() )
    {
        skinnedShader->bind();
        skinnedShader->commit();
        m_api->flushRenderQueue(skinnedShader, skinnedDraws,
                                 transforms.data(), static_cast<uint32_t>(transforms.size()),
                                 palettes.data(),   static_cast<uint32_t>(palettes.size()));
    }
}

void S_Renderer::clearDraws()
{
    m_queue.clear();
}

void S_Renderer::createGraphicsPipeline(const std::vector<S_PipelineDescriptor> &descriptors)
{
    m_api->createGraphicsPipeline(descriptors);
}

void S_Renderer::setRenderCallback(std::function<void()> callback)
{
    m_api->setRenderCallback(std::move(callback));
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
