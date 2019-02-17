#pragma once
#include <chrono>
#include <stdint.h>

class S_ElapsedTime
{
    std::chrono::steady_clock::time_point m_begin;
public:
    S_ElapsedTime();
    void start();
    uint64_t restart();
    uint64_t elapsed();
};
