#include "S_VulkanAllocator.h"
#include <mimalloc.h>

using namespace solo;

bool S_VulkanAllocator::m_isInit = false;
VkAllocationCallbacks S_VulkanAllocator::m_singleton;

void *S_VulkanAllocator::allocation(void */*pUserData*/, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/)
{
    return mi_malloc_aligned(size, alignment);
}

void *S_VulkanAllocator::reallocation(void */*pUserData*/, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope /*allocationScope*/)
{
    return mi_realloc_aligned(pOriginal, size, alignment);
}

void S_VulkanAllocator::free(void */*pUserData*/, void *pMemory)
{
    mi_free(pMemory);
}
