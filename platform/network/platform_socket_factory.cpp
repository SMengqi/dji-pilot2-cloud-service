#define THIS_MODULE MODULE_NETWORK
#include "platform_socket_factory.h"
#include "cross_platform_socket.h"
#include "platform.h"


PlatformSocketFactory* PlatformSocketFactory::GetInstance()
{
    static PlatformSocketFactory __instance;
    return &__instance;
}

PlatformSocketFactory::PlatformSocketFactory()
{
    pad = 0;
}

PlatformSocketFactory::~PlatformSocketFactory()
{

}

PlatformSocket* PlatformSocketFactory::CreatePlatformSocket( INetSession* session , NetProtocol protocol,const SessionData& sessionData )
{
    PlatformSocket* platformSocket = NULL;

    bool isUdpServer =false;
    bool isTcpServer =false;
    bool isSctpServer = false;

    switch( protocol )
    {
    case NETPROTOCOL_UDP:
        if( sessionData.m_PeerPort == 0 )
        {
            isUdpServer = true;
        }
        
        {
            platformSocket = new UdpSocket( session , protocol , sessionData );
        }
        break;
    case NETPROTOCOL_ASFPMAL:
        {
            platformSocket = new AsfPmalSocket( session , protocol , sessionData );
        }
        break;
    case NETPROTOCOL_TCP:
        if( sessionData.m_PeerPort == 0 )
        {
            isTcpServer = true;
            platformSocket = new ServerSocket( session , protocol , sessionData );
        }
        else
        {
            platformSocket = new TcpSocket( session , protocol , sessionData );
        }
        break;
    case NETPROTOCOL_SCTP:
        {
            if( sessionData.m_PeerPort == 0 )
            {
                isSctpServer = true;
                platformSocket = new ServerSocket( session , protocol , sessionData );
            }
            else
            {
                platformSocket = new SctpSocket( session , protocol , sessionData );
            }
        }
        break;
    case NETPROTOCOL_RAWIP:
        {
            platformSocket = new RawIPSocket( session , protocol , sessionData );
        }
        break;
    }

    if( NULL != platformSocket)
    {

        bool ret = platformSocket->CreateSocket();

        if( false == ret )
        {
            delete platformSocket;
            NAS_PrintLog( LOG_ERROR, " Err: PlatformSocketFactory::CreatePlatformSocket  CreateSocket  fail! ");
            PS_CPlus(CM_NES, CMNES_ID_FACTORY_CREATE_SOCKET_FAIL);
            return NULL;
        }

        if( (true == isTcpServer) || (true == isSctpServer) || (true == isUdpServer) )
        {
            //printf("\r\n PlatformSocketFactory::CreatePlatformSocket  BindIO ");
            platformSocket->BindIO( false );
        }
    }


    return platformSocket;
}
