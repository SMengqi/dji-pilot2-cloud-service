#define THIS_MODULE MODULE_NETWORK
#include "cross_platform_socket.h"
#include "net_lib.h"
#include "net_manager.h"
#include "net_buffer.h"
#include <assert.h>
#include "client_connection.h"
#include "scoped_lock.h"
#include "platform.h"
#include <unistd.h>
#include <signal.h>
#ifndef _MSC_VER
//#include "pf_pmal.h"
#include <arpa/inet.h>
#endif
///////////////////////////////////////////////////////////


#define THIS_MODULE MODULE_NETWORK
#define DEFAULT_BUFFER_SIZE 1024 * 1024
#define MAX_CAPACITY 1024 * 1024 * 32
#define SSL_ACCEPT_TIMEOUT_MAX 3

int ssl_accept_timeout;
 void Free_ssl_accept(int sig)
{
    NAS_PrintLog( LOG_INFO," ssl connect timeout free ssl" );

    ssl_accept_timeout = 1;
}
void Start_ssl_accept_timer()
{
    NAS_PrintLog( LOG_INFO," start ssl connect timer" );
    signal(SIGALRM,Free_ssl_accept);
    //signal(SIGALRM, std::bind(PlatformSocket::Free_ssl_connect, this);
    alarm(SSL_ACCEPT_TIMEOUT_MAX);
}

void  Close_ssl_accept_timer()
{
    NAS_PrintLog( LOG_INFO," close ssl connect timer" );
    alarm(0);
}

ServerSocket::ServerSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData )
:PlatformSocket( session ,protocol , sessionData )
{
}

PlatformSocket* ServerSocket::CreateAcceptSocket(void* socket_)
{
    PlatformSocket* obj = NULL;
    if( m_protocol == NETPROTOCOL_TCP )
    {
        obj = new(std::nothrow) ServerTcpClientSocket( m_session , m_protocol , SessionData() );
    }
    else//sctp
    {
        obj = new(std::nothrow) ServerSctpClientSocket( m_session , m_protocol , SessionData() );
    }
    
    return obj;
}

void ServerSocket::OnAccept( )
{
    PlatformSocket* acceptSocket = CreateAcceptSocket(NULL);

    if( NULL != acceptSocket )
    {
        if ( false == AcceptSocket( acceptSocket ))
        {
            NAS_PrintLog( LOG_ERROR,"accept failed\n" );
        }
        else
        {
            if(acceptSocket->IsEncrypted()== true)  /*this socket support ssl */
            {
                if(acceptSocket->SslAccept() == true)
                {
                    acceptSocket->BindIO();
                    acceptSocket->OnAccept();
                }
                else
                {
                    if(Mixed_Mode())/*if this socket support ssl-tcp mixed*/
                    {
                       acceptSocket->BindIO();
                       acceptSocket->OnAccept();
                    }
                    else
                    {
                      int clientSockFd = acceptSocket->GetSocket();
                      NAS_PrintLog( LOG_INFO," Mixed_Mode off delete tcp client  fd:%d ",clientSockFd );
                      acceptSocket->DestroySocket();
                    }
                }
            }
            else /*add to runIO as tcp*/
            {
                acceptSocket->BindIO();
                acceptSocket->OnAccept();
            }
        }
    }
}

ServerTcpClientSocket::ServerTcpClientSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData)
:TcpSocket( session ,protocol , sessionData )
{
}

void ServerTcpClientSocket::OnAccept()
{
    if( NULL == m_clientConn )
    {
        NetManager::GetInstance()->AddPlatformSocket( this );
        m_clientConn = new ClientConnection( m_handle );
    }
    
    m_session->OnAccept( m_clientConn , m_sessionData );
}

