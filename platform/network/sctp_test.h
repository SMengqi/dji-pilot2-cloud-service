
#ifndef __SCTP_TEST
#define __SCTP_TEST
#include "platform_mutex.h"
#include "iclient_connection.h"
#include "iserver_connection.h"
#include <map>
#include <string>
using namespace std;
class CServerClientConntion : public IClientConnection
{
public:
#pragma warning(disable: 4100)
    virtual bool GetClientConnectionInfo( SessionData& sessionData )
    {
        return true;
    }
#pragma warning(default: 4100)

    virtual bool SendData( const uint8_t* data , uint32_t size , SndData* param  );
    virtual bool SetSctpHBPara( int32_t interval , int32_t count );
    virtual void Close();

    virtual uint32_t GetLeftData() { return 0; }
};

class CClientConntion: public IClientConnection
{
public:
    virtual bool GetClientConnectionInfo( SessionData& sessionData );

    virtual bool SendData( const uint8_t* data , uint32_t size , SndData* param  );
    virtual bool SetSctpHBPara( int32_t interval , int32_t count );
    virtual void Close();

    virtual uint32_t GetLeftData() { return 0; }
};

class CWinSctpServer:public IServerConnection
{
public:
    virtual void CloseServer()
    {
    }

    IClientConnection* CreateSctpConnection();

    bool SendDataToServer( const uint8_t* data , uint32_t size );

    bool SendDataToClient( const uint8_t* data , uint32_t size );

    void CloseSctpConnection();
private:
    CServerClientConntion m_server;
    CClientConntion m_client;
    string m_main_ip;
    PlatformMutex m_mutex;
};

#endif //__SCTP_TEST