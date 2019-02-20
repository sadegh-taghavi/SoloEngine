#pragma once
#include "Solo/Memory/S_Allocator.h"

void* operator new[](size_t size, const char* /*name*/, int /*flags*/,
                     unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return solo::S_Allocator::singleton()->allocate( size );
//    return malloc( size );
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*name*/,
                     int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return solo::S_Allocator::singleton()->allocate( size );
    // Substitute your aligned malloc.
//    return malloc_aligned(size, alignment, alignmentOffset);
//    return malloc( size );
}



///////////////////////////////////////////////////////////////////////////////
// Other operator new as typically required by applications.
///////////////////////////////////////////////////////////////////////////////

void* operator new(size_t size)
{
    return solo::S_Allocator::singleton()->allocate( size );
//    return malloc( size );
}


void* operator new[](size_t size)
{
    return solo::S_Allocator::singleton()->allocate( size );
//    return malloc( size );
}


///////////////////////////////////////////////////////////////////////////////
// Operator delete, which is shared between operator new implementations.
///////////////////////////////////////////////////////////////////////////////

void  operator delete(void* __p)
#ifdef __MINGW64__
_GLIBCXX_USE_NOEXCEPT
#else
_NOEXCEPT
#endif
{
    solo::S_Allocator::singleton()->deallocate( __p );
}

void  operator delete(void* __p, const std::nothrow_t&)
#ifdef __MINGW64__
_GLIBCXX_USE_NOEXCEPT
#else
_NOEXCEPT
#endif
{
    solo::S_Allocator::singleton()->deallocate( __p );
}

void  operator delete(void* __p, std::size_t /*__sz*/)
#ifdef __MINGW64__
_GLIBCXX_USE_NOEXCEPT
#else
_NOEXCEPT
#endif
{
    solo::S_Allocator::singleton()->deallocate( __p );
}

void  operator delete[](void* __p)
#ifdef __MINGW64__
_GLIBCXX_USE_NOEXCEPT
#else
_NOEXCEPT
#endif
{
    solo::S_Allocator::singleton()->deallocate( __p );
}

void  operator delete[](void* __p, const std::nothrow_t&)
#ifdef __MINGW64__
_GLIBCXX_USE_NOEXCEPT
#else
_NOEXCEPT
#endif
{
    solo::S_Allocator::singleton()->deallocate( __p );
}

void  operator delete[](void* __p, std::size_t /*__sz*/)
#ifdef __MINGW64__
_GLIBCXX_USE_NOEXCEPT
#else
_NOEXCEPT
#endif
{
    solo::S_Allocator::singleton()->deallocate( __p );
}


//#ifdef __MINGW64__
//void  operator delete(void* __p, std::align_val_t) _GLIBCXX_USE_NOEXCEPT
//#else
//void  operator delete(void* __p, std::align_val_t) _NOEXCEPT
//#endif
//{
//    solo::S_Allocator::singleton()->deallocate( __p );
//}


//#ifdef __MINGW64__
//void  operator delete(void* __p, std::align_val_t, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
//#else
//void  operator delete(void* __p, std::align_val_t, const std::nothrow_t&) _NOEXCEPT
//#endif
//{
//    solo::S_Allocator::singleton()->deallocate( __p );
//}


//#ifdef __MINGW64__
//void  operator delete(void* __p, std::size_t /*__sz*/, std::align_val_t) _GLIBCXX_USE_NOEXCEPT
//#else
//void  operator delete(void* __p, std::size_t /*__sz*/, std::align_val_t) _NOEXCEPT
//#endif
//{
//    solo::S_Allocator::singleton()->deallocate( __p );
//}

//#ifdef __MINGW64__
//void  operator delete[](void* __p, std::align_val_t) _GLIBCXX_USE_NOEXCEPT
//#else
//void  operator delete[](void* __p, std::align_val_t) _NOEXCEPT
//#endif
//{
//    solo::S_Allocator::singleton()->deallocate( __p );
//}


//#ifdef __MINGW64__
//void  operator delete[](void* __p, std::align_val_t, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
//#else
//void  operator delete[](void* __p, std::align_val_t, const std::nothrow_t&) _NOEXCEPT
//#endif
//{
//    solo::S_Allocator::singleton()->deallocate( __p );
//}


//#ifdef __MINGW64__
//void  operator delete[](void* __p, std::size_t /*__sz*/, std::align_val_t) _GLIBCXX_USE_NOEXCEPT
//#else
//void  operator delete[](void* __p, std::size_t /*__sz*/, std::align_val_t) _NOEXCEPT
//#endif
//{
//    solo::S_Allocator::singleton()->deallocate( __p );
//}
