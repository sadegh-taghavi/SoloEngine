#include "S_VulkanShader.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include "solo/application/S_Application.h"
#include "solo/renderer/vulkan/S_VulkanAllocator.h"
#include "solo/renderer/vulkan/S_VulkanTexture.h"
#include "solo/renderer/vulkan/S_VulkanTextureSampler.h"
#include "solo/math/S_Math.h"
#include <algorithm>
#include <limits>

using namespace solo;

static uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

static constexpr uint32_t kMaxDrawsPerSlot  = 256;
static constexpr uint32_t kMaxSetsPerShader = 8;

S_VulkanShader::S_VulkanShader(S_VulkanRendererAPI *api, const std::string &vertexShader, const std::string &fragmentShader, const std::string &geometryShader, const std::string &computeShader):
    S_Shader(), m_maxUniformSetInStages(0), m_maxTextureSetInStages(0), m_commitsCount(0), m_uniformsMemorySize(0),
    m_uniformBuffers(VK_NULL_HANDLE), m_uniformBuffersAllocation(VK_NULL_HANDLE), m_uniformBuffersMappedData(nullptr), m_api( api )

{
    // NOTE: the per-draw, per-shader descriptor path (updateUniformValue /
    // updateTextureValue feeding per-shader sets) is unused — every pipeline renders
    // through the engine-global bindless sets (set 0 = perFrame, set 1 = bindless),
    // bound in commit()/flushRenderQueue. So we build the reflected set LAYOUTS (the
    // pipeline still needs them for sets >= 2) but allocate NO per-shader descriptor
    // pool, sets, or uniform buffer. The old allocation over-subscribed the pool
    // (a 64-texture set x 256 draws x frames) and only worked by luck on some drivers.
    m_bufferAlignment = glm::max( (uint64_t)1, (uint64_t)m_api->physicalDeviceProperties()->limits.minUniformBufferOffsetAlignment );
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

    uint32_t allStagesSetsCount = glm::max( m_maxUniformSetInStages, m_maxTextureSetInStages ) + 1;
    if( allStagesSetsCount > kMaxSetsPerShader )
    {
        s_debugLayer( "S_VulkanShader: allStagesSetsCount", allStagesSetsCount, "> kMaxSetsPerShader", kMaxSetsPerShader, "— clamped" );
        allStagesSetsCount = kMaxSetsPerShader;
    }

    std::vector< std::vector<VkDescriptorSetLayoutBinding> > layoutBindingList( allStagesSetsCount );

    VkDescriptorSetLayoutBinding binding;
    for( auto &reflectionData: m_shaderReflections )
    {
        binding.pImmutableSamplers = nullptr;
        for (auto &uniformBuffer: reflectionData.UniformBuffers)
        {
            if( uniformBuffer.Set >= allStagesSetsCount )
                continue;
            binding.binding = static_cast<unsigned int>(uniformBuffer.Binding);
            binding.stageFlags = m_STAGEMAP[ static_cast<int>( reflectionData.Reflection.Stage ) ];
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = uniformBuffer.ArraySize;
            uint64_t blockSize = static_cast<uint64_t>(uniformBuffer.ArraySize) * uniformBuffer.BlockSize;
            uint64_t aligned   = alignUp(blockSize, m_bufferAlignment);
            uint64_t newSize   = static_cast<uint64_t>(m_uniformsMemorySize) + aligned;
            if( newSize > std::numeric_limits<uint32_t>::max() )
            {
                s_debugLayer( "S_VulkanShader: uniform buffer total exceeds 4 GB — block skipped" );
                continue;
            }
            uniformBuffer.Offset = m_uniformsMemorySize; // globalOffset — set after overflow check
            m_uniformsMemorySize = static_cast<uint32_t>(newSize);
            layoutBindingList.at( uniformBuffer.Set ).push_back( binding );
        }

        for (auto texture: reflectionData.Textures)
        {
            if( texture.Set >= allStagesSetsCount )
                continue;
            binding.binding = static_cast<unsigned int>(texture.Binding);
            binding.stageFlags = m_STAGEMAP[ static_cast<int>( reflectionData.Reflection.Stage ) ];
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = texture.ArraySize;
            layoutBindingList.at( texture.Set ).push_back( binding );
        }
    }

    // Build only the reflected descriptor set LAYOUTS — the pipeline reads them for
    // sets >= 2 (sets 0/1 are replaced by the engine globals). No pool, sets, or
    // uniform buffer are allocated; the per-draw path that used them is unused.
    m_descriptorSetLayouts.resize( allStagesSetsCount, nullptr );
    for( uint32_t i = 0; i < allStagesSetsCount; ++i )
    {
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<unsigned int>( layoutBindingList.at(i).size() );
        layoutInfo.pBindings = layoutBindingList.at(i).data();
        VK_RESULT_CHECK( vkCreateDescriptorSetLayout(m_api->device(), &layoutInfo, S_VulkanAllocator(), &m_descriptorSetLayouts.at(i)) )
    }

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
    if( m_uniformBuffers != VK_NULL_HANDLE )
        vmaDestroyBuffer( m_api->vmaAllocator(), m_uniformBuffers, m_uniformBuffersAllocation );
}

