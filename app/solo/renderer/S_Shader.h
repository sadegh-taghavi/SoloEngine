#pragma once

#include "solo/stl/S_String.h"

namespace solo
{

enum class S_ShaderStage
{
    VertexShader,
    FragmentShader,
    GeometryShader,
    ComputeShader,
    Count,
};

class S_Texture;

class S_Shader
{
public:
    S_Shader();
    virtual ~S_Shader();
    virtual void bind() = 0;
    virtual void commit() = 0;
    virtual void updateUniformValue( const S_String &name, S_ShaderStage stage, const void *value ) = 0;
    virtual void updateTextureValue( const S_String &name, S_ShaderStage stage, const S_Texture &texture, uint32_t arrayIndex = 0 ) = 0;
private:


};

}

