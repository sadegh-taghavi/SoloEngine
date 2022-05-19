#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "solo/platforms/S_SystemDetect.h"
#include "solo/renderer/vulkan/S_VulkanShaderReflection.h"
#include "rapidjson/document.h"
using namespace rapidjson;

namespace fs = std::filesystem;

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

                system( ( std::string( std::string(VULKAN_SDK) + "/bin/glslc " )
                          + strPath + " -o " + strPath + "c" ).c_str() );

                system( ( std::string( std::string(VULKAN_SDK) + "/bin/spirv-cross " )
                          + strPath + "c --reflect >> " + strPath + "_temp" ).c_str() );

                std::ifstream reflectionFile;
                std::vector<char> buffer;
                reflectionFile.open( strPath + "_temp", std::ios::in | std::ios::binary );
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

                std::string strTemp;
                for( size_t i = 0;i < strPath.length(); ++i )
                {

                #ifdef S_PLATFORM_WINDOWS
                    if( strPath.at(i) == '/' )
                         strTemp += "\\";
                    else
                #endif
                        strTemp += strPath.at( i );
                }

#ifdef S_PLATFORM_LINUX
                system(  std::string( "rm " + strTemp + "_temp" ).c_str() );
#elif defined(S_PLATFORM_WINDOWS)
                system(  std::string( "del " + strTemp + "_temp" ).c_str() );
#endif


                std::ofstream binaryReflectionFile( strPath + "r" );
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

constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

inline std::string byteToHexStr(unsigned char byte)
{
    if( 0 == byte )
        return std::string("0x0,");
    std::string hex("0x  ,");

    hex[2] = hexmap[(byte & 0xF0) >> 4];
    hex[3] = hexmap[byte & 0x0F];
    return hex;
}

int main(int argc, char *argv[])
{
    fs::path currentPath = fs::current_path();
//    fs::path currentPath = "D:/git/soloengine";
    fs::path resourcesPath = fs::path( ( currentPath.string() + "/resources/" ).c_str() );
    fs::path hashFilePath = fs::path( ( currentPath.string() + "/resources.log" ).c_str() );
    std::vector<std::string> filePathList;
    std::string filesHash;
    queryPath( &filePathList, &filesHash, resourcesPath );

    std::string lastfilesHash;
    if( fs::exists( hashFilePath ) )
    {
        lastfilesHash.resize( fs::file_size( hashFilePath ) );
        std::ifstream lastHashFile( hashFilePath.string().c_str() );
        lastHashFile.read( lastfilesHash.data(), static_cast<long long >( lastfilesHash.length() ) );
        lastHashFile.close();
    }

    if( lastfilesHash == filesHash )
    {
        std::cout << "Noting changed!" << std::endl;
        return 0;
    }

    std::string outputFilePath = currentPath.string() + "/solo/resource/S_Resources.cpp";

    if( !filePathList.size() )
    {
        std::ofstream outputFile( outputFilePath.c_str() );
        std::ofstream hashFile( hashFilePath.string().c_str() );
        hashFile.close();

        outputFile << "#include \"S_Resources.h\"\n\n" \
                      "using namespace solo;\n\n" \
                      "S_ResourcesContainer::S_ResourcesContainer()\n"\
                      "{\n"\
                      "\tupdateResources( 0, nullptr, nullptr, nullptr );\n" \
                      "}\n";

        outputFile.close();

        std::cout << "Noting to generate!" << std::endl;
        return 0;
    }

    std::ofstream hashFile( hashFilePath.string().c_str() );
    hashFile.write( filesHash.c_str(), static_cast<long long>(filesHash.length() ) );
    hashFile.close();

    std::ofstream outputFile( outputFilePath.c_str() );
    std::ifstream inputFile;
    size_t fileSize;
    outputFile << "#include \"S_Resources.h\"\n\n" \
                  "using namespace solo;\n\n" \
                  "static const size_t __S_NumberOfFiles = " <<
                  std::to_string( filePathList.size() ) << ";\n\n";

    outputFile << "static const unsigned char __S_FileNames[] =\n{";
    std::string activeFileName;
    for ( size_t i = 0; i < filePathList.size(); ++i )
    {
        activeFileName = filePathList.at(i);
        std::cout << "Resource file(" << i + 1 << "):" << activeFileName << std::endl;
        activeFileName = "sr:/" + activeFileName.erase( 0, resourcesPath.string().length() );
        outputFile << "\n//" << activeFileName << std::endl;
        for( size_t ni = 0; ni < 64; ++ni )
        {
            if( ni >= activeFileName.length() )
                outputFile << "0x0,";
            else
                outputFile << byteToHexStr( static_cast<unsigned char>( activeFileName[ni] ) );
        }
    }
    outputFile << "\n};\n" << std::endl;

    std::string activeFilePath;
    outputFile << "static const size_t __S_FileSizes[] =\n{";
    for ( size_t i = 0; i < filePathList.size(); ++i )
    {
        activeFilePath = activeFileName = filePathList.at(i);
        outputFile << "\n//sr:/" << activeFileName.erase( 0, resourcesPath.string().length() ) << "\n";
        outputFile << std::to_string( fs::file_size( fs::path( activeFilePath.c_str() ) ) ) << ",";
    }
    outputFile << "\n};\n" << std::endl;

    outputFile << "static const unsigned char __S_FilesData[] =\n{";
    std::vector<char> buffer;
    for ( size_t i = 0; i < filePathList.size(); ++i )
    {
        activeFileName = filePathList.at(i);
        inputFile.open( activeFileName, std::ios::in | std::ios::binary );
        auto begin = inputFile.tellg();
        inputFile.seekg (0, std::ios::end);
        auto end = inputFile.tellg();
        fileSize = static_cast<size_t>(end - begin);
        buffer.resize(fileSize);
        inputFile.seekg(0,std::ios::beg );
        inputFile.read( buffer.data(),static_cast<std::streamsize>( fileSize ) );
        inputFile.close();
        outputFile << "\n//sr:/" << activeFileName.erase( 0, resourcesPath.string().length() ) << "\n";
        for ( size_t bi = 0; bi < buffer.size(); ++bi )
        {
            outputFile << byteToHexStr( static_cast<unsigned char>( buffer.at( bi ) ) );
            if( !( (bi + 1) % 512) )
                outputFile << std::endl;
        }
    }

    outputFile << "\n};\n" << std::endl;

    outputFile <<
                  "S_ResourcesContainer::S_ResourcesContainer()\n"\
                  "{\n"\
                  "\tupdateResources( __S_NumberOfFiles, __S_FileNames, __S_FileSizes, __S_FilesData );\n" \
                  "}\n";

    outputFile.close();

    std::cout << "\"/solo/resource/S_Resources.cpp\" file has been generated!" << std::endl;
    return 0;

}
