#define THIS_MODULE MODULE_NETWORK
/**********************************************************************************************//**
 * @file  test_network.cpp
 *
 * @brief    
 **************************************************************************************************/
#include <stdio.h>                                                                   
#include <string.h>  
#include "net_lib.h"

#include "platform.h"
#include "pf_stat.h"



#define SERVER_PROTOCOL      NETPROTOCOL_TCP
//#define LOCAL_IP             "127.0.0.1"
//#define LOCAL_IP             "172.16.8.98"
#define LOCAL_IP             "172.16.8.3"
#define LOCAL_SERVER_PORT    5555 


#define CLIENT_PROTOCOL      NETPROTOCOL_TCP
#define LOCAL_CLIENT_PORT    0
//#define PEER_IP              "172.16.8.77"
//#define PEER_PORT            1234


//#define PEER_IP              "172.16.8.98"
//#define PEER_PORT            5555

#define PEER_IP              "172.16.8.3"
//#define PEER_IP             "127.0.0.1"
#define PEER_PORT            5555

//#undef NAS_PrintLog(level, format, ...) 
//#define NAS_PrintLog(level, format, ...) printf(format, ## __VA_ARGS__)



bool server_send_data(IClientConnection* conn , uint8_t * pdata, uint32_t ulLength);




bool gtmpconnection = false;

#define  SEVER_CLIENT_MAX  3 
IClientConnection* g_Server_client_conn[SEVER_CLIENT_MAX] = {NULL};
SessionData udp_client_sessiondata[SEVER_CLIENT_MAX];
uint8_t udp_client_index = 0;


uint32_t client_send_count=0;
uint32_t client_sendfail_count=0;

uint32_t client_receive_count=0;
uint32_t server_receive_count=0;



ServerNetSession g_ServerNetSession;

ClientNetSession g_ClientNetSession[5];


void print_data_hex(void * pdata, uint32_t ulLength)
{
    U8* pu8data = (U8*)pdata;
    int i=0;
    for(i=0;i<ulLength;i++)
    {
        if(0 == (i%16))
        {
            NAS_PrintLog(LOG_INFO, "\r\n");
        }
        
        if(0 == (i%4))
        {
            NAS_PrintLog(LOG_INFO, " ");
        }
        
        NAS_PrintLog(LOG_INFO, "0x%02x, ",pu8data[i]);

    }
    NAS_PrintLog(LOG_INFO, "\r\n");
    
    
}





bool client_send_data(uint32_t clientIndex , void * pdata, uint32_t ulLength)
{
//    NAS_PrintLog(LOG_INFO, "\r\n client_%d_send_data_%d: %s\n",clientIndex, ulLength,pdata);
//    print_data_hex( pdata, ulLength);    
if(true == g_ClientNetSession[clientIndex].Send(pdata,ulLength))
{
    client_send_count++;
    return true;
}
else
{
//    NAS_PrintLog(LOG_INFO, "\r\n client_send_data_fail ");
    client_sendfail_count++;
    return false;
}
}

bool client_send_data_subpacket(uint32_t clientIndex ,void * pdata, uint32_t ulLength, uint32_t packet_len_max)
{
    NAS_PrintLog(LOG_INFO, "\r\n client_send_data_subpacket len_%d, packet_max_%d , %s\n", ulLength,packet_len_max,pdata);
    print_data_hex( pdata, ulLength);    
    
    return g_ClientNetSession[clientIndex].Send(pdata,ulLength,packet_len_max);
}


bool client_init(uint32_t clientIndex , uint32_t port);


void test_client_disconnect(IClientConnection* conn)
{

    SessionData clientsessionData;

    conn->GetClientConnectionInfo(clientsessionData);

//    NAS_PrintLog(LOG_INFO, "\r\n client_disconnect  Local_ %s:%d  Peer_ %s:%d", clientsessionData.m_LocalIP.c_str(),clientsessionData.m_LocalPort, clientsessionData.m_PeerIP.c_str(),clientsessionData.m_PeerPort);
    gtmpconnection = false;;

}







