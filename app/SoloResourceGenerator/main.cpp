#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <array>
#include <thread>
#include "solo/platforms/S_SystemDetect.h"
#include "solo/renderer/vulkan/S_VulkanShaderReflection.h"
#include "rapidjson/document.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_USE_RAPIDJSON
#include <tiny_gltf.h>

#include "solo/mesh/S_MeshBin.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>
#include "solo/ui/S_UIBin.h"

using namespace rapidjson;

namespace fs = std::filesystem;

static constexpr uint32_t PACK_MAGIC   = 0x4b415053;
static constexpr uint32_t PACK_VERSION = 1;

#pragma pack(push, 1)
struct S_PackHeader { uint32_t magic; uint32_t version; uint32_t count; uint32_t reserved; };
struct S_PackEntry  { char name[256]; uint64_t offset; uint64_t size; };
#pragma pack(pop)

static uint16_t floatToHalf(float f)
{
    uint32_t x; memcpy(&x, &f, 4);
    uint32_t sign = (x >> 16) & 0x8000;
    int32_t  exp  = static_cast<int32_t>((x >> 23) & 0xFF) - 127 + 15;
    uint32_t man  = (x >> 13) & 0x3FF;
    if (exp <= 0)  return static_cast<uint16_t>(sign);
    if (exp >= 31) return static_cast<uint16_t>(sign | 0x7C00);
    return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exp) << 10) | man);
}

// KTX1 with RGBA16F mips (equirect environment maps)
static bool writeKtxRgba16fMips(const std::string& path,
                                const std::vector<std::vector<float>>& mipsRGB, // 3 floats per texel
                                uint32_t baseW, uint32_t baseH)
{
    static const uint8_t ident[12] = { 0xAB,0x4B,0x54,0x58,0x20,0x31,0x31,0xBB,0x0D,0x0A,0x1A,0x0A };
    const uint32_t hdr[13] = {
        0x04030201,
        0x140B,     // glType GL_HALF_FLOAT
        2,          // glTypeSize
        0x1908,     // glFormat GL_RGBA
        0x881A,     // glInternalFormat GL_RGBA16F
        0x1908,     // glBaseInternalFormat
        baseW, baseH,
        0, 0, 1,
        static_cast<uint32_t>(mipsRGB.size()),
        0
    };
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(ident), 12);
    f.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));

    uint32_t w = baseW, h = baseH;
    for (const auto& mip : mipsRGB)
    {
        std::vector<uint16_t> half(static_cast<size_t>(w) * h * 4);
        for (uint32_t t = 0; t < w * h; ++t)
        {
            half[t * 4 + 0] = floatToHalf(mip[t * 3 + 0]);
            half[t * 4 + 1] = floatToHalf(mip[t * 3 + 1]);
            half[t * 4 + 2] = floatToHalf(mip[t * 3 + 2]);
            half[t * 4 + 3] = floatToHalf(1.0f);
        }
        const uint32_t imageSize = w * h * 8;
        f.write(reinterpret_cast<const char*>(&imageSize), 4);
        f.write(reinterpret_cast<const char*>(half.data()), imageSize);
        w = w > 1 ? w / 2 : 1;
        h = h > 1 ? h / 2 : 1;
    }
    return f.good();
}

// KTX1 cubemap with RGBA16F mips. mipFaces[mip][face] holds 3 floats/texel; faces
// are in Vulkan/KTX order (+X,-X,+Y,-Y,+Z,-Z). imageSize per the KTX1 spec is the
// size of ONE face (RGBA16F is 4-byte aligned, so no cube/mip padding is needed).
static bool writeKtxCubeRgba16fMips(const std::string& path,
                                    const std::vector<std::vector<std::vector<float>>>& mipFaces,
                                    uint32_t baseW, uint32_t baseH)
{
    static const uint8_t ident[12] = { 0xAB,0x4B,0x54,0x58,0x20,0x31,0x31,0xBB,0x0D,0x0A,0x1A,0x0A };
    const uint32_t hdr[13] = {
        0x04030201,
        0x140B,     // glType GL_HALF_FLOAT
        2,          // glTypeSize
        0x1908,     // glFormat GL_RGBA
        0x881A,     // glInternalFormat GL_RGBA16F
        0x1908,     // glBaseInternalFormat
        baseW, baseH,
        0, 0, 6,    // depth, arrayElements, numberOfFaces (6 = cubemap)
        static_cast<uint32_t>(mipFaces.size()),
        0
    };
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(ident), 12);
    f.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));

    uint32_t w = baseW, h = baseH;
    for (const auto& faces : mipFaces)
    {
        const uint32_t faceSize = w * h * 8; // RGBA16F bytes for one face
        f.write(reinterpret_cast<const char*>(&faceSize), 4);
        for (const auto& faceRGB : faces) // 6 faces
        {
            std::vector<uint16_t> half(static_cast<size_t>(w) * h * 4);
            for (uint32_t t = 0; t < w * h; ++t)
            {
                half[t * 4 + 0] = floatToHalf(faceRGB[t * 3 + 0]);
                half[t * 4 + 1] = floatToHalf(faceRGB[t * 3 + 1]);
                half[t * 4 + 2] = floatToHalf(faceRGB[t * 3 + 2]);
                half[t * 4 + 3] = floatToHalf(1.0f);
            }
            f.write(reinterpret_cast<const char*>(half.data()), faceSize);
        }
        w = w > 1 ? w / 2 : 1;
        h = h > 1 ? h / 2 : 1;
    }
    return f.good();
}

// KTX1 RGBA8 with a full box-filtered mip chain — loadable by libktx
static bool writeKtxRgba8(const std::string& path, const uint8_t* pixels, uint32_t w, uint32_t h)
{
    uint32_t mipCount = 1;
    for (uint32_t mw = w, mh = h; mw > 1 || mh > 1; mw = mw > 1 ? mw / 2 : 1, mh = mh > 1 ? mh / 2 : 1)
        ++mipCount;

    static const uint8_t ident[12] = { 0xAB,0x4B,0x54,0x58,0x20,0x31,0x31,0xBB,0x0D,0x0A,0x1A,0x0A };
    const uint32_t hdr[13] = {
        0x04030201, // endianness
        0x1401,     // glType GL_UNSIGNED_BYTE
        1,          // glTypeSize
        0x1908,     // glFormat GL_RGBA
        0x8058,     // glInternalFormat GL_RGBA8
        0x1908,     // glBaseInternalFormat
        w, h,
        0, 0,       // depth, arrayElements
        1, mipCount,
        0           // keyValue bytes
    };
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(ident), 12);
    f.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));

    std::vector<uint8_t> cur(pixels, pixels + static_cast<size_t>(w) * h * 4);
    uint32_t cw = w, ch = h;
    for (uint32_t m = 0; m < mipCount; ++m)
    {
        const uint32_t imageSize = cw * ch * 4;
        f.write(reinterpret_cast<const char*>(&imageSize), 4);
        f.write(reinterpret_cast<const char*>(cur.data()), imageSize);

        if (m + 1 == mipCount) break;
        const uint32_t nw = cw > 1 ? cw / 2 : 1, nh = ch > 1 ? ch / 2 : 1;
        std::vector<uint8_t> next(static_cast<size_t>(nw) * nh * 4);
        for (uint32_t y = 0; y < nh; ++y)
            for (uint32_t x = 0; x < nw; ++x)
                for (uint32_t c = 0; c < 4; ++c)
                {
                    const uint32_t x0 = x * 2, y0 = y * 2;
                    const uint32_t x1 = cw > 1 ? x0 + 1 : x0, y1 = ch > 1 ? y0 + 1 : y0;
                    const uint32_t sum = cur[(y0 * cw + x0) * 4 + c] + cur[(y0 * cw + x1) * 4 + c]
                                       + cur[(y1 * cw + x0) * 4 + c] + cur[(y1 * cw + x1) * 4 + c];
                    next[(y * nw + x) * 4 + c] = static_cast<uint8_t>(sum / 4);
                }
        cur.swap(next);
        cw = nw; ch = nh;
    }
    return f.good();
}

// ---- HDR environment baking: equirect prefiltered specular + irradiance ----

