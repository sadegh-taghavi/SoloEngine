#pragma once
#include "S_Allocator.h"
#include "solo/platforms/S_SystemDetect.h"

void* operator new[](size_t size, const char* /*name*/, int /*flags*/,
                     unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return solo::S_Allocator::singleton()->allocate( size );
//    return malloc( size );
}

void* operator new[](size_t size, size_t alignment, size_t /*alignmentOffset*/, const char* /*name*/,
                     int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return solo::S_Allocator::singleton()->allocate( size, alignment );
    // Substitute your aligned malloc.
//    return malloc_aligned(size, alignment, alignmentOffset);
//    return malloc( size );
}


#ifdef S_COMPILER_MINGW64
void* operator new(std::size_t size) _GLIBCXX_THROW (std::bad_alloc)
{
    return solo::S_Allocator::singleton()->allocate( size );
}
void* operator new[](std::size_t size) _GLIBCXX_THROW (std::bad_alloc)
{
    return solo::S_Allocator::singleton()->allocate( size );
}
void operator delete(void* p) _GLIBCXX_USE_NOEXCEPT
{
    solo::S_Allocator::singleton()->deallocate( p );
}
void operator delete[](void* p) _GLIBCXX_USE_NOEXCEPT
{
    solo::S_Allocator::singleton()->deallocate( p );
}
void operator delete(void* p, std::size_t) _GLIBCXX_USE_NOEXCEPT
{
    solo::S_Allocator::singleton()->deallocate( p );
}
void operator delete[](void* p, std::size_t) _GLIBCXX_USE_NOEXCEPT
{
    solo::S_Allocator::singleton()->deallocate( p );
}
void* operator new(std::size_t size, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
{
    return solo::S_Allocator::singleton()->allocate( size );
}
void* operator new[](std::size_t size, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
{
    return solo::S_Allocator::singleton()->allocate( size );
}
void operator delete(void* p, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
{
    solo::S_Allocator::singleton()->deallocate( p );
}
void operator delete[](void* p, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
{
    solo::S_Allocator::singleton()->deallocate( p );
}
#else
void* operator new(std::size_t __sz)
{
    return solo::S_Allocator::singleton()->allocate( __sz );
}
void* operator new(std::size_t __sz, const std::nothrow_t&)
{
    return solo::S_Allocator::singleton()->allocate( __sz );
}
void  operator delete(void* __p)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete(void* __p, const std::nothrow_t&)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete(void* __p, std::size_t /*__sz*/)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void* operator new[](std::size_t __sz)
{
    return solo::S_Allocator::singleton()->allocate( __sz );
}
void* operator new[](std::size_t __sz, const std::nothrow_t&)
{
    return solo::S_Allocator::singleton()->allocate( __sz );
}
void  operator delete[](void* __p)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete[](void* __p, const std::nothrow_t&)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete[](void* __p, std::size_t /*__sz*/)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void* operator new(std::size_t __sz, std::align_val_t align)
{
    return solo::S_Allocator::singleton()->allocate( __sz, static_cast<std::size_t>(align) );
}
void* operator new(std::size_t __sz, std::align_val_t  align, const std::nothrow_t&)
{
    return solo::S_Allocator::singleton()->allocate( __sz, static_cast<std::size_t>(align) );
}
void  operator delete(void* __p, std::align_val_t)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete(void* __p, std::align_val_t, const std::nothrow_t&)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete(void* __p, std::size_t /*__sz*/, std::align_val_t)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void* operator new[](std::size_t __sz, std::align_val_t align)
{
    return solo::S_Allocator::singleton()->allocate( __sz, static_cast<std::size_t>(align) );
}
void* operator new[](std::size_t __sz, std::align_val_t align, const std::nothrow_t&)
{
    return solo::S_Allocator::singleton()->allocate( __sz, static_cast<std::size_t>(align) );
}
void  operator delete[](void* __p, std::align_val_t)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete[](void* __p, std::align_val_t, const std::nothrow_t&)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
void  operator delete[](void* __p, std::size_t /*__sz*/, std::align_val_t)
{
    solo::S_Allocator::singleton()->deallocate( __p );
}
#endif
