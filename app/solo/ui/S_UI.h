#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include "solo/math/S_Math.h"
#include "solo/ui/S_UIBin.h"

namespace solo
{

class S_VulkanRendererAPI;
class S_MouseEvent;
class S_TouchEvent;

// Native screen-space UI: SDF text, 9-slice sprites, anchored immediate-mode
// widgets with eased press animations. Assets come from a baked .ui.bin.
// Self-contained Vulkan path (own pipeline/descriptors), draws inside the
// engine render pass after the 3D callback.
class S_UI
{
public:
    S_UI(S_VulkanRendererAPI* api, const std::string& uiBinPath);
    ~S_UI();

    static S_UI* instance() { return s_instance; }

    // engine hooks
    void beginFrame(uint32_t width, uint32_t height);
    void record(VkCommandBuffer cmd, uint32_t frameSlot);
    void onSwapchainRecreated();

    // input (engine forwards; touch maps to the same pointer)
    void onMouseEvent(const S_MouseEvent* event);
    void onTouchEvent(const S_TouchEvent* event);

    // anchored rect: position = anchor * screen + offset, size in pixels
    glm::vec4 anchoredRect(float anchorX, float anchorY, float offsetX, float offsetY,
                           float width, float height) const; // returns {x, y, w, h}, rect centered on anchor point + offset

    // drawing (call between engine beginFrame/record, i.e. from the render callback)
    void  sprite9(const char* name, float x, float y, float w, float h,
                  uint32_t tint = 0xFFFFFFFF, float borderScale = 1.0f);
    void  text(const std::string& str, float x, float y, float pixelHeight,
               uint32_t color = 0xFFFFFFFF, bool centerX = false);
    float textWidth(const std::string& str, float pixelHeight) const;

    // widgets
    void panel(float x, float y, float w, float h, uint32_t tint = 0xFFFFFFFF);
    bool button(const char* id, const std::string& label, float x, float y, float w, float h);

    static uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        return static_cast<uint32_t>(r) | (static_cast<uint32_t>(g) << 8)
             | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(a) << 24);
    }

private:
#pragma pack(push, 1)
    struct Vertex { float x, y, u, v; uint32_t color; float mode; };
#pragma pack(pop)

    struct ButtonState
    {
        bool  hovered = false;
        bool  held    = false;
        float press   = 0.0f; // 0 = idle, 1 = fully pressed (eased)
    };

    void quad(float x, float y, float w, float h, float u0, float v0, float u1, float v1,
              uint32_t color, float mode);
    const UIBinSprite* findSprite(const char* name) const;

    void createDeviceObjects();
    void createPipeline();
    void uploadAtlas(const uint8_t* pixels, uint32_t w, uint32_t h, VkFormat format,
                     VkImage& image, VmaAllocation& alloc, VkImageView& view);

    static S_UI* s_instance;

    S_VulkanRendererAPI* m_api;

    // baked assets
    UIBinHeader                            m_info{};
    std::vector<UIBinGlyph>                m_glyphs;
    std::vector<UIBinSprite>               m_sprites;
    std::unordered_map<uint32_t, uint32_t> m_glyphOfCodepoint;

    // frame state
    uint32_t            m_screenW = 0, m_screenH = 0;
    std::vector<Vertex>   m_vertices;
    std::vector<uint32_t> m_indices;

    // input state
    float m_pointerX = -1.0f, m_pointerY = -1.0f;
    bool  m_pointerDown = false;
    bool  m_pointerClicked = false; // down edge this frame
    bool  m_pointerReleased = false;

    std::unordered_map<std::string, ButtonState> m_buttons;
    std::chrono::steady_clock::time_point        m_lastFrame;

    // vulkan
    static constexpr uint32_t kMaxVertices = 16384;
    static constexpr uint32_t kMaxIndices  = 24576;
    static constexpr uint32_t kMaxSlots    = 4;

    VkImage               m_fontImage   = VK_NULL_HANDLE;
    VmaAllocation         m_fontAlloc   = VK_NULL_HANDLE;
    VkImageView           m_fontView    = VK_NULL_HANDLE;
    VkImage               m_spriteImage = VK_NULL_HANDLE;
    VmaAllocation         m_spriteAlloc = VK_NULL_HANDLE;
    VkImageView           m_spriteView  = VK_NULL_HANDLE;
    VkSampler             m_sampler     = VK_NULL_HANDLE;
    VkDescriptorPool      m_descPool    = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout   = VK_NULL_HANDLE;
    VkDescriptorSet       m_set         = VK_NULL_HANDLE;
    VkPipelineLayout      m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline            m_pipeline       = VK_NULL_HANDLE;
    VkShaderModule        m_vertModule  = VK_NULL_HANDLE;
    VkShaderModule        m_fragModule  = VK_NULL_HANDLE;

    VkBuffer      m_vertexBuffers[kMaxSlots] = {};
    VmaAllocation m_vertexAllocs[kMaxSlots]  = {};
    void*         m_vertexMapped[kMaxSlots]  = {};
    VkBuffer      m_indexBuffers[kMaxSlots]  = {};
    VmaAllocation m_indexAllocs[kMaxSlots]   = {};
    void*         m_indexMapped[kMaxSlots]   = {};
};

}
