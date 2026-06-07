#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform PerFrame {
    mat4  VP;
    float time;
} perFrame;

layout(set = 1, binding = 0) readonly buffer InstanceTransforms {
    mat4 data[];
} transforms;

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialID;
} pc;

void main()
{
    mat4 model    = transforms.data[pc.instanceIndex];
    gl_Position   = perFrame.VP * model * vec4(inPosition, 1.0);
    gl_Position.y = -gl_Position.y;
}
