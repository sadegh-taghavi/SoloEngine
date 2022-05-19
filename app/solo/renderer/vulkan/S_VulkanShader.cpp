#include "S_VulkanShader.h"
#include "solo/memory/S_Allocator.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include "solo/file/S_File.h"
#include "solo/application/S_Application.h"
#include "solo/renderer/vulkan/S_VulkanAllocator.h"
#include "solo/renderer/vulkan/S_VulkanTexture.h"
#include "solo/renderer/vulkan/S_VulkanTextureSampler.h"
#include "solo/math/S_Math.h"

using namespace solo;

S_VulkanShader::S_VulkanShader(S_VulkanRendererAPI *api, const S_String &vertexShader, const S_String &fragmentShader, const S_String &geometryShader, const S_String &computeShader):
    S_Shader(), m_maxUniformSetInStages(0), m_maxTextureSetInStages(0), m_commitsCount(0), m_uniformsMemorySize(0), m_api( api )

{
    S_Array< VkDescriptorPoolSize, 2> poolSizes;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = (m_api->maxFramesInFlight() + 1) * 256 * 8;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = (m_api->maxFramesInFlight() + 1) * 256 * 8;
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = (m_api->maxFramesInFlight() + 1) * 256 * 8;
    VK_RESULT_CHECK( vkCreateDescriptorPool(m_api->device(), &poolInfo, S_VulkanAllocator(), &m_descriptorsPool) );

    m_bufferAlignment = m_api->deviceAllocator()->getAlign(1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    for( size_t i = 0; i < static_cast<int>(S_ShaderStage::Count); ++i )
        m_shaderModules[i] = nullptr;
    if(!vertexShader.empty())
        setShader( S_ShaderStage::VertexShader, vertexShader );
    if(!fragmentShader.empty())
        setShader( S_ShaderStage::FragmentShader, fragmentShader );
    if(!geometryShader.empty())
        setShader( S_ShaderStage::GeometryShader, geometryShader );
    if(!computeShader.empty())
        setShader( S_ShaderStage::ComputeShader, computeShader );

    uint32_t allStagesSetsCount = solo::max( m_maxUniformSetInStages, m_maxTextureSetInStages ) + 1;

    S_Vector< S_Vector<VkDescriptorSetLayoutBinding> > layoutBindingList( allStagesSetsCount );

    VkDescriptorSetLayoutBinding binding;
    for( auto &reflectionData: m_shaderReflections )
    {
        binding.pImmutableSamplers = nullptr;
        for (auto &uniformBuffer: reflectionData.UniformBuffers)
        {
            binding.binding = static_cast<unsigned int>(uniformBuffer.Binding);
            binding.stageFlags = m_STAGEMAP[ static_cast<int>( reflectionData.Reflection.Stage ) ];
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = uniformBuffer.ArraySize;
            uniformBuffer.Offset = m_uniformsMemorySize; // globalOffset
            m_uniformsMemorySize += S_Allocator::makeAlign( uniformBuffer.ArraySize * uniformBuffer.BlockSize, m_bufferAlignment );
            layoutBindingList.at( uniformBuffer.Set ).push_back( binding );
        }

        for (auto texture: reflectionData.Textures)
        {
            binding.binding = static_cast<unsigned int>(texture.Binding);
            binding.stageFlags = m_STAGEMAP[ static_cast<int>( reflectionData.Reflection.Stage ) ];
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = texture.ArraySize;
            layoutBindingList.at( texture.Set ).push_back( binding );
        }
    }

    if( m_uniformsMemorySize )
    {
        m_api->deviceAllocator()->createBuffer( m_uniformsMemorySize * (m_api->maxFramesInFlight() + 1) * 256 * ( m_maxUniformSetInStages + 1 ),
                                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_uniformBuffers, m_uniformBuffersMemory );
    }

    m_descriptorSetLayouts.resize( allStagesSetsCount, nullptr );

    S_Vector<VkDescriptorSetLayout> layouts( (m_api->maxFramesInFlight() + 1) * 256 * allStagesSetsCount );

    for( uint32_t i = 0; i < allStagesSetsCount; ++i )
    {
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<unsigned int>( layoutBindingList.at(i).size() );
        layoutInfo.pBindings = layoutBindingList.at(i).data();

        VK_RESULT_CHECK( vkCreateDescriptorSetLayout(m_api->device(), &layoutInfo, S_VulkanAllocator(), &m_descriptorSetLayouts.at(i)) )
        std::fill_n( layouts.begin() + (m_api->maxFramesInFlight() + 1) * 256 * i, (m_api->maxFramesInFlight() + 1) * 256, m_descriptorSetLayouts.at(i) );
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorsPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>( layouts.size() );
    allocInfo.pSetLayouts = layouts.data();
    m_descriptorSets.resize( (m_api->maxFramesInFlight() + 1) * 256 * allStagesSetsCount );
    VK_RESULT_CHECK( vkAllocateDescriptorSets( m_api->device(), &allocInfo, m_descriptorSets.data() ) );

    m_aboutToUseDescriptorSets.resize( allStagesSetsCount, nullptr );

}

S_VulkanShader::~S_VulkanShader()
{
    vkDestroyDescriptorPool( m_api->device(), m_descriptorsPool, S_VulkanAllocator() );

    for (auto &descriptorSetLayout : m_descriptorSetLayouts)
        vkDestroyDescriptorSetLayout(m_api->device(), descriptorSetLayout, S_VulkanAllocator() );

    for( size_t i = 0; i < static_cast<int>(S_ShaderStage::Count); ++i )
    {
        if( m_shaderModules[i] )
            vkDestroyShaderModule( m_api->device(), m_shaderModules[i], S_VulkanAllocator() );
    }
    m_api->deviceAllocator()->destroy( m_uniformBuffers );
}

void S_VulkanShader::updateUniformValue(const S_String &name, S_ShaderStage stage, const void *value)
{
    auto currentReflectionData = &m_shaderReflections[ static_cast<uint32_t>(stage) ];
    auto currentReflectionMap = &currentReflectionData->UniformBuffersMap;
    auto it = currentReflectionMap->find( name );
    if( it == currentReflectionMap->end() )
        return;
    auto uniformBuffer = &currentReflectionData->UniformBuffers.at( (*it).second );

    uint32_t descriptorBaseIndex = ( uniformBuffer->Set * (m_api->maxFramesInFlight() + 1) * 256
                                    + m_api->nextSwapchainImageIndex() * 256 ) + m_commitsCount;

    VkMappedMemoryRange memoryRange;

    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = m_uniformBuffersMemory.Memory;
    memoryRange.pNext = nullptr;

    memoryRange.size = uniformBuffer->ArraySize * uniformBuffer->BlockSize;
    size_t baseOffset = descriptorBaseIndex * m_uniformsMemorySize + uniformBuffer->Offset;
    memoryRange.offset = baseOffset + m_uniformBuffersMemory.OffsetFromBufferBase;

    auto ptrAddress = reinterpret_cast<void *>(
                reinterpret_cast<size_t>( m_uniformBuffersMemory.MappedPtr )
                + baseOffset );

    memcpy( ptrAddress, value, memoryRange.size );

    memoryRange.size = S_Allocator::makeAlign( memoryRange.size, m_bufferAlignment);

    m_descriptorBufferInfos.push_back( std::make_unique<VkDescriptorBufferInfo>() );
    VkDescriptorBufferInfo *bufferInfo = (*(m_descriptorBufferInfos.end() - 1)).get();
    bufferInfo->buffer = m_uniformBuffers;
    bufferInfo->offset = memoryRange.offset;
    bufferInfo->range = memoryRange.size;

//    s_debug("######################", __LINE__, descriptorBaseIndex, m_descriptorSets.size(), uniformBuffer->Set, m_api->nextSwapchainImageIndex());
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSets.at( descriptorBaseIndex );
    descriptorWrite.dstBinding = uniformBuffer->Binding;
    descriptorWrite.dstArrayElement = 0;

    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;

    descriptorWrite.pBufferInfo = bufferInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    m_aboutToWriteMemoryRanges.push_back( memoryRange );
    m_aboutToWriteDescriptorSets.push_back( descriptorWrite );
    m_aboutToUseDescriptorSets[uniformBuffer->Set] = descriptorWrite.dstSet;

}

void S_VulkanShader::updateTextureValue(const S_String &name, S_ShaderStage stage, const S_Texture &texture, uint32_t arrayIndex)
{
    auto currentReflectionData = &m_shaderReflections[ static_cast<uint32_t>(stage) ];
    auto currentReflectionMap = &currentReflectionData->TextureMap;
    auto it = currentReflectionMap->find( name );
    if( it == currentReflectionMap->end() )
        return;
    auto textureBuffer = &currentReflectionData->Textures.at( (*it).second );

    uint32_t descriptorBaseIndex = ( textureBuffer->Set * (m_api->maxFramesInFlight() + 1) * 256
                                    + m_api->nextSwapchainImageIndex() * 256 ) + m_commitsCount;


    m_descriptorImageInfos.push_back( std::make_unique<VkDescriptorImageInfo>() );
    VkDescriptorImageInfo *imageInfo = (*(m_descriptorImageInfos.end() - 1)).get();
    auto vtexture = dynamic_cast<const S_VulkanTexture *>( &texture );
    imageInfo->sampler = dynamic_cast<const S_VulkanTextureSampler*>( vtexture->sampler())->sampler();
    imageInfo->imageView = vtexture->view();
    imageInfo->imageLayout = vtexture->layout();

//    s_debug("######################", __LINE__, descriptorBaseIndex, m_descriptorSets.size(), uniformBuffer->Set, m_api->nextSwapchainImageIndex());
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSets.at( descriptorBaseIndex );
    descriptorWrite.dstBinding = textureBuffer->Binding;
    descriptorWrite.dstArrayElement = arrayIndex;

    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;

    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pImageInfo = imageInfo;
    descriptorWrite.pTexelBufferView = nullptr;

    m_aboutToWriteDescriptorSets.push_back( descriptorWrite );
    m_aboutToUseDescriptorSets[textureBuffer->Set] = descriptorWrite.dstSet;
}

void S_VulkanShader::bind()
{
    std::fill( m_aboutToUseDescriptorSets.begin(), m_aboutToUseDescriptorSets.end(), nullptr );
    m_commitsCount = 0;
}

void S_VulkanShader::commit()
{
    if( !m_aboutToUseDescriptorSets.size() )
        return;
    if( m_aboutToWriteMemoryRanges.size() )
    {
        VK_RESULT_CHECK( vkFlushMappedMemoryRanges( m_api->device(),
                                                    static_cast<uint32_t>( m_aboutToWriteMemoryRanges.size() ), m_aboutToWriteMemoryRanges.data() ) );
        m_aboutToWriteMemoryRanges.clear();
    }

    if( m_aboutToWriteDescriptorSets.size() )
    {
        vkUpdateDescriptorSets( m_api->device(), static_cast<uint32_t>( m_aboutToWriteDescriptorSets.size() ), m_aboutToWriteDescriptorSets.data(), 0, nullptr );
        m_aboutToWriteDescriptorSets.clear();
        m_descriptorBufferInfos.clear();
        m_descriptorImageInfos.clear();
    }

    vkCmdBindDescriptorSets( m_api->nextFrameRenderCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                             m_api->pipelines()->layouts()->at( 0 ), 0, static_cast<uint32_t>( m_aboutToUseDescriptorSets.size() ),
                             m_aboutToUseDescriptorSets.data(),
                             0, nullptr);
    ++m_commitsCount;
}

void S_VulkanShader::setShader(S_ShaderStage stage, const S_String &name)
{

    S_String shaderCodeName = name + m_EXTENSIONS[ static_cast<int>(stage) ] + "c";
    S_String shaderReflectionName = name + m_EXTENSIONS[ static_cast<int>(stage) ] + "r";

    S_File shaderCodeFile(shaderCodeName);
    S_File shaderReflectionFile(shaderReflectionName);
    if( !shaderCodeFile.open() )
    {
        s_debugLayer("Shader code not found!", shaderCodeName );
        return;
    }

    if( !shaderReflectionFile.open() )
    {
        s_debugLayer("Shader reflection not found!", shaderReflectionName );
        return;
    }

    S_Vector<std::byte> fileData(shaderReflectionFile.size());
    shaderReflectionFile.read( reinterpret_cast<char *>(fileData.data()), fileData.size() );
    shaderReflectionFile.close();

    auto currentReflectionData = &m_shaderReflections[ static_cast<int>(stage) ];
    auto reflection = &currentReflectionData->Reflection;

    auto dataPtr = fileData.data();

    memcpy( reflection, dataPtr, sizeof(S_VulkanShaderReflection) );

    dataPtr += sizeof(S_VulkanShaderReflection);

    if( reflection->NumberOfUniformBuffers > 0 )
    {
        currentReflectionData->UniformBuffers.resize( static_cast<size_t>(reflection->NumberOfUniformBuffers) );

        memcpy( currentReflectionData->UniformBuffers.data(), dataPtr,
                sizeof(S_VulkanShaderReflectionUniformBuffer) * static_cast<size_t>(reflection->NumberOfUniformBuffers ) );
        dataPtr += sizeof(S_VulkanShaderReflectionUniformBuffer) * static_cast<size_t>(reflection->NumberOfUniformBuffers );
    }

    if( reflection->NumberOfTextures > 0 )
    {
        currentReflectionData->Textures.resize( static_cast<size_t>(reflection->NumberOfTextures) );

        memcpy( currentReflectionData->Textures.data(), dataPtr,
                sizeof(S_VulkanShaderReflectionTexture) * static_cast<size_t>(reflection->NumberOfTextures ) );
        dataPtr += sizeof(S_VulkanShaderReflectionTexture) * static_cast<size_t>(reflection->NumberOfTextures );
    }

    uint32_t index = 0;
    for (const auto &uniform : currentReflectionData->UniformBuffers)
    {
        currentReflectionData->UniformBuffersMap[uniform.Name] = index;
        ++index;
    }

    index = 0;
    for (const auto &texture : currentReflectionData->Textures)
    {
        currentReflectionData->TextureMap[texture.Name] = index;
        ++index;
    }

    m_maxUniformSetInStages = solo::max( m_maxTextureSetInStages, currentReflectionData->Reflection.MaxUniformBuffersSet );
    m_maxTextureSetInStages = solo::max( m_maxTextureSetInStages, currentReflectionData->Reflection.MaxTextureSet );

    fileData.resize( shaderCodeFile.size() );
    shaderCodeFile.read( reinterpret_cast<char *>(fileData.data()), fileData.size() );
    shaderCodeFile.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileData.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(fileData.data());
    VK_RESULT_CHECK( vkCreateShaderModule( m_api->device(), &createInfo, S_VulkanAllocator(),
                                           &m_shaderModules[static_cast<int>(stage)]) );
}

VkShaderModule S_VulkanShader::shaderModule(S_ShaderStage type)
{
    return m_shaderModules[ static_cast<int>(type) ];
}

const S_ShaderReflectionData *S_VulkanShader::shaderReflection(S_ShaderStage type)
{
    return &m_shaderReflections[ static_cast<int>(type) ];
}

const S_Vector<VkDescriptorSetLayout> *S_VulkanShader::descriptorSetLayouts()
{
    return &m_descriptorSetLayouts;
}
