#define THIS_MODULE MODULE_NETWORK
/**********************************************************************************************//**
 * @file    platform_netsession_client.cpp
 *
 * @brief    
 **************************************************************************************************/
#include <platform.h>
#include <net_lib.h>
//#include <platform_netsession_client.h>

//#define MAX_DRC_INTERFACE_PDU_SIZE  4096


#ifdef GTEST_EN
extern U16 network_valid_port[32][2];
#endif


ClientNetSession::ClientNetSession()
{
    PF_MUTEX_INIT(&client_mutex);

    m_pNetsession = NULL;
    receive_func_call = NULL;
    disconnect_func_call = NULL;

    u32sendID = 0;
    u32receiveID = 0;
    u32receivecount = 0;
    pu8receivebuf = NULL;
    pu8halfpackbuf = NULL;
    u32halfpacklen = 0;
    u32protocol = NETPROTOCOL_INVALID;
    u32BufferSize = 0;
    u32TcpMemSizes = 0;
    s32findMsghead = 0;
}

ClientNetSession::~ClientNetSession()
{
    PF_MUTEX_LOCK(&client_mutex);

    if (NULL != m_pNetsession)
    {
        NetFactory::GetInstance()->DeleteNetConnection(m_pNetsession);
        m_pNetsession = NULL; 
    }
    receive_func_call = NULL;
    disconnect_func_call = NULL;

    if(pu8receivebuf != NULL)
    {
        pf_free(pu8receivebuf);
        pu8receivebuf = NULL;
    }

    if(pu8halfpackbuf != NULL)
    {
        pf_free(pu8halfpackbuf);
        pu8halfpackbuf = NULL;
        u32halfpacklen = 0;
    }

    PF_MUTEX_UNLOCK(&client_mutex);
}

#pragma warning(disable: 4100)



uint32_t ClientNetSession::CombineSubpacket(const uint8_t* pMessage)
{

    uint32_t packet_len = 0;
    uint32_t packet_num = 0;
    uint32_t packet_id = 0;
    uint32_t packet_offset = 0;

    memcpy(&packet_id, ( uint8_t*)pMessage + PACKET_ID , MEMBER_SIZE_BYTE);

    if(u32receiveID != packet_id)
    {
        if(pu8receivebuf != NULL)
        {
            pf_free(pu8receivebuf);
            pu8receivebuf = NULL;
            PS_CPlus(CM_NES, CMNES_ID_CLIENT_ONRECV_SIZE_FAIL);
        }
        
        memcpy(&packet_len, ( uint8_t*)pMessage + MSG_LENGTH, MEMBER_SIZE_BYTE);
        
        packet_len &=~ PACKET_ENABLE; 
        
        pu8receivebuf = ( uint8_t*)pf_malloc(packet_len + sizeof(stMsgHeader));
        if(pu8receivebuf != NULL)
        {
            u32receivecount = 0;
            u32receiveID = packet_id;
            memcpy(pu8receivebuf,pMessage,sizeof(stMsgHeader));
        }
        else
        {
            u32receivecount = 0;
            u32receiveID = 0;
            NAS_PrintLog(LOG_ERROR, " pf_malloc fail, pu8receivebuf size_%d ",packet_len + sizeof(stMsgHeader));
        }
    }

    
    if(pu8receivebuf != NULL)
    {
        memcpy(&packet_offset, ( uint8_t*)pMessage + SUBPACKET_OFFSET, MEMBER_SIZE_BYTE);
        memcpy(&packet_len, ( uint8_t*)pMessage + SUBPACKET_LEN, MEMBER_SIZE_BYTE);
        memcpy(&packet_num, ( uint8_t*)pMessage + SUBPACKET_TOTALNUM, MEMBER_SIZE_BYTE);
        
        memcpy(pu8receivebuf + sizeof(stMsgHeader) + packet_offset , pMessage + sizeof(PacketHead),packet_len);
        u32receivecount++;
    
        if(u32receivecount == packet_num)
        {
            u32receivecount = 0;
            return PACKET_SUCCESS;
        }
    }
    else
    {
        NAS_PrintLog(LOG_ERROR, " pu8receivebuf is NULL , u32receiveID_%d , packet_id_%d ",u32receiveID,packet_id);
    }

    return PACKET_RECEIVING;

}



