#include "platform_thread.h"
#include "platform_signal.h"
#include <pthread.h>

uint32_t PlatformGetThreadID()
{
    return pthread_self();
}


struct ThreadPriData
{
    ThreadData        m_PriData;
    PlatformSignal    m_Signal;
};

static void* LinuxThreadFunc( void* obj)
{
    ThreadPriData* data = (ThreadPriData*)obj;

    data->m_PriData.threadfunc( data->m_PriData.param );

    delete data;
    
    return 0;
}




bool PlatformRunThread( const ThreadData& data )
{
    ThreadPriData* newdata = new ThreadPriData();
    newdata->m_PriData = data;
    
    pthread_t  workerThreadId;
    pthread_create(&workerThreadId, NULL,LinuxThreadFunc,newdata);

    return true;
}