#define THIS_MODULE MODULE_NETWORK
/**********************************************************************************************//**
 * @file    platform_netsession_server.cpp
 *
 * @brief    
 **************************************************************************************************/
#include <platform.h>
#include <net_lib.h>
//#include <platform_netsession_server.h>
#include <net_manager.h>

//#define MAX_DRC_INTERFACE_PDU_SIZE  4096

#ifdef GTEST_EN
extern U16 network_valid_port[32][2];
#endif


ServerNetSession::ServerNetSession()
{
    PF_MUTEX_INIT(&server_mutex);
    pServerConnection = NULL;
    receive_func_call = NULL;
    accept_func_call = NULL;
    disconnect_func_call = NULL;
    server_udp_handle.Close();
    u32sendID = 0;
    u32clientCount = 0;
    memset(astClientInfo,0,sizeof(astClientInfo));
    u32protocol = NETPROTOCOL_INVALID;
    u32BufferSize = 0;
    u32TcpMemSizes = 0;
    s32findMsghead = 0;
}

ServerNetSession::~ServerNetSession()
{
    PF_MUTEX_LOCK(&server_mutex);

    if (NULL != pServerConnection)
    {
        NetFactory::GetInstance()->DeleteServerConnection(pServerConnection);
        pServerConnection = NULL; 
    }
    
    receive_func_call = NULL;
    accept_func_call = NULL;
    disconnect_func_call = NULL;

    if( true == server_udp_handle.IsValid() )
    {
        NetManager::GetInstance()->Close( server_udp_handle );
        server_udp_handle.Close();
    }


    uint32_t i = 0;

    for(i = 0; i<u32clientCount; i++)
    {
        if(astClientInfo[i].astClientRecv[0].pu8receivebuf != NULL)
        {
            pf_free(astClientInfo[i].astClientRecv[0].pu8receivebuf);

            astClientInfo[i].astClientRecv[0].S32count--;        
            astClientInfo[i].astClientRecv[0].pu8receivebuf = NULL;
        }
        
        if(astClientInfo[i].astClientRecv[1].pu8receivebuf != NULL)
        {
            pf_free(astClientInfo[i].astClientRecv[1].pu8receivebuf);

            astClientInfo[i].astClientRecv[1].S32count--;
            astClientInfo[i].astClientRecv[1].pu8receivebuf = NULL;
        }

        if(astClientInfo[i].pu8halfpackbuf != NULL)
        {
            pf_free(astClientInfo[i].pu8halfpackbuf);

            astClientInfo[i].S32halfcount--;
            astClientInfo[i].pu8halfpackbuf = NULL;
        }
        
    }
    
    PF_MUTEX_UNLOCK(&server_mutex);
}

#pragma warning(disable: 4100)


void ServerNetSession::OnAccept(IClientConnection* conn, const SessionData& sessionData)
{
    /*the received client connection pointer is NULL*/
    if (NULL == conn)
    {
        NAS_PrintLog(LOG_ERROR, " ServerNetSession::OnAccept  the received client connection pointer is NULL. ");
        PS_CPlus(CM_NES, CMNES_ID_SERVER_ONACCEPT_FAIL);
        return;
    }

    SessionData clientsessionData;

    conn->GetClientConnectionInfo(clientsessionData);
    NAS_PrintLog(LOG_FATAL, " ServerNetSession::OnAccept, local_%s:%d, peer_%s:%d , conn_%lx " ,
                clientsessionData.m_LocalIP.c_str(), clientsessionData.m_LocalPort,
                clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort,conn);

    PF_MUTEX_LOCK(&server_mutex);

    if(NULL != accept_func_call)
    {
        accept_func_call( conn, sessionData);
    }
	
    PF_MUTEX_UNLOCK(&server_mutex);
}





uint32_t ServerNetSession::CombineSubpacket(uint32_t client_index ,const uint8_t* pMessage, uint32_t* recv_index)
{

    uint32_t packet_len = 0;
    uint32_t packet_num = 0;
    uint32_t packet_id = 0;
    uint32_t packet_offset = 0;
    uint32_t index = 0;
    
    stClientInfo*  pastClientInfo = &(astClientInfo[client_index]);

    memcpy(&packet_id, ( uint8_t*)pMessage + PACKET_ID , MEMBER_SIZE_BYTE);

    if(pastClientInfo->astClientRecv[0].u32receiveID == packet_id)
    {
        index = 0;
    }
    else if(pastClientInfo->astClientRecv[1].u32receiveID == packet_id) 
    {
        index = 1;
    }
    else
    {
        if(pastClientInfo->astClientRecv[0].u32receiveID < pastClientInfo->astClientRecv[1].u32receiveID)
        {
            index = 0;
        }
        else
        {
            index = 1;
        }

            
        if(pastClientInfo->astClientRecv[index].pu8receivebuf != NULL)
        {
            pf_free(pastClientInfo->astClientRecv[index].pu8receivebuf);
            pastClientInfo->astClientRecv[index].S32count--;
            pastClientInfo->astClientRecv[index].pu8receivebuf = NULL;

            pastClientInfo->u32failcount++;
            
            float failrate = (float)(pastClientInfo->u32failcount*100)/(pastClientInfo->u32okcount + pastClientInfo->u32failcount);
            NAS_PrintLog(LOG_WARNING, " Server:: fail_rate: %f\% ,  OK_%d , fail_%d ,  client_%s:%d  , lost_%d ",failrate , pastClientInfo->u32okcount, pastClientInfo->u32failcount,
                        pastClientInfo->acClientIP , pastClientInfo->u16ClientPort, pastClientInfo->astClientRecv[index].u32receiveID - (pastClientInfo->u32okcount + pastClientInfo->u32failcount));
            NAS_PrintLog(LOG_WARNING, " Server:: fail_packet_id_%d , new_packet_id_%d " , pastClientInfo->astClientRecv[index].u32receiveID ,packet_id);
            PS_CPlus(CM_NES, CMNES_ID_SERVER_ONRECV_FAIL);
        }
            
        memcpy(&packet_len, ( uint8_t*)pMessage + MSG_LENGTH, MEMBER_SIZE_BYTE);
        
        packet_len &=~ PACKET_ENABLE; 

        pastClientInfo->astClientRecv[index].pu8receivebuf = ( uint8_t*)pf_malloc(packet_len + sizeof(stMsgHeader));
        if(pastClientInfo->astClientRecv[index].pu8receivebuf != NULL)
        {

            pastClientInfo->astClientRecv[index].S32count++;
            pastClientInfo->astClientRecv[index].S32len = packet_len + sizeof(stMsgHeader);

            pastClientInfo->astClientRecv[index].u32receivecount = 0;
            pastClientInfo->astClientRecv[index].u32receiveID = packet_id;
            memcpy(pastClientInfo->astClientRecv[index].pu8receivebuf,pMessage,sizeof(stMsgHeader));
        }
        else
        {
            pastClientInfo->astClientRecv[index].u32receivecount = 0;
            pastClientInfo->astClientRecv[index].u32receiveID = 0;
            NAS_PrintLog(LOG_ERROR, " pf_malloc fail, pastClientInfo[%d].astClientRecv[%d].pu8receivebuf , size_%d , pre_packet_id_%d , new_packet_id_%d ",
                                    client_index,index,packet_len + sizeof(stMsgHeader), pastClientInfo->astClientRecv[index].u32receiveID ,packet_id);
        }

    }
    
    if(pastClientInfo->astClientRecv[index].pu8receivebuf != NULL)
    {
        memcpy(&packet_offset, ( uint8_t*)pMessage + SUBPACKET_OFFSET, MEMBER_SIZE_BYTE);
        memcpy(&packet_len, ( uint8_t*)pMessage + SUBPACKET_LEN, MEMBER_SIZE_BYTE);
        memcpy(&packet_num, ( uint8_t*)pMessage + SUBPACKET_TOTALNUM, MEMBER_SIZE_BYTE);
        
        memcpy(pastClientInfo->astClientRecv[index].pu8receivebuf + sizeof(stMsgHeader) + packet_offset , pMessage + sizeof(PacketHead),packet_len);
        pastClientInfo->astClientRecv[index].u32receivecount++;
    
        if(pastClientInfo->astClientRecv[index].u32receivecount == packet_num)
        {
            pastClientInfo->astClientRecv[index].u32receivecount = 0;
            *recv_index = index;
            return PACKET_SUCCESS;
        }
    }
    else
    {
        NAS_PrintLog(LOG_ERROR, " pastClientInfo[%d].astClientRecv[%d].pu8receivebuf is NULL , u32receiveID0_%d , u32receiveID1_%d , packet_id_%d ",
                client_index,index,pastClientInfo->astClientRecv[0].u32receiveID,pastClientInfo->astClientRecv[1].u32receiveID,packet_id);
    }
    
    return PACKET_RECEIVING;


}



