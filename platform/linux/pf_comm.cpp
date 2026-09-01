/*******************************************************************************
**
**  Copyright (c)  2013,  innofidei, Inc.                                                                              
**      All Rights Reserved.                                                                                           
**                                                                                                                     
**  Subsystem   : LTE SMALL CELL                                                                                      
**  File        : txpdcp.cpp                                                                                    
**  Created By  : zhuwenbo
**  Created On  : 2013/4/25
**                                                                                                                     
**  Purpose:                                                                                                           
**  This file contains the main functions of commbuffer tool.                                              
**                                                                                                                     
**  History:                                                                                                           
**  Version        Programmer    Date        Description    
**  -----------   ------------    ----------    --------------------
**   1.0                         2013/4/25   initial version   
**    
**       
*******************************************************************************/

/*
特性列表:

外部使用特性
* create接口提供了buf/que名称的参数，为该数据结构命名，方便用户调试
* 线程检查:commbuf get/ret操作必须由同一个线程执行，get/ret时会作检查
* commbuf的各个buffer块都没有前后地址保护，未来有需求的话可以扩展接口
* commbuf的各个buffer块地址四字节对齐，实际做法是将传入的size参数上取整到4的倍数
* destroy commbuf时必须保证buf为空，会执行该检查
* 当COMMBUF_URGENT_MALLOC打开时，如果get不到元素，会进行malloc操作并report，外部接口不可见
* 建议本模块的默认打印级别为3

* 新增with_address功能，由外部传入起末地址范围:
*     调用方会申请一大片连续区域然后将其中连续区域按照commbuffer进行内存块组织
*    commbuffer会在创建时检查start和end的差值与size*num是否匹配
* 新增mutex功能，通过pl_commbuf_mutex_init功能初始化锁:
*    调用方如果初始化锁，则get/ret都会使用该锁；否则不用
* 以上两个新增功能是用于平台的mempool实现，另外需要注意:
*    不进行size和num的范围检查；不进行thread_id的检查；
*    URGENT_MALLOC功能仍需开启，但是当with_address时不进行urgent_malloc而是返回错误

内部调试特性
* 在COMMBUF_DEBUG打开时，有inCnt/outCnt计数，用于记录当前buf进出以及que进出的总数
* commbuf中的队列元素可以是index或是内存地址，前者方便调试，由COMMBUF_DEBUG宏区分
* 在COMMBUF_DEBUG打开时，MC周期打印会report全局所有的buf和que信息，打印级别为4
*/

#define THIS_MODULE MODULE_COMM
#include "pl_comm.h"

#define COMM_SUCC       0
#define COMM_FAIL       U16_INFINITY


#define COMMBUF_MUTEX_LOCK(pMutex)      (((pMutex) != NULL) && PF_MUTEX_LOCK(pMutex))
#define COMMBUF_MUTEX_UNLOCK(pMutex)    (((pMutex) != NULL) && PF_MUTEX_UNLOCK(pMutex))

#define MAX_COMMBUF_NUM     64
#define MAX_COMMBUF_SIZE    1024
#define MAX_COMMBUF_ELEM_NUM     (1024 * 1024)
static BUF_CTRL_S *g_pastCommBufferGlobal[MAX_COMMBUF_NUM] = {NULL};
static U32 g_pstCommBufferTotalNum = 0;

#define MAX_QUE_NUM 128
#define MAX_QUE_ELEM_NUM    (1024 * 1024)
static QUE_CTRL_S *g_pastQueGlobal[MAX_QUE_NUM] = {NULL};
static U32 g_pstQueTotalNum = 0;


static U32 g_ulBufGetFailCount = 0;

PF_MUTEX_T g_stCommBufferMutex;
PF_MUTEX_T g_stQueMutex;

U64 g_ulPSCnts[CM_MAX][CI_MAX];      /*计数器*/


/*******************************************************************************
Function:
void pl_commbuf_init(void)
Description: 
    初始化commbuffer的全局资源
Input:
    void 
Output:
    void
Return: 
    void
Others:        
*******************************************************************************/
void pl_commbuf_init(void)
{
    PF_MUTEX_INIT(&g_stCommBufferMutex);
    PF_MUTEX_INIT(&g_stQueMutex);
}

/*******************************************************************************
Function:
void* pl_commbuf_malloc(U32 ulSize)
Description: 
    利用操作系统接口申请一个内存
Input:
    ulSize:  待申请的内存大小
Output:
    void
Return: 
   成功: 申请得到的the address of memory
   失败: NULL
Others:        
*******************************************************************************/
void* pl_commbuf_malloc(U32 ulSize)
{    
    if(ulSize)
    {
        void* pAddr = malloc(ulSize);
        if(NULL == pAddr)
        {
            pl_mempool_log(ERR, "failed RETURN NULL ulSize=%d\n", ulSize);
            /*printf("%s:failed RETURN NULL ulSize=%d\n", __FUNCTION__, ulSize);*/
#pragma warning(disable: 4127)
            PS_CPlus(CM_PES, CMPES_ID_COMMBUF_MALLOC_NULL);
            PS_CPlusV(CM_PES, CMPES_ID_COMMBUF_MALLOC_NULL_SIZE, ulSize);
#pragma warning(default: 4127)
            return NULL;
        }

        PS_CPlus(CM_COM, CMCOM_ID_MALLOC_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_MALLOC_SIZE, ulSize);

        pl_mempool_log(INF, "ComMalloc:0x%x\n", (U32)pAddr);
        return pAddr;
    }
    else
    {
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_MALLOC_IN_NULL);
    }
    
    return NULL;
}

/*******************************************************************************
Function:
void pl_commbuf_free(void* pBuff)
Description: 
    利用操作系统接口释放一个内存
Input:
    pBuff:  待释放的内存
Output:
    void
Return: 
    void
Others:        
*******************************************************************************/
void pl_commbuf_free(void* pBuff)
{
    if(pBuff)
    {
        PS_CPlus(CM_COM, CMCOM_ID_FREE_CNT);
        pl_mempool_log(INF, "ComFree:0x%x", (U32)pBuff);
        free(pBuff);
    }
    else
    {
        pl_mempool_log(ERR, "free failed INPUT NULL");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_FREE_NULL);
    }
}



