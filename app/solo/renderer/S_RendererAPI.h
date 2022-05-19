#pragma once
#include "solo/stl/S_String.h"
#include "solo/renderer/S_TextureSampler.h"
#include <stdint.h>
#include <memory>

namespace solo
{
class S_BaseApplication;
struct S_VertexBufferDescriptorArray;
class S_VertexBuffer;
class S_Shader;
class S_Texture;
class S_TextureSampler;

class S_RendererAPI
{
public:
    S_RendererAPI();
    virtual S_VertexBuffer *createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                               std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                               std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray) = 0;
    virtual S_Shader *createShader(const S_String &vertexShader, const S_String &fragmentShader,
                                   const S_String &geometryShader, const S_String &computeShader) = 0;
    virtual S_Texture *createTexture(const S_String &texture) = 0;
    virtual S_TextureSampler *createTextureSampler(const S_TextureSamplerDescriptor &descriptor) = 0;

    virtual void drawFrame() = 0;
    virtual void resize( uint32_t, uint32_t ){}
    virtual void active( bool ){}
    virtual ~S_RendererAPI();

private:

};

}

