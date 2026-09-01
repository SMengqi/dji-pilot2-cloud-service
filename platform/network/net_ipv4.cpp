#define THIS_MODULE MODULE_NETWORK
#include "net_ipv4.h"
#include <string.h>
#include <stdio.h>

bool Ipv4_ParserIPAddress( const uint8_t* ipdata , uint32_t size , 
                          uint32_t& saddr , uint32_t& daddr , uint32_t& protocol )
{
    if( size <= MIN_IPV4_HEADER_LENGTH )
    {
        PS_CPlus(CM_NES, CMNES_ID_IPV4_PARSERIP_SIZE_FAIL);
        return false;
    }

    uint32_t total = ( ipdata[IPV4_TOTAL_LENGTH_OFFSET] << 8 ) | ipdata[IPV4_TOTAL_LENGTH_OFFSET+1];

    if( total != size )
    {
        PS_CPlus(CM_NES, CMNES_ID_IPV4_PARSERIP_TOTAL_SIZE_FAIL);
        return false;
    }

    protocol = ipdata[IPV4_PROTOCOL_OFFSET];

    memcpy( &saddr  , ipdata + IPV4_SOURCE_IP_OFFSET , IPV4_ADDRESS_LENGTH );
    memcpy( &daddr ,  ipdata + IPV4_DEST_IP_OFFSET , IPV4_ADDRESS_LENGTH );

    return true;
}

bool Ipv4_ParserIPAddress( const uint8_t* ipdata , uint32_t size , 
                          IPv4_Header& header )
{
    if( size <= MIN_IPV4_HEADER_LENGTH )
    {
        PS_CPlus(CM_NES, CMNES_ID_IPV4_PARSERIP_SIZE_FAIL);
        return false;
    }

    uint32_t total = ( ipdata[IPV4_TOTAL_LENGTH_OFFSET] << 8 ) | ipdata[IPV4_TOTAL_LENGTH_OFFSET+1];

    if( total != size )
    {
        PS_CPlus(CM_NES, CMNES_ID_IPV4_PARSERIP_TOTAL_SIZE_FAIL);
        return false;
    }

    header.protocol = ipdata[IPV4_PROTOCOL_OFFSET];

    memcpy( &header.saddr  , ipdata + IPV4_SOURCE_IP_OFFSET , IPV4_ADDRESS_LENGTH );
    memcpy( &header.daddr ,  ipdata + IPV4_DEST_IP_OFFSET , IPV4_ADDRESS_LENGTH );

    return true;
}

uint32_t Ipv4_GetDstAddress( const uint8_t* ipdata , uint32_t size )
{
    uint32_t value = 0 ;
    if( size > MIN_IPV4_HEADER_LENGTH )
    {
        memcpy( &value , ipdata + IPV4_DEST_IP_OFFSET , IPV4_ADDRESS_LENGTH );
    }
    return value;
}

uint32_t Ipv4_GetSrcAddress( const uint8_t* ipdata , uint32_t size )
{
    uint32_t value = 0 ;
    if( size > MIN_IPV4_HEADER_LENGTH )
    {
        memcpy( &value , ipdata + IPV4_SOURCE_IP_OFFSET , IPV4_ADDRESS_LENGTH );
    }
    return value;
}