#include "core/memory/S_Allocator.h"
#include "core/algorithm/S_Algorithm.h"
#include "core/math/S_Math.h"


void* operator new  (std::size_t count )
{
    return S_Allocator::singleton()->allocate( static_cast<uint64_t>( count ) );
}

void* operator new[](std::size_t count )
{
    return S_Allocator::singleton()->allocate( static_cast<uint64_t>( count ) );
}

void operator delete (void* ptr)
{
    return S_Allocator::singleton()->deallocate( ptr );
}

void operator delete[](void* ptr)
{
    return S_Allocator::singleton()->deallocate( ptr );
}
