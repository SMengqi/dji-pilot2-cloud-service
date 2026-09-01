/************************************************************
  Copyright (C), BroadXt Inc  2019
  FileName: pf_mbox.cpp
  Author: josephzhou    Version :  1.0   Date: 20190617
  Description:     header of platform
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2019/6/17         1.0        initial
***********************************************************/

#define THIS_MODULE PLATFORM_MBOX

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "pl.h"
#include "os.h"
#include "osport.h"
#include "pf_mbox.h"
#include "pf_thread_mon.h"


static inline int pf_mbox_put(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_put_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_put_head(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_put_head_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_put_copy_data2(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, void* pData2, U32 ulLength2,U32 ulTicks);

static inline int pf_mbox_try_put(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_try_put_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_try_put_head(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_try_put_head_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_try_lock_and_put(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);

static inline int pf_mbox_try_lock_and_put_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);


static U32 g_ulMidCounts = 0;

U32 ulThreadId[THREAD_ARRAY_MAX_ID] = {0};

extern int g_server_sockfd[PROCESS_MAX];

pf_mbox_t msgQArray[MODULE_TASK_MAX] = {0};
MODULE_ENTRY moduleArray[MODULE_TASK_MAX] = {0};
MODULE_INIT  moduleInitArray[MODULE_TASK_MAX] = {0};
pf_thread_t  workerThreads[MODULE_TASK_MAX] = {0};
pf_handle_t  workerhandles[MODULE_TASK_MAX] = {0};
int* workerStack[MODULE_TASK_MAX] = {0};

stCpuCoreMask aulModuleCpucoreMask[MODULE_TASK_MAX] = {0};



 /**********************************************************************************************
 * @API function  thread_mid_init
 * @brief         initial the identity of the module
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void thread_mid_init(void)
{
    int i;
    for(i=0;i<THREAD_ARRAY_MAX_ID;i++)
    {
        ulThreadId[i]=0xDEADBEEF;
    }

    for(i=0; i<MODULE_TASK_MAX; i++)
    {
        msgQArray[i] = NULL;
        moduleArray[i] = NULL;
    }

    for(i=0; i<MODULE_TASK_MAX; i++)
    {
        workerThreads[i] = NULL;
        workerhandles[i] = NULL;
    }
}

 /**********************************************************************************************
 * @API function  pf_set_thread_mid
 * @brief         Set the identity of the module where the thread is located
 * @input         ulMid                 the identity of the module
 * @output        void
 * @return        void 
 *********************************************************************************************/
void pf_set_thread_mid(U32 ulMid)
{
    pf_handle_t handle = workerhandles[ulMid];

    U32 id=((U32)handle >> THREAD_ARRAY_OFFSET)%THREAD_ARRAY_MAX_ID;
    if(0xDEADBEEF == ulThreadId[id])
    {
        ulThreadId[id]=ulMid;
        // printf("usMid[%d],workerhandles[usMid]=%llx,id=0x%x\n", ulMid, workerhandles[ulMid], id);
        pl_log(INF, "usMid[%d],workerhandles[usMid]=%llx,id=0x%x", ulMid, workerhandles[ulMid], id)
    }
    else
    {
        U32 i;
        U32 offset;
        for(i=1;i<THREAD_ARRAY_MAX_ID;i++)
        {
            offset=(id+i)%THREAD_ARRAY_MAX_ID;
            if(ulThreadId[offset] == 0xDEADBEEF)
            {
                ulThreadId[offset] = ulMid;
                printf("usMid[%d],workerhandles[usMid]=%llx,id=0x%x,i=%d,offset=%d\n", ulMid, workerhandles[ulMid], id, i, offset);
                break;
            }
        }
    }

    g_ulMidCounts++;
}

 /**********************************************************************************************
 * @API function  pf_get_thread_mid
 * @brief         get the identity of the module where the thread is located
 * @input         void
 * @output        void
 * @return        ulMid                 the identity of the module where the thread is located
 *********************************************************************************************/
U32 pf_get_thread_mid(void)
{
    pf_handle_t handle = PF_THREAD_SELF();
    
    U32 id=(handle>>THREAD_ARRAY_OFFSET)%THREAD_ARRAY_MAX_ID;
    U32 ulMid = ulThreadId[id];

    if(0xDEADBEEF == ulMid)
    {
        //printf("pf_get_thread_mid input error usMid[%d] id=%d, handle=0x%llx\n", ulMid, id, handle);
        PS_CPlus(CM_COM, CMCOM_ID_THREAD_MID_UNMATCH_CNT);
        return PF_RET_FAILURE;
    }

    if(handle != workerhandles[ulMid])
    {
        U32 i;
        U32 offset;
        U32 ulTmpId;
        for(i=1;i<THREAD_ARRAY_MAX_ID;i++)
        {
            PS_CPlus(CM_COM, CMCOM_ID_THREAD_MID_OVERLAP_CNT);
			
            offset=(id+i)%THREAD_ARRAY_MAX_ID;
            ulTmpId = ulThreadId[offset];
            if(0xDEADBEEF == ulTmpId)
            {
                //printf("get usMid[%d] ulTmpId[%d] failed i=%d id=%d offset=%d, handle=0x%llx\n", ulMid, ulTmpId, i, id, offset, handle);
                PS_CPlus(CM_COM, CMCOM_ID_THREAD_MID_UNMATCH_CNT);
                return PF_RET_FAILURE;
            }
            else if (handle == workerhandles[ulTmpId])
            {
                return ulTmpId;
            }
        }
        return PF_RET_FAILURE;
    }
    else
    {
        return ulMid;
    }
}


 /**********************************************************************************************
 * @API function  pf_get_thread_mid_num
 * @brief         get the number of active thread
 * @input         void
 * @output        void
 * @return        usMId                 the number of module
 *********************************************************************************************/
U32 pf_get_thread_mid_num(void)
{
    return g_ulMidCounts;
}


/**********************************************************************************************
 * @API function  pf_copy_urgent_msg
  * @brief        Transfer urgent message interface function, put the message ahead of queue
                  stMboxMessages will not be lost
  * @input        ulSrcModuleId        source module identification
  *               ulMsgId              message identification
  *               ulDstModuleId        destination module identification
  *               pData                the address of message
  *               ulLength             the length of message
  * @output       void
  * @return       0                    succuss
                  other                failure
 *********************************************************************************************/
extern "C" S32 pf_copy_urgent_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength)
{
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_IN_CNT);
    
    if((ulDstModuleId >= MODULE_MAX) || ((NULL == pData) && (ulLength > 0)))
    {
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_IN_FAIL);
        return PF_RET_FAILURE;
    }
 
    void * destqueue = msgQArray[ulDstModuleId];
    if(NULL == destqueue)
    {
        pl_log(ERR,"%s:Pfailed mid=%d NO EXIST\n", __FUNCTION__, ulDstModuleId);
        PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSGQ_NULL);
        return PF_RET_FAILURE;
    }

    U32 ulTicks = pf_get_ticks_ms();
    U32 ret;

    if(ulLength > SHORT_PACK_SIZE)
    {
        void* mdata = pf_malloc(ulLength);
        if(NULL == mdata)
        {
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_MALLOC_NULL);
            return PF_RET_FAILURE;
        }
        else
        {
            pf_memcpy(mdata, pData, ulLength);
        }
        PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_MALLOC_CNT);
        
        ret = pf_mbox_put_head(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, mdata, ulLength, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, ulLength);
            pf_free(mdata);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
    else
    {
        ret    = pf_mbox_put_head_copy_data(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, ulLength);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
 
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_COPY_MSG_SIZE_L, ulLength);
    return PF_RET_SUCCESS;
}


