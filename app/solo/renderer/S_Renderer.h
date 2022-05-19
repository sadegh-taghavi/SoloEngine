#pragma once

#include <memory>
#include "solo/renderer/S_VertexBuffer.h"
#include "solo/renderer/S_TextureSampler.h"
#include "solo/stl/S_String.h"
#include "solo/utility/S_ElapsedTime.h"

namespace solo
{

class S_BaseApplication;
class S_RendererAPI;
class S_Scene;
class S_Shader;
class S_TextureSampler;
class S_Texture;

class S_Renderer
{

public:
    S_Renderer();
    virtual ~S_Renderer();
    S_RendererAPI *api() const;
    S_VertexBuffer *createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                       std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                       std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray);
    S_Shader *createShader(const S_String &vertexShader, const S_String &fragmentShader,
                                   const S_String &geometryShader, const S_String &computeShader);

    S_Texture *createTexture(const S_String &texture);

    S_TextureSampler *createTextureSampler(const S_TextureSamplerDescriptor &descriptor);

    void beginScene(std::shared_ptr<S_Scene> scene);
    void endScene();
    void drawFrame();
    void resize( uint32_t width, uint32_t height );
    void active(bool active);
    uint64_t elapsedTimeUs();

private:
    S_ElapsedTime m_elapsedTime;
    uint64_t m_elapsedTimeUs;
    std::unique_ptr<S_RendererAPI> m_api;

};

}

