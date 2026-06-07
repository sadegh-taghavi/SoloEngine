#include "S_VulkanRendererAPI.h"
#include "S_VulkanPipeline.h"
#include "S_VulkanAllocator.h"
#include <map>
#include "solo/application/S_Application.h"
#include "solo/debug/S_Debug.h"
#include "solo/renderer/S_Shader.h"
#include "solo/renderer/vulkan/S_VulkanShader.h"
#include <stdint.h>
#include <algorithm>
#include <set>

using namespace solo;

S_VulkanPipeline::S_VulkanPipeline(S_VulkanRendererAPI *api): m_api( api )
{

}

S_VulkanPipeline::~S_VulkanPipeline()
{

}

void S_VulkanPipeline::create( const std::vector<S_PipelineDescriptor> *descriptors )
{
    if( descriptors )
        m_descriptors = *descriptors;

    size_t count = m_descriptors.size();
    if( count < 1 )
        return;

    for( size_t i = 0; i < count; ++i )
    {
        if( !m_descriptors[i].Shader )
        {
            s_debugLayer( "S_VulkanPipeline::create() — descriptor", i, "has null Shader" );
            return;
        }
    }

    m_layouts.resize( count );
    m_pipelines.resize( count );

    std::vector< VkPipelineShaderStageCreateInfo > vertShaderStageInfos;
    vertShaderStageInfos.resize( count );
    std::vector< VkPipelineShaderStageCreateInfo > fragShaderStageInfos;
    fragShaderStageInfos.resize( count );
    std::vector< VkPipelineShaderStageCreateInfo > geometryShaderStageInfos;
    geometryShaderStageInfos.resize( count );
    std::vector< VkPipelineShaderStageCreateInfo > computeShaderStageInfos;
    computeShaderStageInfos.resize( count );
    std::vector< std::vector<VkPipelineShaderStageCreateInfo> > shaderStagess;
    shaderStagess.resize( count );
    std::vector< std::vector<VkVertexInputBindingDescription> > bindingDescriptions;
    bindingDescriptions.resize( count );
    std::vector< std::vector<VkVertexInputAttributeDescription> > attributeDescriptions;
    attributeDescriptions.resize( count );
    std::vector< VkPipelineVertexInputStateCreateInfo > vertexInputInfos;
    vertexInputInfos.resize( count );
    std::vector< VkPipelineInputAssemblyStateCreateInfo > inputAssemblys;
    inputAssemblys.resize( count );
    std::vector< VkViewport > viewports;
    viewports.resize( count );
    std::vector< VkRect2D > scissors;
    scissors.resize( count );
    std::vector< VkPipelineViewportStateCreateInfo > viewportStates;
    viewportStates.resize( count );
    std::vector< VkPipelineRasterizationStateCreateInfo > rasterizers;
    rasterizers.resize( count );
    std::vector< VkPipelineMultisampleStateCreateInfo > multisamplings;
    multisamplings.resize( count );
    std::vector< VkPipelineColorBlendAttachmentState > colorBlendAttachments;
    colorBlendAttachments.resize( count );
    std::vector< VkPipelineColorBlendStateCreateInfo > colorBlendings;
    colorBlendings.resize( count );
    std::vector< VkDynamicState > dyanamicStatess;
    dyanamicStatess.resize( count );
    std::vector< VkPipelineDynamicStateCreateInfo > dynamicCreateInfoStates;
    dynamicCreateInfoStates.resize( count );
    std::vector< VkGraphicsPipelineCreateInfo > pipelineCreateInfos;
    pipelineCreateInfos.resize( count );
    std::vector< VkPipelineDepthStencilStateCreateInfo > depthStencils;
    depthStencils.resize( count );

    S_VulkanShader *shader;
    VkShaderModule shaderModule;
    for( size_t i = 0; i < m_descriptors.size(); ++i )
    {
        if( ( shader = reinterpret_cast<S_VulkanShader *>( m_descriptors.at( i ).Shader ) ) )
        {
            if( ( shaderModule = shader->shaderModule( S_ShaderStage::VertexShader ) ) )
            {
                vertShaderStageInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                vertShaderStageInfos[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
                vertShaderStageInfos[i].module = shaderModule;
                vertShaderStageInfos[i].pName = shader->shaderReflection( S_ShaderStage::VertexShader )->Reflection.EntryPointName;
                shaderStagess[i].push_back( vertShaderStageInfos.at( i ) );
            }

            if( ( shaderModule = shader->shaderModule( S_ShaderStage::FragmentShader ) ) )
            {
                fragShaderStageInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                fragShaderStageInfos[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                fragShaderStageInfos[i].module = shaderModule;
                fragShaderStageInfos[i].pName = shader->shaderReflection( S_ShaderStage::FragmentShader )->Reflection.EntryPointName;
                shaderStagess[i].push_back( fragShaderStageInfos.at( i ) );
            }

            if( ( shaderModule = shader->shaderModule( S_ShaderStage::GeometryShader ) ) )
            {
                geometryShaderStageInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                geometryShaderStageInfos[i].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                geometryShaderStageInfos[i].module = shaderModule;
                geometryShaderStageInfos[i].pName = shader->shaderReflection( S_ShaderStage::GeometryShader )->Reflection.EntryPointName;
                shaderStagess[i].push_back( geometryShaderStageInfos.at( i ) );
            }

            if( ( shaderModule = shader->shaderModule( S_ShaderStage::ComputeShader ) ) )
            {
                computeShaderStageInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                computeShaderStageInfos[i].stage = VK_SHADER_STAGE_COMPUTE_BIT;
                computeShaderStageInfos[i].module = shaderModule;
                computeShaderStageInfos[i].pName = shader->shaderReflection( S_ShaderStage::ComputeShader )->Reflection.EntryPointName;
                shaderStagess[i].push_back( computeShaderStageInfos.at( i ) );
            }
        }

        uint32_t location = 0;
        auto fillVertexBindingsAndAttributes = [&attributeDescriptions, &bindingDescriptions, i, &location](const S_VertexBufferDescriptorArray &descriptorArray, bool instance)
        {
            VkVertexInputBindingDescription vertexInputBindingDescription;

            vertexInputBindingDescription.binding = instance ? 1 : 0;
            vertexInputBindingDescription.inputRate = instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            vertexInputBindingDescription.stride = descriptorArray.stride();

            bindingDescriptions.at(i).push_back( vertexInputBindingDescription );

            VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
            vertexInputAttributeDescription.binding = instance ? 1 : 0;
            for(const auto vertexBufferDesriptor: *descriptorArray.descriptors() )
            {
                vertexInputAttributeDescription.format = static_cast<VkFormat>(vertexBufferDesriptor.Format);
                vertexInputAttributeDescription.offset = vertexBufferDesriptor.Offset;
                vertexInputAttributeDescription.location = location;
                attributeDescriptions.at(i).push_back(vertexInputAttributeDescription);
                ++location;
            }
        };

        fillVertexBindingsAndAttributes( m_descriptors.at(i).VertexBufferDescriptorArray, false );
        if(m_descriptors.at(i).InstanceBufferDescriptorArray.stride() > 0 )
            fillVertexBindingsAndAttributes( m_descriptors.at(i).InstanceBufferDescriptorArray, true );

        vertexInputInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfos[i].vertexBindingDescriptionCount = static_cast<uint32_t>( bindingDescriptions.at(i).size() );
        vertexInputInfos[i].pVertexBindingDescriptions = bindingDescriptions.at(i).data();
        vertexInputInfos[i].vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDescriptions.at(i).size() );
        vertexInputInfos[i].pVertexAttributeDescriptions = attributeDescriptions.at(i).data();


        inputAssemblys[i].sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblys[i].topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblys[i].primitiveRestartEnable = VK_FALSE;

        viewports[i].x = 0.0f;
        viewports[i].y = 0.0f;
        viewports[i].width = static_cast<float>( m_api->swapChainExtent().width );
        viewports[i].height = static_cast<float>( m_api->swapChainExtent().height );
        viewports[i].minDepth = 0.0f;
        viewports[i].maxDepth = 1.0f;

        scissors[i].offset = {0, 0};
        scissors[i].extent = m_api->swapChainExtent();

        viewportStates[i].sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStates[i].viewportCount = 1;
        viewportStates[i].pViewports = &viewports[i];
        viewportStates[i].scissorCount = 1;
        viewportStates[i].pScissors = &scissors[i];

        rasterizers[i].sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizers[i].depthClampEnable = VK_FALSE;

        rasterizers[i].rasterizerDiscardEnable = VK_FALSE;

        rasterizers[i].polygonMode = VK_POLYGON_MODE_FILL;

        rasterizers[i].lineWidth = 1.0f;

        rasterizers[i].cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizers[i].frontFace = VK_FRONT_FACE_CLOCKWISE;

        rasterizers[i].depthBiasEnable = VK_FALSE;
        rasterizers[i].depthBiasConstantFactor = 0.0f;
        rasterizers[i].depthBiasClamp = 0.0f;
        rasterizers[i].depthBiasSlopeFactor = 0.0f;


        multisamplings[i].sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplings[i].sampleShadingEnable = VK_FALSE;
        multisamplings[i].rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplings[i].minSampleShading = 1.0f;
        multisamplings[i].pSampleMask = nullptr;
        multisamplings[i].alphaToCoverageEnable = VK_FALSE;
        multisamplings[i].alphaToOneEnable = VK_FALSE;

        colorBlendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachments[i].blendEnable = VK_TRUE;
        colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;

        colorBlendings[i].sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendings[i].logicOpEnable = VK_FALSE;
        colorBlendings[i].logicOp = VK_LOGIC_OP_COPY;
        colorBlendings[i].attachmentCount = 1;
        colorBlendings[i].pAttachments = &colorBlendAttachments[i];
        colorBlendings[i].blendConstants[0] = 0.0f;
        colorBlendings[i].blendConstants[1] = 0.0f;
        colorBlendings[i].blendConstants[2] = 0.0f;
        colorBlendings[i].blendConstants[3] = 0.0f;

        //        VkDynamicState dynamicStates[] = {
        //            VK_DYNAMIC_STATE_VIEWPORT,
        //            VK_DYNAMIC_STATE_LINE_WIDTH
        //        };

        //        dynamicStates.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        //        dynamicStates.dynamicStateCount = 2;
        //        dynamicStates.pDynamicStates = dynamicStates;


        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>( shader->descriptorSetLayouts()->size() );

        pipelineLayoutInfo.pSetLayouts = shader->descriptorSetLayouts()->data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        VK_RESULT_CHECK( vkCreatePipelineLayout( m_api->device(), &pipelineLayoutInfo, S_VulkanAllocator(),
                                                 &m_layouts[i]) );
        shader->setPipelineLayout( m_layouts[i] );

        pipelineCreateInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfos[i].stageCount = static_cast<uint32_t>( shaderStagess[i].size() );
        pipelineCreateInfos[i].pStages = shaderStagess[i].data();

        pipelineCreateInfos[i].pVertexInputState = &vertexInputInfos[i];
        pipelineCreateInfos[i].pInputAssemblyState = &inputAssemblys[i];
        pipelineCreateInfos[i].pViewportState = &viewportStates[i];
        pipelineCreateInfos[i].pRasterizationState = &rasterizers[i];
        pipelineCreateInfos[i].pMultisampleState = &multisamplings[i];

        depthStencils[i].sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencils[i].depthTestEnable = VK_TRUE;
        depthStencils[i].depthWriteEnable = VK_TRUE;
        depthStencils[i].depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencils[i].depthBoundsTestEnable = VK_FALSE;
        depthStencils[i].minDepthBounds = 0.0f;
        depthStencils[i].maxDepthBounds = 1.0f;
        depthStencils[i].stencilTestEnable = VK_FALSE;
        depthStencils[i].front = {};
        depthStencils[i].back = {};

        pipelineCreateInfos[i].pDepthStencilState = &depthStencils[i];
        pipelineCreateInfos[i].pColorBlendState = &colorBlendings[i];
        pipelineCreateInfos[i].pDynamicState = nullptr;

        pipelineCreateInfos[i].layout = m_layouts[i];
        pipelineCreateInfos[i].renderPass = m_api->renderPass();
        pipelineCreateInfos[i].subpass = 0;

        pipelineCreateInfos[i].basePipelineHandle = nullptr;
        pipelineCreateInfos[i].basePipelineIndex = -1;

    }

    VK_RESULT_CHECK( vkCreateGraphicsPipelines( m_api->device(), nullptr,
                                                static_cast<uint32_t>(pipelineCreateInfos.size()),
                                                pipelineCreateInfos.data(), S_VulkanAllocator(),
                                                &m_pipelines[0] ) );

    for( size_t i = 0; i < m_descriptors.size(); ++i )
    {
        auto* shader = reinterpret_cast<S_VulkanShader*>( m_descriptors.at(i).Shader );
        if( shader )
            shader->setPipeline( m_pipelines[i] );
    }
}

void S_VulkanPipeline::destroy()
{
    for( size_t i = 0; i < m_pipelines.size(); ++i )
    {
        vkDestroyPipeline( m_api->device(), m_pipelines.at( i ), S_VulkanAllocator() );
        vkDestroyPipelineLayout( m_api->device(), m_layouts.at( i ), S_VulkanAllocator() );
    }
    m_pipelines.clear();
    m_layouts.clear();
    for( auto &desc : m_descriptors )
    {
        auto *shader = reinterpret_cast<S_VulkanShader *>( desc.Shader );
        if( shader )
        {
            shader->setPipelineLayout( VK_NULL_HANDLE );
            shader->setPipeline( VK_NULL_HANDLE );
        }
    }
}

void S_VulkanPipeline::recreate()
{
    if( m_descriptors.empty() )
        return;
    destroy();
    create();
}

std::vector<VkPipeline> *S_VulkanPipeline::pipelines()
{
    return &m_pipelines;
}

std::vector<VkPipelineLayout> *S_VulkanPipeline::layouts()
{
    return &m_layouts;
}