void client_receive_data(IClientConnection* conn, uint8_t * pdata,  uint32_t ulLength,const SessionData& sessionData)
{
    uint8_t* data_buf = (uint8_t*)pf_malloc(ulLength);
    if(NULL == data_buf)
    {
        NAS_PrintLog(LOG_ERROR, "pf_malloc fail, data_buf is nullpointer %s %d", __FUNCTION__, __LINE__);
        return ;
    }
    
    memcpy(data_buf,pdata,ulLength);

//    NAS_PrintLog(LOG_INFO, "\r\n client_receive_data    conn_ %d ", conn);
//    NAS_PrintLog(LOG_INFO, "\r\n client_receive_data_%d: %s ", ulLength,data_buf);
    
//    print_data_hex( pdata, ulLength);    



if(g_ClientNetSession[0].u32sessionmode == SESSION_MODE_PACKET_HEAD)
{

client_receive_count++;
//if((receive_count%1024) == 0)
{
    NAS_PrintLog(LOG_INFO, "\r\n client_receive_data_0x%08x: ID_0x%02x%02x%02x%02x ", client_receive_count, pdata[PACKET_ID+3], pdata[PACKET_ID+2], pdata[PACKET_ID+1], pdata[PACKET_ID]);
}
}
else
{
    NAS_PrintLog(LOG_INFO, "\r\n client_receive_data_%d: %s ",strlen((char*)pdata) , (char*)pdata);
}




#if 0
    NAS_PrintLog(LOG_INFO, "\r\n client_receive_data   Local_ %s:%d ", sessionData.m_LocalIP.c_str(),sessionData.m_LocalPort);
    NAS_PrintLog(LOG_INFO, "\r\n client_receive_data    Peer_ %s:%d ", sessionData.m_PeerIP.c_str(),sessionData.m_PeerPort);

    NAS_PrintLog(LOG_INFO, "\r\n client_receive_data   dstaddr_%d , dstport_%d ", inet_addr(sessionData.m_PeerIP.c_str()), htons(sessionData.m_PeerPort));
    
    client_send_data(0,data_buf,ulLength-2);

#else

//    client_send_data(0,data_buf,ulLength);

//    server_send_data(g_Server_client_conn[0], data_buf,ulLength);

    if( SERVER_PROTOCOL   ==   NETPROTOCOL_TCP)
    {
        if((data_buf[0] == 'D')&&(data_buf[1] == 'i')&&(data_buf[2] == 's')&&(data_buf[3] == 'c')&&(data_buf[4] == 'n')&&(data_buf[5] == 't'))
        {
            g_ServerNetSession.DeleteClient( g_Server_client_conn[0]);
        }
    }

    if((data_buf[0] == 'O')&&(data_buf[1] == 'v')&&(data_buf[2] == 'e')&&(data_buf[3] == 'r'))
    {
        g_ServerNetSession.CloseServer();
    }
#endif

    pf_free(data_buf);

}




bool client_init(uint32_t clientIndex , uint32_t port)
{
    SessionData sessionData;

    sessionData.m_LocalIP = LOCAL_IP;
    sessionData.m_LocalPort = port;
    sessionData.m_PeerIP= PEER_IP;
    sessionData.m_PeerPort = PEER_PORT;

    if( false == g_ClientNetSession[clientIndex].InitNetSession(CLIENT_PROTOCOL, sessionData, client_receive_data ,  test_client_disconnect) )
    {
//        NAS_PrintLog(LOG_INFO, "\r\n client%d_init fail! \r\n",clientIndex);
        return false;
    }

//    NAS_PrintLog(LOG_INFO, "\r\n client%d_init OK! ",clientIndex);
    gtmpconnection = true;


    SessionData sessionData1;

g_ClientNetSession[clientIndex].m_pNetsession->GetClientConnectionInfo(sessionData1);

//    NAS_PrintLog(LOG_INFO, " local_%s:%d , peer_%s:%d\r\n",sessionData1.m_LocalIP.c_str(),sessionData1.m_LocalPort,sessionData1.m_PeerIP.c_str(),sessionData1.m_PeerPort);

    return true;

}

















bool server_send_data(IClientConnection* conn , uint8_t * pdata, uint32_t ulLength)
{
//    NAS_PrintLog(LOG_INFO, "\r\n server_send_data _%d: %s\n", ulLength,pdata);
    
//    print_data_hex( pdata, ulLength);    

    if(( SERVER_PROTOCOL   ==   NETPROTOCOL_UDP)&&(udp_client_sessiondata[0].m_PeerPort  != 0))
    {
        return g_ServerNetSession.Send(pdata,ulLength,udp_client_sessiondata[0]);
    }
    else if(conn != NULL)
    {
        return g_ServerNetSession.Send(conn,pdata,ulLength);
    }
    return false;
}




