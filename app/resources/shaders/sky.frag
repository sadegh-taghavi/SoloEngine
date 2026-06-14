#version 450

// Samples mip 0 (the sharp level) of the unified HDR environment probe cube along
// the per-pixel view ray. Same exposure curve as mesh.frag so the background
// matches the lit scene and the reflections.
//
// The near/far points arrive in homogeneous space (exact across the triangle);
// the perspective divide happens here, per fragment, so every pixel gets the
// mathematically correct world-space ray with no interpolation warp.
//
// Note: deliberately does NOT declare the set-0 PerFrame UBO — re-declaring the
// same UBO in both vert and frag breaks the engine's reflection-merged layout.

layout(set = 1, binding = 6) uniform samplerCube uEnv;

layout(location = 0) in vec4 inNearH;
layout(location = 1) in vec4 inFarH;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 dir = normalize(inFarH.xyz / inFarH.w - inNearH.xyz / inNearH.w);
    vec3 c = textureLod(uEnv, dir, 0.0).rgb; // mip 0 = sharp sky
    c = 1.0 - exp(-c * 0.9); // match mesh.frag filmic-ish exposure
    outColor = vec4(c, 1.0);
}
