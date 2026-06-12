#include "S_UI.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include "solo/renderer/vulkan/S_VulkanAllocator.h"
#include "solo/platforms/S_InputEvent.h"
#include "solo/application/S_Application.h"
#include "solo/pack/S_Pack.h"
#include "solo/debug/S_Debug.h"
#include <cstring>

using namespace solo;

S_UI* S_UI::s_instance = nullptr;

S_UI::S_UI(S_VulkanRendererAPI* api, const std::string& uiBinPath)
    : m_api(api), m_lastFrame(std::chrono::steady_clock::now())
{
    auto fileData = S_Application::executingApplication()->pack()->load(uiBinPath);
    if (fileData.size() < sizeof(UIBinHeader))
    {
        s_debugLayer("S_UI: ui.bin missing or truncated", uiBinPath);
        return;
    }

    memcpy(&m_info, fileData.data(), sizeof(m_info));
    if (m_info.magic != UI_BIN_MAGIC || m_info.version != UI_BIN_VERSION)
    {
        s_debugLayer("S_UI: bad ui.bin", uiBinPath);
        return;
    }

    m_glyphs.resize(m_info.glyphCount);
    memcpy(m_glyphs.data(), fileData.data() + m_info.glyphOffset, m_info.glyphCount * sizeof(UIBinGlyph));
    for (uint32_t i = 0; i < m_info.glyphCount; ++i)
        m_glyphOfCodepoint[m_glyphs[i].codepoint] = i;

    m_sprites.resize(m_info.spriteCount);
    memcpy(m_sprites.data(), fileData.data() + m_info.spriteOffset, m_info.spriteCount * sizeof(UIBinSprite));

    uploadAtlas(fileData.data() + m_info.fontPixelsOffset, m_info.fontAtlasWidth, m_info.fontAtlasHeight,
                VK_FORMAT_R8_UNORM, m_fontImage, m_fontAlloc, m_fontView);
    uploadAtlas(fileData.data() + m_info.spritePixelsOffset, m_info.spriteAtlasWidth, m_info.spriteAtlasHeight,
                VK_FORMAT_R8G8B8A8_UNORM, m_spriteImage, m_spriteAlloc, m_spriteView);

    createDeviceObjects();
    createPipeline();

    s_instance = this;
}

S_UI::~S_UI()
{
    s_instance = nullptr;
    VkDevice device = m_api->device();
    vkDeviceWaitIdle(device);

    for (uint32_t i = 0; i < kMaxSlots; ++i)
    {
        if (m_vertexBuffers[i]) vmaDestroyBuffer(m_api->vmaAllocator(), m_vertexBuffers[i], m_vertexAllocs[i]);
        if (m_indexBuffers[i])  vmaDestroyBuffer(m_api->vmaAllocator(), m_indexBuffers[i],  m_indexAllocs[i]);
    }
    if (m_pipeline)       vkDestroyPipeline(device, m_pipeline, S_VulkanAllocator());
    if (m_pipelineLayout) vkDestroyPipelineLayout(device, m_pipelineLayout, S_VulkanAllocator());
    if (m_vertModule)     vkDestroyShaderModule(device, m_vertModule, S_VulkanAllocator());
    if (m_fragModule)     vkDestroyShaderModule(device, m_fragModule, S_VulkanAllocator());
    if (m_descPool)       vkDestroyDescriptorPool(device, m_descPool, S_VulkanAllocator());
    if (m_setLayout)      vkDestroyDescriptorSetLayout(device, m_setLayout, S_VulkanAllocator());
    if (m_sampler)        vkDestroySampler(device, m_sampler, S_VulkanAllocator());
    if (m_fontView)       vkDestroyImageView(device, m_fontView, S_VulkanAllocator());
    if (m_fontImage)      vmaDestroyImage(m_api->vmaAllocator(), m_fontImage, m_fontAlloc);
    if (m_spriteView)     vkDestroyImageView(device, m_spriteView, S_VulkanAllocator());
    if (m_spriteImage)    vmaDestroyImage(m_api->vmaAllocator(), m_spriteImage, m_spriteAlloc);
}

