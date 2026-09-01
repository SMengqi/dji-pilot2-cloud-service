#ifndef __SERVERCONNECTION_H__
#define __SERVERCONNECTION_H__

#include "iserver_connection.h"
#include "net_handle.h"

class ServerConnection: public IServerConnection
{
public:
    ServerConnection( const ConnHandle& handle )
    {
        m_handle = handle;
    }
    virtual ~ServerConnection(){};

    virtual void CloseServer();


private:
    ConnHandle m_handle;
};

#endif //__SERVERCONNECTION_H__