/*******************************************************************************
Function:
BUF_CTRL_S *pl_commbuf_create(U32 ulBufSize, U32 ulBufNum, const CHAR *pucBufName)
Description: 
    创建一块commbuffer，该commbuffer的内存区域malloc得到
Input:
    ulBufSize: 块大小
    ulBufNum: 块个数
    pucBufName: commbuffer名称，用于调试，可不填
Output:
    void
Return: 
    成功: commbuffer句柄
    失败: NULL
Others:        
*******************************************************************************/
BUF_CTRL_S *pl_commbuf_create(U32 ulBufSize, U32 ulBufNum, const CHAR *pucBufName)
{
    BUF_CTRL_S *pstBufCtrl = NULL;
    U32 ulBufIdx = 0;
    
    if ((ulBufSize > MAX_COMMBUF_SIZE)
        || (ulBufNum > MAX_COMMBUF_ELEM_NUM))  
    {
        pl_mempool_log(ERR, "illegal parameter bufsize=%d, bufnum=%d", ulBufSize, ulBufNum);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_NULL);
        return NULL;
    }

    if (g_pstCommBufferTotalNum >= MAX_COMMBUF_NUM)
    {
        pl_mempool_log(ERR, "CommBufferTotalNum reaches maximum %d", g_pstCommBufferTotalNum);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_MAX_NUM);
        return NULL;          
    }
    
    pstBufCtrl = (BUF_CTRL_S*)malloc(sizeof(BUF_CTRL_S));
    if (pstBufCtrl == NULL)
    {
        pl_mempool_log(ERR, "malloc BUF_CTRL_S fail");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_MALLOC_NULL);
        return NULL;
    }  

    ulBufSize = (ulBufSize + 3) & 0xfffffffc;   //multiple of 4
    
    pstBufCtrl->pucBufStart = (U8*)malloc(ulBufSize * ulBufNum);     //Notice: Bufferblock without cache alignment
    if (NULL == pstBufCtrl->pucBufStart)
    {
        pl_mempool_log(ERR, "malloc Data Buf fail");
        free(pstBufCtrl);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_BUFSTART_NULL);
        return NULL;
    }
    
    pstBufCtrl->pstQueCtrl = pl_que_create(ulBufNum, NULL);       //create bufferblock que
    if (NULL == pstBufCtrl->pstQueCtrl)
    {
        pl_mempool_log(ERR, "pl_que_create fail");
        free(pstBufCtrl->pucBufStart);
        free(pstBufCtrl);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUECTRL_NULL);
        return NULL;
    }

    PF_MUTEX_LOCK(&g_stCommBufferMutex);
    g_pastCommBufferGlobal[g_pstCommBufferTotalNum++] = pstBufCtrl;      //record global info
    PF_MUTEX_UNLOCK(&g_stCommBufferMutex);

    if (pucBufName == NULL)
    {
        pstBufCtrl->pucBufName = "";
    }
    else
    {
        pstBufCtrl->pucBufName = pucBufName;
    }
    
    pstBufCtrl->ulBufSize = ulBufSize;
    pstBufCtrl->ulBufNum = ulBufNum;
    pstBufCtrl->ulMaxUsedNum = 0;
    pstBufCtrl->ulUrgentUsedNum = 0;    
#ifdef COMMBUF_DEBUG
    pstBufCtrl->ullInCnt = 0;
    pstBufCtrl->ullOutCnt = 0;
    pstBufCtrl->usThreadId = U16_INFINITY;
#endif
    pstBufCtrl->pucBufEnd = pstBufCtrl->pucBufStart + ulBufSize*ulBufNum;
    pstBufCtrl->ucWithAddress = FALSE;
    pstBufCtrl->pMutex = NULL;

#ifdef COMMBUF_DEBUG
    for (ulBufIdx = 0; ulBufIdx < ulBufNum; ulBufIdx++)     //put all bufferblock index into que
    {
        pl_que_put(pstBufCtrl->pstQueCtrl, ulBufIdx);        
    }
#else
    for (ulBufIdx = 0; ulBufIdx < ulBufNum; ulBufIdx++)     //put all bufferblock address into que
    {
        pl_que_put(pstBufCtrl->pstQueCtrl, (U32)(pstBufCtrl->pucBufStart + ulBufIdx * pstBufCtrl->ulBufSize));        
    }
#endif

    pl_mempool_log(INF, "Buf(%s) successful, size=%d, num=%d, BufStart=%08x", 
        pstBufCtrl->pucBufName, pstBufCtrl->ulBufSize, pstBufCtrl->ulBufNum, pstBufCtrl->pucBufStart);
    
    return pstBufCtrl;
}

