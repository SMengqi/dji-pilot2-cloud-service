#define THIS_MODULE  MODULE_NETWORK
#include "option.h"

#ifndef WPCAP
#define WPCAP
#endif

#ifndef HAVE_REMOTE
#define HAVE_REMOTE
#endif

#include <pcap.h>
#include <map>

#include "pcap_sender.h"
#include "platform_net_tools.h"
#include "platform_thread.h"
#include "net_arp.h"
#include "net_ipv4.h"
#include "net_lib.h"
#include "ipv4address.h"
#include "net_buffer.h"
#include "platform_mutex.h"
#include "scoped_lock.h"
#include "module.h"
#include "pf_timer.h"
#include "platform.h"

#define CAPTURE_SIZE 65535

#define READ_TIMEOUT 10

#define ARP_DURATION 1000

#ifndef PCAP_OPENFLAG_PROMISCUOUS
#define PCAP_OPENFLAG_PROMISCUOUS 1
#endif

enum PcapTimerType
{
    PcapTimerType_ArpRequest = 0,
};

struct PcapTimerParam
{
    PCapSender* pCapSender;
    uint32_t value;
    uint32_t timerType;
};

void register_arp_timer( PCapSender* pSender , uint32_t daddr );

class PCapSender:public INetSession
{
public:

    static void pcap_entry( void* param )
    {
        PCapSender* pSender = (PCapSender*)param;
        pSender->RunPcapRecv();
    }

    static void processPacket(unsigned char *param,const struct pcap_pkthdr *header, const unsigned char *data)
    {
        PCapSender* pSender = (PCapSender*)param;
        pSender->OnRecv( (const uint8_t*)data , header->len );
    }

    virtual void OnRecv( IClientConnection*  , const uint8_t* data , uint32_t size , const SessionData& sessionData  )
    {
        if( size > ETH_HEADER_LENGTH )
        {
            if( memcmp( m_IpInformation.m_mac , data + ETH_ALEN , 6 ) != 0 )
            {
                m_pNetSession->OnRecvIPv4( data + ETH_HEADER_LENGTH , size - ETH_HEADER_LENGTH );
            }
        }
    }

    virtual void OnClose( IClientConnection*  )
    {

    }

