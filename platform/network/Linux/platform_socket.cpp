#define THIS_MODULE MODULE_NETWORK
/* C standard header file */
#include <stdio.h>

/* Linux system header file */
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/sctp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <sys/ioctl.h> 
#include <sys/stat.h>
#include <signal.h>

#include "net_define.h"
#include "platform_io.h"
#include "platform_socket.h"

#include "inet_session.h"
#include "netlib_error.h"
#include "platform.h"
//#include "pf_pmal.h"
#include "platform_net_tools.h"
#include "pcap.h"
#include<netinet/tcp.h>
#include "x509.h"
#include "err.h"
#include "ssl.h"
#include "bio.h"

#include <platform_netsession_client.h>
#include <platform_netsession_server.h>

#define SSL_CONNECT_TIMEOUT_MAX 3
int ssl_connect_timeout;

bool PlatformSocket::PlatformSocketInit()
{
    return true;
}


PlatformSocket::PlatformSocket( INetSession* session , NetProtocol protocol , const SessionData& sessionData )
{
    this->m_session              = session;
    this->m_socketFd         = -1;
    this->m_protocol          = protocol;
    this->m_sessionData       = sessionData;
    this->m_IsListeningSocket = false;
    this->bufferDataLen       = 0;
    this->m_clientConn          = NULL;
    this->m_encrypted = false;
    this->m_ssl              = NULL;
    this->m_Mixed_Mode       = false;
    pf_memset(this->buffer, 0, sizeof(this->buffer));
}


PlatformSocket::~PlatformSocket()
{

}//lint !e1540

int PlatformSocket::GetSocketType( NetProtocol protocol )
{
    switch( protocol )
    {
        case NETPROTOCOL_TCP:
        case NETPROTOCOL_SCTP:
            return SOCK_STREAM;
        case NETPROTOCOL_UDP:

            return SOCK_DGRAM;
        default://NETPROTOCOL_RAWIP
            return SOCK_RAW;
    }
}


int PlatformSocket::GetSocketProtocol( NetProtocol protocol )
{
    switch( protocol )
    {
        case NETPROTOCOL_TCP:
            return IPPROTO_TCP;
        case NETPROTOCOL_SCTP:
            return IPPROTO_SCTP;
        case NETPROTOCOL_UDP:
            return IPPROTO_UDP;
        default://NETPROTOCOL_RAWIP
            return htons(ETH_P_IP);
    }
}


bool PlatformSocket::CreateSocketOnly(void* socketHandle)
{
    if( m_protocol == NETPROTOCOL_INVALID )
    {
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATE_ONLY_PROTOCOL_FAIL);
        return false;
    }
    
    if(NULL == socketHandle)
    {
        int sockettype     = GetSocketType( m_protocol );
        int socketprotocol = GetSocketProtocol( m_protocol );;

        if( sockettype != SOCK_RAW )
        {
            m_socketFd = socket( AF_INET , sockettype , socketprotocol );
        }
        else
        {
            int domain = PF_PACKET;
            m_socketFd = socket( domain , sockettype , socketprotocol );
        }

        /* On success, a file descriptor for the new socket is returned.  On error, -1
           is returned, and errno is set appropriately. */
        if( -1 == m_socketFd)
        {
            NAS_PrintLog( LOG_INFO, " socket error:%d " , errno );
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATE_ONLY_SOCKET_FAIL);
            return false;
        }

        
    }
    else
    {
        int* socketHandleToSet =  (int*)socketHandle;
        m_socketFd = *socketHandleToSet;
    }
    
    return true;
}

bool PlatformSocket::MakeSocketNonBlocking()
{
    int flags = fcntl (this->m_socketFd, F_GETFL , 0 );

    if (flags == -1)
    {
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_NONBLOCKING_FGETFL_FAIL);
        return false;
    }

    flags |= O_NONBLOCK;
    int setResult = fcntl (this->m_socketFd, F_SETFL, flags);
    if (setResult == -1)
    {
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_NONBLOCKING_FSETFL_FAIL);
        return false;
    }

    return true;
}

void PlatformSocket::UpdateSctpInfo( )
{
}

bool PlatformSocket::UpdateIpInfo()
{
    sockaddr_in local;
    socklen_t len = sizeof(local);
    int nRet = getsockname( m_socketFd , (sockaddr*)&local , &len );
    if( nRet == 0 )
    {
        char ipString[256]={0};
        if (NULL != inet_ntop(AF_INET,&(local.sin_addr),ipString,sizeof(local) ) )
        {
            m_sessionData.m_LocalIP = ipString;
            m_sessionData.m_LocalPort = ntohs( local.sin_port );
//            printf( "ip:%s,port:%d\n" , ipString , m_sessionData.m_LocalPort );
//            NAS_PrintLog( LOG_ERROR," IP:%s,PORT:%d " , ipString , m_sessionData.m_LocalPort);
        }
        else
        {
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_UPDATEINFO_INETNTOP_FAIL);
            return false;
        }
    }
    else
    {
        NAS_PrintLog( LOG_ERROR," getsockname error,errno:%d " , errno );
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_UPDATEINFO_GETSOCK_FAIL);
        return false;
    }
    return true;
}

bool PlatformSocket::UpdateRawIpInfo()
{
    if( m_protocol == NETPROTOCOL_RAWIP )
    {
        IpInformation info;
        if( false == FindIpInformation( m_sessionData.m_LocalIP , info ) )
        {
            NAS_PrintLog( LOG_ERROR," local ip is not exist ");
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_UPDATERAWIP_FINDIP_FAIL);
            return false;
        }
        
        struct ifreq ifr;
        
        memset(&ifr, 0, sizeof(ifr));
        strcpy( ifr.ifr_name , info.m_interfaceName.c_str() );
        
        int ret = setsockopt( this->m_socketFd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr) );
        
        if( ret != 0 )
        {
            NAS_PrintLog( LOG_ERROR," failed to bind interface %s" , info.m_interfaceName.c_str() );
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_UPDATERAWIP_SETSOCK_FAIL);
            return false;
        }
    }
    
    return true;
}