void S_VulkanShader::updateUniformValue(const std::string &name, S_ShaderStage stage, const void *value)
{
    auto currentReflectionData = &m_shaderReflections[ static_cast<uint32_t>(stage) ];
    auto currentReflectionMap = &currentReflectionData->UniformBuffersMap;
    auto it = currentReflectionMap->find( name );
    if( it == currentReflectionMap->end() )
        return;
    auto uniformBuffer = &currentReflectionData->UniformBuffers.at( (*it).second );

    if( m_commitsCount >= kMaxDrawsPerSlot )
        return;
    if( uniformBuffer->Set >= m_aboutToUseDescriptorSets.size() )
        return;
    uint32_t descriptorBaseIndex = ( uniformBuffer->Set * (m_api->maxFramesInFlight() + 1) * kMaxDrawsPerSlot
                                    + m_api->nextSwapchainImageIndex() * kMaxDrawsPerSlot ) + m_commitsCount;

    size_t baseOffset = static_cast<size_t>(descriptorBaseIndex) * m_uniformsMemorySize + uniformBuffer->Offset;
    size_t copySize = static_cast<uint64_t>(uniformBuffer->ArraySize) * uniformBuffer->BlockSize;

    auto ptrAddress = reinterpret_cast<void *>(
                reinterpret_cast<size_t>( m_uniformBuffersMappedData )
                + baseOffset );

    memcpy( ptrAddress, value, copySize );

    m_dirtyMin = std::min( m_dirtyMin, baseOffset );
    m_dirtyMax = std::max( m_dirtyMax, baseOffset + copySize );

    size_t alignedSize = alignUp(copySize, m_bufferAlignment);

    m_descriptorBufferInfos.push_back( std::make_unique<VkDescriptorBufferInfo>() );
    VkDescriptorBufferInfo *bufferInfo = (*(m_descriptorBufferInfos.end() - 1)).get();
    bufferInfo->buffer = m_uniformBuffers;
    bufferInfo->offset = baseOffset;
    bufferInfo->range = alignedSize;

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

    m_aboutToWriteDescriptorSets.push_back( descriptorWrite );
    m_aboutToUseDescriptorSets[uniformBuffer->Set] = descriptorWrite.dstSet;

}

