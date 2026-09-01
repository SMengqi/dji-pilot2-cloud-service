#ifndef __PLATFORMSOCKET_H__
#define __PLATFORMSOCKET_H__
#include <stdio.h>
#include <stdint.h>

#include "net_define.h"
#include "net_handle.h"
#include "err.h"
#include "ssl.h"
#include "rand.h"
#include <openssl/bio.h>
class NetConnection;
class INetSession;
class IClientConnection;

class PlatformSocket
{
    friend class NetConnection;
public:
    static bool PlatformSocketInit();
public:
    PlatformSocket( INetSession* session , NetProtocol protocol , const SessionData& sessionData );
    virtual ~PlatformSocket();
public:
    bool CreateSocketOnly(void* socketHandle);
    bool SetSocketOption();
    bool CreateSocket(  );
    bool AcceptSocket( PlatformSocket* server);
    bool DestroySocket();

    bool BindIO(bool needRecv = false);
    bool Wait() {return true;}

    virtual PlatformSocket* CreateAcceptSocket(void* socket) {return NULL;}
    virtual void OnSend( uint32_t ioSize );
    virtual void OnRecv( uint32_t ioSize );
    virtual void OnAccept();
    virtual bool SslAccept();
    virtual void OnClose();

    virtual bool Send( const uint8_t* data , uint32_t size , SndData* param );
    virtual bool Send(){ return false; }
    int SendUdp( const uint8_t* data , uint32_t size, uint32_t dstaddr , uint16_t dstport );
    int  SendSctp( const uint16_t streamID , const uint8_t* data , uint32_t size );
    int SendTcp( const uint8_t* data , uint32_t size );
    int SendRaw( const uint8_t* data , uint32_t size , uint32_t dstaddr );
    int SendSsl( const uint8_t* data , uint32_t size );


    virtual bool Recv();
    bool RecvUdp();
    bool RecvSctp();
    bool RecvTcp();
    bool RecvRaw();
    bool RecvSsl();
    bool  SslServerInit();
    bool  SslClientInit();
	
    virtual uint32_t GetLeftData()
    {
        return 0;
    }

    void SetNetHandle( const ConnHandle& handle )
    {
        m_handle = handle;
    }

    ConnHandle GetNetHandle()
    {
        return m_handle;
    }

    NetProtocol GetProtocol()
    {
        return m_protocol;
    }
    
    void SetClientConnection( IClientConnection* conn )
    {
        m_clientConn = conn;
    }

    IClientConnection* GetClientConnection()
    {
        return m_clientConn;
    }

    bool IsListenSocket()
    {
        return m_IsListeningSocket;
    }
    
    int GetSocket()
    {
        return m_socketFd;
    }

    bool IsEncrypted()
    {
    	return m_encrypted;
    }
    
    bool IsInvalid()
    {
        if(m_socketFd <= 0)
            return true;
        else
        	return false;
    }

    bool Mixed_Mode()
    {
    	return m_Mixed_Mode;
    }
    const SessionData& GetSessionData()
    {
        return m_sessionData;
    }
    bool SetSctpSocketOption();
    bool SetSctpHBPara(int interval, int count);
    bool SetTcpHBPara(int alive , int idle, int cnt, int intv);
private:
    int GetSocketType( NetProtocol protocol );
    int GetSocketProtocol( NetProtocol protocol );
    bool MakeSocketNonBlocking();
    void UpdateSctpInfo( );
    bool UpdateIpInfo();
    bool UpdateRawIpInfo();
protected:
    int            m_socketFd;
    NetProtocol    m_protocol;
    SessionData    m_sessionData;
    ConnHandle     m_handle;
    INetSession*   m_session;
    IClientConnection* m_clientConn;
    SSL *   m_ssl;
 
    uint32_t bufferDataLen;

	bool	m_IsListeningSocket;  /**< whether this socket is used for listening other client to connect */
	bool	m_encrypted;   /*whether this socket support ssl*/
	bool	m_Mixed_Mode;/*whether this ssl socket can support ssl*/
    char buffer[65536];
};


#endif //__PLATFORMSOCKET_H__
