#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec4 instancePosition;
layout(location = 3) in vec4 instanceColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexcoord;

layout(set = 0, binding = 0) uniform UPerObject {
    mat4x4 WVP;
} uPerObject;

void main() {
    gl_Position = uPerObject.WVP * vec4((inPosition + instancePosition).xyz, 1.0);
    gl_Position.y = -gl_Position.y;
    outColor = instanceColor;
    outTexcoord = inTexcoord;
}
