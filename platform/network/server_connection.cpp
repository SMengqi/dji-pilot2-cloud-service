#define THIS_MODULE MODULE_NETWORK
#include "server_connection.h"
#include "net_manager.h"

void ServerConnection::CloseServer()
{
    NetManager::GetInstance()->Close( m_handle );
}