void S_UI::uploadAtlas(const uint8_t* pixels, uint32_t w, uint32_t h, VkFormat format,
                       VkImage& image, VmaAllocation& alloc, VkImageView& view)
{
    const uint32_t     bpp  = (format == VK_FORMAT_R8_UNORM) ? 1 : 4;
    const VkDeviceSize size = static_cast<VkDeviceSize>(w) * h * bpp;

    VkBuffer      stageBuf;
    VmaAllocation stageAlloc;
    VkBufferCreateInfo stageCI{};
    stageCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stageCI.size  = size;
    stageCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo stageAllocCI{};
    stageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
    stageAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo stageInfo;
    VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &stageCI, &stageAllocCI, &stageBuf, &stageAlloc, &stageInfo) )
    memcpy(stageInfo.pMappedData, pixels, size);
    vmaFlushAllocation(m_api->vmaAllocator(), stageAlloc, 0, VK_WHOLE_SIZE);

    VkImageCreateInfo imageCI{};
    imageCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType     = VK_IMAGE_TYPE_2D;
    imageCI.format        = format;
    imageCI.extent        = { w, h, 1 };
    imageCI.mipLevels     = 1;
    imageCI.arrayLayers   = 1;
    imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocationCreateInfo imageAllocCI{};
    imageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
    VK_RESULT_CHECK( vmaCreateImage(m_api->vmaAllocator(), &imageCI, &imageAllocCI, &image, &alloc, nullptr) )

    VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkCommandBuffer cmd = m_api->beginSingleTimeTransferCommands();
    m_api->setImageLayout(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
    VkBufferImageCopy region{};
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageExtent      = { w, h, 1 };
    vkCmdCopyBufferToImage(cmd, stageBuf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    m_api->setImageLayout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
    m_api->endSingleTimeTransferCommands(cmd);

    vmaDestroyBuffer(m_api->vmaAllocator(), stageBuf, stageAlloc);

    VkImageViewCreateInfo viewCI{};
    viewCI.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.image            = image;
    viewCI.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format           = format;
    viewCI.subresourceRange = range;
    VK_RESULT_CHECK( vkCreateImageView(m_api->device(), &viewCI, S_VulkanAllocator(), &view) )
}

void S_UI::createDeviceObjects()
{
    VkDevice device = m_api->device();

    VkSamplerCreateInfo samplerCI{};
    samplerCI.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.magFilter    = VK_FILTER_LINEAR;
    samplerCI.minFilter    = VK_FILTER_LINEAR;
    samplerCI.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VK_RESULT_CHECK( vkCreateSampler(device, &samplerCI, S_VulkanAllocator(), &m_sampler) )

    VkDescriptorSetLayoutBinding bindings[2]{};
    for (uint32_t b = 0; b < 2; ++b)
    {
        bindings[b].binding         = b;
        bindings[b].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[b].descriptorCount = 1;
        bindings[b].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = 2;
    layoutCI.pBindings    = bindings;
    VK_RESULT_CHECK( vkCreateDescriptorSetLayout(device, &layoutCI, S_VulkanAllocator(), &m_setLayout) )

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 };
    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.maxSets       = 1;
    poolCI.poolSizeCount = 1;
    poolCI.pPoolSizes    = &poolSize;
    VK_RESULT_CHECK( vkCreateDescriptorPool(device, &poolCI, S_VulkanAllocator(), &m_descPool) )

    VkDescriptorSetAllocateInfo setAI{};
    setAI.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAI.descriptorPool     = m_descPool;
    setAI.descriptorSetCount = 1;
    setAI.pSetLayouts        = &m_setLayout;
    VK_RESULT_CHECK( vkAllocateDescriptorSets(device, &setAI, &m_set) )

    VkDescriptorImageInfo imageInfos[2]{};
    imageInfos[0] = { m_sampler, m_fontView,   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    imageInfos[1] = { m_sampler, m_spriteView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkWriteDescriptorSet writes[2]{};
    for (uint32_t b = 0; b < 2; ++b)
    {
        writes[b].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[b].dstSet          = m_set;
        writes[b].dstBinding      = b;
        writes[b].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[b].descriptorCount = 1;
        writes[b].pImageInfo      = &imageInfos[b];
    }
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);

    VkPushConstantRange pcRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2 };
    VkPipelineLayoutCreateInfo plCI{};
    plCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCI.setLayoutCount         = 1;
    plCI.pSetLayouts            = &m_setLayout;
    plCI.pushConstantRangeCount = 1;
    plCI.pPushConstantRanges    = &pcRange;
    VK_RESULT_CHECK( vkCreatePipelineLayout(device, &plCI, S_VulkanAllocator(), &m_pipelineLayout) )

    auto loadModule = [&](const char* path) -> VkShaderModule
    {
        auto code = S_Application::executingApplication()->pack()->load(path);
        if (code.empty()) { s_debugLayer("S_UI: shader missing", path); return VK_NULL_HANDLE; }
        VkShaderModuleCreateInfo smCI{};
        smCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smCI.codeSize = code.size();
        smCI.pCode    = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule module = VK_NULL_HANDLE;
        VK_RESULT_CHECK( vkCreateShaderModule(device, &smCI, S_VulkanAllocator(), &module) )
        return module;
    };
    m_vertModule = loadModule("shaders/ui.vertc");
    m_fragModule = loadModule("shaders/ui.fragc");

    VkBufferCreateInfo bufCI{};
    bufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    VmaAllocationCreateInfo bufAllocCI{};
    bufAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
    bufAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    for (uint32_t i = 0; i < kMaxSlots; ++i)
    {
        VmaAllocationInfo info;
        bufCI.size  = kMaxVertices * sizeof(Vertex);
        bufCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &bufAllocCI, &m_vertexBuffers[i], &m_vertexAllocs[i], &info) )
        m_vertexMapped[i] = info.pMappedData;

        bufCI.size  = kMaxIndices * sizeof(uint32_t);
        bufCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VK_RESULT_CHECK( vmaCreateBuffer(m_api->vmaAllocator(), &bufCI, &bufAllocCI, &m_indexBuffers[i], &m_indexAllocs[i], &info) )
        m_indexMapped[i] = info.pMappedData;
    }
}

