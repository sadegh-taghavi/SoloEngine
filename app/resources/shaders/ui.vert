#version 450

layout(location = 0) in vec2  inPos;     // pixels, top-left origin
layout(location = 1) in vec2  inUV;
layout(location = 2) in vec4  inColor;
layout(location = 3) in float inMode;    // 0 = sprite atlas, 1 = SDF font

layout(location = 0) out vec2  outUV;
layout(location = 1) out vec4  outColor;
layout(location = 2) out float outMode;

layout(push_constant) uniform PC {
    vec2 screenSize;
} pc;

void main()
{
    // pixel space -> Vulkan NDC (y down): no flip needed
    gl_Position = vec4(inPos / pc.screenSize * 2.0 - 1.0, 0.0, 1.0);
    outUV    = inUV;
    outColor = inColor;
    outMode  = inMode;
}
