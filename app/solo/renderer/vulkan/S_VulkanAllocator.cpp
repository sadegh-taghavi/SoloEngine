#include "S_VulkanAllocator.h"
#include "solo/memory/S_Allocator.h"

using namespace solo;

bool S_VulkanAllocator::m_isInit = false;
VkAllocationCallbacks S_VulkanAllocator::m_singleton;

void *S_VulkanAllocator::allocation(void */*pUserData*/, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/)
{
//    s_debugLayer( "VK_Allocation", size, alignment );
    return S_Allocator::singleton()->allocate( size, alignment);
}

void *S_VulkanAllocator::reallocation(void */*pUserData*/, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/)
{
//    s_debugLayer( "VK_Reallocation", size, alignment );
    return S_Allocator::singleton()->reallocate( pOriginal, size, alignment );
}

void S_VulkanAllocator::free(void */*pUserData*/, void *pMemory)
{
//    s_debugLayer( "VK_Free" );
    S_Allocator::singleton()->deallocate( pMemory );
}
