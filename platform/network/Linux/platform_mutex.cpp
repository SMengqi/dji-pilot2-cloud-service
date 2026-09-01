#include "platform_mutex.h"
#include <stdio.h>

PlatformMutex::PlatformMutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr); 
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP); 
    pthread_mutex_init( &m_mutex , &attr );
}

PlatformMutex::~PlatformMutex()
{
    pthread_mutex_destroy( &m_mutex );
}

void PlatformMutex::lock()
{
    pthread_mutex_lock( &m_mutex );
}//lint !e454 !e456

bool PlatformMutex::tryLock()
{
    int rc = pthread_mutex_trylock(&m_mutex);
    if (rc == 0)
        return true;
    else 
        return false;
}

void PlatformMutex::unlock()
{
    pthread_mutex_unlock( &m_mutex );//lint !e455
}