void ClientNetSession::CheckPacket(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData)
{                                                                                                                                                                
                                                                                                                                                                 
    uint8_t* pu8data = ( uint8_t*)pMessage;                                                                                                                      
    uint32_t total_len = 0;                                                                                                                                      
    uint32_t packet_len = 0;                                                                                                                                     
    uint32_t issubpacket = 0;                                                                                                                                    
    uint32_t u32msgtype = 0;                                                                                                                                    
                                                                                                                                                                 
                                                                                                                                                                 
    if(u32halfpacklen > 0)                                                                                                                                       
    {                                                                                                                                                            
        if(u32halfpacklen <= MSG_LENGTH)                                                                                                                         
        {                                                                                                                                                        
            memcpy(&packet_len, ( uint8_t*)pMessage + MSG_LENGTH - u32halfpacklen , MEMBER_SIZE_BYTE);                                                           
        }                                                                                                                                                        
        else if(u32halfpacklen < (MSG_LENGTH + MEMBER_SIZE_BYTE))                                                                                                
        {                                                                                                                                                        
            memcpy(&packet_len, ( uint8_t*)pu8halfpackbuf + MSG_LENGTH, u32halfpacklen - MSG_LENGTH);                                                            
            memcpy(( uint8_t*)(&packet_len) + u32halfpacklen - MSG_LENGTH, ( uint8_t*)pMessage, MSG_LENGTH + MEMBER_SIZE_BYTE - u32halfpacklen);          
        }                                                                                                                                                        
        else                                                                                                                                                     
        {                                                                                                                                                        
            memcpy(&packet_len, ( uint8_t*)pu8halfpackbuf + MSG_LENGTH , MEMBER_SIZE_BYTE);                                                                      
        }                                                                                                                                                        
                                                                                                                                                                 
        if((packet_len&PACKET_ENABLE) == 0)                                                                                                                      
        {                                                                                                                                                        
            packet_len = packet_len + sizeof(stMsgHeader);                                                                                                       
            issubpacket = 0;                                                                                                                                     
        }                                                                                                                                                        
        else                                                                                                                                                     
        {                                                                                                                                                        
            issubpacket = 1;                                                                                                                                     
            if(u32halfpacklen <= SUBPACKET_LEN)                                                                                                                  
            {                                                                                                                                                    
                memcpy(&packet_len, ( uint8_t*)pMessage + SUBPACKET_LEN - u32halfpacklen , MEMBER_SIZE_BYTE);                                                    
            }                                                                                                                                                    
            else if(u32halfpacklen < (SUBPACKET_LEN + MEMBER_SIZE_BYTE))                                                                                         
            {                                                                                                                                                    
                memcpy(&packet_len, ( uint8_t*)pu8halfpackbuf + SUBPACKET_LEN, u32halfpacklen - SUBPACKET_LEN);                                                  
                memcpy(( uint8_t*)(&packet_len) + u32halfpacklen - SUBPACKET_LEN, ( uint8_t*)pMessage, SUBPACKET_LEN + MEMBER_SIZE_BYTE - u32halfpacklen);
            }                                                                                                                                                    
            else                                                                                                                                                 
            {                                                                                                                                                    
                memcpy(&packet_len, ( uint8_t*)pu8halfpackbuf + SUBPACKET_LEN , MEMBER_SIZE_BYTE);                                                               
            }                                                                                                                                                    
            packet_len += sizeof(PacketHead);                                                                                                                    
        }                                                                                                                                                        

        if(SINGLE_PACKET_LEN_MAX < packet_len)
        {
            SessionData clientsessionData;
            
            pf_free(pu8halfpackbuf);  
            pu8halfpackbuf = NULL;
        
            m_pNetsession->GetClientConnectionInfo(clientsessionData);

            PS_CPlus(CM_NES, CMNES_ID_CLIENT_PACKETLEN_MSGTYPE_ERR);
            
            NAS_PrintLog(LOG_ERROR, " halfpacketlen_%d , packetlen_%d , SINGLE_PACKET_LEN_MAX_%d , local_%s:%d , peer_%s:%d ",
                        u32halfpacklen,packet_len, SINGLE_PACKET_LEN_MAX,clientsessionData.m_LocalIP.c_str(),
                        clientsessionData.m_LocalPort,clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort);    

            u32halfpacklen = 0;                                                                                                                                      
            pu8data = ( uint8_t*)pMessage;                                                                                                                           
            total_len = 0;    
            s32findMsghead = 1;
        }
        else
        {
            s32findMsghead = 0;
            
            if(u32halfpacklen < sizeof(PacketHead))                                                                                                                  
            {                      
                while(1)
                {
                    pu8data = ( uint8_t*)pf_malloc(packet_len); 
                    if(pu8data == NULL)
                    {
                       pf_usleep(1000);
                    }
                    else
                    {
                       break;
                    }
                }
                memcpy(pu8data,pu8halfpackbuf,u32halfpacklen);                                                                                                       
                pf_free(pu8halfpackbuf);                                                                                                                             
                pu8halfpackbuf = pu8data;                                                                                                                            
            }                                                                                                                                                        
                                                                                                                                                                     
                                                                                                                                                                     
            if(packet_len > messageSize + u32halfpacklen)                                                                                                            
            {                                                                                                                                                        
                memcpy(pu8halfpackbuf + u32halfpacklen, ( uint8_t*)pMessage , messageSize);                                                                          
                u32halfpacklen += messageSize;                                                                                                                       
                NAS_PrintLog(LOG_WARNING, " Client::OnRecv  halfhalf_packet: len_%d , messageSize_%d , total_len_%d ", packet_len, messageSize,total_len);    
                return;                                                                                                                                              
            }                                                                                                                                                        
                                                                                                                                                                     
            memcpy(pu8halfpackbuf + u32halfpacklen, ( uint8_t*)pMessage , packet_len - u32halfpacklen);                                                              
                                                                                                                                                                     
            if(issubpacket == 0)                                                                                                                                     
            {                                                                                                                                                        
                receive_func_call( conn, ( uint8_t*) pu8halfpackbuf, packet_len,sessionData);                                                                        
            }                                                                                                                                                        
            else if(PACKET_SUCCESS == CombineSubpacket(pu8halfpackbuf))                                                                                              
            {                                                                                                                                                        
                stMsgHeader* pMsgHead = (stMsgHeader*)pu8receivebuf;                                                                                                 
                pMsgHead->ulMsgLength &=~ PACKET_ENABLE;                                                                                                             
                receive_func_call( conn, ( uint8_t*) pu8receivebuf, pMsgHead->ulMsgLength + sizeof(stMsgHeader),sessionData);                                        
                u32receivecount = 0;                                                                                                                                 
                u32receiveID = 0;                                                                                                                                    
                pf_free(pu8receivebuf);                                                                                                                              
                pu8receivebuf = NULL;                                                                                                                                
            }                                                                                                                                                        
                                                                                                                                                                     
            total_len = packet_len - u32halfpacklen;                                                                                                                 
            pu8data = ( uint8_t*)pMessage + total_len;                                                                                                               
                                                                                                                                                                     
            u32halfpacklen = 0;                                                                                                                                      
            pf_free(pu8halfpackbuf);  
            pu8halfpackbuf = NULL;
        }
    }                                                                                                                                                            
    else                                                                                                                                                         
    {                                                                                                                                                            
        pu8data = ( uint8_t*)pMessage;                                                                                                                           
    }                                                                                                                                                            
                                                                                                                                                                 
    while(1)                                                                                                                                                     
    {                                                                                                                                                            
        if(messageSize == total_len)                                                                                                                             
        {                                                                                                                                                        
            return;                                                                                                                                              
        }                                                                                                                                                        
        else if((MSG_LENGTH + MEMBER_SIZE_BYTE) > (messageSize - total_len))                                                                                     
        {                                                                                                                                                        
            packet_len = 0;                                                                                                                                      
            u32msgtype = 0;
        }                                                                                                                                                        
        else                                                                                                                                                     
        {             
            memcpy(&packet_len, pu8data + MSG_LENGTH , MEMBER_SIZE_BYTE);   
            
            if((MSG_TYPE + MEMBER_SIZE_BYTE) > (messageSize - total_len))
            {
                u32msgtype = 0;
            }
            else
            {
                memcpy(&u32msgtype, pu8data + MSG_TYPE , MEMBER_SIZE_BYTE);                                                                                        
            }
                                                                                                                                                                 
            if((packet_len&PACKET_ENABLE) == 0)                                                                                                                  
            {                                                                                                                                                    
                packet_len +=  sizeof(stMsgHeader);                                                                                                              
                issubpacket = 0;                                                                                                                                 
            }                                                                                                                                                    
            else                                                                                                                                                 
            {                                                                                                                                                    
                issubpacket = 1;                                                                                                                                 
                if( (SUBPACKET_LEN + MEMBER_SIZE_BYTE) > (messageSize - total_len))                                                                              
                {                                                                                                                                                
                    packet_len = 0;                                                                                                                              
                }                                                                                                                                                
                else                                                                                                                                             
                {                                                                                                                                                
                    memcpy(&packet_len, pu8data+SUBPACKET_LEN, MEMBER_SIZE_BYTE);                                                                                                          
                    packet_len += sizeof(PacketHead);                                                                                                            
                }                                                                                                                                                
            }                                                                                                                                                    
        }                                                                                                                                                        

        if((packet_len == 0)||(packet_len > (messageSize - total_len)))                                                                                          
        {                                                                                                                                                        
            if( u32protocol == NETPROTOCOL_UDP)                                                                                                                  
            {                                                                                                                                                    
                NAS_PrintLog(LOG_WARNING, " UDP_Client::OnRecv  half_packet: len_%d , messageSize_%d , total_len_%d ", packet_len, messageSize,total_len);
                return;                                                                                                                                          
            }                                                                                                                                                    
                                                                                                                                                                 
            if((SINGLE_PACKET_LEN_MAX < packet_len)||(u32msgtype >= LAST_EVENT)||
                ((s32findMsghead == 1)&&((MSG_TYPE + MEMBER_SIZE_BYTE) > (messageSize - total_len))))
            {
                SessionData clientsessionData;
            
                s32findMsghead = 1;
                
                m_pNetsession->GetClientConnectionInfo(clientsessionData);

                PS_CPlus(CM_NES, CMNES_ID_CLIENT_PACKETLEN_MSGTYPE_ERR);
                
                NAS_PrintLog(LOG_ERROR, " messageSize_%d , total_len_%d , packetlen_%d , SINGLE_PACKET_LEN_MAX_%d , u32msgtype_%d , LAST_EVENT_%d , local_%s:%d , peer_%s:%d ",
                            messageSize,total_len,packet_len, SINGLE_PACKET_LEN_MAX,u32msgtype,LAST_EVENT,
                            clientsessionData.m_LocalIP.c_str(), clientsessionData.m_LocalPort,
                            clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort);    
                return;
            }
            
            s32findMsghead = 0;
            
            u32halfpacklen = messageSize - total_len;     

            while(1)
            {
                if(packet_len == 0)                                                                                                                                  
                {                                                                                                                                                    
                    pu8halfpackbuf = ( uint8_t*)pf_malloc(u32halfpacklen);                                                                                           
                }                                                                                                                                                    
                else                                                                                                                                                 
                {                                                                                                                                                    
                    pu8halfpackbuf = ( uint8_t*)pf_malloc(packet_len);                                                                                               
                }                                                                                                                                                    

                if(pu8halfpackbuf == NULL)
                {
                    pf_usleep(1000);
                }
                else
                {
                    break;
                }
            }
            
            memcpy(pu8halfpackbuf,pu8data,u32halfpacklen);                                                                                                       
//            NAS_PrintLog(LOG_WARNING, " Client::OnRecv  half_packet: len_%d , messageSize_%d , total_len_%d  ", packet_len, messageSize,total_len);        
            return;                                                                                                                                              
        }                                                                                                                                                        
                                                                                                                                                                 
        s32findMsghead = 0;
                                                                                                                                                                 
        if(issubpacket == 0)                                                                                                                                     
        {                                                                                                                                                        
            receive_func_call( conn, ( uint8_t*) pu8data, packet_len,sessionData);                                                                               
        }                                                                                                                                                        
        else if(PACKET_SUCCESS == CombineSubpacket(pu8data))                                                                                                     
        {                                                                                                                                                        
            stMsgHeader* pMsgHead = (stMsgHeader*)pu8receivebuf;                                                                                                 
            pMsgHead->ulMsgLength &=~ PACKET_ENABLE;                                                                                                             
            receive_func_call( conn, ( uint8_t*) pu8receivebuf, pMsgHead->ulMsgLength + sizeof(stMsgHeader),sessionData);                                        
            u32receivecount = 0;                                                                                                                                 
            u32receiveID = 0;                                                                                                                                    
            pf_free(pu8receivebuf);                                                                                                                              
            pu8receivebuf = NULL;                                                                                                                                
        }                                                                                                                                                        
                                                                                                                                                                 
        total_len += packet_len;                                                                                                                                 
        pu8data += packet_len;                                                                                                                                   
    }                                                                                                                                                            
                                                                                                                                                                 
}     





