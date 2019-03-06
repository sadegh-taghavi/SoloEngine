#pragma once
#include <thread>

namespace solo
{

class S_Runnable
{
public:
    S_Runnable();
    virtual ~S_Runnable();
    virtual void run() = 0;
    void operator()();
};

class S_Thread : public S_Runnable
{
    std::thread m_thread;
public:
    S_Thread();
    ~S_Thread();
    void start();
    void waitFor();
    static void sleep( uint64_t ms );
    static unsigned int hardwareThreadCount();
};

}