/**********************************************************************************************
 * @API function  pf_copy_try_urgent_msg
 * @brief         Try to transfer urgent message interface function, put the message ahead of queue
                  stMboxMessages may be lost for the full of queue
 * @input         usSrcModuleId         source module identification
 *                usMsgId               message identification
 *                usDstModuleId         destination module identification
 *                pData                 the address of message
 *                usLength              the length of message
 * @output        void
 * @return        0                     succuss
                  other                 failure
 *********************************************************************************************/
extern "C" S32 pf_copy_try_urgent_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength)
{
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_IN_CNT);
    
    if((ulDstModuleId >= MODULE_MAX) || ((NULL == pData) && (ulLength > 0)))
    {
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_IN_FAIL);
        return PF_RET_FAILURE;
    }
 
    void * destqueue = msgQArray[ulDstModuleId];
    if(NULL == destqueue)
    {
        pl_log(ERR,"%s:Pfailed mid=%d NO EXIST\n", __FUNCTION__, ulDstModuleId);
        PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSGQ_NULL);
        return PF_RET_FAILURE;
    }

    U32 ulTicks = pf_get_ticks_ms();
    U32 ret;

    if(ulLength > SHORT_PACK_SIZE)
    {
        void* mdata = pf_malloc(ulLength);
        if(NULL == mdata)
        {
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_MALLOC_NULL);
            return PF_RET_FAILURE;
        }
        else
        {
            pf_memcpy(mdata, pData, ulLength);
        }
        PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_MALLOC_CNT);
                

        ret = pf_mbox_try_put_head(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, mdata, ulLength, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, ulLength);
            pf_free(mdata);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
    else
    {
        ret = pf_mbox_try_put_head_copy_data(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, ulLength);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
 
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_COPY_MSG_SIZE_L, ulLength);
    return PF_RET_SUCCESS;

}

 /**********************************************************************************************
  * @API function pf_copy_msg
  * @brief        Transfer general message interface function
                  stMboxMessages will not be lost
  * @input        ulSrcModuleId        source module identification
  *               ulMsgId              message identification
  *               ulDstModuleId        destination module identification
  *               pData                the address of message
  *               ulLength             the length of message
  * @output       void
  * @return       0                    succuss
                  other                failure
  * @date         2012/11/16
  *********************************************************************************************/