void ServerNetSession::mem_check(int client_i)
{
    U32 totalmemlen = 0;
    U32 totalmemlen2 = 0;


    
    int i = 0;

    for(i = 0; i<u32clientCount; i++)
    {
        if( astClientInfo[i].S32halfcount > 0)
        {
            totalmemlen += astClientInfo[i].S32halflen ;
//            256,  1024, 1536, 4096, 8192, 64000

            if(astClientInfo[i].S32halflen < 256)
            {
                totalmemlen2 += 256;
            }
            else if(astClientInfo[i].S32halflen < 1024)
            {
                totalmemlen2 += 1024;
            }
            else if(astClientInfo[i].S32halflen < 1536)
            {
                totalmemlen2 += 1536;
            }
            else if(astClientInfo[i].S32halflen < 4096)
            {
                totalmemlen2 += 4096;
            }
            else if(astClientInfo[i].S32halflen < 8192)
            {
                totalmemlen2 += 8192;
            }
            else if(astClientInfo[i].S32halflen < 64000)
            {
                totalmemlen2 += 64000;
            }
            else
            {
                totalmemlen2 += astClientInfo[i].S32halflen;
    
                pl_log(LOG_WARNING, "client_%d:  halfcount_%d , halflen_%d ",i, astClientInfo[i].S32halfcount, astClientInfo[i].S32halflen );    
            }


        }

        if( astClientInfo[i].astClientRecv[0].S32count > 0)
        {
            totalmemlen += astClientInfo[i].astClientRecv[0].S32len ;

            if(astClientInfo[i].astClientRecv[0].S32len < 256)
            {
                totalmemlen2 += 256;
            }
            else if(astClientInfo[i].astClientRecv[0].S32len < 1024)
            {
                totalmemlen2 += 1024;
            }
            else if(astClientInfo[i].astClientRecv[0].S32len < 1536)
            {
                totalmemlen2 += 1536;
            }
            else if(astClientInfo[i].astClientRecv[0].S32len < 4096)
            {
                totalmemlen2 += 4096;
            }
            else if(astClientInfo[i].astClientRecv[0].S32len < 8192)
            {
                totalmemlen2 += 8192;
            }
            else if(astClientInfo[i].astClientRecv[0].S32len < 64000)
            {
                totalmemlen2 += 64000;
            }
            else
            {
                totalmemlen2 += astClientInfo[i].astClientRecv[0].S32len;
                pl_log(LOG_WARNING, "client_%d:  recv0_count_%d , recv0_len_%d  ",i,astClientInfo[i].astClientRecv[0].S32count,astClientInfo[i].astClientRecv[0].S32len);    
            }

        }

        if( astClientInfo[i].astClientRecv[1].S32count > 0)
        {
            totalmemlen += astClientInfo[i].astClientRecv[1].S32len ;
            if(astClientInfo[i].astClientRecv[1].S32len < 256)
            {
                totalmemlen2 += 256;
            }
            else if(astClientInfo[i].astClientRecv[1].S32len < 1024)
            {
                totalmemlen2 += 1024;
            }
            else if(astClientInfo[i].astClientRecv[1].S32len < 1536)
            {
                totalmemlen2 += 1536;
            }
            else if(astClientInfo[i].astClientRecv[1].S32len < 4096)
            {
                totalmemlen2 += 4096;
            }
            else if(astClientInfo[i].astClientRecv[1].S32len < 8192)
            {
                totalmemlen2 += 8192;
            }
            else if(astClientInfo[i].astClientRecv[1].S32len < 64000)
            {
                totalmemlen2 += 64000;
            }
            else
            {
                totalmemlen2 += astClientInfo[i].astClientRecv[1].S32len;
                pl_log(LOG_WARNING, "client_%d:  recv1_count_%d , recv1_len_%d  ",i,astClientInfo[i].astClientRecv[1].S32count,astClientInfo[i].astClientRecv[1].S32len);    

            }

        }
    }

    i = client_i;

    pl_log(LOG_WARNING, "client_%d:  halfcount_%d , halflen_%d , recv0_count_%d , recv0_len_%d , recv1_count_%d , recv1_len_%d , total_len_%d , %d ",i, astClientInfo[i].S32halfcount, astClientInfo[i].S32halflen,
    astClientInfo[i].astClientRecv[0].S32count,astClientInfo[i].astClientRecv[0].S32len,astClientInfo[i].astClientRecv[1].S32count,astClientInfo[i].astClientRecv[1].S32len,totalmemlen,totalmemlen2);    
    
    return ;
}





