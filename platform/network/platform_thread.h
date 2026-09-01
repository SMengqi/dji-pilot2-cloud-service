#ifndef __PLATFORM_THREAD_H__
#define __PLATFORM_THREAD_H__

#include <stdint.h>

uint32_t PlatformGetThreadID();

struct ThreadData
{
    void (*threadfunc)(void*);
    void* param;
    uint32_t module;
};

bool PlatformRunThread( const ThreadData& data );

#endif //__PLATFORM_THREAD_H__
