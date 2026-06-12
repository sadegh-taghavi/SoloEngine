#version 450

layout(location = 0) in vec3  inPosition;
layout(location = 1) in uvec4 inJoints;
layout(location = 2) in vec4  inWeights;
layout(location = 3) in vec2  inUV;
layout(location = 4) in vec4  inNormalSign; // snorm8: xyz normal, w tangent sign
layout(location = 5) in vec4  inTangent;    // snorm8
layout(location = 6) in vec4  inColor;      // unorm8

layout(set = 0, binding = 0) uniform PerFrame {
    mat4  VP;
    float time;
    float rtShadows;
    vec2  _pad;
    vec4  lightDir;
    vec4  cameraPos;
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
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec2 outUV;
layout(location = 4) out vec4 outColor;
layout(location = 5) out vec3 outViewDir;
layout(location = 6) out vec4 outTangent; // xyz world tangent, w handedness

void main()
{
    mat4 skin = inWeights.x * palettes.data[pc.paletteOffset + inJoints.x]
              + inWeights.y * palettes.data[pc.paletteOffset + inJoints.y]
              + inWeights.z * palettes.data[pc.paletteOffset + inJoints.z]
              + inWeights.w * palettes.data[pc.paletteOffset + inJoints.w];

    mat4 model    = transforms.data[pc.instanceIndex];
    mat3 nrmMat   = mat3(model) * mat3(skin);
    vec4 worldPos = model * skin * vec4(inPosition, 1.0);
    outWorldPos   = worldPos.xyz;
    outNormal     = normalize(nrmMat * inNormalSign.xyz);
    outTangent    = vec4(normalize(nrmMat * inTangent.xyz), inNormalSign.w);
    outUV         = inUV;
    outColor      = inColor;
    outViewDir    = perFrame.cameraPos.xyz - worldPos.xyz;
    outLightDirShadow = vec4(perFrame.lightDir.xyz, perFrame.rtShadows);
    gl_Position   = perFrame.VP * worldPos;
    gl_Position.y = -gl_Position.y;
}
