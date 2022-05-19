#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
//#include <EASTL/map.h>
#include <map>

namespace solo
{

//template<class _T1, class _T2/*, class _A = S_AlgorithmAllocator<_T>*/ >
//class S_Map : public std::map<_T1, _T2/*, _A*/>
//{

//};

template<class _T1, class _T2/*, class _A = S_AlgorithmAllocator<_T>*/ >
using S_Map = std::map<_T1, _T2/*, _A*/>;

}
