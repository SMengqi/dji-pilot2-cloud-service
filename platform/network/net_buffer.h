#ifndef __NETBUFFER_H__
#define __NETBUFFER_H__

#include "net_define.h"

class CBuffer
{
private:
    uint32_t m_nStart;
    uint32_t m_nEnd;
   /*modify*/
    uint8_t* m_head;
    uint8_t* m_tail;
    uint8_t* m_write;
    uint8_t* m_read;
    uint32_t m_nCapacity;
    uint32_t m_nLength;
    uint8_t* m_pBuf;
    
public:
    CBuffer( uint32_t size );
    
    ~CBuffer();

    void Clear()
    {
        m_nLength = 0;
        m_head = 0;
        m_tail = 0;
        m_write = NULL;
        m_read = NULL;
    }
   
    uint32_t GetLeftData() const
    {
        return m_nLength;
    }

  //  bool AddSizeBuffer( const uint8_t* buf , uint32_t nSize );
  //  void ReleaseSizeBuffer( uint32_t nSize );
    uint8_t* GetSizeBuffer();
    uint32_t GetSizeBufferLength();
    ////////////////////////////////////////////////////////////////////////////////////////////////////
  

	bool printInfo(char* info, uint32_t num);
    bool AddTcpBuffer(const uint8_t* buf, uint32_t nSize);
    void ReleaseTcpBuffer(uint32_t nSize);
    uint8_t* GetTcpBuffer();
    uint32_t GetTcpLength(); 

     ////////////////////////////////////////////////////////////////////////////////////////////////////

    bool AddUdpBuffer(const uint8_t* buf, uint32_t nSize , uint32_t dstaddr, uint16_t dstport );
    void ReleaseUdpBuffer(uint32_t nSize );
    uint8_t* GetUdpBuffer();
    uint32_t GetUdpLength();
    uint32_t GetUdpAddr();
    uint16_t GetUdpPort();

     ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool AddSctpBuffer( const uint16_t streamID , const uint8_t* buf , uint32_t nSize );
    void ReleaseSctpBuffer( uint32_t nSize );
    uint8_t* GetSctpBuffer();
    uint32_t GetSctpLength( );
    uint16_t GetStreamID();
     ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool AddRawIpBuffer( const uint32_t dstAddr , const uint8_t* buf , uint32_t nSize );
    void ReleaseRawIpBuffer( uint32_t nSize );
    uint8_t* GetRawIpBuffer();
    uint32_t GetRawIpLength();
    uint32_t GetDstAddr();

    bool IsEmpty() const
    {
        return m_nLength == 0;
    }

private:
    void AddBuffer( uint32_t value );
    void AddBuffer( uint16_t value );
    void AddBuffer( const uint8_t* data , uint32_t size );
    bool ResizeBuffer( uint32_t nNewSize );

};

#endif