void ServerNetSession::CheckPacket(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData)
{                                                                                                                                                                
                                                                                                                                                                 
    uint8_t* pu8data = ( uint8_t*)pMessage;                                                                                                                      
    uint32_t total_len = 0;                                                                                                                                      
    uint32_t packet_len = 0;                                                                                                                                     
    uint32_t issubpacket = 0;   
    
    uint32_t i = 0;
    uint32_t recv_i = 0;
    uint32_t u32msgtype = 0;                                                                                                                                    

    for(i = 0; i<u32clientCount; i++)
    {
        if(( astClientInfo[i].u16ClientPort == sessionData.m_PeerPort)
            &&( 0 == strcmp( astClientInfo[i].acClientIP ,sessionData.m_PeerIP.c_str())))
        {
            break;
        }
    }

    if(i == u32clientCount)
    {
        if(u32clientCount >= MAX_CLIENT_NUM)
        {
            NAS_PrintLog(LOG_ERROR, " Server::CheckPacket  client count is %d  , can not add new client ", u32clientCount);
            return;
        }
        astClientInfo[i].u16ClientPort = sessionData.m_PeerPort;
        strcpy(astClientInfo[i].acClientIP ,sessionData.m_PeerIP.c_str());
        u32clientCount++;

        NAS_PrintLog(LOG_INFO, "");
        for(int j=0;j<u32clientCount;j++)
        {
            NAS_PrintLog(LOG_INFO, " client_%d: %s:%d  ",j,astClientInfo[j].acClientIP,astClientInfo[j].u16ClientPort);
        }
        NAS_PrintLog(LOG_INFO, "");
    }


    if(astClientInfo[i].u32halfpacklen > 0)                                                                                                                                       
    {                                                                                                                                                            
        if(astClientInfo[i].u32halfpacklen <= MSG_LENGTH)                                                                                                                         
        {                                                                                                                                                        
            memcpy(&packet_len, ( uint8_t*)pMessage + MSG_LENGTH - astClientInfo[i].u32halfpacklen , MEMBER_SIZE_BYTE);                                                           
        }                                                                                                                                                        
        else if(astClientInfo[i].u32halfpacklen < (MSG_LENGTH + MEMBER_SIZE_BYTE))                                                                                                
        {                                                                                                                                                        
            memcpy(&packet_len, ( uint8_t*)(astClientInfo[i].pu8halfpackbuf) + MSG_LENGTH, astClientInfo[i].u32halfpacklen - MSG_LENGTH);                                                            
            memcpy(( uint8_t*)(&packet_len) + astClientInfo[i].u32halfpacklen - MSG_LENGTH, ( uint8_t*)pMessage, MSG_LENGTH + MEMBER_SIZE_BYTE - astClientInfo[i].u32halfpacklen);          
        }                                                                                                                                                        
        else                                                                                                                                                     
        {                                                                                                                                                        
            memcpy(&packet_len, ( uint8_t*)(astClientInfo[i].pu8halfpackbuf) + MSG_LENGTH , MEMBER_SIZE_BYTE);                                                                      
        }                                                                                                                                                        
                                                                                                                                                                 
        if((packet_len&PACKET_ENABLE) == 0)                                                                                                                      
        {                                                                                                                                                        
            packet_len = packet_len + sizeof(stMsgHeader);                                                                                                       
            issubpacket = 0;                                                                                                                                     
        }                                                                                                                                                        
        else                                                                                                                                                     
        {                                                                                                                                                        
            issubpacket = 1;                                                                                                                                     
            if(astClientInfo[i].u32halfpacklen <= SUBPACKET_LEN)                                                                                                                  
            {                                                                                                                                                    
                memcpy(&packet_len, ( uint8_t*)pMessage + SUBPACKET_LEN - astClientInfo[i].u32halfpacklen , MEMBER_SIZE_BYTE);                                                    
            }                                                                                                                                                    
            else if(astClientInfo[i].u32halfpacklen < (SUBPACKET_LEN + MEMBER_SIZE_BYTE))                                                                                         
            {                                                                                                                                                    
                memcpy(&packet_len, ( uint8_t*)(astClientInfo[i].pu8halfpackbuf) + SUBPACKET_LEN, astClientInfo[i].u32halfpacklen - SUBPACKET_LEN);                                                  
                memcpy(( uint8_t*)(&packet_len) + astClientInfo[i].u32halfpacklen - SUBPACKET_LEN, ( uint8_t*)pMessage, SUBPACKET_LEN + MEMBER_SIZE_BYTE - astClientInfo[i].u32halfpacklen);
            }                                                                                                                                                    
            else                                                                                                                                                 
            {                                                                                                                                                    
                memcpy(&packet_len, ( uint8_t*)(astClientInfo[i].pu8halfpackbuf) + SUBPACKET_LEN , MEMBER_SIZE_BYTE);                                                               
            }                                                                                                                                                    
            packet_len += sizeof(PacketHead);                                                                                                                    
        }                                                                                                                                                        

        if(SINGLE_PACKET_LEN_MAX < packet_len)
        {
            SessionData clientsessionData;
            
            pf_free(astClientInfo[i].pu8halfpackbuf); 
            astClientInfo[i].S32halfcount--;
            astClientInfo[i].pu8halfpackbuf = NULL;

            conn->GetClientConnectionInfo(clientsessionData);
            
            PS_CPlus(CM_NES, CMNES_ID_SERVER_PACKETLEN_MSGTYPE_ERR);

            NAS_PrintLog(LOG_ERROR, " halfpacketlen_%d , packetlen_%d , SINGLE_PACKET_LEN_MAX_%d , local_%s:%d , peer_%s:%d ",
                        astClientInfo[i].u32halfpacklen,packet_len, SINGLE_PACKET_LEN_MAX,clientsessionData.m_LocalIP.c_str(), 
                        clientsessionData.m_LocalPort,clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort);    

            astClientInfo[i].u32halfpacklen = 0;                                                                                                                                      
            pu8data = ( uint8_t*)pMessage;                                                                                                                           
            total_len = 0;    
            s32findMsghead = 1;
        }
        else
        {
            s32findMsghead = 0;
            
            if(astClientInfo[i].u32halfpacklen < sizeof(PacketHead))                                                                                                                  
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
                
                memcpy(pu8data,astClientInfo[i].pu8halfpackbuf,astClientInfo[i].u32halfpacklen);                                                                                                       
                pf_free(astClientInfo[i].pu8halfpackbuf);  

                astClientInfo[i].S32halfcount--;

                astClientInfo[i].pu8halfpackbuf = pu8data;  

                astClientInfo[i].S32halfcount++;
                astClientInfo[i].S32halflen = packet_len;

                
            }                                                                                                                                                        
                                                                                                                                                                     
                                                                                                                                                                     
            if(packet_len > messageSize + astClientInfo[i].u32halfpacklen)                                                                                                            
            {                                                                                                                                                        
                memcpy(astClientInfo[i].pu8halfpackbuf + astClientInfo[i].u32halfpacklen, ( uint8_t*)pMessage , messageSize);                                                                          
                astClientInfo[i].u32halfpacklen += messageSize;                                                                                                                       
    //            NAS_PrintLog(LOG_WARNING, " Server::OnRecv  halfhalf_packet: len_%d , messageSize_%d , total_len_%d ", packet_len, messageSize,total_len);    
                return;                                                                                                                                              
            }                                                                                                                                                        
                                                                                                                                                                     
            memcpy(astClientInfo[i].pu8halfpackbuf + astClientInfo[i].u32halfpacklen, ( uint8_t*)pMessage , packet_len - astClientInfo[i].u32halfpacklen);                                                              
                                                                                                                                                                     
            if(issubpacket == 0)                                                                                                                                     
            { 
    //mem_check(i);
                receive_func_call( conn, ( uint8_t*) astClientInfo[i].pu8halfpackbuf, packet_len,sessionData);                                                                        
                astClientInfo[i].u32okcount++;
            }                                                                                                                                                        
            else if(PACKET_SUCCESS == CombineSubpacket( i , astClientInfo[i].pu8halfpackbuf, &recv_i))                                                                                              
            {                                                                                                                                                        
                stMsgHeader* pMsgHead = (stMsgHeader*)astClientInfo[i].astClientRecv[recv_i].pu8receivebuf;                                                                                                 
                pMsgHead->ulMsgLength &=~ PACKET_ENABLE; 
                
    //mem_check(i);
                receive_func_call( conn, ( uint8_t*) astClientInfo[i].astClientRecv[recv_i].pu8receivebuf, pMsgHead->ulMsgLength + sizeof(stMsgHeader),sessionData);                                        
                astClientInfo[i].astClientRecv[recv_i].u32receivecount = 0;                                                                                                                                 
                astClientInfo[i].astClientRecv[recv_i].u32receiveID = 0;                                                                                                                                    
                pf_free(astClientInfo[i].astClientRecv[recv_i].pu8receivebuf); 
                astClientInfo[i].astClientRecv[recv_i].S32count--;

                
                astClientInfo[i].astClientRecv[recv_i].pu8receivebuf = NULL;                                                                                                                                
                astClientInfo[i].u32okcount++;
            }                                                                                                                                                        
                                                                                                                                                                     
            total_len = packet_len - astClientInfo[i].u32halfpacklen;                                                                                                                 
            pu8data = ( uint8_t*)pMessage + total_len;                                                                                                               
                                                                                                                                                                     
            astClientInfo[i].u32halfpacklen = 0;                                                                                                                                      
            pf_free(astClientInfo[i].pu8halfpackbuf); 
            astClientInfo[i].S32halfcount--;

            
            astClientInfo[i].pu8halfpackbuf = NULL;
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
                NAS_PrintLog(LOG_WARNING, " UDP_Server::OnRecv  half_packet: len_%d , messageSize_%d , total_len_%d , client_%s:%d ", packet_len, messageSize, total_len, sessionData.m_PeerIP.c_str(), sessionData.m_PeerPort);

#if 0
                NAS_PrintLog(LOG_WARNING, ", packet_id0:%d , packet_id1:%d , client_%s:%d ",astClientInfo[i].astClientRecv[0].u32receiveID 
                                            ,astClientInfo[i].astClientRecv[1].u32receiveID , sessionData.m_PeerIP.c_str(), sessionData.m_PeerPort);

                NAS_PrintLog(LOG_WARNING,"\r\n\r\npacket_head"); 
                for(int i = 0; i < 40 ; i+=4)
                {
                    if((i%16) == 0)
                    {
                        NAS_PrintLog(LOG_WARNING,"\r\n"); 
                    }
                    NAS_PrintLog(LOG_WARNING," 0x%02x%02x%02x%02x", pu8data[i+3], pu8data[i+2], pu8data[i+1], pu8data[i]); 
                }
                NAS_PrintLog(LOG_WARNING,"\r\n\r\n"); 
#endif
 //               PS_CPlus(CM_NES, CMNES_ID_SERVER_ONRECV_FAIL);				
                return;                                                                                                                                          
            }                                                                                                                                                    


            if((SINGLE_PACKET_LEN_MAX < packet_len)||(u32msgtype >= LAST_EVENT)||
                ((s32findMsghead == 1)&&((MSG_TYPE + MEMBER_SIZE_BYTE) > (messageSize - total_len))))
            {
                SessionData clientsessionData;
                
                s32findMsghead = 1;
                
                conn->GetClientConnectionInfo(clientsessionData);
                PS_CPlus(CM_NES, CMNES_ID_SERVER_PACKETLEN_MSGTYPE_ERR);

                NAS_PrintLog(LOG_ERROR, " messageSize_%d , total_len_%d , packetlen_%d , SINGLE_PACKET_LEN_MAX_%d , u32msgtype_%d , LAST_EVENT_%d , local_%s:%d , peer_%s:%d ",
                            messageSize,total_len,packet_len, SINGLE_PACKET_LEN_MAX,u32msgtype,LAST_EVENT,
                            clientsessionData.m_LocalIP.c_str(), clientsessionData.m_LocalPort,
                            clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort);  
                            
                return;
            }
            
            s32findMsghead = 0;

            astClientInfo[i].u32halfpacklen = messageSize - total_len;      

            while(1)
            {
                if(packet_len == 0)                                                                                                                                  
                {                                                                                                                                                    
                    astClientInfo[i].pu8halfpackbuf = ( uint8_t*)pf_malloc(astClientInfo[i].u32halfpacklen);                                                                                           
                }                                                                                                                                                    
                else                                                                                                                                                 
                {                                                                                                                                                    
                    astClientInfo[i].pu8halfpackbuf = ( uint8_t*)pf_malloc(packet_len);                                                                                               
                }  

                if(astClientInfo[i].pu8halfpackbuf == NULL)
                {

 //                   NAS_PrintLog(LOG_ERROR, "\r\n Server::OnRecv  client[%d]:  pf_malloc fail!   packet_len_%d , messageSize_%d , total_len_%d \r\n", i , packet_len, messageSize,total_len);        
                    pf_usleep(1000);
                }
                else
                {
                    astClientInfo[i].S32halfcount++;

                    if(packet_len == 0)
                    {
                        astClientInfo[i].S32halflen = astClientInfo[i].u32halfpacklen;
                    }
                    else
                    {
                        astClientInfo[i].S32halflen = packet_len;
                    }
                
                    break;
                }
            }

            memcpy(astClientInfo[i].pu8halfpackbuf,pu8data,astClientInfo[i].u32halfpacklen);    

            return;                                                                                                                                              
        }                                                                                                                                                        
                                                                                                                                                                 
        s32findMsghead = 0;
                                                                                                                                                                 
        if(issubpacket == 0)                                                                                                                                     
        {                                                                                                                                                        
            //mem_check(i);

            receive_func_call( conn, ( uint8_t*) pu8data, packet_len,sessionData);                                                                               
            astClientInfo[i].u32okcount++;
        }                                                                                                                                                        
        else if(PACKET_SUCCESS == CombineSubpacket( i , pu8data , &recv_i))                                                                                                     
        {                                                                                                                                                        
            stMsgHeader* pMsgHead = (stMsgHeader*)(astClientInfo[i].astClientRecv[recv_i].pu8receivebuf);                                                                                                 
            pMsgHead->ulMsgLength &=~ PACKET_ENABLE;                                                                                                             
            //mem_check(i);
            receive_func_call( conn, ( uint8_t*) astClientInfo[i].astClientRecv[recv_i].pu8receivebuf, pMsgHead->ulMsgLength + sizeof(stMsgHeader),sessionData);                                        
            astClientInfo[i].astClientRecv[recv_i].u32receivecount = 0;                                                                                                                                 
            astClientInfo[i].astClientRecv[recv_i].u32receiveID = 0;                                                                                                                                    
            pf_free(astClientInfo[i].astClientRecv[recv_i].pu8receivebuf); 
            astClientInfo[i].astClientRecv[recv_i].S32count--;            
            astClientInfo[i].astClientRecv[recv_i].pu8receivebuf = NULL;                                                                                                                                
            astClientInfo[i].u32okcount++;

 //           if(0 == (astClientInfo[i].u32okcount%10))
 //           {
 //               NAS_PrintLog(LOG_WARNING, "\r\n Server::OnRecv  OK_PacketCount:%d , Fail_PacketCount:%d , clientIP_%s , clientPort_%d  \r\n",astClientInfo[i].u32okcount, astClientInfo[i].u32failcount , sessionData.m_PeerIP.c_str(), sessionData.m_PeerPort);
 //           }
        }                                                                                                                                                        
                                                                                                                                                                 
        total_len += packet_len;                                                                                                                                 
        pu8data += packet_len;                                                                                                                                   
    }                                                                                                                                                            
                                                                                                                                                                 
}