/*******************************************************************************
Function:
U16 pl_commbuf_destroy(BUF_CTRL_S *pstBufCtrl)
Description: 
   销毁一块commbuffer
Input:
    pstBufCtrl: 欲销毁的commbuffer句柄
Output:
    void
Return: 
    成功: 0
    失败: 错误码
Others:        
*******************************************************************************/
U32 pl_commbuf_destroy(BUF_CTRL_S *pstBufCtrl)
{
    U32 ulAvailNum;
    U32 ulCommBufIdx;
    
    if ((pstBufCtrl == NULL)
        || (pstBufCtrl->pucBufStart == NULL)
        || (pstBufCtrl->pstQueCtrl == NULL))
    {
        pl_mempool_log(ERR, "illegal parameter, pstBufCtrl=0x%08x", pstBufCtrl);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_DESTROY_NULL);
        return COMM_FAIL;
    }

    ulAvailNum = pl_commbuf_avail_num(pstBufCtrl);
    if (ulAvailNum != pstBufCtrl->ulBufNum)
    {
        pl_mempool_log(ERR, "Buf(%s) fail: Buf Not Empty, availnum(%d)<BufNum(%d)",
            pstBufCtrl->pucBufName, ulAvailNum, pstBufCtrl->ulBufNum);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_DESTROY_BUF_NOT_EMPTY);
        return 1;
    }

    PF_MUTEX_LOCK(&g_stCommBufferMutex);
    for (ulCommBufIdx = 0; ulCommBufIdx < g_pstCommBufferTotalNum; ulCommBufIdx++)
    {
        if (g_pastCommBufferGlobal[ulCommBufIdx] == pstBufCtrl)
        {
            break;
        }
    }
    
    if (ulCommBufIdx == g_pstCommBufferTotalNum)        //NOT Found indicated
    {
        pl_mempool_log(FATAL, "cannot find indicated Buf(%s), but still destroy it", pstBufCtrl->pucBufName);
        PF_MUTEX_UNLOCK(&g_stCommBufferMutex);
        
        pl_que_destroy(pstBufCtrl->pstQueCtrl);
        if (FALSE == pstBufCtrl->ucWithAddress)
        {        
            free(pstBufCtrl->pucBufStart);
        }
        free(pstBufCtrl);
        
        return COMM_SUCC;
    }
    
    g_pastCommBufferGlobal[ulCommBufIdx] = g_pastCommBufferGlobal[--g_pstCommBufferTotalNum];
    PF_MUTEX_UNLOCK(&g_stCommBufferMutex);

    pl_mempool_log(INF, "Buf(%s) successful", pstBufCtrl->pucBufName);
    pl_que_destroy(pstBufCtrl->pstQueCtrl);
    if (FALSE == pstBufCtrl->ucWithAddress)
    {
        free(pstBufCtrl->pucBufStart);
    }
    free(pstBufCtrl);
    
    return COMM_SUCC;
}


/*******************************************************************************
Function:
BUF_CTRL_S *pl_commbuf_create_with_address(U8 *pucBufStart, U8 *pucBufEnd, U32 ulBufSize, U32 ulBufNum, const CHAR *pucBufName)
Description: 
    创建一块commbuffer，该commbuffer的内存区域由调用方提供
Input:
    pucBufStart: 内存区域起始位置
    pucBufEnd: 内存区域结束位置的下一个byte
    ulBufSize: 块大小
    ulBufNum: 块个数
    pucBufName: commbuffer名称，用于调试，可不填
Output:
    void
Return: 
    成功: commbuffer句柄
    失败: NULL
Others:        
*******************************************************************************/
BUF_CTRL_S *pl_commbuf_create_with_address(U8 *pucBufStart, U8 *pucBufEnd, U32 ulBufSize, U32 ulBufNum, const CHAR *pucBufName)
{
    BUF_CTRL_S *pstBufCtrl = NULL;
    U8 *pucBufEndCalc = NULL;
    U32 ulBufIdx = 0;

    if (g_pstCommBufferTotalNum >= MAX_COMMBUF_NUM)
    {
        pl_mempool_log(ERR, "CommBufferTotalNum reaches maximum %d", g_pstCommBufferTotalNum);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_ADDRESS_MAX);
        return NULL;          
    }

    if ((NULL == pucBufStart)
        || (NULL == pucBufEnd))
    {
        pl_mempool_log(ERR, "address NULL pucBufStart=0x%08x, pucBufEnd=0x%08x", 
            pucBufStart, pucBufEnd);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_ADDRESS_NULL);
        return NULL;
    }

    if (0 != ((U64)pucBufStart & 0x03))
    {
        pl_mempool_log(ERR, "pucBufStart NOT multiplier of 4, pucBufStart=0x%08x", 
            pucBufStart);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_ADDRESS_NOT_MULTI);
        return NULL;
    }
    
    pstBufCtrl = (BUF_CTRL_S*)malloc(sizeof(BUF_CTRL_S));
    if (pstBufCtrl == NULL)
    {
        pl_mempool_log(ERR, "malloc BUF_CTRL_S fail");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_ADDRESS_MALLOC_NULL);
        return NULL;
    }  

    ulBufSize = (ulBufSize + 3) & 0xfffffffc;   //multiple of 4
    
    pstBufCtrl->pucBufStart = pucBufStart;
    pucBufEndCalc = pstBufCtrl->pucBufStart + ulBufSize*ulBufNum;
    if (pucBufEndCalc != pucBufEnd)
    {
        pl_mempool_log(ERR, "check BufEnd fail: EndInput=0x%08x, EndCalc=0x%08x, BufSize=%d, BufNum=%d",
            pucBufEnd, pucBufEndCalc, ulBufSize, ulBufNum);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_ADDRESS_BUFEND_FAIL);
        free(pstBufCtrl);
        return NULL;
    }
    pstBufCtrl->pucBufEnd = pucBufEnd;
    pstBufCtrl->ucWithAddress = TRUE;
    pstBufCtrl->pMutex = NULL;
    
    pstBufCtrl->pstQueCtrl = pl_que_create(ulBufNum, pucBufName);       //create bufferblock que
    if (NULL == pstBufCtrl->pstQueCtrl)
    {
        pl_mempool_log(ERR, "pl_que_create fail");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_CREATE_ADDRESS_QUECTRL_NULL);
        free(pstBufCtrl);
        return NULL;
    }

    PF_MUTEX_LOCK(&g_stCommBufferMutex);
    g_pastCommBufferGlobal[g_pstCommBufferTotalNum++] = pstBufCtrl;      //record global info
    PF_MUTEX_UNLOCK(&g_stCommBufferMutex);

    if (pucBufName == NULL)
    {
        pstBufCtrl->pucBufName = "";
    }
    else
    {
        pstBufCtrl->pucBufName = pucBufName;
    }
    
    pstBufCtrl->ulBufSize = ulBufSize;
    pstBufCtrl->ulBufNum = ulBufNum;
    pstBufCtrl->ulMaxUsedNum = 0;
    pstBufCtrl->ulUrgentUsedNum = 0;    
#ifdef COMMBUF_DEBUG
    pstBufCtrl->ullInCnt = 0;
    pstBufCtrl->ullOutCnt = 0;
    pstBufCtrl->usThreadId = U16_INFINITY;
#endif

#ifdef COMMBUF_DEBUG
    for (ulBufIdx = 0; ulBufIdx < ulBufNum; ulBufIdx++)     //put all bufferblock index into que
    {
        pl_que_put(pstBufCtrl->pstQueCtrl, ulBufIdx);        
    }
