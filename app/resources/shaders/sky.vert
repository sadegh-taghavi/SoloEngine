#version 450

// Background skybox: a single fullscreen triangle generated from gl_VertexIndex
// (no vertex buffer bound).

layout(set = 0, binding = 0) uniform PerFrame {
    mat4  VP;
    float time;
    float rtShadows;
    vec2  _pad;
    vec4  lightDir;
    vec4  cameraPos;
} perFrame;

layout(location = 0) out vec4 outNearH;
layout(location = 1) out vec4 outFarH;

void main()
{
    // fullscreen triangle: idx 0,1,2 -> ndc (-1,-1), (3,-1), (-1,3)
    vec2 uv  = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec2 ndc = uv * 2.0 - 1.0;

    // Unproject the near and far planes in HOMOGENEOUS space and pass them as-is.
    // Homogeneous clip coords are linear in screen space, so they interpolate
    // exactly across the triangle; the perspective divide is then done per-fragment
    // to get the correct ray for every pixel. (Interpolating the already-divided
    // direction warps it — a perspective ray is not linear in screen space.)
    // ndc.y is flipped to match the engine's gl_Position.y = -y clip-space flip.
    mat4 invVP = inverse(perFrame.VP);
    outNearH = invVP * vec4(ndc.x, -ndc.y, 0.0, 1.0);
    outFarH  = invVP * vec4(ndc.x, -ndc.y, 1.0, 1.0);

    gl_Position = vec4(ndc, 1.0, 1.0); // z = w -> depth 1.0 (far plane)
}
