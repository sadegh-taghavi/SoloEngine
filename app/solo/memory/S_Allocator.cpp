#include "S_Allocator.h"
#include <stdlib.h>
#include <cstring>
#include <assert.h>
#include <cstring>


using namespace solo;

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
        m_tPool->Signature = POOL_SIGNATURE;
        m_tPool->Allocated = 0;
        m_tPool->StackCounter = 0;
        m_tPool->Memory = reinterpret_cast<void *>( reinterpret_cast<uint64_t>( &m_pools[m_poolsCount] ) + static_cast<uint64_t>(m_tI) * m_poolSize );
    }
}

void *S_Allocator::allocate(uint64_t size, uint64_t alignment)
{
    S_AtomicFlagLocker locker( &m_busyState );
    return allocateNoLock( size, alignment );
}

void S_Allocator::deallocate(void *rawMemory)
{
    if( !rawMemory )
        return;
    S_AtomicFlagLocker locker( &m_busyState );
    deallocateNoLock( rawMemory );
}

void *S_Allocator::reallocate(void *rawMemory, uint64_t size, uint64_t alignment)
{
    S_AtomicFlagLocker locker( &m_busyState );
    m_tHeader1 = reinterpret_cast<MemoryHeader *>( reinterpret_cast<uint64_t>( rawMemory ) - sizeof( MemoryHeader ) );
    if( m_tHeader1->Signature != MEMORY_SIGNATURE )
        return nullptr;

    m_tRawMemory = allocateNoLock( size, alignment );

    ++m_totalDeallocateInvoked;
    m_tPool = &m_pools[m_tHeader1->PoolIndex];
    m_tPool->StackCounter--;
    if( m_tPool->StackCounter == 0 )
        m_tPool->Allocated = 0;

    if( !m_tRawMemory )
        return nullptr;

    std::memcpy( m_tRawMemory, &m_tHeader1[1], m_tHeader1->Size );

    return  m_tRawMemory;
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
        m_tSize += m_tPool->StackCounter;
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
        m_tSize += m_tPool->Allocated;
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
        if( m_tPool->Allocated > 0 )
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


