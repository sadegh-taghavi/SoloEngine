#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include "S_VulkanItemsManager.h"
#include "S_VulkanItemsRequest.h"
#include "S_VulkanVertexBuffer.h"
#include "S_VulkanShader.h"
#include "S_VulkanTexture.h"
#include "S_VulkanTextureSampler.h"
#include "S_VulkanMesh.h"
#include "S_VulkanShader.h"
#include "S_VulkanPerFrame.h"
#include "S_VulkanBindless.h"
#include "S_VulkanSkinning.h"
#include "solo/ui/S_ImGuiLayer.h"
#include "solo/ui/S_UI.h"
#include "solo/renderer/S_PerFrame.h"
#include "solo/renderer/S_RendererAPI.h"
#include "solo/platforms/S_Window.h"
#include "solo/platforms/S_SystemDetect.h"
#include "solo/debug/S_Debug.h"
#include <map>
#include <array>
#include "solo/application/S_Application.h"
#include "solo/thread/S_Thread.h"
#include "solo/utility/S_ElapsedTime.h"
#include "solo/math/S_Math.h"
#if defined(S_PLATFORM_WINDOWS)
#define NOMINMAX
#include "solo/platforms/S_WindowWin32.h"
#include <vulkan/vulkan_win32.h>
#elif defined(S_PLATFORM_LINUX)
#include "solo/platforms/S_WindowX11.h"
#include <vulkan/vulkan_xlib.h>
#elif defined(S_PLATFORM_ANDROID)
#include "solo/platforms/S_WindowAndroid.h"
#include <vulkan/vulkan_android.h>
#endif
#include <stdint.h>
#include <algorithm>
#include <set>
#include <random>



using namespace solo;

#if defined( SOLO_ENABLE_DEBUG_LAYER ) && defined (SOLO_ENABLE_VULKAN_VALIDATION_LAYER)

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportFn(VkDebugReportFlagsEXT msgFlags,
                                             VkDebugReportObjectTypeEXT objType,
                                             uint64_t /*srcObject*/, size_t /*location*/,
                                             int32_t msgCode, const char *pLayerPrefix,
                                             const char *pMsg, void */*pUserData*/)
{
    if(objType == VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT && msgCode <= 1) return false;
    switch(msgFlags){
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
        s_debugLayer( "VK_INFORMATION:", "(", pLayerPrefix , ")", pMsg );
        return false;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT:
        s_debugLayer( "VK_WARNING:", "(", pLayerPrefix , ")", pMsg );
        return false;
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
        s_debugLayer( "VK_PERFORMANCE_WARNING:", "(", pLayerPrefix , ")", pMsg );
        return false;
    case VK_DEBUG_REPORT_ERROR_BIT_EXT:
        s_debugLayer( "VK_ERROR:", "(", pLayerPrefix , ")", pMsg );
        return true;
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        s_debugLayer( "VK_DEBUG:", "(", pLayerPrefix , ")", pMsg );
        return false;
    default:
        return false;
    }
}
#endif

void S_VulkanRendererAPI::createInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Solo";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2; // ray query needs BDA (core 1.2)
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    S_VulkanItemsInstanceExtensionRequest instanceExtensions;
#if defined(S_PLATFORM_WINDOWS)
    instanceExtensions.addRequestItem( "VK_KHR_win32_surface" );
#elif defined(S_PLATFORM_LINUX)
    instanceExtensions.addRequestItem( "VK_KHR_xlib_surface" );
#elif defined(S_PLATFORM_ANDROID)
    instanceExtensions.addRequestItem( "VK_KHR_android_surface" );
#endif
    instanceExtensions.addRequestItem(VK_KHR_SURFACE_EXTENSION_NAME );
    instanceExtensions.addRequestItem( VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME );
    instanceExtensions.addRequestItem( VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME );
    instanceExtensions.addRequestItem( VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME );
    instanceExtensions.addRequestItem( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
    instanceExtensions.addRequestItem( VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME );
    instanceExtensions.addRequestItem( VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME );
#if defined( SOLO_ENABLE_DEBUG_LAYER ) && defined (SOLO_ENABLE_VULKAN_VALIDATION_LAYER)
    instanceExtensions.addRequestItem( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
#endif
    instanceExtensions.queryItems();

    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.enabledItems()->size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.enabledItems()->data();

#if defined( SOLO_ENABLE_DEBUG_LAYER ) && defined (SOLO_ENABLE_VULKAN_VALIDATION_LAYER)
    S_VulkanItemsInstanceLayersRequest instanceLayers;
    instanceLayers.addRequestItem( "VK_LAYER_GOOGLE_threading" );
    instanceLayers.addRequestItem( "VK_LAYER_LUNARG_parameter_validation" );
    instanceLayers.addRequestItem( "VK_LAYER_LUNARG_core_validation" );
    instanceLayers.addRequestItem( "VK_LAYER_LUNARG_object_tracker" );
    instanceLayers.addRequestItem( "VK_LAYER_GOOGLE_unique_objects" );
    instanceLayers.queryItems();

    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.enabledItems()->size());
    instanceCreateInfo.ppEnabledLayerNames = instanceLayers.enabledItems()->data();
#endif

    VK_RESULT_CHECK( vkCreateInstance( &instanceCreateInfo, S_VulkanAllocator(), &m_instance ) )
}

