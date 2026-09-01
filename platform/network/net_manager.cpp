#define THIS_MODULE MODULE_NETWORK
#include <stdio.h>

#include "net_manager.h"
#include "platform_socket.h"
#include "platform_socket_factory.h"
#include "platform_thread.h"
#include "scoped_lock.h"
#include "platform.h"


NetManager* NetManager::GetInstance()
{
    static NetManager __instance;
    return &__instance;
}

NetManager::NetManager()
{
    init();
}

NetManager::~NetManager()
{

}

#define MAX_CONNECTION_COUNT 2048
void NetManager::init()
{
    m_connList.resize( MAX_CONNECTION_COUNT );

    for( uint32_t connindex = 0; connindex <  MAX_CONNECTION_COUNT ; connindex++ )
    {
        m_connList.at(connindex) = NULL;
    }

    NAS_PrintLog( LOG_INFO," MAX_CONNECTION_COUNT_%d " , MAX_CONNECTION_COUNT);
    
}



void ListenProc( void* obj)
{
    PlatformSocket* platformSocket = (PlatformSocket*)obj;

    platformSocket->Wait();
}



ConnHandle NetManager::createClientConnection( INetSession* session , NetProtocol protocol, const SessionData& sessionData )
{
    ConnHandle handle;

    PlatformSocket* platformSocket = PlatformSocketFactory::GetInstance()->CreatePlatformSocket( session , protocol , sessionData );
    if( NULL != platformSocket )
    {
        int32_t alloc = allocIndex( platformSocket );

        if( alloc >= 0 )
        {
            handle.updateHandle( protocol , alloc );
            platformSocket->SetNetHandle( handle );
        }
    }

    return handle;
}

ConnHandle NetManager::createServerConnection( INetSession* session ,NetProtocol protocol , const SessionData& sessionData )
{
    ConnHandle handle;

    PlatformSocket* platformSocket = PlatformSocketFactory::GetInstance()->CreatePlatformSocket( session , protocol , sessionData );
    if( NULL != platformSocket )
    {
        handle = AddPlatformSocket( platformSocket );
        
    }

    return handle;
}

bool NetManager::GetClientConnectionInfo( const ConnHandle& handle , SessionData& sessionData )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* platformSocket = FromHandle( handle );
    if( platformSocket )
    {
        sessionData = platformSocket->GetSessionData();
        return true;
    }
    else
    {
        return false;
    }
}


bool NetManager::SendData( const ConnHandle& handle , const uint8_t* data , uint32_t size , SndData* param )
{

    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* platformSocket = FromHandle( handle );
    if( platformSocket )
    {
        return platformSocket->Send( data , size , param );
    }
    else
    {
        NAS_PrintLog( LOG_ERROR," Err: NetManager::SendData  platformSocket is NULL. " );
        PS_CPlus(CM_NES, CMNES_ID_MANAGER_SENDDATA_FAIL);
        return false;
    }
}

bool NetManager::SetSctpHBPara( const ConnHandle& handle , int32_t interval , int32_t count )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* platformSocket = FromHandle( handle );
    if( platformSocket )
    {
        return platformSocket->SetSctpHBPara( interval , count );
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_MANAGER_SENDSCTPDATA_FAIL);
        return false;
    }
}



bool NetManager::SetTcpHBPara( const ConnHandle& handle ,int heartbeat_en, int idle, int cnt, int intv)
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* platformSocket = FromHandle( handle );
    if( platformSocket )
    {
        return platformSocket->SetTcpHBPara(heartbeat_en,  idle,  cnt,  intv);
    }
    
//    PS_CPlus(CM_NES, CMNES_ID_MANAGER_SENDTCPDATA_FAIL);
    return false;
}



