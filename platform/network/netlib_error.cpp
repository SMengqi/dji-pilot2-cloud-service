#define THIS_MODULE MODULE_NETWORK
#include "netlib_error.h"
#include "platform_io.h"
#include "platform_socket.h"
#include "platform_mutex.h"
#include "scoped_lock.h"
#include "platform_thread.h"

#include <map>
using namespace std;


static map<uint32_t,uint32_t> g_errorMap;
static PlatformMutex g_mutex;


uint32_t NetlibGetError()
{
    uint32_t id = PlatformGetThreadID();

    ScopedLock<PlatformMutex> lock(g_mutex);
    map<uint32_t,uint32_t>::iterator itor = g_errorMap.find(id);
    if( itor != g_errorMap.end() )
    {
        uint32_t value =  itor->second;
        g_errorMap.erase( itor );
        return value;
    }
    return NetlibError_None;
}

void NetlibSetError( uint32_t error )
{
    uint32_t id = PlatformGetThreadID();
    ScopedLock<PlatformMutex> lock(g_mutex);
    g_errorMap[id]= error;
    PS_CPlus(CM_NES, error);
}