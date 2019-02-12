#include <stdlib.h>
#include <cstring>
#include "S_Allocator.h"

S_Allocator::S_Allocator(uint64_t poolSize, uint64_t poolsCount)
{
    m_poolSize = poolSize;
    m_poolsCount = poolsCount;
    m_allocatedMemory = malloc( m_poolSize * m_poolsCount + sizeof( Pool ) * m_poolsCount );
//    memset( m_allocatedMemory, 0, m_poolSize * m_poolsCount + sizeof( Pool ) * m_poolsCount );
    m_pools = static_cast<Pool *>( m_allocatedMemory );
    for( m_tI = 0; m_tI < m_poolsCount; ++m_tI )
    {
        m_tPool = &m_pools[m_tI];
//        m_tPool->m_signature[0] = 'P';
//        m_tPool->m_signature[1] = 'O';
        m_tPool->m_allocated = 0;
        m_tPool->m_stackCounter = 0;
        m_tPool->m_memory = reinterpret_cast<void *>( reinterpret_cast<uint64_t>( &m_pools[m_poolsCount] ) + m_tI * m_poolSize );
    }
}

void *S_Allocator::allocate(uint64_t size)
{
    m_tSize = size + sizeof( MemoryHeader );
    for( m_tI = 0; m_tI < m_poolsCount ; ++m_tI )
    {
        m_tPool = &m_pools[m_tI];
        if( m_poolSize - m_tPool->m_allocated >= m_tSize )
        {
            m_tHeader = reinterpret_cast<MemoryHeader *>( m_tPool->m_memory + m_tPool->m_allocated );
            m_tHeader->m_poolIndex = m_tI;
//            m_tHeader->m_signature[0] = 'S';
//            m_tHeader->m_signature[1] = 'E';
            m_tPool->m_allocated += m_tSize;
            m_tPool->m_stackCounter++;
            return reinterpret_cast<void *>( reinterpret_cast<int64_t>(&m_tHeader[1]) );
        }
    }
    return nullptr;
}

void S_Allocator::deAllocate(void *rawMemory)
{
    m_tHeader = reinterpret_cast<MemoryHeader *>( reinterpret_cast<uint64_t>( rawMemory ) - sizeof( MemoryHeader ) );
    m_tPool = &m_pools[m_tHeader->m_poolIndex];
    m_tPool->m_stackCounter--;
    if( m_tPool->m_stackCounter == 0 )
        m_tPool->m_allocated = 0;
}

S_Allocator::~S_Allocator()
{
    free( m_allocatedMemory );
}


