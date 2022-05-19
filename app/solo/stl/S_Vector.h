#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
//#include <EASTL/vector.h>
#include <vector>
namespace solo
{

//template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
//class S_Vector : public std::vector<_T/*, _A*/>
//{

//};

template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
using S_Vector = std::vector<_T/*, _A*/>;

}