void ClientNetSession::OnRecv(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData)
{
    PF_MUTEX_LOCK(&client_mutex);

    if(NULL != receive_func_call)
    {
        if(conn != m_pNetsession)
        {
            NAS_PrintLog(LOG_ERROR, " ClientNetSession::OnRecv   conn_%lx  !=  m_pNetsession_%lx  ",conn , m_pNetsession);
        }
        
		if(SESSION_MODE_PASS_THROUGH == u32sessionmode)
		{
			receive_func_call( m_pNetsession,  ( uint8_t*)pMessage, messageSize, sessionData);
		}
		else
		{
			CheckPacket(m_pNetsession, pMessage, messageSize, sessionData);
		}
    }

	
    PF_MUTEX_UNLOCK(&client_mutex);
}

void ClientNetSession::OnClose(IClientConnection* conn)
{
    PF_MUTEX_LOCK(&client_mutex);

    if(conn != m_pNetsession)
    {
        NAS_PrintLog(LOG_FATAL, " ClientNetSession::OnClose   conn_%lx  !=  m_pNetsession_%lx  ",conn , m_pNetsession);
        PF_MUTEX_UNLOCK(&client_mutex);
        return;
    }

    if(NULL != disconnect_func_call)
    {
        disconnect_func_call(m_pNetsession);
    }
    else
    {
        NAS_PrintLog(LOG_ERROR, " ClientNetSession::OnClose disconnect_func_call is NULL. ");
    }

    if(m_pNetsession != NULL)
    {
        SessionData clientsessionData;

        m_pNetsession->GetClientConnectionInfo(clientsessionData);
        
        NAS_PrintLog(LOG_FATAL, " ClientNetSession:: OnClose, local_%s:%d , peer_%s:%d , m_pNetsession_%lx ",
                    clientsessionData.m_LocalIP.c_str(), clientsessionData.m_LocalPort,
                    clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort,m_pNetsession);
    }
    PF_MUTEX_UNLOCK(&client_mutex);
	
}


