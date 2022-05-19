#pragma once
#include "solo/thread/S_Mutex.h"
#include <stdint.h>

namespace solo
{

#define s_safe_delete(p) { if( (p) ) { delete (p); (p) = nullptr; } }

#define POOL_SIGNATURE 2691618315
#define MEMORY_SIGNATURE 5138161962

class S_Allocator
{
public:
    static S_Allocator *singleton();

    void *allocate(uint64_t size, uint64_t alignment = 8 );
    void deallocate(void * rawMemory);
    void *reallocate(void * rawMemory, uint64_t size, uint64_t alignment = 8 );
    uint64_t getTotalAllocatedItems();
    uint64_t getTotalAllocatedBytes();
    uint64_t getTotalUsedPools();
    uint64_t getTotalAllocateInvoked();
    uint64_t getTotalDeallocateInvoked();
    static uint64_t makeAlign(uint64_t size, uint64_t alignment );
private:
    void *allocateNoLock(uint64_t size, uint64_t alignment );
    void deallocateNoLock(void * rawMemory);
    S_Allocator( uint64_t poolSize = 10 * 1024 * 1024, uint64_t poolsCount = 32 );
    ~S_Allocator();
    struct Pool
    {
        uint64_t Signature;
        uint64_t Allocated;
        uint64_t StackCounter;
        void* Memory;
    };
    struct MemoryHeader
    {
        uint64_t Signature;
        uint64_t Size;
        uint64_t PoolIndex;
        uint64_t __padding;
    };

    Pool *m_tPool;
    Pool *m_pools;
    void *m_allocatedMemory;
    uint64_t m_totalAllocateInvoked;
    uint64_t m_totalDeallocateInvoked;
    uint64_t m_poolSize;
    int64_t m_poolsCount;
    int64_t m_tI;
    uint64_t m_tSize;
    uint64_t m_tBasePtr;
    uint64_t m_tAlignPtr;
    uint64_t m_lastPool;
    MemoryHeader *m_tHeader;
    MemoryHeader *m_tHeader1;
    void         *m_tRawMemory;
    S_AtomicFlag m_busyState;
    static S_Allocator m_singleton;
};

}

#include "S_Allocator.inl"