#else
    for (ulBufIdx = 0; ulBufIdx < ulBufNum; ulBufIdx++)     //put all bufferblock address into que
    {
        pl_que_put(pstBufCtrl->pstQueCtrl, (U32)(pstBufCtrl->pucBufStart + ulBufIdx * pstBufCtrl->ulBufSize));        
    }
#endif

    pl_mempool_log(INF, "Buf(%s) successful, size=%d, num=%d, BufStart=%08x", 
        pstBufCtrl->pucBufName, pstBufCtrl->ulBufSize, pstBufCtrl->ulBufNum, pstBufCtrl->pucBufStart);
    
    return pstBufCtrl;
}

/*******************************************************************************
Function:
U32 pl_commbuf_mutex_init(BUF_CTRL_S *pstBufCtrl, PF_MUTEX_T *pMutex)
Description: 
    初始化一块commbuffer的mutex
    不要求一块commbuffer要通过本接口去初始化mutex
    判断依据是看有多少线程会去访问该commbuffer
Input:
    pstBufCtrl: 待操作的commbuffer句柄
Output:
    void
Return: 
    成功: 0
    失败: 错误码
Others:        
*******************************************************************************/
U32 pl_commbuf_mutex_init(BUF_CTRL_S *pstBufCtrl, PF_MUTEX_T *pMutex)
{
    if ((NULL == pstBufCtrl) 
        || (NULL == pMutex))
    {
        pl_log(ERR, "mutex init parameter fail");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_MUTEX_INIT_NULL);
        return COMM_FAIL;
    }
    
    pstBufCtrl->pMutex = pMutex;

    return COMM_SUCC;
}

/*******************************************************************************
Function:
void *pl_commbuf_get(BUF_CTRL_S *pstBufCtrl)
Description: 
   从一块commbuffer中获取一块空闲的内存单元
   如果没有空闲的，有可能会从系统中临时申请一个the address of memory
Input:
    pstBufCtrl: 待操作的commbuffer句柄
Output:
    void
Return: 
    成功: 空闲的the address of memory
    失败: NULL
Others:        
*******************************************************************************/
void *pl_commbuf_get(BUF_CTRL_S *pstBufCtrl)
{
#ifdef COMMBUF_DEBUG
    U16 usThreadId;
#endif
    U32 ulUsedNum;
    void *pRet;
    
    if (pstBufCtrl == NULL)
    {
        pl_mempool_log(ERR, "illegal parameter pstBufCtrl=0x%08x", pstBufCtrl);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_GET_NULL);
        return NULL;
    }

#ifdef COMMBUF_DEBUG        
#ifndef MEMORY_IN_MEMPOOL
    if (NULL == pstBufCtrl->pMutex) //if no mutex, threadId check
    {
        usThreadId = PF_THREAD_SELF();
        if (pstBufCtrl->usThreadId == U16_INFINITY)     //first action, record threadid
        {
            pstBufCtrl->usThreadId = usThreadId;
        }
        else if (pstBufCtrl->usThreadId != usThreadId)
        {
            pl_mempool_log(ERR, "Buf(%s) check thread id fail: creater=%d, caller=%d", 
                pstBufCtrl->pucBufName, pstBufCtrl->usThreadId, usThreadId);
            PS_CPlus(CM_PES, CMPES_ID_COMMBUF_GET_THREAD_FAIL);
            return NULL;
        }
    }
#endif
#endif

    COMMBUF_MUTEX_LOCK(pstBufCtrl->pMutex);
    pRet = (void*)pl_que_get(pstBufCtrl->pstQueCtrl);
    COMMBUF_MUTEX_UNLOCK(pstBufCtrl->pMutex);
    if (pRet == (void*)U32_INFINITY) 
    {
        if (TRUE == pstBufCtrl->ucWithAddress)
        {
            pl_mempool_log(WARN, "Buf(%s) fail for CommbufWithAddress", pstBufCtrl->pucBufName);
            g_ulBufGetFailCount++;
            PS_CPlus(CM_PES, CMPES_ID_COMMBUF_GET_ADDRESS_FAIL);
            return NULL;
        }
    
#ifdef COMMBUF_URGENT_MALLOC        
        pRet = pl_commbuf_malloc(pstBufCtrl->ulBufSize);
        if (NULL == pRet)
        {
            pl_mempool_log(FATAL, "Buf(%s) urgent malloc fail, bufSize=%d", 
                pstBufCtrl->pucBufName, pstBufCtrl->ulBufSize, pRet);
            PS_CPlus(CM_PES, CMPES_ID_COMMBUF_GET_MALLOC_FAIL);
            return NULL;
        }
        else
        {
            pl_mempool_log(WARN, "Buf(%s) urgent malloc, pRet=0x%08x, bufSize=%d, bufNum=%d",
                pstBufCtrl->pucBufName, pRet, pstBufCtrl->ulBufSize, pstBufCtrl->ulBufNum);
        }
        
        pstBufCtrl->ulUrgentUsedNum++;
        ulUsedNum = pstBufCtrl->ulBufNum + pstBufCtrl->ulUrgentUsedNum;
        if (ulUsedNum > pstBufCtrl->ulMaxUsedNum)
        {
            pstBufCtrl->ulMaxUsedNum = ulUsedNum;
        }
#else
        pl_mempool_log(WARN, "Buf(%s) pl_que_get fail: bufNum=%d", 
            pstBufCtrl->pucBufName, pstBufCtrl->ulBufNum);
        pRet = NULL;
#endif
        g_ulBufGetFailCount++;
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_GET_FAIL);
        return pRet;
    }

#ifdef COMMBUF_DEBUG
    pl_mempool_log(TRC, "BUF(%s) successful, BufIndex=%d, BufStart=%08x, BufGet=%08x, InCnt=%lld, OutCnt=%lld", 
                    pstBufCtrl->pucBufName, pRet, pstBufCtrl->pucBufStart,
                    pRet, pstBufCtrl->ullInCnt, pstBufCtrl->ullOutCnt);
    pstBufCtrl->ullOutCnt++;
