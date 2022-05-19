#pragma once

#include <vulkan/vulkan.h>

namespace solo
{

class S_VulkanAllocator
{
public:
    operator VkAllocationCallbacks*() const;

private:
    static bool m_isInit;
    static VkAllocationCallbacks m_singleton;
    static void* VKAPI_CALL allocation( void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
    static void* VKAPI_CALL reallocation( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
    static void VKAPI_CALL free(void* pUserData, void* pMemory );
};

}


#include "S_VulkanAllocator.inl"

