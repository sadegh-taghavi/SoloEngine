#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
//#include <EASTL/list.h>
#include <list>

namespace solo {

//template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
//class S_List : public std::list<_T/*, _A*/>
//{

//};

template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
using S_List = std::list<_T/*, _A*/>;

}