void S_VulkanRendererAPI::createDebugCallback()
{
#if defined( SOLO_ENABLE_DEBUG_LAYER ) && defined (SOLO_ENABLE_VULKAN_VALIDATION_LAYER)
    VkDebugReportCallbackCreateInfoEXT debugCreate_info = {};
    debugCreate_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debugCreate_info.pNext = nullptr;
    debugCreate_info.flags =
            VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
            VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_ERROR_BIT_EXT |
            VK_DEBUG_REPORT_DEBUG_BIT_EXT |
            0;
    debugCreate_info.pfnCallback = DebugReportFn;
    debugCreate_info.pUserData = nullptr;

    auto vkCreateDebugReportCallbackEXT  = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>( vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT" ) );

    VK_RESULT_CHECK( vkCreateDebugReportCallbackEXT( m_instance, &debugCreate_info, S_VulkanAllocator(), &m_debugReportCallback) )
        #endif
}

void S_VulkanRendererAPI::createWindowSurface()
{
#ifdef S_PLATFORM_WINDOWS
    const S_WindowWin32* windowWin32 = dynamic_cast<const S_WindowWin32*>(S_BaseApplication::executingApplication()->window());
    VkWin32SurfaceCreateInfoKHR createInfoSurface = {};
    createInfoSurface.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfoSurface.hwnd = windowWin32->hWnd();
    createInfoSurface.hinstance = windowWin32->hInstance();
    VK_RESULT_CHECK( vkCreateWin32SurfaceKHR( m_instance, &createInfoSurface, S_VulkanAllocator(), &m_windowSurface) )
#elif defined( S_PLATFORM_LINUX )
    const S_WindowX11* windowX11 = dynamic_cast<const S_WindowX11*>(S_BaseApplication::executingApplication()->window());
    VkXlibSurfaceCreateInfoKHR createInfoSurface = {};
    createInfoSurface.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfoSurface.dpy = windowX11->display();
    createInfoSurface.window = windowX11->window();
    VK_RESULT_CHECK( vkCreateXlibSurfaceKHR( m_instance, &createInfoSurface, S_VulkanAllocator(), &m_windowSurface) )
#elif defined( S_PLATFORM_ANDROID )
    const S_WindowAndroid* windowAndroid = dynamic_cast<const S_WindowAndroid*>(S_BaseApplication::executingApplication()->window());
    VkAndroidSurfaceCreateInfoKHR createInfoSurface = {};
    createInfoSurface.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfoSurface.window = windowAndroid->app()->window;
    VK_RESULT_CHECK( vkCreateAndroidSurfaceKHR( m_instance, &createInfoSurface, S_VulkanAllocator(), &m_windowSurface) )
#endif
}

void S_VulkanRendererAPI::destroyWindowSurface()
{
    vkDestroySurfaceKHR( m_instance, m_windowSurface, S_VulkanAllocator() );
}

S_VulkanRendererAPI::QueueIndices S_VulkanRendererAPI::findQueueFamilies(VkPhysicalDevice device)
{
    QueueIndices foundIndices;
    std::vector<VkQueueFamilyProperties> queueFamilies;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    queueFamilies.resize( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );
    int index = 0;
    std::vector<int32_t> graphicsIndices;
    std::vector<int32_t> computeIndices;
    std::vector<int32_t> transferIndices;
    std::vector<int32_t> presentIndices;
    graphicsIndices.insert( graphicsIndices.begin(), 4, -1 );
    computeIndices.insert( computeIndices.begin(), 4, -1 );
    transferIndices.insert( transferIndices.begin(), 4, -1 );
    presentIndices.insert( presentIndices.begin(), 4, -1 );

    for (const auto& queueFamily : queueFamilies)
    {
        if( queueFamily.queueCount > 0 )
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, static_cast<uint32_t>(index), m_windowSurface, &presentSupport);

            if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                if( !presentSupport && !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) )
                    graphicsIndices[0] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) )
                    graphicsIndices[1] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) )
                    graphicsIndices[2] = index;
                else
                    graphicsIndices[3] = index;
            }

            if( queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT )
            {
                if( !presentSupport && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)  )
                    computeIndices[0] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) )
                    computeIndices[1] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) )
                    computeIndices[2] = index;
                else
                    computeIndices[3] = index;
            }

            if( queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT )
            {
                if( !presentSupport && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) )
                    transferIndices[0] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) )
                    transferIndices[1] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) )
                    transferIndices[2] = index;
                else
                    transferIndices[3] = index;
            }

            if( presentSupport )
            {
                if( !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) )
                    presentIndices[0] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) )
                    presentIndices[1] = index;
                else if( !(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) )
                    presentIndices[2] = index;
                else
                    presentIndices[3] = index;
            }
        }
        ++index;
    }

    for( uint32_t i = 0; i < 4; ++i )
    {
        if( graphicsIndices.at( i ) != -1 && foundIndices.IndexGraphics == -1 )
            foundIndices.IndexGraphics = graphicsIndices.at( i );

        if( presentIndices.at( i ) != -1 && foundIndices.IndexPresent == -1 )
            foundIndices.IndexPresent = presentIndices.at( i );

        if( transferIndices.at( i ) != -1 && foundIndices.IndexTransfer == -1 )
            foundIndices.IndexTransfer = transferIndices.at( i );

        if( computeIndices.at( i ) != -1 && foundIndices.IndexCompute == -1 )
            foundIndices.IndexCompute = computeIndices.at( i );
    }
    s_debugLayer( "IndexGraphics =", foundIndices.IndexGraphics, " IndexPresent =", foundIndices.IndexPresent, " IndexTransfer =", foundIndices.IndexTransfer, " IndexCompute =", foundIndices.IndexCompute );

    return foundIndices;
}

