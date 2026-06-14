#version 460
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) flat in vec4 inLightDirShadow; // xyz = light dir, w = rt toggle
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec4 inColor;
layout(location = 5) in vec3 inViewDir;
layout(location = 6) in vec4 inTangent;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 2) uniform accelerationStructureEXT uTLAS;

struct Material
{
    vec4  baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    uint  baseColorTex;
    uint  normalTex;
    uint  mrTex;
    uint  pad0, pad1, pad2;
};

layout(set = 1, binding = 3) readonly buffer Materials {
    Material data[];
} materials;

layout(set = 1, binding = 4) uniform sampler2D uTextures[64];

// Unified HDR environment probe cube (Qt/hdreditor layout): mip0 = sharp mirror,
// mips 1..4 = GGX roughness 0.25..1.0, last mip (5) = Lambertian diffuse irradiance.
layout(set = 1, binding = 6) uniform samplerCube uEnv;

// RT hit shading: per-instance geometry access through buffer references.
// rtHitData is 5 uints per vertex: uv.xy (floats), packed normal, packed
// tangent, materialID — the stream the mesh format carries for exactly this.
layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer UintBuf {
    uint data[];
};

struct ShadeRecord
{
    UintBuf indices;
    UintBuf hitData;
    uint    materialBase;
    uint    useLocalMaterial;
    uint    pad0, pad1;
};

layout(set = 1, binding = 5) readonly buffer RtShade {
    ShadeRecord data[];
} rtShade;

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialID;
} pc;

const float PI = 3.14159265359;

float distributionGGX(float NdotH, float a)
{
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-6);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float r  = roughness + 1.0;
    float k  = (r * r) / 8.0; // direct-light remapping
    float gv = NdotV / (NdotV * (1.0 - k) + k);
    float gl = NdotL / (NdotL * (1.0 - k) + k);
    return gv * gl;
}

vec3 fresnelSchlick(float VdotH, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

float shadowRay(vec3 origin, vec3 n, vec3 L)
{
    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, uTLAS,
                          gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF,
                          origin + n * 0.05, 0.05, L, 200.0);
    while (rayQueryProceedEXT(rq)) { }
    return rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT
         ? 1.0 : 0.0;
}

// Sample the unified probe cube along a world-space direction. The mip layout is
// read from the texture itself (textureQueryLevels) so it tracks whatever the
// baker produced — no hardcoded mip-count constants: specular maps roughness 0..1
// onto mips 0..(N-2), diffuse irradiance is the last mip (N-1), the sky is mip 0.
vec3 envSpecularSample(vec3 R, float roughness)
{
    return textureLod(uEnv, R, roughness * float(textureQueryLevels(uEnv) - 2)).rgb;
}

vec3 envIrradianceSample(vec3 N)
{
    return textureLod(uEnv, N, float(textureQueryLevels(uEnv) - 1)).rgb;
}

// Precomputed split-sum BRDF (DFG) LUT (pinned at material slot 1): R = scale,
// G = bias, indexed by (NdotV, roughness). Size is queried from the texture; uv is
// clamped to texel centres so the REPEAT bindless sampler never wraps at the edges.
vec2 envDFG(float roughness, float NdotV)
{
    float lutSize = float(textureSize(uTextures[1], 0).x);
    vec2  uv = clamp(vec2(NdotV, roughness), 0.5 / lutSize, 1.0 - 0.5 / lutSize);
    return textureLod(uTextures[1], uv, 0.0).rg;
}

// trace a reflection ray and shade the hit using the rtHitData stream
vec3 reflectionRay(vec3 origin, vec3 geoN, vec3 dir, vec3 L)
{
    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, uTLAS, gl_RayFlagsOpaqueEXT, 0xFF,
                          origin + geoN * 0.05, 0.05, dir, 200.0);
    while (rayQueryProceedEXT(rq)) { }
    if (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT)
        return envSpecularSample(dir, 0.1); // miss = HDR sky

    const int  instId = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true);
    const int  primId = rayQueryGetIntersectionPrimitiveIndexEXT(rq, true);
    const vec2 bary   = rayQueryGetIntersectionBarycentricsEXT(rq, true);

    ShadeRecord rec = rtShade.data[instId];

    const uint i0 = rec.indices.data[3 * primId + 0];
    const uint i1 = rec.indices.data[3 * primId + 1];
    const uint i2 = rec.indices.data[3 * primId + 2];

    const vec3 w = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);

    vec2 uv =  w.x * vec2(uintBitsToFloat(rec.hitData.data[5 * i0 + 0]), uintBitsToFloat(rec.hitData.data[5 * i0 + 1]))
            +  w.y * vec2(uintBitsToFloat(rec.hitData.data[5 * i1 + 0]), uintBitsToFloat(rec.hitData.data[5 * i1 + 1]))
            +  w.z * vec2(uintBitsToFloat(rec.hitData.data[5 * i2 + 0]), uintBitsToFloat(rec.hitData.data[5 * i2 + 1]));

    vec3 nObj = w.x * unpackSnorm4x8(rec.hitData.data[5 * i0 + 2]).xyz
              + w.y * unpackSnorm4x8(rec.hitData.data[5 * i1 + 2]).xyz
              + w.z * unpackSnorm4x8(rec.hitData.data[5 * i2 + 2]).xyz;

    mat4x3 objToWorld = rayQueryGetIntersectionObjectToWorldEXT(rq, true);
    vec3 N = normalize(mat3(objToWorld) * nObj);
    N = faceforward(N, dir, N);

    uint matID = rec.materialBase;
    if (rec.useLocalMaterial != 0u)
        matID += rec.hitData.data[5 * i0 + 4];

    Material mat = materials.data[nonuniformEXT(matID)];
    vec3 albedo  = texture(uTextures[nonuniformEXT(mat.baseColorTex)], uv).rgb
                 * mat.baseColorFactor.rgb;

    // single-bounce lambert lit by sun + irradiance; no secondary shadow ray
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    return albedo * (envIrradianceSample(N) + vec3(2.2) * NdotL);
}