void Show_Certs(SSL* ssl)
{
    X509 *cert;
    char * line;

    cert = SSL_get1_peer_certificate(ssl);
    if(cert)
    {
        NAS_PrintLog(INF,"cert information\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert),0,0);
        NAS_PrintLog(INF,"cert %s\n",line);
        
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        NAS_PrintLog(INF,"issuer name: %s\n", line);
    }
    else
    {
        NAS_PrintLog(INF,"no cert information\n");
    }
}

 void Free_ssl_connect(int sig)
{
    NAS_PrintLog( LOG_INFO," ssl connect timeout free ssl" );

    ssl_connect_timeout = 1;
}

void Start_ssl_connect_timer()
{
    NAS_PrintLog( LOG_INFO," start ssl connect timer" );
    signal(SIGALRM,Free_ssl_connect);
    //signal(SIGALRM, std::bind(PlatformSocket::Free_ssl_connect, this);
    alarm(SSL_CONNECT_TIMEOUT_MAX);
}

void  Close_ssl_connect_timer()
{
    NAS_PrintLog( LOG_INFO," close ssl connect timer" );
    alarm(0);
}

#define MAX_QUEUING_CONNECTION_NUMBER (10)
bool PlatformSocket::CreateSocket( )
{


    /* handle ASF PMAL socket seperately */
    if (NETPROTOCOL_ASFPMAL == m_protocol)
    {
#if 0 //ndef RUN_ON_PC
        m_socketFd = pf_pmal_get_gtp_rx_socket();
        
        if (m_socketFd == -1) 
            return false;
        else
            return true;
#endif            
            return true;
    }

    /* handle normal socket */
    if( false == CreateSocketOnly(NULL) )
    {
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_ONLY_FAIL);
        return false;
    }

    int sockettype = GetSocketType( m_protocol );
    
    
    if( false == UpdateRawIpInfo() )
    {
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_UPDATERAWIP_FAIL);
        return false;
    }

    if( m_protocol != NETPROTOCOL_RAWIP )
    {
        if( m_sessionData.m_LocalIP.empty() == false || m_sessionData.m_LocalPort != 0)
        {
            sockaddr_in local;
            local.sin_family = AF_INET;

            if (m_sessionData.m_LocalIP.empty() == true)
            {
                local.sin_addr.s_addr = INADDR_ANY;
            }
            else
            {
                local.sin_addr.s_addr = inet_addr(m_sessionData.m_LocalIP.c_str());

                if (INADDR_NONE == local.sin_addr.s_addr)
                {
                    DestroySocket();
                    PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_INADDR_FAIL);
                    return false;
                }
            }

            local.sin_port = htons( m_sessionData.m_LocalPort );

            if( m_protocol == NETPROTOCOL_SCTP )
            {
                uint32_t value = 1;
                setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
            }

           /* ���� SIGPIPE �źţ����� socket �ڽ���д����ʱ������ SIGPIPE �źš�
                int set = 1;
                unix  : setsockopt(sd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
                linux : setsockopt(sd, SOL_SOCKET, MSG_NOSIGNAL, (void*)&set, sizeof(int));
            */
            if( m_protocol == NETPROTOCOL_TCP )
            {
                NAS_PrintLog( LOG_FATAL," CreateSocket  TCP  SO_REUSEADDR  SO_NOSIGPIPE , m_socketFd = %d , Local_%s:%d , Peer_%s:%d ",m_socketFd,m_sessionData.m_LocalIP.c_str(), m_sessionData.m_LocalPort ,m_sessionData.m_PeerIP.c_str(), m_sessionData.m_PeerPort );
                uint32_t value = 1;
                setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
                setsockopt(m_socketFd, SOL_SOCKET, MSG_NOSIGNAL, &value, sizeof(value));
            }
            else if( m_protocol == NETPROTOCOL_UDP )
            {
                NAS_PrintLog( LOG_FATAL," CreateSocket  UDP, m_socketFd = %d  , Local_%s:%d , Peer_%s:%d ",m_socketFd,m_sessionData.m_LocalIP.c_str(), m_sessionData.m_LocalPort ,m_sessionData.m_PeerIP.c_str(), m_sessionData.m_PeerPort );
            }

            int nRet = bind( m_socketFd , (sockaddr*)&local , sizeof(local) );
            if( -1 == nRet )
            {
                NetlibSetError( NetlibError_BindError );
                NAS_PrintLog( LOG_ERROR," m_socketFd = %d , bind error:%d ",m_socketFd , errno );
                DestroySocket();
                return false;
            }
        }
    }
    

    if(SOCK_STREAM == sockettype)
    {
//printf("\r\n PlatformSocket::CreateSocket  m_LocalIP = %s:%d ",m_sessionData.m_LocalIP.c_str(), m_sessionData.m_LocalPort );

        if( m_sessionData.m_PeerIP.empty() == true || m_sessionData.m_PeerPort == 0 )
        {
            if (false == MakeSocketNonBlocking())
            {
                DestroySocket();

                NAS_PrintLog( LOG_ERROR," PlatformSocket::CreateSocket  MakeSocketNonBlocking fail ");
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_NONBLOCKING_FAIL);

                return false;
            }

            int nRet = listen( m_socketFd , MAX_QUEUING_CONNECTION_NUMBER );

            if( -1 == nRet )
            {
                NetlibSetError( NetlibError_ListenError );
                NAS_PrintLog( LOG_ERROR," m_socketFd = %d , listen error:%d " , m_socketFd, errno );
                DestroySocket();
                return false;
            }
            this->m_IsListeningSocket = true;
            
           if(m_sessionData.m_CryptoCfg == CRYPTO_CFG_SSL)
           {
               this->m_encrypted = 1;    
               this->m_Mixed_Mode = 0;
           }
           else if(m_sessionData.m_CryptoCfg == CRYPTO_CFG_SSL_MIXED)
           { 
               this->m_encrypted = 1;
               this->m_Mixed_Mode = 1;
            }    
           else 
           { 
               this->m_encrypted = 0;    
               this->m_Mixed_Mode = 0;     
           }            
     
        }
        else
        {
            sockaddr_in peer;
            peer.sin_family      = AF_INET;
            peer.sin_addr.s_addr = inet_addr(m_sessionData.m_PeerIP.c_str());

            if (INADDR_NONE == peer.sin_addr.s_addr)
            {
                DestroySocket();
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_INETADDR_FAIL);
                NAS_PrintLog( LOG_ERROR," CreateSocket  peer.sin_addr.s_addr fail ");
                return false;
            }
            
            if( false == SetSocketOption() )
            {
                DestroySocket();
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_SETSOCKET_FAIL);
                NAS_PrintLog( LOG_ERROR," CreateSocket  SetSocketOption fail ");
                return false;
            }

            peer.sin_port   = htons(m_sessionData.m_PeerPort);
            int nRet        = connect( m_socketFd ,  (sockaddr*)&peer , sizeof(peer) );

            if( -1 == nRet )
            {
                if( errno == EINPROGRESS )
                {
                    fd_set fds;
                    FD_ZERO( &fds );
                    FD_SET( m_socketFd , &fds );//lint !e573
                    
                    int nfds = m_socketFd+1;
                    timeval t;
                    t.tv_sec = 2;
                    t.tv_usec = 0;
                    nRet = select( nfds , NULL , &fds , NULL , &t );

                    if( nRet != 1 )
                    {
                        //timeout
                        NAS_PrintLog( LOG_ERROR, " CreateSocket  select fail , nRet=%d ",nRet);
                        DestroySocket();
                        NetlibSetError( NetlibError_ConnectionTimeout );

                        return false;
                    }
                    else
                    {
                        int getopterr = 0;
                        socklen_t len = sizeof(int);
                        getsockopt(m_socketFd,SOL_SOCKET,SO_ERROR,&getopterr,&len);
                        if( getopterr != 0 )
                        {
                            //timeout
                            NAS_PrintLog( LOG_ERROR, " CreateSocket  getsockopt SO_ERROR: %d ",getopterr);
                            DestroySocket();
                            NetlibSetError( NetlibError_ConnectionTimeout );

                            return false;
                        }
                    }
                }
                else
                {
                    NAS_PrintLog( LOG_ERROR," connection error:%d " , errno );
                    DestroySocket();
                    NetlibSetError( NetlibError_ConnectionError );
                    return false;
                }
            }

            CHAR acPathSrc[256];
            pf_memset(acPathSrc, 0, 256);
            sprintf(acPathSrc, "%s%s", pf_get_root_path(), "/config/bootup/dr_auto_ssl");
            
            if(pf_is_file_exist((S8 *) acPathSrc)) /*start SSL connect*/
            {
                this->m_encrypted = true;
                NAS_PrintLog( LOG_INFO," SSL connect start! ");
                    
                if(!SslClientInit())
                {
                     NAS_PrintLog( LOG_ERROR, " SSL Client Init Error ");
                     return false;
                }
                else
                {
                    if (0 >= SSL_set_fd (m_ssl, m_socketFd))
                    {
                         NAS_PrintLog( LOG_ERROR,"ssl set fd error" );
                         return false;
                    }
                    SSL_set_connect_state(m_ssl);
              
                    bool isContinue = true;
                    Start_ssl_connect_timer();

                    while(isContinue)
                    {
                       if(ssl_connect_timeout)
                       {
                            if(m_ssl !=NULL)
                            {
                                SSL_free(m_ssl);
                            }
                            this->m_encrypted = false;
                            ssl_connect_timeout = 0;
                            break;
                        }
                        
                        isContinue = false;
                        if(SSL_connect(m_ssl) == -1)
                        {
                            int icode = -1;
                            int iret = SSL_get_error(m_ssl,icode);

                            if ((iret == SSL_ERROR_WANT_WRITE) || (iret == SSL_ERROR_WANT_READ))
                            {
                               isContinue = true;
                            }
                            else
                            {                       
                                Close_ssl_connect_timer();
                                this->m_encrypted = false;
                                NAS_PrintLog( LOG_ERROR,"ssl connection error:%d " , iret );
                               // return true;
                            }
                        }
                        else
                        {
                            Close_ssl_connect_timer();
                            NAS_PrintLog( LOG_INFO,"ssl connection success ");
                           // Show_Certs(m_ssl);
                        }
                    }
                    
                }
            }

            if(m_protocol == NETPROTOCOL_SCTP)
                 SetSctpSocketOption();

            if( false == UpdateIpInfo() )
            {
                NAS_PrintLog( LOG_ERROR, " CreateSocket  UpdateIpInfo fail ");
                DestroySocket();
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_UPDATEIPINFO_FAIL);
                return false;
            }
                        
            UpdateSctpInfo();
        }
    }
    else
    {
        if(SOCK_DGRAM == sockettype)
        {
            NAS_PrintLog( LOG_FATAL, " PlatformSocket::CreateSocket  SOCK_DGRAM == sockettype ");
        }

        if( false == SetSocketOption() )
        {
            DestroySocket();
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_CREATESOCKET_SETOPTION_FAIL);
            NAS_PrintLog( LOG_ERROR, " PlatformSocket::CreateSocket  SetSocketOption fail!! ");
            return false;
        }
        this->UpdateIpInfo();
    }

    NAS_PrintLog( LOG_FATAL," CreateSocket OK!! m_socketFd_%d ",m_socketFd);
    return true;
}