void S_UI::createPipeline()
{
    VkDevice device = m_api->device();
    if (!m_vertModule || !m_fragModule)
        return;

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = m_vertModule;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = m_fragModule;
    stages[1].pName  = "main";

    VkVertexInputBindingDescription binding{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    VkVertexInputAttributeDescription attrs[4]{};
    attrs[0] = { 0, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, x) };
    attrs[1] = { 1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, u) };
    attrs[2] = { 2, 0, VK_FORMAT_R8G8B8A8_UNORM,   offsetof(Vertex, color) };
    attrs[3] = { 3, 0, VK_FORMAT_R32_SFLOAT,       offsetof(Vertex, mode) };

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = 1;
    vertexInput.pVertexBindingDescriptions      = &binding;
    vertexInput.vertexAttributeDescriptionCount = 4;
    vertexInput.pVertexAttributeDescriptions    = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode    = VK_CULL_MODE_NONE;
    raster.frontFace   = VK_FRONT_FACE_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO; // test/write off: UI is on top

    VkPipelineColorBlendAttachmentState blend{};
    blend.blendEnable         = VK_TRUE;
    blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend.colorBlendOp        = VK_BLEND_OP_ADD;
    blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alphaBlendOp        = VK_BLEND_OP_ADD;
    blend.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendState{};
    blendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState.attachmentCount = 1;
    blendState.pAttachments    = &blend;

    VkDynamicState dynamics[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = dynamics;

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount          = 2;
    pipelineCI.pStages             = stages;
    pipelineCI.pVertexInputState   = &vertexInput;
    pipelineCI.pInputAssemblyState = &inputAssembly;
    pipelineCI.pViewportState      = &viewportState;
    pipelineCI.pRasterizationState = &raster;
    pipelineCI.pMultisampleState   = &msaa;
    pipelineCI.pDepthStencilState  = &depth;
    pipelineCI.pColorBlendState    = &blendState;
    pipelineCI.pDynamicState       = &dynamicState;
    pipelineCI.layout              = m_pipelineLayout;
    pipelineCI.renderPass          = m_api->renderPass();
    pipelineCI.subpass             = 0;
    VK_RESULT_CHECK( vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCI, S_VulkanAllocator(), &m_pipeline) )
}