namespace envbake
{
struct Img { std::vector<float> rgb; uint32_t w = 0, h = 0; };

static glm::vec3 sampleBilinear(const Img& img, float u, float v)
{
    u = u - std::floor(u); // wrap horizontally
    v = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    float fx = u * img.w - 0.5f, fy = v * img.h - 0.5f;
    int x0 = static_cast<int>(std::floor(fx)), y0 = static_cast<int>(std::floor(fy));
    float tx = fx - x0, ty = fy - y0;
    auto texel = [&](int x, int y) {
        x = (x % static_cast<int>(img.w) + img.w) % img.w;
        y = y < 0 ? 0 : (y >= static_cast<int>(img.h) ? img.h - 1 : y);
        const float* p = &img.rgb[(static_cast<size_t>(y) * img.w + x) * 3];
        return glm::vec3(p[0], p[1], p[2]);
    };
    return glm::mix(glm::mix(texel(x0, y0),     texel(x0 + 1, y0),     tx),
                    glm::mix(texel(x0, y0 + 1), texel(x0 + 1, y0 + 1), tx), ty);
}

// World direction for a cube-face texel. s,t in [0,1] with t increasing downward
// (row 0 = top). Standard Vulkan/D3D cube convention, inverted from the sampler's
// major-axis selection so generation matches hardware sampling exactly.
static glm::vec3 dirFromCubeFace(int face, float s, float t)
{
    const float sc = 2.0f * s - 1.0f;
    const float tc = 2.0f * t - 1.0f;
    glm::vec3 d;
    switch (face)
    {
        case 0: d = glm::vec3( 1.0f,  -tc,  -sc); break; // +X
        case 1: d = glm::vec3(-1.0f,  -tc,   sc); break; // -X
        case 2: d = glm::vec3(  sc,  1.0f,   tc); break; // +Y
        case 3: d = glm::vec3(  sc, -1.0f,  -tc); break; // -Y
        case 4: d = glm::vec3(  sc,  -tc,  1.0f); break; // +Z
        default:d = glm::vec3( -sc,  -tc, -1.0f); break; // -Z
    }
    return glm::normalize(d);
}

static glm::vec2 equirectFromDir(const glm::vec3& d)
{
    return { std::atan2(d.z, d.x) / (2.0f * 3.14159265f) + 0.5f,
             std::acos(glm::clamp(d.y, -1.0f, 1.0f)) / 3.14159265f };
}

static float radicalInverse(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
}

static void basis(const glm::vec3& n, glm::vec3& t, glm::vec3& b)
{
    t = std::fabs(n.y) < 0.99f ? glm::normalize(glm::cross(glm::vec3(0, 1, 0), n))
                               : glm::vec3(1, 0, 0);
    b = glm::cross(n, t);
}

// Renders one square cube face (dim x dim, 3 floats/texel) by evaluating shade(N)
// at the world direction of each texel center.
template <class Shade>
static std::vector<float> renderCubeFace(int face, uint32_t dim, Shade&& shade)
{
    std::vector<float> out(static_cast<size_t>(dim) * dim * 3);
    for (uint32_t y = 0; y < dim; ++y)
        for (uint32_t x = 0; x < dim; ++x)
        {
            const glm::vec3 N = dirFromCubeFace(face, (x + 0.5f) / dim, (y + 0.5f) / dim);
            const glm::vec3 c = shade(N);
            out[(static_cast<size_t>(y) * dim + x) * 3 + 0] = c.x;
            out[(static_cast<size_t>(y) * dim + x) * 3 + 1] = c.y;
            out[(static_cast<size_t>(y) * dim + x) * 3 + 2] = c.z;
        }
    return out;
}

// Renders all six faces concurrently (each face is independent; shade() only reads
// shared source data). Faces returned in +X,-X,+Y,-Y,+Z,-Z order.
template <class Shade>
static std::array<std::vector<float>, 6> renderCubeFaces(uint32_t dim, Shade&& shade)
{
    std::array<std::vector<float>, 6> faces;
    std::array<std::thread, 6> th;
    for (int f = 0; f < 6; ++f)
        th[f] = std::thread([&faces, dim, f, &shade]() { faces[f] = renderCubeFace(f, dim, shade); });
    for (auto& t : th) t.join();
    return faces;
}

// A cubemap held CPU-side as six square faces (3 floats/texel).
struct CubeImg { uint32_t dim = 0; std::vector<float> faces[6]; };

// Resample the equirect source into a sharp cube (mip 0 of the prefilter source).
static CubeImg buildCube(const Img& eq, uint32_t dim)
{
    CubeImg c; c.dim = dim;
    auto out = renderCubeFaces(dim, [&](const glm::vec3& N) {
        const glm::vec2 uv = equirectFromDir(N);
        return sampleBilinear(eq, uv.x, uv.y);
    });
    for (int f = 0; f < 6; ++f) c.faces[f] = std::move(out[f]);
    return c;
}

// Per-face box-filtered mip chain down to 1x1 (matches GPU generateMips: faces are
// filtered independently). Karis importance sampling reads from these levels.
static std::vector<CubeImg> buildCubeMipChain(const CubeImg& base)
{
    std::vector<CubeImg> mips;
    mips.push_back(base);
    while (mips.back().dim > 1)
    {
        const CubeImg& s = mips.back();
        CubeImg d; d.dim = std::max(1u, s.dim / 2);
        for (int f = 0; f < 6; ++f)
        {
            d.faces[f].resize(static_cast<size_t>(d.dim) * d.dim * 3);
            for (uint32_t y = 0; y < d.dim; ++y)
                for (uint32_t x = 0; x < d.dim; ++x)
                {
                    const uint32_t x0 = x * 2, y0 = y * 2;
                    const uint32_t x1 = std::min(x0 + 1, s.dim - 1), y1 = std::min(y0 + 1, s.dim - 1);
                    for (int ch = 0; ch < 3; ++ch)
                        d.faces[f][(static_cast<size_t>(y) * d.dim + x) * 3 + ch] = 0.25f *
                            (s.faces[f][(static_cast<size_t>(y0) * s.dim + x0) * 3 + ch] + s.faces[f][(static_cast<size_t>(y0) * s.dim + x1) * 3 + ch]
                           + s.faces[f][(static_cast<size_t>(y1) * s.dim + x0) * 3 + ch] + s.faces[f][(static_cast<size_t>(y1) * s.dim + x1) * 3 + ch]);
                }
        }
        mips.push_back(std::move(d));
    }
    return mips;
}

// Bilinear cube sample: select the major-axis face (inverse of dirFromCubeFace),
// then bilinear within the face (clamp-to-edge).
static glm::vec3 sampleCubeBilinear(const CubeImg& c, const glm::vec3& d)
{
    const float ax = std::fabs(d.x), ay = std::fabs(d.y), az = std::fabs(d.z);
    int face; float sc, tc, ma;
    if (ax >= ay && ax >= az) { ma = ax; if (d.x >= 0.f) { face = 0; sc = -d.z; tc = -d.y; } else { face = 1; sc =  d.z; tc = -d.y; } }
    else if (ay >= az)        { ma = ay; if (d.y >= 0.f) { face = 2; sc =  d.x; tc =  d.z; } else { face = 3; sc =  d.x; tc = -d.z; } }
    else                      { ma = az; if (d.z >= 0.f) { face = 4; sc =  d.x; tc = -d.y; } else { face = 5; sc = -d.x; tc = -d.y; } }
    const int   dim = static_cast<int>(c.dim);
    const float s = 0.5f * (sc / ma + 1.0f), t = 0.5f * (tc / ma + 1.0f);
    const float fx = s * c.dim - 0.5f, fy = t * c.dim - 0.5f;
    const int   x0 = static_cast<int>(std::floor(fx)), y0 = static_cast<int>(std::floor(fy));
    const float txf = fx - x0, tyf = fy - y0;
    auto cl = [dim](int v) { return v < 0 ? 0 : (v >= dim ? dim - 1 : v); };
    auto px = [&](int x, int y) { const float* p = &c.faces[face][(static_cast<size_t>(cl(y)) * dim + cl(x)) * 3]; return glm::vec3(p[0], p[1], p[2]); };
    return glm::mix(glm::mix(px(x0, y0), px(x0 + 1, y0), txf), glm::mix(px(x0, y0 + 1), px(x0 + 1, y0 + 1), txf), tyf);
}

static glm::vec3 sampleCubeLod(const std::vector<CubeImg>& mips, const glm::vec3& d, float lod)
{
    const int maxm = static_cast<int>(mips.size()) - 1;
    lod = lod < 0.f ? 0.f : (lod > static_cast<float>(maxm) ? static_cast<float>(maxm) : lod);
    const int   m0 = static_cast<int>(std::floor(lod));
    const int   m1 = std::min(m0 + 1, maxm);
    const float f  = lod - static_cast<float>(m0);
    return glm::mix(sampleCubeBilinear(mips[m0], d), sampleCubeBilinear(mips[m1], d), f);
}

// GGX-prefiltered radiance for direction N (V = N), Karis importance sampling the
// cube mip chain. saTexel uses 6 faces, matching Qt/hdreditor's computeLod.
static glm::vec3 prefilterDir(const std::vector<CubeImg>& mips, const glm::vec3& N, float roughness, uint32_t samples)
{
    const float a = roughness * roughness;
    glm::vec3 T, B; basis(N, T, B);
    glm::vec3 sum(0.0f); float wsum = 0.0f;
    // Matches hdreditor/Qt prefilter.frag computeLod = 0.5*log2(6*res^2/(N*pdf)):
    // omits the 4pi of textbook Karis omegaP on purpose, biasing each sample to a
    // blurrier source mip so an intense sun is pre-averaged instead of ringing.
    const float saTexel = 1.0f / (6.0f * static_cast<float>(mips[0].dim) * static_cast<float>(mips[0].dim));
    for (uint32_t s = 0; s < samples; ++s)
    {
        const float u1 = (s + 0.5f) / samples, u2 = radicalInverse(s);
        const float phi      = 2.0f * 3.14159265f * u1;
        const float cosTheta = std::sqrt((1.0f - u2) / (1.0f + (a * a - 1.0f) * u2));
        const float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
        const glm::vec3 Hl(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
        const glm::vec3 H = T * Hl.x + B * Hl.y + N * Hl.z;
        const glm::vec3 L = glm::normalize(2.0f * glm::dot(N, H) * H - N); // V = N
        const float NdotL = glm::dot(N, L);
        if (NdotL <= 0.0f) continue;
        const float NoH = cosTheta;
        const float D   = (a * a) / (3.14159265f * std::pow(NoH * NoH * (a * a - 1.0f) + 1.0f, 2.0f) + 1e-8f);
        const float saSample = 1.0f / (static_cast<float>(samples) * (D * 0.25f) + 1e-8f);
        const float lod = roughness <= 0.0f ? 0.0f : std::max(0.0f, 0.5f * std::log2(saSample / saTexel));
        sum  += sampleCubeLod(mips, L, lod) * NdotL;
        wsum += NdotL;
    }
    if (wsum > 0.0f) return sum / wsum;
    return sampleCubeLod(mips, N, 0.0f);
}

// Cosine-weighted diffuse irradiance for direction N (Lambertian distribution),
// same Karis importance sampling against the cube mip chain.
static glm::vec3 irradianceDir(const std::vector<CubeImg>& mips, const glm::vec3& N, uint32_t samples)
{
    glm::vec3 T, B; basis(N, T, B);
    glm::vec3 sum(0.0f);
    // Matches hdreditor/Qt prefilter.frag computeLod = 0.5*log2(6*res^2/(N*pdf)):
    // omits the 4pi of textbook Karis omegaP on purpose, biasing each sample to a
    // blurrier source mip so an intense sun is pre-averaged instead of ringing.
    const float saTexel = 1.0f / (6.0f * static_cast<float>(mips[0].dim) * static_cast<float>(mips[0].dim));
    for (uint32_t s = 0; s < samples; ++s)
    {
        const float u1 = (s + 0.5f) / samples, u2 = radicalInverse(s);
        const float phi = 2.0f * 3.14159265f * u1;
        const float ct  = std::sqrt(1.0f - u2); // cosine-weighted
        const float st  = std::sqrt(u2);
        const glm::vec3 L = glm::normalize(T * (st * std::cos(phi)) + B * (st * std::sin(phi)) + N * ct);
        const float pdf = ct / 3.14159265f + 1e-8f;
        const float saSample = 1.0f / (static_cast<float>(samples) * pdf);
        const float lod = std::max(0.0f, 0.5f * std::log2(saSample / saTexel));
        sum += sampleCubeLod(mips, L, lod);
    }
    return sum / static_cast<float>(samples);
}
}

static bool bakeEnvironment(const std::string& hdrPath, const std::string& outProbe)
{
    int w = 0, h = 0, comp = 0;
    float* data = stbi_loadf(hdrPath.c_str(), &w, &h, &comp, 3);
    if (!data)
    {
        std::cout << "HDR load failed: " << hdrPath << std::endl;
        return false;
    }
    envbake::Img src;
    src.w = static_cast<uint32_t>(w);
    src.h = static_cast<uint32_t>(h);
    src.rgb.assign(data, data + static_cast<size_t>(w) * h * 3);
    stbi_image_free(data);

    // auto-exposure: HDRIs come in arbitrary calibrations; normalize average
    // luminance to ~0.8 so the shader's exposure curve sees scene-scale values
    double lumSum = 0.0;
    for (size_t t = 0; t < src.rgb.size(); t += 3)
        lumSum += 0.2126 * src.rgb[t] + 0.7152 * src.rgb[t + 1] + 0.0722 * src.rgb[t + 2];
    const float avgLum = static_cast<float>(lumSum / (src.rgb.size() / 3));
    if (avgLum > 1e-6f)
    {
        const float scale = std::min(16.0f, std::max(0.25f, 0.8f / avgLum));
        for (float& v : src.rgb) v *= scale;
        std::cout << "  env auto-exposure: avg " << avgLum << " -> scale " << scale << std::endl;
    }

    // Unified environment probe cube (Qt / hdreditor layout): one RGBA16F cubemap
    // whose mip chain holds everything. The Karis prefilter reads from a CUBE source
    // (equirect -> sharp cube -> per-face mip chain) for uniform per-texel solid
    // angle, matching Qt's createEnvironmentMap / hdreditor's prefilter.frag.
    //   mip 0           = sharp mirror (skybox + sharp reflections)
    //   mips 1..kMips-2 = GGX specular, roughness = mip/(kMips-2) -> 0..1
    //   mip kMips-1     = Lambertian diffuse irradiance (last, smallest mip)
    // Consumers: specular lod = roughness*(kMips-2), irradiance lod = kMips-1, sky lod = 0.
    constexpr uint32_t kBase = 512, kMips = 6, kSamples = 1024; // mip5 (16x16) = irradiance
    const envbake::CubeImg              base     = envbake::buildCube(src, kBase);
    const std::vector<envbake::CubeImg> cubeMips = envbake::buildCubeMipChain(base);

    std::vector<std::vector<std::vector<float>>> mipFaces(kMips);
    for (uint32_t m = 0; m < kMips; ++m)
    {
        const uint32_t dim = std::max(1u, kBase >> m);
        if (m == 0) // sharp mirror = the base cube itself (skybox + sharp reflections)
        {
            mipFaces[0] = { base.faces[0], base.faces[1], base.faces[2],
                            base.faces[3], base.faces[4], base.faces[5] };
            continue;
        }
        std::array<std::vector<float>, 6> faces;
        if (m == kMips - 1) // diffuse irradiance in the last mip
            faces = envbake::renderCubeFaces(dim, [&](const glm::vec3& N) {
                return envbake::irradianceDir(cubeMips, N, kSamples);
            });
        else
        {
            const float roughness = static_cast<float>(m) / static_cast<float>(kMips - 2); // .25,.5,.75,1
            faces = envbake::renderCubeFaces(dim, [&](const glm::vec3& N) {
                return envbake::prefilterDir(cubeMips, N, roughness, kSamples);
            });
        }
        mipFaces[m] = { faces[0], faces[1], faces[2], faces[3], faces[4], faces[5] };
    }
    if (!writeKtxCubeRgba16fMips(outProbe, mipFaces, kBase, kBase))
        return false;

    std::cout << "Baked environment probe cube (cube-source Karis, " << kSamples << " spp): "
              << outProbe << std::endl;
    return true;
}

// Split-sum BRDF (DFG) integration: returns (scale, bias) so specular IBL =
// prefilteredColor * (F0*scale + bias). Standard Karis/Khronos integration using
// the SAME GGX importance sampling (alpha = roughness^2) as the prefilter, with the
// IBL geometry remap k = roughness^2/2. Env-independent — purely a function of
// (NdotV, roughness).
static glm::vec2 integrateBRDF(float NdotV, float roughness, uint32_t samples)
{
    NdotV = std::max(NdotV, 1e-4f);
    const glm::vec3 V(std::sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);
    const float a = roughness * roughness;          // alpha (GGX sampling)
    const float k = (roughness * roughness) / 2.0f; // IBL geometry term
    float A = 0.0f, B = 0.0f;
    for (uint32_t i = 0; i < samples; ++i)
    {
        const float u1 = (i + 0.5f) / samples, u2 = envbake::radicalInverse(i);
        const float phi  = 2.0f * 3.14159265f * u1;
        const float cosT = std::sqrt((1.0f - u2) / (1.0f + (a * a - 1.0f) * u2));
        const float sinT = std::sqrt(1.0f - cosT * cosT);
        const glm::vec3 H(sinT * std::cos(phi), sinT * std::sin(phi), cosT);
        const glm::vec3 L = 2.0f * glm::dot(V, H) * H - V;
        const float NdotL = std::max(L.z, 0.0f);
        const float NdotH = std::max(H.z, 0.0f);
        const float VdotH = std::max(glm::dot(V, H), 0.0f);
        if (NdotL > 0.0f)
        {
            const float gv   = NdotV / (NdotV * (1.0f - k) + k);
            const float gl   = NdotL / (NdotL * (1.0f - k) + k);
            const float gVis = (gv * gl * VdotH) / (NdotH * NdotV);
            const float Fc   = std::pow(1.0f - VdotH, 5.0f);
            A += (1.0f - Fc) * gVis;
            B += Fc * gVis;
        }
    }
    return glm::vec2(A / static_cast<float>(samples), B / static_cast<float>(samples));
}

// Bakes the env-independent BRDF LUT to a 2D RGBA16F KTX (R=scale, G=bias),
// indexed by (NdotV on X, roughness on Y). Rows split across threads.
static bool bakeBrdfLut(const std::string& outPath)
{
    constexpr uint32_t kSize = 256, kSamples = 1024;
    std::vector<float> rgb(static_cast<size_t>(kSize) * kSize * 3, 0.0f);
    const unsigned nthreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    auto worker = [&](uint32_t y0, uint32_t y1) {
        for (uint32_t y = y0; y < y1; ++y)
            for (uint32_t x = 0; x < kSize; ++x)
            {
                const glm::vec2 ab = integrateBRDF((x + 0.5f) / kSize, (y + 0.5f) / kSize, kSamples);
                rgb[(static_cast<size_t>(y) * kSize + x) * 3 + 0] = ab.x;
                rgb[(static_cast<size_t>(y) * kSize + x) * 3 + 1] = ab.y;
            }
    };
    const uint32_t chunk = (kSize + nthreads - 1) / nthreads;
    for (uint32_t t = 0; t < nthreads; ++t)
    {
        const uint32_t y0 = t * chunk, y1 = std::min(kSize, y0 + chunk);
        if (y0 < y1) pool.emplace_back(worker, y0, y1);
    }
    for (auto& th : pool) th.join();

    std::vector<std::vector<float>> mips{ std::move(rgb) };
    if (!writeKtxRgba16fMips(outPath, mips, kSize, kSize))
        return false;
    std::cout << "Baked BRDF LUT: " << outPath << std::endl;
    return true;
}

static int8_t packSnorm8(float v)
{
    v = v < -1.f ? -1.f : (v > 1.f ? 1.f : v);
    return static_cast<int8_t>(v * 127.f);
}

static uint8_t packUnorm8(float v)
{
    v = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    return static_cast<uint8_t>(v * 255.f);
}

struct GltfAttr
{
    const uint8_t* base   = nullptr;
    int            stride = 0;
    uint32_t       count  = 0;