#pragma warning(default: 4100)

bool ClientNetSession::InitNetSession( NetProtocol protocol , const SessionData& sessionData , func_receive receive_func, func_disconnect disconnect_func)
{
    PF_MUTEX_LOCK(&client_mutex);

    if(pu8receivebuf != NULL)
    {
        pf_free(pu8receivebuf);
        pu8receivebuf = NULL;
    }

    if(pu8halfpackbuf != NULL)
    {
        pf_free(pu8halfpackbuf);
        pu8halfpackbuf = NULL;
        u32halfpacklen = 0;
    }

    u32sendID = 0;
    u32receiveID = 0;
    u32receivecount = 0;
//    pu8receivebuf = NULL;
//    pu8halfpackbuf = NULL;
    u32halfpacklen = 0;
    u32protocol = protocol;
	u32sessionmode = SESSION_MODE_PACKET_HEAD;
    

#ifdef GTEST_EN
    int i=0;
    isvalid = false;
    
    while((i<32)&&(( 0 != network_valid_port[i][0])||( 0 != network_valid_port[i][1])))
    {
 
        if(( sessionData.m_LocalPort == network_valid_port[i][0])
            &&( sessionData.m_PeerPort == network_valid_port[i][1]))
        {
            isvalid = true;
            break;
        }
        i++;
    }

    if(isvalid != true)
    {
        NAS_PrintLog(LOG_WARNING, " ClientNetSession::InitNetSession   LocalPort:%d  PeerPort:%d  invalid for gtest ",
                   sessionData.m_LocalPort,sessionData.m_PeerPort);
		
		PF_MUTEX_UNLOCK(&client_mutex);
        return true;
    }

#endif

    if( m_pNetsession != NULL )
    {
        NAS_PrintLog(LOG_ERROR, " ClientNetSession::InitNetSession  m_pNetsession is not NULL ");
        NetFactory::GetInstance()->DeleteNetConnection(m_pNetsession);
        m_pNetsession = NULL; 
    }
    else
    {
//        NAS_PrintLog(LOG_ERROR, " ClientNetSession::InitNetSession  m_pNetsession is NULL ");
    }

    switch(protocol)
    {
        case NETPROTOCOL_UDP:
            m_pNetsession = NetFactory::GetInstance()->CreateUdpConnection( this , sessionData );
            break;

        case NETPROTOCOL_TCP:
            m_pNetsession = NetFactory::GetInstance()->CreateTcpConnection( this , sessionData );
            break;

        default:
            PS_CPlus(CM_NES, CMNES_ID_CLIENT_INIT_SESSION_PROTOCOL_FAIL);
            NAS_PrintLog(LOG_ERROR, " ClientNetSession::InitNetSession  the protocol_%d  is invalid ",protocol);

			PF_MUTEX_UNLOCK(&client_mutex);
            return false;
    }

    if( m_pNetsession == NULL )
    {
        PS_CPlus(CM_NES, CMNES_ID_CLIENT_INIT_SESSION_NULL_FAIL);
        NAS_PrintLog(LOG_ERROR, " ClientNetSession::InitNetSession fail!  Local_%s:%d  Peer_%s:%d  ",
                   sessionData.m_LocalIP.c_str(),sessionData.m_LocalPort,sessionData.m_PeerIP.c_str(),sessionData.m_PeerPort);

		PF_MUTEX_UNLOCK(&client_mutex);
        return false;
    }

    receive_func_call = receive_func;
    disconnect_func_call = disconnect_func;

    SessionData  clientsessionData;
    m_pNetsession->GetClientConnectionInfo(clientsessionData);
    NAS_PrintLog(LOG_FATAL, " ClientNetSession::InitNetSession OK!  Local_%s:%d , Peer_%s:%d  ,  m_pNetsession_%lx ",
               clientsessionData.m_LocalIP.c_str(),clientsessionData.m_LocalPort,clientsessionData.m_PeerIP.c_str(),clientsessionData.m_PeerPort,m_pNetsession);
	
    PF_MUTEX_UNLOCK(&client_mutex);
    return true;

}