void S_VulkanRendererAPI::createPhysicalDevice()
{
    uint32_t deviceCount = 0;
    VK_RESULT_CHECK( vkEnumeratePhysicalDevices( m_instance, &deviceCount, nullptr ) )
            std::vector<VkPhysicalDevice> devices;
    devices.resize( deviceCount );
    VK_RESULT_CHECK( vkEnumeratePhysicalDevices( m_instance, &deviceCount, devices.data() ) )
            m_physicalDevice = nullptr;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    std::multimap<int, VkPhysicalDevice> candidates;
    int score;
    QueueIndices queueIndices;

    for (const auto& device : devices)
    {
        score = 0;
        vkGetPhysicalDeviceProperties( device, &deviceProperties);
        vkGetPhysicalDeviceFeatures( device, &deviceFeatures );
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;
        score += deviceProperties.limits.maxImageDimension2D;
        if (!deviceFeatures.geometryShader)
            score = 0;
        else
        {
            queueIndices = findQueueFamilies( device );
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            if( queueIndices.IndexTransfer == -1 ||
                    queueIndices.IndexGraphics == -1 ||
                    queueIndices.IndexCompute == -1 ||
                    queueIndices.IndexPresent == -1 ||
                    swapChainSupport.Formats.empty()
                    || swapChainSupport.PresentModes.empty() )
                score = 0;
        }
        candidates.insert( std::make_pair(score, device ) );
    }

    for( const auto& candidate : candidates )
    {
        if (candidate.first > 0)
            m_physicalDevice = candidate.second;
    }
    s_debugLayer( m_physicalDevice == nullptr ? "Suitable device not found!" : "Device has been selected." );

    vkGetPhysicalDeviceMemoryProperties( m_physicalDevice, &m_physicalDeviceMemoryProperties );
    vkGetPhysicalDeviceProperties( m_physicalDevice, &m_physicalDeviceProperties );
}

void S_VulkanRendererAPI::createDevice()
{
    QueueIndices selectedQueueIndices = findQueueFamilies( m_physicalDevice );

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies =
    {
        static_cast<uint32_t>(selectedQueueIndices.IndexTransfer),
        static_cast<uint32_t>(selectedQueueIndices.IndexGraphics),
        static_cast<uint32_t>(selectedQueueIndices.IndexCompute),
        static_cast<uint32_t>(selectedQueueIndices.IndexPresent)
    };

    float queuePriority = 1.0f;
    for ( uint32_t queueFamily : uniqueQueueFamilies )
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back( queueCreateInfo );
    }

    VkPhysicalDeviceFeatures enableDeviceFeatures = {};

    // ray tracing feature chain (targets are RT-capable by project decision)
    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing  = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures = {};
    asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    asFeatures.accelerationStructure = VK_TRUE;
    asFeatures.pNext = &features12;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;
    rayQueryFeatures.pNext = &asFeatures;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &rayQueryFeatures;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &enableDeviceFeatures;

    S_VulkanItemsDeviceExtensionRequest deviceExtensions;
    deviceExtensions.addRequestItem(VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    deviceExtensions.addRequestItem(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME );
    deviceExtensions.addRequestItem(VK_KHR_RAY_QUERY_EXTENSION_NAME );
    deviceExtensions.addRequestItem(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME );
    deviceExtensions.queryItems( this );

    if( deviceExtensions.enabledItems()->size() < 4 )
        s_debugLayer( "WARNING: ray tracing extensions missing on this device — RT shadows will fail" );

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.enabledItems()->size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.enabledItems()->data();

#if defined( SOLO_ENABLE_DEBUG_LAYER ) && defined (SOLO_ENABLE_VULKAN_VALIDATION_LAYER)
    S_VulkanItemsDeviceLayersRequest deviceLayers;
    deviceLayers.addRequestItem( "VK_LAYER_GOOGLE_threading" );
    deviceLayers.addRequestItem( "VK_LAYER_LUNARG_parameter_validation" );
    deviceLayers.addRequestItem( "VK_LAYER_LUNARG_core_validation" );
    deviceLayers.addRequestItem( "VK_LAYER_LUNARG_object_tracker" );
    deviceLayers.addRequestItem( "VK_LAYER_GOOGLE_unique_objects" );
    deviceLayers.queryItems( this );

    deviceCreateInfo.enabledLayerCount= static_cast<uint32_t>(deviceLayers.enabledItems()->size());
    deviceCreateInfo.ppEnabledLayerNames = deviceLayers.enabledItems()->data();
#endif

    VK_RESULT_CHECK( vkCreateDevice(m_physicalDevice, &deviceCreateInfo, S_VulkanAllocator(), &m_device) )
            vkGetDeviceQueue(m_device, static_cast<uint32_t>( selectedQueueIndices.IndexGraphics ), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, static_cast<uint32_t>( selectedQueueIndices.IndexCompute ), 0, &m_computeQueue );
    vkGetDeviceQueue(m_device, static_cast<uint32_t>( selectedQueueIndices.IndexTransfer ), 0, &m_transferQueue );
    vkGetDeviceQueue(m_device, static_cast<uint32_t>( selectedQueueIndices.IndexPresent ), 0, &m_presentQueue );
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    VK_RESULT_CHECK( vmaCreateAllocator( &allocatorInfo, &m_vmaAllocator ) )

}

