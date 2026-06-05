#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include "solo/platforms/S_SystemDetect.h"
#include "solo/renderer/vulkan/S_VulkanShaderReflection.h"
#include "rapidjson/document.h"
using namespace rapidjson;

namespace fs = std::filesystem;

static constexpr uint32_t PACK_MAGIC   = 0x4b415053;
static constexpr uint32_t PACK_VERSION = 1;

#pragma pack(push, 1)
struct S_PackHeader { uint32_t magic; uint32_t version; uint32_t count; uint32_t reserved; };
struct S_PackEntry  { char name[256]; uint64_t offset; uint64_t size; };
#pragma pack(pop)

void queryPath( std::vector<std::string> *filesPathList, std::string *filesHash, fs::path path )
{
    std::cout << "Query path:" << path << std::endl;

    std::string strPath;
    std::string vsShaderExt = ".vert";
    std::string psShaderExt = ".frag";
    std::string vsShadercExt = ".vertc";
    std::string psShadercExt = ".fragc";
    std::string vsShaderrExt = ".vertr";
    std::string psShaderrExt = ".fragr";
    for (const auto & entry : fs::directory_iterator(path))
    {
        if( fs::is_directory( entry.path() ) )
            queryPath( filesPathList, filesHash, entry.path() );
        else if( fs::file_size( entry.path() ) )
        {
            strPath = entry.path().string();
            std::replace( strPath.begin(), strPath.end(), '\\', '/' );
            if( strPath.substr( strPath.length() - vsShadercExt.length(), vsShadercExt.length() ) == vsShadercExt ||
                    strPath.substr( strPath.length() - psShadercExt.length(), psShadercExt.length() ) == psShadercExt
                    || strPath.substr( strPath.length() - vsShaderrExt.length(), vsShaderrExt.length() ) == vsShaderrExt ||
                    strPath.substr( strPath.length() - psShaderrExt.length(), psShaderrExt.length() ) == psShaderrExt )
                continue;
            (*filesHash) += strPath + std::to_string( fs::last_write_time(entry.path()).time_since_epoch().count() );
            if( strPath.substr( strPath.length() - vsShaderExt.length(), vsShaderExt.length() ) == vsShaderExt ||
                    strPath.substr( strPath.length() - psShaderExt.length(), psShaderExt.length() ) == psShaderExt )
            {
                std::string strTemp;
                for( size_t i = 0; i < strPath.length(); ++i )
                {
                #ifdef S_PLATFORM_WINDOWS
                    if( strPath.at(i) == '/' )
                         strTemp += "\\";
                    else
                #endif
                        strTemp += strPath.at( i );
                }

                std::string glslcCmd = std::string(VULKAN_SDK) + "/bin/glslc " + strTemp + " -o " + strTemp + "c";
                std::string spirvCmd = std::string(VULKAN_SDK) + "/bin/spirv-cross " + strTemp + "c --reflect > " + strTemp + "_temp 2>&1";
                std::cout << glslcCmd << std::endl;
                std::cout << spirvCmd << std::endl;
                system( glslcCmd.c_str() );
                system( spirvCmd.c_str() );

                std::ifstream reflectionFile;
                std::vector<char> buffer;
                reflectionFile.open( strTemp + "_temp", std::ios::in | std::ios::binary );
                auto begin = reflectionFile.tellg();
                reflectionFile.seekg (0, std::ios::end);
                auto end = reflectionFile.tellg();
                size_t fileSize = static_cast<size_t>(end - begin);
                buffer.resize(fileSize);
                reflectionFile.seekg(0,std::ios::beg );
                reflectionFile.read( buffer.data(),static_cast<std::streamsize>( fileSize ) );
                reflectionFile.close();
                Document document;
                buffer.push_back('\0');
                document.Parse(buffer.data());

                if( document.HasParseError() || !document.HasMember("entryPoints") )
                {
                    std::cout << "spirv-cross reflection failed: " << buffer.data() << std::endl;
#ifdef S_PLATFORM_LINUX
                    system( std::string( "rm " + strTemp + "_temp" ).c_str() );
#elif defined(S_PLATFORM_WINDOWS)
                    system( std::string( "del " + strTemp + "_temp" ).c_str() );
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
                    strcpy( reflection.EntryPointName, (*it)["name"].GetString() );
                    std::string typeStr = (*it)["mode"].GetString();
                    if( typeStr == "vert" )
                        reflection.Stage = solo::S_ShaderStage::VertexShader;
                    else if( typeStr == "frag" )
                        reflection.Stage = solo::S_ShaderStage::FragmentShader;
                    else
                        reflection.Stage = solo::S_ShaderStage::GeometryShader;
                }
                uint32_t maxUniformSet = 0;

                if( document.HasMember("ubos") )
                {
                    const Value& value = document["ubos"];
                    reflection.NumberOfUniformBuffers = value.Size();
                    uint32_t offset = 0;
                    auto it = value.Begin();
                    for (;it != value.End();++it)
                    {
                        if( (*it).HasMember("array") )
                            uniformBuffer.ArraySize = (*(*it)["array"].Begin()).GetUint();
                        else
                            uniformBuffer.ArraySize = 1;
                        strcpy( uniformBuffer.Name, (*it)["name"].GetString() );
                        uniformBuffer.BlockSize = (*it)["block_size"].GetUint();
                        uniformBuffer.Set = (*it)["set"].GetUint();
                        maxUniformSet = std::max(maxUniformSet, uniformBuffer.Set);
                        uniformBuffer.Binding = (*it)["binding"].GetUint();
                        uniformBuffer.Offset = offset;
                        offset += uniformBuffer.BlockSize * uniformBuffer.ArraySize;
                        uniformsList.push_back( uniformBuffer );
                    }
                }else
                    reflection.NumberOfUniformBuffers = 0;

                reflection.MaxUniformBuffersSet = maxUniformSet;

                uint32_t maxTextureSet = 0;
                if( document.HasMember("textures") )
                {
                    const Value& value = document["textures"];
                    reflection.NumberOfTextures = value.Size();
                    uint32_t offset = 0;
                    auto it = value.Begin();
                    for (;it != value.End();++it)
                    {
                        if( (*it).HasMember("array") )
                            texture.ArraySize = (*(*it)["array"].Begin()).GetUint();
                        else
                            texture.ArraySize = 1;
                        strcpy( texture.Name, (*it)["name"].GetString() );
                        texture.Set = (*it)["set"].GetUint();
                        maxTextureSet = std::max(maxTextureSet, texture.Set);
                        texture.Binding = (*it)["binding"].GetUint();
                        texture.Offset = offset;
                        offset += 1;
                        textureList.push_back( texture );
                    }
                }else
                    reflection.NumberOfTextures = 0;

                reflection.MaxTextureSet = maxTextureSet;

#ifdef S_PLATFORM_LINUX
                system(  std::string( "rm " + strTemp + "_temp" ).c_str() );
#elif defined(S_PLATFORM_WINDOWS)
                system(  std::string( "del " + strTemp + "_temp" ).c_str() );
#endif

                std::ofstream binaryReflectionFile( strPath + "r", std::ios::binary );
                binaryReflectionFile.write( reinterpret_cast<const char *>(&reflection), sizeof(solo::S_VulkanShaderReflection) );
                if( reflection.NumberOfUniformBuffers > 0 )
                    binaryReflectionFile.write( reinterpret_cast<const char *>(uniformsList.data()),
                                                sizeof(solo::S_VulkanShaderReflectionUniformBuffer) * static_cast<unsigned int>(reflection.NumberOfUniformBuffers) );
                if( reflection.NumberOfTextures > 0 )
                    binaryReflectionFile.write( reinterpret_cast<const char *>(textureList.data()),
                                                sizeof(solo::S_VulkanShaderReflectionTexture) * static_cast<unsigned int>(reflection.NumberOfTextures) );
                binaryReflectionFile.close();

                filesPathList->push_back( strPath + "r" );
                strPath += "c";
            }
            filesPathList->push_back( strPath );
        }
    }
}

