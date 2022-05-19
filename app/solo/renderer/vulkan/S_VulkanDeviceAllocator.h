#pragma once

#include "S_VulkanRendererAPI.h"
#include "solo/stl/S_UnorderedMap.h"
#include "solo/stl/S_Vector.h"
#include "solo/thread/S_Mutex.h"
#include <vulkan/vulkan.h>


namespace solo
{
class S_VulkanRendererAPI;

struct S_VulkanDeviceMemory
{
    VkDeviceMemory Memory;
    void *MappedPtr;
    uint64_t ActualSize;
    uint64_t OffsetFromBufferBase;
};

class S_VulkanDeviceAllocator
{
public:
    S_VulkanDeviceAllocator(S_VulkanRendererAPI *api, uint64_t poolSize = 10 * 1024 * 1024, uint64_t poolsCount = 4);
    void destroy(VkBuffer &buffer);
    void destroy(VkImage &image);
    void destroy();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, S_VulkanDeviceMemory &memory);
    void createImage(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t arrayLayers, VkFormat format, VkImageType imageType, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, S_VulkanDeviceMemory &memory);
    uint64_t getAlign(uint64_t size, VkBufferUsageFlags usage );
    uint64_t poolSize() const;
    uint64_t poolsCount() const;

private:
    uint32_t findMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void allocateMemory(VkMemoryPropertyFlags properties, uint64_t bufferID, const VkMemoryRequirements &memRequirements, S_VulkanDeviceMemory &memory );
    void destroyMemory( uint64_t bufferID );

    struct DevicePoolList;
    struct DevicePool;
    struct MemoryBuffer;

    struct MemoryBuffer
    {
        DevicePool *Pool;
        uint64_t Size;
        uint64_t Offset;
    };

    struct DevicePool
    {
        DevicePool(S_VulkanDeviceAllocator::DevicePoolList *devicePoolList, uint64_t index, uint64_t offset, uint64_t deviceMemoryIndex );
        uint64_t Allocated;
        uint64_t StackCounter;
        uint64_t Index;
        uint64_t Offset;
        uint64_t DeviceMemoryIndex;
        DevicePoolList *PoolList;
    };

    struct DeviceMemory
    {
        VkDeviceMemory Memory;
        void *MappedPtr;
    };

    struct DevicePoolList
    {
        DevicePoolList(S_VulkanDeviceAllocator *allocator, VkMemoryAllocateInfo *allocInfo );
        void realloc( S_VulkanDeviceAllocator *allocator, VkMemoryAllocateInfo *allocInfo );
        uint64_t LastPool;
        S_Vector<std::unique_ptr<DevicePool>> Pools;
        S_Vector<DeviceMemory> DeviceMemories;

    };

    S_VulkanRendererAPI *m_api;
    uint64_t m_poolSize;
    uint64_t m_poolsCount;
    S_UnorderedMap<size_t, std::unique_ptr<MemoryBuffer>> m_allBuffers;
    S_UnorderedMap<size_t, std::unique_ptr<DevicePoolList>> m_specificPools;
    S_UnorderedMap<VkBufferUsageFlags, uint64_t> m_usageAlignments;
    S_AtomicFlag m_busyState;
};

}
