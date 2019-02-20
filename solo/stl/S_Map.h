#include <EASTL/map.h>
#include "../memory/S_AlgorithmAllocator.h"

namespace solo
{

template<class _T1, class _T2/*, class _A = S_AlgorithmAllocator<_T>*/ >
class S_Map : public eastl::map<_T1, _T2/*, _A*/>
{

};

}