void server_accept(IClientConnection* conn,  const SessionData& sessionData)
{
    int i = 0;
    for(i = 0; i < SEVER_CLIENT_MAX; i++)
    {
        if(g_Server_client_conn[i] == NULL)
        {
            g_Server_client_conn[i] = conn;

//            NAS_PrintLog(LOG_INFO, "\r\n server_accept   Local_ %s:%d , Peer_ %s:%d \r\n", sessionData.m_LocalIP.c_str(),sessionData.m_LocalPort, sessionData.m_PeerIP.c_str(),sessionData.m_PeerPort);
            break;
        }
    }


g_ServerNetSession.SetTcpHBPara(conn, 1, 10, 5, 3);

    if(i >= SEVER_CLIENT_MAX)
    {
        NAS_PrintLog(LOG_INFO, "\r\n server_accept   client is full!   Delete  conn_ %d \r\n", conn);
        g_ServerNetSession.DeleteClient( conn);
    }
}






void server_client_disconnect(IClientConnection* conn)
{

    SessionData clientsessionData;

    conn->GetClientConnectionInfo(clientsessionData);

//    NAS_PrintLog(LOG_INFO, "\r\n server_client_disconnect   Local_ %s:%d , Peer_ %s:%d ", clientsessionData.m_LocalIP.c_str(),clientsessionData.m_LocalPort, clientsessionData.m_PeerIP.c_str(),clientsessionData.m_PeerPort);

    for(int i = 0; i<SEVER_CLIENT_MAX; i++)
    {
        if(g_Server_client_conn[i] == conn)
        {
            g_Server_client_conn[i] = NULL;
            break;
        }
    }


}





void server_receive_data(IClientConnection* conn, uint8_t * pdata,  uint32_t ulLength,  const SessionData& sessionData)
{
    uint8_t* data_buf = (uint8_t*)pf_malloc(ulLength);
    if(NULL == data_buf)
    {
        NAS_PrintLog(LOG_ERROR, "pf_malloc fail, data_buf is nullpointer %s %d", __FUNCTION__, __LINE__);
        return ;
    }
    
    
    memcpy(data_buf,pdata,ulLength);

    NAS_PrintLog(LOG_INFO, "\r\n server_receive_data    conn_ %d ", conn);
    NAS_PrintLog(LOG_INFO, "\r\n server_receive_data_%d: %s ", ulLength,data_buf);
    NAS_PrintLog(LOG_INFO, "\r\n server_receive_data Local_%s: %d ", sessionData.m_LocalIP.c_str(),sessionData.m_LocalPort);
    NAS_PrintLog(LOG_INFO, "\r\n server_receive_data  Peer_%s: %d ", sessionData.m_PeerIP.c_str(),sessionData.m_PeerPort);
    NAS_PrintLog(LOG_INFO, "\r\n server_receive_data  user_data: 0x%lx ", sessionData.user_data);


    if( SERVER_PROTOCOL   ==   NETPROTOCOL_UDP)
    {
        int i = 0;
        for( i = 0; i<SEVER_CLIENT_MAX; i++)
        {
            if((udp_client_sessiondata[i].m_PeerIP == sessionData.m_PeerIP)&&(udp_client_sessiondata[i].m_PeerPort == sessionData.m_PeerPort))
            {
                break;
            }
        }

        if(i >= SEVER_CLIENT_MAX )
        {
            udp_client_sessiondata[udp_client_index] = sessionData;
            udp_client_index  =  ((udp_client_index + 1) %SEVER_CLIENT_MAX ) ;
        }

//        NAS_PrintLog(LOG_INFO, "\r\n udp server_receive_data    udp_client_index_ %d ", udp_client_index);

        
    }


if(g_ServerNetSession.u32sessionmode == SESSION_MODE_PACKET_HEAD)
{

server_receive_count++;
PacketHead* stphead = (PacketHead*)data_buf;
if(server_receive_count != stphead->packetID)
{
    NAS_PrintLog(LOG_INFO, "\r\nErr: server_receive_0x%08x:  len_%d , PacketID_0x%lx , clientsend_0x%08x ,  clientsendfail_0x%08x", server_receive_count,
              ulLength, stphead->packetID , client_send_count, client_sendfail_count);

    print_data_hex( pdata, 40);    
}
else if(server_receive_count %(1024*1024) == 1)
{
    NAS_PrintLog(LOG_INFO, "\r\n server_receive_0x%08x:  len_%d , PacketID_0x%lx , clientsend_0x%08x ,  clientsendfail_0x%08x", server_receive_count,
              ulLength, stphead->packetID , client_send_count, client_sendfail_count);
}
}
else
{

    NAS_PrintLog(LOG_INFO, "\r\n server_receive : len_%d, %s ,",strlen((char*)pdata), (char*)pdata);
	server_send_data( conn,  pdata, ulLength);

}

//print_data_hex( pdata, ulLength);    

//	server_send_data(conn, data_buf,ulLength);

    if(gtmpconnection == true)
    {
//        client_send_data(0,data_buf,ulLength);
    }
    else
    {
//        client_init(0,LOCAL_CLIENT_PORT);
    }




    if( CLIENT_PROTOCOL   ==   NETPROTOCOL_TCP)
    {
        if((data_buf[0] == 'D')&&(data_buf[1] == 'i')&&(data_buf[2] == 's')&&(data_buf[3] == 'c')&&(data_buf[4] == 'n')&&(data_buf[5] == 't'))
        {
//           g_ClientNetSession.DisConnect();
//           gtmpconnection = false;
        }

    }

    pf_free(data_buf);

}





