#pragma once
#include "solo/renderer/S_VertexBuffer.h"

namespace solo
{

class S_Shader;

struct S_PipelineDescriptor
{
    S_VertexBufferDescriptorArray VertexBufferDescriptorArray;
    S_VertexBufferDescriptorArray InstanceBufferDescriptorArray;
    S_VertexBufferDescriptorArray SkinBufferDescriptorArray;     // second per-vertex stream (binding 1)
    S_Shader*                     Shader              = nullptr;
    bool                          UseEngineGlobals    = false;
};

}