void S_UI::onSwapchainRecreated()
{
    if (m_pipeline)
    {
        vkDestroyPipeline(m_api->device(), m_pipeline, S_VulkanAllocator());
        m_pipeline = VK_NULL_HANDLE;
    }
    createPipeline();
}

void S_UI::beginFrame(uint32_t width, uint32_t height)
{
    m_screenW = width;
    m_screenH = height;
    m_vertices.clear();
    m_indices.clear();
    m_prevHitRects.swap(m_hitRects);
    m_hitRects.clear();
    m_pointerHeldOnWidget = false; // widgets re-assert this while they hold the pointer

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrame).count();
    m_lastFrame = now;

    // ease press animation toward target (snappy in, softer out)
    for (auto& kv : m_buttons)
    {
        ButtonState& b = kv.second;
        float target = b.held ? 1.0f : 0.0f;
        float speed  = b.held ? 18.0f : 8.0f;
        b.press += (target - b.press) * glm::min(speed * dt, 1.0f);
    }
}

void S_UI::record(VkCommandBuffer cmd, uint32_t frameSlot)
{
    // consume edge events even if nothing drawn
    m_pointerClicked  = false;
    m_pointerReleased = false;

    if (m_vertices.empty() || !m_pipeline || frameSlot >= kMaxSlots)
        return;

    const uint32_t vtxCount = glm::min(static_cast<uint32_t>(m_vertices.size()), kMaxVertices);
    const uint32_t idxCount = glm::min(static_cast<uint32_t>(m_indices.size()),  kMaxIndices);
    memcpy(m_vertexMapped[frameSlot], m_vertices.data(), vtxCount * sizeof(Vertex));
    memcpy(m_indexMapped[frameSlot],  m_indices.data(),  idxCount * sizeof(uint32_t));
    vmaFlushAllocation(m_api->vmaAllocator(), m_vertexAllocs[frameSlot], 0, vtxCount * sizeof(Vertex));
    vmaFlushAllocation(m_api->vmaAllocator(), m_indexAllocs[frameSlot],  0, idxCount * sizeof(uint32_t));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_set, 0, nullptr);

    VkViewport viewport{ 0.0f, 0.0f, static_cast<float>(m_screenW), static_cast<float>(m_screenH), 0.0f, 1.0f };
    VkRect2D   scissor{ { 0, 0 }, { m_screenW, m_screenH } };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    float screenSize[2] = { static_cast<float>(m_screenW), static_cast<float>(m_screenH) };
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(screenSize), screenSize);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffers[frameSlot], &offset);
    vkCmdBindIndexBuffer(cmd, m_indexBuffers[frameSlot], 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, idxCount, 1, 0, 0, 0);
}

void S_UI::onMouseEvent(const S_MouseEvent* event)
{
    m_pointerX = static_cast<float>(event->x());
    m_pointerY = static_cast<float>(event->y());
    if (event->button() == S_MouseButton::Left)
    {
        if (event->state() == S_MouseEventState::Down) { m_pointerDown = true;  m_pointerClicked  = true; }
        if (event->state() == S_MouseEventState::Up)   { m_pointerDown = false; m_pointerReleased = true; }
    }
}