bool PlatformSocket::AcceptSocket( PlatformSocket* client )
{
    sockaddr_in clientAddress;
    socklen_t       clientAddressLen = sizeof(clientAddress);
    
//    NAS_PrintLog( LOG_ERROR," accept... " );
    int acceptResult = accept(this->m_socketFd,(sockaddr*)&clientAddress,&clientAddressLen);

    /* On success, these system calls return a nonnegative integer that is a
       descriptor for the accepted socket.  On error, -1 is returned, and errno is
       set appropriately. */
    if (acceptResult >= 0)
    {
        char ipString[256] = {0};

		client->m_sessionData = this->GetSessionData();
		
        if (NULL != inet_ntop(AF_INET,&(clientAddress.sin_addr),ipString,sizeof(clientAddress)))
        {
            client->m_sessionData.m_PeerIP = ipString;
            client->m_sessionData.m_PeerPort = ntohs( clientAddress.sin_port );
        }
        
        client->m_socketFd = acceptResult;
        client->SetSocketOption();
        client->UpdateSctpInfo();
    } 
    else
    {
        NAS_PrintLog( LOG_ERROR," accept error,errno:%d " , errno );
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_ACCEPTSOCKET_FAIL);
        return false;
    }
    
    NAS_PrintLog( LOG_FATAL," AcceptSocket:  loacal_%s:%d , peer_%s:%d ",client->m_sessionData.m_LocalIP.c_str(), client->m_sessionData.m_LocalPort, client->m_sessionData.m_PeerIP.c_str(), client->m_sessionData.m_PeerPort);

    if(this->m_encrypted)
    {
        client->m_encrypted = true;
    }
    if(this->m_Mixed_Mode)
    {
        client->m_Mixed_Mode = true;
    }
    return true;
}

bool PlatformSocket::DestroySocket()
{
    /* handle ASF PMAL socket seperately */
    if( NETPROTOCOL_ASFPMAL == m_protocol )
    {
        return true;
    }

    if(m_encrypted)
    {
        SSL_free (m_ssl);   
    }
    
    /* handle normal socket */
    if( m_socketFd >= 0 )
    {
        PlatformIO::GetInstance()->UnBindSocket(&m_socketFd);
        close( m_socketFd );
        this->m_socketFd = -1;
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_DESTROYSOCKET_FAIL);
        return false;
    }

    return true;
}