    PCapSender()
    {
        m_pCap = NULL;
        m_pNetSession = NULL;
        memset( m_mac , 0 , sizeof(m_mac) );
        memset( m_macipv4 , 0 , sizeof(m_macipv4) );
        m_gwAddress = 0;
        m_IpInformation = {"", 0, "", 0, 0, "", 0};
//        m_gwNetmask = 0;
//        m_gwNetwork = 0;
    }
public:
    bool CreateSender( IPcapRecv *session , IpInformation info,
        const char* devicename , const std::string& gateway )
    {
        char errbuf[PCAP_ERRBUF_SIZE]={0};
        /* Open the output device */
        m_pCap = pcap_open_live(devicename,            // name of the device
            CAPTURE_SIZE,                // portion of the packet to capture (only the first 100 bytes)
            PCAP_OPENFLAG_PROMISCUOUS,  // promiscuous mode
            READ_TIMEOUT,               // read timeout
            errbuf);              // error buffer

        if( NULL == m_pCap )
        {
            NAS_PrintLog( LOG_ERROR , "pcap_open_live return error(%s) in CreateSender.\n" , errbuf );
            return false;
        }

        struct bpf_program fcode;

        bpf_u_int32 netmask = 0xffffffff;

        //ether dst ehost
             uint8_t szcompile[256]={0};
        sprintf( (char*)szcompile , "ether dst %.2x:%.2x:%.2x:%.2x:%.2x:%.2x or arp",
                info.m_mac[0],info.m_mac[1],info.m_mac[2],info.m_mac[3],info.m_mac[4],info.m_mac[5] );
        if ( pcap_compile(m_pCap, &fcode, (char*)szcompile, 1, netmask) < 0 ) 
        {
            NAS_PrintLog( LOG_ERROR , "pcap_compile return error in CreateSender.\n"  );
            pcap_close( m_pCap );
            m_pCap = NULL;
            return false;
        }

        NAS_PrintLog(LOG_INFO, "%s pcap_setfilter=\"%s\"", __FUNCTION__, szcompile);
        pcap_setfilter( m_pCap , &fcode);

        if( gateway == info.m_ip )
        {
            ArpInformation arp;
            arp.dwAddr = info.m_ipValue;
            memcpy( arp.physAddr , info.m_mac , 6 );
                    AddArpMap(info.m_ipValue, info.m_mac);
            UpdateArp( arp );
        }

        m_IpInformation = info;
        uint8_t gatewaymac[ETH_ALEN]={0};
        m_gwAddress = IPv4Address::ConvertToInteger( gateway.c_str() );
//        m_gwNetmask = info.m_maskValue;
//        m_gwNetwork = m_gwAddress & m_gwNetmask;

        UpdateArp();

        uint8_t *dstMac;
            dstMac = FindArpMap(m_gwAddress);
            if (NULL != dstMac)
        {
            memcpy( gatewaymac , dstMac, ETH_ALEN );
        }

        m_pNetSession = session;
        memcpy( m_mac , info.m_mac , ETH_ALEN );

        //init ipv4 mac header
        memcpy( m_macipv4 , gatewaymac , ETH_ALEN );
        memcpy( m_macipv4 + ETH_ALEN , m_mac , ETH_ALEN );
        uint16_t value = ETH_TYPE_IPV4;
        m_macipv4[ETH_ALEN*2] = (uint8_t)(value >> 8);
        m_macipv4[ETH_ALEN*2+1] = (uint8_t)(value); 


        /*SessionData sessiondata;
        sessiondata.m_LocalIP = info.m_ip;
        IClientConnection* rawconn = NetFactory::GetInstance()->CreateRawIpConnection( this , sessiondata );
        if( rawconn == NULL )
        {
            NAS_PrintLog( LOG_ERROR , "create raw socket failed.\n"  );
        }*/
        ThreadData data;

        data.threadfunc = pcap_entry;
        data.param = this;
        data.module = 0;
        PlatformRunThread( data );

        return true;
    }

