#define THIS_MODULE MODULE_NETWORK
#include "sctp_test.h"
#include "scoped_lock.h"
#include "platform_mutex.h"
//#include "sctp_s1ap_if.h"
#include "event.h"
#include "net_define.h"
#include "platform_net_tools.h"
#include "platform.h"

#ifdef ALL_IN_ONE
#include "sctp_s1ap_interface.h"


CWinSctpServer g_WinSctpServer;

bool CServerClientConntion::SendData( const uint8_t* data , uint32_t size , SndData* param  )
{
    return g_WinSctpServer.SendDataToClient( data , size );
}

void CServerClientConntion::Close()
{
    g_WinSctpServer.CloseSctpConnection( );
}

bool CClientConntion::GetClientConnectionInfo( SessionData& sessionData )
{
    sessionData.m_LocalIP = "127.0.0.1";
    sessionData.m_LocalPort = 1000;
    return true;
}

bool CClientConntion::SetSctpHBPara( int32_t interval , int32_t count )
{
    return true;
}

bool CServerClientConntion::SetSctpHBPara( int32_t interval , int32_t count )
{
    return true;
}

bool CClientConntion::SendData( const uint8_t* data , uint32_t size , SndData* param  )
{
    return g_WinSctpServer.SendDataToServer( data , size );
}

void CClientConntion::Close()
{
    g_WinSctpServer.CloseSctpConnection();
}

IClientConnection* CWinSctpServer::CreateSctpConnection()
{
    ScopedLock<PlatformMutex> lock(m_mutex);
    if( m_main_ip.empty() )
    {
        m_main_ip = ::GetMainIP();
    }
    SCTP_S1AP_NEW_ENB_IND_MSG SctpS1apNewEnbInd;

    SctpS1apNewEnbInd.m_pClientConnection = &m_server;
    NAS_CopyMessage(MODULE_SCTP, SCTP_S1AP_NEW_ENB_IND, MODULE_MMES1AP, &SctpS1apNewEnbInd, sizeof(SCTP_S1AP_NEW_ENB_IND_MSG));

    return &m_client;
}

#if 0
bool CWinSctpServer::SendDataToServer( const uint8_t* data , uint32_t size )
{
    SCTP_S1AP_DATA_IND_MSG SctpS1apDataIndMessage;

    SctpS1apDataIndMessage.m_pClientConnection            = &m_server;
    SctpS1apDataIndMessage.m_S1MessagePdu.m_PduLength    = size;
    NAS_MemoryCopy(SctpS1apDataIndMessage.m_S1MessagePdu.m_PduBuf, (void *)data, size );

    NAS_CopyMessage(MODULE_SCTP, SCTP_S1AP_DATA_IND, MODULE_MMES1AP, &SctpS1apDataIndMessage, sizeof(SCTP_S1AP_DATA_IND_MSG));
    return true;
}
#endif
bool CWinSctpServer::SendDataToServer( const uint8_t* data , uint32_t size )
{
    uint32_t totalSize = sizeof(SCTP_S1AP_DATA_IND_MSG) + size;
    SCTP_S1AP_DATA_IND_MSG *sctpS1apDataInd = (SCTP_S1AP_DATA_IND_MSG *)NAS_MemoryMalloc(totalSize);
    if(NULL == sctpS1apDataInd)
    {
        NAS_PrintLog(LOG_ERROR, "NAS_MemoryMalloc fail, sctpS1apDataInd is nullpointer %s %d", __FUNCTION__, __LINE__);
        return false;
    }
    
    sctpS1apDataInd->m_pClientConnection            = &m_server;
    sctpS1apDataInd->m_PduLength = size;
    NAS_MemoryCopy(sctpS1apDataInd->m_PduBuf, (uint8_t*)data, sctpS1apDataInd->m_PduLength);

    /*向MMES1AP发送SCTP_S1AP_DATA_IND消息*/
    NAS_CopyMessage(MODULE_SCTP, SCTP_S1AP_DATA_IND, MODULE_MMES1AP, sctpS1apDataInd, (uint16_t)totalSize);

    pf_free(sctpS1apDataInd);
    return true;

}

bool CWinSctpServer::SendDataToClient( const uint8_t* data , uint32_t size )
{
    uint32_t totalSize = sizeof(SCTP_S1AP_DATA_IND_S) + size;
    SCTP_S1AP_DATA_IND_S *sctpS1apDataInd = (SCTP_S1AP_DATA_IND_S *)NAS_MemoryMalloc(totalSize);
    if(NULL == sctpS1apDataInd)
    {
        NAS_PrintLog(LOG_ERROR, "NAS_MemoryMalloc fail, sctpS1apDataInd is nullpointer %s %d", __FUNCTION__, __LINE__);
        return false;
    }
    
    sctpS1apDataInd->stMmeIpAddress.usIpAddrLen = (uint16_t)m_main_ip.size();
    strcpy((char*)sctpS1apDataInd->stMmeIpAddress.aucIpAddr, (const char*)m_main_ip.c_str());

    sctpS1apDataInd->ulLength                    = size;
    NAS_MemoryCopy(sctpS1apDataInd->aucData, (uint8_t*)data, sctpS1apDataInd->ulLength);

    /*向S1AP发送SCTP_S1AP_DATA_IND消息*/
    NAS_CopyMessage(MODULE_SCTP, SCTP_S1AP_DATA_IND, MODULE_S1AP, sctpS1apDataInd, (uint16_t)totalSize);


    pf_free(sctpS1apDataInd);
    return true;
}

void CWinSctpServer::CloseSctpConnection( )
{
    SCTP_S1AP_ENB_DISCONNECTION_IND_MSG disconnectionInd;
    disconnectionInd.m_pClientConnection = &m_server;

    NAS_CopyMessage(MODULE_SCTP, SCTP_S1AP_ENB_DISCONNECTION_IND, MODULE_MMES1AP, &disconnectionInd, sizeof(disconnectionInd));

    //
    SCTP_S1AP_CLOSE_IND_S sctpS1apCloseInd;

    sctpS1apCloseInd.pConnection = &m_client;

    NAS_CopyMessage(MODULE_SCTP, SCTP_S1AP_CLOSE_IND, MODULE_S1AP, &sctpS1apCloseInd, sizeof(sctpS1apCloseInd));
}
#endif

