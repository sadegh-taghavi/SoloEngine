#version 450

layout(location = 0) in vec3  inPosition;
layout(location = 1) in uvec4 inJoints;
layout(location = 2) in vec4  inWeights;

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

layout(set = 1, binding = 1) readonly buffer JointPalettes {
    mat4 data[];
} palettes;

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialID;
    uint paletteOffset;
} pc;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) flat out vec4 outLightDirShadow;

void main()
{
    mat4 skin = inWeights.x * palettes.data[pc.paletteOffset + inJoints.x]
              + inWeights.y * palettes.data[pc.paletteOffset + inJoints.y]
              + inWeights.z * palettes.data[pc.paletteOffset + inJoints.z]
              + inWeights.w * palettes.data[pc.paletteOffset + inJoints.w];

    mat4 model    = transforms.data[pc.instanceIndex];
    vec4 worldPos = model * skin * vec4(inPosition, 1.0);
    outWorldPos   = worldPos.xyz;
    outLightDirShadow = vec4(perFrame.lightDir.xyz, perFrame.rtShadows);
    gl_Position   = perFrame.VP * worldPos;
    gl_Position.y = -gl_Position.y;
}
