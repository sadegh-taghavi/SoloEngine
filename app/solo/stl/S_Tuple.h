#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
//#include <EASTL/tuple.h>
#include <tuple>

namespace solo {

//template <class ..._Tp/*, class _A = S_AlgorithmAllocator<_T>*/ >
//class S_Tuple : public std::tuple<_Tp.../*, _A*/>
//{

//};

template <class ..._Tp/*, class _A = S_AlgorithmAllocator<_T>*/ >
using S_Tuple = std::tuple<_Tp.../*, _A*/>;

}
