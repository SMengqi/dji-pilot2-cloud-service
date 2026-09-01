#ifndef ICLIENTCONNECTION_H
#define ICLIENTCONNECTION_H

#include <stdint.h>

struct SessionData;
struct SndData;

/**********************************************************************************************//**
* @class    IClientConnection
*
* @brief    Network connection for client side.
*
* @date    2012/10/9
**************************************************************************************************/
class IClientConnection
{
protected:
    virtual ~IClientConnection(){};
public:
    virtual bool GetClientConnectionInfo( SessionData& sessionData ) = 0;

    virtual bool SendData( const uint8_t* data , uint32_t size , SndData* param = 0 ) = 0;
    virtual bool SetSctpHBPara( int32_t interval , int32_t count ) = 0;
    virtual bool SetTcpHBPara(int heartbeat_en, int idle, int cnt, int intv) = 0;

    virtual void Close() = 0;

    virtual uint32_t GetLeftData() = 0;
};

#endif
