#define THIS_MODULE MODULE_NETWORK
#include "client_connection.h"
#include "net_manager.h"

bool ClientConnection::GetClientConnectionInfo( SessionData& sessionData )
{
    return NetManager::GetInstance()->GetClientConnectionInfo( m_handle , sessionData );
}

bool ClientConnection::SendData( const uint8_t* data , uint32_t size , SndData* param )
{
    return NetManager::GetInstance()->SendData( m_handle , data , size , param );
}

bool ClientConnection::SetSctpHBPara( int32_t interval , int32_t count )
{
    return NetManager::GetInstance()->SetSctpHBPara( m_handle , interval, count);
}

bool ClientConnection::SetTcpHBPara(int heartbeat_en, int idle, int cnt, int intv)
{
    return NetManager::GetInstance()->SetTcpHBPara( m_handle , heartbeat_en, idle, cnt, intv);
}

void ClientConnection::Close()
{
    NetManager::GetInstance()->Close( m_handle );
    m_handle.Close();
}

uint32_t ClientConnection::GetLeftData()
{
    return NetManager::GetInstance()->GetLeftData( m_handle );
}