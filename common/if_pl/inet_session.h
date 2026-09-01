#ifndef INETSESSION_H
#define INETSESSION_H

#include <stdint.h>

class IClientConnection;
class IServerConnection;
struct SessionData;

class INetSession
{
public:
    INetSession(){}
    virtual ~INetSession(){}
    virtual void OnRecv( IClientConnection*  , const uint8_t* data , uint32_t size , const SessionData& sessionData  ) = 0;
    virtual void OnClose( IClientConnection*  ) = 0;
    virtual void OnAccept( IClientConnection* , const SessionData&   ){};
    virtual void OnCloseServer( IServerConnection* ){};
    virtual void OnRecvASF( IClientConnection*  , uint8_t* , uint32_t , const SessionData&  ){};

    uint32_t u32BufferSize;
    uint32_t u32TcpMemSizes;
};

#endif
