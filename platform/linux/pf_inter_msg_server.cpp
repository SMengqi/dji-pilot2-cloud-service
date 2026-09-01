/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    eCos_timer_service.cpp

  Author:       Version:        Date: 
  josephzhou    1.0             2013-04-07
  
  Description:  This file implements unsocket service interfaces between process.

*******************************************************************************/

#define THIS_MODULE PLATFORM_INTER_MSG
/* include files*/
#include "pl.h"

#include "event.h"
#include "module.h"

#include "osport.h"
#include "os.h"
#include "pf_mbox.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdarg.h>


#define MAX_PATH_LENGTH 128


char g_buffer[PROCESS_MAX][0x10100];   //不完整的消息内容
U32 g_ulBufferLen[PROCESS_MAX] = {0};    //不完整的the length of message
char g_acPath[PROCESS_MAX][MAX_PATH_LENGTH];
int g_server_sockfd[PROCESS_MAX];
int g_sockfdFlag[PROCESS_MAX];
pf_handle_t g_handle[PROCESS_MAX];

int g_client_sockfd;
U32 ulCurPID /*= LTEPS*/;               

extern pf_thread_t  workerThreads[];
extern pf_handle_t  workerhandles[];

DECLTASK(unsocket_server, 131072)


void handle_pipe(int sig) 
{
    return;
}

void init_socket_to_handle_pipe(void )
{
    struct sigaction action;
    action.sa_handler = handle_pipe;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGPIPE, &action, NULL);
}

/**********************************************************************************************
 * @function      pf_interChip_msg_init
 * @brief         Initialization interface for message passing between processes
 * @input         ulPid         main process ID
                  pcPath        Main process path name
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_interChip_msg_init(U32 ulPid, const char* pcPath)
{
    if((ulPid >= PROCESS_MAX) || (NULL == pcPath))
    {
        pl_log(ERR, "%s:Error input: MID=%d, pcPath=0x%s", __FUNCTION__, ulPid, pcPath);// lint -e506
        ASSERT(0);
        return;
    }

    U32 ulPathLen = strlen(pcPath);
    if((0 == ulPathLen) || (MAX_PATH_LENGTH <= ulPathLen))
    {
        pl_log(ERR, "%s:Error input Length: MID=%d, pcPath=0x%x,ulPathLen=%d", __FUNCTION__, ulPid, pcPath, ulPathLen);// lint -e506
        ASSERT(0);
        return;
    }

    //if (ulPid == LTEPS)
        init_socket_to_handle_pipe();

    pf_memset(g_sockfdFlag, 0, sizeof(g_sockfdFlag));
    pf_memset(g_acPath[ulPid], 0, MAX_PATH_LENGTH);
    pf_memcpy(g_acPath[ulPid],  pcPath, ulPathLen);
    ulCurPID = ulPid;

    CREATETASK(0, UNSOCK_SERVER, unsocket_server);
}


/**********************************************************************************************
 * @function      client_entry
 * @brief         thread entry function
 * @input         mid                   Message Queue Identification Number
 * @output        void
 * @return        void
 *********************************************************************************************/

void client_entry(pf_addrword_t ulPid)
{    
    int server_len;
    struct sockaddr_un server_address;
    int result;

    g_server_sockfd[ulPid]=socket(AF_UNIX, SOCK_STREAM, 0);
    if(-1 == g_server_sockfd[ulPid])
    {
        pl_log(ERR, "server_Socket error:%s\n\a",strerror(errno));
        ASSERT(0);
    }

    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, g_acPath[ulPid]);
    server_len = sizeof(server_address);
       
    while(!g_sockfdFlag[ulPid])
    {
        result = connect(g_server_sockfd[ulPid], (const struct sockaddr *)&server_address, (socklen_t)server_len);
        if(-1 == result)
        {
            pf_usleep(50000);
            continue;
        }
        else
        {
            pl_log(INF, "%s:socket connect OK\t\n", __FUNCTION__);
            g_sockfdFlag[ulPid] = TRUE;
        }

        pf_usleep(10000000);
    }
}