void ServerNetSession::OnRecv(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData)
{
#if 0
int i=0;
printf( "\r\nOnRecv_%d",messageSize);
for(i=0;i<messageSize;i++)
{
    if(0 == (i%16))
    {
        printf( "\r\n");
    }
    
    if(0 == (i%4))
    {
        printf(  " ");
    }
    
    printf( "%02x ",pMessage[i]);

}
printf( "\r\n");
#endif

    PF_MUTEX_LOCK(&server_mutex);
    if(NULL != receive_func_call)
    {

		if(SESSION_MODE_PASS_THROUGH == u32sessionmode)
		{
			receive_func_call( conn,  ( uint8_t*)pMessage, messageSize, sessionData);
		}
		else
		{
			CheckPacket(conn, pMessage, messageSize, sessionData);
		}

    }
    PF_MUTEX_UNLOCK(&server_mutex);
}


void ServerNetSession::OnClose(IClientConnection* conn)
{
    PF_MUTEX_LOCK(&server_mutex);

    SessionData clientsessionData;

	if(disconnect_func_call != NULL)
	{
		disconnect_func_call(conn);
	}
    
    if(conn != NULL)
    {
        conn->GetClientConnectionInfo(clientsessionData);
        NAS_PrintLog(LOG_FATAL, " ServerNetSession::OnClose, local_%s:%d, peer_%s:%d , conn_%lx " ,
                    clientsessionData.m_LocalIP.c_str(), clientsessionData.m_LocalPort,
                    clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort,conn);
        conn->Close();
    }

    for(int i = 0; i<u32clientCount; i++)
    {
        if(( astClientInfo[i].u16ClientPort == clientsessionData.m_PeerPort)
            &&( 0 == strcmp( astClientInfo[i].acClientIP ,clientsessionData.m_PeerIP.c_str())))
        {

            if(astClientInfo[i].astClientRecv[0].pu8receivebuf != NULL)
            {
                pf_free(astClientInfo[i].astClientRecv[0].pu8receivebuf);

                astClientInfo[i].astClientRecv[0].S32count--;                
                astClientInfo[i].astClientRecv[0].pu8receivebuf = NULL;
            }

            if(astClientInfo[i].astClientRecv[1].pu8receivebuf != NULL)
            {
                pf_free(astClientInfo[i].astClientRecv[1].pu8receivebuf);
                astClientInfo[i].astClientRecv[1].S32count--;

                astClientInfo[i].astClientRecv[1].pu8receivebuf = NULL;
            }

            if(astClientInfo[i].pu8halfpackbuf != NULL)
            {
                pf_free(astClientInfo[i].pu8halfpackbuf);
                astClientInfo[i].S32halfcount--;
                astClientInfo[i].pu8halfpackbuf = NULL;
            }
        
            memcpy(&(astClientInfo[i]), &(astClientInfo[u32clientCount-1]),sizeof(stClientInfo));
            memset(&(astClientInfo[u32clientCount-1]),0,sizeof(stClientInfo));
            u32clientCount--;
            break;
        }
    }

//    NAS_PrintLog(LOG_INFO, "  client_toatal_%d " ,u32clientCount);

    NAS_PrintLog(LOG_INFO, "");
    for(int j=0;j<u32clientCount;j++)
    {
        NAS_PrintLog(LOG_INFO, " client_%d: %s:%d ",j,astClientInfo[j].acClientIP,astClientInfo[j].u16ClientPort);
    }
    NAS_PrintLog(LOG_INFO, "");
	
	PF_MUTEX_UNLOCK(&server_mutex);
   
}