bool server_init(void)
{
    SessionData sessionData;

    sessionData.m_LocalIP = LOCAL_IP;
    sessionData.m_LocalPort = LOCAL_SERVER_PORT;
    sessionData.m_PeerIP= '0';
    sessionData.m_PeerPort = 0;
	sessionData.user_data = 0x12345678;

    if( false == g_ServerNetSession.InitNetSession(SERVER_PROTOCOL, sessionData , server_receive_data, server_accept, server_client_disconnect) )
    {
//        NAS_PrintLog(LOG_INFO, "\r\n server_init fail! \r\n");
        return false;
    }

//    NAS_PrintLog(LOG_INFO, "\r\n server_init OK! \r\n");
    return true;

}


#if 1


void data_out( uint8_t* pdata_out, uint8_t* pdata_in ,  uint32_t size )
{

    if(pdata_out != NULL)
    {
        memcpy(pdata_out,pdata_in,size);
    }

#if 0
    
    int i=0;
    for(i=0;i<size;i++)
    {
        if(0 == (i%8))
        {
            NAS_PrintLog(LOG_INFO, "\r\n");
        }
        
        if(0 == (i%4))
        {
            NAS_PrintLog(LOG_INFO, " ");
        }
        
        NAS_PrintLog(LOG_INFO, "0x%02x, ",pdata_in[i]);

    }
    NAS_PrintLog(LOG_INFO, "\r\n");
#endif

}

uint32_t send_data_subpacket( uint32_t packet_id,uint8_t * data, uint32_t size, uint32_t packet_max_len, uint8_t * pout)
{
    if(size > packet_max_len)
    {
        PacketHead stpackethead;
        uint8_t* pdata = NULL;
        uint32_t len = sizeof(PacketHead);
        uint32_t out_len = 0;


        stpackethead.packetID = packet_id;
        stpackethead.packetLen = packet_max_len - len;
        stpackethead.totalPacketNum = ((size - sizeof(stMsgHeader) + stpackethead.packetLen -1)/stpackethead.packetLen);
        stpackethead.packetIndex = 1;
        stpackethead.packetOffset = 0;
        stpackethead.padding = 0xffffffff;
        
        memcpy(&(stpackethead.msgHead),data,sizeof(stMsgHeader));
        stpackethead.msgHead.ulMsgLength |= PACKET_ENABLE;

        pdata = data+sizeof(stMsgHeader);
        while(1)
        {
            data_out(pout+out_len,(uint8_t*)&stpackethead,len);
            out_len += len; 
            data_out(pout+out_len,pdata,stpackethead.packetLen);
            pdata += stpackethead.packetLen;   
            out_len += stpackethead.packetLen;
     
            stpackethead.packetIndex++;
            stpackethead.packetOffset += stpackethead.packetLen;

            if(stpackethead.packetIndex == stpackethead.totalPacketNum)
            {
                stpackethead.packetLen = size - stpackethead.packetOffset - sizeof(stMsgHeader);
            }
            else if(stpackethead.packetIndex > stpackethead.totalPacketNum)
            {
                return out_len;
            }
        }
    }
    else
    {

        data_out(pout,data,size);
        return size;
//            return m_pNetsession->SendData( (const uint8_t*)data , size , NULL );
    }
}