#endif
    pRet = (void*)(pstBufCtrl->pucBufStart + ((U64)pRet) * pstBufCtrl->ulBufSize);   //BufIndex --> pRet
    ulUsedNum = pstBufCtrl->ulBufNum - pl_que_avail_num(pstBufCtrl->pstQueCtrl);
    if (ulUsedNum > pstBufCtrl->ulMaxUsedNum)
    {
        pstBufCtrl->ulMaxUsedNum = ulUsedNum;
    }

    return pRet;
}

/*******************************************************************************
Function:
U32 pl_commbuf_ret(BUF_CTRL_S *pstBufCtrl, void *pBufRet)
Description: 
   向一块commbuffer中归还一个内存单元
Input:
    pstBufCtrl: 待操作的commbuffer句柄
    pBufRet: 待归还的the address of memory
Output:
    void
Return: 
    成功: 0
    失败: 错误码
Others:        
*******************************************************************************/
U32 pl_commbuf_ret(BUF_CTRL_S *pstBufCtrl, void *pBufRet)
{
    U32 ulBufIndex;
#ifdef COMMBUF_DEBUG
    U16 usThreadId;
#endif
    U16 usRslt;
    
    if ((pstBufCtrl == NULL)
        || (pBufRet == NULL))
    {
        pl_mempool_log(ERR, "illegal input parameter pstBufCtrl=0x%08x, pBufRet=0x%08x", pstBufCtrl, pBufRet);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_RET_NULL);
        return COMM_FAIL;
    }

#ifdef COMMBUF_DEBUG
#ifndef MEMORY_IN_MEMPOOL
    if (NULL == pstBufCtrl->pMutex) //if no mutex, threadId check
    {
        usThreadId = PF_THREAD_SELF();
        if (pstBufCtrl->usThreadId != usThreadId)
        {
            pl_mempool_log(ERR, "Buf(%s) check thread id fail: creater=%d, caller=%d", 
                pstBufCtrl->pucBufName, pstBufCtrl->usThreadId, usThreadId);
            PS_CPlus(CM_PES, CMPES_ID_COMMBUF_RET_THREAD_FAIL);
            return COMM_FAIL;
        }
    }
#endif    
#endif

    if ((pBufRet < pstBufCtrl->pucBufStart)
        || (pBufRet >= pstBufCtrl->pucBufEnd))
    {
        if (TRUE == pstBufCtrl->ucWithAddress)
        {
            pl_mempool_log(ERR, "Buf(%s) to free error pBufRet=0x%08x, start=0x%08x, end=0x%08x", 
                pstBufCtrl->pucBufName, pBufRet, pstBufCtrl->pucBufStart, pstBufCtrl->pucBufEnd);
            PS_CPlus(CM_PES, CMPES_ID_COMMBUF_RET_ADDRESS_FAIL);
            return COMM_FAIL;        
        }
        
#ifdef COMMBUF_URGENT_MALLOC    
        pl_mempool_log(WARN, "Buf(%s) successful urgent free pBufRet=0x%08x", pstBufCtrl->pucBufName, pBufRet);
        pl_commbuf_free(pBufRet);
        pstBufCtrl->ulUrgentUsedNum--;
        return COMM_SUCC;
#else        //check free address range
        pl_mempool_log(ERR, "Buf(%s) to free error pBufRet=0x%08x, start=0x%08x, end=0x%08x", 
                pstBufCtrl->pucBufName, pBufRet, pstBufCtrl->pucBufStart, pstBufCtrl->pucBufEnd);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_RET_FAIL);
        return COMM_FAIL;
#endif        
    }

    COMMBUF_MUTEX_LOCK(pstBufCtrl->pMutex);
#ifdef COMMBUF_DEBUG
    ulBufIndex = ((U32)((U8*)pBufRet - pstBufCtrl->pucBufStart)) / pstBufCtrl->ulBufSize;
#else
    ulBufIndex = (U32)pBufRet;
#endif
    usRslt = pl_que_put(pstBufCtrl->pstQueCtrl, ulBufIndex);
    COMMBUF_MUTEX_UNLOCK(pstBufCtrl->pMutex);
    if (usRslt != COMM_SUCC)
    {
        pl_mempool_log(FATAL, "BUF(%s) pl_que_put fail", pstBufCtrl->pucBufName);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_RET_PUT_FAIL);
        return COMM_FAIL;
    }

#ifdef COMMBUF_DEBUG    
    pstBufCtrl->ullInCnt++;
    pl_mempool_log(TRC, "BUF(%s) successful, BufIndex=%d, BufStart=%08x, BufRet=%08x, InCnt=%lld, OutCnt=%lld",
                        pstBufCtrl->pucBufName, ulBufIndex, pstBufCtrl->pucBufStart, 
                        pBufRet, pstBufCtrl->ullInCnt, pstBufCtrl->ullOutCnt);
#endif

    return COMM_SUCC;
}

/*******************************************************************************
Function:
U32 pl_commbuf_avail_num(BUF_CTRL_S *pstBufCtrl)
Description: 
   返回一块commbuffer中当前可用的内存单元个数
Input:
    pstBufCtrl: 待查看的commbuffer句柄
Output:
    void
Return: 
    成功: 个数
    失败: INFINITY
Others:        
*******************************************************************************/
U32 pl_commbuf_avail_num(BUF_CTRL_S *pstBufCtrl)
{
    U32 ulAvailNum;
    
    if ((pstBufCtrl == NULL)
        || (pstBufCtrl->pstQueCtrl == NULL))
    {
        pl_mempool_log(ERR, "illegal input parameter pstBufCtrl=0x%08x", pstBufCtrl);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_AVAIL_NULL);
        return U32_INFINITY;        
    }

    ulAvailNum = pl_que_avail_num(pstBufCtrl->pstQueCtrl);

    return ulAvailNum;    
}