    bool valid() const { return base != nullptr; }

    const float* f3(uint32_t i) const { return reinterpret_cast<const float*>(base + i * stride); }
    const float* f4(uint32_t i) const { return reinterpret_cast<const float*>(base + i * stride); }
    const float* f2(uint32_t i) const { return reinterpret_cast<const float*>(base + i * stride); }
};

static GltfAttr getAttr(const tinygltf::Model& model, int accIdx)
{
    if (accIdx < 0) return {};
    auto& acc = model.accessors[accIdx];
    auto& bv  = model.bufferViews[acc.bufferView];
    int stride = acc.ByteStride(bv);
    if (stride <= 0)
        stride = tinygltf::GetNumComponentsInType(acc.type)
               * tinygltf::GetComponentSizeInBytes(acc.componentType);
    return { model.buffers[bv.buffer].data.data() + bv.byteOffset + acc.byteOffset,
             stride, static_cast<uint32_t>(acc.count) };
}

static void compileMesh(const std::string& gltfPath, const std::string& outputPath,
                        std::vector<std::string>* extraOutputs)
{
    tinygltf::Model     model;
    tinygltf::TinyGLTF  loader;
    std::string err, warn;

    // Geometry-only compile: skip decoding embedded images (stb is compiled out)
    loader.SetImageLoader([](tinygltf::Image*, const int, std::string*, std::string*,
                             int, int, const unsigned char*, int, void*) { return true; },
                          nullptr);

    bool ok = (gltfPath.size() >= 4 && gltfPath.substr(gltfPath.size() - 4) == ".glb")
        ? loader.LoadBinaryFromFile(&model, &err, &warn, gltfPath)
        : loader.LoadASCIIFromFile (&model, &err, &warn, gltfPath);

    if (!ok) { std::cout << "Mesh compile failed (" << gltfPath << "): " << err << std::endl; return; }
    if (!warn.empty()) std::cout << "Mesh compile warning: " << warn << std::endl;

    std::vector<uint32_t>            indices;
    std::vector<MeshBinPosition>     positions;
    std::vector<MeshBinRasterAttrib> rasterAttribs;
    std::vector<MeshBinRTHitData>    rtHitData;
    std::vector<MeshBinSkinVertex>   skinData;
    std::vector<MeshBinPrimitive>    primitives;
    bool hasSkin = false;

    for (auto& mesh : model.meshes)
    {
        for (auto& prim : mesh.primitives)
        {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES) continue;

            auto posAttr  = getAttr(model, prim.attributes.count("POSITION")   ? prim.attributes.at("POSITION")   : -1);
            auto normAttr = getAttr(model, prim.attributes.count("NORMAL")     ? prim.attributes.at("NORMAL")     : -1);
            auto tangAttr = getAttr(model, prim.attributes.count("TANGENT")    ? prim.attributes.at("TANGENT")    : -1);
            auto uvAttr   = getAttr(model, prim.attributes.count("TEXCOORD_0") ? prim.attributes.at("TEXCOORD_0") : -1);
            auto j0Attr   = getAttr(model, prim.attributes.count("JOINTS_0")   ? prim.attributes.at("JOINTS_0")   : -1);
            auto w0Attr   = getAttr(model, prim.attributes.count("WEIGHTS_0")  ? prim.attributes.at("WEIGHTS_0")  : -1);

            if (!posAttr.valid()) continue;

            uint32_t vertexBase = static_cast<uint32_t>(positions.size());
            uint32_t indexBase  = static_cast<uint32_t>(indices.size());
            uint32_t vertCount  = posAttr.count;
            uint32_t matID      = prim.material >= 0 ? static_cast<uint32_t>(prim.material) : 0;
            bool     primSkin   = j0Attr.valid() && w0Attr.valid();
            if (primSkin) hasSkin = true;

            for (uint32_t vi = 0; vi < vertCount; ++vi)
            {
                const float* p = posAttr.f3(vi);
                MeshBinPosition pos{};
                pos.x = p[0]; pos.y = p[1]; pos.z = p[2];
                positions.push_back(pos);

                float nx = 0, ny = 1, nz = 0;
                if (normAttr.valid()) { const float* n = normAttr.f3(vi); nx=n[0]; ny=n[1]; nz=n[2]; }

                float tx = 1, ty = 0, tz = 0, tw = 1;
                if (tangAttr.valid()) { const float* t = tangAttr.f4(vi); tx=t[0]; ty=t[1]; tz=t[2]; tw=t[3]; }

                float u = 0, v = 0;
                if (uvAttr.valid()) { const float* uv = uvAttr.f2(vi); u=uv[0]; v=uv[1]; }

                MeshBinRasterAttrib ra{};
                ra.uv[0] = u; ra.uv[1] = v;
                ra.normal[0] = packSnorm8(nx); ra.normal[1] = packSnorm8(ny); ra.normal[2] = packSnorm8(nz);
                ra.tangentSign = packSnorm8(tw);
                ra.tangent[0] = packSnorm8(tx); ra.tangent[1] = packSnorm8(ty); ra.tangent[2] = packSnorm8(tz);
                ra.color[0] = ra.color[1] = ra.color[2] = ra.color[3] = 255;
                rasterAttribs.push_back(ra);

                MeshBinRTHitData rt{};
                rt.uv[0] = u; rt.uv[1] = v;
                rt.normal[0] = packSnorm8(nx); rt.normal[1] = packSnorm8(ny); rt.normal[2] = packSnorm8(nz);
                rt.tangent[0] = packSnorm8(tx); rt.tangent[1] = packSnorm8(ty); rt.tangent[2] = packSnorm8(tz);
                rt.materialID = matID;
                rtHitData.push_back(rt);

                MeshBinSkinVertex sk{};
                if (primSkin)
                {
                    auto& jacc = model.accessors[prim.attributes.at("JOINTS_0")];
                    if (jacc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    {
                        const uint8_t* j = j0Attr.base + vi * j0Attr.stride;
                        sk.joints[0]=j[0]; sk.joints[1]=j[1]; sk.joints[2]=j[2]; sk.joints[3]=j[3];
                    }
                    else
                    {
                        const uint16_t* j = reinterpret_cast<const uint16_t*>(j0Attr.base + vi * j0Attr.stride);
                        sk.joints[0]=static_cast<uint8_t>(j[0]); sk.joints[1]=static_cast<uint8_t>(j[1]);
                        sk.joints[2]=static_cast<uint8_t>(j[2]); sk.joints[3]=static_cast<uint8_t>(j[3]);
                    }
                    const float* w = w0Attr.f4(vi);
                    sk.weights[0]=packUnorm8(w[0]); sk.weights[1]=packUnorm8(w[1]);
                    sk.weights[2]=packUnorm8(w[2]); sk.weights[3]=packUnorm8(w[3]);
                }
                else { sk.joints[0]=0; sk.weights[0]=255; }
                skinData.push_back(sk);
            }

            // Gather source triangles in their original winding (i0,i1,i2 each).
            std::vector<uint32_t> tri;
            if (prim.indices >= 0)
            {
                auto& acc = model.accessors[prim.indices];
                auto& bv  = model.bufferViews[acc.bufferView];
                const uint8_t* idxBase = model.buffers[bv.buffer].data.data() + bv.byteOffset + acc.byteOffset;
                auto readIdx = [&](size_t ii) -> uint32_t
                {
                    switch (acc.componentType)
                    {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return idxBase[ii];
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return reinterpret_cast<const uint16_t*>(idxBase)[ii];
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return reinterpret_cast<const uint32_t*>(idxBase)[ii];
                    }
                    return 0;
                };
                tri.reserve(acc.count);
                for (size_t ii = 0; ii + 2 < acc.count; ii += 3)
                    { tri.push_back(readIdx(ii)); tri.push_back(readIdx(ii + 1)); tri.push_back(readIdx(ii + 2)); }
            }
            else
            {
                tri.reserve(vertCount);
                for (uint32_t ii = 0; ii + 2 < vertCount; ii += 3)
                    { tri.push_back(ii); tri.push_back(ii + 1); tri.push_back(ii + 2); }
            }

            // The engine renders clockwise-front: the emitted winding must have its
            // geometric normal pointing OPPOSITE the surface normal. glTF is CCW so
            // this is normally a swap — but solo-gen emits CW spheres, so detect the
            // actual source winding from the authored normals and only swap CCW input.
            long vote = 0;
            if (normAttr.valid())
                for (size_t t = 0; t + 2 < tri.size(); t += 3)
                {
                    const MeshBinPosition& A = positions[vertexBase + tri[t]];
                    const MeshBinPosition& B = positions[vertexBase + tri[t + 1]];
                    const MeshBinPosition& C = positions[vertexBase + tri[t + 2]];
                    const float e1x=B.x-A.x, e1y=B.y-A.y, e1z=B.z-A.z;
                    const float e2x=C.x-A.x, e2y=C.y-A.y, e2z=C.z-A.z;
                    const float gx=e1y*e2z-e1z*e2y, gy=e1z*e2x-e1x*e2z, gz=e1x*e2y-e1y*e2x;
                    const float* n0=normAttr.f3(tri[t]); const float* n1=normAttr.f3(tri[t+1]); const float* n2=normAttr.f3(tri[t+2]);
                    const float d = gx*(n0[0]+n1[0]+n2[0]) + gy*(n0[1]+n1[1]+n2[1]) + gz*(n0[2]+n1[2]+n2[2]);
                    if (d > 0.f) ++vote; else if (d < 0.f) --vote;
                }
            const bool swap = vote >= 0; // CCW source (or no normals) -> swap to engine CW-front

            uint32_t indexCount = 0;
            for (size_t t = 0; t + 2 < tri.size(); t += 3)
            {
                indices.push_back(vertexBase + tri[t]);
                if (swap) { indices.push_back(vertexBase + tri[t + 2]); indices.push_back(vertexBase + tri[t + 1]); }
                else      { indices.push_back(vertexBase + tri[t + 1]); indices.push_back(vertexBase + tri[t + 2]); }
                indexCount += 3;
            }

            MeshBinPrimitive pout{};
            pout.indexOffset  = indexBase;
            pout.indexCount   = indexCount;
            pout.vertexOffset = vertexBase;
            pout.vertexCount  = vertCount;
            pout.materialID   = matID;
            primitives.push_back(pout);
        }
    }

    if (primitives.empty()) { std::cout << "Mesh compile: no valid primitives in " << gltfPath << std::endl; return; }

    // Skeleton + animation export (skin 0; vertex joint indices reference skin.joints order)
    std::vector<MeshBinJoint>       joints;
    std::vector<MeshBinAnimation>   animations;
    std::vector<MeshBinAnimChannel> channels;
    std::vector<float>              keyData;
    uint32_t                        skinJointCount = 0;

    if (hasSkin && !model.skins.empty())
    {
        const tinygltf::Skin& skin = model.skins[0];

        std::vector<int> parentOf(model.nodes.size(), -1);
        for (size_t n = 0; n < model.nodes.size(); ++n)
            for (int c : model.nodes[n].children)
                parentOf[static_cast<size_t>(c)] = static_cast<int>(n);

        // compact joint array: skin joints first (palette order), then ancestor nodes so
        // animated/static parent transforms above the skeleton root are honored
        std::vector<int>             nodeOfJoint;
        std::unordered_map<int, int> jointOfNode;
        for (int n : skin.joints)
        {
            jointOfNode[n] = static_cast<int>(nodeOfJoint.size());
            nodeOfJoint.push_back(n);
        }
        skinJointCount = static_cast<uint32_t>(nodeOfJoint.size());
        for (size_t j = 0; j < nodeOfJoint.size(); ++j)
        {
            int p = parentOf[static_cast<size_t>(nodeOfJoint[j])];
            if (p >= 0 && !jointOfNode.count(p))
            {
                jointOfNode[p] = static_cast<int>(nodeOfJoint.size());
                nodeOfJoint.push_back(p);
            }
        }

        auto ibmAttr = getAttr(model, skin.inverseBindMatrices);

        joints.resize(nodeOfJoint.size());
        for (size_t j = 0; j < nodeOfJoint.size(); ++j)
        {
            const tinygltf::Node& node = model.nodes[static_cast<size_t>(nodeOfJoint[j])];
            MeshBinJoint& out = joints[j];

            int p = parentOf[static_cast<size_t>(nodeOfJoint[j])];
            out.parent = (p >= 0 && jointOfNode.count(p)) ? jointOfNode[p] : -1;

            static const float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
            if (j < skinJointCount && ibmAttr.valid())
                memcpy(out.invBind, ibmAttr.base + j * ibmAttr.stride, sizeof(out.invBind));
            else
                memcpy(out.invBind, identity, sizeof(out.invBind));

            out.t[0] = 0; out.t[1] = 0; out.t[2] = 0;
            out.r[0] = 0; out.r[1] = 0; out.r[2] = 0; out.r[3] = 1;
            out.s[0] = 1; out.s[1] = 1; out.s[2] = 1;
            if (!node.matrix.empty())
                std::cout << "Mesh compile warning: joint node with matrix transform not supported ("
                          << node.name << ")" << std::endl;
            for (size_t k = 0; k < node.translation.size() && k < 3; ++k) out.t[k] = static_cast<float>(node.translation[k]);
            for (size_t k = 0; k < node.rotation.size()    && k < 4; ++k) out.r[k] = static_cast<float>(node.rotation[k]);
            for (size_t k = 0; k < node.scale.size()       && k < 3; ++k) out.s[k] = static_cast<float>(node.scale[k]);
        }

        for (const auto& anim : model.animations)
        {
            MeshBinAnimation outAnim{};
            strncpy(outAnim.name, anim.name.c_str(), sizeof(outAnim.name) - 1);
            outAnim.channelOffset = static_cast<uint32_t>(channels.size());
            float duration = 0.0f;

            for (const auto& ch : anim.channels)
            {
                auto it = jointOfNode.find(ch.target_node);
                if (it == jointOfNode.end()) continue;

                uint32_t path, comps;
                if      (ch.target_path == "translation") { path = MESH_BIN_ANIM_TRANSLATION; comps = 3; }
                else if (ch.target_path == "rotation")    { path = MESH_BIN_ANIM_ROTATION;    comps = 4; }
                else if (ch.target_path == "scale")       { path = MESH_BIN_ANIM_SCALE;       comps = 3; }
                else continue; // morph weights unsupported

                const auto& smp     = anim.samplers[static_cast<size_t>(ch.sampler)];
                auto        inAttr  = getAttr(model, smp.input);
                auto        outAttr = getAttr(model, smp.output);
                if (!inAttr.valid() || !outAttr.valid() || inAttr.count == 0) continue;
                if (model.accessors[static_cast<size_t>(smp.output)].componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    std::cout << "Mesh compile warning: non-float animation output skipped ("
                              << anim.name << ")" << std::endl;
                    continue;
                }

                // CUBICSPLINE output holds [inTangent, value, outTangent] per key; keep values, drop tangents
                const bool cubic = smp.interpolation == "CUBICSPLINE";

                MeshBinAnimChannel outCh{};
                outCh.joint      = static_cast<uint32_t>(it->second);
                outCh.path       = path;
                outCh.interp     = smp.interpolation == "STEP" ? MESH_BIN_ANIM_STEP : MESH_BIN_ANIM_LINEAR;
                outCh.keyCount   = inAttr.count;
                outCh.timeOffset = static_cast<uint32_t>(keyData.size());

                for (uint32_t k = 0; k < inAttr.count; ++k)
                    keyData.push_back(*reinterpret_cast<const float*>(inAttr.base + k * inAttr.stride));
                duration = std::max(duration, keyData.back());

                outCh.valueOffset = static_cast<uint32_t>(keyData.size());
                for (uint32_t k = 0; k < inAttr.count; ++k)
                {
                    const uint32_t srcKey = cubic ? k * 3 + 1 : k;
                    const float*   v      = reinterpret_cast<const float*>(outAttr.base + srcKey * outAttr.stride);
                    for (uint32_t c = 0; c < comps; ++c) keyData.push_back(v[c]);
                }

                channels.push_back(outCh);
            }

            outAnim.channelCount = static_cast<uint32_t>(channels.size()) - outAnim.channelOffset;
            outAnim.duration     = duration;
            if (outAnim.channelCount > 0)
                animations.push_back(outAnim);
        }
    }

    // materials: factors + a baseColor texture resolved to a pack-relative KTX.
    // External URIs swap extension to .ktx (pre-converted next to the model);
    // embedded images (GLB) are decoded here and baked to KTX beside the model.
    std::vector<MeshBinMaterial> materials;
    {
        const size_t resPos = gltfPath.find("/resources/");
        const std::string packRoot = resPos != std::string::npos ? gltfPath.substr(0, resPos + 11) : "";
        const std::string gltfDir  = gltfPath.substr(0, gltfPath.rfind('/') + 1);
        const std::string stem     = gltfPath.substr(gltfPath.rfind('/') + 1,
                                     gltfPath.rfind('.') - gltfPath.rfind('/') - 1);

        auto resolveTexture = [&](int texIndex, size_t mi, const char* tag) -> std::string
        {
            if (texIndex < 0 || texIndex >= static_cast<int>(model.textures.size()))
                return {};
            const int src = model.textures[texIndex].source;
            if (src < 0 || src >= static_cast<int>(model.images.size()))
                return {};

            const auto& img = model.images[src];
            std::string packPath;
            if (!img.uri.empty())
            {
                std::string rel = img.uri;
                const size_t dot = rel.rfind('.');
                if (dot != std::string::npos) rel = rel.substr(0, dot) + ".ktx";
                packPath = (gltfDir + rel).substr(packRoot.size());
            }
            else if (img.bufferView >= 0)
            {
                const auto& bv  = model.bufferViews[img.bufferView];
                const auto& buf = model.buffers[bv.buffer];
                int w = 0, h = 0, comp = 0;
                stbi_uc* pixels = stbi_load_from_memory(
                    buf.data.data() + bv.byteOffset, static_cast<int>(bv.byteLength),
                    &w, &h, &comp, 4);
                if (pixels)
                {
                    const std::string ktxFile = gltfDir + stem + "_mat" + std::to_string(mi)
                                              + "_" + tag + ".ktx";
                    if (writeKtxRgba8(ktxFile, pixels,
                                      static_cast<uint32_t>(w), static_cast<uint32_t>(h)))
                    {
                        packPath = ktxFile.substr(packRoot.size());
                        if (extraOutputs)
                            extraOutputs->push_back(ktxFile);
                    }
                    stbi_image_free(pixels);
                }
                else
                    std::cout << "Material image decode failed: " << gltfPath
                              << " material " << mi << " (" << tag << ")" << std::endl;
            }
            if (!packPath.empty())
                std::cout << "  material " << mi << " " << tag << ": " << packPath << std::endl;
            return packPath;
        };

        for (size_t mi = 0; mi < model.materials.size(); ++mi)
        {
            const auto& m   = model.materials[mi];
            const auto& pbr = m.pbrMetallicRoughness;

            MeshBinMaterial outMat{};
            for (int c = 0; c < 4; ++c)
                outMat.baseColorFactor[c] = pbr.baseColorFactor.size() > static_cast<size_t>(c)
                                          ? static_cast<float>(pbr.baseColorFactor[c]) : 1.0f;
            outMat.metallicFactor  = static_cast<float>(pbr.metallicFactor);
            outMat.roughnessFactor = static_cast<float>(pbr.roughnessFactor);

            std::string p;
            p = resolveTexture(pbr.baseColorTexture.index, mi, "baseColor");
            strncpy(outMat.baseColorPath, p.c_str(), sizeof(outMat.baseColorPath) - 1);
            p = resolveTexture(m.normalTexture.index, mi, "normal");
            strncpy(outMat.normalPath, p.c_str(), sizeof(outMat.normalPath) - 1);
            p = resolveTexture(pbr.metallicRoughnessTexture.index, mi, "metallicRoughness");
            strncpy(outMat.metallicRoughnessPath, p.c_str(), sizeof(outMat.metallicRoughnessPath) - 1);

            materials.push_back(outMat);
        }
    }

    MeshBinHeader header{};
    header.magic          = MESH_BIN_MAGIC;
    header.version        = MESH_BIN_VERSION;
    header.vertexCount    = static_cast<uint32_t>(positions.size());
    header.indexCount     = static_cast<uint32_t>(indices.size());
    header.primitiveCount = static_cast<uint32_t>(primitives.size());
    header.flags          = hasSkin ? MESH_BIN_FLAG_SKINNED : 0u;

    header.jointCount     = static_cast<uint32_t>(joints.size());
    header.skinJointCount = skinJointCount;
    header.animationCount = static_cast<uint32_t>(animations.size());
    header.channelCount   = static_cast<uint32_t>(channels.size());
    header.keyFloatCount  = static_cast<uint32_t>(keyData.size());
    header.materialCount  = static_cast<uint32_t>(materials.size());

    uint64_t off = sizeof(MeshBinHeader);
    header.primitiveOffset    = off; off += primitives.size()    * sizeof(MeshBinPrimitive);
    header.indexOffset        = off; off += indices.size()       * sizeof(uint32_t);
    header.positionOffset     = off; off += positions.size()     * sizeof(MeshBinPosition);
    header.rasterAttribOffset = off; off += rasterAttribs.size() * sizeof(MeshBinRasterAttrib);
    header.rtHitDataOffset    = off; off += rtHitData.size()     * sizeof(MeshBinRTHitData);
    header.skinOffset         = hasSkin ? off : 0; if (hasSkin) off += skinData.size() * sizeof(MeshBinSkinVertex);
    header.jointOffset        = joints.empty()     ? 0 : off; off += joints.size()     * sizeof(MeshBinJoint);
    header.animationOffset    = animations.empty() ? 0 : off; off += animations.size() * sizeof(MeshBinAnimation);
    header.channelOffset      = channels.empty()   ? 0 : off; off += channels.size()   * sizeof(MeshBinAnimChannel);
    header.keyDataOffset      = keyData.empty()    ? 0 : off; off += keyData.size()    * sizeof(float);
    header.materialOffset     = materials.empty()  ? 0 : off;

    std::ofstream out(outputPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&header),        sizeof(header));
    out.write(reinterpret_cast<const char*>(primitives.data()),    primitives.size()    * sizeof(MeshBinPrimitive));
    out.write(reinterpret_cast<const char*>(indices.data()),       indices.size()       * sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(positions.data()),     positions.size()     * sizeof(MeshBinPosition));
    out.write(reinterpret_cast<const char*>(rasterAttribs.data()), rasterAttribs.size() * sizeof(MeshBinRasterAttrib));
    out.write(reinterpret_cast<const char*>(rtHitData.data()),     rtHitData.size()     * sizeof(MeshBinRTHitData));
    if (hasSkin)
        out.write(reinterpret_cast<const char*>(skinData.data()), skinData.size() * sizeof(MeshBinSkinVertex));
    if (!joints.empty())
        out.write(reinterpret_cast<const char*>(joints.data()),     joints.size()     * sizeof(MeshBinJoint));
    if (!animations.empty())
        out.write(reinterpret_cast<const char*>(animations.data()), animations.size() * sizeof(MeshBinAnimation));
    if (!channels.empty())
        out.write(reinterpret_cast<const char*>(channels.data()),   channels.size()   * sizeof(MeshBinAnimChannel));
    if (!keyData.empty())
        out.write(reinterpret_cast<const char*>(keyData.data()),    keyData.size()    * sizeof(float));
    if (!materials.empty())
        out.write(reinterpret_cast<const char*>(materials.data()),  materials.size()  * sizeof(MeshBinMaterial));
    out.close();

