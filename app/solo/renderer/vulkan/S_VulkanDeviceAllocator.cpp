#include "S_VulkanDeviceAllocator.h"
#include "S_VulkanAllocator.h"
#include "solo/memory/S_Allocator.h"

using namespace solo;

S_VulkanDeviceAllocator::S_VulkanDeviceAllocator(S_VulkanRendererAPI *api, uint64_t poolSize, uint64_t poolsCount) :
    m_api(api), m_poolSize( poolSize ), m_poolsCount( poolsCount )
{

}


void S_VulkanDeviceAllocator::destroy(VkBuffer &buffer)
{
    S_AtomicFlagLocker locker( &m_busyState );
    vkDestroyBuffer( m_api->device(), buffer, S_VulkanAllocator() );
    destroyMemory( reinterpret_cast<uint64_t>( reinterpret_cast<uint64_t>( buffer ) ) );
}

void S_VulkanDeviceAllocator::destroy(VkImage &image)
{
    S_AtomicFlagLocker locker( &m_busyState );
    vkDestroyImage( m_api->device(), image, S_VulkanAllocator() );
    destroyMemory( reinterpret_cast<uint64_t>( image ) );
}

void S_VulkanDeviceAllocator::destroyMemory(uint64_t bufferID)
{
    auto memoryBuffer = m_allBuffers.find( bufferID );
    if( memoryBuffer == m_allBuffers.end() )
        return;
    DevicePool *devicePool = memoryBuffer->second->Pool;
    devicePool->StackCounter--;
    if( devicePool->StackCounter == 0 )
        devicePool->Allocated = 0;
    m_allBuffers.erase( memoryBuffer );
}

void S_VulkanDeviceAllocator::destroy()
{
    S_AtomicFlagLocker locker( &m_busyState );
    m_allBuffers.clear();
    for (auto &poolList : m_specificPools)
    {
        for (auto &deviceMemory : poolList.second->DeviceMemories )
        {
            if( deviceMemory.MappedPtr )
                vkUnmapMemory( m_api->device(), deviceMemory.Memory );
            vkFreeMemory( m_api->device(), deviceMemory.Memory, S_VulkanAllocator() );
        }
    }
    m_specificPools.clear();
}

void S_VulkanDeviceAllocator::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, S_VulkanDeviceMemory &memory)
{
    S_AtomicFlagLocker locker( &m_busyState );
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_RESULT_CHECK( vkCreateBuffer( m_api->device(), &bufferInfo, S_VulkanAllocator(), &buffer) );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( m_api->device(), buffer, &memRequirements );
    allocateMemory( properties, reinterpret_cast<uint64_t>( buffer ), memRequirements, memory );

    VK_RESULT_CHECK( vkBindBufferMemory( m_api->device(), buffer, memory.Memory, memory.OffsetFromBufferBase) )
}

void S_VulkanDeviceAllocator::createImage(uint32_t width, uint32_t height, uint32_t depth,
                                          uint32_t mipLevels, uint32_t arrayLayers, VkFormat format,
                                          VkImageType imageType,VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                                          VkImage &image, S_VulkanDeviceMemory &memory)
{
    S_AtomicFlagLocker locker( &m_busyState );

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_RESULT_CHECK( vkCreateImage( m_api->device(), &imageInfo, S_VulkanAllocator(), &image) );

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements( m_api->device(), image, &memRequirements );
    allocateMemory( properties, reinterpret_cast<uint64_t>( image ), memRequirements, memory );

    VK_RESULT_CHECK( vkBindImageMemory( m_api->device(), image, memory.Memory, memory.OffsetFromBufferBase) )
}

