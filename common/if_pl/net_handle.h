#ifndef __NETHANDLE_H__
#define __NETHANDLE_H__

#include <stdint.h>
static uint32_t g_value = 1;
class ConnHandle
{
public:
    ConnHandle()
    {
        m_IsValid = false;
        m_connProtocol = 0;
        m_connIndex = 0;
        m_connMagic = g_value++;
    }

    ConnHandle( const ConnHandle& handle )
    {
        m_IsValid = handle.m_IsValid;
        m_connProtocol = handle.m_connProtocol;
        m_connIndex = handle.m_connIndex;
        m_connMagic = handle.m_connMagic;
    }

    bool IsValid() const
    {
        return this->m_IsValid;
    }

    void Close()
    {
        m_connProtocol = 0;
        m_connIndex = 0;
        m_connMagic = 0;
        m_IsValid = false;
    }

    void updateHandle( uint32_t connProtocol , uint32_t connIndex )
    {
        m_connProtocol = connProtocol;
        m_connIndex    = connIndex;
        m_IsValid = true;
    }

    bool operator == ( const ConnHandle& handle ) const 
    {
        return m_IsValid == handle.m_IsValid &&
            m_connProtocol == handle.m_connProtocol &&
            m_connMagic == handle.m_connMagic &&
            m_connIndex == handle.m_connIndex;
    }

    bool operator != ( const ConnHandle& handle ) const 
    {
        return m_IsValid != handle.m_IsValid ||
            m_connProtocol != handle.m_connProtocol ||
            m_connMagic != handle.m_connMagic ||
            m_connIndex != handle.m_connIndex;
    }

    const ConnHandle& operator = ( const ConnHandle& handle )
    {
        if( this != &handle )
        {
            m_IsValid = handle.m_IsValid;
            m_connProtocol = handle.m_connProtocol;
            m_connIndex = handle.m_connIndex;
            m_connMagic = handle.m_connMagic;
        }
        return *this;
    }

    uint32_t GetConnIndex() const
    {
        return m_connIndex;
    }

private:
    uint32_t m_connProtocol;
    uint32_t m_connMagic;
    uint32_t m_connIndex;
    bool m_IsValid;
};

#endif //__NETHANDLE_H__
