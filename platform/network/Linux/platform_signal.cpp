#define THIS_MODULE MODULE_NETWORK
#include "platform_signal.h"

PlatformSignal::PlatformSignal()
{
    pthread_mutex_init(&m_locker, NULL);
    pthread_cond_init( &m_cond, NULL);
}


PlatformSignal::~PlatformSignal()
{
}

void PlatformSignal::Wait( uint32_t timeout )
{    
    pthread_mutex_lock(&m_locker);
    pthread_cond_wait (&m_cond, &m_locker);
    pthread_mutex_unlock(&m_locker);    
}

void PlatformSignal::UnWait()
{
    pthread_cond_signal(&m_cond);
}