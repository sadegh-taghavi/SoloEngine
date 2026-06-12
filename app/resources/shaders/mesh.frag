#version 460
#extension GL_EXT_ray_query : require

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) flat in vec4 inLightDirShadow; // xyz = light dir, w = rt toggle

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 2) uniform accelerationStructureEXT uTLAS;

void main()
{
    // faceted lighting from screen-space derivatives (no vertex normals needed)
    vec3 n = normalize(cross(dFdy(inWorldPos), dFdx(inWorldPos)));
    vec3 L = normalize(inLightDirShadow.xyz);
    float light = clamp(dot(n, L), 0.0, 1.0);

    // hard ray-traced shadow: one occlusion probe toward the light
    if (inLightDirShadow.w > 0.5 && light > 0.0)
    {
        rayQueryEXT rq;
        rayQueryInitializeEXT(rq, uTLAS,
                              gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF,
                              inWorldPos + n * 0.05, 0.05, L, 200.0);
        while (rayQueryProceedEXT(rq)) { } // traversal must run to completion
        if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT)
            light = 0.0;
    }

    outColor = vec4(vec3(0.9, 0.7, 0.5) * (0.35 + 0.65 * light), 1.0);
}
