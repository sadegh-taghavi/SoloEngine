#include "S_ImGuiLayer.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include "solo/renderer/vulkan/S_VulkanAllocator.h"
#include "solo/platforms/S_InputEvent.h"
#include "solo/platforms/S_BaseApplication.h"
#include "solo/debug/S_Debug.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <cstring>

using namespace solo;

S_ImGuiLayer* S_ImGuiLayer::s_instance = nullptr;

S_ImGuiLayer::S_ImGuiLayer(S_VulkanRendererAPI* api)
    : m_api(api), m_lastFrame(std::chrono::steady_clock::now())
{
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
    };
    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCI.maxSets       = 64;
    poolCI.poolSizeCount = 1;
    poolCI.pPoolSizes    = poolSizes;
    VK_RESULT_CHECK( vkCreateDescriptorPool(m_api->device(), &poolCI, S_VulkanAllocator(), &m_pool) )

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.IniFilename = nullptr; // no imgui.ini clutter next to the exe
    ImGui::StyleColorsDark();

    ImGui_ImplVulkan_InitInfo info{};
    info.Instance        = m_api->instance();
    info.PhysicalDevice  = m_api->physicalDevice();
    info.Device          = m_api->device();
    info.QueueFamily     = m_api->graphicsQueueFamilyIndex();
    info.Queue           = m_api->graphicsQueue();
    info.DescriptorPool  = m_pool;
    info.RenderPass      = m_api->renderPass();
    info.MinImageCount   = 2;
    info.ImageCount      = m_api->swapchainImageCount();
    info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    m_initialized = ImGui_ImplVulkan_Init(&info);
    if (!m_initialized)
        s_debugLayer("S_ImGuiLayer: ImGui_ImplVulkan_Init failed");

    s_instance = this;
}

S_ImGuiLayer::~S_ImGuiLayer()
{
    s_instance = nullptr;
    if (m_initialized)
        ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(m_api->device(), m_pool, S_VulkanAllocator());
}

void S_ImGuiLayer::newFrame(uint32_t width, uint32_t height)
{
    if (!m_initialized) return;

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrame).count();
    m_lastFrame = now;

    ImGuiIO& io    = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.DeltaTime   = dt > 0.0f ? dt : 1.0f / 60.0f;
    if (auto* app = S_BaseApplication::executingApplication())
        io.FontGlobalScale = app->window()->scaleFactor();

    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void S_ImGuiLayer::render(VkCommandBuffer cmd)
{
    if (!m_initialized) return;
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void S_ImGuiLayer::onMouseEvent(const S_MouseEvent* event)
{
    if (!m_initialized) return;
    ImGuiIO& io = ImGui::GetIO();

    io.AddMousePosEvent(static_cast<float>(event->x()), static_cast<float>(event->y()));

    if (event->state() == S_MouseEventState::Wheel)
    {
        io.AddMouseWheelEvent(0.0f, static_cast<float>(event->z()) / 120.0f);
        return;
    }

    if (event->state() == S_MouseEventState::Down || event->state() == S_MouseEventState::Up)
    {
        int button = -1;
        switch (event->button())
        {
        case S_MouseButton::Left:   button = 0; break;
        case S_MouseButton::Right:  button = 1; break;
        case S_MouseButton::Middle: button = 2; break;
        default: break;
        }
        if (button >= 0)
            io.AddMouseButtonEvent(button, event->state() == S_MouseEventState::Down);
    }
}

// S_Key uses HID usage codes
static ImGuiKey hidToImGuiKey(uint32_t hid)
{
    if (hid >= 4 && hid <= 29)   return static_cast<ImGuiKey>(ImGuiKey_A + (hid - 4));
    if (hid >= 30 && hid <= 38)  return static_cast<ImGuiKey>(ImGuiKey_1 + (hid - 30));
    switch (hid)
    {
    case 39:  return ImGuiKey_0;
    case 40:  return ImGuiKey_Enter;
    case 41:  return ImGuiKey_Escape;
    case 42:  return ImGuiKey_Backspace;
    case 43:  return ImGuiKey_Tab;
    case 44:  return ImGuiKey_Space;
    case 73:  return ImGuiKey_Insert;
    case 74:  return ImGuiKey_Home;
    case 75:  return ImGuiKey_PageUp;
    case 76:  return ImGuiKey_Delete;
    case 77:  return ImGuiKey_End;
    case 78:  return ImGuiKey_PageDown;
    case 79:  return ImGuiKey_RightArrow;
    case 80:  return ImGuiKey_LeftArrow;
    case 81:  return ImGuiKey_DownArrow;
    case 82:  return ImGuiKey_UpArrow;
    case 224: return ImGuiKey_LeftCtrl;
    case 225: return ImGuiKey_LeftShift;
    case 226: return ImGuiKey_LeftAlt;
    case 228: return ImGuiKey_RightCtrl;
    case 229: return ImGuiKey_RightShift;
    case 230: return ImGuiKey_RightAlt;
    default:  return ImGuiKey_None;
    }
}

void S_ImGuiLayer::onKeyboardEvent(const S_KeyboardEvent* event)
{
    if (!m_initialized) return;
    ImGuiIO& io = ImGui::GetIO();

    const uint32_t hid  = static_cast<uint32_t>(event->key());
    const bool     down = event->state() == S_KeyboardEventState::Down;

    if (hid == 224 || hid == 228) io.AddKeyEvent(ImGuiMod_Ctrl, down);
    if (hid == 225 || hid == 229) io.AddKeyEvent(ImGuiMod_Shift, down);
    if (hid == 226 || hid == 230) io.AddKeyEvent(ImGuiMod_Alt, down);

    ImGuiKey key = hidToImGuiKey(hid);
    if (key != ImGuiKey_None)
        io.AddKeyEvent(key, down);
}

void S_ImGuiLayer::onCharacterEvent(const S_CharacterEvent* event)
{
    if (!m_initialized) return;
    char utf8[5] = {};
    memcpy(utf8, event->character(), 4);
    ImGui::GetIO().AddInputCharactersUTF8(utf8);
}

bool S_ImGuiLayer::wantCaptureMouse() const
{
    return m_initialized && ImGui::GetIO().WantCaptureMouse;
}

bool S_ImGuiLayer::wantCaptureKeyboard() const
{
    return m_initialized && ImGui::GetIO().WantCaptureKeyboard;
}