bool PlatformSocket::SetSctpSocketOption()
{
    struct sctp_paddrparams paddr_params;
    socklen_t len = sizeof(struct sctp_paddrparams);
    memset( (void *)&paddr_params, 0, sizeof(paddr_params) );
    int ret = getsockopt( m_socketFd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &paddr_params, &len ); 

    if( ret < 0 )
    {
        NAS_PrintLog( LOG_ERROR," get SCTP_PEER_ADDR_PARAMS error:%d " , errno );
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPOPT_GETSOCK_ADDR_FAIL);
        return false;
    }
    else
    {
        NAS_PrintLog( LOG_INFO," spp_hbinterval:%d,spp_pathmaxrxt:%d,spp_flags:%d " , paddr_params.spp_hbinterval,paddr_params.spp_pathmaxrxt,paddr_params.spp_flags);
        paddr_params.spp_hbinterval = 15000;
        paddr_params.spp_pathmaxrxt = 1;
        ret = setsockopt( m_socketFd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &paddr_params, len ); 
        if( ret < 0 )
        {
            NAS_PrintLog( LOG_ERROR," set SCTP_PEER_ADDR_PARAMS error:%d " , errno );
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPOPT_SETSOCK_ADDR_FAIL);
            return false;
        }
    }

    struct sctp_assocparams passoc_params;
    memset( (void *)&passoc_params, 0, sizeof(passoc_params) );
    len = sizeof(struct sctp_assocparams);
    ret = getsockopt( m_socketFd, SOL_SCTP, SCTP_ASSOCINFO, &passoc_params, &len ); 

    if( ret < 0 )
    {
        NAS_PrintLog( LOG_ERROR," get SCTP_ASSOCINFO error:%d " , errno );
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPOPT_GETSOCK_INFO_FAIL);
        return false;
    }
    else
    {
        NAS_PrintLog( LOG_INFO," sasoc_asocmaxrxt:%d " , passoc_params.sasoc_asocmaxrxt);        
        passoc_params.sasoc_asocmaxrxt = 1;
        ret = setsockopt( m_socketFd, SOL_SCTP, SCTP_ASSOCINFO, &passoc_params, len ); 
        if( ret < 0 )
        {
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPOPT_SETSOCK_INFO_FAIL);
            NAS_PrintLog( LOG_ERROR," set SCTP_ASSOCINFO error:%d " , errno );
            return false;
        }
    }
    return true;
}

bool PlatformSocket::SetSctpHBPara(int interval, int count)
{
    struct sctp_paddrparams paddr_params;
    socklen_t len = sizeof(struct sctp_paddrparams);
    memset( (void *)&paddr_params, 0, sizeof(paddr_params) );
    int ret = getsockopt( m_socketFd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &paddr_params, &len ); 

    if( ret < 0 )
    {
        NAS_PrintLog( LOG_INFO," get SCTP_PEER_ADDR_PARAMS error:%d " , errno );
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPHD_GETSOCK_ADDR_FAIL);
        return false;
    }
    else
    {
        NAS_PrintLog( LOG_INFO," spp_hbinterval:%d,spp_pathmaxrxt:%d,spp_flags:%d " , paddr_params.spp_hbinterval,paddr_params.spp_pathmaxrxt,paddr_params.spp_flags);
        paddr_params.spp_hbinterval = interval;
        paddr_params.spp_pathmaxrxt = 2;
        ret = setsockopt( m_socketFd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &paddr_params, len ); 
        if( ret < 0 )
        {
            NAS_PrintLog( LOG_INFO," set SCTP_PEER_ADDR_PARAMS error:%d " , errno );
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPHD_SETSOCK_ADDR_FAIL);
            return false;
        }
    }

    struct sctp_assocparams passoc_params;
    memset( (void *)&passoc_params, 0, sizeof(passoc_params) );
    len = sizeof(struct sctp_assocparams);
    ret = getsockopt( m_socketFd, SOL_SCTP, SCTP_ASSOCINFO, &passoc_params, &len ); 

    if( ret < 0 )
    {
        NAS_PrintLog( LOG_INFO,"get SCTP_ASSOCINFO error:%d\n" , errno );
        PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPHD_GETSOCK_INFO_FAIL);
        return false;
    }
    else
    {
        NAS_PrintLog( LOG_INFO,"sasoc_asocmaxrxt:%d\n" , passoc_params.sasoc_asocmaxrxt);        
        passoc_params.sasoc_asocmaxrxt = count;
        ret = setsockopt( m_socketFd, SOL_SCTP, SCTP_ASSOCINFO, &passoc_params, len ); 
        if( ret < 0 )
        {
            NAS_PrintLog( LOG_INFO,"set SCTP_ASSOCINFO error:%d\n" , errno );
            PS_CPlus(CM_NES, CMNES_ID_SOCKET_SETSCTPHD_SETSOCK_INFO_FAIL);
            return false;
        }
    }
    return true;
}





bool PlatformSocket::SetTcpHBPara(int alive , int idle, int cnt, int intv)
{  
    //int alive;
    /*int flag, idle, cnt, intv;  */




int iKeepAlive = 9; 
socklen_t iOptLen = sizeof(iKeepAlive); 
//getsockopt(m_socketFd,SOL_SOCKET,SO_KEEPALIVE,&iKeepAlive,&iOptLen);
//NAS_PrintLog( LOG_INFO,"\r\n Local_%s:%d  Peer_%s:%d   SetTcpHBPara: SO_KEEPALIVE :%d " ,m_sessionData.m_LocalIP.c_str(),m_sessionData.m_LocalPort,m_sessionData.m_PeerIP.c_str(),m_sessionData.m_PeerPort, iKeepAlive );


 
    /* Set: use keepalive on fd */  
    /* alive = 1;  */
    if (setsockopt(m_socketFd, SOL_SOCKET, SO_KEEPALIVE, &alive,  sizeof(alive)) != 0)  
    {  
        NAS_PrintLog( LOG_ERROR,"\r\n Set keepalive fail" );
        return false;
    }  


getsockopt(m_socketFd,SOL_SOCKET,SO_KEEPALIVE,&iKeepAlive,&iOptLen);
//NAS_PrintLog( LOG_INFO,"\r\n SetTcpHBPara: SO_KEEPALIVE :%d " , iKeepAlive );
NAS_PrintLog( LOG_FATAL," Local_%s:%d  Peer_%s:%d   SetTcpHBPara: SO_KEEPALIVE :%d " ,m_sessionData.m_LocalIP.c_str(),m_sessionData.m_LocalPort,m_sessionData.m_PeerIP.c_str(),m_sessionData.m_PeerPort, iKeepAlive );


 
    /* idle���������ݣ�����������ƣ����ͱ���� */  
    /*idle = 10;  */
    if (setsockopt (m_socketFd, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) != 0)  
    {  
        NAS_PrintLog( LOG_ERROR," Set keepalive idle fail " );
        return false;
    }  
 
    /* ���û���յ���Ӧ����intv���Ӻ��ط������ */  
    /*intv = 5;  */
    if (setsockopt (m_socketFd, SOL_TCP, TCP_KEEPINTVL, &intv, sizeof(intv)) != 0)  
    {  
        NAS_PrintLog( LOG_ERROR," Set keepalive intv fail " );
        return false;
    }  
 
    /* ����cnt��û�յ����������Ϊ����ʧЧ */  
    /*cnt = 3;  */
    if (setsockopt (m_socketFd, SOL_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt)) != 0)  
    {  
        NAS_PrintLog( LOG_ERROR," Set keepalive cnt fail " );
        return false;
    }  
 
    return true;
}




