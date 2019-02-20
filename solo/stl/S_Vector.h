#include <EASTL/vector.h>
#include "../memory/S_AlgorithmAllocator.h"

namespace solo
{

template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
class S_Vector : public eastl::vector<_T/*, _A*/>
{

};

}
