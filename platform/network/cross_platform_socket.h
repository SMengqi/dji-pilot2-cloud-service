#ifndef __CROSSPLATFORMSOCKET_H__
#define __CROSSPLATFORMSOCKET_H__

#include "platform_socket.h"
#include "platform_mutex.h"
#include <list>

struct Node
{
    int8_t* m_buffer;
    uint32_t m_size;
};

class StreamSocket;
class CBuffer;

class ServerSocket: public PlatformSocket
{
public:
    ServerSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData );

    virtual PlatformSocket* CreateAcceptSocket(void* socket_);
    virtual void OnAccept();
    virtual bool Recv()
    {
        return false;
    }
};



class TcpSocket: public PlatformSocket
{
public:
    TcpSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData);
    ~TcpSocket();

    virtual void OnSend( uint32_t ioSize );
    virtual void OnRecv( uint32_t ioSize );
    virtual bool Send( const uint8_t* data , uint32_t size , SndData* param );
    virtual bool Send();
    virtual bool Recv();
    virtual uint32_t GetLeftData();
private:
    CBuffer* m_sendBuffer;
    PlatformMutex m_mutex;
};

class SctpSocket: public PlatformSocket
{
public:
    SctpSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData);
    virtual ~SctpSocket();

    virtual void OnSend( uint32_t ioSize );
    virtual void OnRecv( uint32_t ioSize );
    virtual bool Send( const uint8_t* data , uint32_t size , SndData* param);
    virtual bool Send();
    virtual bool Recv();
    virtual uint32_t GetLeftData();
private:
    CBuffer* m_sendBuffer;
    PlatformMutex m_mutex;
};

class UdpSocket: public PlatformSocket
{
public:
    UdpSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData );
    virtual ~UdpSocket();

    virtual void OnSend( uint32_t ioSize );
    virtual void OnRecv( uint32_t ioSize );
    virtual bool Send( const uint8_t* data , uint32_t size , SndData* param);
    virtual bool Send();
    virtual bool Recv();
private:
    CBuffer* m_sendBuffer;
    PlatformMutex m_mutex;
};

class AsfPmalSocket: public PlatformSocket
{
public:
    AsfPmalSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData );
    virtual ~AsfPmalSocket();

    virtual void OnSend( uint32_t ioSize );
    virtual void OnRecv( uint32_t ioSize );
    virtual bool Send( const uint8_t* data , uint32_t size , SndData* param);
    virtual bool Send();
    virtual bool Recv();
private:
    //CBuffer* m_sendBuffer;
    //PlatformMutex m_mutex;
    unsigned char* p_gtp_buffer;
};

class RawIPSocket: public PlatformSocket
{
public:
    RawIPSocket( INetSession* session , NetProtocol protocol , const SessionData& sessionData );
    virtual ~RawIPSocket();

    virtual void OnSend( uint32_t ioSize );
    virtual void OnRecv( uint32_t ioSize );
    virtual bool Send( const uint8_t* data , uint32_t size , SndData* param);
    virtual bool Send();
    virtual bool Recv();
private:
    CBuffer* m_sendBuffer;
    PlatformMutex m_mutex;
};


class ServerTcpClientSocket: public TcpSocket
{
public:
    ServerTcpClientSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData );

    virtual void OnAccept();
    virtual bool SslAccept();
};

class ServerSctpClientSocket: public SctpSocket
{
public:

    ServerSctpClientSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData );

    virtual void OnAccept();
};
#endif //__CROSSPLATFORMSOCKET_H__
