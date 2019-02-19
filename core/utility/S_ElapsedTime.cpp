#include "S_ElapsedTime.h"

using namespace Solo;

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
    std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();
    return static_cast<uint64_t>( std::chrono::duration_cast<std::chrono::microseconds>(current - m_begin).count() );
}
