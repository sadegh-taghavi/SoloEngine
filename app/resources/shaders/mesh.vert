#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform PerFrame {
    mat4  VP;
    float time;
} perFrame;

layout(set = 1, binding = 0) uniform PerObject {
    mat4 model;
} perObject;

void main()
{
    gl_Position   = perFrame.VP * perObject.model * vec4(inPosition, 1.0);
    gl_Position.y = -gl_Position.y;
}
