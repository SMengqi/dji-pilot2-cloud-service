#ifndef __PLATFORM_NET_TOOLS_H__
#define __PLATFORM_NET_TOOLS_H__
#include <string>
#include <list>
#include <stdint.h>

struct IpPair
{
    std::string m_ip;
    std::string m_mask;
};

struct InterfaceInformation
{
    std::list<IpPair> m_ipAddressList;
    uint8_t     m_mac[6];
    std::string m_macAddress;
    std::string m_interfaceName;
    uint32_t    m_interfaceIndex;
};

struct IpInformation
{
    std::string m_ip;
    uint32_t    m_ipValue;
    std::string m_mask;
    uint32_t    m_maskValue;
    uint8_t     m_mac[6];
    std::string m_interfaceName;
    uint32_t    m_interfaceIndex;
};

bool FindIpInformation( const std::string& ipaddr , IpInformation& info );

bool GetAllIpInformation( std::list<InterfaceInformation>& intfInformationList );

struct ArpInformation
{
    uint32_t dwAddr;
    uint8_t  physAddr[6];
};

bool GetArpTableInformation( std::list<ArpInformation>& arpInformationList );

void cross_sleep( uint32_t millseconds );

std::string GetBinDirectory();

std::string GetMainIP();
std::string GetMainIPfromDev(std::string dev);
std::string GetGateway();

bool CreateNameSemaphore(const char* name );

#endif //__PLATFORM_NET_TOOLS_H__