bool ClientNetSession::Netsession_isvalid(void)
{
    if( m_pNetsession != NULL )
    {
        return true;
    }
    else
    {
        return false;
    }
}





bool ClientNetSession::Send( void* data , uint32_t size  )
{

#ifdef GTEST_EN

    if(isvalid != true)
    {
        uint8_t * pMollcData = (uint8_t*)pl_malloc(size);
        
        pl_memcpy(pMollcData,data,size);

        pf_copy_msg(MODULE_NETWORK, GTEST_SOCKET_MSG_IND , MODULE_GTEST, pMollcData, size);

        pl_free(pMollcData);
        
        return true;
    }
#endif

    PF_MUTEX_LOCK(&client_mutex);

    if (NULL != m_pNetsession)
    {
        bool ret = m_pNetsession->SendData( (const uint8_t*)data , size , NULL );
		
	    PF_MUTEX_UNLOCK(&client_mutex);
		return ret;
    }
    PS_CPlus(CM_NES, CMNES_ID_CLIENT_SEND_SESSION_FAIL);
	
    PF_MUTEX_UNLOCK(&client_mutex);
    return false;
    
}


bool ClientNetSession::Send( void* data , uint32_t size , uint32_t packet_max_len)
{

	if(u32sessionmode != SESSION_MODE_PACKET_HEAD)
	{
		NAS_PrintLog(LOG_ERROR, " ClientNetSession::Send   Current sessionmode is not SESSION_MODE_PACKET_HEAD ");
		return false;
	}


#ifdef GTEST_EN
    if(isvalid != true)
    {
        uint8_t * pMollcData = (uint8_t*)pl_malloc(size);
        
        pl_memcpy(pMollcData,data,size);

        pf_copy_msg(MODULE_NETWORK, GTEST_SOCKET_MSG_IND , MODULE_GTEST, pMollcData, size);

        pl_free(pMollcData);
        
        return true;
    }
#endif


    PF_MUTEX_LOCK(&client_mutex);

    if (NULL != m_pNetsession)
    {
        if(size > packet_max_len)
        {
            PacketHead stpackethead;
            uint8_t* pdata = NULL;
            uint32_t len = sizeof(PacketHead);

            if(packet_max_len <= len)
            { 
                NAS_PrintLog(LOG_ERROR, " ClientNetSession::Send   packet_max_len < %d ", len);
                PS_CPlus(CM_NES, CMNES_ID_CLIENT_SEND_PACKET_LEN_FAIL);
				
				PF_MUTEX_UNLOCK(&client_mutex);
                return false;
            }

            if(u32sendID < 0xFFFFFFF0)
            {
                u32sendID++;
            }
            else
            {
                u32sendID = 1;
            }

            stpackethead.packetID = u32sendID;
            stpackethead.packetIndex = 1;
            stpackethead.packetLen = packet_max_len - len;
            stpackethead.packetOffset = 0;
            stpackethead.totalPacketNum = ((size - sizeof(stMsgHeader) + stpackethead.packetLen -1)/stpackethead.packetLen);

            memcpy(&(stpackethead.msgHead),data,sizeof(stMsgHeader));
            stpackethead.msgHead.ulMsgLength |= PACKET_ENABLE;

            pdata = (uint8_t*)pf_malloc(packet_max_len);
            if(NULL == pdata)
            {
                NAS_PrintLog(LOG_ERROR, "pf_malloc fail, pdata is nullpointer %s %d", __FUNCTION__, __LINE__);
                return false;
            }
            
            memcpy(pdata,&stpackethead,len);
            memcpy(pdata+len,data+sizeof(stMsgHeader),stpackethead.packetLen);
                
            if( false == m_pNetsession->SendData( (const uint8_t*)pdata , packet_max_len , NULL ))
            {
                pf_free(pdata);
                PS_CPlus(CM_NES, CMNES_ID_CLIENT_SEND_PACKET_DATA_FAIL);
				
				PF_MUTEX_UNLOCK(&client_mutex);
                return false;
            }
            pf_free(pdata);

            pdata = (uint8_t*)data + sizeof(stMsgHeader) + stpackethead.packetLen - len;
            
            while(1)
            {
                stpackethead.packetIndex++;
                stpackethead.packetOffset += stpackethead.packetLen;

                if(stpackethead.packetIndex == stpackethead.totalPacketNum)
                {
                    stpackethead.packetLen = size - stpackethead.packetOffset - sizeof(stMsgHeader);
                    memcpy(pdata,&stpackethead,len);
                    bool ret = m_pNetsession->SendData( (const uint8_t*)pdata , stpackethead.packetLen + len , NULL );

					PF_MUTEX_UNLOCK(&client_mutex);
					return ret;
                }

                memcpy(pdata,&stpackethead,len);
                if( false == m_pNetsession->SendData( (const uint8_t*)pdata , packet_max_len , NULL ))
                {
                    PS_CPlus(CM_NES, CMNES_ID_CLIENT_SEND_PACKET_DATA_FAIL);
					
					PF_MUTEX_UNLOCK(&client_mutex);
                    return false;
                }
                
                pdata = pdata + stpackethead.packetLen; 
            }
        }
        else
        {
            bool ret = m_pNetsession->SendData( (const uint8_t*)data , size , NULL );
			
			PF_MUTEX_UNLOCK(&client_mutex);
			return ret;
        }
    }
	
    PF_MUTEX_UNLOCK(&client_mutex);
    return false;
    
}


