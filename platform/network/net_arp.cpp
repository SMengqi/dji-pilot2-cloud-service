#define THIS_MODULE MODULE_NETWORK
#include <string.h>
#include <stdio.h>
#include "net_arp.h"
#include "ipv4address.h"

CArpSender::CArpSender( const MacAddr &mac )
{
    m_macAddr = mac;
}

CArpSender::~CArpSender()
{

}

bool CArpSender::FakeArpResponse( const uint8_t* arpPacket , uint32_t arpPacketLength , 
                                   uint8_t* outPacket , uint32_t& outPacketLength  )
{
    if( outPacketLength < ETH_HEADER_LENGTH + ARP_HEADER_LENGTH )
        return false;
    if( arpPacketLength < ETH_HEADER_LENGTH + ARP_HEADER_LENGTH )
        return false;

    const uint8_t* data = arpPacket + ETH_HEADER_LENGTH;


    uint16_t protocoltype = data[ARP_PRO_OFFSET] << 8 | data[ARP_PRO_OFFSET+1];

    if( protocoltype != ETH_TYPE_IPV4 )
    {
        PS_CPlus(CM_NES, CMNES_ID_ARP_RSP_TYPE_FAIL);
        return false;
    }

        
        
    if( data[ARP_HLN_OFFSET] != ETH_ALEN || 
        data[ARP_PLN_OFFSET] != 4 ||        
        data[ARP_OP_OFFSET] != 0 || 
        data[ARP_OP_OFFSET+1] != 1 )
    {
        PS_CPlus(CM_NES, CMNES_ID_ARP_RSP_OFFSET_FAIL);
        return false;
    }
    
    uint8_t* dst = 0;
    const uint8_t *src = 0;
    uint8_t* start = outPacket;

    src = data + ARP_TPA_OFFSET;
    
    
    memcpy( start , arpPacket , ARP_HEADER_LENGTH + ETH_HEADER_LENGTH );
    
    
    //将arp查询的源eth地址填写到目的eth地址
    dst = start; 
    src = arpPacket + ETH_ALEN;
    dst[0]=src[0];
    dst[1]=src[1];
    dst[2]=src[2];
    dst[3]=src[3];
    dst[4]=src[4];
    dst[5]=src[5];
    
    
    //将本地eth地址填写到源eth地址
    dst = start + ETH_ALEN;
    dst[0]=m_macAddr.m_mac[0];
    dst[1]=m_macAddr.m_mac[1];
    dst[2]=m_macAddr.m_mac[2];
    dst[3]=m_macAddr.m_mac[3];
    dst[4]=m_macAddr.m_mac[4];
    dst[5]=m_macAddr.m_mac[5];


    uint8_t* arp = start + ETH_HEADER_LENGTH;
        //填写Arp Opcode
    arp[7] = 2;

    //将本地eth地址填写到sender eth地址
    dst = arp + ARP_SHA_OFFSET;
    dst[0]=m_macAddr.m_mac[0];
    dst[1]=m_macAddr.m_mac[1];
    dst[2]=m_macAddr.m_mac[2];
    dst[3]=m_macAddr.m_mac[3];
    dst[4]=m_macAddr.m_mac[4];
    dst[5]=m_macAddr.m_mac[5];

    //将arp查询中的target IP 填写到sender ip
    dst = arp + ARP_SPA_OFFSET; 
    src = data+    ARP_TPA_OFFSET;
    dst[0]=src[0];
    dst[1]=src[1];
    dst[2]=src[2];
    dst[3]=src[3];

    //将arp查询中的源eth地址填写到target eth地址
    dst = arp + ARP_THA_OFFSET;
    src = arpPacket + ETH_ALEN;
    dst[0]=src[0];
    dst[1]=src[1];
    dst[2]=src[2];
    dst[3]=src[3];
    dst[4]=src[4];
    dst[5]=src[5];


    //将arp查询中的sender IP填写到target IP
    dst = arp + ARP_TPA_OFFSET; 
    src = data+    ARP_SPA_OFFSET;
    dst[0]=src[0];
    dst[1]=src[1];
    dst[2]=src[2];
    dst[3]=src[3];
    
    outPacketLength = ETH_HEADER_LENGTH + ARP_HEADER_LENGTH;

    return true;
}


bool CArpSender::GenerateArpRequest( const string& sourceIP , const string& targetIP , uint8_t* outPacket , uint32_t& outPacketLength )
{
    if( outPacketLength < ETH_HEADER_LENGTH + ARP_HEADER_LENGTH )
        return false;

    //out mac header
    memset( outPacket , 0xff , ETH_ALEN );
    memcpy( outPacket + ETH_ALEN , m_macAddr.m_mac , ETH_ALEN );
    outPacket[ ETH_ALEN * 2 ]        =    (uint8_t)( ETH_TYPE_ARP >> 8 );
    outPacket[ ETH_ALEN * 2 + 1]    =    (uint8_t)( ETH_TYPE_ARP & 0x00ff );

    //output arp request message

    uint32_t index = ETH_HEADER_LENGTH; 
    outPacket[ index++ ] = 0x00;
    outPacket[ index++ ] = 0x01;
    outPacket[ index++ ] = (uint8_t)( ETH_TYPE_IPV4 >> 8 );
    outPacket[ index++ ] = (uint8_t)( ETH_TYPE_IPV4 & 0x0f );
    outPacket[ index++ ] = ETH_ALEN;
    outPacket[ index++ ] = 4;
    outPacket[ index++ ]= (uint8_t) ( 0 );
    outPacket[ index++ ]= (uint8_t) ( Arp_Request & 0x0f );

    memcpy( outPacket + index , m_macAddr.m_mac , ETH_ALEN );//sender mac address
    index += ETH_ALEN;

    uint32_t value = IPv4Address::ConvertToInteger( sourceIP.c_str() );//sender ip address
    memcpy( outPacket + index , &value , sizeof(uint32_t) );
    index += sizeof(uint32_t);

    memset( outPacket + index , 0x00 , ETH_ALEN );//target mac address
    index += ETH_ALEN;


    value = IPv4Address::ConvertToInteger( targetIP.c_str() );//target ip address
    memcpy( outPacket + index , &value , sizeof(uint32_t) );
    index += sizeof(uint32_t);

    outPacketLength = index;

    return true;
}