void main()
{
    Material mat = materials.data[pc.materialID];

    vec4 baseColor = texture(uTextures[mat.baseColorTex], inUV) * mat.baseColorFactor * inColor;
    vec3 albedo    = baseColor.rgb;

    float metallic  = mat.metallicFactor;
    float roughness = mat.roughnessFactor;
    if (mat.mrTex != 0u)
    {
        vec2 mr   = texture(uTextures[mat.mrTex], inUV).bg; // glTF: B = metallic, G = roughness
        metallic  *= mr.x;
        roughness *= mr.y;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 N = normalize(inNormal);
    if (mat.normalTex != 0u)
    {
        vec3 T   = normalize(inTangent.xyz - N * dot(N, inTangent.xyz));
        vec3 B   = cross(N, T) * inTangent.w;
        vec3 nTS = texture(uTextures[mat.normalTex], inUV).xyz * 2.0 - 1.0;
        N = normalize(mat3(T, B, N) * nTS);
    }

    vec3 V = normalize(inViewDir);
    vec3 L = normalize(inLightDirShadow.xyz);
    vec3 H = normalize(V + L);

    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotV = clamp(dot(N, V), 1e-4, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float VdotH = clamp(dot(V, H), 0.0, 1.0);

    float a  = roughness * roughness;
    vec3  F0 = mix(vec3(0.04), albedo, metallic);

    vec3  F    = fresnelSchlick(VdotH, F0);
    float D    = distributionGGX(NdotH, a);
    float G    = geometrySmith(NdotV, NdotL, roughness);
    vec3  spec = F * (D * G / max(4.0 * NdotV * NdotL, 1e-4));
    vec3  kd   = (1.0 - F) * (1.0 - metallic);

    float shadow = 1.0;
    if (inLightDirShadow.w > 0.5 && NdotL > 0.0)
        shadow = shadowRay(inWorldPos, normalize(inNormal), L);

    const vec3 lightColor = vec3(2.4); // sun; ambient comes from the HDR environment

    vec3 color = (kd * albedo / PI + spec) * lightColor * NdotL * shadow;

    // Image-based lighting with Fdez-Aguera multiple-scattering energy compensation:
    // single-scatter GGX loses energy at high roughness (rough metals go dark), so
    // the FmsEms term feeds the lost energy back, tinted by the average Fresnel.
    vec3 R = reflect(-V, N);
    vec3 specEnv;
    if (inLightDirShadow.w > 0.5 && roughness < 0.35)
        specEnv = reflectionRay(inWorldPos, normalize(inNormal), R, L); // sharp: trace the scene
    else
        specEnv = envSpecularSample(R, roughness);                      // glossy: prefiltered env

    vec3  irradiance = envIrradianceSample(N);
    vec2  dfg    = envDFG(roughness, NdotV);
    vec3  FssEss = F0 * dfg.x + dfg.y;                     // single-scatter specular
    float Ems    = 1.0 - (dfg.x + dfg.y);                  // energy missing after one bounce
    vec3  Favg   = F0 + (1.0 - F0) * (1.0 / 21.0);         // average Fresnel
    vec3  FmsEms = (Ems * FssEss * Favg) / (1.0 - Favg * Ems);
    vec3  kD     = albedo * (1.0 - metallic) * (1.0 - FssEss - FmsEms);
    color += FssEss * specEnv + (FmsEms + kD) * irradiance;

    color = 1.0 - exp(-color * 0.9); // filmic-ish exposure curve
    outColor = vec4(color, baseColor.a);
}
