#ifndef NETFACTORY_H
#define NETFACTORY_H

#include <stdint.h>

#include <string>

class INetSession;
struct SessionData;
class IClientConnection;
class IServerConnection;
class ConnHandle;

class NetFactory
{
public:
    static NetFactory* GetInstance();
private:
    NetFactory();
    ~NetFactory();
public:
    IClientConnection* CreateUdpConnection( INetSession* session , const SessionData& sessionData );

    IClientConnection* CreateTcpConnection( INetSession* session , const SessionData& sessionData );
    
    IClientConnection* CreateSctpConnection( INetSession* session , const SessionData& sessionData );

    IClientConnection* CreateRawIpConnection( INetSession* session , const SessionData& sessionData );

    IClientConnection* CreateAsfPmalConnection( INetSession* session , const SessionData& sessionData );
    
    void DeleteNetConnection( IClientConnection* conn );

    ConnHandle CreateUdpServer( INetSession* session , const SessionData& sessionData );
    
    IServerConnection* CreateTcpServer( INetSession* session , const SessionData& sessionData  );
    
    IServerConnection* CreateSctpServer( INetSession* session , const SessionData& sessionData  );
    
    void DeleteServerConnection( IServerConnection* conn );
private:
    uint32_t pad;
};


#endif
