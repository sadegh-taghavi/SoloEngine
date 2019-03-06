#pragma once
#include <stdint.h>
#include "solo/thread/S_Mutex.h"

namespace solo
{

class S_Allocator
{
public:
    static S_Allocator *singleton();
    void *allocate( uint64_t size );
    void deallocate(void * rawMemory);
    uint64_t getTotalAllocatedItems();
    uint64_t getTotalAllocatedBytes();
    uint64_t getTotalUsedPools();
    uint64_t getTotalAllocateInvoked();
    uint64_t getTotalDeallocateInvoked();
private:
    S_Allocator( uint64_t poolSize = 8 * 1024 * 1024, uint64_t poolsCount = 32 );
    ~S_Allocator();
    class Pool
    {
        friend class S_Allocator;
//      uint64_t m_signature;
        uint64_t m_allocated;
        uint64_t m_stackCounter;
        void* m_memory;
    };
    class MemoryHeader
    {
        uint64_t m_signature;
        friend class S_Allocator;
        uint64_t m_poolIndex;
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
    uint64_t m_lastPool;
    MemoryHeader *m_tHeader;
    S_AtomicFlag m_busyState;
    static S_Allocator m_singleton;
};

}
