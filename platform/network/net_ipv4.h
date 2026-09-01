#ifndef __SGW_IPV4_H__
#define __SGW_IPV4_H__

#include <stdint.h>
#include "pl.h"

#define IPV4_ADDRESS_LENGTH 4
struct IPv4_Header
{    
    uint8_t ver;                    /* IP version  */
    uint8_t headerLength;            /*ip header length */
    uint8_t tos;                    /* type of service */
    uint16_t total_Length;            /* total length */
    uint16_t identification ;        /* identification */
    uint16_t flag_off;                /* fragment offset field */
    uint8_t ttl;                    /* time to live */
    uint8_t protocol;                /* up layer protocol */
    uint16_t checksum;                /* header checksum */
    uint32_t saddr;                    /* source address */
    uint32_t daddr;                    /* destination address */
};

#define MIN_IPV4_HEADER_LENGTH 20
#define IPV4_TOTAL_LENGTH_OFFSET 2
#define IPV4_PROTOCOL_OFFSET 9
#define IPV4_SOURCE_IP_OFFSET 12
#define IPV4_DEST_IP_OFFSET 16

bool Ipv4_ParserIPAddress( const uint8_t* ipdata , uint32_t size , 
                          uint32_t& saddr , uint32_t& daddr , uint32_t& protocol );

bool Ipv4_ParserIPAddress( const uint8_t* ipdata , uint32_t size , 
                          IPv4_Header& header );

uint32_t Ipv4_GetDstAddress( const uint8_t* ipdata , uint32_t size );

uint32_t Ipv4_GetSrcAddress( const uint8_t* ipdata , uint32_t size );

#endif //__SGW_IPV4_H__