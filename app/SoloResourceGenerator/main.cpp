#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
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

            uint32_t indexCount = 0;
            if (prim.indices >= 0)
            {
                auto& acc = model.accessors[prim.indices];
                auto& bv  = model.bufferViews[acc.bufferView];
                const uint8_t* idxBase = model.buffers[bv.buffer].data.data() + bv.byteOffset + acc.byteOffset;
                indexCount = static_cast<uint32_t>(acc.count);
                for (size_t ii = 0; ii < acc.count; ++ii)
                {
                    uint32_t idx = 0;
                    switch (acc.componentType)
                    {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  idx = idxBase[ii]; break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: idx = reinterpret_cast<const uint16_t*>(idxBase)[ii]; break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   idx = reinterpret_cast<const uint32_t*>(idxBase)[ii]; break;
                    }
                    indices.push_back(vertexBase + idx);
                }
            }
            else
            {
                indexCount = vertCount;
                for (uint32_t ii = 0; ii < vertCount; ++ii) indices.push_back(vertexBase + ii);
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

    MeshBinHeader header{};
    header.magic          = MESH_BIN_MAGIC;
    header.version        = MESH_BIN_VERSION;
    header.vertexCount    = static_cast<uint32_t>(positions.size());
    header.indexCount     = static_cast<uint32_t>(indices.size());
    header.primitiveCount = static_cast<uint32_t>(primitives.size());
    header.flags          = hasSkin ? MESH_BIN_FLAG_SKINNED : 0u;

    uint64_t off = sizeof(MeshBinHeader);
    header.primitiveOffset    = off; off += primitives.size()    * sizeof(MeshBinPrimitive);
    header.indexOffset        = off; off += indices.size()       * sizeof(uint32_t);
    header.positionOffset     = off; off += positions.size()     * sizeof(MeshBinPosition);
    header.rasterAttribOffset = off; off += rasterAttribs.size() * sizeof(MeshBinRasterAttrib);
    header.rtHitDataOffset    = off; off += rtHitData.size()     * sizeof(MeshBinRTHitData);
    header.skinOffset         = hasSkin ? off : 0;

    std::ofstream out(outputPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&header),        sizeof(header));
    out.write(reinterpret_cast<const char*>(primitives.data()),    primitives.size()    * sizeof(MeshBinPrimitive));
    out.write(reinterpret_cast<const char*>(indices.data()),       indices.size()       * sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(positions.data()),     positions.size()     * sizeof(MeshBinPosition));
    out.write(reinterpret_cast<const char*>(rasterAttribs.data()), rasterAttribs.size() * sizeof(MeshBinRasterAttrib));
    out.write(reinterpret_cast<const char*>(rtHitData.data()),     rtHitData.size()     * sizeof(MeshBinRTHitData));
    if (hasSkin)
        out.write(reinterpret_cast<const char*>(skinData.data()), skinData.size() * sizeof(MeshBinSkinVertex));
    out.close();

    std::cout << "Compiled mesh: " << gltfPath
              << " (" << primitives.size() << " prims, "
              << positions.size() << " verts, "
              << indices.size() << " idx)" << std::endl;
}

static bool endsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
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
            endsWith(strPath, meshBinExt))
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
