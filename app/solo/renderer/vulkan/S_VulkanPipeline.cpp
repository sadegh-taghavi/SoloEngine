#include "S_VulkanRendererAPI.h"
#include "S_VulkanPipeline.h"
#include "S_VulkanAllocator.h"
#include "solo/stl/S_Map.h"
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

void S_VulkanPipeline::create( const S_Vector<S_PipelineDescriptor> *descriptors )
{
    if( descriptors )
        m_descriptors = *descriptors;

    size_t count = m_descriptors.size();
    if( count < 1 )
        return;

    m_layouts.resize( count );
    m_pipelines.resize( count );

    S_Vector< VkPipelineShaderStageCreateInfo > vertShaderStageInfos;
    vertShaderStageInfos.resize( count );
    S_Vector< VkPipelineShaderStageCreateInfo > fragShaderStageInfos;
    fragShaderStageInfos.resize( count );
    S_Vector< VkPipelineShaderStageCreateInfo > geometryShaderStageInfos;
    geometryShaderStageInfos.resize( count );
    S_Vector< VkPipelineShaderStageCreateInfo > computeShaderStageInfos;
    computeShaderStageInfos.resize( count );
    S_Vector< S_Vector<VkPipelineShaderStageCreateInfo> > shaderStagess;
    shaderStagess.resize( count );
    S_Vector< S_Vector<VkVertexInputBindingDescription> > bindingDescriptions;
    bindingDescriptions.resize( count );
    S_Vector< S_Vector<VkVertexInputAttributeDescription> > attributeDescriptions;
    attributeDescriptions.resize( count );
    S_Vector< VkPipelineVertexInputStateCreateInfo > vertexInputInfos;
    vertexInputInfos.resize( count );
    S_Vector< VkPipelineInputAssemblyStateCreateInfo > inputAssemblys;
    inputAssemblys.resize( count );
    S_Vector< VkViewport > viewports;
    viewports.resize( count );
    S_Vector< VkRect2D > scissors;
    scissors.resize( count );
    S_Vector< VkPipelineViewportStateCreateInfo > viewportStates;
    viewportStates.resize( count );
    S_Vector< VkPipelineRasterizationStateCreateInfo > rasterizers;
    rasterizers.resize( count );
    S_Vector< VkPipelineMultisampleStateCreateInfo > multisamplings;
    multisamplings.resize( count );
    S_Vector< VkPipelineColorBlendAttachmentState > colorBlendAttachments;
    colorBlendAttachments.resize( count );
    S_Vector< VkPipelineColorBlendStateCreateInfo > colorBlendings;
    colorBlendings.resize( count );
    S_Vector< VkDynamicState > dyanamicStatess;
    dyanamicStatess.resize( count );
    S_Vector< VkPipelineDynamicStateCreateInfo > dynamicCreateInfoStates;
    dynamicCreateInfoStates.resize( count );
    S_Vector< VkGraphicsPipelineCreateInfo > pipelineCreateInfos;
    pipelineCreateInfos.resize( count );

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
        colorBlendAttachments[i].blendEnable = VK_FALSE;
        colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;

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

        pipelineCreateInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfos[i].stageCount = static_cast<uint32_t>( shaderStagess[i].size() );
        pipelineCreateInfos[i].pStages = shaderStagess[i].data();

        pipelineCreateInfos[i].pVertexInputState = &vertexInputInfos[i];
        pipelineCreateInfos[i].pInputAssemblyState = &inputAssemblys[i];
        pipelineCreateInfos[i].pViewportState = &viewportStates[i];
        pipelineCreateInfos[i].pRasterizationState = &rasterizers[i];
        pipelineCreateInfos[i].pMultisampleState = &multisamplings[i];

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        pipelineCreateInfos[i].pDepthStencilState =  &depthStencil;
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
}

void S_VulkanPipeline::destroy()
{
    for( size_t i = 0; i < m_pipelines.size(); ++i )
    {
        vkDestroyPipeline( m_api->device(), m_pipelines.at( i ), S_VulkanAllocator() );
        vkDestroyPipelineLayout( m_api->device(), m_layouts.at( i ), S_VulkanAllocator() );
    }
}

void S_VulkanPipeline::recreate()
{
    //    destroy();
    create();
}

S_Vector<VkPipeline> *S_VulkanPipeline::pipelines()
{
    return &m_pipelines;
}

S_Vector<VkPipelineLayout> *S_VulkanPipeline::layouts()
{
    return &m_layouts;
}
