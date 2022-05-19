#include "S_VulkanRendererAPI.h"
#include "S_VulkanAllocator.h"
#include "S_VulkanDeviceAllocator.h"
#include "S_VulkanItemsManager.h"
#include "S_VulkanItemsRequest.h"
#include "S_VulkanVertexBuffer.h"
#include "S_VulkanShader.h"
#include "S_VulkanTexture.h"
#include "S_VulkanTextureSampler.h"
#include "solo/platforms/S_Window.h"
#include "solo/platforms/S_SystemDetect.h"
#include "solo/debug/S_Debug.h"
#include "solo/stl/S_Map.h"
#include "solo/application/S_Application.h"
#include "solo/resource/S_ResourceManager.h"
#include "solo/thread/S_Thread.h"
#include "solo/utility/S_ElapsedTime.h"
#include "solo/renderer/S_Camera.h"
#include "solo/renderer/S_CameraController.h"
#include "solo/renderer/S_Model.h"
#include "solo/math/S_Math.h"
#if defined(S_PLATFORM_WINDOWS)
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
    appInfo.apiVersion = VK_API_VERSION_1_0;
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
    S_Vector<VkQueueFamilyProperties> queueFamilies;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    queueFamilies.resize( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );
    int index = 0;
    S_Vector<int32_t> graphicsIndices;
    S_Vector<int32_t> computeIndices;
    S_Vector<int32_t> transferIndices;
    S_Vector<int32_t> presentIndices;
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
            S_Vector<VkPhysicalDevice> devices;
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

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &enableDeviceFeatures;

    S_VulkanItemsDeviceExtensionRequest deviceExtensions;
    deviceExtensions.addRequestItem(VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    deviceExtensions.queryItems( this );

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
    m_deviceAllocator = std::make_unique<S_VulkanDeviceAllocator>(this);

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


VkSurfaceFormatKHR S_VulkanRendererAPI::chooseSwapSurfaceFormat(const S_Vector<VkSurfaceFormatKHR>& availableFormats)
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
        actualExtent.width = solo::max(capabilities.minImageExtent.width, solo::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = solo::max(capabilities.minImageExtent.height, solo::min(capabilities.maxImageExtent.height, actualExtent.height));

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
    S_VulkanDeviceMemory depthImageMemory;
    m_deviceAllocator->createImage( m_swapChainExtent.width, m_swapChainExtent.height, 1, 1, 1, depthFormat,
                                    VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, depthImageMemory );

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

    S_Array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
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
        S_Array<VkImageView, 2> attachments = { m_swapChainImageViews[i], m_depthImageView };
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
            || S_BaseApplication::executingApplication()->window()->width() == 0 ||
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
}

VkFormat S_VulkanRendererAPI::findSupportedFormat(const S_Vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
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
    S_Vector<VkFormat> vf;
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

S_VulkanDeviceAllocator *S_VulkanRendererAPI::deviceAllocator()
{
    return m_deviceAllocator.get();
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

S_VulkanRendererAPI::S_VulkanRendererAPI() : S_RendererAPI (), m_nextFrameRenderCommandBuffer(nullptr), m_framebufferResized(false), m_currentFrame(0)
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
    auto ptr = std::make_unique<S_Model>("sr:/models/scene.gltf");
    // ptr->~S_Model();
    m_pipelines = std::make_unique<S_VulkanPipeline>( this );
    m_itemsManager = std::make_unique<S_VulkanItemsManager>(this);
    struct Vertex
    {
        S_Vec3 position;
        S_Vec2 texcoord;       
    };

    S_Vector<S_VertexBufferDescriptor> veticesDescriptors;
    veticesDescriptors.resize( 2 );
    veticesDescriptors[0].Size = sizeof( S_Vec3 );
    veticesDescriptors[0].Format = S_VertexBufferDescriptorFormat::R32G32B32_SFLOAT;
    veticesDescriptors[0].Offset = offsetof( Vertex, position );
    veticesDescriptors[1].Size = sizeof( S_Vec2 );
    veticesDescriptors[1].Format = S_VertexBufferDescriptorFormat::R32G32_SFLOAT;
    veticesDescriptors[1].Offset = offsetof( Vertex, texcoord );


    struct Instance
    {
        S_Vec4 transform;
        S_Vec4 color;
    };

    S_Vector<S_VertexBufferDescriptor> instancesDescriptors;
    instancesDescriptors.resize( 2 );
    instancesDescriptors[0].Size = sizeof( S_Vec4 );
    instancesDescriptors[0].Format = S_VertexBufferDescriptorFormat::R32G32B32A32_SFLOAT;
    instancesDescriptors[0].Offset = offsetof( Instance, transform );
    instancesDescriptors[1].Size = sizeof( S_Vec4 );
    instancesDescriptors[1].Format = S_VertexBufferDescriptorFormat::R32G32B32A32_SFLOAT;
    instancesDescriptors[1].Offset = offsetof( Instance, color );

    S_Vector<S_PipelineDescriptor> pds;
    S_PipelineDescriptor pd;
    pd.VertexBufferDescriptorArray = S_VertexBufferDescriptorArray( sizeof (Vertex), veticesDescriptors );
    pd.InstanceBufferDescriptorArray = S_VertexBufferDescriptorArray( sizeof (Instance), instancesDescriptors );
    vShader = createShader( "sr:/shaders/vs", "sr:/shaders/ps", "", "" );
    pd.Shader = vShader;
    pds.push_back( pd );

    m_pipelines->create( &pds );

    createDepthResources();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();

    vVB = createVertexBuffer( 4, 6, 1600,
                              std::make_unique<S_VertexBufferDescriptorArray>( sizeof(Vertex), veticesDescriptors ),
                              std::make_unique<S_VertexBufferDescriptorArray>( sizeof(Instance), instancesDescriptors ) );
    auto vbr = vVB->beginVerticesData();
    Vertex *v = reinterpret_cast<Vertex*>( vbr.first );
    uint32_t *i = reinterpret_cast<uint32_t*>( vbr.second );
    v[0].position = S_Vec3(-0.5, 0.0, -0.5 );
    v[1].position = S_Vec3(0.5, 0.0, -0.5 );
    v[2].position = S_Vec3(-0.5, 0.0, 0.5 );
    v[3].position = S_Vec3(0.5, 0.0, 0.5 );
    v[0].texcoord = S_Vec2(0.0, 0.0);
    v[1].texcoord = S_Vec2(1.0, 0.0);
    v[2].texcoord = S_Vec2(0.0, 1.0);
    v[3].texcoord = S_Vec2(1.0, 1.0);

    i[0] = 0;
    i[1] = 1;
    i[2] = 2;
    i[3] = 2;
    i[4] = 1;
    i[5] = 3;

    vVB->endVerticesData();

    Instance *ind = reinterpret_cast<Instance*>( vVB->beginInstancesData() );
    uint32_t cntr = 0;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-2.0, 2.0);


    for( int xx = -20 ; xx < 20; ++xx )
    {
        for( int zz = -20 ; zz < 20; ++zz )
        {
            ind[cntr].transform = S_Vec4( xx, dist(mt), zz, 0.0 );
            ind[cntr].color = S_Vec4( 1.0, 1.0, 1.0, 1.0 );
            ++cntr;
        }
    }

//    ind[1].transform = S_Vec4( 1.0, 0.0, 0.0, 0.0 );
//    ind[1].color = S_Vec4( 1.0, 0.0, 0.0, 1.0 );
//    ind[2].transform = S_Vec4( 0.0, 1.0, 0.0, 0.0 );
//    ind[2].color = S_Vec4( 0.0, 1.0, 0.0, 1.0 );
//    ind[3].transform = S_Vec4( 0.0, 0.0, 1.0, 0.0 );
//    ind[3].color = S_Vec4( 0.0, 0.0, 1.0, 1.0 );

    vVB->endInstancesData();

    //    VkFormatProperties prop;
    //    vkGetPhysicalDeviceFormatProperties( m_physicalDevice, VK_FORMAT_BC7_UNORM_BLOCK, &prop );
    //    s_debug( "DDDDDDDDFFFFFFF", prop.bufferFeatures, prop.linearTilingFeatures, prop.optimalTilingFeatures );

    vTexture = m_itemsManager->createTexture("sr:/textures/sign.ktx");
    vTexture ->setSampler( m_itemsManager->createTextureSampler( S_TextureSamplerDescriptor()  ) );

    vCam = std::make_shared<S_CameraPerspective>();
    vCam->setPosition( S_Vec3( 0, 2.0, 10.0) );

    vCamController = std::make_shared<S_FirstPersonCameraController>();
    vCamController->setCamera( vCam );
}

void S_VulkanRendererAPI::cleanupSwapChain()
{
    vkDestroyImageView(m_device, m_depthImageView, S_VulkanAllocator());
    m_deviceAllocator->destroy(m_depthImage);

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
    m_deviceAllocator->destroy();
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

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    m_nextFrameRenderCommandBuffer = m_commandBuffersSwapChain[m_nextSwapchainImageIndex];
    VK_RESULT_CHECK( vkBeginCommandBuffer( m_nextFrameRenderCommandBuffer, &beginInfo) );


    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[m_nextSwapchainImageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    S_Array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.5f, 0.5f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass( m_nextFrameRenderCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

    vkCmdBindPipeline( m_nextFrameRenderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines->pipelines()->at(0) );

    vShader->bind();

    struct UPerObjectVS{
        S_Mat4x4 MVP;
    }uPerObjectVS;

    struct UPerObjectFS{
        S_Vec4 Color;
    }uPerObjectFS;

    S_Mat4x4 model;
    vCam->setWidth( S_Application::executingApplication()->window()->width() );
    vCam->setHeight( S_Application::executingApplication()->window()->height() );

    vCamController->update();

    uPerObjectVS.MVP = model.spr( S_Vec3(0, 0, 0), S_Quat().identity(), S_Vec3(1.0, 1.0, 1.0) ) * vCam->viewProjection();
    uPerObjectFS.Color = S_Vec4( 1.0, 1.0, 1.0, 1.0 );
    vShader->updateTextureValue("texSampler", S_ShaderStage::FragmentShader, *vTexture );

    vShader->updateUniformValue("UPerObject", S_ShaderStage::VertexShader, &uPerObjectVS);
    vShader->updateUniformValue("UPerObject", S_ShaderStage::FragmentShader, &uPerObjectFS);
    vShader->commit();
    vVB->draw();

    vkCmdEndRenderPass( m_nextFrameRenderCommandBuffer );
    VK_RESULT_CHECK( vkEndCommandBuffer( m_nextFrameRenderCommandBuffer ) );
    m_nextFrameRenderCommandBuffer = nullptr;

    if (m_imagesInFlight[m_nextSwapchainImageIndex] != nullptr )
        vkWaitForFences( m_device, 1, &m_imagesInFlight[m_nextSwapchainImageIndex], VK_TRUE, UINT64_MAX );

    m_imagesInFlight[m_nextSwapchainImageIndex] = m_inFlightFences[m_currentFrame];

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

S_Shader *S_VulkanRendererAPI::createShader(const S_String &vertexShader, const S_String &fragmentShader, const S_String &geometryShader, const S_String &computeShader)
{
    return m_itemsManager->createShader( vertexShader, fragmentShader, geometryShader, computeShader );
}

S_Texture *S_VulkanRendererAPI::createTexture(const S_String &texture)
{
    return m_itemsManager->createTexture( texture );
}

S_TextureSampler *S_VulkanRendererAPI::createTextureSampler(const S_TextureSamplerDescriptor &descriptor)
{
    return m_itemsManager->createTextureSampler(descriptor);
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