S_VulkanRendererAPI::SwapChainSupportDetails S_VulkanRendererAPI::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    VK_RESULT_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_windowSurface, &details.Capabilities) )

            uint32_t formatCount;
    VK_RESULT_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, nullptr) )
            details.Formats.resize( formatCount );
    VK_RESULT_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, details.Formats.data() ) )

            uint32_t presentModeCount;
    VK_RESULT_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &presentModeCount, nullptr) )
            details.PresentModes.resize( presentModeCount );
    VK_RESULT_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &presentModeCount, details.PresentModes.data() ) )

            return details;
}


VkSurfaceFormatKHR S_VulkanRendererAPI::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR S_VulkanRendererAPI::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D S_VulkanRendererAPI::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }else
    {
        VkExtent2D actualExtent = {S_BaseApplication::executingApplication()->window()->width(), S_BaseApplication::executingApplication()->window()->height()};
        actualExtent.width = glm::max(capabilities.minImageExtent.width, glm::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = glm::max(capabilities.minImageExtent.height, glm::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void S_VulkanRendererAPI::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport( m_physicalDevice );
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.Formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.PresentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
    if ( swapChainSupport.Capabilities.maxImageCount > 0 &&
         imageCount > swapChainSupport.Capabilities.maxImageCount )
    {
        imageCount = swapChainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    QueueIndices indices = findQueueFamilies( m_physicalDevice );
    uint32_t queueFamilyIndices[] =
    {
        static_cast<uint32_t>(indices.IndexGraphics),
        static_cast<uint32_t>(indices.IndexPresent)
    };

    if (indices.IndexGraphics != indices.IndexPresent )
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
#ifdef S_PLATFORM_ANDROID
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
#elif defined(S_PLATFORM_LINUX )
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
#elif defined( S_PLATFORM_WINDOWS )
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
#endif
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = nullptr;
    VK_RESULT_CHECK( vkCreateSwapchainKHR( m_device, &createInfo, S_VulkanAllocator(), &m_swapChain ) )
            VK_RESULT_CHECK( vkGetSwapchainImagesKHR( m_device, m_swapChain, &imageCount, nullptr ) )
            m_swapChainImages.resize( imageCount );
    VK_RESULT_CHECK( vkGetSwapchainImagesKHR( m_device, m_swapChain, &imageCount, m_swapChainImages.data()) )
            m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void S_VulkanRendererAPI::createImageViews()
{
    m_swapChainImageViews.resize( m_swapChainImages.size() );
    for (size_t i = 0; i < m_swapChainImages.size(); ++i)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        VK_RESULT_CHECK( vkCreateImageView( m_device, &createInfo, S_VulkanAllocator(), &m_swapChainImageViews[i]) )
    }
}

void S_VulkanRendererAPI::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    VkImageCreateInfo depthImageInfo = {};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.extent.width = m_swapChainExtent.width;
    depthImageInfo.extent.height = m_swapChainExtent.height;
    depthImageInfo.extent.depth = 1;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.format = depthFormat;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo depthAllocInfo = {};
    depthAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    VK_RESULT_CHECK( vmaCreateImage( m_vmaAllocator, &depthImageInfo, &depthAllocInfo, &m_depthImage, &m_depthImageAllocation, nullptr ) )

    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = m_depthImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = depthFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    VK_RESULT_CHECK( vkCreateImageView( m_device, &createInfo, S_VulkanAllocator(), &m_depthImageView) );
//    setImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, createInfo.subresourceRange);


}

void S_VulkanRendererAPI::createRenderPass()
{
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;

    VK_RESULT_CHECK( vkCreateRenderPass( m_device, &renderPassInfo, S_VulkanAllocator(), &m_renderPass) )

}

void S_VulkanRendererAPI::createFramebuffers()
{

    m_swapChainFramebuffers.resize( m_swapChainImageViews.size() );
    for ( size_t i = 0; i < m_swapChainImageViews.size(); ++i )
    {
        std::array<VkImageView, 2> attachments = { m_swapChainImageViews[i], m_depthImageView };
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        VK_RESULT_CHECK( vkCreateFramebuffer( m_device, &framebufferInfo, S_VulkanAllocator(), &m_swapChainFramebuffers[i] ) )
    }
}

void S_VulkanRendererAPI::createCommandPool()
{
    QueueIndices queueFamilyIndices = findQueueFamilies( m_physicalDevice );

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = static_cast<unsigned int>( queueFamilyIndices.IndexGraphics );
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_RESULT_CHECK( vkCreateCommandPool(m_device, &poolInfo, S_VulkanAllocator(), &m_commandPoolGraphics ) )

            poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = static_cast<unsigned int>( queueFamilyIndices.IndexTransfer );
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_RESULT_CHECK( vkCreateCommandPool(m_device, &poolInfo, S_VulkanAllocator(), &m_commandPoolTransfers ) )
}

void S_VulkanRendererAPI::createCommandBuffers()
{
    m_commandBuffersSwapChain.resize( m_swapChainFramebuffers.size() );
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPoolGraphics;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>( m_commandBuffersSwapChain.size() );

    VK_RESULT_CHECK( vkAllocateCommandBuffers( m_device, &allocInfo, m_commandBuffersSwapChain.data() ) )

            allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPoolTransfers;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_RESULT_CHECK( vkAllocateCommandBuffers( m_device, &allocInfo, &m_commandBufferVertexTransfer ) );

}

void S_VulkanRendererAPI::createSyncObjects()
{
    m_imageAvailableSemaphores.resize( m_MAX_FRAMES_IN_FLIGHT );
    m_renderFinishedSemaphores.resize( m_MAX_FRAMES_IN_FLIGHT );
    m_inFlightFences.resize( m_MAX_FRAMES_IN_FLIGHT );
    m_imagesInFlight.resize( m_swapChainImages.size(), nullptr );

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for ( size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++i )
    {
        VK_RESULT_CHECK( vkCreateSemaphore( m_device, &semaphoreInfo, S_VulkanAllocator(), &m_imageAvailableSemaphores[i] ) )
                VK_RESULT_CHECK( vkCreateSemaphore( m_device, &semaphoreInfo, S_VulkanAllocator(), &m_renderFinishedSemaphores[i] ) )
                VK_RESULT_CHECK( vkCreateFence( m_device, &fenceInfo, S_VulkanAllocator(), &m_inFlightFences[i]) )
    }
}

void S_VulkanRendererAPI::recreateSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport( m_physicalDevice );

    if( S_BaseApplication::executingApplication()->window()->width() == 0
            || S_BaseApplication::executingApplication()->window()->height() == 0 ||
            swapChainSupport.Capabilities.currentExtent.width == 0 ||
            swapChainSupport.Capabilities.currentExtent.height == 0 )
    {
        S_Thread::sleep( 100 );
        m_framebufferResized = true;
        return;
    }

    vkDeviceWaitIdle( m_device );

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    m_pipelines->recreate();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
    m_imagesInFlight.assign( m_swapChainImages.size(), nullptr );

    m_imguiLayer.reset(); // render pass changed — lazily re-initialized next frame
    if( S_UI::instance() )
        S_UI::instance()->onSwapchainRecreated();
}

VkFormat S_VulkanRendererAPI::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties( m_physicalDevice, format, &props );
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }
    return VK_FORMAT_UNDEFINED;
}

