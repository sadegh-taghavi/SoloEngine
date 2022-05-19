#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
//#include <EASTL/map.h>
#include <functional>

namespace solo
{

//template<class _T1/*, class _A = S_AlgorithmAllocator<_T>*/ >
//class S_Hash : public std::hash<_T1/*, _A*/>
//{

//};

template<class _T1/*, class _A = S_AlgorithmAllocator<_T>*/ >
using S_Hash = std::hash<_T1/*, _A*/>;

template<typename T>
void _hash_combine (size_t& seed, const T& val)
{
    seed ^= S_Hash<T>()( val ) + 0x9e3779b9 + ( seed<<6 ) + ( seed>>2 );
}

template<typename RT = size_t, typename... Types>
RT hash_combine (const Types&... args)
{
    std::size_t seed = 0;
    (_hash_combine(seed,args), ... );
    return seed;
}


}