/**********************************************************************************************
 * @function      pf_interChip_msg_connect
 * @brief         Initialization interface for message passing between processes
 * @input         ulPid         Auxiliary process ID 
                  pcPath        Auxiliary process path name
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_interChip_msg_connect(U32 ulPid, const char* pcPath)
{
    if((ulPid >= PROCESS_MAX) || (ulPid == ulCurPID) || (NULL == pcPath))
    {
        pl_log(ERR, "%s:Error input: MID=%d, pcPath=%s", __FUNCTION__, ulPid, pcPath);        // lint -e506
        ASSERT(0);
        return;
    }

    U32 ulPathLen = strlen(pcPath);
    if((0 == ulPathLen) || (MAX_PATH_LENGTH <= ulPathLen))
    {
        pl_log(ERR, "%s:Error input Length: MID=%d, pcPath=%s,ulPathLen=%d", __FUNCTION__, ulPid, pcPath, ulPathLen);
        ASSERT(0);
        return;
    }

    pf_memset(g_acPath[ulPid], 0, MAX_PATH_LENGTH);
    pf_memcpy(g_acPath[ulPid], pcPath, ulPathLen);

    pthread_attr_t attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (0 != ret)
    {
       pl_log(ERR, "pthread_attr_init ret=%d\n",ret);
    }

    ret = pthread_create(&g_handle[ulPid], &attr, (void*(*)(void*))client_entry,(void*)ulPid);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_create error ret=%d\n",ret);
    }

}

/**********************************************************************************************
 * @function      cmd_line_init
 * @brief         Initialization of cmd_line threads
                  Perform libcmd_line.so library detection and unsocket initialization
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
S32 unsocket_server_init(U32 ulModuleId)
{

    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  get_message_from_client
 * @brief         Interface function of main process reading message to client process
 * @input         pBuf                  the address of received message
 *                ulLength              the length of received message
 * @output        void
 * @return        other                 success
                  0                     failure
 *********************************************************************************************/
extern "C" U32 get_message_from_client(char* pBuf, U32 ulLength)
{
    U32 read_num;

    read_num = read(g_client_sockfd, pBuf, ulLength);
    if((read_num == 0) || (read_num > ulLength))
    {
        pl_log(ERR, "get_message_from_client failed, read_num=%u, g_client_sockfd=%d\n", read_num, g_client_sockfd);   
        read_num = 0;
        close(g_client_sockfd);
        
        S32 client_len;
        struct sockaddr_un client_address;
        int result;

        result = listen(g_server_sockfd[ulCurPID], 5);
        if(-1 == result)
        {
            pl_log(ERR, "listen error:%s\n\a",strerror(errno));
            ASSERT(0);
        }

        g_client_sockfd = accept(g_server_sockfd[ulCurPID], (struct sockaddr *)&client_address, (socklen_t*)&client_len);
        if(-1 == g_client_sockfd)
        {
            pl_log(ERR, "server_Socket error:%s\n\a",strerror(errno));
            ASSERT(0);
        }   
    }

    return read_num;
}

/**********************************************************************************************
 * @API function  unsocket_copy_msg
 * @brief         Parse and distribute received inter-process messages
 * @input         pcBuf                 the address of sending message
 *                ulReadNum             the length of sending message
 * @output        void
 * @return        other                 success
                  0                     failure
 *********************************************************************************************/
U32 unsocket_copy_msg(char* pcBuf, U32 ulReadNum)
{
    stMboxMessage* pstMsg = (stMboxMessage*)pcBuf;
    U32 ulMsgLen = 0;
    U32 ulMboxLen = sizeof(stMboxMessage);

    // pl_log(INF, "msgId=%d, dst=%d, src=%d, content=0x%x, length=%d, pstMsg=0x%x\n", pstMsg->msgId, pstMsg->dst, pstMsg->src, pstMsg->content, pstMsg->length, pstMsg);
    if(ulMboxLen + pstMsg->ulLength <= ulReadNum)
    {
        pf_copy_msg(pstMsg->ulSrcModuleId, pstMsg->ulMsgId, pstMsg->ulDstModuleId, (void *)(pcBuf + ulMboxLen),  pstMsg->ulLength);
    }
    else
    {
        return 0;
    }
            
    ulMsgLen = ulMboxLen + pstMsg->ulLength;
            
    while(ulMsgLen < ulReadNum)
    {
        pstMsg = (stMboxMessage*)(pcBuf + ulMsgLen);

        if(ulMsgLen + ulMboxLen + pstMsg->ulLength <= ulReadNum)
        {
            pf_copy_msg(pstMsg->ulSrcModuleId, pstMsg->ulMsgId, pstMsg->ulDstModuleId, (void *)(pcBuf + ulMsgLen + ulMboxLen),  pstMsg->ulLength);
            ulMsgLen += ulMboxLen + pstMsg->ulLength;
        }
        else
        {
            return ulMsgLen;
        }
    }
    
    return ulMsgLen;
}