    std::cout << "Compiled mesh: " << gltfPath
              << " (" << primitives.size() << " prims, "
              << positions.size() << " verts, "
              << indices.size() << " idx, "
              << joints.size() << " joints, "
              << animations.size() << " anims)" << std::endl;
}

static bool endsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

// rounded-rect signed distance, used to draw the procedural 9-slice sprites
static float roundedRectSD(float px, float py, float cx, float cy, float hw, float hh, float r)
{
    float qx = std::fabs(px - cx) - hw + r;
    float qy = std::fabs(py - cy) - hh + r;
    float ax = std::max(qx, 0.0f), ay = std::max(qy, 0.0f);
    return std::sqrt(ax * ax + ay * ay) + std::min(std::max(qx, qy), 0.0f) - r;
}

static void drawNineSliceSprite(std::vector<uint8_t>& rgba, uint32_t atlasW, uint32_t x0, uint32_t y0,
                                uint32_t size, float radius, const uint8_t fill[4], const uint8_t border[4])
{
    const float c  = size * 0.5f;
    const float hw = size * 0.5f - 2.0f;
    for (uint32_t y = 0; y < size; ++y)
        for (uint32_t x = 0; x < size; ++x)
        {
            float d = roundedRectSD(x + 0.5f, y + 0.5f, c, c, hw, hw, radius);
            float inside  = std::min(std::max(0.5f - d, 0.0f), 1.0f);           // antialiased coverage
            float edge    = std::min(std::max(0.5f - std::fabs(d + 2.5f), 0.0f) * 0.5f + (d > -5.0f && d < 0.0f ? 1.0f : 0.0f), 1.0f);
            uint8_t* p = &rgba[((y0 + y) * atlasW + (x0 + x)) * 4];
            for (int ch = 0; ch < 4; ++ch)
            {
                float v = fill[ch] * (1.0f - edge) + border[ch] * edge;
                p[ch] = static_cast<uint8_t>(v * inside);
            }
        }
}

