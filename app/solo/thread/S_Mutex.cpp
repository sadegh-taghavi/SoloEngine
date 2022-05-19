#include "S_Mutex.h"

using namespace solo;

S_Mutex::S_Mutex()
{

}

void S_Mutex::lock()
{
    m_mutex.lock();
}

bool S_Mutex::try_lock()
{
    return m_mutex.try_lock();
}

void S_Mutex::unlock()
{
    m_mutex.unlock();
}

S_MutexLocker::S_MutexLocker(S_Mutex *mutex)
{
    m_mutex = mutex;
    m_mutex->lock();
}

S_MutexLocker::~S_MutexLocker()
{
    m_mutex->unlock();
}

S_AtomicFlag::S_AtomicFlag()
{

}

bool S_AtomicFlag::testAndSet()
{
    return m_flag.test_and_set(std::memory_order_acquire);
}

void S_AtomicFlag::acquire()
{
    for(;m_flag.test_and_set(std::memory_order_acquire););
}

void S_AtomicFlag::clear()
{
    m_flag.clear(std::memory_order_release);
}


S_AtomicFlagLocker::S_AtomicFlagLocker(S_AtomicFlag *flag)
{
    m_flag = flag;
    m_flag->acquire();
}

S_AtomicFlagLocker::~S_AtomicFlagLocker()
{
    m_flag->clear();
}
