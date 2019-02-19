#include <EASTL/list.h>
#include "../memory/S_AlgorithmAllocator.h"

namespace solo {

template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
class S_List : public eastl::list<_T/*, _A*/>
{

};

}