bool ServerTcpClientSocket::SslAccept()
{
    NAS_PrintLog( LOG_ERROR," SSL server start! ");

    if(!SslServerInit())
    {

        NAS_PrintLog( LOG_ERROR," server ssl init error ");
     
    }
    else
    {
        if (0 == SSL_set_fd ( m_ssl, m_socketFd))
        {
            NAS_PrintLog( LOG_ERROR," ssl fd set error ");
        }
        SSL_set_accept_state(m_ssl);

        bool isContinue = true;
        Start_ssl_accept_timer();
        while(isContinue)
        {
            if(ssl_accept_timeout)
            {
                if(m_ssl !=NULL)
                {
                     SSL_free(m_ssl);
                }
                ssl_accept_timeout = 0;
                this->m_encrypted = false;
                return false;
            }
            isContinue = false;
            if(SSL_accept(m_ssl) !=1)
            {
                int icode = -1;
                int iret = SSL_get_error(m_ssl,icode);
                if ((iret == SSL_ERROR_WANT_WRITE) || (iret == SSL_ERROR_WANT_READ))
                {
                    isContinue = true;
                }
                else
                {
                    Close_ssl_accept_timer();
                    NAS_PrintLog( LOG_ERROR," ssl server accept error %d ",iret);
                    return false;
                }

            }
            else
            {
                Close_ssl_accept_timer();
                NAS_PrintLog( LOG_ERROR," SSLAcceptSocket:  loacal_%s:%d , peer_%s:%d ",m_sessionData.m_LocalIP.c_str(), m_sessionData.m_LocalPort, m_sessionData.m_PeerIP.c_str(), m_sessionData.m_PeerPort);
               return true;
            }

        }
 
    }
    
}
///////////////////////////////////////////////////////////
ServerSctpClientSocket::ServerSctpClientSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData)
:SctpSocket( session ,protocol , sessionData)
{
}

void ServerSctpClientSocket::OnAccept()
{
    if( NULL == m_clientConn )
    {
        NetManager::GetInstance()->AddPlatformSocket( this );
        m_clientConn = new ClientConnection( m_handle );
    }
    
    m_session->OnAccept( m_clientConn , m_sessionData );
}


///////////////////////////////////////////////////////////
TcpSocket::TcpSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData )
:PlatformSocket( session , protocol , sessionData )
{
    if(session->u32BufferSize == 0)
    {
         m_sendBuffer = new(std::nothrow) CBuffer( MAX_CAPACITY);
    }
    else
    {
        m_sendBuffer = new(std::nothrow) CBuffer( session->u32BufferSize);
    }
}

TcpSocket::~TcpSocket()
{
    if( m_sendBuffer )
    {
        try
        {
            delete m_sendBuffer;
        }
        catch(...)
        {
        }
        m_sendBuffer = NULL;
    }
}


void TcpSocket::OnSend( uint32_t ioSize )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    if( m_sendBuffer )
        m_sendBuffer->ReleaseTcpBuffer( ioSize );
    Send();
}

void TcpSocket::OnRecv( uint32_t ioSize )
{
    m_session->OnRecv( m_clientConn , ( const uint8_t*)buffer , ioSize , m_sessionData );
}

bool TcpSocket::Send( const uint8_t* data , uint32_t size, SndData* )
{
    if( NULL == m_sendBuffer )
        return false;

    ScopedLock<PlatformMutex> lock(m_mutex);
    bool isEmpty = m_sendBuffer->IsEmpty();
    if( false == m_sendBuffer->AddTcpBuffer( data , size ) )
        return false;
    if( isEmpty )
    {
        return Send();
    }
    else
    {
        return true;
    }
}

bool TcpSocket::Send()
{
    int ret;
    ScopedLock<PlatformMutex> lock(m_mutex);
    while( m_sendBuffer )
    {
        const uint8_t* buf = m_sendBuffer->GetTcpBuffer();
        uint32_t size = m_sendBuffer->GetTcpLength();
        if( size == 0 )
            return true;
        if(m_encrypted == true)
        {
            ret  = PlatformSocket::SendSsl( buf , size );
        }
        else
        {
             ret = PlatformSocket::SendTcp( buf , size );
        }
        
        if( ret == 0 )
            break;
        if( ret < 0 )
            return false;

        m_sendBuffer->ReleaseTcpBuffer( ret );
    }

    return true;
}

bool TcpSocket::Recv()
{
    return PlatformSocket::RecvTcp();
}

uint32_t TcpSocket::GetLeftData()
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    if( m_sendBuffer )
    {
        return m_sendBuffer->GetLeftData();
    }
    else
    {
        return 0;
    }
}

///////////////////////////////////////////////////////////
SctpSocket::SctpSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData )
:PlatformSocket( session , protocol , sessionData)
{
    m_sendBuffer = new CBuffer( DEFAULT_BUFFER_SIZE );
}

SctpSocket::~SctpSocket()
{
    if( m_sendBuffer )
    {
        try
        {
            delete m_sendBuffer;
        }
        catch(...)
        {
        }
        m_sendBuffer = NULL;
    }
}

void SctpSocket::OnSend( uint32_t)
{

}

void SctpSocket::OnRecv( uint32_t ioSize )
{
    m_session->OnRecv( m_clientConn , (const uint8_t*)buffer , ioSize , m_sessionData );

}

