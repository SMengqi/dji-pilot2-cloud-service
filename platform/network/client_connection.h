#ifndef __CLIENTCONNECTION_H__
#define __CLIENTCONNECTION_H__

#include "iclient_connection.h"
#include "net_handle.h"

class ClientConnection: public IClientConnection
{
public:
    ClientConnection( const ConnHandle& handle )
    {
        m_handle = handle;
    }

    virtual ~ClientConnection()
    {

    }

    virtual bool GetClientConnectionInfo( SessionData& sessionData );

    virtual bool SendData( const uint8_t* data , uint32_t size , SndData* param = 0 );
    virtual bool SetSctpHBPara( int32_t interval , int32_t count );
    virtual bool SetTcpHBPara(int heartbeat_en, int idle, int cnt, int intv);

    virtual void Close();

    virtual uint32_t GetLeftData();

private:
    ConnHandle m_handle;
};

#endif //__CLIENTCONNECTION_H__