extern "C" S32 pf_copy_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void *pData, U32 ulLength)
{
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_IN_CNT);
    
    if((ulDstModuleId >= MODULE_MAX) || ((NULL == pData) && (ulLength > 0)))
    {
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_IN_FAIL);
        return PF_RET_FAILURE;
    }
 
    void * destqueue = msgQArray[ulDstModuleId];
    if(NULL == destqueue)
    {
        pl_log(ERR,"%s:Pfailed mid=%d NO EXIST\n", __FUNCTION__, ulDstModuleId);
        PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSGQ_NULL);
        return PF_RET_FAILURE;
    }
 
    U32 ulTicks = pf_get_ticks_ms();
    U32 ret;
 
    if(ulLength > SHORT_PACK_SIZE)
    {
        void* mdata = pf_malloc(ulLength);
        if(NULL == mdata)
        {
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_MALLOC_NULL);
            return PF_RET_FAILURE;
        }
        else
        {
            pf_memcpy(mdata, pData, ulLength);
        }
        PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_MALLOC_CNT);
                
        ret = pf_mbox_put(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, mdata, ulLength, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, ulLength);
            pf_free(mdata);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
    else
    {
        ret = pf_mbox_put_copy_data(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, ulLength);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
 
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_COPY_MSG_SIZE_L, ulLength);
    return PF_RET_SUCCESS;
}






extern "C" S32 pf_copy_msg2(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void *pData, U32 ulLength ,void *pData2, U32 ulLength2)
{
    
    U32 u32MsgLen = ulLength + ulLength2;
    
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_IN_CNT);
    if((ulDstModuleId >= MODULE_MAX) || ((NULL == pData) && (ulLength > 0))|| ((NULL == pData2) && (ulLength2 > 0)))
    {
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_IN_FAIL);
        return PF_RET_FAILURE;
    }
    
 
    void * destqueue = msgQArray[ulDstModuleId];
    if(NULL == destqueue)
    {
        pl_log(ERR,"%s:Pfailed mid=%d NO EXIST\n", __FUNCTION__, ulDstModuleId);
        PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
        PS_CPlus(CM_PES, CMPES_ID_COPY_MSGQ_NULL);
        return PF_RET_FAILURE;
    }
 
    U32 ulTicks = pf_get_ticks_ms();
    U32 ret;
 
    if(u32MsgLen > SHORT_PACK_SIZE)
    {
        void* mdata = pf_malloc(u32MsgLen);
        if(NULL == mdata)
        {
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            PS_CPlus(CM_PES, CMPES_ID_COPY_MSG_MALLOC_NULL);
            return PF_RET_FAILURE;
        }
        else
        {
            pf_memcpy(mdata, pData, ulLength);
            pf_memcpy((U8*)mdata+ulLength, pData2, ulLength2);
        }
        PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_MALLOC_CNT);
        
        ret = pf_mbox_put(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, mdata, u32MsgLen, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, u32MsgLen);
            pf_free(mdata);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
    else
    {
        ret = pf_mbox_put_copy_data2(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength ,pData2, ulLength2, ulTicks);
        if(false == ret)
        {
            pl_log(ERR, "%s:L%d, Pfailed SendMbox src=%d dst=%d len=%d\n", __FUNCTION__, __LINE__, ulSrcModuleId, ulDstModuleId, u32MsgLen);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
 
    PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_COPY_MSG_SIZE_L, u32MsgLen);
 
    return PF_RET_SUCCESS;
}





/**********************************************************************************************
 * @API function  pf_copy_try_msg
 * @brief         Try to transfer general message interface function
                  stMboxMessages may be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 * @output        void
 * @return        0                     succuss
                  other                 failure
 * @date          2012/11/16
 *********************************************************************************************/