bool PlatformSocket::SetSocketOption()
{
    int nRecvBuf;
    int nSendBuf;    

    if(m_session->u32TcpMemSizes== 0)
    {
        nRecvBuf=8*1024*1024;
        nSendBuf=8*1024*1024;   
    }
    else
    {
        nRecvBuf = m_session->u32TcpMemSizes;
        nSendBuf = m_session->u32TcpMemSizes; 
    }
    
    setsockopt(m_socketFd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
    NAS_PrintLog( LOG_FATAL," network received socket buffer is :%d K " , nRecvBuf/1024 );
    
    int tmpRecvBuf = 0;
    socklen_t optlen1 = sizeof(tmpRecvBuf);
    getsockopt(m_socketFd,SOL_SOCKET,SO_RCVBUF,&tmpRecvBuf,&optlen1);
    NAS_PrintLog( LOG_FATAL," After setsockoption for received socket, network received socket buffer is :%d K " , tmpRecvBuf/1024 );

    setsockopt(m_socketFd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
    NAS_PrintLog( LOG_FATAL," network send socket buffer is :%d K" , nSendBuf/1024 );
    
    int tmpSendBuf = 0;    
    socklen_t optlen2 = sizeof(tmpSendBuf);
    getsockopt(m_socketFd,SOL_SOCKET,SO_SNDBUF,&tmpSendBuf,&optlen2);
    NAS_PrintLog( LOG_FATAL," After setsockoption for send socket, network send socket buffer is :%d K " , tmpSendBuf/1024 );

    if( m_protocol == NETPROTOCOL_SCTP )
    {
        struct sctp_event_subscribe events;
        memset( (void *)&events, 0, sizeof(events) );
        events.sctp_data_io_event = 1;
        
        setsockopt( m_socketFd, SOL_SCTP, SCTP_EVENTS,
                             (const void *)&events, sizeof(events) );

        struct sctp_initmsg initmsg;
        memset( &initmsg, 0, sizeof(initmsg) );
        initmsg.sinit_num_ostreams = m_sessionData.m_num_ostreams;
        initmsg.sinit_max_instreams = m_sessionData.m_max_instreams;
        initmsg.sinit_max_attempts = 1;
        int nRet = setsockopt( m_socketFd, IPPROTO_SCTP, SCTP_INITMSG, 
                         (const char*)&initmsg, sizeof(initmsg) );
        if( nRet < 0 )
        {
            NAS_PrintLog( LOG_ERROR," sctp setsockopt error:%d " , errno );
        }

        uint32_t flag = 1;
        nRet = setsockopt( m_socketFd , IPPROTO_SCTP , SCTP_NODELAY , (char*)&flag , sizeof(flag) );

        if( nRet < 0 )
        {
            NAS_PrintLog( LOG_INFO," sctp_nodelay error:%d " , errno );
        }
    }
    
    MakeSocketNonBlocking();
    
    return true;
}

bool PlatformSocket::BindIO( bool needRecv)
{
    bool bindResult = PlatformIO::GetInstance()->BindSocket( &m_socketFd , (void*)this );

    return bindResult;
}

void PlatformSocket::OnSend( uint32_t)
{
}

void PlatformSocket::OnRecv( uint32_t)
{
}

void PlatformSocket::OnAccept()
{

}

bool PlatformSocket::SslAccept()
{

}

void PlatformSocket::OnClose()
{
    if( 0 >= this->m_socketFd )
    {
        NAS_PrintLog(LOG_ERROR, " m_socketFd_%d , m_clientConn_%d  is invalid  ",this->m_socketFd,m_clientConn);
        return;
    }
    
    m_session->OnClose( m_clientConn );
}

bool PlatformSocket::Send( const uint8_t* , uint32_t, SndData* )
{
    return false;
}

int PlatformSocket::SendUdp( const uint8_t* data , uint32_t size , uint32_t dstaddr , uint16_t dstport)
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return -1;
    }
    sockaddr_in addr;
    addr.sin_family      = AF_INET;
    
    if( 0 == dstaddr )
    {
        addr.sin_addr.s_addr = inet_addr(m_sessionData.m_PeerIP.c_str());
        addr.sin_port        = htons(m_sessionData.m_PeerPort);
    }
    else
    {
        addr.sin_addr.s_addr = dstaddr;
        addr.sin_port = dstport; 
    }

    int sendResult = sendto( this->m_socketFd, data, size, 0, (sockaddr*)&addr, sizeof(sockaddr_in) );

    /* On success, these calls return the number of characters sent.  On error, -1
       is returned, and errno is set appropriately. */
    if( sendResult > 0 )
    {
        PS_CPlus(CM_COM, CMCOM_ID_SEND_UDP_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_SEND_UDP_SIZES_L, sendResult);
        if(PS_CGet(CM_COM, CMCOM_ID_SEND_UDP_SIZES_L) < sendResult)
        {
            PS_CPlus(CM_COM, CMCOM_ID_SEND_UDP_SIZES_H);
        }
        return sendResult;
    }
    else
    {
        int errorCode = errno;

        if( errorCode == EWOULDBLOCK ||
            errorCode == EINTR )
        {
            return 0;
        }

        NetlibSetError( NetlibError_SendError );
        NAS_PrintLog( LOG_ERROR," send error : %d , Local_%s:%d , Peer_%s:%d " , errorCode, 
            m_sessionData.m_LocalIP.c_str() , m_sessionData.m_LocalPort , m_sessionData.m_PeerIP.c_str() , m_sessionData.m_PeerPort);
        //DestroySocket();
        return -1;
    }
}