void S_VulkanDeviceAllocator::allocateMemory(VkMemoryPropertyFlags properties, uint64_t bufferID, 
                                             const VkMemoryRequirements &memRequirements, S_VulkanDeviceMemory &memory )
{
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex( memRequirements.memoryTypeBits, properties );

    uint64_t actualSize = S_Allocator::makeAlign( memRequirements.size, memRequirements.alignment );

    MemoryBuffer *memoryBuffer = nullptr;
    DevicePool *devicePool = nullptr;
    DevicePoolList *devicePoolList = nullptr;
    allocInfo.allocationSize = m_poolSize * m_poolsCount;
    auto poolList = m_specificPools.find( allocInfo.memoryTypeIndex );
    if( poolList == m_specificPools.end() )
    {
        devicePoolList = ( m_specificPools[allocInfo.memoryTypeIndex] = std::make_unique<DevicePoolList>( this, &allocInfo) ).get();
        devicePool = devicePoolList->Pools.at( 0 ).get();
    }else
    {
        devicePoolList = poolList->second.get();
        auto checkPool = [this, &actualSize, &devicePool, &devicePoolList, &memRequirements](uint64_t poolIndex)
        {
            devicePool = devicePoolList->Pools.at( poolIndex ).get();
            if( m_poolSize - S_Allocator::makeAlign( devicePool->Allocated, memRequirements.alignment ) >= actualSize )
            {
                devicePoolList->LastPool = poolIndex;
                return true;
            }
            return false;
        };

        bool found = false;
        for( uint64_t i = devicePoolList->LastPool; i < devicePoolList->Pools.size() ; ++i )
        {
            if( checkPool( i ) )
            {
                found = true;
                break;
            }
        }

        if( !found && devicePoolList->LastPool > 0 )
        {
            for( int64_t i = static_cast<int64_t>(devicePoolList->LastPool) - 1; i >= 0; --i )
            {
                if( checkPool( static_cast<uint64_t>(i) ) )
                {
                    found = true;
                    break;
                }
            }
        }

        if( !found )
        {
            devicePoolList->realloc( this, &allocInfo );
            devicePoolList->LastPool = devicePoolList->Pools.size() - ( m_poolsCount + 1 );
            devicePool = devicePoolList->Pools.at( devicePoolList->LastPool ).get();
        }
    }

    memoryBuffer = ( m_allBuffers[ bufferID ] = std::make_unique<MemoryBuffer>() ).get();
    memoryBuffer->Size = actualSize;
    memoryBuffer->Pool = devicePool;
    uint64_t alignAllocatedPoolSize = S_Allocator::makeAlign( devicePool->Allocated, memRequirements.alignment );
    memoryBuffer->Offset = alignAllocatedPoolSize + devicePool->Offset * m_poolSize;
    devicePool->Allocated = alignAllocatedPoolSize + actualSize;
    devicePool->StackCounter++;
    auto deviceMemory = devicePoolList->DeviceMemories.at( devicePool->DeviceMemoryIndex );
    memory.Memory = deviceMemory.Memory;
    memory.OffsetFromBufferBase = memoryBuffer->Offset;
    memory.ActualSize = actualSize;
    if( deviceMemory.MappedPtr )
        memory.MappedPtr = reinterpret_cast<void *>( reinterpret_cast<uint64_t>( deviceMemory.MappedPtr ) + memory.OffsetFromBufferBase );
    else
        memory.MappedPtr = nullptr;
}

uint64_t S_VulkanDeviceAllocator::getAlign(uint64_t size, VkBufferUsageFlags usage)
{
    S_AtomicFlagLocker locker( &m_busyState );
    auto it = m_usageAlignments.find(usage);
    uint64_t alignment = 256;
    if( it == m_usageAlignments.end() )
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer;
        VK_RESULT_CHECK( vkCreateBuffer( m_api->device(), &bufferInfo, S_VulkanAllocator(), &buffer) )
                VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements( m_api->device(), buffer, &memRequirements );
        vkDestroyBuffer( m_api->device(), buffer, S_VulkanAllocator() );

        alignment = m_usageAlignments[usage] = memRequirements.alignment;

    }else
    {
        alignment = (*it).second;
    }

    return S_Allocator::makeAlign( size, alignment );
}


uint32_t S_VulkanDeviceAllocator::findMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    for (uint32_t i = 0; i < m_api->physicalDeviceMemoryProperties()->memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (m_api->physicalDeviceMemoryProperties()->memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    s_debugLayer("Memory type index not found!", typeFilter );
    return 0;
}

uint64_t S_VulkanDeviceAllocator::poolsCount() const
{
    return m_poolsCount;
}

uint64_t S_VulkanDeviceAllocator::poolSize() const
{
    return m_poolSize;
}

S_VulkanDeviceAllocator::DevicePool::DevicePool( S_VulkanDeviceAllocator::DevicePoolList *devicePoolList, uint64_t index, uint64_t offset, uint64_t deviceMemoryIndex ) :
    Allocated( 0 ), StackCounter( 0 ), Index( index ), Offset(offset), DeviceMemoryIndex(deviceMemoryIndex), PoolList( devicePoolList )
{

}

S_VulkanDeviceAllocator::DevicePoolList::DevicePoolList(S_VulkanDeviceAllocator *allocator, VkMemoryAllocateInfo *allocInfo): LastPool( 0 )
{
    realloc( allocator, allocInfo );
}

void S_VulkanDeviceAllocator::DevicePoolList::realloc(S_VulkanDeviceAllocator *allocator, VkMemoryAllocateInfo *allocInfo)
{
    DeviceMemory deviceMemory;
    VK_RESULT_CHECK( vkAllocateMemory( allocator->m_api->device(), allocInfo, S_VulkanAllocator(), &deviceMemory.Memory ) );
    if( ( allocator->m_api->physicalDeviceMemoryProperties()->memoryTypes[allocInfo->memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) )
    {
        VK_RESULT_CHECK( vkMapMemory( allocator->m_api->device(), deviceMemory.Memory, 0, VK_WHOLE_SIZE , 0, &deviceMemory.MappedPtr ) );
    }
    else
        deviceMemory.MappedPtr = nullptr;

    DeviceMemories.push_back( deviceMemory );
    for( uint64_t i = 0; i < allocator->poolsCount(); ++i )
        Pools.push_back( std::make_unique<DevicePool>( this, Pools.size(), i, DeviceMemories.size() - 1 ) );
}

