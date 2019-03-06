#include <chrono>
#include "S_Thread.h"
#include "solo/debug/S_Debug.h"

using namespace solo;


S_Runnable::~S_Runnable()
{

}

void S_Runnable::operator()()
{
    run();
}

S_Thread::S_Thread()
{
    m_started = false;
}

S_Thread::~S_Thread()
{
    waitFor();
}

void S_Thread::start( S_Runnable *runnable )
{
    waitFor();
    m_thread = std::thread{ std::ref( *runnable ) };
    m_started = true;
}

void S_Thread::waitFor()
{
    if( m_started )
    {
        m_thread.join();
        m_started = false;
    }
}

void S_Thread::sleep(uint64_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned int S_Thread::hardwareThreadCount()
{
    return std::thread::hardware_concurrency();
}

void S_RunnableThread::start()
{
    S_Thread::start( this );
}
