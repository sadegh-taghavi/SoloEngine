#include <stdlib.h>
#include <cstring>
#include <assert.h>
#include "S_Allocator.h"

using namespace solo;

#define POOL_SIGNATURE 2691618315
#define MEMORY_SIGNATURE 5138161962

S_Allocator S_Allocator::m_singleton;

S_Allocator::S_Allocator(uint64_t poolSize, uint64_t poolsCount)
{
    S_AtomicFlagLocker locker( &m_busyState );
    m_totalAllocateInvoked = 0;
    m_totalDeallocateInvoked = 0;
    m_lastPool = 0;
    m_poolSize = poolSize;
    m_poolsCount = static_cast<int64_t>( poolsCount );
    m_allocatedMemory = malloc( m_poolSize * static_cast<uint64_t>(m_poolsCount) + sizeof( Pool ) * static_cast<uint64_t>(m_poolsCount) );
//    memset( m_allocatedMemory, 0, m_poolSize * m_poolsCount + sizeof( Pool ) * m_poolsCount );
    m_pools = static_cast<Pool *>( m_allocatedMemory );
    for( m_tI = 0; m_tI < m_poolsCount; ++m_tI )
    {
        m_tPool = &m_pools[m_tI];
//        m_tPool->m_signature = POOL_SIGNATURE;
        m_tPool->m_allocated = 0;
        m_tPool->m_stackCounter = 0;
        m_tPool->m_memory = reinterpret_cast<void *>( reinterpret_cast<uint64_t>( &m_pools[m_poolsCount] ) + static_cast<uint64_t>(m_tI) * m_poolSize );
    }
}

void *S_Allocator::allocate(uint64_t size)
{
    S_AtomicFlagLocker locker( &m_busyState );

    ++m_totalAllocateInvoked;
    m_tSize = size + sizeof( MemoryHeader );

    auto checkPool = [this]()
    {
        m_tPool = &m_pools[m_tI];
        if( m_poolSize - m_tPool->m_allocated >= m_tSize )
        {
            m_tHeader = reinterpret_cast<MemoryHeader *>( reinterpret_cast<uint64_t>( m_tPool->m_memory ) + m_tPool->m_allocated );
            m_tHeader->m_poolIndex = static_cast<uint64_t>(m_tI);
            m_tHeader->m_signature = MEMORY_SIGNATURE;
            m_tPool->m_allocated += m_tSize;
            m_tPool->m_stackCounter++;
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
//    return malloc( size );
}

void S_Allocator::deallocate(void *rawMemory)
{
    S_AtomicFlagLocker locker( &m_busyState );
    ++m_totalDeallocateInvoked;
    m_tHeader = reinterpret_cast<MemoryHeader *>( reinterpret_cast<uint64_t>( rawMemory ) - sizeof( MemoryHeader ) );
    if( m_tHeader->m_signature != MEMORY_SIGNATURE )
        return;
    m_tPool = &m_pools[m_tHeader->m_poolIndex];
    m_tPool->m_stackCounter--;
    if( m_tPool->m_stackCounter == 0 )
        m_tPool->m_allocated = 0;
}

S_Allocator *S_Allocator::singleton()
{
    return &m_singleton;
}

S_Allocator::~S_Allocator()
{
    S_AtomicFlagLocker locker( &m_busyState );
    free( m_allocatedMemory );
}

uint64_t S_Allocator::getTotalAllocatedItems()
{
    S_AtomicFlagLocker locker( &m_busyState );
    m_tSize = 0;
    for( m_tI = 0; m_tI < m_poolsCount; ++m_tI )
    {
        m_tPool = &m_pools[m_tI];
        m_tSize += m_tPool->m_stackCounter;
    }
    return m_tSize;
}

uint64_t S_Allocator::getTotalAllocatedBytes()
{
    S_AtomicFlagLocker locker( &m_busyState );
    m_tSize = 0;
    for( m_tI = 0; m_tI < m_poolsCount; ++m_tI )
    {
        m_tPool = &m_pools[m_tI];
        m_tSize += m_tPool->m_allocated;
    }
    return m_tSize;
}

uint64_t S_Allocator::getTotalUsedPools()
{
    S_AtomicFlagLocker locker( &m_busyState );
    m_tSize = 0;
    for( m_tI = 0; m_tI < m_poolsCount; ++m_tI )
    {
        m_tPool = &m_pools[m_tI];
        if( m_tPool->m_allocated > 0 )
            ++m_tSize;

    }
    return m_tSize;
}

uint64_t S_Allocator::getTotalAllocateInvoked()
{
    S_AtomicFlagLocker locker( &m_busyState );
    return m_totalAllocateInvoked;
}

uint64_t S_Allocator::getTotalDeallocateInvoked()
{
    S_AtomicFlagLocker locker( &m_busyState );
    return m_totalDeallocateInvoked;
}