/*发送arp请求包，用于arp重定向，原则上GenerateArpRequest的工作内容可以统一到本函数
但因为GenerateArpRequest使用了private变量m_macAddr，caller访问不到所以暂时保留之*/
void CArpSender::GenerateRawArpRequest(uint8_t *pucSenderMac, 
                                                                uint32_t ulSenderIp, 
                                                                uint32_t ulTargetIp, 
                                                                uint8_t *pucData, 
                                                                uint32_t *pulDataLen)
{
    uint8_t *pucCurData = pucData;

    //mac首部destination IP为广播地址
    memset(pucCurData, 0xff, 6);
    pucCurData += 6;

    //mac首部源IP为目标EPC的mac地址
    memcpy(pucCurData, pucSenderMac, 6);
    pucCurData += 6;

    //mac首部type字段
    *(U16*)pucCurData = htons(ETH_TYPE_ARP);
    pucCurData += 2;
        
    //arp内容HardwareType
    *(U16*)pucCurData = htons(1);
    pucCurData += 2;

    //arp内容ProtocolType
    *(U16*)pucCurData = htons(0x0800);
    pucCurData += 2;

    //arp内容HardwareSize
    *pucCurData = 0x06;
    pucCurData++;

    //arp内容ProtocolSize
    *pucCurData = 0x04;
    pucCurData++;

    //arp内容Opcode
    *(U16*)pucCurData = htons(1);
    pucCurData += 2;

    //arp内容sender mac为目标EPC的mac地址
    memcpy(pucCurData, pucSenderMac, 6);
    pucCurData += 6;

    //arp内容sender ip为ue的ip
    *(U32*)pucCurData = ulSenderIp; //已经是网络序的?
    pucCurData += 4;

    //arp内容target mac为全零
    memset(pucCurData, 0x00, 6);
    pucCurData += 6;

    //arp内容target ip为server的ip
    *(U32*)pucCurData = ulTargetIp; //已经是网络序的?
    pucCurData += 4;    

    *pulDataLen = (pucCurData - pucData);

    return;
}


bool ParseArpTargetAddress( const uint8_t* data , uint32_t arpPacketLength , 
                           uint32_t& targetIP , uint32_t& arpType )
{
    if( arpPacketLength < ETH_HEADER_LENGTH + ARP_HEADER_LENGTH )
    {
        PS_CPlus(CM_NES, CMNES_ID_ARP_TARGET_LENGTH_FAIL);
        return false;
    }

    const uint8_t * arpPacket = data + ETH_HEADER_LENGTH ;

    if( arpPacket[ ARP_HLN_OFFSET ] != ETH_ALEN &&
        arpPacket[ ARP_PLN_OFFSET ] != 4 )
    {
        PS_CPlus(CM_NES, CMNES_ID_ARP_TARGET_OFFSET_FAIL);
        return false;
    }

    memcpy( &targetIP , arpPacket + ARP_TPA_OFFSET   , 4 );

    arpType = ( arpPacket[ARP_OP_OFFSET]<< 8 ) | arpPacket[ARP_OP_OFFSET+1];
    
    return true;
}

bool ParserArpSenderAddress( const uint8_t* data , uint32_t arpPacketLength, 
                                uint32_t* senderIP , uint8_t* senderMac , uint32_t* arpType )
{
    if( arpPacketLength < ETH_HEADER_LENGTH + ARP_HEADER_LENGTH )
    {
        PS_CPlus(CM_NES, CMNES_ID_ARP_SENDER_LENGTH_FAIL);
        return false;
    }

    const uint8_t * arpPacket = data + ETH_HEADER_LENGTH ;

    if( arpPacket[ ARP_HLN_OFFSET ] != ETH_ALEN &&
        arpPacket[ ARP_PLN_OFFSET ] != 4 )
    {
        PS_CPlus(CM_NES, CMNES_ID_ARP_SENDER_OFFSET_FAIL);
        return false;
    }

    if (NULL == senderIP)
    {
        return false;
    }

    memcpy(senderIP , arpPacket + ARP_SPA_OFFSET , 4);
    memcpy(senderMac , arpPacket + ARP_SHA_OFFSET , ETH_ALEN);

    uint8_t aucZeroBytes[ETH_ALEN] = {0};
    if (!memcmp(senderMac,aucZeroBytes , ETH_ALEN))
    {
        PS_CPlus(CM_NES, CMNES_ID_ARP_SENDER_MEMCMP_FAIL);
        return false;
    }

    *arpType = (arpPacket[ARP_OP_OFFSET] << 8) | arpPacket[ARP_OP_OFFSET+1];

    return true;
}