extern "C" S32 pf_copy_try_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void *pData, U32 ulLength)
{
    PS_CPlus(CM_COM, CMCOM_ID_COPY_TRY_MSG_IN_CNT);

    if((ulDstModuleId >= MODULE_MAX) || ((NULL == pData) && (ulLength > 0)))
    {
        PS_CPlus(CM_PES, CMPES_ID_COPY_TRY_MSG_IN_FAIL);
        return PF_RET_FAILURE;
    }

    void * destqueue = msgQArray[ulDstModuleId];
    if(NULL == destqueue)
    {
        PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
        PS_CPlus(CM_PES, CMPES_ID_COPY_TRY_MSGQ_NULL);
        return PF_RET_FAILURE;
    }

    U32 ulTicks = pf_get_ticks_ms();
    U32 ret;

    if(ulLength > SHORT_PACK_SIZE)
    {
        void* mdata = pf_malloc(ulLength);
        if(NULL == mdata)
        {
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            PS_CPlus(CM_PES, CMPES_ID_COPY_TRY_MSG_MALLOC_NULL);
            return PF_RET_FAILURE;
        }
        else
        {
            pf_memcpy(mdata, pData, ulLength);
        }

        PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_MALLOC_CNT);

        ret = pf_mbox_try_put(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, mdata, ulLength, ulTicks);
        if(false == ret)
        {
            pf_free(mdata);
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }
    else
    {
        ret = pf_mbox_try_put_copy_data(destqueue, ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
        if(false == ret)
        {
            PS_CPlus(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + ulDstModuleId));
            return PF_RET_FAILURE;
        }
    }

    PS_CPlus(CM_COM, CMCOM_ID_COPY_TRY_MSG_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_COPY_TRY_MSG_SIZE_L, ulLength);
    
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @function      get
 * @brief         Get the content of the message structure in the message queue
 * @input         void
 * @output        pstMsg                message infomation
 * @return        true                  succuss
                  false                 failure
 *********************************************************************************************/
template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::get(stMboxMessage& pstMsg)
{
    pthread_mutex_lock(&qlock);
    while(m_tail == m_head)
    {
        pthread_cond_wait(&qready,&qlock);
        //FUNCTION_TRACE;
    }
#ifdef GTEST_EN
    if(p_array)
    {
#endif
        pstMsg = p_array[m_head];
        m_head = (m_head + 1)%m_size;  
#ifdef GTEST_EN        
     }
#endif

    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);

    return true;
}

