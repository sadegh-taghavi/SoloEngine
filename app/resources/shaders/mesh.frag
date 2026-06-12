#version 450

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

void main()
{
    // faceted lighting from screen-space derivatives (no vertex normals needed)
    vec3 n = normalize(cross(dFdy(inWorldPos), dFdx(inWorldPos)));
    float light = clamp(dot(n, normalize(vec3(0.35, 0.9, 0.25))), 0.0, 1.0);
    outColor = vec4(vec3(0.9, 0.7, 0.5) * (0.35 + 0.65 * light), 1.0);
}
