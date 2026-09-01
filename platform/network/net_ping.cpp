#define THIS_MODULE MODULE_NETWORK
#include "net_ping.h"
#include "net_ipv4.h"



uint16_t cal_chksum( const uint16_t *addr,uint32_t len)
{       
    uint32_t nleft=len;
    uint32_t sum=0;
    const uint16_t *w=addr;
    uint16_t answer=0;
    
    while(nleft>1)
    {       
        sum+=*w++;
        nleft-=2;
    }
    
    if( nleft==1)
    {   
        *(uint8_t *)(&answer)=*(uint8_t *)w;
        sum+=answer;
    }
    
    sum=(sum>>16)+(sum&0xffff);
    sum+=(sum>>16);
    answer=(uint16_t)~sum;
    return answer;
}

CPing::CPing( const std::string& sourceIP , const std::string& destIP , uint32_t sequence , uint32_t datasize )
:m_inputStream(ETH_HEADER_LENGTH+IPV4_HEADER_LENGTH+ICMP_HEADER_LENGTH+MAX_ICMP_DATA)
{
    m_nIcmpDataSize = datasize;
    if( m_nIcmpDataSize > MAX_ICMP_DATA )
        m_nIcmpDataSize = MAX_ICMP_DATA;

    m_nSequence = sequence;

    IPv4_Header ipv4_hdr;

    ipv4_hdr.ver = 4;
    ipv4_hdr.headerLength = IPV4_HEADER_LENGTH >> 2;

    ipv4_hdr.tos = 0;

    ipv4_hdr.total_Length = (uint16_t)( IPV4_HEADER_LENGTH + ICMP_HEADER_LENGTH + datasize );

    ipv4_hdr.identification = 0x1234;

    ipv4_hdr.flag_off = 0;

    ipv4_hdr.ttl = 32;

    ipv4_hdr.protocol = 1;

    ipv4_hdr.checksum = 0;

    ipv4_hdr.saddr = IPv4Address::IPv4AddressConvertToInteger( sourceIP.c_str() );
    ipv4_hdr.daddr = IPv4Address::IPv4AddressConvertToInteger( destIP.c_str()  );


    uint8_t first = ipv4_hdr.ver << 4 | ipv4_hdr.headerLength;

    m_inputStream << first << ipv4_hdr.tos ;
    m_inputStream << (uint8_t)( ipv4_hdr.total_Length >> 8 ) << (uint8_t)ipv4_hdr.total_Length;
    m_inputStream << (uint8_t)( ipv4_hdr.identification >> 8 ) << (uint8_t)ipv4_hdr.identification;

    m_inputStream << ipv4_hdr.flag_off << ipv4_hdr.ttl << ipv4_hdr.protocol ;

    uint32_t checksumoffset = m_inputStream.GetBufferSize();
    m_inputStream << ipv4_hdr.checksum << ipv4_hdr.saddr << ipv4_hdr.daddr;


    uint16_t checksum = cal_chksum( (const uint16_t*)m_inputStream.GetBuffer() , m_inputStream.GetBufferSize() );

    
    m_inputStream.SetOffsetValue( checksumoffset , checksum );

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    sgw_icmphdr icmp_hdr;
    icmp_hdr.type = 8;//echo request
    icmp_hdr.code = 0;
    icmp_hdr.checksum = 0;
    icmp_hdr.id = 0x1234;
    icmp_hdr.sequence = (uint16_t)m_nSequence;

    uint32_t icmp_start = m_inputStream.GetBufferSize();


    m_inputStream << icmp_hdr.type << icmp_hdr.code ;
    uint16_t icmp_checksum_offset = (uint16_t)m_inputStream.GetBufferSize();
    m_inputStream << icmp_hdr.checksum ;
    m_inputStream << (uint8_t)( icmp_hdr.id >> 8 ) << (uint8_t)( icmp_hdr.id );
    m_inputStream << (uint8_t)( icmp_hdr.sequence >> 8 ) <<  (uint8_t)( icmp_hdr.sequence );

    uint8_t data[MAX_ICMP_DATA]={0};
    m_inputStream.InputData( data , m_nIcmpDataSize );

    uint32_t icmp_size = m_inputStream.GetBufferSize() - icmp_start;
    const uint8_t* icmp_data = m_inputStream.GetBuffer() + icmp_start;
    uint16_t icmp_checksum = cal_chksum( (const uint16_t*)icmp_data , icmp_size );


    m_inputStream.SetOffsetValue( icmp_checksum_offset , icmp_checksum );

}

CPing::~CPing()
{
}

const uint8_t* CPing::GetPingBuffer()
{
    return m_inputStream.GetBuffer();
}

uint32_t CPing::GetPingBufferSize()
{
    return m_inputStream.GetBufferSize();
}