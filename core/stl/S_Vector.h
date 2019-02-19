#include <vector>
#include "../memory/S_AlgorithmAllocator.h"

namespace Solo
{

template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
class S_Vector : public std::vector<_T/*, _A*/>
{

};

}
