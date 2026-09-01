#ifndef __ECOS_COMM_H__
#define __ECOS_COMM_H__

#include "pl.h"

#define COMMBUF_DEBUG
#define COMMQUE_DEBUG
#define COMMBUF_URGENT_MALLOC


#ifdef MEMORY_IN_MEMPOOL
#define pl_mempool_log(...)
#else
#define pl_mempool_log  pl_log
#endif

#ifndef U16_INFINITY
#define U16_INFINITY 0xffff
#endif
#ifndef U32_INFINITY 
#define U32_INFINITY 0xffffffff
#endif

typedef struct
{
    U32 ulElemNum;
    U32 ulQueHead;
    U32 ulQueTail;
#ifdef COMMBUF_DEBUG
    U64 ullInCnt;
    U64 ullOutCnt;
#endif
    U32 *aulQueElem;
    const CHAR *pucQueName;
}QUE_CTRL_S;


typedef struct
{
    U32 ulBufSize;
    U32 ulBufNum;
    U8 *pucBufStart;
    U8 * pucBufEnd;
    QUE_CTRL_S *pstQueCtrl;
    
#ifdef COMMBUF_DEBUG
    U64 ullInCnt;
    U64 ullOutCnt;
    U16 usThreadId;
#endif

    U32 ulUrgentUsedNum;
    U32 ulMaxUsedNum;
    const CHAR *pucBufName;
    PF_MUTEX_T *pMutex;
    U32 ucWithAddress;
}BUF_CTRL_S;

void pl_commbuf_init(void);
void* pl_commbuf_malloc(U32 ulSize);
void pl_commbuf_free(void* pBuff);

BUF_CTRL_S *pl_commbuf_create(U32 ulBufSize, U32 ulBufNum, const CHAR *pucBufName);

BUF_CTRL_S *pl_commbuf_create_with_address(U8 *pucBufStart, U8 *pucBufEnd, U32 ulBufSize, U32 ulBufNum, const CHAR *pucBufName);
U32 pl_commbuf_destroy(BUF_CTRL_S *pstBufCtrl);
U32 pl_commbuf_mutex_init(BUF_CTRL_S *pstBufCtrl, PF_MUTEX_T *pMutex);
void *pl_commbuf_get(BUF_CTRL_S *pstBufCtrl);
U32 pl_commbuf_ret(BUF_CTRL_S *pstBufCtrl, void *pBufRet);
U32 pl_commbuf_avail_num(BUF_CTRL_S *pstBufCtrl);
void pl_commbuf_statistic(void);
void pl_commbuf_clear_statics(void);
void pl_commbuf_get_total_available_membytes(U64 *pullAvailMemBytes, U64 *pullTotalMemBytes);
void pl_commbuf_log(const BUF_CTRL_S *pstBufCtrl);
void pl_commbuf_clear(void);
    
QUE_CTRL_S *pl_que_create(U32 ulElemNum, const CHAR *pucQueName);
void pl_que_destroy(QUE_CTRL_S *pstQueCtrl);
U64 pl_que_get(QUE_CTRL_S *pstQueCtrl);
U16 pl_que_put(QUE_CTRL_S *pstQueCtrl, U32 ulElemPut);
U32 pl_que_avail_num(QUE_CTRL_S *pstQueCtrl);
void pl_que_statistic(void);
void pl_que_log(const QUE_CTRL_S *pstQueCtrl);


#endif