/**********************************************************************************************
 * @function      try_get
 * @brief         Try to get the content of the message structure in the message queue
 * @input         void
 * @output        pstMsg                message content
 * @return        true                  succuss
                  false                 failure
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::try_get(stMboxMessage& pstMsg)
{
    pthread_mutex_lock(&qlock);
    if(m_tail == m_head)
    {
        PS_CPlus(CM_PES, CMPES_ID_MBOX_TRY_GET_NULL);
        pthread_mutex_unlock(&qlock);
        return false;
    }

    pstMsg = p_array[m_head];
    m_head = (m_head + 1)%m_size;
    pthread_cond_signal(&qready);

    pthread_mutex_unlock(&qlock);
    return true;
}

/**********************************************************************************************
 * @function      try_put
 * @brief         Try to send a messaging interface that does not require a copy of the content. 
                  stMboxMessages may be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
                  false                 failure
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::try_put(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    pthread_mutex_lock(&qlock);
    if(((m_tail + 1) % m_size) == m_head)
    {
        PS_CPlus(CM_PES, CMPES_ID_MBOX_TRY_PUT_FULL);
        pthread_mutex_unlock(&qlock);
        return false;
    }

    stMboxMessage* pstMsg         = &p_array[m_tail];
    pstMsg->ulSrcModuleId   = ulSrcModuleId;
    pstMsg->ulMsgId         = ulMsgId;
    pstMsg->ulDstModuleId   = ulDstModuleId;
    pstMsg->ulLength        = ulLength;
    pstMsg->pucData         = (U8*)pData;
    pstMsg->ulTick          = ulTicks;
    m_tail = (m_tail + 1) % m_size;
    PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}

/**********************************************************************************************
 * @function      try_put_copy_data
 * @brief         Try to send a messaging interface that requires a copy of the content. 
                  stMboxMessages may be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
                  false                 failure
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::try_put_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    pthread_mutex_lock(&qlock);
    if(((m_tail + 1) % m_size) == m_head)
    {
        PS_CPlus(CM_PES, CMPES_ID_MBOX_TRY_PUT_COPY_FULL);
        pthread_mutex_unlock(&qlock);
        return false;
    }
#ifdef GTEST_EN
    if(p_array)
    {
#endif
        stMboxMessage* pstMsg         = &p_array[m_tail];
        pstMsg->ulSrcModuleId   = ulSrcModuleId;
        pstMsg->ulMsgId         = ulMsgId;
        pstMsg->ulDstModuleId   = ulDstModuleId;
        pstMsg->ulLength        = ulLength;
        pstMsg->pucData         = (U8*)&pstMsg->shortpayload[0];
        pstMsg->ulTick          = ulTicks;
        pf_memcpy(pstMsg->pucData, pData, ulLength);
        m_tail = (m_tail + 1) % m_size;
        PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
#ifdef GTEST_EN
    }
#endif
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}


/**********************************************************************************************
 * @function      put
 * @brief         Send a messaging interface that does not require a copy of the content. 
                  stMboxMessages will not be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::put(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    pthread_mutex_lock(&qlock);
    while(((m_tail + 1) % m_size) == m_head)
    {
        pthread_cond_wait(&qready,&qlock);
        PS_CPlus(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_FAIL);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_MSGID, ulMsgId);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_DEST_MODULEID, ulDstModuleId);
    }
    
#ifdef GTEST_EN
    if(p_array != NULL)
    {
#endif
        stMboxMessage* pstMsg         = &p_array[m_tail];
        pstMsg->ulSrcModuleId   = ulSrcModuleId;
        pstMsg->ulMsgId         = ulMsgId;
        pstMsg->ulDstModuleId   = ulDstModuleId;
        pstMsg->ulLength        = ulLength;
        pstMsg->pucData         = (U8*)pData;
        pstMsg->ulTick          = ulTicks;
        m_tail = (m_tail + 1) % m_size;
        PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId)); 
        PS_CMax(CM_PES, CMPES_ID_MBOX_PUT_MAX_SIZE, ((m_tail + m_size - m_head )% m_size)); 
#ifdef GTEST_EN
    }
#endif
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}

/**********************************************************************************************
 * @function      put_copy_data
 * @brief         Send a messaging interface that requires a copy of the content. 
                  stMboxMessages will not be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::put_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    pthread_mutex_lock(&qlock);
    while(((m_tail + 1) % m_size) == m_head)
    {
        pthread_cond_wait(&qready,&qlock);
        PS_CPlus(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_FAIL);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_MSGID, ulMsgId);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_DEST_MODULEID, ulDstModuleId);
    }
#ifdef GTEST_EN
    if(p_array)
    {
#endif
        stMboxMessage* pstMsg         = &p_array[m_tail];
        pstMsg->ulSrcModuleId   = ulSrcModuleId;
        pstMsg->ulMsgId         = ulMsgId;
        pstMsg->ulDstModuleId   = ulDstModuleId;
        pstMsg->ulLength        = ulLength;
        pstMsg->pucData         = (U8*)&pstMsg->shortpayload[0];
        pstMsg->ulTick          = ulTicks;
        pf_memcpy((void*)pstMsg->pucData, pData, ulLength);
        m_tail = (m_tail + 1) % m_size;
        PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
#ifdef GTEST_EN
    }
#endif
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}


template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::put_copy_data2(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, void* pData2, U32 ulLength2,U32 ulTicks)
{
    pthread_mutex_lock(&qlock);
    while(((m_tail + 1) % m_size) == m_head)
    {
        pthread_cond_wait(&qready,&qlock);
        PS_CPlus(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_FAIL);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_MSGID, ulMsgId);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_DEST_MODULEID, ulDstModuleId);
    }
    
    stMboxMessage* pstMsg         = &p_array[m_tail];
    pstMsg->ulSrcModuleId   = ulSrcModuleId;
    pstMsg->ulMsgId         = ulMsgId;
    pstMsg->ulDstModuleId   = ulDstModuleId;
    pstMsg->ulLength        = ulLength + ulLength2;
    pstMsg->pucData         = (U8*)&pstMsg->shortpayload[0];
    pstMsg->ulTick          = ulTicks;
    pf_memcpy((void*)pstMsg->pucData, pData, ulLength);
    pf_memcpy((U8*)pstMsg->pucData+ulLength, pData2, ulLength2);
    m_tail = (m_tail + 1) % m_size;
    PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}


/**********************************************************************************************
 * @function      try_put_head
 * @brief         Try to send a messaging interface that does not require a copy of the content. 
                  stMboxMessages may be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
                  false                 failure
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::try_put_head(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    S32 ret = pthread_mutex_trylock(&qlock);
    if(0 != ret)
    {
        PS_CPlus(CM_PES, CMPES_ID_MBOX_TRY_LOCK_PUT_FAIL);
        return false;
    }

    if(((m_tail + 1) % m_size) == m_head)
    {
        PS_CPlus(CM_PES, CMPES_ID_MBOX_TRY_LOCK_PUT_FULL_FAIL);
        pthread_mutex_unlock(&qlock);
        return false;
    }
    m_head = (m_head + m_size - 1)%m_size;
    stMboxMessage* pstMsg = &p_array[m_head];
    pstMsg->ulSrcModuleId   = ulSrcModuleId;
    pstMsg->ulMsgId         = ulMsgId;
    pstMsg->ulDstModuleId   = ulDstModuleId;
    pstMsg->ulLength        = ulLength;
    pstMsg->pucData         = (U8*)pData;
    pstMsg->ulTick          = ulTicks;
    PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}


/**********************************************************************************************
 * @function      try_put_head_copy_data
 * @brief         Try to send a messaging interface that requires a copy of the content. 
                  stMboxMessages may be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
                  false                 failure
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::try_put_head_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    S32 ret = pthread_mutex_trylock(&qlock);
    if(0 != ret)
    {
        PS_CPlus(CM_PES, CMPES_ID_MBOX_TRY_LOCK_PUT_COPY_FAIL);
        return false;
    }
    
    if(((m_tail + 1) % m_size) == m_head)
    {
        PS_CPlus(CM_PES, CMPES_ID_MBOX_TRY_LOCK_PUT_COPY_FULL_FAIL);
        pthread_mutex_unlock(&qlock);
        return false;
    }

    m_head = (m_head + m_size - 1)%m_size;
    stMboxMessage* pstMsg = &p_array[m_head];
    pstMsg->ulSrcModuleId   = ulSrcModuleId;
    pstMsg->ulMsgId         = ulMsgId;
    pstMsg->ulDstModuleId   = ulDstModuleId;
    pstMsg->ulLength        = ulLength;
    pstMsg->pucData         = (U8*)&pstMsg->shortpayload[0];
    pstMsg->ulTick          = ulTicks;
    pf_memcpy((void*)pstMsg->pucData, pData, ulLength);
    PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}


/**********************************************************************************************
 * @function      put_head
 * @brief         Send a messaging interface that does not require a copy of the content. 
                  stMboxMessages will not be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::put_head(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    pthread_mutex_lock(&qlock);

    while(((m_tail + 1) % m_size) == m_head)
    {
        pthread_cond_wait(&qready,&qlock);
        PS_CPlus(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_FAIL);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_MSGID, ulMsgId);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_DEST_MODULEID, ulDstModuleId);
    }

    m_head = (m_head + m_size - 1)%m_size;
    stMboxMessage* pstMsg = &p_array[m_head];
    pstMsg->ulSrcModuleId   = ulSrcModuleId;
    pstMsg->ulMsgId         = ulMsgId;
    pstMsg->ulDstModuleId   = ulDstModuleId;
    pstMsg->ulLength        = ulLength;
    pstMsg->pucData         = (U8*)pData;
    pstMsg->ulTick          = ulTicks;
    PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}

/**********************************************************************************************
 * @function      put_head_copy_data
 * @brief         Send a messaging interface that requires a copy of the content. 
                  stMboxMessages will not be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::put_head_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    pthread_mutex_lock(&qlock);

    while(((m_tail + 1) % m_size) == m_head)
    {
        pthread_cond_wait(&qready,&qlock);
        PS_CPlus(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_FAIL);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_MSGID, ulMsgId);
        PS_CSet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_DEST_MODULEID, ulDstModuleId);
    }

    m_head = (m_head + m_size - 1)%m_size;
    stMboxMessage* pstMsg = &p_array[m_head];
    pstMsg->ulSrcModuleId   = ulSrcModuleId;
    pstMsg->ulMsgId         = ulMsgId;
    pstMsg->ulDstModuleId   = ulDstModuleId;
    pstMsg->ulLength        = ulLength;
    pstMsg->pucData         = (U8*)&pstMsg->shortpayload[0];
    pstMsg->ulTick          = ulTicks;
    pf_memcpy((void*)pstMsg->pucData, pData, ulLength);
    PS_CPlus(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + ulDstModuleId));
    pthread_cond_signal(&qready);
    pthread_mutex_unlock(&qlock);
    return true;
}


/**********************************************************************************************
 * @function      size
 * @brief         get the number of message queue
 * @input         void
 * @output        void
 * @return        size                  the number of message queue
 *********************************************************************************************/

