#ifndef __NET_ARP_H__
#define __NET_ARP_H__
#include <stdint.h>
#include <string>
#include <string.h>
#include "pl.h"

#define ETH_ALEN 6

#define ETH_HEADER_LENGTH 14

#define IPV4_HEADER_LENGTH 20

#define ICMP_HEADER_LENGTH 8

#define ARP_HEADER_LENGTH 28

#define ARP_PRO_OFFSET        2  
#define ARP_HLN_OFFSET        4  
#define ARP_PLN_OFFSET        5  
#define ARP_OP_OFFSET        6  
#define ARP_SHA_OFFSET        8  
#define ARP_SPA_OFFSET        14 
#define ARP_THA_OFFSET        18 
#define ARP_TPA_OFFSET        24 


enum Ethernet_Frame_Types
{
    ETH_PPPOE_DISCOVERY = 0x8863,
    ETH_PPPOE_SESSION   = 0x8864,
    ETH_TYPE_IPV4        = 0x0800,
    ETH_TYPE_IPV6        = 0x86DD,
    ETH_TYPE_ARP        = 0x0806,
};


enum Arp_Message_Type
{
    Arp_Request          = 0x01,
    Arp_Response      = 0x02
};

struct ARPHdr
{
    uint16_t arp_hrd; 
    uint16_t arp_pro; 
    uint8_t arp_hln; 
    uint8_t arp_pln; 
    uint16_t arp_op;
    uint8_t arp_sha[6]; 
    uint32_t arp_spa; 
    uint8_t arp_tha[6]; 
    uint32_t arp_tpa;    

};

struct MacAddr
{
    uint8_t m_mac[ETH_ALEN];
    MacAddr()
    {
        for( uint8_t index = 0 ; index < ETH_ALEN; index++ )
        {
            m_mac[index] = 0;
        }
    }
    MacAddr( const uint8_t* mac )
    {
        for( uint8_t index = 0 ; index < ETH_ALEN; index++ )
        {
            m_mac[index] = mac[index];
        }
    }
};



class CArpSender
{
public:
    CArpSender( const MacAddr& mac );
    ~CArpSender();
public:
    bool FakeArpResponse( const uint8_t* arpPacket , uint32_t arpPacketLength , uint8_t* outPacket , uint32_t& outPacketLength  );

    bool GenerateArpRequest( const std::string& sourceIP , const std::string& targetIP , uint8_t* outPacket , uint32_t& outPacketLength );
    
    void GenerateRawArpRequest(uint8_t *pucSenderMac, uint32_t ulSenderIp, uint32_t ulTargetIp, uint8_t *pucData, uint32_t *pulDataLen);
private:
    MacAddr m_macAddr;
};


bool ParseArpTargetAddress( const uint8_t* data , uint32_t arpPacketLength, 
                           uint32_t& targetIP , uint32_t& arpType );

bool ParserArpSenderAddress( const uint8_t* data , uint32_t arpPacketLength, 
                            uint32_t* senderIP , uint8_t* senderMac , uint32_t* arpType );


#endif //__NET_ARP_H__
