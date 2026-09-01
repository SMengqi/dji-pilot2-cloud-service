#ifndef __PLATFORM_NETSESSION_CLIENT_H__
#define __PLATFORM_NETSESSION_CLIENT_H__
#include <net_lib.h>
 


class ClientNetSession:public INetSession
{
    public:
        ClientNetSession();
        ~ClientNetSession();
        
    private:
        virtual void OnRecv(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData);
        virtual void OnClose(IClientConnection* conn);
        
    public:
        bool InitNetSession( NetProtocol protocol, const SessionData& sessionData , func_receive receive_func , func_disconnect disconnect_func);
        bool Send( void* data , uint32_t size  );
        bool Send( void* data , uint32_t size , uint32_t packet_max_len);
        void DisConnect(void);
        bool Netsession_isvalid(void);
        bool SetTcpHBPara( int heartbeat_en, int idle, int cnt, int intv);
        void SetSessionMode( SessionMode mode );
        void SetBufferSize( int nSize);
        void SetTcpMemSize( int nSize); 
        IClientConnection* m_pNetsession; 
        uint64_t u64data;
        uint32_t u32sessionmode;
        uint32_t u32devId;
       
    private:
        bool isvalid;
        uint32_t u32sendID;
        uint32_t u32receiveID;
        uint32_t u32receivecount;
        uint8_t* pu8receivebuf;
        uint8_t* pu8halfpackbuf;
        uint32_t u32halfpacklen;
        uint32_t u32protocol;
        func_receive receive_func_call;
        func_disconnect disconnect_func_call;
        uint32_t CombineSubpacket(const uint8_t* pMessage);
        void CheckPacket(IClientConnection* conn, const uint8_t* pMessage, uint32_t messageSize, const SessionData& sessionData);
        PF_MUTEX_T client_mutex;
        int32_t s32findMsghead;

        
};

#endif //__PLATFORM_NETSESSION_CLIENT_H__

