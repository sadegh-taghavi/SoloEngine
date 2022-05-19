#pragma once
#include "solo/renderer/S_RendererAPI.h"
#include "S_VulkanPipeline.h"
#include "solo/stl/S_Vector.h"
#include "solo/debug/S_Debug.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace solo
{

class S_VulkanDeviceAllocator;
class S_VulkanItemsManager;

#ifdef SOLO_ENABLE_DEBUG_LAYER
static const char* ShowVkResult(VkResult err)
{
    switch (err) {
#define STR(r) case r: return #r
    STR(VK_SUCCESS);
    STR(VK_NOT_READY);
    STR(VK_TIMEOUT);
    STR(VK_EVENT_SET);
    STR(VK_EVENT_RESET);
    STR(VK_INCOMPLETE);

    STR(VK_ERROR_OUT_OF_HOST_MEMORY);
    STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    STR(VK_ERROR_INITIALIZATION_FAILED);
    STR(VK_ERROR_DEVICE_LOST);
    STR(VK_ERROR_MEMORY_MAP_FAILED);
    STR(VK_ERROR_LAYER_NOT_PRESENT);
    STR(VK_ERROR_EXTENSION_NOT_PRESENT);
    STR(VK_ERROR_FEATURE_NOT_PRESENT);
    STR(VK_ERROR_INCOMPATIBLE_DRIVER);
    STR(VK_ERROR_TOO_MANY_OBJECTS);
    STR(VK_ERROR_FORMAT_NOT_SUPPORTED);

    STR(VK_ERROR_SURFACE_LOST_KHR);
    STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
    STR(VK_SUBOPTIMAL_KHR);
    STR(VK_ERROR_OUT_OF_DATE_KHR);
    STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
    STR(VK_ERROR_VALIDATION_FAILED_EXT);
    STR(VK_ERROR_INVALID_SHADER_NV);
#undef STR
    default: return "UNKNOWN_RESULT";
    }
}

#define VK_RESULT_CHECK(VKFN) \
{ \
    VkResult VKRESULT = VKFN;\
    if( VKRESULT != VK_SUCCESS ) \
        s_debugLayer( ShowVkResult(VKRESULT), #VKFN, __FILE__, __LINE__); \
}

#else
#define VK_RESULT_CHECK(VKFN) { VKFN; }
#endif

class S_Window;
class S_BaseApplication;

class S_VulkanRendererAPI : public S_RendererAPI
{
public:
    S_VulkanRendererAPI();
    virtual ~S_VulkanRendererAPI();

    virtual void drawFrame();
    virtual void resize( uint32_t width, uint32_t height );
    virtual void active( bool active );
    virtual S_VertexBuffer *createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount,
                                               std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray,
                                               std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray);
    virtual S_Shader *createShader(const S_String &vertexShader, const S_String &fragmentShader,
                                   const S_String &geometryShader, const S_String &computeShader);

    virtual S_Texture *createTexture(const S_String &texture);

    virtual S_TextureSampler *createTextureSampler(const S_TextureSamplerDescriptor &descriptor);

    VkInstance instance() const;

    VkPhysicalDevice physicalDevice() const;

    VkDevice device() const;

    VkQueue transferQueue() const;

    VkQueue graphicsQueue() const;

    VkQueue computeQueue() const;

    VkQueue presentQueue() const;

    VkRenderPass renderPass() const;

    VkExtent2D swapChainExtent() const;

    VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties();

    VkPhysicalDeviceProperties *physicalDeviceProperties();


    S_VulkanDeviceAllocator *deviceAllocator();

    S_VulkanItemsManager *itemsManager();

    static uint32_t maxFramesInFlight();

    uint32_t currentFrame() const;

    uint32_t nextSwapchainImageIndex() const;

    VkCommandBuffer nextFrameRenderCommandBuffer() const;

    S_VulkanPipeline *pipelines() const;

    VkCommandBuffer beginSingleTimeTransferCommands();

    void endSingleTimeTransferCommands(VkCommandBuffer commandBuffer);
    void setImageLayout( VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout,
                         VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);

private:
    static const uint32_t m_MAX_FRAMES_IN_FLIGHT = 3;
    struct QueueIndices
    {
        QueueIndices()
        {
            IndexTransfer = -1;
            IndexGraphics = -1;
            IndexCompute = -1;
            IndexPresent = -1;
        }
        int32_t IndexTransfer;
        int32_t IndexGraphics;
        int32_t IndexCompute;
        int32_t IndexPresent;
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        S_Vector<VkSurfaceFormatKHR> Formats;
        S_Vector<VkPresentModeKHR> PresentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    void createInstance();
    void createDebugCallback();
    void createWindowSurface();
    void destroyWindowSurface();
    QueueIndices findQueueFamilies(VkPhysicalDevice device);
    void createPhysicalDevice();
    void createDevice();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void cleanupSwapChain();
    void recreateSwapChain();
    VkFormat findSupportedFormat(const S_Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);


    VkInstance m_instance;
#ifdef SOLO_ENABLE_DEBUG_LAYER
    VkDebugReportCallbackEXT m_debugReportCallback;
#endif
    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
    VkDevice m_device;
    VkSwapchainKHR m_swapChain;
    S_Vector<VkImage> m_swapChainImages;
    S_Vector<VkImageView> m_swapChainImageViews;
    VkImage m_depthImage;
    VkImageView m_depthImageView;
    VkQueue m_transferQueue;
    VkQueue m_graphicsQueue;
    VkQueue m_computeQueue;
    VkQueue m_presentQueue;
    VkSurfaceKHR m_windowSurface;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    SwapChainSupportDetails m_swapChainSupportDetails;
    VkRenderPass m_renderPass;
    S_Vector<VkFramebuffer> m_swapChainFramebuffers;
    std::unique_ptr<S_VulkanPipeline> m_pipelines;
    VkCommandPool m_commandPoolGraphics;
    VkCommandPool m_commandPoolTransfers;
    VkCommandBuffer m_nextFrameRenderCommandBuffer;
    VkCommandBuffer m_commandBufferVertexTransfer;
    S_Vector<VkCommandBuffer> m_commandBuffersSwapChain;
    S_Vector<VkSemaphore> m_imageAvailableSemaphores;
    S_Vector<VkSemaphore> m_renderFinishedSemaphores;
    S_Vector<VkFence> m_inFlightFences;
    S_Vector<VkFence> m_imagesInFlight;
    bool m_framebufferResized;
    bool m_active;
    uint32_t m_currentFrame;
    uint32_t m_nextSwapchainImageIndex;
    std::unique_ptr<S_VulkanDeviceAllocator> m_deviceAllocator;
    std::unique_ptr<S_VulkanItemsManager> m_itemsManager;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    class S_VertexBuffer *vVB;
    class S_Shader *vShader;
    class S_Texture *vTexture;
    std::shared_ptr<class S_CameraPerspective> vCam;
    std::shared_ptr<class S_FirstPersonCameraController> vCamController;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const S_Vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};

}

