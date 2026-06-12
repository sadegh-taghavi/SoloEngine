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

    // pinned bindless texture slots the shader addresses directly:
    // 0 = white, 1 = prefiltered HDR environment, 2 = irradiance
    textureSlot("textures/white.ktx");
    textureSlot("textures/env.ktx");
    textureSlot("textures/env_irr.ktx");
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

    // resolve the mesh's glTF materials into the global material table
    if (!raw->pendingMaterials().empty())
    {
        std::vector<uint32_t> ids;
        ids.reserve(raw->pendingMaterials().size());
        for (const MeshBinMaterial& m : raw->pendingMaterials())
            ids.push_back(createMaterial(
                glm::vec4(m.baseColorFactor[0], m.baseColorFactor[1],
                          m.baseColorFactor[2], m.baseColorFactor[3]),
                m.baseColorPath[0] ? std::string(m.baseColorPath) : std::string(),
                m.metallicFactor, m.roughnessFactor,
                m.normalPath[0] ? std::string(m.normalPath) : std::string(),
                m.metallicRoughnessPath[0] ? std::string(m.metallicRoughnessPath) : std::string()));
        raw->setGlobalMaterialIDs(std::move(ids));
    }

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

uint32_t S_Renderer::textureSlot(const std::string& packPath)
{
    auto it = m_textureSlotCache.find(packPath);
    if (it != m_textureSlotCache.end())
        return it->second;

    S_Texture* tex = getTexture(createTexture(packPath));
    if (!tex)
    {
        s_debugLayer("S_Renderer: material texture missing, using white:", packPath);
        return 0;
    }
    const uint32_t slot = static_cast<uint32_t>(m_materialTextures.size());
    m_materialTextures.push_back(tex);
    m_textureSlotCache[packPath] = slot;
    m_materialsDirty = true;
    return slot;
}

uint32_t S_Renderer::createMaterial()
{
    return createMaterial(glm::vec4(1.0f));
}

uint32_t S_Renderer::createMaterial(const glm::vec4& baseColorFactor, const std::string& baseColorTexture,
                                    float metallic, float roughness,
                                    const std::string& normalTexture,
                                    const std::string& metallicRoughnessTexture)
{
    if (m_materialTextures.empty())
        textureSlot("textures/white.ktx"); // slot 0 = default white

    S_MaterialRecord rec;
    rec.baseColorFactor  = baseColorFactor;
    rec.metallicFactor   = metallic;
    rec.roughnessFactor  = roughness;
    rec.baseColorTexSlot = baseColorTexture.empty()         ? 0u : textureSlot(baseColorTexture);
    rec.normalTexSlot    = normalTexture.empty()            ? 0u : textureSlot(normalTexture);
    rec.mrTexSlot        = metallicRoughnessTexture.empty() ? 0u : textureSlot(metallicRoughnessTexture);
    m_materialRecords.push_back(rec);
    m_materialsDirty = true;
    return static_cast<uint32_t>(m_materialRecords.size() - 1);
}

void S_Renderer::syncMaterials()
{
    if (!m_materialsDirty) return;
    static_cast<S_VulkanRendererAPI*>(m_api.get())->updateMaterialTable(m_materialRecords, m_materialTextures);
    m_materialsDirty = false;
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

    syncMaterials();

    // static draws double as next frame's TLAS instances; skinned draws are
    // captured with their palettes for the compute-skin + dynamic BLAS pre-pass
    std::vector<S_VulkanRT::Instance>        rtInstances;
    std::vector<S_VulkanRT::SkinnedInstance> rtSkinned;
    rtInstances.reserve(m_queue.draws().size());

    std::vector<S_ResolvedDraw> staticDraws, skinnedDraws;
    staticDraws.reserve(m_queue.draws().size());
    for( const auto& draw : m_queue.draws() )
    {
        auto* mesh = getMesh(draw.mesh);
        if( !mesh ) continue;
        const uint32_t matBase  = mesh->hasMaterials() ? mesh->globalMaterialID(0, draw.materialID)
                                                       : draw.materialID;
        const uint32_t useLocal = mesh->hasMaterials() ? 1u : 0u;

        if( draw.paletteOffset == S_NO_PALETTE && mesh->blasAddress() != 0 )
        {
            S_VulkanRT::Instance inst;
            inst.blasAddress      = mesh->blasAddress();
            inst.transform        = m_queue.transforms()[draw.instanceIndex];
            inst.indexAddress     = mesh->indexAddress();
            inst.hitDataAddress   = mesh->hitDataAddress();
            inst.materialBase     = matBase;
            inst.useLocalMaterial = useLocal;
            rtInstances.push_back(inst);
        }
        // skinned draws fall back to the static shader (bind pose) when no skinned shader given
        if( skinnedShader && draw.paletteOffset != S_NO_PALETTE )
        {
            skinnedDraws.push_back({ mesh, draw.instanceIndex, draw.materialID, draw.paletteOffset });

            const uint32_t jointCount = mesh->skinJointCount();
            if( jointCount && draw.paletteOffset + jointCount <= m_queue.palettes().size() )
            {
                S_VulkanRT::SkinnedInstance entry;
                entry.mesh      = mesh;
                entry.transform = m_queue.transforms()[draw.instanceIndex];
                entry.palette.assign(m_queue.palettes().begin() + draw.paletteOffset,
                                     m_queue.palettes().begin() + draw.paletteOffset + jointCount);
                entry.indexAddress     = mesh->indexAddress();
                entry.hitDataAddress   = mesh->hitDataAddress();
                entry.materialBase     = matBase;
                entry.useLocalMaterial = useLocal;
                rtSkinned.push_back(std::move(entry));
            }
        }
        else
            staticDraws.push_back({ mesh, draw.instanceIndex, draw.materialID, S_NO_PALETTE });
    }

    auto* vkApi = static_cast<S_VulkanRendererAPI*>(m_api.get());
    vkApi->setRtInstances(std::move(rtInstances));
    vkApi->setRtSkinnedInstances(std::move(rtSkinned));

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
