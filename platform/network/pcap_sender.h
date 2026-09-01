#ifndef __ARP_SENDER_H__
#define __ARP_SENDER_H__

#include "net_lib.h"
#include "platform_net_tools.h"
#include "option.h"

class PCapSender;

class IPcapRecv
{
public:
    virtual ~IPcapRecv(){};
    virtual void OnRecvIPv4( const uint8_t* data , uint32_t size )=0;

    virtual void OnRecvArp( const uint8_t* data , uint32_t size )=0;
};

class CPcapSender
{
    friend class PCapSender;
public:
    static CPcapSender* CreateConnection( IPcapRecv* session ,const IpInformation& info, const std::string& gateway );
    static void DestroyConnection(CPcapSender*pArpSender);
    
private:
    CPcapSender();
    ~CPcapSender();
public:
    bool SendArp( const uint8_t* data , uint32_t size  );
    bool SendIpv4( const uint8_t* data , uint32_t size  );
    bool SendRaw( const uint8_t* data , uint32_t size );
    void UpdateRemoteArp(const uint8_t* data , uint32_t size);
    void Close();
    void UpdateGwAddress(uint32_t ulGatewayIp);
private:
    bool CreatePCapSender( IPcapRecv* session , const IpInformation& info, const std::string& gateway  );

    PCapSender* m_pCapSender;
};


#endif //__ARP_SENDER_H__