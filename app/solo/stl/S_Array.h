#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
//#include <EASTL/array.h>
#include <array>

namespace solo {

//template<class _T, size_t _S/*, class _A = S_AlgorithmAllocator<_T>*/ >
//class S_Array : public std::array<_T, _S/*, _A*/>
//{

//};

template<class _T, size_t _S/*, class _A = S_AlgorithmAllocator<_T>*/ >
using S_Array = std::array<_T, _S/*, _A*/>;

}