void NetManager::Close( const ConnHandle& handle )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* platformSocket = FromHandle( handle );
    if( platformSocket )
    {
        U32 ret = PF_TIMER_FAIL;
        U32 timerid = 0;
        int socketFd = platformSocket->GetSocket();
        
        platformSocket->DestroySocket();

        uint32_t connindex = handle.GetConnIndex();

        releaseIndex( connindex );

	    ret = pf_timer_start(MODULE_LOG, TIMER_INVALID_SOCKET_DELETE_MS, TIMER_INVALID_SOCKET_DELETE_REQ, (U64)platformSocket, &timerid);

        if(PF_TIMER_SUCCESS != ret)
        {
            NAS_PrintLog(LOG_ERROR," delete platformSocket_0x%08x  socketFd_%d , start timer_%d (%d ms) fail",platformSocket,socketFd,timerid,TIMER_INVALID_SOCKET_DELETE_MS );
        }
        else
        {
            NAS_PrintLog( LOG_WARNING," delete platformSocket_0x%08x  socketFd_%d , start timer_%d (%d ms) ",platformSocket,socketFd,timerid,TIMER_INVALID_SOCKET_DELETE_MS );
        }


//        delete platformSocket;
    }
}

uint32_t NetManager::GetLeftData( const ConnHandle& handle )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* platformSocket = FromHandle( handle );
    if( platformSocket )
    {
        return platformSocket->GetLeftData();
    }
    else
    {
        return 0;
    }
}

bool NetManager::SetClientConnection( const ConnHandle& handle , IClientConnection* conn )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* platformSocket = FromHandle( handle );
    if( platformSocket )
    {
        platformSocket->SetClientConnection( conn );
        return true;
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_MANAGER_SET_CLIENT_CONNECTION_FAIL);
        return false;
    }
}





bool NetManager::BindHandle( const ConnHandle& handle )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    PlatformSocket* conn = FromHandle( handle );
    if( conn != NULL )
    {
//        NAS_PrintLog( LOG_ERROR," BindHandle BindIO " );
        return conn->BindIO();
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_MANAGER_BINDHANDLE_FAIL);
        NAS_PrintLog( LOG_ERROR," Err: BindHandle NULL " );
        return false;
    }
}

bool NetManager::InitializeConnection(const ConnHandle& handle)
{
    if ( false == BindHandle(handle) )
    {
        return false;
    }

    return true;
}


ConnHandle NetManager::AddPlatformSocket( PlatformSocket* platformSocket )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    int32_t alloc = allocIndex( platformSocket );

    ConnHandle handle;
    if( alloc >= 0 )
    {
        handle.updateHandle( platformSocket->GetProtocol() , alloc );
        platformSocket->SetNetHandle( handle );
    }

    return handle;
}

PlatformSocket* NetManager::FromHandle( const ConnHandle& handle )
{
    uint32_t connindex = handle.GetConnIndex();
    if( connindex >= m_connList.size() )
    {
        pl_log( LOG_ERROR," connindex_%d  >=  m_connList.size_%d ",connindex,m_connList.size() );
        return NULL;
    }

    PlatformSocket* conn = m_connList.at(connindex);

    if( conn == NULL )
    {
        pl_log( LOG_ERROR," conn is NULL, connindex_%d , m_connList.size_%d ",connindex,m_connList.size() );
        PS_CPlus(CM_NES, CMNES_ID_MANAGER_FROMHANDLE_CONN_FAIL);
        return NULL;
    }

    if( conn->GetNetHandle() != handle )
    {
        pl_log( LOG_ERROR," conn->GetNetHandle() != handle , connindex_%d , m_connList.size_%d ",connindex,m_connList.size() );
        PS_CPlus(CM_NES, CMNES_ID_MANAGER_FROMHANDLE_GET_FAIL);
        return NULL;
    }

    return conn;
}

int32_t NetManager::allocIndex(PlatformSocket* conn)
{
    int32_t alloc = -1;
    int32_t connindex = 0;
    for(connindex = m_currIndex; connindex <  m_connList.size() ; connindex++ )
    {
        if( m_connList.at(connindex) == NULL )
        {
            alloc = connindex;
            break;
        }
    }

    if( -1 == alloc )
    {
        for(connindex = 0 ; connindex < m_currIndex ; connindex++ )
        {
            if( m_connList.at(connindex) == NULL )
            {
                alloc = connindex;
                break;
            }
        }
    }

    if( alloc >= 0 )
    {
        m_connList.at( alloc ) = conn;
        m_currIndex = alloc;
    }

    return alloc;
}


PlatformSocket* NetManager::releaseIndex( int32_t connindex )
{
    PlatformSocket* conn = NULL;

    if( connindex >= 0 && connindex <= (int32_t)m_connList.size() )
    {
        conn = m_connList.at( connindex );

        m_connList.at( connindex ) = 0;
    }
    return conn;
}