bool SctpSocket::Send( const uint8_t* data , uint32_t size , SndData* param )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    bool isEmpty = m_sendBuffer->IsEmpty();
    int ret = 0;
    uint16_t streamID = 0;

    if( param )
    {
        streamID = param->m_sctp_streamID;
    }
    if( isEmpty )
    {
#ifdef S1_WORK_AROUND_TEST    //PC模块测试直接调用接口将data发到test端
        ret = s1_ps_send_data_to_test((uint8_t*)data, size, S1_TEST_DATA_TYPE_SCTP, inet_addr(this->m_sessionData.m_PeerIP.c_str()), 0,streamID);
#else
        ret = PlatformSocket::SendSctp( streamID , data , size );
#endif
        if( ret == 0 )
        {
            if( false == m_sendBuffer->AddSctpBuffer( streamID , data , size ) )
                return false;
        }
        else if ( ret < 0 )
        {
            return false;
        }
    }
    else
    {
        if( false == m_sendBuffer->AddSctpBuffer( streamID , data , size ) )
            return false;
    }

    return true;
}



bool SctpSocket::Send()
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    while( m_sendBuffer )
    {
        uint16_t streamID = 0;
        uint32_t size = m_sendBuffer->GetSctpLength();
        if( size == 0 )
            return true;

        uint8_t* buf = m_sendBuffer->GetSctpBuffer();
        streamID = m_sendBuffer->GetStreamID();
        
        int ret = PlatformSocket::SendSctp( streamID , buf , size );
        
        if( ret < 0 )
            return false;
        if( ret == 0 )
            break;

        m_sendBuffer->ReleaseSctpBuffer( (uint32_t)ret );
    }

    return true;
}

bool SctpSocket::Recv()
{
    return PlatformSocket::RecvSctp();
}

uint32_t SctpSocket::GetLeftData()
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    if( m_sendBuffer )
    {
        return m_sendBuffer->GetLeftData();
    }
    else
    {
        return 0;
    }
}


/////////////////////////////////////////////////
UdpSocket::UdpSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData )
:PlatformSocket( session , protocol , sessionData )
{
    m_sendBuffer = new CBuffer( DEFAULT_BUFFER_SIZE );
}

UdpSocket::~UdpSocket()
{
    if( m_sendBuffer )
    {
        try
        {
            delete m_sendBuffer;
        }
        catch(...)
        {
        }
        m_sendBuffer = NULL;
    }
}

void UdpSocket::OnSend( uint32_t ioSize )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    if( m_sendBuffer )
    {
        m_sendBuffer->ReleaseUdpBuffer( ioSize );
    }
}

void UdpSocket::OnRecv( uint32_t ioSize )
{
    m_session->OnRecv( m_clientConn , (const uint8_t*)this->buffer , ioSize , m_sessionData );
}


bool UdpSocket::Send( const uint8_t* data , uint32_t size, SndData* param )
{

    if( NULL == m_sendBuffer )
    {
        return false;
    }
    ScopedLock<PlatformMutex> lock(m_mutex);
    bool isEmpty = m_sendBuffer->IsEmpty();

    uint32_t dstaddr = 0;
    uint16_t dstport = 0xFFFF;
    if( param )
    {
        dstaddr = param->m_dst_address;
        dstport = param->m_dst_port;
    }
    if( isEmpty )
    {
        int ret = PlatformSocket::SendUdp( data , size , dstaddr , dstport);

        if( ret == 0 )
        {
            if( false == m_sendBuffer->AddUdpBuffer( data , size , dstaddr, dstport ) )
                return false;
        }
        else if ( ret < 0 )
        {
            return false;
        }
    }
    else
    {
        if( false == m_sendBuffer->AddUdpBuffer( data , size , dstaddr , dstport) )
            return false;
    }


    return true;
}

bool UdpSocket::Send()
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    while( m_sendBuffer )
    {
        uint32_t size = m_sendBuffer->GetUdpLength();
        if( size == 0 )
            return true;

        uint8_t* buf = m_sendBuffer->GetUdpBuffer();
        uint32_t dstaddr = m_sendBuffer->GetUdpAddr();
        uint16_t dstport = m_sendBuffer->GetUdpPort();

        int ret = PlatformSocket::SendUdp( buf , size , dstaddr , dstport );
        
        if( ret < 0 )
            return false;
        if( ret == 0 )
            break;
        m_sendBuffer->ReleaseUdpBuffer( (uint32_t)ret );
    }

    return true;
}

bool UdpSocket::Recv()
{
    return PlatformSocket::RecvUdp();
}

//----------------------------------------------------------------------------
AsfPmalSocket::AsfPmalSocket( INetSession* session ,NetProtocol protocol , const SessionData& sessionData )
:PlatformSocket( session , protocol , sessionData )
{

}

