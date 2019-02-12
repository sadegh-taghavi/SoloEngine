#include <vector>
#include "S_AlgorithmAllocator.h"

template<class _T, class _A = S_AlgorithmAllocator<_T> >
class S_Vector : public std::vector<_T, _A>
{

};
