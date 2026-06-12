#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform PerFrame {
    mat4  VP;
    float time;
    float rtShadows;
    vec2  _pad;
    vec4  lightDir;
} perFrame;

layout(set = 1, binding = 0) readonly buffer InstanceTransforms {
    mat4 data[];
} transforms;

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialID;
} pc;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) flat out vec4 outLightDirShadow;

void main()
{
    mat4 model    = transforms.data[pc.instanceIndex];
    vec4 worldPos = model * vec4(inPosition, 1.0);
    outWorldPos   = worldPos.xyz;
    outLightDirShadow = vec4(perFrame.lightDir.xyz, perFrame.rtShadows);
    gl_Position   = perFrame.VP * worldPos;
    gl_Position.y = -gl_Position.y;
}
