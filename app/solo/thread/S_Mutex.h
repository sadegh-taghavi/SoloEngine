#pragma once
#include <mutex>
#include <atomic>

namespace solo
{

class S_Mutex
{
    std::mutex m_mutex;

public:
    S_Mutex();
    void lock();
    bool try_lock();
    void unlock();
};

class S_MutexLocker
{
    S_Mutex *m_mutex;
public:
    S_MutexLocker( S_Mutex *mutex );
    ~S_MutexLocker();
};

class S_AtomicFlag
{
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
public:
    S_AtomicFlag();
    bool testAndSet();
    void acquire();
    void clear();
};

class S_AtomicFlagLocker
{
    S_AtomicFlag *m_flag;
public:
    S_AtomicFlagLocker( S_AtomicFlag *flag );
    ~S_AtomicFlagLocker();
};


}