/**********************************************************************************************
 * @function      unsocket_server_entry
 * @brief         the entry of unsocket_server
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void unsocket_server_entry (pf_addrword_t data)
{
    U32 read_num = 0;
    int result;
    U32 ulMsgOffset = 0;
    char buffer[UNSOCKET_BUFFER_LENGTH];
    char* pcBuf = NULL;
    U32 ulBufLen = 0;
    int rcd ;
    U32 ci ;
    int watch_fd_list[PROCESS_MAX+1] ;
    fd_set catch_fd_set ;
    fd_set watchset ;
    int new_cli_fd ;
    int maxfd;
    int socklen;
    int server_len;
    struct sockaddr_un server_address;

    unlink(g_acPath[ulCurPID]);

    g_server_sockfd[ulCurPID]=socket(AF_UNIX, SOCK_STREAM, 0);
    if(-1 == g_server_sockfd[ulCurPID])
    {
        pl_log(ERR, "server_Socket error:%s\n\a",strerror(errno));
        ASSERT(0);
    }

    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, g_acPath[ulCurPID]);
    server_len = sizeof(server_address);

    result = bind(g_server_sockfd[ulCurPID], (struct sockaddr *)&server_address, server_len);
    if(-1 == result)
    {
        pl_log(ERR, "Bind error:%s\n\a",strerror(errno));
        ASSERT(0);
    }

    for (ci=0;ci<PROCESS_MAX+1;ci++)
    {
        watch_fd_list[ci]=-1; 
    }

    result = listen(g_server_sockfd[ulCurPID], 5);
    if(-1 == result)
    {
        pl_log(ERR, "listen error:%s\n\a",strerror(errno));
        ASSERT(0);
    }

    watch_fd_list[0]=g_server_sockfd[ulCurPID];
    FD_ZERO(&watchset);
    FD_SET(g_server_sockfd[ulCurPID], &watchset);
    maxfd=watch_fd_list[0];

    while (1)
    {
        int nread;
        struct sockaddr_un cli_sockaddr ;

        catch_fd_set=watchset;
        rcd = select( maxfd+1, &catch_fd_set, NULL, NULL, (struct timeval *)0 ) ; 

        if ( rcd < 0 ) 
        {
            pl_log(ERR, "SERVER::Server 5");
            continue;
        }

        if ( FD_ISSET( g_server_sockfd[ulCurPID], &catch_fd_set ) ) 
        {
            socklen = sizeof( cli_sockaddr ) ; 
            new_cli_fd = accept( g_server_sockfd[ulCurPID], ( struct sockaddr * )&( cli_sockaddr ), (socklen_t*)&socklen ) ;
            pl_log(ERR, " SERVER::open communication with Client %s on socket %d", cli_sockaddr.sun_path,new_cli_fd); 
    
            for (ci=1;ci<PROCESS_MAX+1;ci++)
            {
                if(watch_fd_list[ci] != -1) 
                    continue;
                else
                { 
                    watch_fd_list[ci] = new_cli_fd;
                    break;
                } 
            }
            FD_ZERO( &watchset );
            for(ci=0;ci<=PROCESS_MAX;ci++)
            {
                if(watch_fd_list[ci]!=-1)
                {
                    FD_SET(watch_fd_list[ci] , &watchset ) ;
                    if(maxfd<watch_fd_list[ci])
                    {
                        maxfd=watch_fd_list[ci];
                    }    
                }
            }
            continue; 
        }

        for ( ci = 1; ci<PROCESS_MAX+1 ; ci++ ) 
        {

            if (watch_fd_list[ ci ]==-1) 
                continue;
            if ( !FD_ISSET( watch_fd_list[ ci ], &catch_fd_set ) ) 
            {
                continue ;
            }

            ioctl(watch_fd_list[ ci ],FIONREAD,&nread);
            if (nread==0)
            {
                pl_log(ERR, "the client is disconnect:ci=%d,watch_fd_list[ %d ]=%d\n",ci,ci,watch_fd_list[ ci ]);
                watch_fd_list[ ci ]=-1;
                continue;
            } 

            read_num = read( watch_fd_list[ ci ], buffer, UNSOCKET_BUFFER_LENGTH) ;
            if(read_num > 0)
            {        
                if(g_ulBufferLen[ci] > 0)
                {
                    pcBuf = g_buffer[ci];
                    pf_memcpy(&g_buffer[ci][g_ulBufferLen[ci]], buffer, read_num);
                    g_ulBufferLen[ci] += read_num;
                    ulBufLen = g_ulBufferLen[ci];
                }
                else
                {
                    pcBuf = buffer;
                    ulBufLen = read_num;
                }

                ulMsgOffset = unsocket_copy_msg(pcBuf, ulBufLen);
                if(ulMsgOffset != ulBufLen)
                {
                    pf_memcpy(&g_buffer[ci][0], &pcBuf[ulMsgOffset], ulBufLen-ulMsgOffset);
                    g_ulBufferLen[ci] = ulBufLen-ulMsgOffset;
                }
                else
                {
                    g_ulBufferLen[ci] = 0;
                }
            
            }

        }     
        rcd = listen( g_server_sockfd[ulCurPID], 5 ) ;
        
        FD_ZERO( &watchset ) ;
        for(ci=0;ci<=PROCESS_MAX;ci++)
        {
            if(watch_fd_list[ci]!=-1)
            {
                FD_SET(watch_fd_list[ci] , &watchset ) ;
                if(maxfd<watch_fd_list[ci])
                {
                    maxfd=watch_fd_list[ci];
                }    
            }
        }  
    }

}

