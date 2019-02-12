#include <stdlib.h>
#include "S_Allocator.h"


S_Allocator::S_Allocator(uint64_t poolSize, uint64_t poolCount)
{
    m_poolSize = poolSize;
    m_poolCount = poolCount;
    m_memory = malloc( m_poolSize * m_poolCount );
}

S_Allocator::~S_Allocator()
{
    free( m_memory );
}