#pragma warning(default: 4100)

bool ServerNetSession::InitNetSession(NetProtocol protocol, const SessionData& sessionData, func_receive receive_func, func_accept accept_func, func_disconnect disconnect_func)
{
    PF_MUTEX_LOCK(&server_mutex);

    if(protocol != NETPROTOCOL_TCP && sessionData.m_CryptoCfg != CRYPTO_CFG_NONE)
    {
        NAS_PrintLog(LOG_ERROR, " config ssl cfg but protocol not tcp");
        return false;
    }
    
    int i=0;

    for(i = 0; i<u32clientCount; i++)
    {
        if(astClientInfo[i].astClientRecv[0].pu8receivebuf != NULL)
        {
            pf_free(astClientInfo[i].astClientRecv[0].pu8receivebuf);

            astClientInfo[i].astClientRecv[0].S32count--;

            
            astClientInfo[i].astClientRecv[0].pu8receivebuf = NULL;
        }

        if(astClientInfo[i].astClientRecv[1].pu8receivebuf != NULL)
        {
            pf_free(astClientInfo[i].astClientRecv[1].pu8receivebuf);

            astClientInfo[i].astClientRecv[1].S32count--;
            astClientInfo[i].astClientRecv[1].pu8receivebuf = NULL;
        }

        if(astClientInfo[i].pu8halfpackbuf != NULL)
        {
            pf_free(astClientInfo[i].pu8halfpackbuf);

            astClientInfo[i].S32halfcount--;
            astClientInfo[i].pu8halfpackbuf = NULL;
        }
        
    }

    u32sendID = 0;
    u32clientCount = 0;
    memset(astClientInfo,0,sizeof(astClientInfo));
    u32protocol = protocol;
	u32sessionmode = SESSION_MODE_PACKET_HEAD;

#ifdef GTEST_EN

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
        NAS_PrintLog(LOG_WARNING, " ServerNetSession::InitNetSession   LocalPort:%d  PeerPort:%d  invalid for gtest ",
                   sessionData.m_LocalPort,sessionData.m_PeerPort);
		
		PF_MUTEX_UNLOCK(&server_mutex);

        return true;
    }

