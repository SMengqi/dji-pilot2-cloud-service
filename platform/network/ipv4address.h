/**********************************************************************************************//**
 * @file    CommonClass\IPv4Address.h
 *
 * @brief    Declares the IPv4Address interface.
 **************************************************************************************************/

#ifndef S1AP_IPADDRESS_H
#define S1AP_IPADDRESS_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include "pl_type.h"
#ifdef WIN32
#include <WinSock.h>
#else
#include <arpa/inet.h>
#endif

using namespace std;

#define MAX_IP_V4_ADDRESS_LEN   32

/**********************************************************************************************//**
 * @class    IPv4Address
 *
 * @brief    IPv4 address.
 **************************************************************************************************/
class IPv4Address
{
public:
    IPv4Address()
    {
        bIsValid = false;
        ulAddrInt = 0;
        memset(aucAddr4IntArr, 0, sizeof(aucAddr4IntArr));
        memset(acAddrStr, 0, sizeof(acAddrStr));
    }
    
    virtual ~IPv4Address(){};


    bool        bIsValid;            /**< whether IPv4 address is valid*/
    uint32_t    ulAddrInt;            /**< integer format for IPv4 address, big endian NBO*/
    uint8_t        aucAddr4IntArr[4];
    char        acAddrStr[MAX_IP_V4_ADDRESS_LEN+1];/**< Character type IP address. for example:"172.168.10.12" */
    
    bool operator < (const IPv4Address& stPeerAddr) const;
    bool operator == (const IPv4Address& stPeerAddr) const;

    bool IsValid() const
    {
        return bIsValid;
    }
    
    uint32_t GetInterger() const
    {
        return ulAddrInt;
    }

    string GetString() const
    {
        return string(acAddrStr);
    };

    static string    ConvertToString(uint32_t ulAddr);
    static uint32_t    ConvertToInteger(const char* cpAddr);
    uint16_t GetStringLen();
    uint32_t GetIPv4AddressFromString() const;
    void SetIPv4AddressFromU32(U32 ulAddr);
    void SetIPv4AddressFromString(char *cpAddr);
    void SetIPv4AddressFrom4U8(uint8_t *aucAddr4IntArray);
//    void SetIpAddrInterface(IP_ADDRESS_S *pstIpAddrIf);
    void Clean();
};


#endif
