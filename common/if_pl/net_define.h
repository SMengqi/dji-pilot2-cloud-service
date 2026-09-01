#ifndef __NETDEFINE_H__
#define __NETDEFINE_H__

#include <stdint.h>
#include <memory.h>

#include <string>

#include "pl.h"


#ifndef NULL
#define NULL 0
#endif //NULL


/*接收端单包限定在1M字节 1024*1024=1048576*/
#define SINGLE_PACKET_LEN_MAX 1048576 


enum SessionMode
{
    SESSION_MODE_PACKET_HEAD = 0,
    SESSION_MODE_PASS_THROUGH,
};


enum NetProtocol
{
    NETPROTOCOL_INVALID = 0,
    NETPROTOCOL_UDP,
    NETPROTOCOL_TCP,
    NETPROTOCOL_SCTP,
    NETPROTOCOL_RAWIP,
    NETPROTOCOL_ASFPMAL,
};

enum CRYPTO_CFG
{
    CRYPTO_CFG_NONE = 0,
    CRYPTO_CFG_SSL,
    CRYPTO_CFG_SSL_MIXED,
    CRYPTO_CFG_AES,
    CRYPTO_CFG_MAX,
};

struct SessionData
{
    uint32_t     m_CryptoCfg; /**<connection status for distinguish tcp or ssl or mixedmode ps: only use for server*/
    uint16_t     m_LocalPort;  /**< Local port number used for specific connection */
    uint16_t     m_PeerPort;   /**< Peer port number used for specific connection  */
    
    uint16_t     m_num_ostreams; /**< the number of output stream supported by this sctp connection  */
    uint16_t     m_max_instreams; /**< the maximal of input stream supported by this sctp connection  */
    uint32_t     u32_data;
    uint64_t     user_data;
    std::string  m_LocalIP;    /**< Local IP address used for specific connection  */
    std::string  m_PeerIP;     /**< Peer IP address used for specific connection   */
    std::string  m_bindIfname; /**the name of network card, for raw socket only*/


    SessionData()
    {
        m_CryptoCfg = CRYPTO_CFG_NONE; 
        m_LocalPort = 0;
        m_PeerPort  = 0;
        m_num_ostreams = 5;
        m_max_instreams = 5;
    }
};

struct SndData
{
    uint16_t m_sctp_streamID;
    uint32_t m_dst_address;//dest ip address in network order
    uint16_t m_dst_port;
    SndData()
    {
        m_sctp_streamID = 0;
        m_dst_address = 0;
        m_dst_port = 0;
    }
};

#define SCTP_PAYLOAD_PROTOCOL_ID_S1AP 18

#endif //__NETDEFINE_H__

