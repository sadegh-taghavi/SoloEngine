#include "S_ElapsedTime.h"

using namespace solo;

S_ElapsedTime::S_ElapsedTime()
{
    start();
}

void S_ElapsedTime::start()
{
    m_begin = std::chrono::steady_clock::now();
}

uint64_t S_ElapsedTime::restart()
{
    uint64_t us = elapsed();
    start();
    return us;
}

uint64_t S_ElapsedTime::elapsed()
{
    return static_cast<uint64_t>( std::chrono::duration_cast<std::chrono::microseconds>(
                                      std::chrono::steady_clock::now() - m_begin).count() );
}