VkFormat S_VulkanRendererAPI::findDepthFormat()
{
    std::vector<VkFormat> vf;
    vf.push_back( VK_FORMAT_D32_SFLOAT );
    vf.push_back( VK_FORMAT_D32_SFLOAT_S8_UINT );
    vf.push_back( VK_FORMAT_D24_UNORM_S8_UINT );
    return findSupportedFormat( vf, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

bool S_VulkanRendererAPI::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

S_VulkanPipeline *S_VulkanRendererAPI::pipelines() const
{
    return m_pipelines.get();
}

VkCommandBuffer S_VulkanRendererAPI::beginSingleTimeTransferCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPoolTransfers;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VK_RESULT_CHECK( vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) );

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_RESULT_CHECK( vkBeginCommandBuffer(commandBuffer, &beginInfo) );

    return commandBuffer;
}

void S_VulkanRendererAPI::endSingleTimeTransferCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_RESULT_CHECK( vkQueueSubmit(m_transferQueue, 1, &submitInfo, nullptr) );
    VK_RESULT_CHECK( vkQueueWaitIdle(m_transferQueue) );

    vkFreeCommandBuffers(m_device, m_commandPoolTransfers, 1, &commandBuffer);
}

void S_VulkanRendererAPI::setImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier imageMemoryBarrier = { };

    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    switch (oldLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter).
        // Only valid as initial layout. No flags required.
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized.
        // Only valid as initial layout for linear images; preserves memory
        // contents. Make sure host writes have finished.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment.
        // Make sure writes to the color buffer have finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment.
        // Make sure any writes to the depth/stencil buffer have finished.
        imageMemoryBarrier.srcAccessMask
                = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source.
        // Make sure any reads from the image have finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination.
        // Make sure any writes to the image have finished.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader.
        // Make sure any shader reads from the image have finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    // Target layouts (new)
    // The destination access mask controls the dependency for the new image
    // layout.
    switch (newLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination.
        // Make sure any writes to the image have finished.
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source.
        // Make sure any reads from and writes to the image have finished.
        imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment.
        // Make sure any writes to the color buffer have finished.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment.
        // Make sure any writes to depth/stencil buffer have finished.
        imageMemoryBarrier.dstAccessMask
                = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment).
        // Make sure any writes to the image have finished.
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask
                    = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;

    }

    // Put barrier on top of pipeline.
    VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    // Add the barrier to the passed command buffer
    vkCmdPipelineBarrier( cmdBuffer, srcStageFlags, destStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}


VkCommandBuffer S_VulkanRendererAPI::nextFrameRenderCommandBuffer() const
{
    return m_nextFrameRenderCommandBuffer;
}

uint32_t S_VulkanRendererAPI::currentFrame() const
{
    return m_currentFrame;
}

uint32_t S_VulkanRendererAPI::nextSwapchainImageIndex() const
{
    return m_nextSwapchainImageIndex;
}

void S_VulkanRendererAPI::createGraphicsPipeline(const std::vector<S_PipelineDescriptor> &descriptors)
{
    m_pipelines->create(&descriptors);
}

