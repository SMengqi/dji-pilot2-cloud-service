#define THIS_MODULE MODULE_NETWORK
#include "inet_session.h"
#include "iclient_connection.h"
#include "iserver_connection.h"
#include "net_factory.h"
#include "net_manager.h"
#include "client_connection.h"
#include "server_connection.h"
//#include "sctp_test.h"

#ifdef ALL_IN_ONE
extern CWinSctpServer g_WinSctpServer;
#endif //

NetFactory* NetFactory::GetInstance()
{
    static NetFactory __instance;
    return &__instance;
}

NetFactory::NetFactory()
{
    pad = 0;
}

NetFactory::~NetFactory()
{

}

IClientConnection* NetFactory::CreateUdpConnection( INetSession* session , const SessionData& sessionData )
{
    
    ConnHandle handle = NetManager::GetInstance()->createClientConnection( session , NETPROTOCOL_UDP , sessionData );

    if( false == handle.IsValid() )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_UDPCONNECTION_UNVALID_FAIL);
        return NULL;
    }

    if ( false == NetManager::GetInstance()->InitializeConnection( handle ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_UDPCONNECTION_INITIALIZE_FAIL);
        return NULL;
    }

    ClientConnection* conn = new(std::nothrow) ClientConnection( handle );

    NetManager::GetInstance()->SetClientConnection( handle , conn );

    return conn;
}

IClientConnection* NetFactory::CreateAsfPmalConnection( INetSession* session , const SessionData& sessionData )
{
    ConnHandle handle = NetManager::GetInstance()->createClientConnection( session , NETPROTOCOL_ASFPMAL , sessionData );

    if( false == handle.IsValid() )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_ASFCONNECTION_UNVALID_FAIL);
        return NULL;
    }

    if ( false == NetManager::GetInstance()->InitializeConnection( handle ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_ASFCONNECTION_INITIALIZE_FAIL);
        return NULL;
    }

    ClientConnection* conn = new(std::nothrow) ClientConnection( handle );

    NetManager::GetInstance()->SetClientConnection( handle , conn );

    return conn;

}


IClientConnection* NetFactory::CreateTcpConnection( INetSession* session , const SessionData& sessionData )
{
    ConnHandle handle = NetManager::GetInstance()->createClientConnection( session , NETPROTOCOL_TCP , sessionData );

    if( false == handle.IsValid() )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_TCPCONNECTION_UNVALID_FAIL);
        return NULL;
    }

    if ( false == NetManager::GetInstance()->InitializeConnection( handle ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_TCPCONNECTION_INITIALIZE_FAIL);
        return NULL;
    }

    ClientConnection* conn = new(std::nothrow) ClientConnection( handle );

    NetManager::GetInstance()->SetClientConnection( handle , conn );

    return conn;
}

#pragma warning(disable: 4702)
IClientConnection* NetFactory::CreateSctpConnection( INetSession* session , const SessionData& sessionData )
{
#ifdef ALL_IN_ONE
    return g_WinSctpServer.CreateSctpConnection();
#endif
    ConnHandle handle = NetManager::GetInstance()->createClientConnection( session , NETPROTOCOL_SCTP , sessionData );

    if( false == handle.IsValid() )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_SCTPCONNECTION_UNVALID_FAIL);
        return NULL;
    }

    if ( false == NetManager::GetInstance()->InitializeConnection( handle ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_SCTPCONNECTION_INITIALIZE_FAIL);
        return NULL;
    }

    ClientConnection* conn = new(std::nothrow) ClientConnection( handle );

    NetManager::GetInstance()->SetClientConnection( handle , conn );

    return conn;
}


IClientConnection* NetFactory::CreateRawIpConnection( INetSession* session , const SessionData& sessionData )
{
    ConnHandle handle = NetManager::GetInstance()->createClientConnection( session , NETPROTOCOL_RAWIP , sessionData );

    if( false == handle.IsValid() )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_RAWIPCONNECTION_UNVALID_FAIL);
        return NULL;
    }

    if ( false == NetManager::GetInstance()->InitializeConnection( handle ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_RAWIPCONNECTION_INITIALIZE_FAIL);
        return NULL;
    }

    ClientConnection* conn = new(std::nothrow) ClientConnection( handle );

    NetManager::GetInstance()->SetClientConnection( handle , conn );
    
    return conn;
}

void NetFactory::DeleteNetConnection( IClientConnection* conn )
{
    conn->Close();
    ClientConnection* pConnection = (ClientConnection*)conn;
    delete pConnection;
}


ConnHandle NetFactory::CreateUdpServer( INetSession* session , const SessionData& sessionData )
{
   // SessionData sessionData;
   // sessionData.m_LocalIP   = ip;
   // sessionData.m_LocalPort = port;
    ConnHandle handle = NetManager::GetInstance()->createServerConnection( session , NETPROTOCOL_UDP , sessionData );


    return handle;


#if 0
    if( false == handle.IsValid() )
    {
        return NULL;
    }

    ServerConnection* conn = new(std::nothrow) ServerConnection( handle );

    return conn;
#endif

}


IServerConnection* NetFactory::CreateTcpServer( INetSession* session , const SessionData& sessionData)
{
 //   SessionData sessionData;
//    sessionData.m_LocalIP   = ip;
//    sessionData.m_LocalPort = port;
    ConnHandle handle = NetManager::GetInstance()->createServerConnection( session , NETPROTOCOL_TCP , sessionData );

    if( false == handle.IsValid() )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_TCPSERVER_CREATE_FAIL);
        return NULL;
    }


    ServerConnection* conn = new(std::nothrow) ServerConnection( handle );

    return conn;
}


IServerConnection* NetFactory::CreateSctpServer( INetSession* session , const SessionData& sessionData )
{
#ifdef ALL_IN_ONE
    return &g_WinSctpServer;
#endif
    SessionData local;
    local.m_LocalIP   = sessionData.m_LocalIP;
    local.m_LocalPort = sessionData.m_LocalPort;
    local.m_num_ostreams = sessionData.m_num_ostreams;
    local.m_max_instreams = sessionData.m_max_instreams;
    ConnHandle handle = NetManager::GetInstance()->createServerConnection( session , NETPROTOCOL_SCTP , local );

    if( false == handle.IsValid() )
    {
        PS_CPlus(CM_NES, CMNES_ID_FACTORY_SCTPSERVER_CREATE_FAIL);
        return NULL;
    }

    ServerConnection* conn = new(std::nothrow) ServerConnection( handle );

    return conn;
}

void NetFactory::DeleteServerConnection( IServerConnection* conn )
{
    conn->CloseServer();
    ServerConnection* pConnection = (ServerConnection*)conn;
    delete pConnection;
}
