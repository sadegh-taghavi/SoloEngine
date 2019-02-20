
#include "solo/memory/S_Allocator.h"
#include "solo/memory/S_AlgorithmAllocator.h"

#include "solo/math/S_Vec2.h"
#include "solo/math/S_Vec3.h"
#include "solo/math/S_Vec4.h"
#include "solo/math/S_Quat.h"
#include "solo/math/S_Mat4x4.h"

#include "solo/stl/S_Map.h"
#include "solo/stl/S_List.h"
#include "solo/stl/S_Vector.h"
#include "solo/stl/S_String.h"

#include "solo/debug/S_Debug.h"
#include "solo/utility/S_ElapsedTime.h"

using namespace solo;

void* operator new[](size_t size, const char* /*name*/, int /*flags*/,
                     unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return S_Allocator::singleton()->allocate( size );
//    return malloc( size );
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*name*/,
                     int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return S_Allocator::singleton()->allocate( size );
    // Substitute your aligned malloc.
//    return malloc_aligned(size, alignment, alignmentOffset);
//    return malloc( size );
}



///////////////////////////////////////////////////////////////////////////////
// Other operator new as typically required by applications.
///////////////////////////////////////////////////////////////////////////////

void* operator new(size_t size)
{
    return S_Allocator::singleton()->allocate( size );
//    return malloc( size );
}


void* operator new[](size_t size)
{
    return S_Allocator::singleton()->allocate( size );
//    return malloc( size );
}


///////////////////////////////////////////////////////////////////////////////
// Operator delete, which is shared between operator new implementations.
///////////////////////////////////////////////////////////////////////////////

#ifdef __MINGW64__
void operator delete(void* p) _GLIBCXX_USE_NOEXCEPT
#else
void operator delete(void* p) _NOEXCEPT
#endif
{
      S_Allocator::singleton()->deallocate( p );
}

#ifdef __MINGW64__
void operator delete(void* p, std::size_t) _GLIBCXX_USE_NOEXCEPT
#else
void operator delete(void* p, std::size_t) _NOEXCEPT
#endif
{
      S_Allocator::singleton()->deallocate( p );
}

#ifdef __MINGW64__
void operator delete(void* p, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
#else
void operator delete(void* p, const std::nothrow_t&) _NOEXCEPT
#endif
{
      S_Allocator::singleton()->deallocate( p );
}

#ifdef __MINGW64__
void operator delete[](void* p) _GLIBCXX_USE_NOEXCEPT
#else
void operator delete[](void* p) _NOEXCEPT
#endif
{
    S_Allocator::singleton()->deallocate( p );
}

#ifdef __MINGW64__
void operator delete[](void* p, const std::nothrow_t&) _GLIBCXX_USE_NOEXCEPT
#else
void operator delete[](void* p, const std::nothrow_t&) _NOEXCEPT
#endif
{
    S_Allocator::singleton()->deallocate( p );
}

#ifdef __MINGW64__
void operator delete[](void* p, std::size_t) _GLIBCXX_USE_NOEXCEPT
#else
void operator delete[](void* p, std::size_t) _NOEXCEPT
#endif
{
      S_Allocator::singleton()->deallocate( p );
}

