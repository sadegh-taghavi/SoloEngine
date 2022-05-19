#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
//#include <EASTL/map.h>
#include <unordered_map>

namespace solo
{

//template<class _Key, class _Tp,
//     class _Hash = std::hash<_Key>,
//     class _Pred = std::equal_to<_Key>
//    /*class _A = S_AlgorithmAllocator<_T>*/ >
//class S_UnorderedMap : public std::unordered_map<_Key, _Tp, _Hash, _Pred /*, _A*/>
//{

//};

template<class _Key, class _Tp,
     class _Hash = std::hash<_Key>,
     class _Pred = std::equal_to<_Key>
    /*class _A = S_AlgorithmAllocator<_T>*/ >
using S_UnorderedMap = std::unordered_map<_Key, _Tp, _Hash, _Pred /*, _A*/>;

}