    bool Send( const uint8_t* data , uint32_t size )
    {
        if( m_pCap )
        {
            if (pcap_sendpacket(m_pCap, data, size ) == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    //pcap_sendqueue_transmit
    bool Send( const uint8_t* data , uint32_t size, Ethernet_Frame_Types type )
    {
        if( m_pCap )
        {
            uint8_t* macdata = (uint8_t*)data;
            uint32_t allsize = size;
                    uint8_t* dstMac = NULL;
            if( type == ETH_TYPE_IPV4 )
            {
                macdata -=  ETH_HEADER_LENGTH ;
                allsize += ETH_HEADER_LENGTH;
                uint32_t dstAddr = Ipv4_GetDstAddress( data , size );

/*                if( ( dstAddr &  m_gwNetmask ) != m_gwNetwork )
                {
                    dstAddr = m_gwAddress;
                }*/
                        if((dstAddr & m_IpInformation.m_maskValue)
                            != (m_IpInformation.m_ipValue & m_IpInformation.m_maskValue))
                {
                    dstAddr = m_gwAddress;
                }

                memcpy( macdata , m_macipv4 , ETH_HEADER_LENGTH );
                         dstMac = FindArpMap(dstAddr);
                         if (NULL != dstMac)
                {
                    memcpy( macdata , dstMac, ETH_ALEN );
                }
                else
                {
                    NAS_PrintLog(LOG_UINFO, "PCapSender::Send, BufferPacket for no arp info for IP %s", 
                                    inet_ntoa(*(struct in_addr*)&(dstAddr)));
                    PacketBuffer( dstAddr , macdata , size + ETH_HEADER_LENGTH );
                    return true;
                }
            }
            
            if (pcap_sendpacket(m_pCap, macdata, allsize ) == 0)
            {
                     NAS_PrintLog(LOG_ERROR, "PCapSender::Send,  pcap_sendpacket success");
                return true;
            }
            else
            {
                NAS_PrintLog(LOG_ERROR, "PCapSender::Send,  pcap_sendpacket FAIL");
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    bool ClearBufferPacket( uint32_t daddr )
    {
        ScopedLock<PlatformMutex> lock(m_buffer_mutex);
        std::map<uint32_t,CBuffer*>::iterator itor =  m_ipbufferMap.find( daddr );
        if( itor != m_ipbufferMap.end() )
        {
            delete itor->second;
            m_ipbufferMap.erase( itor );
        }
        return true;
    }

    void PacketBuffer( uint32_t daddr , const uint8_t* data , uint32_t size )
    {
        ScopedLock<PlatformMutex> lock(m_buffer_mutex);
        std::map<uint32_t,CBuffer*>::iterator itor =  m_ipbufferMap.find( daddr );
        if( itor != m_ipbufferMap.end() )
        {
            itor->second->AddSizeBuffer( data , size );
        }
        else
        {
            CBuffer* pBuffer = new CBuffer( 10 * 1024 );
            pBuffer->AddSizeBuffer( data , size );
            m_ipbufferMap[daddr] = pBuffer;
            SendArp( daddr );
        }
    }

    bool HavePacketBuffer( uint32_t daddr )
    {
        ScopedLock<PlatformMutex> lock(m_buffer_mutex);
        std::map<uint32_t,CBuffer*>::iterator itor =  m_ipbufferMap.find( daddr );
        if( itor != m_ipbufferMap.end() )
        {
            return true;
        }
        return false;
    }

    void OnRecv( const uint8_t* data , uint32_t size )
    {
        if( size <= ETH_HEADER_LENGTH )
        {
            return;
        }

        uint16_t eth_type = data[ETH_ALEN*2]<<8 | data [ETH_ALEN*2+1];

        switch( eth_type )
        {
        case ETH_TYPE_IPV4:
            {
                m_pNetSession->OnRecvIPv4( data + ETH_HEADER_LENGTH , size - ETH_HEADER_LENGTH  );
            }
            break;
        case ETH_TYPE_ARP:
            {
                ArpInformation info;
                uint32_t arpType = Arp_Request;

                if( false == ParserArpSenderAddress( data , size , &info.dwAddr , info.physAddr , &arpType ) )
                {
                    return ;
                }

                if( arpType == Arp_Response )
                {
                        AddArpMap(info.dwAddr, info.physAddr);
                    UpdateArp( info );
                }
                else
                {
                    m_pNetSession->OnRecvArp( data , size );
                }
            }
            break;
        }

    }

    void RunPcapRecv()
    {
        pcap_loop(m_pCap, -1 , processPacket,(u_char*)this );
    }

    void Close()
    {
        pcap_close( m_pCap );
    }

    void UpdateGwAddress(uint32_t ulGatewayIp)
    {
        m_gwAddress = ulGatewayIp;
    }
    
    void UpdateRemoteArp(const uint8_t *pucData, uint32_t ulSize)
    {
        uint32_t arpType = Arp_Request;
        ArpInformation info;
        
        if( false == ParserArpSenderAddress(pucData , ulSize , &info.dwAddr , info.physAddr , &arpType))
        {
            return ;
        }

        if( arpType == Arp_Request)
        {
            AddArpMap(info.dwAddr, info.physAddr);
        }
        
        return;
    }
    
private:
    bool SendArp( uint32_t targetIP )
    {
        MacAddr addr( m_IpInformation.m_mac );

        CArpSender arp( addr );

        std::string strTargetIP = IPv4Address::ConvertToString( targetIP );
        uint8_t outPacket[256]={0};
        uint32_t outPacketLength = sizeof( outPacket );
        NAS_PrintLog(LOG_INFO, "%s: targetIP=%s", __FUNCTION__, inet_ntoa(*(struct in_addr*)&(targetIP))); //说明一次打印多个IP时，不能使用inet_ntoa
        if( true == arp.GenerateArpRequest( m_IpInformation.m_ip , strTargetIP , outPacket , outPacketLength ) )
        {
            NAS_PrintLog(LOG_INFO, "GenerateArpRequest targetIP=%s", inet_ntoa(*(struct in_addr*)&(targetIP)));
            pcap_sendpacket(m_pCap, outPacket, outPacketLength );
            register_arp_timer( this , targetIP );
            return true;
        }
        return false;
    }

    void UpdateArp()    //仅初始化时使用，故未加锁
    {
        std::list<ArpInformation> arplist;
        GetArpTableInformation( arplist );
        {
            ScopedLock<PlatformMutex> lock(m_arp_mutex);                
            std::list<ArpInformation>::iterator itor = arplist.begin();
            for( ; itor != arplist.end(); itor++ )
            {
                std::string ip = IPv4Address::ConvertToString( itor->dwAddr );
                m_arpMap[ itor->dwAddr ] = MacAddr( &itor->physAddr[0] );
            }
        }
    }

    uint8_t *FindArpMap(uint32_t ipAddr)
    {
        ScopedLock<PlatformMutex> lock(m_arp_mutex);
        std::map<uint32_t,MacAddr>::iterator itor = m_arpMap.find(ipAddr);
        if (itor != m_arpMap.end())
        {
            return (itor->second.m_mac);
        }
        else
        {
            return NULL;
        }
    }

    void AddArpMap(uint32_t ipAddr, uint8_t *macAddr)
    {
        {
            ScopedLock<PlatformMutex> lock(m_arp_mutex);
            m_arpMap[ipAddr] = MacAddr(macAddr);
        }
        NAS_PrintLog(LOG_INFO, "%s: IP=%s, MAC=%02x:%02x:%02x:%02x:%02x:%02x",
            __FUNCTION__, inet_ntoa(*(struct in_addr*)&(ipAddr)),
            macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    }

    void UpdateArp( const ArpInformation& info )
    {
        ScopedLock<PlatformMutex> lock(m_buffer_mutex);

        std::map<uint32_t,CBuffer*>::iterator itor = m_ipbufferMap.find( info.dwAddr );
        if( itor != m_ipbufferMap.end() )
        {
            CBuffer* pBuffer = itor->second;
            while( pBuffer )
            {
                uint32_t size = pBuffer->GetSizeBufferLength();
                if( size == 0 )
                    break;

                uint8_t* buf = pBuffer->GetSizeBuffer();


                memcpy( buf  , info.physAddr , ETH_ALEN );

                pcap_sendpacket(m_pCap, buf, size );

                pBuffer->ReleaseSizeBuffer( size );
            }
            delete pBuffer;
            m_ipbufferMap.erase( itor );
        }
    }
private:
    pcap_t * m_pCap;
    IPcapRecv* m_pNetSession;
    uint8_t m_mac[6];
    uint8_t m_macipv4[14];
    IpInformation m_IpInformation;
    uint32_t m_gwAddress;
//    uint32_t m_gwNetmask;
//    uint32_t m_gwNetwork;

        PlatformMutex m_arp_mutex;      //针对m_arpMap的互斥保护
        std::map<uint32_t,MacAddr> m_arpMap;
    PlatformMutex m_buffer_mutex;      //针对m_ipbufferMap的互斥保护    
    std::map<uint32_t,CBuffer*> m_ipbufferMap;
};

void arp_init()
{
}

void arp_entry( uint16_t srcModuleId, uint16_t msgId, uint16_t dstModuleId, void* msgData, uint16_t msgLength )
{
    if( msgId == TIMER_EXPIRY_MSG )
    {
        TIMER_EXPIRE_MSG_S* pMsg = (TIMER_EXPIRE_MSG_S*)msgData;
        PcapTimerParam* pParam = (PcapTimerParam*)pMsg->ulPara;
        
        switch( pParam->timerType )
        {
        case PcapTimerType_ArpRequest:
            pParam->pCapSender->ClearBufferPacket( pParam->value );
            break;
        }
        NAS_MemoryFree( pParam );
    }
    return ;
}

void register_arp_timer( PCapSender* pSender , uint32_t daddr )
{
    PcapTimerParam* param = (PcapTimerParam*)NAS_MemoryMalloc( sizeof(PcapTimerParam) );
    if(NULL == param)
    {
        NAS_PrintLog(LOG_ERROR, "NAS_MemoryMalloc fail, param is empty");
        return ;
    }
    param->pCapSender = pSender;
    param->value = daddr;
    param->timerType = PcapTimerType_ArpRequest;

    static U32 tmpTimerId=0;
    if( 0 != NAS_TimerStart(THIS_MODULE, ARP_DURATION,0,(U32)param , &tmpTimerId ) )
    {
        NAS_PrintLog( LOG_ERROR , "Failed to create Arp request timer .\n" );
    }
    
}

CPcapSender* CPcapSender::CreateConnection( IPcapRecv* session , const IpInformation& info, const std::string& gateway )
{
    CPcapSender* pArpSender = new CPcapSender;
    if( false == pArpSender->CreatePCapSender( session , info, gateway ) )
    {
        delete pArpSender;
        pArpSender = NULL;
    }

    if( NULL != pArpSender )
    {

    }
    return pArpSender;
}

void CPcapSender::DestroyConnection(CPcapSender*pArpSender)
{
        pArpSender->Close();
        delete pArpSender;
}
CPcapSender::CPcapSender()
{
    m_pCapSender = new PCapSender();
}

CPcapSender::~CPcapSender()
{
    if( NULL != m_pCapSender )
    {
        delete m_pCapSender;
        m_pCapSender = NULL;
    }
}

bool CPcapSender::CreatePCapSender(IPcapRecv *session, const IpInformation& info , const std::string& gateway )
{
    if( m_pCapSender )
    {
            NAS_PrintLog( LOG_INFO , "ip:%s,mask:%s,mac:%.2d.%.2d.%.2d.%.2d.%.2d.%.2d,intf name:%s,gw:%s in CreatePCapSender.\n" ,
            info.m_ip.c_str() , info.m_mask.c_str() , info.m_mac[0],info.m_mac[1],info.m_mac[2],info.m_mac[3],info.m_mac[4],
            info.m_mac[5], info.m_interfaceName.c_str() , gateway.c_str() );

        std::string devicename;

        pcap_if_t *alldevs;
        char errbuf[PCAP_ERRBUF_SIZE]={0};
        if (pcap_findalldevs(&alldevs, errbuf) == -1)
        {
            NAS_PrintLog( LOG_ERROR , "pcap_findalldevs return error(%s) in CreatePCapSender.\n" , errbuf );
            return false;
        }

        /* Print the list */ 
        for(pcap_if_t * d= alldevs; d != NULL; d= d->next)
        {
            NAS_PrintLog( LOG_INFO , "network devicename:%s\n" , d->name );
            if( NULL != strstr( d->name , info.m_interfaceName.c_str() ) )
            {
                devicename = d->name;
                break;
            }
        }

        if( true == devicename.empty() )
        {
            NAS_PrintLog( LOG_ERROR , "failed to find network device %s in CreatePCapSender.\n" , errbuf );
            return false;
        }

        return m_pCapSender->CreateSender( session ,info , devicename.c_str(), gateway );
    }
    else
    {
        return false;
    }
}

bool CPcapSender::SendArp( const uint8_t* data , uint32_t size )
{
    if( m_pCapSender )
    {
        return m_pCapSender->Send( data , size , ETH_TYPE_ARP );

    }
    else
    {
        return false;
    }
}

bool CPcapSender::SendIpv4(  const uint8_t* data , uint32_t size )
{
    if( m_pCapSender )
    {
        return m_pCapSender->Send( data , size , ETH_TYPE_IPV4 );

    }
    else
    {
        return false;
    }
}

bool CPcapSender::SendRaw( const uint8_t* data , uint32_t size )
{
    if( m_pCapSender )
    {
        return m_pCapSender->Send( data , size );
    }
    else
    {
        return false;
    }
}

void CPcapSender::UpdateRemoteArp(const uint8_t* pucData, uint32_t ulSize)
{
    m_pCapSender->UpdateRemoteArp(pucData, ulSize);
    return;
}

void CPcapSender::Close()
{
    if( m_pCapSender )
    {
        m_pCapSender->Close();
    }
}
void CPcapSender::UpdateGwAddress(uint32_t ulGatewayIp)
{
    if( m_pCapSender )
    {
        m_pCapSender->UpdateGwAddress(ulGatewayIp);
    }
}