void ClientNetSession::DisConnect(void)
{
    PF_MUTEX_LOCK(&client_mutex);

    if (NULL != m_pNetsession)
    {

        SessionData clientsessionData;
    
        m_pNetsession->GetClientConnectionInfo(clientsessionData);
        NAS_PrintLog(LOG_FATAL, " ClientNetSession:: DisConnect, local_%s:%d , peer_%s:%d , m_pNetsession_%lx ",
                    clientsessionData.m_LocalIP.c_str(), clientsessionData.m_LocalPort,
                    clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort,m_pNetsession);
    
        NetFactory::GetInstance()->DeleteNetConnection(m_pNetsession);
        m_pNetsession = NULL; 
    }
    else
    {
        PS_CPlus(CM_NES, CMNES_ID_CLIENT_DISCONNECT_SESSION_FAIL);
        NAS_PrintLog(LOG_FATAL, " ClientNetSession::DisConnect  m_pNetsession is NULL ");
    }



    if(pu8receivebuf != NULL)
    {
        pf_free(pu8receivebuf);
        pu8receivebuf = NULL;
    }

    if(pu8halfpackbuf != NULL)
    {
        pf_free(pu8halfpackbuf);
        pu8halfpackbuf = NULL;
        u32halfpacklen = 0;
    }

    u32sendID = 0;
    u32receiveID = 0;
    u32receivecount = 0;
	
    PF_MUTEX_UNLOCK(&client_mutex);

   
}