static void compileUI(const std::string& ttfPath, const std::string& outputPath)
{
    std::ifstream ttfFile(ttfPath, std::ios::binary | std::ios::ate);
    if (!ttfFile) { std::cout << "UI compile failed (cannot open " << ttfPath << ")" << std::endl; return; }
    std::vector<uint8_t> ttf(static_cast<size_t>(ttfFile.tellg()));
    ttfFile.seekg(0);
    ttfFile.read(reinterpret_cast<char*>(ttf.data()), static_cast<std::streamsize>(ttf.size()));

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, ttf.data(), 0)) { std::cout << "UI compile failed (bad ttf): " << ttfPath << std::endl; return; }

    constexpr float    kPixelHeight = 48.0f;
    constexpr int      kSdfPadding  = 8;
    constexpr uint8_t  kOnEdge      = 128;
    constexpr float    kDistScale   = 128.0f / kSdfPadding;
    constexpr uint32_t kAtlasW = 1024, kAtlasH = 1024;
    constexpr int      kFirstChar = 32, kLastChar = 126;

    const float scale = stbtt_ScaleForPixelHeight(&font, kPixelHeight);
    int ascentI, descentI, lineGapI;
    stbtt_GetFontVMetrics(&font, &ascentI, &descentI, &lineGapI);

    struct GlyphBitmap { int codepoint; unsigned char* pixels; int w, h, xoff, yoff; float xadvance; };
    std::vector<GlyphBitmap> glyphBitmaps;
    std::vector<stbrp_rect>  rects;

    for (int cp = kFirstChar; cp <= kLastChar; ++cp)
    {
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &adv, &lsb);
        int w = 0, h = 0, xoff = 0, yoff = 0;
        unsigned char* sdf = stbtt_GetCodepointSDF(&font, scale, cp, kSdfPadding, kOnEdge, kDistScale,
                                                   &w, &h, &xoff, &yoff);
        glyphBitmaps.push_back({ cp, sdf, w, h, xoff, yoff, adv * scale });
        stbrp_rect r{};
        r.id = static_cast<int>(glyphBitmaps.size()) - 1;
        r.w  = static_cast<stbrp_coord>(w + 1); // 1px gutter
        r.h  = static_cast<stbrp_coord>(h + 1);
        rects.push_back(r);
    }

    std::vector<stbrp_node> nodes(kAtlasW);
    stbrp_context packCtx;
    stbrp_init_target(&packCtx, kAtlasW, kAtlasH, nodes.data(), static_cast<int>(nodes.size()));
    if (!stbrp_pack_rects(&packCtx, rects.data(), static_cast<int>(rects.size())))
        std::cout << "UI compile warning: font atlas too small, some glyphs dropped" << std::endl;

    std::vector<uint8_t>    fontPixels(kAtlasW * kAtlasH, 0);
    std::vector<UIBinGlyph> glyphs;
    for (const auto& r : rects)
    {
        const GlyphBitmap& g = glyphBitmaps[static_cast<size_t>(r.id)];
        if (!r.was_packed) continue;
        if (g.pixels)
            for (int y = 0; y < g.h; ++y)
                memcpy(&fontPixels[(r.y + y) * kAtlasW + r.x], &g.pixels[y * g.w], static_cast<size_t>(g.w));

        UIBinGlyph out{};
        out.codepoint = static_cast<uint32_t>(g.codepoint);
        out.u0 = static_cast<float>(r.x)       / kAtlasW;
        out.v0 = static_cast<float>(r.y)       / kAtlasH;
        out.u1 = static_cast<float>(r.x + g.w) / kAtlasW;
        out.v1 = static_cast<float>(r.y + g.h) / kAtlasH;
        out.xoff = static_cast<float>(g.xoff);
        out.yoff = static_cast<float>(g.yoff);
        out.xadvance = g.xadvance;
        out.width  = static_cast<float>(g.w);
        out.height = static_cast<float>(g.h);
        glyphs.push_back(out);
    }
    for (auto& g : glyphBitmaps)
        if (g.pixels) stbtt_FreeSDF(g.pixels, nullptr);

    // procedural sprite atlas: "panel"/"button" 9-slice sprites + circular "knob"
    constexpr uint32_t kSprAtlasW = 192, kSprAtlasH = 64, kSprSize = 64;
    std::vector<uint8_t> spritePixels(kSprAtlasW * kSprAtlasH * 4, 0);
    std::vector<UIBinSprite> sprites;

    const uint8_t panelFill[4]   = { 30,  34,  46, 235 };
    const uint8_t panelBorder[4] = { 90, 100, 130, 255 };
    const uint8_t btnFill[4]     = { 52, 110, 220, 255 };
    const uint8_t btnBorder[4]   = { 130, 175, 255, 255 };
    const uint8_t knobFill[4]    = { 235, 238, 248, 255 };
    const uint8_t knobBorder[4]  = { 255, 255, 255, 255 };
    drawNineSliceSprite(spritePixels, kSprAtlasW, 0,            0, kSprSize, 14.0f, panelFill, panelBorder);
    drawNineSliceSprite(spritePixels, kSprAtlasW, kSprSize,     0, kSprSize, 18.0f, btnFill,   btnBorder);
    drawNineSliceSprite(spritePixels, kSprAtlasW, kSprSize * 2, 0, kSprSize, kSprSize * 0.5f - 2.0f, knobFill, knobBorder); // full radius = circle

    auto addSprite = [&](const char* name, uint32_t x, uint32_t y, float border)
    {
        UIBinSprite s{};
        strncpy(s.name, name, sizeof(s.name) - 1);
        s.u0 = static_cast<float>(x)            / kSprAtlasW;
        s.v0 = static_cast<float>(y)            / kSprAtlasH;
        s.u1 = static_cast<float>(x + kSprSize) / kSprAtlasW;
        s.v1 = static_cast<float>(y + kSprSize) / kSprAtlasH;
        s.borderL = s.borderT = s.borderR = s.borderB = border;
        s.pixelWidth = s.pixelHeight = static_cast<float>(kSprSize);
        sprites.push_back(s);
    };
    addSprite("panel",  0,            0, 22.0f);
    addSprite("button", kSprSize,     0, 22.0f);
    addSprite("knob",   kSprSize * 2, 0, 0.0f); // border 0: stretches as one quad, stays circular

    UIBinHeader header{};
    header.magic           = UI_BIN_MAGIC;
    header.version         = UI_BIN_VERSION;
    header.fontAtlasWidth  = kAtlasW;
    header.fontAtlasHeight = kAtlasH;
    header.fontPixelHeight = kPixelHeight;
    header.ascent          = ascentI  * scale;
    header.descent         = descentI * scale;
    header.lineGap         = lineGapI * scale;
    header.glyphCount      = static_cast<uint32_t>(glyphs.size());
    header.spriteAtlasWidth  = kSprAtlasW;
    header.spriteAtlasHeight = kSprAtlasH;
    header.spriteCount       = static_cast<uint32_t>(sprites.size());

    uint64_t off = sizeof(UIBinHeader);
    header.glyphOffset        = off; off += glyphs.size()  * sizeof(UIBinGlyph);
    header.spriteOffset       = off; off += sprites.size() * sizeof(UIBinSprite);
    header.fontPixelsOffset   = off; off += fontPixels.size();
    header.spritePixelsOffset = off;

    std::ofstream out(outputPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&header),             sizeof(header));
    out.write(reinterpret_cast<const char*>(glyphs.data()),       static_cast<std::streamsize>(glyphs.size()  * sizeof(UIBinGlyph)));
    out.write(reinterpret_cast<const char*>(sprites.data()),      static_cast<std::streamsize>(sprites.size() * sizeof(UIBinSprite)));
    out.write(reinterpret_cast<const char*>(fontPixels.data()),   static_cast<std::streamsize>(fontPixels.size()));
    out.write(reinterpret_cast<const char*>(spritePixels.data()), static_cast<std::streamsize>(spritePixels.size()));
    out.close();

    std::cout << "Compiled UI: " << ttfPath << " (" << glyphs.size() << " glyphs, "
              << sprites.size() << " sprites)" << std::endl;
}