int PlatformSocket::SendSctp( const uint16_t streamID , const uint8_t* data , uint32_t size )
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return -1;
    }
    int sendResult = sctp_sendmsg( this->m_socketFd, (void *)data, size ,
                         NULL, 0, htonl(SCTP_PAYLOAD_PROTOCOL_ID_S1AP), 0, streamID, 0, 0 );

    /* On success, these calls return the number of characters sent.  On error, -1
    is returned, and errno is set appropriately. */
    if( sendResult > 0 )
    {
        PS_CPlus(CM_COM, CMCOM_ID_SEND_SCTP_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_SEND_SCTP_SIZES_L, sendResult);
        if(PS_CGet(CM_COM, CMCOM_ID_SEND_SCTP_SIZES_L) < sendResult)
        {
            PS_CPlus(CM_COM, CMCOM_ID_SEND_SCTP_SIZES_H);
        }
        return sendResult;
    }
    else
    {
        int errorCode = errno;

        if( errorCode == EWOULDBLOCK ||
            errorCode == EINTR )
        {
            NAS_PrintLog( LOG_ERROR," send block " );
            return 0;
        }

        NetlibSetError( NetlibError_SendError );
        NAS_PrintLog( LOG_ERROR," send error , %d " , errorCode );
        DestroySocket();
        return -1;
    }
}

int PlatformSocket::SendTcp( const uint8_t* data , uint32_t size )
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return -1;
    }
    int sendResult = send( this->m_socketFd, data, size, 0);

    /* On success, these calls return the number of characters sent.  On error, -1
    is returned, and errno is set appropriately. */
    if( sendResult > 0 )
    {
        PS_CPlus(CM_COM, CMCOM_ID_SEND_TCP_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_SEND_TCP_SIZES_L, sendResult);
        if(PS_CGet(CM_COM, CMCOM_ID_SEND_TCP_SIZES_L) < sendResult)
        {
            PS_CPlus(CM_COM, CMCOM_ID_SEND_TCP_SIZES_H);
        }
        return sendResult;
    }
    else
    {
        int errorCode = errno;

        if( errorCode == EWOULDBLOCK ||
            errorCode == EINTR )
        {
            NAS_PrintLog( LOG_ERROR," send block " );
            PS_CPlus(CM_NES, CMNES_ID_NETLIB_SEND_BLOCK_FAIL);
            return 0;
        }

        NetlibSetError( NetlibError_SendError );
        NAS_PrintLog( LOG_ERROR," send error : %d , Local_%s:%d , Peer_%s:%d " , errorCode, 
            m_sessionData.m_LocalIP.c_str() , m_sessionData.m_LocalPort , m_sessionData.m_PeerIP.c_str() , m_sessionData.m_PeerPort);
        DestroySocket();
        return -1;
    }
}


int PlatformSocket::SendSsl( const uint8_t* data , uint32_t size )
{
    if( 0 == this->m_socketFd || 0 == this->m_ssl)
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return -1;
    }
    int sendResult = SSL_write( m_ssl, data, size);
      //  printf("client send msg time : %d  %s  %d \n",pf_get_ticks_ms(),__FUNCTION__,__LINE__);

    int nRes = SSL_get_error(m_ssl, sendResult);
    
    switch(sendResult)
    {
        case -1:
        case 0:
        {
                if(nRes == SSL_ERROR_WANT_WRITE )
                {
                    return 0;
                   // continue;
                }
                else
                {
                    NAS_PrintLog(LOG_ERROR, " ssl write error sendResult:%d res:%d ",sendResult,nRes);
                    return -1;
                }
        }
        default:
            {
                return sendResult;
            }                
    
        }
}
int PlatformSocket::SendRaw( const uint8_t* data , uint32_t size , uint32_t dstaddr )
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return -1;
    }
    
    int sendResult;
    if( 0 != dstaddr )
    {
        sockaddr_in addr;

        addr.sin_family      = AF_INET;
        addr.sin_port        = 0;
        addr.sin_addr.s_addr = dstaddr;
        
         sendResult = sendto( this->m_socketFd, data, size, 0 , (sockaddr*)&addr , sizeof(addr) );
    }
    else
    {
        sendResult = send( this->m_socketFd , data , size , 0 );
    }

    /* On success, these calls return the number of characters sent.  On error, -1
    is returned, and errno is set appropriately. */
    if( sendResult > 0 )
    {
        PS_CPlus(CM_COM, CMCOM_ID_SEND_RAW_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_SEND_RAW_SIZES_L, sendResult);
        if(PS_CGet(CM_COM, CMCOM_ID_SEND_RAW_SIZES_L) < sendResult)
        {
            PS_CPlus(CM_COM, CMCOM_ID_SEND_RAW_SIZES_H);
        }
        return sendResult;
    }
    else
    {
        int errorCode = errno;

        if( errorCode == EWOULDBLOCK ||
            errorCode == EINTR )
        {
            NAS_PrintLog( LOG_ERROR,"send block\n" );
            return 0;
        }

        NetlibSetError( NetlibError_SendError );
        NAS_PrintLog( LOG_ERROR,"send error , %d\n" , errorCode );
        DestroySocket();
        return -1;
    }
}
bool PlatformSocket::Recv()
{
    return false;
}

bool PlatformSocket::RecvUdp()
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return false;
    }
    while( true )
    {
        sockaddr_in peerAddr;
        socklen_t   peerAddrLen = sizeof(sockaddr_in);

        int recvResult = recvfrom(this->m_socketFd,this->buffer,sizeof(this->buffer),0,(sockaddr*)&peerAddr,&peerAddrLen);

        /*These calls return the number of bytes received, or -1 if an error
        occurred.  The return value will be 0 when the peer has performed an
        orderly shutdown.*/

        switch (recvResult)
        {
            case -1:
            {
                if( errno != EAGAIN )
                {
                    NAS_PrintLog( LOG_ERROR," RecvUdp error:%d , Local_%s:%d , Peer_%s:%d " , errno 
                        , m_sessionData.m_LocalIP.c_str() , m_sessionData.m_LocalPort , m_sessionData.m_PeerIP.c_str() , m_sessionData.m_PeerPort);
                    PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVUDP_ERRNO_FAIL);
                    return false;
                }
                else
                {
                    return true;
                }
            }
            
            case 0:
            {
                if( errno != EAGAIN )
                {
                    NAS_PrintLog( LOG_ERROR," RecvUdp error:%d , Local_%s:%d , Peer_%s:%d " , errno 
                    , m_sessionData.m_LocalIP.c_str() , m_sessionData.m_LocalPort , m_sessionData.m_PeerIP.c_str() , m_sessionData.m_PeerPort);
                    PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVUDP_ERRNO_FAIL);
                    return false;
                }
                else
                {
                    return true;
                }
            }
            
            default:
            {
                char ipString[256] = {0};
                if (NULL == inet_ntop(AF_INET,&(peerAddr.sin_addr),ipString,sizeof(peerAddr)))
                {
                    
                }
                else
                {
                    this->m_sessionData.m_PeerIP.assign(ipString);

                    if(0 != ntohs(peerAddr.sin_port))
                    {
                        this->m_sessionData.m_PeerPort = ntohs(peerAddr.sin_port);
                    }
                    else
                    {
                        NAS_PrintLog( LOG_ERROR," Be careful, we find m_PeerPort is zero! And Peer ip:%s , port:%d " ,  m_sessionData.m_PeerIP.c_str() , m_sessionData.m_LocalPort );
                        NAS_PrintLog( LOG_ERROR," Context is: %02x %02x %02x %02x %02x %02x %02x %02x ",
                            this->buffer[0], this->buffer[1],this->buffer[2], this->buffer[3],
                            this->buffer[4], this->buffer[5],this->buffer[6], this->buffer[7]);
                        NAS_PrintLog( LOG_ERROR," Context is: %02x %02x %02x %02x %02x %02x %02x %02x ",
                            this->buffer[8], this->buffer[9],this->buffer[10], this->buffer[11],
                            this->buffer[12], this->buffer[13],this->buffer[14], this->buffer[15]);                                       
                   }

                }
                        
                PS_CPlus(CM_COM, CMCOM_ID_RECV_UDP_CNT);
                PS_CPlusV(CM_COM, CMCOM_ID_RECV_UDP_SIZES_L, recvResult);
                if(PS_CGet(CM_COM, CMCOM_ID_RECV_UDP_SIZES_L) < recvResult)
                {
                    PS_CPlus(CM_COM, CMCOM_ID_RECV_UDP_SIZES_H);
                }
                OnRecv( recvResult );
            }
            break;
        }
    
    }

    //return true;
}