/*******************************************************************************
Function:
void pl_commbuf_get_total_available_membytes(U64 *pullAvailMemBytes, U64 *pullTotalMemBytes)
Description: 
    获取当前plcomm未被占用的字节数和总字节数，仅作参考
Input:
    void
Output:
    pullAvailMemBytes    返回要获取的空余字节数
    pullTotalMemBytes   返回要获取的总字节数
Return: 
    void
Others:        
*******************************************************************************/
void pl_commbuf_get_total_available_membytes(U64 *pullAvailMemBytes, U64 *pullTotalMemBytes)
{
    U32 ulCommbufIdx = 0;
    BUF_CTRL_S *pstBufCtrl = NULL;
    U64 ullFreeSize = 0;
    U64 ullTotalSize = 0;
    
 #ifdef COMMBUF_DEBUG     
    for (ulCommbufIdx = 0; ulCommbufIdx < g_pstCommBufferTotalNum; ulCommbufIdx++)
    {
        pstBufCtrl = g_pastCommBufferGlobal[ulCommbufIdx];
        ullFreeSize += (((U64)pstBufCtrl->ulBufNum - (pstBufCtrl->ullOutCnt - pstBufCtrl->ullInCnt)) * pstBufCtrl->ulBufSize);
        ullTotalSize += (U64)pstBufCtrl->ulBufNum * (U64)pstBufCtrl->ulBufSize;
    }

    *pullAvailMemBytes = ullFreeSize;
    *pullTotalMemBytes = ullTotalSize;
 #endif
    return;
}

/*******************************************************************************
Function:
void pl_commbuf_log(const BUF_CTRL_S *pstBufCtrl)
Description: 
   打印一块commbuffer的控制和统计信息
Input:
    pstBufCtrl: 待查看的commbuffer句柄
Output:
    void
Return: 
    void
Others:        
*******************************************************************************/
void pl_commbuf_log(const BUF_CTRL_S *pstBufCtrl)
{
    QUE_CTRL_S *pstQueCtrl;
    
    if ((pstBufCtrl == NULL)
        || (pstBufCtrl->pstQueCtrl == NULL))
    {
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_LOG_NULL);
        return;
    }
    pstQueCtrl = pstBufCtrl->pstQueCtrl;
    
#ifdef COMMBUF_DEBUG     
    pl_log(INF, "Buf(%s): size=%d, num=%d, Start=0x%08x, End=0x%08x, InCnt=%lld, OutCnt=%lld, UsedCnt=%lld, UrgNum=%d, MaxUsedNum=%d; "
                      "Que: TotalNum=%d, head=%d, tail=%d, InCnt=%lld, OutCnt=%lld", 
                    pstBufCtrl->pucBufName, pstBufCtrl->ulBufSize, pstBufCtrl->ulBufNum, 
                    pstBufCtrl->pucBufStart, pstBufCtrl->pucBufEnd,
                    pstBufCtrl->ullInCnt, pstBufCtrl->ullOutCnt, (pstBufCtrl->ullOutCnt - pstBufCtrl->ullInCnt),
                    pstBufCtrl->ulUrgentUsedNum, pstBufCtrl->ulMaxUsedNum,
                    pstQueCtrl->ulElemNum, pstQueCtrl->ulQueHead, pstQueCtrl->ulQueTail, 
                    pstQueCtrl->ullInCnt, pstQueCtrl->ullOutCnt);
#else
    pl_log(INF, "Buf(%s): size=%d, num=%d, Start=0x%08x, End=0x%08x, UrgNum=%d, MaxUsedNum=%d; Que: TotalNum=%d, head=%d, tail=%d", 
                     pstBufCtrl->pucBufName, pstBufCtrl->ulBufSize, pstBufCtrl->ulBufNum, 
            pstBufCtrl->pucBufStart, pstBufCtrl->pucBufEnd,
            pstBufCtrl->ulUrgentUsedNum, pstBufCtrl->ulMaxUsedNum, 
                     pstQueCtrl->ulElemNum, pstQueCtrl->ulQueHead, pstQueCtrl->ulQueTail);
#endif

    return;
}

/*******************************************************************************
Function:
void pl_commbuf_statistic(void)
Description: 
   打印全局所有commbuffer的控制和统计信息
Input:
    void
Output:
    void
Return: 
    void
Others:        
*******************************************************************************/
void pl_commbuf_statistic(void)
{
    U32 ulCommbufIdx = 0;
    BUF_CTRL_S *pstBufCtrl = NULL;
    U64 ullMallocCount = 0;
    U64 ullFreeCount = 0;

    for (ulCommbufIdx = 0; ulCommbufIdx < g_pstCommBufferTotalNum; ulCommbufIdx++)
    {
        pstBufCtrl = g_pastCommBufferGlobal[ulCommbufIdx];
        pl_commbuf_log(pstBufCtrl);
#ifdef COMMBUF_DEBUG           
        if (TRUE == pstBufCtrl->ucWithAddress)
        {
            ullMallocCount += pstBufCtrl->pstQueCtrl->ullOutCnt;
            ullFreeCount += (pstBufCtrl->pstQueCtrl->ullInCnt - pstBufCtrl->pstQueCtrl->ulElemNum);
        }
#endif
    }

    PS_CSet(CM_COM, CMCOM_ID_COMMBUF_MALLOC_CNT, ullMallocCount);
    PS_CSet(CM_COM, CMCOM_ID_COMMBUF_FREE_CNT, ullFreeCount);
    pl_log(WARN, "Commbuf Error Statistic: BufGetFail/UrgentMalloc=%d", g_ulBufGetFailCount);

    return;
}

/*******************************************************************************
Function:
void pl_commbuf_clear_statics(void)
Description: 
   清空全局所有commbuffer的控制和统计信息
Input:
    void
Output:
    void
Return: 
    void
Others:        
*******************************************************************************/
void pl_commbuf_clear_statics(void)
{
    U32 ulBufIdx = 0;
    BUF_CTRL_S *pstBufCtrl = NULL;

    for (ulBufIdx = 0; ulBufIdx<g_pstCommBufferTotalNum; ulBufIdx++)
    {
        pstBufCtrl = g_pastCommBufferGlobal[ulBufIdx];

        if (pstBufCtrl == NULL)
        {
            pl_mempool_log(ERR, "statics Buf NULL");
            continue;
        }

#ifdef COMMBUF_DEBUG    
        pstBufCtrl->ullInCnt = 0;
        pstBufCtrl->ullOutCnt = 0;
#endif
        pstBufCtrl->ulUrgentUsedNum = 0; 
        pstBufCtrl->ulMaxUsedNum = 0;
    }
        
    return;
}


