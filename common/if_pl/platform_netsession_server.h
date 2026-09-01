#ifndef __PLATFORM_NETSESSION_SERVER_H__
#define __PLATFORM_NETSESSION_SERVER_H__
#include <net_lib.h>

#define MAX_CLIENT_NUM 1024

struct stRecvbuf
{
    uint8_t*     pu8receivebuf;
    uint32_t     u32receivecount;
    uint32_t     u32receiveID;  

    int32_t      S32count;
    uint32_t     S32len;
};


struct stClientInfo
{
    stRecvbuf    astClientRecv[2];
    uint8_t*     pu8halfpackbuf;
    uint32_t     u32halfpacklen;
    uint16_t     u16ClientPort;   
    uint16_t     u16ClientIndex;  

    uint32_t     u32okcount;
    uint32_t     u32failcount;
    
    char         acClientIP[24];    
    int32_t      S32halfcount;
    uint32_t     S32halflen;
};



class ServerNetSession:public INetSession
{
    public:
        
        ServerNetSession();
        ~ServerNetSession();

        virtual void OnRecv(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData);
        virtual void OnClose(IClientConnection* conn);
        virtual void OnAccept(IClientConnection* conn, const SessionData& sessionData);

        bool InitNetSession(NetProtocol protocol, const SessionData& sessionData, func_receive receive_func, func_accept accept_func, func_disconnect disconnect_func);
        bool Send( IClientConnection* conn, uint8_t* data , uint32_t size );
        bool Send( IClientConnection* conn, uint8_t* data , uint32_t size, uint32_t packet_max_len );

        bool Send( uint8_t* data , uint32_t size , const SessionData& sessionData );
        bool Send( uint8_t* data , uint32_t size , const SessionData& sessionData, uint32_t packet_max_len );

        void DeleteClient(IClientConnection* conn);
        void DeleteClient( char* cpclient_IP,  uint16_t client_port);

        void CloseServer(void);
        bool Netsession_isvalid(void);
        bool SetTcpHBPara(IClientConnection* conn, int heartbeat_en, int idle, int cnt, int intv);
        void SetSessionMode( SessionMode mode );
        void SetBufferSize( int nSize);
        void SetTcpMemSize( int nSize);
        void mem_check(int client_i);
        
        uint64_t u64data;
        uint32_t u32sessionmode;
        uint32_t u32devId;
        
        
    private:

        bool isvalid;
        uint32_t u32protocol;
        uint32_t u32sendID;
        uint32_t u32clientCount;
        stClientInfo astClientInfo[MAX_CLIENT_NUM];
        
        IServerConnection* pServerConnection;
        func_receive receive_func_call;
        func_accept accept_func_call;
        func_disconnect disconnect_func_call;
        ConnHandle  server_udp_handle;
        
        uint32_t CombineSubpacket(uint32_t client_index ,const uint8_t* pMessage, uint32_t* recv_index);
        void CheckPacket(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData);
        PF_MUTEX_T server_mutex;
        int32_t s32findMsghead;

};

#endif //__PLATFORM_NETSESSION_SERVER_H__