#endif






uint8_t pacet_test_data[571]={

 0x21, 0x43, 0x65, 0x87,  0x78, 0x56, 0x34, 0x12,  0x41, 0x00, 0x00, 0x80,  0x01, 0x00, 0x00, 0xa0, 
 0x01, 0x00, 0x00, 0x00,  0x05, 0x00, 0x00, 0x00,  0x01, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 
 0x0d, 0x00, 0x00, 0x00,  0xff, 0xff, 0xff, 0xff,  0x00, 0x01, 0x02, 0x03,  0x04, 0x05, 0x06, 0x07, 
 0x08, 0x09, 0x0a, 0x0b,  0x0c, 0x21, 0x43, 0x65,  0x87, 0x78, 0x56, 0x34,  0x12, 0x41, 0x00, 0x00, 
 0x80, 0x01, 0x00, 0x00,  0xa0, 0x01, 0x00, 0x00,  0x00, 0x05, 0x00, 0x00,  0x00, 0x02, 0x00, 0x00, 
 0x00, 0x0d, 0x00, 0x00,  0x00, 0x0d, 0x00, 0x00,  0x00, 0xff, 0xff, 0xff,  0xff, 0x0d, 0x0e, 0x0f, 
 0x10, 0x11, 0x12, 0x13,  0x14, 0x15, 0x16, 0x17,  0x18, 0x19, 0x21, 0x43,  0x65, 0x87, 0x78, 0x56, 
 0x34, 0x12, 0x41, 0x00,  0x00, 0x80, 0x01, 0x00,  0x00, 0xa0, 0x01, 0x00,  0x00, 0x00, 0x05, 0x00, 
 0x00, 0x00, 0x03, 0x00,  0x00, 0x00, 0x1a, 0x00,  0x00, 0x00, 0x0d, 0x00,  0x00, 0x00, 0xff, 0xff, 
 0xff, 0xff, 0x1a, 0x1b,  0x1c, 0x1d, 0x1e, 0x1f,  0x20, 0x21, 0x22, 0x23,  0x24, 0x25, 0x26, 0x21, 
 0x43, 0x65, 0x87, 0x78,  0x56, 0x34, 0x12, 0x41,  0x00, 0x00, 0x80, 0x01,  0x00, 0x00, 0xa0, 0x01, 
 0x00, 0x00, 0x00, 0x05,  0x00, 0x00, 0x00, 0x04,  0x00, 0x00, 0x00, 0x27,  0x00, 0x00, 0x00, 0x0d, 
 0x00, 0x00, 0x00, 0xff,  0xff, 0xff, 0xff, 0x27,  0x28, 0x29, 0x2a, 0x2b,  0x2c, 0x2d, 0x2e, 0x2f, 
 0x30, 0x31, 0x32, 0x33, 



 0x21, 0x43, 0x65,  0x87, 0x78, 0x56, 0x34, 
 0x12, 0x42, 0x00, 0x00,  0x80, 0x02, 0x00, 0x00,  0xa0, 0x02, 0x00, 0x00,  0x00, 0x06, 0x00, 0x00, 
 0x00, 0x01, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x0d, 0x00, 0x00,  0x00, 0xff, 0xff, 0xff, 
 0xff, 0x00, 0x01, 0x02,  0x03, 0x04, 0x05, 0x06,  0x07, 0x08, 0x09, 0x0a,  0x0b, 0x0c, 
 0x21, 0x43, 
 0x65, 0x87, 0x78, 0x56,  0x34, 0x12, 0x42, 0x00,  0x00, 0x80, 0x02, 0x00,  0x00, 0xa0, 0x02, 0x00, 
 0x00, 0x00, 0x06, 0x00,  0x00, 0x00, 0x02, 0x00,  0x00, 0x00, 0x0d, 0x00,  0x00, 0x00, 0x0d, 0x00, 
 0x00, 0x00, 0xff, 0xff,  0xff, 0xff, 0x0d, 0x0e,  0x0f, 0x10, 0x11, 0x12,  0x13, 0x14, 0x15, 0x16, 
 0x17, 0x18, 0x19, 
 

 0x21, 0x43, 0x65, 0x87,  0x78, 0x56, 0x34, 0x12,  0x41, 0x00, 0x00, 0x80, 
 0x01, 0x00, 0x00, 0xa0,  0x01, 0x00, 0x00, 0x00,  0x05, 0x00, 0x00, 0x00,  0x05, 0x00, 0x00, 0x00, 
 0x34, 0x00, 0x00, 0x00,  0x0d, 0x00, 0x00, 0x00,  0xff, 0xff, 0xff, 0xff,  0x34, 0x35, 0x36, 0x37, 
 0x38, 0x39, 0x3a, 0x3b,  0x3c, 0x3d, 0x3e, 0x3f,  0x40, 

 

 0x21,  0x43, 0x65, 0x87, 0x78,  0x56, 0x34, 0x12, 0x42,  0x00, 0x00, 0x80, 0x02, 
 0x00, 0x00, 0xa0, 0x02,  0x00, 0x00, 0x00, 0x06,  0x00, 0x00, 0x00, 0x03,  0x00, 0x00, 0x00, 0x1a, 
 0x00, 0x00, 0x00, 0x0d,  0x00, 0x00, 0x00, 0xff,  0xff, 0xff, 0xff, 0x1a,  0x1b, 0x1c, 0x1d, 0x1e, 
 0x1f, 0x20, 0x21, 0x22,  0x23, 0x24, 0x25, 0x26,  0x21, 0x43, 0x65, 0x87,  0x78, 0x56, 0x34, 0x12, 
 0x42, 0x00, 0x00, 0x80,  0x02, 0x00, 0x00, 0xa0,  0x02, 0x00, 0x00, 0x00,  0x06, 0x00, 0x00, 0x00, 
 0x04, 0x00, 0x00, 0x00,  0x27, 0x00, 0x00, 0x00,  0x0d, 0x00, 0x00, 0x00,  0xff, 0xff, 0xff, 0xff, 
 0x27, 0x28, 0x29, 0x2a,  0x2b, 0x2c, 0x2d, 0x2e,  0x2f, 0x30, 0x31, 0x32,  0x33, 0x21, 0x43, 0x65, 
 0x87, 0x78, 0x56, 0x34,  0x12, 0x42, 0x00, 0x00,  0x80, 0x02, 0x00, 0x00,  0xa0, 0x02, 0x00, 0x00, 
 0x00, 0x06, 0x00, 0x00,  0x00, 0x05, 0x00, 0x00,  0x00, 0x34, 0x00, 0x00,  0x00, 0x0d, 0x00, 0x00, 
 0x00, 0xff, 0xff, 0xff,  0xff, 0x34, 0x35, 0x36,  0x37, 0x38, 0x39, 0x3a,  0x3b, 0x3c, 0x3d, 0x3e, 
 0x3f, 0x40, 0x21, 0x43,  0x65, 0x87, 0x78, 0x56,  0x34, 0x12, 0x42, 0x00,  0x00, 0x80, 0x02, 0x00, 
 0x00, 0xa0, 0x02, 0x00,  0x00, 0x00, 0x06, 0x00,  0x00, 0x00, 0x06, 0x00,  0x00, 0x00, 0x41, 0x00, 
 0x00, 0x00, 0x01, 0x00,  0x00, 0x00, 0xff, 0xff,  0xff, 0xff, 0x41, 
};


