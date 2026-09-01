#ifndef __PLATFORM_SIGNAL_H__
#define __PLATFORM_SIGNAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

class PlatformSignal
{
public:
    PlatformSignal();

    ~PlatformSignal();

public:
    void Wait( uint32_t timeout );
    void UnWait();
private:
    pthread_mutex_t m_locker;//lint !e18
    pthread_cond_t  m_cond;
};
#endif //__PLATFORM_SIGNAL_H__