template < U32 QUEUE_SIZE>
int pf_mbox<QUEUE_SIZE>::size()
{
    return (m_tail + m_size - m_head )% m_size;
}

/**********************************************************************************************
 * @function      pf_mbox_create
 * @brief         Create message queue
 * @input         void
 * @output        void
 * @return        void*                 the pointer of message queue
 *********************************************************************************************/
const void* pf_mbox_create(inno_mbox* pmbox)
{
    pthread_mutexattr_t inherit_attr;    
    if(pthread_mutexattr_init(&inherit_attr) != 0) 
    {        
        pl_log(ERR, "pthread_mutexattr_init fail\n");         
        PS_CPlus(CM_PES, CMPES_ID_MBOX_CREATE_MUTEXATTR_INIT_FAIL);
        return NULL;    
    }  
    
    if(pthread_mutexattr_setprotocol(&inherit_attr, PTHREAD_PRIO_INHERIT) != 0) 
    {       
        pl_log(ERR, "pthread_mutexattr_setprotocol fail\n");        
        PS_CPlus(CM_PES, CMPES_ID_MBOX_CREATE_PROTOCOL_FAIL);
        return NULL;    
    }    

    if (pthread_mutex_init(&pmbox->qlock, &inherit_attr) != 0) 
    {        
        pl_log(ERR, "pthread_mutex_init fail\n");         
        PS_CPlus(CM_PES, CMPES_ID_MBOX_CREATE_MUTEX_INIT_FAIL);
        return NULL;    
    }    

    return (void*)pmbox;
}