void queryPath( std::vector<std::string> *filesPathList, std::string *filesHash, fs::path path )
{
    std::cout << "Query path:" << path << std::endl;

    std::string vsShaderExt  = ".vert";
    std::string psShaderExt  = ".frag";
    std::string csShaderExt  = ".comp";
    std::string gltfExt      = ".gltf";
    std::string glbExt       = ".glb";
    std::string meshBinExt   = ".mesh.bin";
    std::string gltfBinExt   = ".bin";

    for (const auto& entry : fs::directory_iterator(path))
    {
        if (fs::is_directory(entry.path()))
        {
            queryPath(filesPathList, filesHash, entry.path());
            continue;
        }
        if (!fs::file_size(entry.path())) continue;

        std::string strPath = entry.path().string();
        std::replace(strPath.begin(), strPath.end(), '\\', '/');

        if (endsWith(strPath, ".vertc") || endsWith(strPath, ".fragc") ||
            endsWith(strPath, ".vertr") || endsWith(strPath, ".fragr") ||
            endsWith(strPath, ".compc") ||
            endsWith(strPath, meshBinExt) || endsWith(strPath, ".ui.bin"))
            continue;

        (*filesHash) += strPath + std::to_string(fs::last_write_time(entry.path()).time_since_epoch().count());

        if (endsWith(strPath, gltfExt) || endsWith(strPath, glbExt))
        {
            std::string outPath = strPath.substr(0, strPath.rfind('.')) + ".mesh.bin";
            compileMesh(strPath, outPath, filesPathList);
            if (fs::exists(fs::path(outPath)))
                filesPathList->push_back(outPath);
            continue;
        }

        if (endsWith(strPath, ".ttf"))
        {
            std::string outPath = strPath.substr(0, strPath.rfind('.')) + ".ui.bin";
            compileUI(strPath, outPath);
            if (fs::exists(fs::path(outPath)))
                filesPathList->push_back(outPath);
            continue;
        }

        if (endsWith(strPath, gltfBinExt))
            continue;

        if (endsWith(strPath, ".hdr"))
        {
            const std::string base  = strPath.substr(0, strPath.rfind('.'));
            const std::string probe = base + ".ktx";
            if (bakeEnvironment(strPath, probe))
                filesPathList->push_back(probe);
            // split-sum BRDF (DFG) LUT — env-independent, baked once beside the probe
            const std::string lut = strPath.substr(0, strPath.find_last_of("/\\") + 1) + "brdf_lut.ktx";
            if (bakeBrdfLut(lut))
                filesPathList->push_back(lut);
            continue; // the source .hdr itself is not packed
        }

        if (endsWith(strPath, csShaderExt))
        {
            // compute shaders: compile only, engine-side layouts need no reflection
            std::string strTemp = strPath;
#ifdef S_PLATFORM_WINDOWS
            std::replace(strTemp.begin(), strTemp.end(), '/', '\\');
#endif
            std::string glslcCmd = std::string(VULKAN_SDK) + "/bin/glslc --target-env=vulkan1.2 "
                                 + strTemp + " -o " + strTemp + "c";
            std::cout << glslcCmd << std::endl;
            if (system(glslcCmd.c_str()) != 0)
            {
                std::cout << "Compute shader compile FAILED: " << strPath << std::endl;
                continue;
            }
            filesPathList->push_back(strPath + "c");
            continue;
        }

        if (endsWith(strPath, vsShaderExt) || endsWith(strPath, psShaderExt))
        {
            std::string strTemp;
            for (size_t i = 0; i < strPath.length(); ++i)
            {
            #ifdef S_PLATFORM_WINDOWS
                if (strPath.at(i) == '/') strTemp += "\\";
                else
            #endif
                    strTemp += strPath.at(i);
            }

            std::string glslcCmd = std::string(VULKAN_SDK) + "/bin/glslc --target-env=vulkan1.2 " + strTemp + " -o " + strTemp + "c";
            std::string spirvCmd = std::string(VULKAN_SDK) + "/bin/spirv-cross " + strTemp + "c --reflect > " + strTemp + "_temp";
            std::cout << glslcCmd << std::endl;
            std::cout << spirvCmd << std::endl;
            system(glslcCmd.c_str());
            system(spirvCmd.c_str());

            std::ifstream reflectionFile;
            std::vector<char> buffer;
            reflectionFile.open(strTemp + "_temp", std::ios::in | std::ios::binary);
            auto begin = reflectionFile.tellg();
            reflectionFile.seekg(0, std::ios::end);
            auto end = reflectionFile.tellg();
            size_t fileSize = static_cast<size_t>(end - begin);
            buffer.resize(fileSize);
            reflectionFile.seekg(0, std::ios::beg);
            reflectionFile.read(buffer.data(), static_cast<std::streamsize>(fileSize));
            reflectionFile.close();

            Document document;
            buffer.push_back('\0');
            const char* jsonStart = buffer.data();
            while (*jsonStart && *jsonStart != '{') ++jsonStart;
            document.Parse(jsonStart);

            if (document.HasParseError() || !document.HasMember("entryPoints"))
            {
                std::cout << "spirv-cross reflection failed: " << buffer.data() << std::endl;
            #ifdef S_PLATFORM_LINUX
                system(std::string("rm " + strTemp + "_temp").c_str());
            #elif defined(S_PLATFORM_WINDOWS)
                system(std::string("del " + strTemp + "_temp").c_str());
            #endif
                continue;
            }

            solo::S_VulkanShaderReflection reflection;
            std::vector<solo::S_VulkanShaderReflectionUniformBuffer> uniformsList;
            std::vector<solo::S_VulkanShaderReflectionTexture> textureList;
            solo::S_VulkanShaderReflectionUniformBuffer uniformBuffer;
            solo::S_VulkanShaderReflectionTexture texture;

            {
                auto it = document["entryPoints"].GetArray().begin();
                strcpy(reflection.EntryPointName, (*it)["name"].GetString());
                std::string typeStr = (*it)["mode"].GetString();
                if      (typeStr == "vert") reflection.Stage = solo::S_ShaderStage::VertexShader;
                else if (typeStr == "frag") reflection.Stage = solo::S_ShaderStage::FragmentShader;
                else                        reflection.Stage = solo::S_ShaderStage::GeometryShader;
            }

            uint32_t maxUniformSet = 0;
            if (document.HasMember("ubos"))
            {
                const Value& value = document["ubos"];
                reflection.NumberOfUniformBuffers = value.Size();
                uint32_t offset = 0;
                for (auto it = value.Begin(); it != value.End(); ++it)
                {
                    uniformBuffer.ArraySize = (*it).HasMember("array") ? (*(*it)["array"].Begin()).GetUint() : 1;
                    strcpy(uniformBuffer.Name, (*it)["name"].GetString());
                    uniformBuffer.BlockSize = (*it)["block_size"].GetUint();
                    uniformBuffer.Set       = (*it)["set"].GetUint();
                    maxUniformSet = std::max(maxUniformSet, uniformBuffer.Set);
                    uniformBuffer.Binding   = (*it)["binding"].GetUint();
                    uniformBuffer.Offset    = offset;
                    offset += uniformBuffer.BlockSize * uniformBuffer.ArraySize;
                    uniformsList.push_back(uniformBuffer);
                }
            }
            else reflection.NumberOfUniformBuffers = 0;
            reflection.MaxUniformBuffersSet = maxUniformSet;

            uint32_t maxTextureSet = 0;
            if (document.HasMember("textures"))
            {
                const Value& value = document["textures"];
                reflection.NumberOfTextures = value.Size();
                uint32_t offset = 0;
                for (auto it = value.Begin(); it != value.End(); ++it)
                {
                    texture.ArraySize = (*it).HasMember("array") ? (*(*it)["array"].Begin()).GetUint() : 1;
                    strcpy(texture.Name, (*it)["name"].GetString());
                    texture.Set     = (*it)["set"].GetUint();
                    maxTextureSet = std::max(maxTextureSet, texture.Set);
                    texture.Binding = (*it)["binding"].GetUint();
                    texture.Offset  = offset++;
                    textureList.push_back(texture);
                }
            }
            else reflection.NumberOfTextures = 0;
            reflection.MaxTextureSet = maxTextureSet;

        #ifdef S_PLATFORM_LINUX
            system(std::string("rm " + strTemp + "_temp").c_str());
        #elif defined(S_PLATFORM_WINDOWS)
            system(std::string("del " + strTemp + "_temp").c_str());
        #endif

            std::ofstream binaryReflectionFile(strPath + "r", std::ios::binary);
            binaryReflectionFile.write(reinterpret_cast<const char*>(&reflection), sizeof(solo::S_VulkanShaderReflection));
            if (reflection.NumberOfUniformBuffers > 0)
                binaryReflectionFile.write(reinterpret_cast<const char*>(uniformsList.data()),
                    sizeof(solo::S_VulkanShaderReflectionUniformBuffer) * reflection.NumberOfUniformBuffers);
            if (reflection.NumberOfTextures > 0)
                binaryReflectionFile.write(reinterpret_cast<const char*>(textureList.data()),
                    sizeof(solo::S_VulkanShaderReflectionTexture) * reflection.NumberOfTextures);
            binaryReflectionFile.close();

            filesPathList->push_back(strPath + "r");
            strPath += "c";
        }

        filesPathList->push_back(strPath);
    }
}