VmaAllocator S_VulkanRendererAPI::vmaAllocator() const
{
    return m_vmaAllocator;
}

S_VulkanItemsManager *S_VulkanRendererAPI::itemsManager()
{
    return m_itemsManager.get();
}

uint32_t S_VulkanRendererAPI::maxFramesInFlight()
{
    return m_MAX_FRAMES_IN_FLIGHT;
}

VkPhysicalDeviceMemoryProperties *S_VulkanRendererAPI::physicalDeviceMemoryProperties()
{
    return &m_physicalDeviceMemoryProperties;
}

VkPhysicalDeviceProperties *S_VulkanRendererAPI::physicalDeviceProperties()
{
    return &m_physicalDeviceProperties;
}

VkExtent2D S_VulkanRendererAPI::swapChainExtent() const
{
    return m_swapChainExtent;
}

VkRenderPass S_VulkanRendererAPI::renderPass() const
{
    return m_renderPass;
}

S_VulkanRendererAPI::S_VulkanRendererAPI() : S_RendererAPI (), m_nextFrameRenderCommandBuffer(nullptr), m_framebufferResized(false), m_active(false), m_currentFrame(0)
{
    createInstance();
    createDebugCallback();
    createWindowSurface();
    createPhysicalDevice();
    s_debugLayer("Before Create Device");

    createDevice();
    s_debugLayer("After Create Device");
    createSwapChain();
    createImageViews();
    createRenderPass();
    m_pipelines = std::make_unique<S_VulkanPipeline>( this );
    m_itemsManager = std::make_unique<S_VulkanItemsManager>(this);
    m_perFrame     = std::make_unique<S_VulkanPerFrame>(this, m_MAX_FRAMES_IN_FLIGHT, sizeof(S_PerFrameData));
    m_rt           = std::make_unique<S_VulkanRT>(this, m_MAX_FRAMES_IN_FLIGHT);
    if( m_rt->available() )
        m_skinning = std::make_unique<S_VulkanSkinning>(this, m_MAX_FRAMES_IN_FLIGHT);
    m_bindless     = std::make_unique<S_VulkanBindless>(this, m_MAX_FRAMES_IN_FLIGHT);
    if( m_rt->available() )
    {
        VkAccelerationStructureKHR tlases[m_MAX_FRAMES_IN_FLIGHT];
        for( uint32_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++i )
            tlases[i] = m_rt->tlas(i);
        m_bindless->setTlas(tlases);
    }
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void S_VulkanRendererAPI::cleanupSwapChain()
{
    vkDestroyImageView(m_device, m_depthImageView, S_VulkanAllocator());
    vmaDestroyImage( m_vmaAllocator, m_depthImage, m_depthImageAllocation );

    for ( size_t i = 0; i < m_swapChainFramebuffers.size(); ++i )
        vkDestroyFramebuffer( m_device, m_swapChainFramebuffers[i], S_VulkanAllocator() );

    vkFreeCommandBuffers( m_device, m_commandPoolGraphics, static_cast<uint32_t>(m_commandBuffersSwapChain.size()), m_commandBuffersSwapChain.data() );

    vkFreeCommandBuffers( m_device, m_commandPoolTransfers, 1, &m_commandBufferVertexTransfer );
    m_pipelines->destroy();
    vkDestroyRenderPass( m_device, m_renderPass, S_VulkanAllocator() );

    for ( size_t i = 0; i < m_swapChainImageViews.size(); ++i )
        vkDestroyImageView( m_device, m_swapChainImageViews[i], S_VulkanAllocator() );

    vkDestroySwapchainKHR( m_device, m_swapChain, S_VulkanAllocator() );
}

S_VulkanRendererAPI::~S_VulkanRendererAPI()
{
    vkDeviceWaitIdle( m_device );
    m_imguiLayer.reset(); // must release its Vulkan objects before the device goes away
    m_itemsManager->destroy();
    cleanupSwapChain();
    for ( size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++i )
    {
        vkDestroySemaphore( m_device, m_renderFinishedSemaphores[i], S_VulkanAllocator() );
        vkDestroySemaphore( m_device, m_imageAvailableSemaphores[i], S_VulkanAllocator() );
        vkDestroyFence( m_device, m_inFlightFences[i], S_VulkanAllocator() );
    }
    vkDestroyCommandPool( m_device, m_commandPoolTransfers, S_VulkanAllocator() );
    vkDestroyCommandPool( m_device, m_commandPoolGraphics, S_VulkanAllocator() );
    destroyWindowSurface();
    m_bindless.reset();
    m_perFrame.reset();
    m_skinning.reset();
    m_rt.reset();
    vmaDestroyAllocator( m_vmaAllocator );
    vkDestroyDevice( m_device, S_VulkanAllocator() );
#if defined( SOLO_ENABLE_DEBUG_LAYER ) && defined (SOLO_ENABLE_VULKAN_VALIDATION_LAYER)
    auto vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>( vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT") );
    vkDestroyDebugReportCallbackEXT( m_instance, m_debugReportCallback, S_VulkanAllocator() );
#endif
    vkDestroyInstance(  m_instance, S_VulkanAllocator() );
}

void S_VulkanRendererAPI::drawFrame()
{
    if( !m_active )
        return;
    if( m_pipelines->pipelines()->empty() )
        return;
    if( !m_imguiLayer )
        m_imguiLayer = std::make_unique<S_ImGuiLayer>(this);
    static S_ElapsedTime et;
    //    s_debug( "ELAPSSSSSED:", et.restart() / 1000 );
//    S_Thread::sleep( 10 );
    vkWaitForFences( m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX );


    VkResult result;
    VK_RESULT_CHECK( result = vkAcquireNextImageKHR( m_device, m_swapChain, UINT64_MAX,
                                                     m_imageAvailableSemaphores[m_currentFrame], nullptr, &m_nextSwapchainImageIndex ) )
            if( result == VK_ERROR_SURFACE_LOST_KHR )
    {
        destroyWindowSurface();
        createWindowSurface();
        recreateSwapChain();
        return;
    }else if ( result == VK_ERROR_OUT_OF_DATE_KHR )
    {
        recreateSwapChain();
        return;
    }/*else if( result == VK_SUBOPTIMAL_KHR )
    {

    }*/

    if( m_imagesInFlight[m_nextSwapchainImageIndex] != nullptr )
        vkWaitForFences( m_device, 1, &m_imagesInFlight[m_nextSwapchainImageIndex], VK_TRUE, UINT64_MAX );
    m_imagesInFlight[m_nextSwapchainImageIndex] = m_inFlightFences[m_currentFrame];

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    m_nextFrameRenderCommandBuffer = m_commandBuffersSwapChain[m_nextSwapchainImageIndex];
    VK_RESULT_CHECK( vkBeginCommandBuffer( m_nextFrameRenderCommandBuffer, &beginInfo) );


    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[m_nextSwapchainImageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.5f, 0.5f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    m_imguiLayer->newFrame(m_swapChainExtent.width, m_swapChainExtent.height);
    if( S_UI::instance() )
        S_UI::instance()->beginFrame(m_swapChainExtent.width, m_swapChainExtent.height);

    // TLAS for this frame from last frame's draw list (one frame stale); must
    // be recorded outside the render pass. Indexed by swapchain image to match
    // the bindless set that exposes it (binding 2).
    if( m_rt->available() )
    {
        // skinned meshes: compute-skin positions, rebuild their BLASes in-cmd,
        // then they join the TLAS like any static instance
        std::vector<S_VulkanRT::Instance> allInstances = m_rtInstances;
        if( !m_rtSkinned.empty() && m_skinning && m_skinning->available() )
        {
            m_skinning->beginFrame(m_nextSwapchainImageIndex);
            std::vector<VkBuffer> skinnedBuffers(m_rtSkinned.size(), VK_NULL_HANDLE);
            bool anyDispatched = false;
            for( size_t i = 0; i < m_rtSkinned.size(); ++i )
            {
                auto& entry = m_rtSkinned[i];
                skinnedBuffers[i] = m_skinning->dispatch(
                    m_nextFrameRenderCommandBuffer, static_cast<S_VulkanMesh*>(entry.mesh),
                    entry.palette.data(), static_cast<uint32_t>(entry.palette.size()),
                    m_nextSwapchainImageIndex);
                anyDispatched = anyDispatched || skinnedBuffers[i] != VK_NULL_HANDLE;
            }
            if( anyDispatched )
            {
                m_skinning->barrierToAsBuild(m_nextFrameRenderCommandBuffer);
                for( size_t i = 0; i < m_rtSkinned.size(); ++i )
                {
                    if( skinnedBuffers[i] == VK_NULL_HANDLE ) continue;
                    auto* mesh = static_cast<S_VulkanMesh*>(m_rtSkinned[i].mesh);
                    VkDeviceAddress addr = m_rt->buildDynamicBlas(
                        m_nextFrameRenderCommandBuffer, mesh, skinnedBuffers[i],
                        mesh->vertexCount(), mesh->indexBuffer(), mesh->indexCount(),
                        m_nextSwapchainImageIndex);
                    if( addr )
                        allInstances.push_back({ addr, m_rtSkinned[i].transform });
                }
                m_rt->barrierBlasToTlas(m_nextFrameRenderCommandBuffer);
            }
        }
        m_rt->buildTlas(m_nextFrameRenderCommandBuffer, allInstances, m_nextSwapchainImageIndex);
    }

    vkCmdBeginRenderPass( m_nextFrameRenderCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

    if( m_renderCallback )
        m_renderCallback();

    if( S_UI::instance() )
        S_UI::instance()->record(m_nextFrameRenderCommandBuffer, m_nextSwapchainImageIndex);

    m_imguiLayer->render(m_nextFrameRenderCommandBuffer);

    vkCmdEndRenderPass( m_nextFrameRenderCommandBuffer );
    VK_RESULT_CHECK( vkEndCommandBuffer( m_nextFrameRenderCommandBuffer ) );
    m_nextFrameRenderCommandBuffer = nullptr;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffersSwapChain[m_nextSwapchainImageIndex];

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences( m_device, 1, &m_inFlightFences[m_currentFrame] );

    VK_RESULT_CHECK( vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) )

            //            VkSubpassDependency dependency = {};
            //    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            //    dependency.dstSubpass = 0;

            //    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            //    dependency.srcAccessMask = 0;

            //    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            //    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_nextSwapchainImageIndex;
    presentInfo.pResults = nullptr;

    VK_RESULT_CHECK( result = vkQueuePresentKHR( m_presentQueue, &presentInfo ) )

            if( result == VK_ERROR_SURFACE_LOST_KHR )
    {
        destroyWindowSurface();
        createWindowSurface();
        recreateSwapChain();
    }else if ( result == VK_ERROR_OUT_OF_DATE_KHR || m_framebufferResized )
    {
        m_framebufferResized = false;
        recreateSwapChain();
    }/*else if( result == VK_SUBOPTIMAL_KHR )
    {

    }*/

    m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;
}

void S_VulkanRendererAPI::resize(uint32_t, uint32_t)
{
    m_framebufferResized = true;
}

void S_VulkanRendererAPI::active(bool active)
{
    m_active = active;
}

S_VertexBuffer *S_VulkanRendererAPI::createVertexBuffer(uint32_t verticesCount, uint32_t indicesCount, uint32_t instancesCount, std::unique_ptr<S_VertexBufferDescriptorArray> verticesDescriptorArray, std::unique_ptr<S_VertexBufferDescriptorArray> instancesDescriptorArray )
{
    return m_itemsManager->createVertexBuffer( verticesCount, indicesCount, instancesCount, std::move(verticesDescriptorArray), std::move(instancesDescriptorArray) );
}

S_Shader *S_VulkanRendererAPI::createShader(const std::string &vertexShader, const std::string &fragmentShader, const std::string &geometryShader, const std::string &computeShader)
{
    return m_itemsManager->createShader( vertexShader, fragmentShader, geometryShader, computeShader );
}

S_Texture *S_VulkanRendererAPI::createTexture(const std::string &texture)
{
    return m_itemsManager->createTexture( texture );
}

S_TextureSampler *S_VulkanRendererAPI::createTextureSampler(const S_TextureSamplerDescriptor &descriptor)
{
    return m_itemsManager->createTextureSampler(descriptor);
}

S_Mesh *S_VulkanRendererAPI::createMesh(const std::string &path)
{
    return m_itemsManager->createMesh(path);
}

void S_VulkanRendererAPI::updatePerFrame(const void* data, size_t size)
{
    if( m_perFrame )
        m_perFrame->update(data, m_nextSwapchainImageIndex);
}

VkDescriptorSet S_VulkanRendererAPI::currentPerFrameSet() const
{
    return m_perFrame ? m_perFrame->set(m_nextSwapchainImageIndex) : VK_NULL_HANDLE;
}

S_VulkanPerFrame* S_VulkanRendererAPI::perFrame() const  { return m_perFrame.get(); }
S_VulkanBindless* S_VulkanRendererAPI::bindless() const  { return m_bindless.get(); }

uint32_t S_VulkanRendererAPI::graphicsQueueFamilyIndex()
{
    return static_cast<uint32_t>( findQueueFamilies(m_physicalDevice).IndexGraphics );
}

S_VulkanRT* S_VulkanRendererAPI::rt() const
{
    return m_rt.get();
}

void S_VulkanRendererAPI::setRtInstances(std::vector<S_VulkanRT::Instance>&& instances)
{
    m_rtInstances = std::move(instances);
}

void S_VulkanRendererAPI::setRtSkinnedInstances(std::vector<S_VulkanRT::SkinnedInstance>&& instances)
{
    m_rtSkinned = std::move(instances);
}

uint32_t S_VulkanRendererAPI::swapchainImageCount() const
{
    return static_cast<uint32_t>( m_swapChainImages.size() );
}

void S_VulkanRendererAPI::flushRenderQueue(S_Shader* shader,
                                            const std::vector<S_ResolvedDraw>& draws,
                                            const glm::mat4* transforms,
                                            uint32_t instanceCount,
                                            const glm::mat4* palettes,
                                            uint32_t paletteCount)
{
    if( draws.empty() ) return;

    auto* vkShader = static_cast<S_VulkanShader*>( shader );
    VkCommandBuffer cmd = m_nextFrameRenderCommandBuffer;

    m_bindless->uploadTransforms(transforms, instanceCount, m_nextSwapchainImageIndex);
    if( palettes && paletteCount )
        m_bindless->uploadPalettes(palettes, paletteCount, m_nextSwapchainImageIndex);
    m_bindless->bind(cmd, vkShader->pipelineLayout(), m_nextSwapchainImageIndex);

    for( const auto& draw : draws )
    {
        struct PC { uint32_t instanceIndex; uint32_t materialID; uint32_t paletteOffset; uint32_t pad; };
        PC pc = { draw.instanceIndex, draw.materialID, draw.paletteOffset, 0 };
        vkCmdPushConstants(cmd, vkShader->pipelineLayout(), VK_SHADER_STAGE_ALL_GRAPHICS,
                           0, sizeof(PC), &pc);
        static_cast<S_VulkanMesh*>( draw.mesh )->draw();
    }
}

VkInstance S_VulkanRendererAPI::instance() const
{
    return m_instance;
}

VkPhysicalDevice S_VulkanRendererAPI::physicalDevice() const
{
    return m_physicalDevice;
}

VkDevice S_VulkanRendererAPI::device() const
{
    return m_device;
}

VkQueue S_VulkanRendererAPI::transferQueue() const
{
    return m_transferQueue;
}

VkQueue S_VulkanRendererAPI::graphicsQueue() const
{
    return m_graphicsQueue;
}

VkQueue S_VulkanRendererAPI::computeQueue() const
{
    return m_computeQueue;
}

VkQueue S_VulkanRendererAPI::presentQueue() const
{
    return m_presentQueue;
}