/*******************************************************************************
Function:
void pl_commbuf_statistic(void)
Description: 
   打印全局所有commbuffer的控制和统计信息
Input:
    void
Output:
    void
Return: 
    void
Others:        
*******************************************************************************/
void pl_commbuf_clear()     //Force to clear all combuffer, this interface should be provoked carefully
{
    U32 ulBufIdx = 0;
    BUF_CTRL_S *pstBufCtrl = NULL;

    for (ulBufIdx = 0; ulBufIdx<g_pstCommBufferTotalNum; ulBufIdx++)
    {
        pstBufCtrl = g_pastCommBufferGlobal[ulBufIdx];

        pl_mempool_log(INF, "%s Buf(%s) successful", __FUNCTION__, pstBufCtrl->pucBufName);
        pl_que_destroy(pstBufCtrl->pstQueCtrl);
        if (FALSE == pstBufCtrl->ucWithAddress)
        {
            free(pstBufCtrl->pucBufStart);
        }
        free(pstBufCtrl);
    }
    
    g_pstCommBufferTotalNum = 0;
    
    return;
}

/*******************************************************************************
Function:
QUE_CTRL_S *pl_que_create(U32 ulElemNum, const CHAR *pucQueName)
Description: 
    创建一个队列
Input:
    ulElemNum: 队列长度
    pucQueName: 队列名称，用于调试，可不填
Output:
    void
Return: 
    成功: 队列句柄
    失败: NULL
Others:        
*******************************************************************************/
QUE_CTRL_S *pl_que_create(U32 ulElemNum, const CHAR *pucQueName)
{
    QUE_CTRL_S *pstQueCtrl;

    if (ulElemNum > MAX_QUE_ELEM_NUM)
    {
        pl_mempool_log(ERR, "illegal input parameter ElemNum=%d", ulElemNum);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_CREATE_NUM_ERROR);
        return NULL;             
    }

    pstQueCtrl = (QUE_CTRL_S *)malloc(sizeof(QUE_CTRL_S));
    if (pstQueCtrl == NULL)
    {
        pl_mempool_log(ERR, "malloc QUE_CTRL_S fail");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_CREATE_MALLOC_CTRL_NULL);
        return NULL;
    }

    pstQueCtrl->aulQueElem = (U32*)malloc((ulElemNum+1) * sizeof(U32));
    if (pstQueCtrl->aulQueElem == NULL)
    {
        pl_mempool_log(ERR, "malloc QueElem array fail");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_CREATE_MALLOC_ELEM_NULL);
        free(pstQueCtrl);
        return NULL;
    }

    PF_MUTEX_LOCK(&g_stQueMutex);
    g_pastQueGlobal[g_pstQueTotalNum++] = pstQueCtrl;      //record global info
    PF_MUTEX_UNLOCK(&g_stQueMutex);

    if (pucQueName == NULL)
    {
        pstQueCtrl->pucQueName = "";
    }
    else
    {
        pstQueCtrl->pucQueName = pucQueName;
    }

    pstQueCtrl->ulElemNum = ulElemNum;
    pstQueCtrl->ulQueHead = 0;
    pstQueCtrl->ulQueTail = 0;

#ifdef COMMQUE_DEBUG
    pstQueCtrl->ullInCnt = 0;
    pstQueCtrl->ullOutCnt = 0;
#endif

    pl_mempool_log(INF, "Que(%s) successful, ElemNum=%d, QueCtrl=%08x", 
        pstQueCtrl->pucQueName, ulElemNum, pstQueCtrl);

    return pstQueCtrl;
}

/*******************************************************************************
Function:
void pl_que_destroy(QUE_CTRL_S *pstQueCtrl)
Description: 
    销毁一个队列
Input:
    pstQueCtrl: 待操作的队列句柄
Output:
    void
Return: 
    void
Others:        
*******************************************************************************/
void pl_que_destroy(QUE_CTRL_S *pstQueCtrl)
{
    U32 ulQueIdx;

    if ((pstQueCtrl == NULL)
        || (pstQueCtrl->aulQueElem == NULL))
    {
        pl_mempool_log(ERR, "Que(%s) Fail: illegal parameter");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_DESTROY_NULL);
        return;
    }

    //NOT need to gurantee that que is empty

    PF_MUTEX_LOCK(&g_stQueMutex);
    for (ulQueIdx = 0; ulQueIdx < g_pstQueTotalNum; ulQueIdx++)
    {
        if (g_pastQueGlobal[ulQueIdx] == pstQueCtrl)
        {
            break;
        }
    }
    if (ulQueIdx == g_pstQueTotalNum)        //NOT Found indicated
    {
        pl_mempool_log(FATAL, "cannot find indicated Que(%s), but still destroy it", pstQueCtrl->pucQueName);
        PF_MUTEX_UNLOCK(&g_stQueMutex);
        free(pstQueCtrl->aulQueElem);
        free(pstQueCtrl);        
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_DESTROY_NOT_FOUND);
        return;
    }
    g_pastQueGlobal[ulQueIdx] = g_pastQueGlobal[--g_pstQueTotalNum];
    PF_MUTEX_UNLOCK(&g_stQueMutex);

    pl_mempool_log(INF, "destroy Que(%s) successful", pstQueCtrl->pucQueName);
    free(pstQueCtrl->aulQueElem);
    free(pstQueCtrl);

    return;
}

/*******************************************************************************
Function:
U32 pl_que_get(QUE_CTRL_S *pstQueCtrl)
Description: 
    从一个队列中取一个元素
Input:
    pstQueCtrl: 待操作的队列句柄
Output:
    void
Return: 
    成功: 队列元素值
    失败: U32_INFINITY
Others:    
  TAIL always points to the next slot of Queue Tail Element, HEAD always points to the previous slot of Queue Head Element
*******************************************************************************/