/**********************************************************************************************
 * @function      pf_get_message
 * @brief         Get the header node of the message queue
 * @input         pMsgQ                 stMboxMessage queue address
 * @output        pstMsg                message structer infomation
 * @return        true                  succuss
 *********************************************************************************************/
int pf_get_message(void* pMsgQ, stMboxMessage& pstMsg)
{
    int ret;
    ret = ((inno_mbox*)pMsgQ)->get(pstMsg);
    pstMsg.restore();
    return ret;
}

/**********************************************************************************************
 * @function      pf_try_get_message
 * @brief         Attempt to get the header node of the message queue
 * @input         pMsgQ                 stMboxMessage queue address
 * @output        pstMsg                message structer infomation
 * @return        true                  succuss
                  false                 failure
 *********************************************************************************************/
int pf_try_get_message(void* pMsgQ, stMboxMessage& pstMsg)
{
    int ret;
    ret = ((inno_mbox*)pMsgQ)->try_get(pstMsg);
    return ret;
}

/**********************************************************************************************
 * @function      pf_mbox_peek
 * @brief         get the total number of message queue
 * @input         pMsgQ                 the address of message queue
 * @output        void
 * @return        size                  the number of message queue
 *********************************************************************************************/
int pf_mbox_peek(void* pMsgQ)
{
    if(pMsgQ)
    {
        return ((inno_mbox*)pMsgQ)->size();
    }
    else
    {
        return 0;
    }
}

/**********************************************************************************************
 * @function      pf_mbox_put
 * @brief         ���Ͳ���Ҫ���ݿ�������Ϣ���ݽӿڡ���Ϣ���ᶪʧ
 * @input         pMsgQ                 the address of message queue
 *                ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/
static inline int pf_mbox_put(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->put(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}

/**********************************************************************************************
 * @function      pf_mbox_put_copy_data
 * @brief         ������Ҫ���ݿ�������Ϣ���ݽӿڡ���Ϣ���ᶪʧ
 * @input         pMsgQ                 the address of message queue
 *                ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/
static inline int pf_mbox_put_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->put_copy_data(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}

static inline int pf_mbox_put_copy_data2(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, void* pData2, U32 ulLength2,U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->put_copy_data2(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, pData2, ulLength2, ulTicks);
}


/**********************************************************************************************
 * @function      pf_mbox_put_head
 * @brief         ���ͽ�����Ϣ����Ϣ�����ݲ���Ҫ��������Ϣ���ݽӿڡ���Ϣ���ᶪʧ
 * @input         pMsgQ                 the address of message queue
 *                ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/
static inline int pf_mbox_put_head(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->put_head(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}

/**********************************************************************************************
 * @function      pf_mbox_put_head
 * @brief         ���ͽ�����Ϣ����Ϣ��������Ҫ��������Ϣ���ݽӿڡ���Ϣ���ᶪʧ
 * @input         pMsgQ                 the address of message queue
 *                ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
 *********************************************************************************************/
static inline int pf_mbox_put_head_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->put_head_copy_data(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}


/**********************************************************************************************
 * @function      pf_mbox_try_put
 * @brief         ���Է��Ͳ���Ҫ���ݿ�������Ϣ���ݽӿڡ���Ϣ���ܶ�ʧ
 * @input         pMsgQ                 the address of message queue
 *                ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
                  false                 failure
 *********************************************************************************************/