#endif

    if (NULL != pServerConnection)
    {
        NetFactory::GetInstance()->DeleteServerConnection(pServerConnection);
        pServerConnection = NULL; 
        NAS_PrintLog(LOG_WARNING, " ServerNetSession::InitNetSession DeleteServerConnection ");
    }
    
    receive_func_call = NULL;
    accept_func_call = NULL;
    disconnect_func_call = NULL;

    if( true == server_udp_handle.IsValid() )
    {
        NetManager::GetInstance()->Close( server_udp_handle );
        server_udp_handle.Close();
        NAS_PrintLog(LOG_INFO, " ServerNetSession::InitNetSession Close( server_udp_handle ) ");
    }

    if(NULL == receive_func)
    {
        PS_CPlus(CM_NES, CMNES_ID_SERVER_INIT_RECEIVE_FUNC_FAIL);
        NAS_PrintLog(LOG_ERROR, " ServerNetSession::InitNetSession receive_func is NULL ");

		PF_MUTEX_UNLOCK(&server_mutex);
		
        return false;
    }

    receive_func_call = receive_func;

    switch(protocol)
    {
        case NETPROTOCOL_UDP:
            server_udp_handle = NetFactory::GetInstance()->CreateUdpServer(this, sessionData);
            
            if( false == server_udp_handle.IsValid() )
            {
                NAS_PrintLog(LOG_ERROR, " InitNetSession server_udp_handle is invalid ");
                PS_CPlus(CM_NES, CMNES_ID_SERVER_INIT_UDP_HANDLE_FAIL);
				
				PF_MUTEX_UNLOCK(&server_mutex);
                return false;
            }
            break;

        case NETPROTOCOL_TCP:
            pServerConnection = NetFactory::GetInstance()->CreateTcpServer(this, sessionData);
            
            if (NULL == pServerConnection)
            {
                NAS_PrintLog(LOG_ERROR, " InitNetSession pServerConnection is NULL ");
                PS_CPlus(CM_NES, CMNES_ID_SERVER_INIT_TCP_HANDLE_FAIL);

				PF_MUTEX_UNLOCK(&server_mutex);
                return false;
            }
            break;

        default:
            NAS_PrintLog(LOG_ERROR, " ServerNetSession::InitNetSession  protocol is invalid ");
            PS_CPlus(CM_NES, CMNES_ID_SERVER_INIT_PROTOCOL_FAIL);
			PF_MUTEX_UNLOCK(&server_mutex);
            return false;
    }

    accept_func_call = accept_func;
    disconnect_func_call = disconnect_func;
    
    PF_MUTEX_UNLOCK(&server_mutex);
    NAS_PrintLog(LOG_FATAL, " ServerNetSession::InitNetSession OK!  Local_%s:%d  Peer_%s:%d ",
               sessionData.m_LocalIP.c_str(),sessionData.m_LocalPort,sessionData.m_PeerIP.c_str(),sessionData.m_PeerPort);

    return true;
}


bool ServerNetSession::Netsession_isvalid(void)
{
    if(( pServerConnection != NULL )||( true == server_udp_handle.IsValid() ))
    {
        return true;
    }
    else
    {
        return false;
    }
}





bool ServerNetSession::Send(IClientConnection* conn, uint8_t* data , uint32_t size )
{

#ifdef GTEST_EN

    if(isvalid != true)
    {

        uint8_t * pMollcData = (uint8_t*)pl_malloc(size);
        
        pl_memcpy(pMollcData,data,size);

        pf_copy_msg(MODULE_NETWORK, GTEST_SOCKET_MSG_IND, MODULE_GTEST, pMollcData, size);

        pl_free(pMollcData);

        return true;
    }

#endif

    if(NULL != conn)
    {
        return conn->SendData( (const uint8_t*)data , size , NULL);
    }

    NAS_PrintLog(LOG_ERROR, " ServerNetSession::Send   conn is NULL ");
    PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_CONN_FAIL);
    return false;
}


