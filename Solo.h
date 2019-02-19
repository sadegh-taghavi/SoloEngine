
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

void* operator new  (std::size_t count )
{
    return S_Allocator::singleton()->allocate( static_cast<uint64_t>( count ) );
}

void* operator new[](std::size_t count )
{
    return S_Allocator::singleton()->allocate( static_cast<uint64_t>( count ) );
}

void* __cdecl operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
    return S_Allocator::singleton()->allocate( size );
}

void __cdecl operator delete[](void* ptr, const char*, int, unsigned, const char*, int)
{
    S_Allocator::singleton()->deallocate( ptr );
}
