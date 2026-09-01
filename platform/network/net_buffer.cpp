#define THIS_MODULE MODULE_NETWORK
#include "net_buffer.h"
#include "scoped_lock.h"
#include "platform.h"



#define MAX_CAPACITY 1024 * 1024 * 32

CBuffer::CBuffer( uint32_t size )
{
    m_nLength    = 0;

    m_nStart    = 0;
    m_nEnd        = 0;
    m_nCapacity = size;
    m_pBuf = new uint8_t[ m_nCapacity ];
    m_head = m_pBuf;
    m_tail = m_pBuf+m_nCapacity;
    m_write = m_head;
    m_read = m_head;
}

CBuffer::~CBuffer()
{
    if( m_pBuf )
    {
        delete [] m_pBuf;
        m_pBuf = NULL;
    }
}

void CBuffer::AddBuffer( uint32_t value )
{
//    uint32_t* current = (uint32_t*)(m_pBuf + m_nEnd);
//    *current = value;

    memcpy(m_pBuf+m_nEnd ,&value ,sizeof(uint32_t));


    m_nLength += sizeof(uint32_t);
    m_nEnd    += sizeof(uint32_t);
}

void CBuffer::AddBuffer( uint16_t value )
{
//    uint16_t* current = (uint16_t*)(m_pBuf + m_nEnd);
//    *current = value;

    memcpy(m_pBuf+m_nEnd ,&value ,sizeof(uint16_t));

    m_nLength += sizeof(uint16_t);
    m_nEnd    += sizeof(uint16_t);
}

void CBuffer::AddBuffer( const uint8_t* data , uint32_t size )
{
    memcpy( m_pBuf + m_nEnd , data , size );
    m_nLength += size;
    m_nEnd    += size;
}



bool CBuffer::ResizeBuffer( uint32_t nNewSize )
{

//    ScopedLock<PlatformMutex> lock(m_lockstate);

    bool bExpand = false;

    if((m_nStart + m_nLength) > m_nEnd)
    {
        // m_nStart - (m_nStart + m_nLength - m_nEnd) = m_nEnd - m_nLength
        if((m_nEnd - m_nLength) < nNewSize)
        {
            bExpand = true;
        }
    }
    else if( (m_nCapacity - m_nEnd) < nNewSize )
    {
        bExpand = true;
    }

    if(bExpand)
    {
        uint32_t copy = m_nCapacity;

        while(1)
        {
            if (copy >= MAX_CAPACITY)
            {
                PS_CPlus(CM_NES, CMNES_ID_BUFFER_RESIZE_CAPACITY_FAIL);
                return false;
            }
            
            copy *= 2;
            if( copy > MAX_CAPACITY )
            {
                copy = MAX_CAPACITY;
            }
            
            if (m_nLength + nNewSize <= copy )
            {
                NAS_PrintLog(INF,"\r\n\r\n old buf_0x%lx  , m_nCapacity_%d , addsize_%d , m_nStart_%d , m_nLength_%d , m_nEnd_%d ",m_pBuf,m_nCapacity,nNewSize,m_nStart,m_nLength,m_nEnd);
                m_nCapacity = copy;
                uint8_t* newbuf = new uint8_t[m_nCapacity];
                if((m_nStart + m_nLength) <= m_nEnd)
                {
                    memcpy( newbuf , m_pBuf + m_nStart , m_nLength );
                }
                else
                {
                    memcpy( newbuf , m_pBuf + m_nStart , m_nEnd - m_nStart );
                    memcpy( newbuf + m_nEnd - m_nStart , m_pBuf ,m_nLength - (m_nEnd - m_nStart) );
                }
                delete [] m_pBuf;
                m_pBuf = newbuf;
                m_nStart = 0;
                m_nEnd = m_nLength;

                NAS_PrintLog(INF,"\r\n new buf_0x%lx  , m_nCapacity_%d , addsize_%d , m_nStart_%d , m_nLength_%d , m_nEnd_%d \r\n",m_pBuf,m_nCapacity,nNewSize,m_nStart,m_nLength,m_nEnd);

#if 0
uint32_t u32len = 0;
uint32_t u32id = 0;
memcpy(&u32len,m_pBuf,4 );
memcpy(&u32id,m_pBuf+20,4 );

printf("\r\n packet0: len_0x%08x, id_0x%08x ",u32len,u32id);

memcpy(&u32id,m_pBuf+u32len+8+16,4 );
memcpy(&u32len,m_pBuf+u32len+4,4 );

printf("\r\n packet1: len_0x%08x, id_0x%08x \r\n",u32len,u32id);
#endif
                return true;    
            }
        }
    }

    return true;
}