bool ServerNetSession::Send(IClientConnection* conn, uint8_t* data , uint32_t size, uint32_t packet_max_len )
{

	if(u32sessionmode != SESSION_MODE_PACKET_HEAD)
	{
		NAS_PrintLog(LOG_ERROR, " ServerNetSession::Send   Current sessionmode is not SESSION_MODE_PACKET_HEAD ");
		return false;
	}


#ifdef GTEST_EN
    if(isvalid != true)
    {

        uint8_t * pMollcData = (uint8_t*)pl_malloc(size);
        
        pl_memcpy(pMollcData,data,size);

        pf_copy_msg(MODULE_NETWORK, GTEST_SOCKET_MSG_IND, MODULE_GTEST, pMollcData, size);

        pl_free(pMollcData);

        return true;
    }

#endif

    if(NULL != conn)
    {
        if(size > packet_max_len)
        {
            PacketHead stpackethead;
            uint8_t* pdata = NULL;
            uint32_t len = sizeof(PacketHead);

            if(packet_max_len <= len)
            { 
                NAS_PrintLog(LOG_ERROR, " ServerNetSession::Send   packet_max_len < %d ", len);
                PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_PACKET_LEN_FAIL);
                return false;
            }

			PF_MUTEX_LOCK(&server_mutex);
            if(u32sendID < 0xFFFFFFF0)
            {
                u32sendID++;
            }
            else
            {
                u32sendID = 1;
            }

            stpackethead.packetID = u32sendID;
			PF_MUTEX_UNLOCK(&server_mutex);
            stpackethead.packetIndex = 1;
            stpackethead.packetLen = packet_max_len - len;
            stpackethead.packetOffset = 0;
            stpackethead.totalPacketNum = ((size - sizeof(stMsgHeader) + stpackethead.packetLen -1)/stpackethead.packetLen);

            memcpy(&(stpackethead.msgHead),data,sizeof(stMsgHeader));
            stpackethead.msgHead.ulMsgLength |= PACKET_ENABLE;

            pdata = (uint8_t*)pf_malloc(packet_max_len);
            if(NULL == pdata)
            {
                NAS_PrintLog(LOG_ERROR, "pf_malloc fail, pdata is nullpointer");
                return false;
            }
            memcpy(pdata,&stpackethead,len);
            memcpy(pdata+len,data+sizeof(stMsgHeader),stpackethead.packetLen);
                
            if( false == conn->SendData( (const uint8_t*)pdata , packet_max_len , NULL ))
            {
                PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_PACKET_DATA_FAIL);
                pf_free(pdata);
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
                    return conn->SendData( (const uint8_t*)pdata , stpackethead.packetLen + len , NULL );
                }

                memcpy(pdata,&stpackethead,len);
                if( false == conn->SendData( (const uint8_t*)pdata , packet_max_len , NULL ))
                {
                    PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_PACKET_DATA_FAIL);
                    return false;
                }
                
                pdata = pdata + stpackethead.packetLen; 
            }
        }
        else
        {
            return conn->SendData( (const uint8_t*)data , size , NULL);
        }
    }

    NAS_PrintLog(LOG_ERROR, " ServerNetSession::Send   conn is NULL ");
    return false;
}


bool ServerNetSession::Send( uint8_t* data , uint32_t size , const SessionData& sessionData )
{
#ifdef GTEST_EN
    if(isvalid != true)
    {

        uint8_t * pMollcData = (uint8_t*)pl_malloc(size);
        
        pl_memcpy(pMollcData,data,size);

        pf_copy_msg(MODULE_NETWORK, GTEST_SOCKET_MSG_IND, MODULE_GTEST, pMollcData, size);

        pl_free(pMollcData);

        return true;
    }

#endif
		PF_MUTEX_LOCK(&server_mutex);

        if( false == server_udp_handle.IsValid() )
        {
            NAS_PrintLog(LOG_ERROR, " server_udp_handle is invalid ");
            PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_UDP_VALID_FAIL);
			PF_MUTEX_UNLOCK(&server_mutex);
            return false;
        }

        SndData udp_snddata;

        udp_snddata.m_dst_address = inet_addr(sessionData.m_PeerIP.c_str());
        udp_snddata.m_dst_port = htons(sessionData.m_PeerPort);

        bool ret = NetManager::GetInstance()->SendData( server_udp_handle , data , size , &udp_snddata );
		PF_MUTEX_UNLOCK(&server_mutex);
		return ret;
        
}


bool ServerNetSession::Send( uint8_t* data , uint32_t size , const SessionData& sessionData, uint32_t packet_max_len )
{

	if(u32sessionmode != SESSION_MODE_PACKET_HEAD)
	{
		NAS_PrintLog(LOG_ERROR, " ServerNetSession::Send   Current sessionmode is not SESSION_MODE_PACKET_HEAD ");
		return false;
	}


#ifdef GTEST_EN

    if(isvalid != true)
    {

        uint8_t * pMollcData = (uint8_t*)pl_malloc(size);
        
        pl_memcpy(pMollcData,data,size);

        pf_copy_msg(MODULE_NETWORK, GTEST_SOCKET_MSG_IND, MODULE_GTEST, pMollcData, size);

        pl_free(pMollcData);

        return true;
    }

#endif

        if( false == server_udp_handle.IsValid() )
        {
            NAS_PrintLog(LOG_ERROR, " server_udp_handle is invalid ");
            PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_SESSION_ISVALID_FAIL);
            return false;
        }

        SndData udp_snddata;

        udp_snddata.m_dst_address = inet_addr(sessionData.m_PeerIP.c_str());
        udp_snddata.m_dst_port = htons(sessionData.m_PeerPort);

        if(size > packet_max_len)
        {
            PacketHead stpackethead;
            uint8_t* pdata = NULL;
            uint32_t len = sizeof(PacketHead);

            if(packet_max_len <= len)
            { 
                NAS_PrintLog(LOG_ERROR, " ServerNetSession::Send   packet_max_len < %d ", len);
                PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_SESSION_LEN_FAIL);
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
                
            if( false == NetManager::GetInstance()->SendData( server_udp_handle , pdata , packet_max_len , &udp_snddata ))
            {
                PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_SESSION_DATA_FAIL);
                pf_free(pdata);
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
                    return NetManager::GetInstance()->SendData( server_udp_handle , pdata , stpackethead.packetLen + len , &udp_snddata );
                }

                memcpy(pdata,&stpackethead,len);
                if( false == NetManager::GetInstance()->SendData( server_udp_handle , pdata , packet_max_len , &udp_snddata ))
                {
                    PS_CPlus(CM_NES, CMNES_ID_SERVER_SEND_SESSION_DATA_FAIL);
                    return false;
                }
                
                pdata = pdata + stpackethead.packetLen; 
            }
        }
        else
        {
            return NetManager::GetInstance()->SendData( server_udp_handle , data , size , &udp_snddata );
        }
        
}


