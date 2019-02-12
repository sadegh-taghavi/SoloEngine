#pragma once
#include <stdint.h>

class S_Allocator
{
public:
    S_Allocator( uint64_t poolSize = 32 * 1024 * 1024, uint64_t poolCount = 16 );
    ~S_Allocator();
private:
    void *m_memory;
    uint64_t m_poolSize;
    uint64_t m_poolCount;
};
