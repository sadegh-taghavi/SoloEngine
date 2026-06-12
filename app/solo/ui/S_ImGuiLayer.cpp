#include "S_ImGuiLayer.h"
#include "solo/renderer/vulkan/S_VulkanRendererAPI.h"
#include "solo/renderer/vulkan/S_VulkanAllocator.h"
#include "solo/platforms/S_InputEvent.h"
#include "solo/debug/S_Debug.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

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

bool S_ImGuiLayer::wantCaptureMouse() const
{
    return m_initialized && ImGui::GetIO().WantCaptureMouse;
}