void S_VulkanShader::updateTextureValue(const std::string &name, S_ShaderStage stage, const S_Texture &texture, uint32_t arrayIndex)
{
    auto currentReflectionData = &m_shaderReflections[ static_cast<uint32_t>(stage) ];
    auto currentReflectionMap = &currentReflectionData->TextureMap;
    auto it = currentReflectionMap->find( name );
    if( it == currentReflectionMap->end() )
        return;
    auto textureBuffer = &currentReflectionData->Textures.at( (*it).second );

    if( m_commitsCount >= kMaxDrawsPerSlot )
        return;
    if( textureBuffer->Set >= m_aboutToUseDescriptorSets.size() )
        return;
    uint32_t descriptorBaseIndex = ( textureBuffer->Set * (m_api->maxFramesInFlight() + 1) * kMaxDrawsPerSlot
                                    + m_api->nextSwapchainImageIndex() * kMaxDrawsPerSlot ) + m_commitsCount;


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

void S_VulkanShader::clearPendingDescriptorState()
{
    m_aboutToWriteDescriptorSets.clear();
    m_descriptorBufferInfos.clear();
    m_descriptorImageInfos.clear();
}

void S_VulkanShader::bind()
{
    std::fill( m_aboutToUseDescriptorSets.begin(), m_aboutToUseDescriptorSets.end(), nullptr );
    clearPendingDescriptorState();
    m_commitsCount = 0;
    m_dirtyMin = SIZE_MAX;
    m_dirtyMax = 0;
}

void S_VulkanShader::commit()
{
    if( !m_aboutToUseDescriptorSets.size() )
        return;
    if( m_pipelineLayout == VK_NULL_HANDLE )
    {
        s_debugLayer( "S_VulkanShader::commit() called before setPipelineLayout()" );
        return;
    }
    if( m_pipeline != VK_NULL_HANDLE )
        vkCmdBindPipeline( m_api->nextFrameRenderCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );

    if( !m_aboutToUseDescriptorSets.empty() && m_aboutToUseDescriptorSets[0] == VK_NULL_HANDLE )
    {
        VkDescriptorSet pfSet = m_api->currentPerFrameSet();
        if( pfSet != VK_NULL_HANDLE )
            vkCmdBindDescriptorSets( m_api->nextFrameRenderCommandBuffer(),
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     m_pipelineLayout, 0, 1, &pfSet, 0, nullptr );
    }
    if( m_commitsCount >= kMaxDrawsPerSlot )
    {
        s_debugLayer( "S_VulkanShader: exceeded", kMaxDrawsPerSlot, "draw calls per frame-slot — draw skipped" );
        return;
    }
    if( m_aboutToWriteDescriptorSets.size() )
    {
        if( m_uniformBuffers != VK_NULL_HANDLE && m_dirtyMin < m_dirtyMax )
        {
            VK_RESULT_CHECK( vmaFlushAllocation( m_api->vmaAllocator(), m_uniformBuffersAllocation,
                                                 m_dirtyMin, m_dirtyMax - m_dirtyMin ) )
            m_dirtyMin = SIZE_MAX;
            m_dirtyMax = 0;
        }
        vkUpdateDescriptorSets( m_api->device(), static_cast<uint32_t>( m_aboutToWriteDescriptorSets.size() ), m_aboutToWriteDescriptorSets.data(), 0, nullptr );
        clearPendingDescriptorState();
    }

    uint32_t firstSet = std::numeric_limits<uint32_t>::max();
    uint32_t lastSet  = 0;
    bool     valid    = true;
    for( uint32_t i = 0; i < static_cast<uint32_t>( m_aboutToUseDescriptorSets.size() ); ++i )
    {
        if( m_aboutToUseDescriptorSets[i] != VK_NULL_HANDLE )
        {
            if( firstSet == std::numeric_limits<uint32_t>::max() ) firstSet = i;
            lastSet = i;
        }
        else if( firstSet != std::numeric_limits<uint32_t>::max() )
        {
            s_debugLayer( "S_VulkanShader: null descriptor set gap at slot", i, "— draw skipped" );
            valid = false;
            break;
        }
    }
    if( firstSet != std::numeric_limits<uint32_t>::max() && valid )
    {
        uint32_t count = lastSet - firstSet + 1;
        vkCmdBindDescriptorSets( m_api->nextFrameRenderCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 m_pipelineLayout, firstSet, count,
                                 m_aboutToUseDescriptorSets.data() + firstSet,
                                 0, nullptr );
        ++m_commitsCount;
    }
}

void S_VulkanShader::setShader(S_ShaderStage stage, const std::string &name)
{

    std::string shaderCodeName = name + m_EXTENSIONS[ static_cast<int>(stage) ] + "c";
    std::string shaderReflectionName = name + m_EXTENSIONS[ static_cast<int>(stage) ] + "r";

    auto* pack = S_Application::executingApplication()->pack();
    auto reflData = pack->load(shaderReflectionName);
    if( reflData.empty() )
    {
        s_debugLayer("Shader reflection not found!", shaderReflectionName );
        return;
    }
    auto codeData = pack->load(shaderCodeName);
    if( codeData.empty() )
    {
        s_debugLayer("Shader code not found!", shaderCodeName );
        return;
    }

    auto currentReflectionData = &m_shaderReflections[ static_cast<int>(stage) ];
    auto reflection = &currentReflectionData->Reflection;

    auto dataPtr = reflData.data();

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

    m_maxUniformSetInStages = glm::max( m_maxUniformSetInStages, currentReflectionData->Reflection.MaxUniformBuffersSet );
    m_maxTextureSetInStages = glm::max( m_maxTextureSetInStages, currentReflectionData->Reflection.MaxTextureSet );

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeData.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(codeData.data());
    VK_RESULT_CHECK( vkCreateShaderModule( m_api->device(), &createInfo, S_VulkanAllocator(),
                                           &m_shaderModules[static_cast<int>(stage)]) );
}

void S_VulkanShader::setPipelineLayout(VkPipelineLayout layout)
{
    m_pipelineLayout = layout;
}

void S_VulkanShader::setPipeline(VkPipeline pipeline)
{
    m_pipeline = pipeline;
}

VkShaderModule S_VulkanShader::shaderModule(S_ShaderStage type)
{
    return m_shaderModules[ static_cast<int>(type) ];
}

const S_ShaderReflectionData *S_VulkanShader::shaderReflection(S_ShaderStage type)
{
    return &m_shaderReflections[ static_cast<int>(type) ];
}

const std::vector<VkDescriptorSetLayout> *S_VulkanShader::descriptorSetLayouts()
{
    return &m_descriptorSetLayouts;
}