void ServerNetSession:: DeleteClient(IClientConnection* conn)
{
	PF_MUTEX_LOCK(&server_mutex);

    SessionData clientsessionData;
    uint32_t i = 0;
    
    if(conn != NULL)
    {
        conn->GetClientConnectionInfo(clientsessionData);
        conn->Close();
    }

    for(i = 0; i<u32clientCount; i++)
    {
        if(( astClientInfo[i].u16ClientPort == clientsessionData.m_PeerPort)
            &&( 0 == strcmp( astClientInfo[i].acClientIP ,clientsessionData.m_PeerIP.c_str())))
        {

            if(astClientInfo[i].astClientRecv[0].pu8receivebuf != NULL)
            {
                pf_free(astClientInfo[i].astClientRecv[0].pu8receivebuf);

                astClientInfo[i].astClientRecv[0].S32count--;
                astClientInfo[i].astClientRecv[0].pu8receivebuf = NULL;
            }

            if(astClientInfo[i].astClientRecv[1].pu8receivebuf != NULL)
            {
                pf_free(astClientInfo[i].astClientRecv[1].pu8receivebuf);

                astClientInfo[i].astClientRecv[1].S32count--;
                astClientInfo[i].astClientRecv[1].pu8receivebuf = NULL;
            }

            if(astClientInfo[i].pu8halfpackbuf != NULL)
            {
                pf_free(astClientInfo[i].pu8halfpackbuf);
                astClientInfo[i].S32halfcount--;
                astClientInfo[i].pu8halfpackbuf = NULL;
            }
        
            memcpy(&(astClientInfo[i]), &(astClientInfo[u32clientCount-1]),sizeof(stClientInfo));
            memset(&(astClientInfo[u32clientCount-1]),0,sizeof(stClientInfo));
            u32clientCount--;
            break;
        }
    }

    NAS_PrintLog(LOG_FATAL, " ServerNetSession::DeleteClient  conn_%lx , ip: %s , port: %d , client_toatal_%d ",
                conn,clientsessionData.m_PeerIP.c_str(), clientsessionData.m_PeerPort,u32clientCount);
    for(int j=0;j<u32clientCount;j++)
    {
        NAS_PrintLog(LOG_INFO, " client_%d: %s:%d ",j,astClientInfo[j].acClientIP,astClientInfo[j].u16ClientPort);
    }
	
	PF_MUTEX_UNLOCK(&server_mutex);
}


void ServerNetSession:: DeleteClient( char* cpclient_IP,  uint16_t client_port)
{
	PF_MUTEX_LOCK(&server_mutex);

    uint32_t i = 0;

    for(i = 0; i<u32clientCount; i++)
    {
        if(( astClientInfo[i].u16ClientPort == client_port)
            &&( 0 == strcmp( astClientInfo[i].acClientIP ,cpclient_IP)))
        {

            if(astClientInfo[i].astClientRecv[0].pu8receivebuf != NULL)
            {
                pf_free(astClientInfo[i].astClientRecv[0].pu8receivebuf);
                astClientInfo[i].astClientRecv[0].S32count--;
                astClientInfo[i].astClientRecv[0].pu8receivebuf = NULL;
            }

            if(astClientInfo[i].astClientRecv[1].pu8receivebuf != NULL)
            {
                pf_free(astClientInfo[i].astClientRecv[1].pu8receivebuf);
                astClientInfo[i].astClientRecv[1].S32count--;
                astClientInfo[i].astClientRecv[1].pu8receivebuf = NULL;
            }
            
            if(astClientInfo[i].pu8halfpackbuf != NULL)
            {
                pf_free(astClientInfo[i].pu8halfpackbuf);
                astClientInfo[i].S32halfcount--;
                astClientInfo[i].pu8halfpackbuf = NULL;
            }
        
            memcpy(&(astClientInfo[i]), &(astClientInfo[u32clientCount-1]),sizeof(stClientInfo));
            memset(&(astClientInfo[u32clientCount-1]),0,sizeof(stClientInfo));
            u32clientCount--;
            break;
        }
    }

    NAS_PrintLog(LOG_FATAL, " UDPServer::DeleteClient ip: %s , port: %d , client_toatal_%d ",cpclient_IP , client_port,u32clientCount);
    for(int j=0;j<u32clientCount;j++)
    {
        NAS_PrintLog(LOG_INFO, " client_%d: %s:%d ",j,astClientInfo[j].acClientIP,astClientInfo[j].u16ClientPort);
    }
	PF_MUTEX_UNLOCK(&server_mutex);
}



void ServerNetSession:: CloseServer(void)
{
	PF_MUTEX_LOCK(&server_mutex);

	if (NULL != pServerConnection)
    {
        NetFactory::GetInstance()->DeleteServerConnection(pServerConnection);
        pServerConnection = NULL; 
        NAS_PrintLog(LOG_WARNING, " ServerNetSession::CloseServer DeleteServerConnection ");
    }
    
    receive_func_call = NULL;
    accept_func_call = NULL;
    disconnect_func_call = NULL;

    if( true == server_udp_handle.IsValid() )
    {
        NetManager::GetInstance()->Close( server_udp_handle );
        server_udp_handle.Close();
    }


    uint32_t i = 0;

    for(i = 0; i<u32clientCount; i++)
    {
        if(astClientInfo[i].astClientRecv[0].pu8receivebuf != NULL)
        {
            pf_free(astClientInfo[i].astClientRecv[0].pu8receivebuf);
            astClientInfo[i].astClientRecv[0].S32count--;

            
            astClientInfo[i].astClientRecv[0].pu8receivebuf = NULL;
        }

        if(astClientInfo[i].astClientRecv[1].pu8receivebuf != NULL)
        {
            pf_free(astClientInfo[i].astClientRecv[1].pu8receivebuf);
            astClientInfo[i].astClientRecv[1].S32count--;

            
            astClientInfo[i].astClientRecv[1].pu8receivebuf = NULL;
        }


        if(astClientInfo[i].pu8halfpackbuf != NULL)
        {
            pf_free(astClientInfo[i].pu8halfpackbuf);
            astClientInfo[i].S32halfcount--;
            astClientInfo[i].pu8halfpackbuf = NULL;
        }
        
    }

    memset(astClientInfo,0,sizeof(astClientInfo));

    PF_MUTEX_UNLOCK(&server_mutex);

    NAS_PrintLog(LOG_FATAL, " ServerNetSession::CloseServer ");
  
}



bool ServerNetSession:: SetTcpHBPara(IClientConnection* conn, int heartbeat_en, int idle, int cnt, int intv)
{
    if(NULL != conn)
    {
        SessionData clientsessionData;
        conn->GetClientConnectionInfo(clientsessionData);
    
        NAS_PrintLog(LOG_FATAL, " ServerNetSession:: SetTcpHBPara,  %s:%d   heartbeat_en:%d , idle:%d, cnt:%d, intv:%d ",
            clientsessionData.m_PeerIP.c_str(),clientsessionData.m_PeerPort,heartbeat_en,idle,cnt,intv);
        return conn->SetTcpHBPara( heartbeat_en,  idle,  cnt,  intv);
    }
    
    PS_CPlus(CM_NES, CMNES_ID_SERVER_SETTCPHBPARA_SESSION_FAIL);
    NAS_PrintLog(LOG_ERROR, " ServerNetSession::SetTcpHBPara conn is NULL ");
    return false;
}


void ServerNetSession::SetSessionMode( SessionMode mode )
{
    u32sessionmode = mode;
}

void ServerNetSession::SetBufferSize( int nSize)
{
    u32BufferSize = nSize;
}

void ServerNetSession::SetTcpMemSize( int nSize)
{
    u32TcpMemSizes = nSize;
}


