#include <map>
#include "../memory/S_AlgorithmAllocator.h"

namespace Solo
{

template<class _T1, class _T2/*, class _A = S_AlgorithmAllocator<_T>*/ >
class S_Map : public std::map<_T1, _T2/*, _A*/>
{

};

}