//#define TEST_DATA_LEN  0x00001F40   // 8000
#define TEST_DATA_LEN  1400   // 8000


//extern S32 pf_stat_get_cpu_load(S32 *p_cpuload, S32 *p_sysload);




void network_test2(void)
{      

    server_init();

   
    client_init(0,LOCAL_CLIENT_PORT);

	g_ServerNetSession.SetSessionMode(SESSION_MODE_PASS_THROUGH);

	g_ClientNetSession[0].SetSessionMode(SESSION_MODE_PASS_THROUGH);


	client_send_data(0,(void*)"hello test", 10);


}





void network_test(void)
{            

    char test_char[8016] = {0};
    uint32_t i = 0;
    uint32_t j = 0;
    

    PacketHead* pmsgtest = (PacketHead*)test_char;
    

    pmsgtest->msgHead.ulSendSubSysId = 0x00002000;
    pmsgtest->msgHead.ulRcvdSubSysId = 0x00001000;
    pmsgtest->msgHead.ulMsgType = 0xFFFFFFFF;
    pmsgtest->msgHead.ulMsgLength = TEST_DATA_LEN;
    pmsgtest->packetID = 0;

    for(i = 20;i<8016;i++)
    {
        test_char[i] = i-20;
    }
    
//    print_data_hex(test_char,16+16);
    server_init();


    
    client_init(0,LOCAL_CLIENT_PORT);
    client_init(1,LOCAL_CLIENT_PORT);
    client_init(2,LOCAL_CLIENT_PORT);


g_ClientNetSession[0].SetTcpHBPara(1, 10, 5, 3);
g_ClientNetSession[1].SetTcpHBPara(1, 10, 5, 3);
//g_ClientNetSession[2].SetTcpHBPara(1, 10, 5, 3);


return;

while(1)
{
    pf_usleep(5000000);
    if(false == client_send_data(0,test_char, pmsgtest->msgHead.ulMsgLength+16))
    {
        NAS_PrintLog(LOG_INFO, "\r\n  client0_send_data fail");
    }
    else
    {
        NAS_PrintLog(LOG_INFO, "\r\n  client0_send_data OK ");
    }
}

return;

    i = 0;
    pmsgtest->packetID = 1;

while(1)
{

    if(gtmpconnection == false)
    {
        pf_usleep(1000000);
        j++;
        NAS_PrintLog(LOG_INFO, "\r\n  client_init_%d , send_%d ",j,i);
        if(true == client_init(0,LOCAL_CLIENT_PORT))
        {
            j = 0;
        }

        
    }
    else
    {
//        pf_usleep(1000000);
        i++;
//        sprintf(test_char,"testdata_%d",i);

        if(false == client_send_data(0,test_char, pmsgtest->msgHead.ulMsgLength+16))
        {
 //           gtmpconnection = false;
            j++;
        }
        else
        {
            if(j>0)
            {
                NAS_PrintLog(LOG_INFO, "\r\n  send_fail_%d , send_total_%d , id_0x%08x , len_%d ",j,i, pmsgtest->packetID,pmsgtest->msgHead.ulMsgLength);
                j = 0;

{

int s32cpuload = 0;
int s32sysload = 0;
stru_mem_stat stMemStat = {0};
S32 s32ProcCpuMil = 0;

pf_stat_get_cpu_load(&s32cpuload,&s32sysload);
pf_stat_get_mem_info(&stMemStat);
pf_stat_get_process_cpu(&s32ProcCpuMil);

NAS_PrintLog(LOG_INFO, "\r\n\r\n  s32cpuload_%02d\% , s32sysload_%02d\%, s32ProcCpuMil_%d ",s32cpuload,s32sysload,s32ProcCpuMil);
NAS_PrintLog(LOG_INFO, "\r\n  mem_total_%lu , mem_used_%lu, mem_free_%lu ",stMemStat.mem_total,stMemStat.mem_used,stMemStat.mem_free);
NAS_PrintLog(LOG_INFO, "\r\n  mem_total_0x%lx , mem_used_0x%lx, mem_free_0x%lx \r\n",stMemStat.mem_total,stMemStat.mem_used,stMemStat.mem_free);
}

                
            }
            pmsgtest->packetID++;
            
if((pmsgtest->packetID%3) == 0)
{
    pmsgtest->msgHead.ulMsgLength = 37;
}
else
{
    pmsgtest->msgHead.ulMsgLength = TEST_DATA_LEN;
}
            
        }

        if(j>0)
        {
 //           NAS_PrintLog(LOG_INFO, "\r\n  wait_1s_%d , send_%d ",j,i);
 //           pf_usleep(1000000);
        }

if((pmsgtest->packetID%(1024*1024)) == 1)
{

int s32cpuload = 0;
int s32sysload = 0;
stru_mem_stat stMemStat = {0};
S32 s32ProcCpuMil = 0;

pf_stat_get_cpu_load(&s32cpuload,&s32sysload);
pf_stat_get_mem_info(&stMemStat);
pf_stat_get_process_cpu(&s32ProcCpuMil);

NAS_PrintLog(LOG_INFO, "\r\n\r\n  s32cpuload_%02d\% , s32sysload_%02d\%, s32ProcCpuMil_%d ",s32cpuload,s32sysload,s32ProcCpuMil);
NAS_PrintLog(LOG_INFO, "\r\n  mem_total_%lu , mem_used_%lu, mem_free_%lu ",stMemStat.mem_total,stMemStat.mem_used,stMemStat.mem_free);
NAS_PrintLog(LOG_INFO, "\r\n  mem_total_0x%lx , mem_used_0x%lx, mem_free_0x%lx \r\n",stMemStat.mem_total,stMemStat.mem_used,stMemStat.mem_free);
}

       
    }
    

}


return;
    
    
    if( true ==   client_init(0,LOCAL_CLIENT_PORT))
    {
        uint8_t pDataOut[2048] = {0};
        uint8_t test_data[256] = {0};
        uint32_t DataOut_len = 0;
        int i=0;
        
        for(i = sizeof(stMsgHeader);i<(256-sizeof(stMsgHeader));i++)
        {
            test_data[i] = i-sizeof(stMsgHeader);
        }
        
        stMsgHeader* pmsgtest = (stMsgHeader*)test_data;
        
        i = 1;

        pmsgtest->ulRcvdSubSysId = 0x12345678;
        pmsgtest->ulSendSubSysId = 0x87654321;
        pmsgtest->ulMsgType = 0xa0000000;
        pmsgtest->ulMsgLength = 64;


client_send_count = 0;
client_sendfail_count = 0;
server_receive_count = 0;

pf_usleep(1000000);




client_send_data(0,pacet_test_data, 571);


//g_ClientNetSession.DisConnect();

pf_usleep(1000000);
if(g_Server_client_conn[0] != NULL)
{
g_ServerNetSession.DeleteClient(g_Server_client_conn[0]);
}




while(1)
{

    i++;
    pf_usleep(1000000);

	//printf("\r\n client_init_%d \r\n",i);

    if(true == client_init(0,LOCAL_CLIENT_PORT))
    {
        break;
    }
    
}

return;


while(1)
{
        pmsgtest->ulMsgType++;
        pmsgtest->ulMsgLength++;

        if(pmsgtest->ulMsgLength == 80)
        {
            pmsgtest->ulMsgLength = 64;
        }
        
//        DataOut_len = 0;
        DataOut_len += send_data_subpacket(i++, test_data, sizeof(stMsgHeader) + pmsgtest->ulMsgLength, 53 ,pDataOut+DataOut_len);

        NAS_PrintLog(LOG_INFO, "\r\n  DataOut_len_%d , ulMsgLength_%d \r\n",DataOut_len , pmsgtest->ulMsgLength);

        client_send_data(0,pDataOut, DataOut_len);



if(client_send_count >= 13)
{
return;
}

}


        
        pmsgtest->ulMsgLength = 137;
        DataOut_len += send_data_subpacket(i++,test_data, sizeof(stMsgHeader) + pmsgtest->ulMsgLength, 57 ,pDataOut+DataOut_len);

//client_send_data_subpacket(0,pDataOut, DataOut_len, 53);


        NAS_PrintLog(LOG_INFO, "\r\n  DataOut_len_%d \r\n",DataOut_len);




        client_send_data(0,pDataOut, 217);

//        client_send_data(0,pDataOut, 347);


//        client_send_data(0,pDataOut+217, DataOut_len-217);

        client_send_data(0,pDataOut+313, DataOut_len-313);
        
//        client_send_data(0,test_data, sizeof(stMsgHeader) + pmsgtest->ulMsgLength);

    }

}                                                                          

//#undef NAS_PrintLog(level, format, ...)
//#define NAS_PrintLog(level, format, ...) pl_log(level,format, ## __VA_ARGS__)