#if 0

bool CBuffer::ResizeBuffer( uint32_t nNewSize )
{
    bool bExpand = false;
    
    uint32_t copy = m_nCapacity;
    while (m_nLength + nNewSize > copy )
    {
        if (copy >= MAX_CAPACITY)
        {
            PS_CPlus(CM_NES, CMNES_ID_BUFFER_RESIZE_CAPACITY_FAIL);
            return false;
        }
        copy *= 2;
        if( copy >= MAX_CAPACITY )
        {
            copy = MAX_CAPACITY;
        }
        bExpand = true;
    }

    if( bExpand )
    {
        m_nCapacity = copy;
        uint8_t* newbuf = new uint8_t[m_nCapacity];
        memcpy( newbuf , m_pBuf + m_nStart , m_nLength );
        delete [] m_pBuf;
        m_pBuf = newbuf;
        m_nStart = 0;
        m_nEnd = m_nLength;
    }

    uint32_t left = m_nCapacity - m_nEnd;
    if( left < nNewSize )   //当m_nStart不是0时，这种情况才可能发生，将已缓存数据往前挪动
    {
        memcpy( m_pBuf , m_pBuf + m_nStart , m_nLength );
        m_nStart = 0;
        m_nEnd = m_nLength;
    }
    
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CBuffer::AddSizeBuffer( const uint8_t* buf , uint32_t nSize )
{
    uint32_t nNewSize = nSize + sizeof(uint32_t) ;
    
    if( false == ResizeBuffer( nNewSize ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_BUFFER_ADDSIZE_RESIZE_FAIL);
        return false;
    }

    AddBuffer( nSize );
    AddBuffer( buf , nSize );
    return true;
}

void CBuffer::ReleaseSizeBuffer( uint32_t nSize )
{
    uint32_t nNewSize = nSize + sizeof(uint32_t) ;
    if ( nNewSize >= m_nLength )
    {
        m_nLength = 0;
        m_nStart = 0;
        m_nEnd = 0;
    }
    else
    {
        m_nLength -= nNewSize;
        m_nStart += nNewSize;
    }
}

#endif

uint8_t* CBuffer::GetSizeBuffer()
{
    return m_pBuf + sizeof(uint32_t) + m_nStart;
}

uint32_t CBuffer::GetSizeBufferLength()
{
    if( m_nLength > 0 )
    {
        return *((uint32_t*)(m_pBuf + m_nStart)); 
    }
    else
    {
        return 0;
    }
}


bool CBuffer::printInfo(char* info, uint32_t num)
{
    uint32_t total; 
    if(m_write >= m_read)
    {
        total = m_write - m_read;
    }
    else
    {
        total = m_tail - m_read + m_write -m_head;
    }
/*    
printf("%s TEST NUM %d  m_sendBufferLen %d Cap %d total:%d write:%x read:%x addr: %x head :%x tail:%x\n", info, num,
	m_nLength,
	m_nCapacity,
	total,
	m_write,
	m_read,
	m_pBuf,
	m_head,
	m_tail);*/
    pl_log(INF,"%s TEST NUM %d  m_sendBufferLen %d Cap %d total:%d write:%x read:%x addr: %x head :%x tail:%x\n", info, num,
	m_nLength,
	m_nCapacity,
	total,
	m_write,
	m_read,
	m_pBuf,
	m_head,
	m_tail);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CBuffer::AddTcpBuffer(const uint8_t* buf, uint32_t nSize)
{
    uint32_t nMaxSize;
    uint32_t nCopySize;
    
    if(m_write >= m_read)
    {
        nMaxSize = m_nCapacity - (m_write - m_read);
    }
    else
    {
        nMaxSize = m_read - m_write;
    }

    if(nSize >=  nMaxSize)
    {
        PS_CPlus(CM_NES, CMNES_ID_BUFFER_ADDTCP_RESIZE_FAIL);
        pl_log(ERR,"AddTcpBuffer failed ,nsize:%d nMaxSize: %d",nSize,nMaxSize);
        return false;
    }

    if(m_write >= m_read)
    {
        if((m_tail - m_write) >= nSize)
        {
            pf_memcpy(m_write,buf,nSize);
            m_write += nSize;
        }
        else
        {
            nCopySize = m_tail - m_write;
            pf_memcpy(m_write,buf,nCopySize);
            pf_memcpy(m_head,buf+nCopySize,nSize-nCopySize);
            m_write = m_head+nSize-nCopySize;
        }
    }
    else
    {
        pf_memcpy(m_write,buf,nSize);
        m_write+=nSize;
    }
    
   PS_CPlusV(CM_COM, CMCOM_ID_ADD_BUFFER_SIZE, nSize);
   m_nLength += nSize;
   return true;    
}


void CBuffer::ReleaseTcpBuffer(uint32_t nSize)
{
    uint32_t TailSize;
    uint32_t HeadSize;

    if(nSize > m_nLength)
    {
        NAS_PrintLog( LOG_ERROR," Err: ReleaseTcpBuffer nSize_%d > m_nLength_%d " , nSize, m_nLength );
        return;
    }
    
    if( m_read <= m_write)
    {
        m_read += nSize;
    }
    else
    {
        TailSize = m_tail - m_read;
        if(nSize < TailSize)
        {
            m_read += nSize;
        }
        else
        {
            HeadSize = nSize - TailSize;
            m_read = m_head + HeadSize;
        }
    }

    m_nLength -= nSize;
    PS_CPlusV(CM_COM, CMCOM_ID_MOVE_BUFFER_SIZE, nSize);
}



uint8_t* CBuffer::GetTcpBuffer()
{
    return m_read;
}

uint32_t CBuffer::GetTcpLength() 
{
    if(m_write >= m_read)
    {
        return m_nLength;
    }
    else
    {
        return (m_tail - m_read);
    }
}





////////////////////////////////////////////////////////////////////////////////////////////////////
bool CBuffer::AddUdpBuffer(const uint8_t* buf, uint32_t nSize, uint32_t dstaddr , uint16_t dstport)
{
    uint32_t nNewSize = nSize + sizeof(uint32_t) + sizeof( uint32_t ) + sizeof( uint16_t );
    
    if( false == ResizeBuffer( nNewSize ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_BUFFER_ADDUDP_RESIZE_FAIL);
        return false;
    }

    if(((m_nStart + m_nLength) == m_nEnd)&&((m_nCapacity - m_nEnd) >= nNewSize ))
    {
        memcpy(m_pBuf + m_nEnd ,&nSize ,sizeof(uint32_t));
        m_nEnd += sizeof(uint32_t);
        memcpy(m_pBuf + m_nEnd ,&dstaddr ,sizeof(uint32_t));
        m_nEnd += sizeof(uint32_t);
        memcpy(m_pBuf + m_nEnd ,&dstport ,sizeof(uint16_t));
        m_nEnd += sizeof(uint16_t);
        memcpy(m_pBuf + m_nEnd ,buf , nSize);
        m_nEnd += nSize;
    }
    else
    {
        uint32_t u32index = m_nStart + m_nLength - m_nEnd ;

        memcpy(m_pBuf + u32index ,&nSize ,sizeof(uint32_t));
        u32index += sizeof(uint32_t);
        memcpy(m_pBuf + u32index ,&dstaddr ,sizeof(uint32_t));
        u32index += sizeof(uint32_t);
        memcpy(m_pBuf + u32index ,&dstport ,sizeof(uint16_t));
        u32index += sizeof(uint16_t);
        memcpy(m_pBuf + u32index ,buf , nSize);
    }

    m_nLength += nNewSize;

    return true;
}

void CBuffer::ReleaseUdpBuffer(uint32_t nSize)
{
    uint32_t nNewSize = nSize + sizeof(uint32_t)+sizeof(uint32_t) + sizeof( uint16_t );

    if ( nNewSize >= m_nLength )
    {
        if( nNewSize > m_nLength )
        {
            NAS_PrintLog( LOG_ERROR," Err: ReleaseUdpBuffer nSize_%d > m_nLength_%d " , nSize, m_nLength );
        }
        m_nLength = 0;
        m_nStart = 0;
        m_nEnd = 0;
    }
    else
    {
        uint32_t packetlen = 0;
        memcpy( &packetlen, m_pBuf+m_nStart,sizeof(uint32_t) );
    
        if(packetlen > nSize)
        {
            packetlen = packetlen - nSize;
            memcpy(m_pBuf + m_nStart + nSize, &packetlen, sizeof(uint32_t) );
            memcpy(m_pBuf + m_nStart + nSize + sizeof(uint32_t) , m_pBuf + m_nStart + sizeof(uint32_t), sizeof(uint32_t) + sizeof( uint16_t ) );
            nNewSize = nSize;
        }
        else if(packetlen < nSize)
        {
            NAS_PrintLog( LOG_ERROR," Err: ReleaseUdpBuffer nSize_%d > packetlen_%d " , nSize, packetlen );
            nNewSize = packetlen + sizeof(uint32_t) + sizeof(uint32_t) + sizeof( uint16_t ); 
        }

        if ( nNewSize >= m_nLength )
        {
            m_nLength = 0;
            m_nStart = 0;
            m_nEnd = 0;
        }
        else
        {
            m_nLength -= nNewSize;
            m_nStart += nNewSize;

            if(m_nStart >= m_nEnd)
            {
                if( m_nStart > m_nEnd )
                {
                    NAS_PrintLog( LOG_ERROR," Err ReleaseUdpBuffer: m_nStart_%d > m_nEnd_%d addsize_%d , m_nCapacity_%d , m_nStart_%d , m_nLength_%d , m_nEnd_%d " 
                        , m_nStart, m_nEnd ,nNewSize,m_nCapacity,m_nStart,m_nLength,m_nEnd );

                    m_nStart -= m_nEnd;
                    m_nEnd = m_nStart + m_nLength;
                }
                else
                {
                    m_nStart = 0;
                    m_nEnd = m_nLength;
                }
            }
        }
    }
}


uint32_t CBuffer::GetUdpLength() 
{ 
    if( m_nLength > 0 )
    {
        uint32_t len = 0;
        memcpy(&len, m_pBuf+m_nStart,sizeof(uint32_t));
        return len;

//        return *((uint32_t*)( m_pBuf+m_nStart)); 
    }
    else
    {
        return 0;
    }
}

uint32_t CBuffer::GetUdpAddr()
{
    if( m_nLength > 0 )
    {
        uint32_t u32addr = 0;
        memcpy(&u32addr, m_pBuf+m_nStart+sizeof(uint32_t),sizeof(uint32_t));
        return u32addr;

//        return *((uint32_t*)( m_pBuf+m_nStart+sizeof(uint32_t))); 
    }
    else
    {
        return 0;
    }
}

uint16_t CBuffer::GetUdpPort()
{
    if( m_nLength > 0 )
    {
        uint16_t u16port = 0;
        memcpy(&u16port, m_pBuf+m_nStart+sizeof(uint32_t)+sizeof(uint32_t),sizeof(uint16_t));
        return u16port;
//        return *((uint32_t*)( m_pBuf+m_nStart+sizeof(uint32_t)+sizeof(uint32_t))); 
    }
    else
    {
        return 0xFFFF;
    }
}

uint8_t* CBuffer::GetUdpBuffer()
{ 
    return m_pBuf+m_nStart + sizeof(uint32_t) + sizeof(uint32_t) + sizeof( uint16_t ); 
}



////////////////////////////////////////////////////////////////////////////////////////////////////

bool CBuffer::AddSctpBuffer( const uint16_t streamID , const uint8_t* buf , uint32_t nSize )
{
    uint32_t nNewSize = nSize + sizeof(uint32_t) + sizeof( uint16_t );
    bool bExpand = false;

    uint32_t copy = m_nCapacity;
    while (m_nLength + nNewSize > copy )
    {
        copy *= 2;
        if( copy >= MAX_CAPACITY )
        {
            PS_CPlus(CM_NES, CMNES_ID_BUFFER_ADDSCTP_MAX_CAPACITY_FAIL);
            return false;
        }
        bExpand = true;
    }

    if( bExpand )
    {
        m_nCapacity = copy;
        uint8_t* newbuf = new uint8_t[m_nCapacity];
        memcpy( newbuf , m_pBuf , m_nLength );
        delete [] m_pBuf;
        m_pBuf = newbuf;
        m_nStart = 0;
        m_nEnd = m_nLength;
    }

    uint32_t left = m_nCapacity - m_nEnd;

    if( left < nNewSize )
    {
        memcpy( m_pBuf , m_pBuf + m_nStart , m_nLength );
        m_nStart = 0;
        m_nEnd = m_nLength;
    }

    *((uint32_t*)( m_pBuf + m_nEnd ))=nSize;
    m_nEnd += sizeof( uint32_t );
    *((uint16_t*)( m_pBuf + m_nEnd ))=streamID;
    m_nEnd += sizeof( uint16_t );
    memcpy((unsigned char*)&m_pBuf[m_nEnd ], buf, nSize);
    m_nEnd += nSize;
    m_nLength += nNewSize;
    return true;
}

void CBuffer::ReleaseSctpBuffer( uint32_t nSize )
{
    uint32_t nNewSize = nSize + sizeof(uint32_t) + sizeof( uint16_t );

    if ( nNewSize >= m_nLength )
    {
        m_nLength = 0;
        m_nStart = 0;
        m_nEnd = 0;
    }
    else
    {  
        m_nLength -= nNewSize;

        m_nStart += nNewSize;
    }
}

uint8_t* CBuffer::GetSctpBuffer()
{
    return m_pBuf+m_nStart + sizeof(uint32_t)+sizeof(uint16_t); 
}

uint32_t CBuffer::GetSctpLength()
{
    if( m_nLength > 0 )
    {
        return *((uint32_t*)( m_pBuf+m_nStart)); 
    }
    else
    {
        return 0;
    }
}

uint16_t CBuffer::GetStreamID()
{
    if( m_nLength > 0 )
    {
        return *((uint16_t*)( m_pBuf+m_nStart+sizeof(uint32_t))); 
    }
    else
    {
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CBuffer::AddRawIpBuffer( const uint32_t dstAddr , const uint8_t* buf , uint32_t nSize )
{
    uint32_t nNewSize = nSize + sizeof(uint32_t) + sizeof( uint32_t );
    
    if( false == ResizeBuffer( nNewSize ) )
    {
        PS_CPlus(CM_NES, CMNES_ID_BUFFER_ADDRAWIP_RESIZE_FAIL);
        return false;
    }
    
    if(((m_nStart + m_nLength) == m_nEnd)&&((m_nCapacity - m_nEnd) >= nNewSize ))
    {
        memcpy(m_pBuf + m_nEnd ,&nSize ,sizeof(uint32_t));
        m_nEnd += sizeof(uint32_t);
        memcpy(m_pBuf + m_nEnd ,&dstAddr ,sizeof(uint32_t));
        m_nEnd += sizeof(uint32_t);
        memcpy(m_pBuf + m_nEnd ,buf , nSize);
        m_nEnd += nSize;
    }
    else
    {
        uint32_t u32index = m_nStart + m_nLength - m_nEnd ;

        memcpy(m_pBuf + u32index ,&nSize ,sizeof(uint32_t));
        u32index += sizeof(uint32_t);
        memcpy(m_pBuf + u32index ,&dstAddr ,sizeof(uint32_t));
        u32index += sizeof(uint32_t);
        memcpy(m_pBuf + u32index ,buf , nSize);
    }

    m_nLength += nNewSize;
 
    return true;
}

void CBuffer::ReleaseRawIpBuffer( uint32_t nSize )
{
    uint32_t nNewSize = nSize + sizeof(uint32_t) + sizeof( uint32_t );

    if ( nNewSize >= m_nLength )
    {
        if( nNewSize > m_nLength )
        {
            NAS_PrintLog( LOG_ERROR," Err: ReleaseRawIpBuffer nSize_%d > m_nLength_%d " , nSize, m_nLength );
        }
        m_nLength = 0;
        m_nStart = 0;
        m_nEnd = 0;
    }
    else
    {
        uint32_t packetlen = 0;
        memcpy( &packetlen, m_pBuf+m_nStart,sizeof(uint32_t) );
    
        if(packetlen > nSize)
        {
            packetlen = packetlen - nSize;
            memcpy(m_pBuf + m_nStart + nSize, &packetlen, sizeof(uint32_t) );
            memcpy(m_pBuf + m_nStart + nSize + sizeof(uint32_t) , m_pBuf + m_nStart + sizeof(uint32_t), sizeof(uint32_t) );
            nNewSize = nSize;
        }
        else if(packetlen < nSize)
        {
            NAS_PrintLog( LOG_ERROR," Err: ReleaseRawIpBuffer nSize_%d > packetlen_%d " , nSize, packetlen );
            nNewSize = packetlen + sizeof(uint32_t) + sizeof(uint32_t); 
        }

        if ( nNewSize >= m_nLength )
        {
            m_nLength = 0;
            m_nStart = 0;
            m_nEnd = 0;
        }
        else
        {
            m_nLength -= nNewSize;
            m_nStart += nNewSize;

            if(m_nStart >= m_nEnd)
            {
                if( m_nStart > m_nEnd )
                {
                    NAS_PrintLog( LOG_ERROR," Err ReleaseRawIpBuffer: m_nStart_%d > m_nEnd_%d addsize_%d , m_nCapacity_%d , m_nStart_%d , m_nLength_%d , m_nEnd_%d " 
                        , m_nStart, m_nEnd ,nNewSize,m_nCapacity,m_nStart,m_nLength,m_nEnd );

                    m_nStart -= m_nEnd;
                    m_nEnd = m_nStart + m_nLength;
                }
                else
                {
                    m_nStart = 0;
                    m_nEnd = m_nLength;
                }
            }
        }
    }

}

uint8_t* CBuffer::GetRawIpBuffer()
{
    return m_pBuf+m_nStart + sizeof(uint32_t)+sizeof(uint32_t); 
}

uint32_t CBuffer::GetRawIpLength()
{
    if( m_nLength > 0 )
    {
        return *((uint32_t*)( m_pBuf+m_nStart)); 
    }
    else
    {
        return 0;
    }
}

uint32_t CBuffer::GetDstAddr()
{
    if( m_nLength > 0 )
    {
        return *((uint32_t*)( m_pBuf+m_nStart+sizeof(uint32_t))); 
    }
    else
    {
        return 0;
    }
}