bool PlatformSocket::RecvSctp()
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return false;
    }
    while( true )
    {
        int flags = 0;
        int recvResult = sctp_recvmsg(this->m_socketFd,(void*)this->buffer,sizeof(this->buffer),0 , 0 , 0 , &flags);

        /*These calls return the number of bytes received, or -1 if an error
        occurred.  The return value will be 0 when the peer has performed an
        orderly shutdown.*/
        

        switch (recvResult)
        {
        case -1:
        {
            if( errno != EAGAIN )
            {
                NAS_PrintLog( LOG_ERROR,"sctp_recvmsg error:%d\n" , errno );
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVSCTP_ERRNO_FAIL);
                return false;
            }
            else
            {
                return true;
            }
        }
        case 0:
            {
                NAS_PrintLog( LOG_ERROR,"sctp_recvmsg error:%d\n" , errno );
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVSCTP_ERRNO_FAIL);
                return false;
            }
        default:
            {
                PS_CPlus(CM_COM, CMCOM_ID_RECV_SCTP_CNT);
                PS_CPlusV(CM_COM, CMCOM_ID_RECV_SCTP_SIZES_L, recvResult);
                if(PS_CGet(CM_COM, CMCOM_ID_RECV_SCTP_SIZES_L) < recvResult)
                {
                    PS_CPlus(CM_COM, CMCOM_ID_RECV_SCTP_SIZES_H);
                }
                OnRecv( recvResult );
            }
            break;
        }
    }
    
    return true;
}

 

bool PlatformSocket::RecvTcp()
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return false;
    }
    while( true )
    {
        int recvResult = recv(this->m_socketFd,this->buffer,sizeof(this->buffer),0);

        /*These calls return the number of bytes received, or -1 if an error
        occurred.  The return value will be 0 when the peer has performed an
        orderly shutdown.*/

        switch (recvResult)
        {
        case -1:
        {
            if( errno != EAGAIN )
            {
                NAS_PrintLog( LOG_ERROR," m_socketFd_%d , RecvTcp error:%d , Local_%s:%d , Peer_%s:%d ",m_socketFd, errno , 
                    this->m_sessionData.m_LocalIP.c_str(),this->m_sessionData.m_LocalPort, 
                    this->m_sessionData.m_PeerIP.c_str(),this->m_sessionData.m_PeerPort );
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVTCP_ERRNO_FAIL);
                return false;
            }
            else
            {
                return true;
            }
        }
        case 0:
            {
                NAS_PrintLog( LOG_ERROR," m_socketFd_%d , RecvTcp error:%d , Local_%s:%d , Peer_%s:%d ",m_socketFd, errno , 
                    this->m_sessionData.m_LocalIP.c_str(),this->m_sessionData.m_LocalPort, 
                    this->m_sessionData.m_PeerIP.c_str(),this->m_sessionData.m_PeerPort);
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVTCP_ERRNO_FAIL);
                return false;
            }
        default:
            {
                PS_CPlus(CM_COM, CMCOM_ID_RECV_TCP_CNT);
                PS_CPlusV(CM_COM, CMCOM_ID_RECV_TCP_SIZES_L, recvResult);
                if(PS_CGet(CM_COM, CMCOM_ID_RECV_TCP_SIZES_L) < recvResult)
                {
                    PS_CPlus(CM_COM, CMCOM_ID_RECV_TCP_SIZES_H);
                }
                this->bufferDataLen = recvResult;
                OnRecv( recvResult);
            }
        }
    }

    //return true;
}

bool PlatformSocket::RecvSsl()
{
        int recvResult = 0;
        int ires = 0;
        while(true)
        {

            recvResult = SSL_read(m_ssl,this->buffer,sizeof(this->buffer));
            ires = SSL_get_error(m_ssl, recvResult);

          //  printf("recvResult:%d  ires: %d",recvResult,ires);
            switch(recvResult)
            {
                case -1:
                {
                        return true;
                }
                case 0:
                {
                        if(ires == SSL_ERROR_WANT_READ || ires == SSL_ERROR_SSL) // maybe handshake
                        {
                            return true;
                           // continue;
                        }
                        else
                        {
                            NAS_PrintLog(LOG_ERROR, " ssl read error recvResult:%d res:%d ",recvResult,ires);
                            return false;
                        }
                }
            default:
                {
                    PS_CPlus(CM_COM, CMCOM_ID_RECV_TCP_CNT);
                    PS_CPlusV(CM_COM, CMCOM_ID_RECV_TCP_SIZES_L, recvResult);
                    if(PS_CGet(CM_COM, CMCOM_ID_RECV_TCP_SIZES_L) < recvResult)
                    {
                        PS_CPlus(CM_COM, CMCOM_ID_RECV_TCP_SIZES_H);
                    }
                    this->bufferDataLen = recvResult;
                    OnRecv( recvResult);
                }            
            }
         
        }
}
bool PlatformSocket::RecvRaw()
{
    if( 0 == this->m_socketFd )
    {
        NetlibSetError( NetlibError_SocketInvalid );
        return false;
    }
    
    while( true )
    {
        int recvResult = recv(this->m_socketFd,this->buffer,sizeof(this->buffer),0);

        /*These calls return the number of bytes received, or -1 if an error
        occurred.  The return value will be 0 when the peer has performed an
        orderly shutdown.*/

        switch (recvResult)
        {
        case -1:
        {
            if( errno != EAGAIN )
            {
                NAS_PrintLog( LOG_ERROR,"RecvRaw error:%d\n" , errno );
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVRAW_ERRNO_FAIL);
                return false;
            }
            else
            {
                return true;
            }
        }
        
        case 0:
            {
                NAS_PrintLog( LOG_ERROR,"RecvRaw error:%d\n" , errno );
                PS_CPlus(CM_NES, CMNES_ID_SOCKET_RECVRAW_ERRNO_FAIL);
                return false;
            }
            
        default:
            {
                PS_CPlus(CM_COM, CMCOM_ID_RECV_RAW_CNT);
                PS_CPlusV(CM_COM, CMCOM_ID_RECV_RAW_SIZES_L, recvResult);
                if(PS_CGet(CM_COM, CMCOM_ID_RECV_RAW_SIZES_L) < recvResult)
                {
                    PS_CPlus(CM_COM, CMCOM_ID_RECV_RAW_SIZES_H);
                }
                OnRecv( recvResult );
            }
            break;
        }
    
    }

    //return true;
}