int main(int argc, char *argv[])
{
    fs::path currentPath  = fs::current_path();
    fs::path resourcesPath = fs::path( ( currentPath.string() + "/resources/" ).c_str() );
    fs::path hashFilePath  = fs::path( ( currentPath.string() + "/resources.log" ).c_str() );
    std::vector<std::string> filePathList;
    std::string filesHash;
    queryPath( &filePathList, &filesHash, resourcesPath );

    std::string lastfilesHash;
    if( fs::exists( hashFilePath ) )
    {
        lastfilesHash.resize( fs::file_size( hashFilePath ) );
        std::ifstream lastHashFile( hashFilePath.string().c_str() );
        lastHashFile.read( lastfilesHash.data(), static_cast<long long>( lastfilesHash.length() ) );
        lastHashFile.close();
    }

    if( lastfilesHash == filesHash )
    {
        std::cout << "Nothing changed!" << std::endl;
        return 0;
    }

    std::ofstream hashFile( hashFilePath.string().c_str() );
    hashFile.write( filesHash.c_str(), static_cast<long long>( filesHash.length() ) );
    hashFile.close();

    std::string outputDir = argc > 1 ? std::string(argv[1]) : currentPath.string();
    std::replace(outputDir.begin(), outputDir.end(), '\\', '/');
    std::string outputFilePath = outputDir + "/resources.spk";

    struct FileInfo { std::string name; std::string path; uint64_t size; };
    std::vector<FileInfo> files;
    for( const auto& p : filePathList )
    {
        std::string rel = p;
        rel.erase( 0, resourcesPath.string().length() );
        uint64_t sz = static_cast<uint64_t>( fs::file_size( fs::path( p ) ) );
        files.push_back( { rel, p, sz } );
        std::cout << "Packing: " << rel << std::endl;
    }

    std::ofstream out( outputFilePath, std::ios::binary );

    S_PackHeader header{};
    header.magic    = PACK_MAGIC;
    header.version  = PACK_VERSION;
    header.count    = static_cast<uint32_t>( files.size() );
    out.write( reinterpret_cast<const char*>( &header ), sizeof(header) );

    uint64_t offset = sizeof(S_PackHeader) + files.size() * sizeof(S_PackEntry);
    for( const auto& f : files )
    {
        S_PackEntry entry{};
        strncpy( entry.name, f.name.c_str(), sizeof(entry.name) - 1 );
        entry.offset = offset;
        entry.size   = f.size;
        out.write( reinterpret_cast<const char*>( &entry ), sizeof(entry) );
        offset += f.size;
    }

    std::vector<char> buf;
    for( const auto& f : files )
    {
        buf.resize( f.size );
        std::ifstream in( f.path, std::ios::binary );
        in.read( buf.data(), static_cast<std::streamsize>( f.size ) );
        out.write( buf.data(), static_cast<std::streamsize>( f.size ) );
    }

    out.close();
    std::cout << "\"resources.spk\" generated (" << files.size() << " assets)" << std::endl;
    return 0;
}