void S_UI::onTouchEvent(const S_TouchEvent* event)
{
    // first touch point acts as the pointer — same code path as mouse
    auto* self = const_cast<S_TouchEvent*>(event);
    if (event->activeCount() > 0)
    {
        const S_TouchPoint& p = self->pointByIndex(0);
        m_pointerX = p.x();
        m_pointerY = p.y();
        if (p.state() == S_TouchEventState::Down) { m_pointerDown = true;  m_pointerClicked  = true; }
        if (p.state() == S_TouchEventState::Up)   { m_pointerDown = false; m_pointerReleased = true; }
    }
}

glm::vec4 S_UI::anchoredRect(float anchorX, float anchorY, float offsetX, float offsetY,
                             float width, float height) const
{
    float cx = anchorX * m_screenW + offsetX;
    float cy = anchorY * m_screenH + offsetY;
    return { cx - width * 0.5f, cy - height * 0.5f, width, height };
}

void S_UI::quad(float x, float y, float w, float h, float u0, float v0, float u1, float v1,
                uint32_t color, float mode)
{
    uint32_t base = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back({ x,     y,     u0, v0, color, mode });
    m_vertices.push_back({ x + w, y,     u1, v0, color, mode });
    m_vertices.push_back({ x + w, y + h, u1, v1, color, mode });
    m_vertices.push_back({ x,     y + h, u0, v1, color, mode });
    m_indices.push_back(base);     m_indices.push_back(base + 1); m_indices.push_back(base + 2);
    m_indices.push_back(base);     m_indices.push_back(base + 2); m_indices.push_back(base + 3);
}

const UIBinSprite* S_UI::findSprite(const char* name) const
{
    for (const auto& s : m_sprites)
        if (strncmp(s.name, name, sizeof(s.name)) == 0)
            return &s;
    return nullptr;
}

void S_UI::sprite9(const char* name, float x, float y, float w, float h, uint32_t tint, float borderScale)
{
    const UIBinSprite* s = findSprite(name);
    if (!s) return;

    // dest border sizes, clamped so opposite borders never overlap
    float bl = glm::min(s->borderL * borderScale, w * 0.5f);
    float br = glm::min(s->borderR * borderScale, w * 0.5f);
    float bt = glm::min(s->borderT * borderScale, h * 0.5f);
    float bb = glm::min(s->borderB * borderScale, h * 0.5f);

    const float uw = s->u1 - s->u0, vh = s->v1 - s->v0;
    float ul = s->borderL / s->pixelWidth  * uw;
    float ur = s->borderR / s->pixelWidth  * uw;
    float vt = s->borderT / s->pixelHeight * vh;
    float vb = s->borderB / s->pixelHeight * vh;

    const float dx[4] = { x, x + bl, x + w - br, x + w };
    const float dy[4] = { y, y + bt, y + h - bb, y + h };
    const float du[4] = { s->u0, s->u0 + ul, s->u1 - ur, s->u1 };
    const float dv[4] = { s->v0, s->v0 + vt, s->v1 - vb, s->v1 };

    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
        {
            float qw = dx[col + 1] - dx[col];
            float qh = dy[row + 1] - dy[row];
            if (qw <= 0.0f || qh <= 0.0f) continue;
            quad(dx[col], dy[row], qw, qh, du[col], dv[row], du[col + 1], dv[row + 1], tint, 0.0f);
        }
}

float S_UI::textWidth(const std::string& str, float pixelHeight) const
{
    const float scale = pixelHeight / m_info.fontPixelHeight;
    float w = 0.0f;
    for (char c : str)
    {
        auto it = m_glyphOfCodepoint.find(static_cast<uint32_t>(static_cast<unsigned char>(c)));
        if (it != m_glyphOfCodepoint.end())
            w += m_glyphs[it->second].xadvance * scale;
    }
    return w;
}

