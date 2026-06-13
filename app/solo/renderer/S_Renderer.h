#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include "solo/renderer/S_Handle.h"
#include "solo/renderer/S_PerFrame.h"
#include "solo/renderer/S_RenderQueue.h"
#include "solo/renderer/S_VertexBuffer.h"
#include "solo/renderer/S_TextureSampler.h"
#include "solo/renderer/S_PipelineDescriptor.h"
#include <string>
#include "solo/utility/S_ElapsedTime.h"

namespace solo
{

// GPU material record, std430-compatible (48 bytes)
struct S_MaterialRecord
{
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float     metallicFactor  = 0.0f;
    float     roughnessFactor = 1.0f;
    uint32_t  baseColorTexSlot = 0;
    uint32_t  normalTexSlot    = 0; // 0 = none (slot 0 is white, never a normal map)
    uint32_t  mrTexSlot        = 0; // 0 = none
    uint32_t  pad[3] = {};
};

class S_BaseApplication;
class S_RendererAPI;
class S_Scene;
class S_Shader;
class S_TextureSampler;
class S_Texture;
class S_Mesh;

class S_Renderer
{

public:
    S_Renderer();
    virtual ~S_Renderer();
    S_RendererAPI *api() const;

    S_VertexBufferHandle createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                            std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                            std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray);
    S_ShaderHandle       createShader(const std::string &vertexShader, const std::string &fragmentShader,
                                      const std::string &geometryShader, const std::string &computeShader);
    S_TextureHandle      createTexture(const std::string &texture);
    S_SamplerHandle      createTextureSampler(const S_TextureSamplerDescriptor &descriptor);

    S_VertexBuffer*   getVertexBuffer(S_VertexBufferHandle h) const;
    S_Shader*         getShader(S_ShaderHandle h) const;
    S_Texture*        getTexture(S_TextureHandle h) const;
    S_TextureSampler* getSampler(S_SamplerHandle h) const;
    S_MeshHandle      createMesh(const std::string &path);
    S_Mesh*           getMesh(S_MeshHandle h) const;
    void              updatePerFrame(const S_PerFrameData& data);
    uint32_t          createMaterial(); // plain white
    uint32_t          createMaterial(const glm::vec4& baseColorFactor,
                                     const std::string& baseColorTexture = std::string(),
                                     float metallic = 0.0f, float roughness = 1.0f,
                                     const std::string& normalTexture = std::string(),
                                     const std::string& metallicRoughnessTexture = std::string());
    uint32_t          textureSlot(const std::string& packPath); // bindless slot, cached by path
    void              submitDraw(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID = 0);
    void              submitDraw(S_MeshHandle mesh, const glm::mat4& transform, uint32_t materialID,
                                 const std::vector<glm::mat4>& jointPalette);
    void              flushDraws(S_ShaderHandle shader, S_ShaderHandle skinnedShader = S_ShaderHandle());
    void              clearDraws();

    void createGraphicsPipeline(const std::vector<S_PipelineDescriptor> &descriptors);
    // standard static + skinned pipelines for mesh.bin content; the vertex
    // layouts live here, next to the format, instead of in every app
    void createMeshPipelines(S_ShaderHandle meshShader, S_ShaderHandle skinnedShader);
    void setRenderCallback(std::function<void()> callback);
    void drawFrame();
    void resize( uint32_t width, uint32_t height );
    void active(bool active);
    uint64_t elapsedTimeUs();

private:
    template<typename T>
    struct Slot { T* ptr = nullptr; uint32_t gen = 0; bool live = false; };

    template<typename T, uint32_t Tag>
    S_Handle<Tag> poolInsert(std::vector<Slot<T>>& slots, std::vector<uint32_t>& free, T* ptr)
    {
        uint32_t idx;
        if (!free.empty()) { idx = free.back(); free.pop_back(); }
        else { idx = static_cast<uint32_t>(slots.size()); slots.push_back({}); }
        slots[idx].ptr  = ptr;
        slots[idx].live = true;
        return { idx, slots[idx].gen };
    }

    template<typename T, uint32_t Tag>
    T* poolGet(const std::vector<Slot<T>>& slots, S_Handle<Tag> h) const
    {
        if (!h.valid() || h.index >= static_cast<uint32_t>(slots.size())) return nullptr;
        const auto& s = slots[h.index];
        return (s.live && s.gen == h.generation) ? s.ptr : nullptr;
    }

    std::vector<Slot<S_VertexBuffer>>   m_vbSlots;
    std::vector<Slot<S_Shader>>         m_shaderSlots;
    std::vector<Slot<S_Texture>>        m_textureSlots;
    std::vector<Slot<S_TextureSampler>> m_samplerSlots;
    std::vector<Slot<S_Mesh>>           m_meshSlots;

    std::vector<uint32_t> m_freeVB;
    std::vector<uint32_t> m_freeShader;
    std::vector<uint32_t> m_freeTexture;
    std::vector<uint32_t> m_freeSampler;
    std::vector<uint32_t> m_freeMesh;

    std::unordered_map<std::string, S_TextureHandle> m_textureCache;
    std::unordered_map<std::string, S_ShaderHandle>  m_shaderCache;
    std::unordered_map<std::string, S_MeshHandle>    m_meshCache;

    S_RenderQueue  m_queue;

    void syncMaterials();
    std::vector<S_MaterialRecord> m_materialRecords;
    std::vector<S_Texture*>       m_materialTextures; // slot-ordered
    std::unordered_map<std::string, uint32_t> m_textureSlotCache;
    bool m_materialsDirty = true;

    S_ElapsedTime m_elapsedTime;
    uint64_t m_elapsedTimeUs = 0;
    std::unique_ptr<S_RendererAPI> m_api;

};

}
