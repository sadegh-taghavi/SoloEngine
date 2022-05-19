#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform UPerObject {
    vec4 color;
} uPerObject;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

void main()
{
    outColor = inColor * uPerObject.color * vec4( pow( texture(texSampler, inTexcoord ).rgb, vec3(1.0f / 2.2f)) , 1.0f );
}
