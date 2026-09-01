/**********************************************************************************************//**
 * @file    NetLib.h
 *
 * @brief    Network library functionality.
 **************************************************************************************************/

#ifndef __NETLIB_H__
#define __NETLIB_H__

#include "net_define.h"

#define DEFAULT_SCTP_PORT 36412
#define DEFAULT_GTP_PORT 0


/**********************************************************************************************//**
 * @fn    void NetlibInit();
 *
 * @brief    Initialize network functionality.
 *
 * @date    2012/10/9
 **************************************************************************************************/

void NetlibInit( void );

/**********************************************************************************************//**
 * @fn    void network_init();
 *
 * @brief    for using in epc_module.
 *
 * @date    2012/10/9
 **************************************************************************************************/
void network_init( void );

/**********************************************************************************************//**
 * @fn    void network_entry();
 *
 * @brief    for using in epc_module.
 *
 * @date    2012/10/9
 **************************************************************************************************/
void network_entry( unsigned long long  );

#include "inet_session.h"
#include "iclient_connection.h"
#include "iserver_connection.h"
#include "net_factory.h"
#include "net_handle.h"
//#include "cross_platform_socket.h"


typedef void (*func_accept)(IClientConnection* , const SessionData&  );
typedef void (*func_disconnect)(IClientConnection* );
typedef void (*func_receive)(IClientConnection* , uint8_t* , uint32_t, const SessionData&  );

#include "platform_netsession_server.h"
#include "platform_netsession_client.h"


#endif //__NETLIB_H__
