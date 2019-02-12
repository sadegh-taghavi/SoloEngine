#pragma once
#include <stdint.h>
#include <mutex>

class S_Allocator
{
public:
    S_Allocator( uint64_t poolSize = 2 * 1024 * 1024, uint64_t poolsCount = 16 );
    void *allocate( uint64_t size );
    void deAllocate(void * rawMemory);
    ~S_Allocator();
private:
    class Pool
    {
        friend class S_Allocator;
//        char m_signature[2];
        uint64_t m_allocated;
        uint64_t m_stackCounter;
        void* m_memory;
    };
    class MemoryHeader
    {
//        char m_signature[2];
        friend class S_Allocator;
        uint64_t m_poolIndex;
    };

    Pool *m_tPool;
    Pool *m_pools;
    void *m_allocatedMemory;
    uint64_t m_poolSize;
    uint64_t m_poolsCount;
    uint64_t m_tI;
    uint64_t m_tSize;
    MemoryHeader *m_tHeader;
    std::mutex m_mutex;
};
