#ifndef __SGW_PING_H__
#define __SGW_PING_H__
#include <stdint.h>
#include <string>
#include <CommonLib/CommonIE/gtpc_common/gtpc_macro.h>
#include <CommonLib/CommonIE/gtpc_common/gtpc_inputstream.h>


struct sgw_icmphdr
{
    uint8_t type;        /* message type */
    uint8_t code;        /* type sub-code */
    uint16_t checksum;
    uint16_t    id;
    uint16_t    sequence;
};

#define MAX_ICMP_DATA 256

class CPing
{
public:
    CPing( const std::string& sourceIP , const std::string& destIP , uint32_t sequence , uint32_t datasize = 32 );
    ~CPing();
public:
    const uint8_t* GetPingBuffer();
    uint32_t GetPingBufferSize();
private:
    uint32_t m_nSequence;
    uint32_t m_nIcmpDataSize;
    GTPC_InputStream m_inputStream;

};
#endif //__SGW_PING_H__