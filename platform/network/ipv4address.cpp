#include <stdio.h>
#include <string.h>

#include "ipv4address.h"

bool IPv4Address::operator == (const IPv4Address& stPeerAddr) const
{
    if ( false == stPeerAddr.bIsValid || false == this->bIsValid )
    {
        return false;
    }

    if ( this->ulAddrInt != stPeerAddr.ulAddrInt )
    {
        return false;
    }

    return true;
}

bool IPv4Address::operator< (const IPv4Address& stPeerAddr) const
{
    if ( false == stPeerAddr.bIsValid || false == this->bIsValid )
    {
        return false;
    }

    if (this->ulAddrInt < stPeerAddr.ulAddrInt)
    {
        return true;
    } 
    return false;
}

#if 0
//从本结构体中获取字符串
std::string IPv4Address::GetString() const
{
    return string(acAddrStr);
}
#endif

//将一个网络字的整数转成字符串
std::string IPv4Address::ConvertToString( uint32_t ulAddr )
{
    char tmpArray[20] = {0};
    uint8_t* p = (uint8_t*)&ulAddr;
    uint8_t tmpByte1 = p[0];
    uint8_t tmpByte2 = p[1];
    uint8_t tmpByte3 = p[2];
    uint8_t tmpByte4 = p[3];

    sprintf(tmpArray,"%d.%d.%d.%d", tmpByte1,tmpByte2,tmpByte3,tmpByte4);

    return string(tmpArray);
}

//将一个字符串转成网络字的整数
uint32_t IPv4Address::ConvertToInteger(const char* cpAddr)
{
    uint32_t ulAddr = inet_addr(cpAddr);
    if (INADDR_NONE == ulAddr)
        ulAddr = 0;
    return ulAddr;
};

//这个函数很特殊，是根据字符串成员变量返回一个网络字的整数，但并不能设置其他的成员变量因为外层有const型参数传递
uint32_t IPv4Address::GetIPv4AddressFromString() const
{
    U32 ulTempAddrInt = inet_addr(acAddrStr);
    if (INADDR_NONE == ulTempAddrInt)
        ulTempAddrInt = 0;

    return ulTempAddrInt;
}

#if 0
void IPv4Address::SetIpAddrInterface(IP_ADDRESS_S *pstIpAddrIf)
{
    pstIpAddrIf->usIpAddrLen = (U16)strnlen(this->acAddrStr, MAX_IP_V4_ADDRESS_LEN);
    memcpy(pstIpAddrIf->aucIpAddr, this->acAddrStr, pstIpAddrIf->usIpAddrLen);

    return;
}

#endif

//获取字符串的长度不包括结束符，一般用于从IPv4Address向IP_ADDRESS_S中赋值时填入长度
uint16_t IPv4Address::GetStringLen()
{
    uint16_t usStrLen = (uint16_t)strlen(acAddrStr);
    if (usStrLen > MAX_IP_V4_ADDRESS_LEN)
    {
        usStrLen = MAX_IP_V4_ADDRESS_LEN;
    }
    return usStrLen;
}

void IPv4Address::Clean()
{
    bIsValid = false;
    ulAddrInt = 0;
    memset(aucAddr4IntArr, 0, sizeof(aucAddr4IntArr));
    memset(acAddrStr, 0, sizeof(acAddrStr));

    return;
}


//根据输入的U32来设置所有的变量
void IPv4Address::SetIPv4AddressFromU32(U32 ulAddr)
{
    ulAddrInt = ulAddr;

    uint8_t *p= (uint8_t*)&ulAddrInt;
    aucAddr4IntArr[0] = p[0];
    aucAddr4IntArr[1] = p[1];
    aucAddr4IntArr[2] = p[2];
    aucAddr4IntArr[3] = p[3];

    struct in_addr temp_addr;
    temp_addr.s_addr = ulAddrInt;
    memcpy(acAddrStr, inet_ntoa(temp_addr), MAX_IP_V4_ADDRESS_LEN+1);

    bIsValid = true;
}

//根据输入的字符串来设置所有的变量
void IPv4Address::SetIPv4AddressFromString(char *cpAddr)
{
    ulAddrInt = inet_addr(cpAddr);

    uint8_t *p= (uint8_t*)&ulAddrInt;
    aucAddr4IntArr[0] = p[0];
    aucAddr4IntArr[1] = p[1];
    aucAddr4IntArr[2] = p[2];
    aucAddr4IntArr[3] = p[3];

    struct in_addr temp_addr;
    temp_addr.s_addr = ulAddrInt;
    memcpy(acAddrStr, inet_ntoa(temp_addr), MAX_IP_V4_ADDRESS_LEN+1);

    bIsValid = true;
}

//根据输入的四字节字节型，四字节的低索引存放高位，来设置所有的变量
void IPv4Address::SetIPv4AddressFrom4U8(uint8_t *aucAddr4IntArray)
{
    aucAddr4IntArr[0] = aucAddr4IntArray[0];
    aucAddr4IntArr[1] = aucAddr4IntArray[1];
    aucAddr4IntArr[2] = aucAddr4IntArray[2];
    aucAddr4IntArr[3] = aucAddr4IntArray[3];

    uint8_t* p = (uint8_t*)&ulAddrInt;
    p[0] = aucAddr4IntArr[0];
    p[1] = aucAddr4IntArr[1];
    p[2] = aucAddr4IntArr[2];
    p[3] = aucAddr4IntArr[3];

    struct in_addr temp_addr;
    temp_addr.s_addr = ulAddrInt;
    memcpy(acAddrStr, inet_ntoa(temp_addr), MAX_IP_V4_ADDRESS_LEN+1);

    bIsValid = true;
}

