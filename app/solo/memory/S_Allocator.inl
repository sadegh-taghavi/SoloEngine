#pragma once
#include "S_Allocator.h"
#include "solo/debug/S_Debug.h"
#include <stdlib.h>
#include <cstring>
#include <assert.h>
#include <stdint.h>
using namespace solo;

inline void *S_Allocator::allocateNoLock(uint64_t size, uint64_t alignment)
{
    ++m_totalAllocateInvoked;
    size = makeAlign( size, alignment );

    m_tSize = size + sizeof( MemoryHeader );

    auto checkPool = [this, size, alignment]()
    {
        m_tPool = &m_pools[m_tI];
        if( m_poolSize - m_tPool->Allocated >= m_tSize + 16 )
        {
            m_tBasePtr = reinterpret_cast<uint64_t>( m_tPool->Memory ) + m_tPool->Allocated;
            m_tAlignPtr = makeAlign( m_tBasePtr, alignment );
            m_tHeader = reinterpret_cast<MemoryHeader *>( m_tAlignPtr );
            m_tHeader->PoolIndex = static_cast<uint64_t>(m_tI);
            m_tHeader->Signature = MEMORY_SIGNATURE;
            m_tHeader->Size = size;
            m_tPool->Allocated += m_tSize + (m_tAlignPtr - m_tBasePtr);
            m_tPool->StackCounter++;
            m_lastPool = static_cast<uint64_t>(m_tI);
            return true;
        }
        return false;
    };

    for( m_tI = static_cast<int64_t>(m_lastPool); m_tI < m_poolsCount ; ++m_tI )
    {
        if( checkPool() )
            return reinterpret_cast<void *>( reinterpret_cast<int64_t>(&m_tHeader[1]) );
    }

    if( m_lastPool > 0 )
    {
        for( m_tI = static_cast<int64_t>(m_lastPool) - 1; m_tI >= 0; --m_tI )
        {
            if( checkPool() )
                return reinterpret_cast<void *>( reinterpret_cast<int64_t>(&m_tHeader[1]) );
        }
    }
    return nullptr;

}

inline void S_Allocator::deallocateNoLock(void *rawMemory)
{
    ++m_totalDeallocateInvoked;
    m_tHeader = reinterpret_cast<MemoryHeader *>( reinterpret_cast<uint64_t>( rawMemory ) - sizeof( MemoryHeader ) );
    if( m_tHeader->Signature != MEMORY_SIGNATURE )
        return;
    m_tPool = &m_pools[m_tHeader->PoolIndex];
    m_tPool->StackCounter--;
    if( m_tPool->StackCounter == 0 )
        m_tPool->Allocated = 0;
}


inline uint64_t S_Allocator::makeAlign(uint64_t size, uint64_t alignment)
{
    if( size == 0 )
        return 0;
    uint64_t alignmentSize = size;
    if( alignmentSize < alignment )
        alignmentSize = alignment;
    else if( alignmentSize % alignment > 0 )
        alignmentSize += alignment - ( alignmentSize % alignment );

    return alignmentSize;
}