U64 pl_que_get(QUE_CTRL_S *pstQueCtrl)
{
    U32 ulRetElem;

    if (pstQueCtrl == NULL)
    {
        pl_mempool_log(ERR, "get illegal parameter");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_GET_NULL);
        return U32_INFINITY;
    }

    if (pstQueCtrl->ulQueHead == pstQueCtrl->ulQueTail)
    {
        pl_mempool_log(INF, "get Que(%s) fail for empty que", pstQueCtrl->pucQueName);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_GET_EMPTY);
        return U32_INFINITY;
    }

    ulRetElem = pstQueCtrl->aulQueElem[pstQueCtrl->ulQueHead];
    pstQueCtrl->ulQueHead = (pstQueCtrl->ulQueHead + 1) % (pstQueCtrl->ulElemNum + 1);

#ifdef COMMQUE_DEBUG
    pstQueCtrl->ullOutCnt++;
    pl_mempool_log(TRC, "get Que(%s) successful, QueHead=%d, QueTail=%d, ElemGet=%d, InCnt=%lld, OutCnt=%lld", 
                        pstQueCtrl->pucQueName, pstQueCtrl->ulQueHead, pstQueCtrl->ulQueTail, 
                        ulRetElem, pstQueCtrl->ullInCnt, pstQueCtrl->ullOutCnt);
#endif

    return ulRetElem;
}

/*******************************************************************************
Function:
U16 pl_que_put(QUE_CTRL_S *pstQueCtrl, U32 ulElemPut)
Description: 
    从一个队列中取一个元素
Input:
    pstQueCtrl: 待操作的队列句柄
    ulElemPut: 待放入队列的元素值
Output:
    void
Return: 
    成功: 0
    失败: 错误码
Others:    
  TAIL always points to the next slot of Queue Tail Element, HEAD always points to the previous slot of Queue Head Element
*******************************************************************************/
U16 pl_que_put(QUE_CTRL_S *pstQueCtrl, U32 ulElemPut)
{
    U32 ulNewQueTail;
    
    if (pstQueCtrl == NULL)
    {
        pl_mempool_log(ERR, "put illegal parameter");
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_PET_NULL);
        return COMM_FAIL;
    }

    ulNewQueTail = (pstQueCtrl->ulQueTail + 1) % (pstQueCtrl->ulElemNum + 1);
    if (ulNewQueTail == pstQueCtrl->ulQueHead)
    {
        pl_mempool_log(INF, "put Que(%s) fail for full que", pstQueCtrl->pucQueName);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_PET_FULL);
        return COMM_FAIL;
    }

    pstQueCtrl->aulQueElem[pstQueCtrl->ulQueTail] = ulElemPut;
    pstQueCtrl->ulQueTail = ulNewQueTail;

    
#ifdef COMMQUE_DEBUG
    pstQueCtrl->ullInCnt++;
    pl_mempool_log(TRC, "put Que(%s) successful, QueHead=%d, QueTail=%d, ElemPut=%d, InCnt=%lld, OutCnt=%lld", 
                        pstQueCtrl->pucQueName, pstQueCtrl->ulQueHead, pstQueCtrl->ulQueTail, 
                        ulElemPut, pstQueCtrl->ullInCnt, pstQueCtrl->ullOutCnt);
#endif

    return COMM_SUCC;
}

/*******************************************************************************
Function:
U32 pl_que_avail_num(QUE_CTRL_S *pstQueCtrl)
Description: 
    获取一个队列的已有元素个数
Input:
    pstQueCtrl: 待查看的队列句柄
Output:
    void
Return: 
    成功: 0
    失败: 错误码
Others:    
*******************************************************************************/
U32 pl_que_avail_num(QUE_CTRL_S *pstQueCtrl)
{
    U32 ulQueHead;
    U32 ulQueTail;
    U32 ulAvailNum;
    
    if (pstQueCtrl == NULL)
    {
        pl_mempool_log(ERR, "illegal input parameter pstQueCtrl=0x%08x", pstQueCtrl);
        PS_CPlus(CM_PES, CMPES_ID_COMMBUF_QUE_AVAIL_NULL);
        return U32_INFINITY;        
    }

    ulQueHead = pstQueCtrl->ulQueHead;
    ulQueTail = pstQueCtrl->ulQueTail;

    if (ulQueHead > ulQueTail)
    {
        ulQueTail += (pstQueCtrl->ulElemNum + 1);
    }
    ulAvailNum = ulQueTail - ulQueHead;

    return ulAvailNum;    
}

/*******************************************************************************
Function:
void pl_que_statistic(void)
Description: 
   打印全局所有队列的控制和统计信息    
Input:
    pstQueCtrl: 待查看的队列句柄
Output:
    void
Return: 
    void
Others:    
    与commbuffer打印重复，暂不放入performance中
*******************************************************************************/
void pl_que_statistic(void)     
{
    U32 ulQueIdx;
    
    for (ulQueIdx = 0; ulQueIdx < g_pstQueTotalNum; ulQueIdx++)
    {
        pl_que_log((const QUE_CTRL_S *)g_pastQueGlobal[ulQueIdx]);
    }

    return;
}

/*******************************************************************************
Function:
void pl_que_statistic(void)
Description: 
   打印一个队列的控制和统计信息
Input:
    pstQueCtrl: 待查看的队列句柄
Output:
    void
Return: 
    void
Others: 
*******************************************************************************/
void pl_que_log(const QUE_CTRL_S *pstQueCtrl)
{    
    if (pstQueCtrl == NULL)
        return;

#ifdef COMMQUE_DEBUG
    pl_log(INF, "Que(%s): TotalNum=%d, head=%d, tail=%d, InCnt=%lld, OutCnt=%lld",
            pstQueCtrl->pucQueName, pstQueCtrl->ulElemNum, pstQueCtrl->ulQueHead, 
            pstQueCtrl->ulQueTail, pstQueCtrl->ullInCnt, pstQueCtrl->ullOutCnt);
#else
    pl_log(INF, "Que(%s): TotalNum=%d, head=%d, tail=%d",
            pstQueCtrl->pucQueName, pstQueCtrl->ulElemNum, pstQueCtrl->ulQueHead, pstQueCtrl->ulQueTail);
#endif

    return;
}

