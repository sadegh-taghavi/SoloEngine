#pragma once
//#include "../memory/S_AlgorithmAllocator.h"
#include <EASTL/tuple.h>
#include <tuple>

namespace solo {

template<typename _T1, typename _T2/*, class _A = S_AlgorithmAllocator<_T>*/ >
class S_Pair : public std::pair<_T1, _T2/*, _A*/>
{

};

}