void S_UI::text(const std::string& str, float x, float y, float pixelHeight, uint32_t color, bool centerX)
{
    const float scale  = pixelHeight / m_info.fontPixelHeight;
    float cursorX = centerX ? x - textWidth(str, pixelHeight) * 0.5f : x;
    const float baseline = y + m_info.ascent * scale;

    for (char c : str)
    {
        auto it = m_glyphOfCodepoint.find(static_cast<uint32_t>(static_cast<unsigned char>(c)));
        if (it == m_glyphOfCodepoint.end()) continue;
        const UIBinGlyph& g = m_glyphs[it->second];
        if (g.width > 0.0f && g.height > 0.0f)
            quad(cursorX + g.xoff * scale, baseline + g.yoff * scale,
                 g.width * scale, g.height * scale,
                 g.u0, g.v0, g.u1, g.v1, color, 1.0f);
        cursorX += g.xadvance * scale;
    }
}

void S_UI::registerHitRect(float x, float y, float w, float h)
{
    m_hitRects.push_back({ x, y, w, h });
}

bool S_UI::wantCaptureMouse() const
{
    if (m_pointerHeldOnWidget)
        return true;
    for (const glm::vec4& r : m_prevHitRects)
        if (m_pointerX >= r.x && m_pointerX <= r.x + r.z &&
            m_pointerY >= r.y && m_pointerY <= r.y + r.w)
            return true;
    return false;
}

void S_UI::panel(float x, float y, float w, float h, uint32_t tint)
{
    sprite9("panel", x, y, w, h, tint);
    registerHitRect(x, y, w, h);
}

bool S_UI::button(const char* id, const std::string& label, float x, float y, float w, float h)
{
    ButtonState& state = m_buttons[id];
    registerHitRect(x, y, w, h);

    const bool inside = m_pointerX >= x && m_pointerX <= x + w &&
                        m_pointerY >= y && m_pointerY <= y + h;
    state.hovered = inside;

    bool clicked = false;
    if (inside && m_pointerClicked)
        state.held = true;
    if (state.held && m_pointerReleased)
    {
        clicked    = inside;
        state.held = false;
    }
    if (!m_pointerDown)
        state.held = false;
    if (state.held)
        m_pointerHeldOnWidget = true;

    // pressed scale + hover tint
    const float scale = 1.0f - 0.07f * state.press;
    const float sw = w * scale, sh = h * scale;
    const float sx = x + (w - sw) * 0.5f, sy = y + (h - sh) * 0.5f;

    uint32_t tint = state.hovered ? rgba(255, 255, 255) : rgba(225, 228, 235);
    sprite9("button", sx, sy, sw, sh, tint);
    const float labelPx = 22.0f * m_scale * scale;
    text(label, sx + sw * 0.5f, sy + (sh - labelPx) * 0.5f - 4.0f * m_scale, labelPx,
         rgba(255, 255, 255), true);

    return clicked;
}

glm::vec2 S_UI::joystick(const char* id, float centerX, float centerY, float radius)
{
    JoystickState& state = m_joysticks[id];

    const float baseX = centerX - radius, baseY = centerY - radius, baseSize = radius * 2.0f;
    registerHitRect(baseX, baseY, baseSize, baseSize);

    const float dx = m_pointerX - centerX, dy = m_pointerY - centerY;
    const bool insideBase = dx * dx + dy * dy <= radius * radius;

    if (m_pointerClicked && insideBase)
        state.active = true;
    if (!m_pointerDown)
        state.active = false;

    if (state.active)
    {
        m_pointerHeldOnWidget = true;
        glm::vec2 v(dx / radius, dy / radius);
        float len = glm::length(v);
        state.value = (len > 1.0f) ? v / len : v;
    }
    else
        state.value = { 0.0f, 0.0f };

    sprite9("knob", baseX, baseY, baseSize, baseSize, rgba(40, 44, 58, 170));

    const float knobR = radius * 0.42f;
    const float kx = centerX + state.value.x * (radius - knobR);
    const float ky = centerY + state.value.y * (radius - knobR);
    sprite9("knob", kx - knobR, ky - knobR, knobR * 2.0f, knobR * 2.0f,
            state.active ? rgba(140, 180, 255, 235) : rgba(110, 130, 180, 200));

    return state.value;
}
