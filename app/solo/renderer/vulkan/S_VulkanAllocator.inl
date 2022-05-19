#pragma once
#include "S_VulkanAllocator.h"

using namespace solo;

inline S_VulkanAllocator::operator VkAllocationCallbacks *() const
{
    if( m_isInit )
        return &m_singleton;
    m_singleton.pUserData = nullptr;
    m_singleton.pfnAllocation = &allocation;
    m_singleton.pfnReallocation = &reallocation;
    m_singleton.pfnFree = &free;
    m_singleton.pfnInternalAllocation = nullptr;
    m_singleton.pfnInternalFree = nullptr;
    m_isInit = true;
    return &m_singleton;
}
