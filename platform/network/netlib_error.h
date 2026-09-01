#ifndef __NETLIBERROR_H__
#define __NETLIBERROR_H__

#include <stdint.h>
enum
{
    NetlibError_None                    = 0,
    NetlibError_ConnectionTimeout        = 1,
    NetlibError_ConnectionError            = 2,
    NetlibError_BindError                = 3,
    NetlibError_ListenError                = 4,
    NetlibError_SendError                = 5,
    NetlibError_RecvError                = 6,
    NetlibError_SocketInvalid            = 7,
    NetlibError_BufferOver                = 8,
};

uint32_t NetlibGetError();

void NetlibSetError( uint32_t error );//do not call this function

#endif //__NETLIBERROR_H__
