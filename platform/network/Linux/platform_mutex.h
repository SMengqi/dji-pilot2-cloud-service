#ifndef __PLATFORMMUTEX_H__
#define __PLATFORMMUTEX_H__

#include <pthread.h>
#include <errno.h>

class  PlatformMutex
{
public:
    PlatformMutex();
    ~PlatformMutex();
    void lock();
    bool tryLock();
    void unlock();
    
private:
    pthread_mutex_t m_mutex;
};

#endif //__PLATFORMMUTEX_H__