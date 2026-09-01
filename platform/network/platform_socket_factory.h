#ifndef __PLATFORMSOCKETFACTORY_H__
#define __PLATFORMSOCKETFACTORY_H__

class PlatformSocket;
class INetSession;
#include "net_define.h"

class PlatformSocketFactory
{
public:
    static PlatformSocketFactory* GetInstance();
private:
    PlatformSocketFactory();
    ~PlatformSocketFactory();
public:
    PlatformSocket* CreatePlatformSocket( INetSession* session , NetProtocol protocol, const SessionData& sessionData );
private:
    uint32_t pad;
};
#endif //__PLATFORMSOCKETFACTORY_H__