bool ClientNetSession::SetTcpHBPara( int heartbeat_en, int idle, int cnt, int intv)
{
    PF_MUTEX_LOCK(&client_mutex);
    if (NULL != m_pNetsession)
    {
        SessionData clientsessionData;
        m_pNetsession->GetClientConnectionInfo(clientsessionData);
    
        NAS_PrintLog(LOG_FATAL, " ClientNetSession:: SetTcpHBPara,  local_%s:%d   heartbeat_en:%d , idle:%d, cnt:%d, intv:%d ",
            clientsessionData.m_LocalIP.c_str(),clientsessionData.m_LocalPort,heartbeat_en,idle,cnt,intv);

		bool ret = m_pNetsession->SetTcpHBPara( heartbeat_en, idle, cnt, intv);

	    PF_MUTEX_UNLOCK(&client_mutex);
		return ret;
    }
    
    PS_CPlus(CM_NES, CMNES_ID_CLIENT_SETTCPHBPARA_SESSION_FAIL);
    NAS_PrintLog(LOG_ERROR, " ClientNetSession::SetTcpHBPara  m_pNetsession is NULL ");
	
    PF_MUTEX_UNLOCK(&client_mutex);
    return false;
    
}



void ClientNetSession::SetSessionMode( SessionMode mode )
{
	u32sessionmode = mode;
}

void ClientNetSession::SetBufferSize( int nSize)
{
    u32BufferSize = nSize;
}

void ClientNetSession::SetTcpMemSize( int nSize)
{
    u32TcpMemSizes = nSize;
}