static inline int pf_mbox_try_put(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->try_put(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}

/**********************************************************************************************
 * @function      pf_mbox_try_put_copy_data
 * @brief         ���Է�����Ҫ���ݿ�������Ϣ���ݽӿڡ���Ϣ���ܶ�ʧ
 * @input         pMsgQ                 the address of message queue
 *                ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 *                ulTicks               time of message generation
 * @output        void
 * @return        true                  success
                  false                 failure
 *********************************************************************************************/
static inline int pf_mbox_try_put_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->try_put_copy_data(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}

/**********************************************************************************************
 * @function      pf_mbox_try_put_head
 * @brief         ���Է��ͽ�����Ϣ����Ϣ��������Ҫ��������Ϣ���ݽӿڡ���Ϣ���ܶ�ʧ
 * @input         pMsgQ                 ��Ϣ���е�ַ
 *                usSrcModuleId         Դģ���ʶ
 *                usMsgId               ��Ϣ��ʶ
 *                usDstModuleId         Ŀ��ģ���ʶ
 *                pData                 ��Ϣ��ַ
 *                usLength              ��Ϣ����
 *                ulTicks               ��Ϣʱ��
 * @output        ��
 * @return        ture                  ���ͳɹ�
                  false                 ����ʧ��
 *********************************************************************************************/
static inline int pf_mbox_try_put_head(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->try_put_head(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}

/**********************************************************************************************
 * @function      pf_mbox_try_put_head_copy_data
 * @brief         ���Է��ͽ�����Ϣ����Ϣ�����ݲ���Ҫ��������Ϣ���ݽӿڡ���Ϣ���ܶ�ʧ
 * @input         pMsgQ                 ��Ϣ���е�ַ
 *                usSrcModuleId         Դģ���ʶ
 *                usMsgId               ��Ϣ��ʶ
 *                usDstModuleId         Ŀ��ģ���ʶ
 *                pData                 ��Ϣ��ַ
 *                usLength              ��Ϣ����
 *                ulTicks               ��Ϣʱ��
 * @output        ��
 * @return        ture                  ���ͳɹ�
                  false                 ����ʧ��
 *********************************************************************************************/

static inline int pf_mbox_try_put_head_copy_data(void* pMsgQ, U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks)
{
    return ((inno_mbox*)pMsgQ)->try_put_head_copy_data(ulSrcModuleId, ulMsgId, ulDstModuleId, pData, ulLength, ulTicks);
}

/**********************************************************************************************
 * @function      msg_entry
 * @brief         the entry of thread
 * @input         mid                   message queue identification
 * @output        void
 * @return        void
 *********************************************************************************************/
#include <sched.h>
void msg_entry(pf_addrword_t mid)
{
    stMboxMessage msg;
    U32 ulDstId = MODULE_TASK_MAX;
 
    if(moduleInitArray[mid])
    {
        moduleInitArray[mid](mid);
        //printf("msg_entry module init %d \r\n", mid);
        printf("msg_entry module[%d] %s init , pf_get_thread_mid %d\r\n", mid, pf_get_module_name(mid), pf_get_thread_mid());
    }

    if((aulModuleCpucoreMask[mid].mask0 != 0 )||(aulModuleCpucoreMask[mid].mask1 != 0 )
        ||(aulModuleCpucoreMask[mid].mask2 != 0 )||(aulModuleCpucoreMask[mid].mask3 != 0 ))
    {
        pf_set_thread_cpucore(0,aulModuleCpucoreMask[mid]);
    }
    
    if(pf_thread_mon_interval_debug_is_open())
    {
        long time_mArray_bef = 0, time_mArray_now = 0, time_mArray = 0;
        
        while(1)
        {
            pf_get_message(msgQArray[(int)mid], msg);
            ulDstId = msg.ulDstModuleId;

            PS_CPlus(CM_COM, CMCOM_ID_GET_MSG_CNT);
            
            PS_CPlus(CM_RMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + ulDstId));

            pf_thread_mon_update(mid, msg.ulSrcModuleId, msg.ulMsgId, ulDstId);
            /*This is about execution time test*/
            time_mArray_bef = pf_get_ticks_ns();

            moduleArray[ulDstId](msg.ulSrcModuleId, msg.ulMsgId, ulDstId, msg.pucData, msg.ulLength);
            pf_thread_mon_update_count(mid);

            time_mArray_now = pf_get_ticks_ns();
            
            time_mArray = time_mArray_now - time_mArray_bef;
            if(time_mArray > PS_CGet(CM_MMAX, ulDstId))
            {
                PS_CSet(CM_MMID, ulDstId, msg.ulMsgId);
                PS_CSet(CM_MMAX, ulDstId, time_mArray);    
            }

            PS_CPlusV(CM_MTT, ulDstId, time_mArray);

            msg.free();
        }
    }
    else
    {
        while(1)
        {
            pf_get_message(msgQArray[(int)mid], msg);
            ulDstId = msg.ulDstModuleId;

            PS_CPlus(CM_COM, CMCOM_ID_GET_MSG_CNT);
            
            PS_CPlus(CM_RMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + ulDstId));

            pf_thread_mon_update(mid, msg.ulSrcModuleId, msg.ulMsgId, ulDstId);

            moduleArray[ulDstId](msg.ulSrcModuleId, msg.ulMsgId, ulDstId, msg.pucData, msg.ulLength);
            pf_thread_mon_update_count(mid);

            msg.free();
        }
    }
}



void pf_create_module_group(
    pf_addrword_t pri,                      /* scheduling info (eg pri)  */
    CHAR* module_name,                      /* module thread name        */
    U32 module_id,                          /* module thread handle id   */
    MODULE_ENTRY module_entry,              /* entry point function      */
    MODULE_INIT module_init,                /* module init function      */
    void* module_mbox,                      /* module msg box            */
    void* modulestack,                      /* stack base                */
    U32 stack_size,                         /* stack size                */
    U32 log_size                            /* log size                  */
    )
{
    moduleArray[module_id] = module_entry;                   
    moduleInitArray[module_id] = module_init; 	               
    msgQArray[module_id] = (void*)pf_mbox_create((inno_mbox*)module_mbox);  
    pf_thread_create_mid((pf_addrword_t)pri, msg_entry, (pf_addrword_t)module_id, module_name, (void*)modulestack, stack_size,module_id, log_size); 
    pf_thread_resume(workerhandles[module_id]);          
}

