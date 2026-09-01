/************************************************************
  Copyright (C), BroadXt Inc  2019
  FileName: pf_mbox.h
  Author: josephzhou    Version :  1.0   Date: 20190617
  Description:     header of platform
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2019/6/17         1.0        initial
***********************************************************/
#ifndef _PF_MBOX_H
#define _PF_MBOX_H


template <U32 QUEUE_SIZE>
class pf_mbox
{
    private:
        pthread_cond_t qready;
        stMboxMessage* p_array;
        int m_size ;
        int m_tail;
        int m_head;
    public:
        pthread_mutex_t qlock; 
        pf_mbox()
        {
            qready = PTHREAD_COND_INITIALIZER;
            //qlock = PTHREAD_MUTEX_INITIALIZER;
            p_array = (stMboxMessage*)malloc(QUEUE_SIZE * sizeof(stMboxMessage));
            m_size = QUEUE_SIZE;
            m_tail = 0;
            m_head = 0;
        };

        ~pf_mbox()
        {
            free(p_array);
        };

        int get(stMboxMessage& pstMsg);
        int try_get(stMboxMessage& pstMsg);
        int put(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int try_put(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int put_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int put_copy_data2(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, void* pData2, U32 ulLength2,U32 ulTicks);
        int try_put_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int put_head(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int put_head_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int try_put_head(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int try_put_head_copy_data(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength, U32 ulTicks);
        int size();
};

typedef pf_mbox<MESSAGE_QUEUE_SIZE> inno_mbox;

/**********************************************************************************************
 * @function      pf_mbox_create
 * @brief         Create message queue
 * @input         void
 * @output        void
 * @return        void*                 the pointer of message queue
 *********************************************************************************************/
const void* pf_mbox_create(inno_mbox* pmbox);

/**********************************************************************************************
 * @function      pf_get_message
 * @brief         Get the header node of the message queue
 * @input         pMsgQ                 stMboxMessage queue address
 * @output        pstMsg                message structer infomation
 * @return        true                  succuss
 *********************************************************************************************/
int pf_get_message(void* pMsgQ, stMboxMessage& pstMsg);

/**********************************************************************************************
 * @function      pf_try_get_message
 * @brief         Attempt to get the header node of the message queue
 * @input         pMsgQ                 stMboxMessage queue address
 * @output        pstMsg                message structer infomation
 * @return        true                  succuss
                  false                 failure
 *********************************************************************************************/
int pf_try_get_message(void* pMsgQ, stMboxMessage& pstMsg);

/**********************************************************************************************
 * @function      pf_mbox_peek
 * @brief         get the total number of message queue
 * @input         pMsgQ                 the address of message queue
 * @output        void
 * @return        size                  the number of message queue
 *********************************************************************************************/
int pf_mbox_peek(void* pMsgQ);

#define THREAD_ARRAY_MAX_ID 512
#define THREAD_ARRAY_OFFSET 20


#endif//_PF_MBOX_H

