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
#include "solo/ui/S_UIBin.h"

using namespace rapidjson;

namespace fs = std::filesystem;

static constexpr uint32_t PACK_MAGIC   = 0x4b415053;
static constexpr uint32_t PACK_VERSION = 1;

#pragma pack(push, 1)
struct S_PackHeader { uint32_t magic; uint32_t version; uint32_t count; uint32_t reserved; };
struct S_PackEntry  { char name[256]; uint64_t offset; uint64_t size; };
#pragma pack(pop)

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

static void compileMesh(const std::string& gltfPath, const std::string& outputPath)
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

            // glTF front faces are CCW; engine convention is CW, so swap each
            // triangle's last two indices while emitting
            uint32_t indexCount = 0;
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
                for (size_t ii = 0; ii + 2 < acc.count; ii += 3)
                {
                    indices.push_back(vertexBase + readIdx(ii));
                    indices.push_back(vertexBase + readIdx(ii + 2));
                    indices.push_back(vertexBase + readIdx(ii + 1));
                    indexCount += 3;
                }
            }
            else
            {
                for (uint32_t ii = 0; ii + 2 < vertCount; ii += 3)
                {
                    indices.push_back(vertexBase + ii);
                    indices.push_back(vertexBase + ii + 2);
                    indices.push_back(vertexBase + ii + 1);
                    indexCount += 3;
                }
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
    header.keyDataOffset      = keyData.empty()    ? 0 : off;

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
            endsWith(strPath, meshBinExt) || endsWith(strPath, ".ui.bin"))
            continue;

        (*filesHash) += strPath + std::to_string(fs::last_write_time(entry.path()).time_since_epoch().count());

        if (endsWith(strPath, gltfExt) || endsWith(strPath, glbExt))
        {
            std::string outPath = strPath.substr(0, strPath.rfind('.')) + ".mesh.bin";
            compileMesh(strPath, outPath);
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

            std::string glslcCmd = std::string(VULKAN_SDK) + "/bin/glslc " + strTemp + " -o " + strTemp + "c";
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
