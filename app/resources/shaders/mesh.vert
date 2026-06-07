#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform UPerObject {
    mat4x4 WVP;
} uPerObject;

void main()
{
    gl_Position = uPerObject.WVP * vec4(inPosition, 1.0);
    gl_Position.y = -gl_Position.y;
}
