#pragma once
#include "S_Mutex.h"
#include <thread>

namespace solo
{

class S_Runnable
{
    S_AtomicFlag m_runningFlag;
    bool m_running;
    char _padding[6];
public:
    S_Runnable();
    virtual ~S_Runnable();
    virtual void run() = 0;
    void operator()();
    bool isRunning();
};

class S_Thread
{
    std::thread m_thread;
    bool m_started;
    char _padding[7];
public:
    S_Thread();
    virtual ~S_Thread();
    void start( S_Runnable *runnable );
    void waitFor();
    static void sleep( uint64_t ms );
    static unsigned int hardwareThreadCount();
};

class S_RunnableThread : public S_Runnable, public S_Thread
{
public:
    void start();
};

}
