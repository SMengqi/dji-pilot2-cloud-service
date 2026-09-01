#ifndef __NETMANAGER_H__
#define __NETMANAGER_H__

#include "option.h"
#include "net_define.h"
#include "net_handle.h"
#include "platform_mutex.h"
#include <vector>

using namespace std;

class INetSession;
class PlatformSocket;
class IClientConnection;


class NetManager
{
public:
    static NetManager* GetInstance();

    ConnHandle createClientConnection( INetSession* session ,NetProtocol protocol , const SessionData& sessionData );
    ConnHandle createServerConnection( INetSession* session ,NetProtocol protocol , const SessionData& sessionData );
    bool GetClientConnectionInfo( const ConnHandle& handle , SessionData& sessionData ) ;

    bool SendData( const ConnHandle& handle , const uint8_t* data , uint32_t size , SndData* param );
    bool SetSctpHBPara( const ConnHandle& handle , int32_t interval , int32_t count );
    bool SetTcpHBPara( const ConnHandle& handle ,int heartbeat_en, int idle, int cnt, int intv);
    void Close( const ConnHandle& handle );

    uint32_t GetLeftData( const ConnHandle& handle );

    bool SetClientConnection( const ConnHandle& handle , IClientConnection* conn );

    bool InitializeConnection(const ConnHandle& handle);

    ConnHandle AddPlatformSocket( PlatformSocket* platSocket );
private:
    bool BindHandle( const ConnHandle& handle );
    PlatformSocket* FromHandle( const ConnHandle& handle );

    int32_t allocIndex(PlatformSocket*);
    PlatformSocket* releaseIndex( int32_t connindex );

    NetManager();

    ~NetManager();
    void init();

    vector<PlatformSocket*> m_connList;
    int32_t m_currIndex;
    PlatformMutex m_mutex;
};

#endif //__NETMANAGER_H__
