#include <list>
#include "../memory/S_AlgorithmAllocator.h"

namespace Solo {

template<class _T/*, class _A = S_AlgorithmAllocator<_T>*/ >
class S_List : public std::list<_T/*, _A*/>
{

};

}
