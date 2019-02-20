
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
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*name*/,
                     int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return S_Allocator::singleton()->allocate( size );
    // Substitute your aligned malloc.
//    return malloc_aligned(size, alignment, alignmentOffset);
}



///////////////////////////////////////////////////////////////////////////////
// Other operator new as typically required by applications.
///////////////////////////////////////////////////////////////////////////////

void* operator new(size_t size)
{
    return S_Allocator::singleton()->allocate( size );
}


void* operator new[](size_t size)
{
    return S_Allocator::singleton()->allocate( size );
}


///////////////////////////////////////////////////////////////////////////////
// Operator delete, which is shared between operator new implementations.
///////////////////////////////////////////////////////////////////////////////

void operator delete(void* p)
{
    S_Allocator::singleton()->deallocate( p );
}


void operator delete[](void* p)
{
    S_Allocator::singleton()->deallocate( p );
}