int main(int argc, char *argv[])
{
    fs::path currentPath   = fs::current_path();
    fs::path resourcesPath = fs::path((currentPath.string() + "/resources/").c_str());
    fs::path hashFilePath  = fs::path((currentPath.string() + "/resources.log").c_str());
    std::vector<std::string> filePathList;
    std::string filesHash;
    queryPath(&filePathList, &filesHash, resourcesPath);

    std::string lastfilesHash;
    if (fs::exists(hashFilePath))
    {
        lastfilesHash.resize(fs::file_size(hashFilePath));
        std::ifstream lastHashFile(hashFilePath.string().c_str());
        lastHashFile.read(lastfilesHash.data(), static_cast<long long>(lastfilesHash.length()));
        lastHashFile.close();
    }

    if (lastfilesHash == filesHash)
    {
        std::cout << "Nothing changed!" << std::endl;
        return 0;
    }

    std::ofstream hashFile(hashFilePath.string().c_str());
    hashFile.write(filesHash.c_str(), static_cast<long long>(filesHash.length()));
    hashFile.close();

    // baked textures can be both scanned and pushed explicitly — dedupe
    std::sort(filePathList.begin(), filePathList.end());
    filePathList.erase(std::unique(filePathList.begin(), filePathList.end()), filePathList.end());

    std::string outputDir = argc > 1 ? std::string(argv[1]) : currentPath.string();
    std::replace(outputDir.begin(), outputDir.end(), '\\', '/');
    std::string outputFilePath = outputDir + "/resources.spk";

    struct FileInfo { std::string name; std::string path; uint64_t size; };
    std::vector<FileInfo> files;
    for (const auto& p : filePathList)
    {
        std::string rel = p;
        rel.erase(0, resourcesPath.string().length());
        uint64_t sz = static_cast<uint64_t>(fs::file_size(fs::path(p)));
        files.push_back({ rel, p, sz });
        std::cout << "Packing: " << rel << std::endl;
    }

    std::ofstream out(outputFilePath, std::ios::binary);

    S_PackHeader header{};
    header.magic    = PACK_MAGIC;
    header.version  = PACK_VERSION;
    header.count    = static_cast<uint32_t>(files.size());
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    uint64_t offset = sizeof(S_PackHeader) + files.size() * sizeof(S_PackEntry);
    for (const auto& f : files)
    {
        S_PackEntry entry{};
        strncpy(entry.name, f.name.c_str(), sizeof(entry.name) - 1);
        entry.offset = offset;
        entry.size   = f.size;
        out.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
        offset += f.size;
    }

    std::vector<char> buf;
    for (const auto& f : files)
    {
        buf.resize(f.size);
        std::ifstream in(f.path, std::ios::binary);
        in.read(buf.data(), static_cast<std::streamsize>(f.size));
        out.write(buf.data(), static_cast<std::streamsize>(f.size));
    }

    out.close();
    std::cout << "\"resources.spk\" generated (" << files.size() << " assets)" << std::endl;
    return 0;
}