bool PlatformSocket::SslServerInit()
{
    NAS_PrintLog(LOG_INFO, " SSL SERVER INIT START ");
    
    SSL_METHOD *meth;
    SSL_CTX *ctx;
    CHAR acPathCa[256];
    CHAR acPathkey[256];
    CHAR acPathcert[256];

    sprintf(acPathCa, "%s%s", pf_get_root_path(), "/DR_APP/inf/cacert_chain.pem");
    sprintf(acPathkey, "%s%s", pf_get_root_path(),"/DR_APP/inf/cryptkey.pem");
    sprintf(acPathcert, "%s%s", pf_get_root_path(), "/DR_APP/inf/crypt.pem");
        
#if 0
    //SSL����ʼ��
    SSL_library_init();
    //��������SSL�㷨
    OpenSSL_add_ssl_algorithms ();
    //�������д�����Ϣ
    SSL_load_error_strings ();
#endif
    meth = (SSL_METHOD *)SSLv23_method();
    ctx = SSL_CTX_new (meth);
    if (NULL == ctx)
    {
        NAS_PrintLog(LOG_ERROR, " Err:ServerNetSession::Ssl Server Init Fail (ctx new fail) ");
        return false;
    }

    SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_load_verify_locations (ctx, acPathCa, NULL);

    //����֤���˽Կ
    if (0 == SSL_CTX_use_certificate_file (ctx, acPathcert, SSL_FILETYPE_PEM))
    {
           // ERR_print_errors_fp (stderr);
        NAS_PrintLog(LOG_ERROR, " Err:ServerNetSession::Ssl Server Init Fail (load cert fail)");
        return false;
    }
    if (0 == SSL_CTX_use_PrivateKey_file (ctx, acPathkey, SSL_FILETYPE_PEM))
    {
       // ERR_print_errors_fp (stderr);
        NAS_PrintLog(LOG_ERROR, " Err:ServerNetSession::Ssl Server Init Fail (load private key fail)");
        return false;
    }
    if (!SSL_CTX_check_private_key (ctx))
    {
         NAS_PrintLog(LOG_ERROR, " Err:ServerNetSession::Private key does not match the certificate public key");
         return false;
    }
    SSL_CTX_set_cipher_list (ctx, "AES128-SHA256");
    SSL_CTX_set_mode (ctx, SSL_MODE_AUTO_RETRY);
    
    m_ssl = SSL_new (ctx);
    if (NULL == m_ssl)
    {
        NAS_PrintLog(LOG_ERROR, " Err:ServerNetSession::Private key does not match the certificate public key");
        return false;    
    }
 
    return true;
}

bool PlatformSocket::SslClientInit()
{
    NAS_PrintLog(LOG_INFO, " ClientNetSession::SslClientInit Start ");

    SSL_METHOD *meth;
    SSL_CTX *ctx;
    int seed_int[100];
    CHAR acPathCa[256];
    CHAR acPathkey[256];
    CHAR acPathcert[256];

    sprintf(acPathCa, "%s%s", pf_get_root_path(), "/DR_APP/inf/cacert_chain.pem");
    sprintf(acPathkey, "%s%s", pf_get_root_path(),"/DR_APP/inf/cryptkey.pem");
    sprintf(acPathcert, "%s%s", pf_get_root_path(), "/DR_APP/inf/crypt.pem");

#if 0
    /* ��ʼ��OpenSSL */
    SSL_library_init();
    /*�����㷨�� */
    OpenSSL_add_ssl_algorithms ();
    /*���ش�������Ϣ */
    SSL_load_error_strings ();
#endif   
    /* ѡ��ỰЭ�� */
    meth = (SSL_METHOD *) SSLv23_client_method();
    /* �����Ự���� */
    ctx = SSL_CTX_new (meth);
    if (NULL == ctx)
    {
        NAS_PrintLog(LOG_ERROR, " ClientNetSession::SslClientInit ctx init fail ");
        return false;
    }
    /* �ƶ�֤����֤��ʽ */
    SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, NULL);
     
    /* ΪSSL�Ự����CA֤��*/
    SSL_CTX_load_verify_locations (ctx, acPathCa, NULL);

    /* ΪSSL�Ự�����û�֤�� */
    if (0 == SSL_CTX_use_certificate_file (ctx, acPathcert, SSL_FILETYPE_PEM))
    {
        NAS_PrintLog(LOG_INFO, " ClientNetSession::SslClientInit load cert fail ");
       // ERR_print_errors_fp (stderr);
        return false;
    }
    /* ΪSSL�Ự�����û�˽Կ */
    if (0 == SSL_CTX_use_PrivateKey_file (ctx, acPathkey, SSL_FILETYPE_PEM))
    {
        NAS_PrintLog(LOG_INFO, " ClientNetSession::SslClientInit load private key fail ");
       // ERR_print_errors_fp (stderr);
        return false;
    }
    /* ��֤˽Կ��֤���Ƿ���� */
    if (!SSL_CTX_check_private_key (ctx))
    {
        NAS_PrintLog(ERR,"Private key does not match the certificate public key\n");
        return false;
    }
 
    /* ��������� */
    srand ((unsigned) time (NULL));
    for (int i = 0; i < 100; i++)
    {
        seed_int[i] = rand ();
    }
    RAND_seed (seed_int, sizeof (seed_int));
    /* ָ������������ */
 //   SSL_CTX_set_cipher_list (ctx, "AES128-SHA256");
    SSL_CTX_set_mode (ctx, SSL_MODE_AUTO_RETRY);

    /* ����һ��SSL�׽���*/
    m_ssl = SSL_new (ctx);
    return true;
}



