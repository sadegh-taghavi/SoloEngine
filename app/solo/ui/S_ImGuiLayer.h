#pragma once
#include <vulkan/vulkan.h>
#include <chrono>

namespace solo
{

class S_VulkanRendererAPI;
class S_MouseEvent;
class S_KeyboardEvent;

// Engine-integrated Dear ImGui layer for tools/debug UI. Rendering goes through
// the engine's render pass; input is fed from the engine event system (not the
// ImGui Win32 backend) so X11/Android backends work through the same path.
class S_ImGuiLayer
{
public:
    explicit S_ImGuiLayer(S_VulkanRendererAPI* api);
    ~S_ImGuiLayer();

    void newFrame(uint32_t width, uint32_t height);
    void render(VkCommandBuffer cmd);

    void onMouseEvent(const S_MouseEvent* event);

    bool wantCaptureMouse() const;

    static S_ImGuiLayer* instance() { return s_instance; }

private:
    static S_ImGuiLayer* s_instance;

    S_VulkanRendererAPI* m_api;
    VkDescriptorPool     m_pool = VK_NULL_HANDLE;
    bool                 m_initialized = false;
    std::chrono::steady_clock::time_point m_lastFrame;
};

}