AsfPmalSocket::~AsfPmalSocket()
{

}

void AsfPmalSocket::OnSend( uint32_t ioSize )
{

}

bool AsfPmalSocket::Send( const uint8_t* data , uint32_t size, SndData* param )
{
#if 0 //ndef RUN_ON_PC    
#ifndef _MSC_VER
    if (pf_pmal_send_gtp((U8*)data, size, param->m_dst_address, param->m_dst_port) != PF_PMAL_SUCCESS)
    {
        return false;
    }
#endif
#endif
#ifdef S1_WORK_AROUND_TEST    //PC模块测试直接调用接口将data发到test端
    s1_ps_send_data_to_test((uint8_t*)data, size, S1_TEST_DATA_TYPE_ASF, param->m_dst_address,param->m_dst_port, 0);
#endif
    return true;
}

bool AsfPmalSocket::Send()
{
    return true;
}

bool AsfPmalSocket::Recv()
{
#if  0 //ndef RUN_ON_PC
#ifndef _MSC_VER
    int ret;
    unsigned short ioSize;
    struct sockaddr_in stSockAddr = {0};
    socklen_t lSockAddrLen = 0;
    char ipString[64] = {0};
    
    ret = pf_pmal_recv_gtp((uint8_t**)&this->p_gtp_buffer, &ioSize, (struct sockaddr*)&stSockAddr, &lSockAddrLen, 0);
    while (ret == PF_PMAL_SUCCESS) 
    {
        if (NULL != inet_ntop(AF_INET,&(stSockAddr.sin_addr),ipString,sizeof(stSockAddr)))
        {
            m_sessionData.m_PeerIP = ipString;
            m_sessionData.m_PeerPort = ntohs(stSockAddr.sin_port);
        }
        AsfPmalSocket::OnRecv(ioSize);

        ret = pf_pmal_recv_gtp((uint8_t**)&this->p_gtp_buffer, &ioSize, (struct sockaddr*)&stSockAddr, &lSockAddrLen, 0);
    }
#endif 
#endif
    return true;
    
}

void AsfPmalSocket::OnRecv( uint32_t ioSize )
{
    m_session->OnRecvASF( m_clientConn , (uint8_t*)this->p_gtp_buffer , ioSize , m_sessionData );
}

//----------------------------------------------------------------------------

RawIPSocket::RawIPSocket( INetSession *session , NetProtocol protocol , const SessionData &sessionData )
:PlatformSocket( session , protocol , sessionData )
{
    m_sendBuffer = new CBuffer( DEFAULT_BUFFER_SIZE );
}

RawIPSocket::~RawIPSocket()
{
    if( m_sendBuffer )
    {
        try
        {
            delete m_sendBuffer;
        }
        catch(...)
        {
        }
        m_sendBuffer = NULL;
    }
}


void RawIPSocket::OnSend( uint32_t ioSize )
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    m_sendBuffer->ReleaseRawIpBuffer( ioSize );
}

void RawIPSocket::OnRecv( uint32_t ioSize )
{
    m_session->OnRecv( m_clientConn , (const uint8_t*)this->buffer , ioSize , m_sessionData );
}


bool RawIPSocket::Send( const uint8_t* data , uint32_t size, SndData* param )
{
    if( NULL == param )
        return false;
    ScopedLock<PlatformMutex> lock(m_mutex);
    bool isEmpty = m_sendBuffer->IsEmpty();

    
    if( isEmpty )
    {
        int ret = PlatformSocket::SendRaw( data , size , param->m_dst_address );
        if( ret == 0 )
        {
            if( false == m_sendBuffer->AddRawIpBuffer( param->m_dst_address , data , size ) )
                return false;
        }
        else if ( ret < 0 )
        {
            return false;
        }
    }
    else
    {
        if( false == m_sendBuffer->AddRawIpBuffer( param->m_dst_address , data , size ) )
            return false;
    }


    return true;
}

bool RawIPSocket::Send()
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    while( true )
    {
        uint8_t* buf = m_sendBuffer->GetRawIpBuffer();

        uint32_t size = m_sendBuffer->GetRawIpLength();
        if( size == 0 )
            return true;

        uint32_t dstaddr = m_sendBuffer->GetDstAddr();
        
        int ret = PlatformSocket::SendRaw( buf , size , dstaddr );
        
        if( ret < 0 )
            return false;
        if( ret == 0 )
            break;
        m_sendBuffer->ReleaseRawIpBuffer( size );
    }

    return true;
}


bool RawIPSocket::Recv()
{
    return PlatformSocket::RecvRaw();
}
