#include <chrono>
#include "S_Thread.h"

using namespace solo;


S_Runnable::S_Runnable()
{

}

S_Runnable::~S_Runnable()
{

}

void S_Runnable::operator()()
{
    run();
}

S_Thread::S_Thread()
{

}

S_Thread::~S_Thread()
{
    m_thread.join();
}

void S_Thread::start()
{
    m_thread.join();
    m_thread = std::thread( std::ref( *this ) );
}

void S_Thread::waitFor()
{
    m_thread.join();
}

void S_Thread::sleep(uint64_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned int S_Thread::hardwareThreadCount()
{
    return std::thread::hardware_concurrency();
}
