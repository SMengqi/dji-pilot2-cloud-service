/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    eCos_timer.cpp

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-11-15
  
  Description:  This file implements timer manager interfaces and internal functions.

  Function List:
  tm_init_timer_manager: initialize timer manager.
  tm_uninit_timer_manager: uninitialize timer manager.
  tm_set_timer: setup a timer to timer manager.
  tm_kill_timer: kill a timer from timer manager.
  tm_is_set_timer: query a timer, check whether it is runing and exist.
  tm_tick: timer manager tick maintenance.
  tm_prepare_calibration: timer manager prepare timers before DRX sleep.
  tm_calibrate_timer: timer manager calibrate timers after DRX wakeup.
  tm_get_timer_count: get the number of running timers
  tm_alloc_timer(internal): allocate a timer from freeTimerList, and add to allocTimerList.
  tm_delete_timer(internal): delete a timer from allocTimerList, return to freeTimerList.
  tm_insert_timer(internal): insert a timer to a activeTimerList in hash table.
  tm_remove_timer(internal): remove a timer from a activeTimerList in hash table.
  tm_expire_timer(internal): do timeout operations for an expired timer.
  
  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-11-15  1.0         First implementation of timer manager 

*******************************************************************************/

#define THIS_MODULE MODULE_TIMER

#include <pl.h>

#include "pf_timer_manager.h"

//#define PF_TIMER_DEBUG

/*******************************************************************************
*  Function:    tm_alloc_timer
*
*  Description: Allocate a free timer from freeTimerList. If succeed, 
*               add it to allocTimerList. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager
*
*  Output:      ppstTimer       pointer of pointer of the allocated timer
*
*  Return:      PF_TIMER_SUCCESS        allocation succeeded.
*               PF_TIMER_NO_RESOURCE    no free timer unit, allocation failed.
*
*******************************************************************************/
inline U32 tm_alloc_timer(TIMER_MANAGER_S * const pstTMGR, TIMER_UNIT_S **ppstTimer)
{
    DOUBLE_LIST_S *pstNode;

    pstNode = DOUBLE_LIST_FIRST(&pstTMGR->stFreeTimerList);

    if (pstNode == &pstTMGR->stFreeTimerList)
    {
        pl_log(ERR, 
            "timer manager : alloc timer fail, no reource available. tick interval: %u, timer array size: %u.", 
            pstTMGR->u32TickInterval,
            pstTMGR->u32TimerArraySize); 
        *ppstTimer = NULL;
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_ALLOC_FAIL);
        return PF_TIMER_NO_RESOURCE;
    }
    
    pstTMGR->u32TimerCount++;
    *ppstTimer = LIST_NODE_TO_TIMER(pstNode);
    
    DOUBLE_LIST_REMOVE(pstNode); /* remove from freeTimerList */
    DOUBLE_LIST_INSERT_HEAD(&pstTMGR->stAllocTimerList, pstNode); /* insert to allocTimerList */
    
    return PF_TIMER_SUCCESS;
}

/*******************************************************************************
*  Function:    tm_delete_timer
*
*  Description: Delete a running timer unit from allocTimerList and 
*               return it to freeTimerList.
*                
*  Input:       pstTMGR         pointer of timer manager
*               pstTimer        pointer of the timer to be deleted
*
*  Output:
*
*  Return:      
*
*******************************************************************************/
inline void tm_delete_timer(TIMER_MANAGER_S * const pstTMGR, TIMER_UNIT_S *pstTimer)
{
    pstTimer->u8Running = FALSE;

    /* remove timer from allocTimerList */
    DOUBLE_LIST_REMOVE(&pstTimer->stNode);

    /* return timer to freeTimerList */
    DOUBLE_LIST_INSERT_HEAD(&pstTMGR->stFreeTimerList, &pstTimer->stNode);

    pstTMGR->u32TimerCount--;

    return;
}

/*******************************************************************************
*  Function:    tm_insert_timer
*
*  Description: insert a running timer to activeTimerList in hash table.
*
*  Input:       pstTMGR         pointer of timer manager
*               pstTimer        pointer of the timer to be inserted
*  Output:      
*
*  Return:      
*               
*
*******************************************************************************/
inline void tm_insert_timer(TIMER_MANAGER_S * const pstTMGR, TIMER_UNIT_S *pstTimer)
{
    U32 u32Index;
    HASH_TABLE_UNIT_S *pstHashTableUnit;

    u32Index= pstTimer->u32AbsExpTick & pstTMGR->u32HashTblLastIdx;
    pstHashTableUnit = pstTMGR->pstHashTable + u32Index;
    
    DOUBLE_LIST_INSERT_TAIL(&pstHashTableUnit->stActiveTimerList, 
                        &pstTimer->stHashNode);

    return;
}

/*******************************************************************************
*  Function:    tm_remove_timer
*
*  Description: remove a running timer from activeTimerList in hash table.
*
*  Input:       pstTimer        pointer of the timer to be removed
*               
*  Output:      
*
*  Return:      
*               
*
*******************************************************************************/
inline void tm_remove_timer(TIMER_UNIT_S * const pstTimer)
{
    /* remove timer from active timer list in current hash table unit */
    DOUBLE_LIST_REMOVE(&pstTimer->stHashNode);

    return;
}

/*******************************************************************************
*  Function:    tm_expire_timer
*
*  Description: a running timer is expired. Send a timeout message to application
*               and remove it from current activeTimerList in hashTable. If it is 
*               one-shot timer, delete it; if it is cycle timer, calculate next 
*               expiration tick and insert it to new activeTimerList in hash table.
*
*  Input:       pstTMGR         pointer of timer manager
*               pstTimer        pointer of the timer that expired
*  Output:      
*
*  Return:      
*               
*
*******************************************************************************/
inline void tm_expire_timer(TIMER_MANAGER_S * const pstTMGR, TIMER_UNIT_S *pstTimer)
{
    S32 ret;

    tm_remove_timer(pstTimer);

    FUNCTION_TRACE;
            
    //pf_copy_msg(
    ret = pf_copy_try_msg(
                MODULE_TIMER, 
                TIMER_EXPIRY_MSG, 
                pstTimer->u32ModuleId, 
                &pstTimer->stTmExpMsg, 
                sizeof(pstTimer->stTmExpMsg));

#ifdef PF_TIMER_DEBUG
    /* Note: log print severely affect response time */
    pl_log(TRC, 
            "timer manager(%u): expired u32TimerId: 0x%x, ret: %d", 
            pstTMGR->u32CurrentTick,
            pstTimer->stTmExpMsg.ulTimerId,
            ret);
#endif

    if (ret != PF_RET_SUCCESS)
    {
        /* try to resend timer expire message in next tick */
        pstTimer->u32AbsExpTick = pstTMGR->u32CurrentTick + 1;
        tm_insert_timer(pstTMGR, pstTimer);
        pstTMGR->u32TimerMsgFailCount++;
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_EXPIRE_COPY_FAIL);
    }
    else if (FALSE == pstTimer->u8Cycle)
    { 
        /* one-shot timer: just remove it */
        tm_delete_timer(pstTMGR, pstTimer);
    }
    else
    { 
        /* periodical(cycle) timer: reset ulAbsExpTick and insert to new hashTableUnit */
        pstTimer->u32AbsExpTick = pstTMGR->u32CurrentTick + pstTimer->u32Duration;
        tm_insert_timer(pstTMGR, pstTimer);
    }
    
    return;
}

/*******************************************************************************
*  Function:    tm_init_timer_manager
*
*  Description: Initialize a timer manager with given parameters. Return error code.
*
*  Input:       pstTMGR             pointer of timer manager to be initialized
*               u32MaxTimerNum      maximum timer number that this timer manager 
*                                   supports. value must <= PF_TIMER_ARRAY_MAX_SIZE.
*               u32HashTableSize    size of the hash table, must be 2^n.
*               u32TickInterval     millisec interval of 1 tick. 
*                                   value shall be PF_TIMER_TICK_INTERVAL_VALUE_E
*                                   
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        initialization succeeded.
*               PF_TIMER_PARAM_ERROR    one of the input parameter is wrong. 
*                                       check logs for details.
*               PF_TIMER_NO_RESOURCE    insufficient system heap memories.
*
*******************************************************************************/
U32 tm_init_timer_manager(
                        TIMER_MANAGER_S * const pstTMGR, 
                        U32 u32MaxTimerNum, 
                        U32 u32HashTableSize, 
                        U32 u32TickInterval)
{
    U32 u32LoopIndex;
    TIMER_UNIT_S *pstTimer;

    /* parameter check*/
    if (NULL == pstTMGR)
    {
        pl_log(ERR, 
            "timer manager: initialization parameter error, pstTMGR is NULL.",
            u32MaxTimerNum);
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_INIT_NULL);
        return PF_TIMER_PARAM_ERROR;
    }
    
    if (u32MaxTimerNum > PF_TIMER_ARRAY_MAX_SIZE)
    {
        pl_log(ERR, 
            "timer manager: initialization parameter error, u32MaxTimerNum: %u",
            u32MaxTimerNum);
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_INIT_MAXSIZE_FAIL);
        return PF_TIMER_PARAM_ERROR;
    }

    if ((0 == u32HashTableSize) 
        || ((u32HashTableSize & (u32HashTableSize - 1)) != 0)) /* ulHashTableSize must be 2^n */
    {
        pl_log(ERR, 
            "timer manager: initialization parameter error, u32HashTableSize: %u",
            u32HashTableSize);
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_INIT_TABLE_FAIL);
        return PF_TIMER_PARAM_ERROR;
    }

    if ((PF_TIMER_TICK_INTERVAL_1MS != u32TickInterval)
        && (PF_TIMER_TICK_INTERVAL_10MS != u32TickInterval))
    {
        pl_log(ERR, 
            "timer manager: initialization parameter error, u32TickInterval: %u",
            u32TickInterval);
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_INIT_INTERVAL_FAIL);
        return PF_TIMER_PARAM_ERROR;
    }

    /* now start timer manager initializaion */    
    pstTMGR->u32TickInterval = u32TickInterval;
    pstTMGR->u32CurrentTick = 0;
    pstTMGR->u32TimerCount = 0;
    pstTMGR->u32TimerCountHistory = 0;
    pstTMGR->u32TimerMsgFailCount = 0;

    PF_MUTEX_INIT(&pstTMGR->stMutex);

    DOUBLE_LIST_INIT(&pstTMGR->stFreeTimerList);
    DOUBLE_LIST_INIT(&pstTMGR->stAllocTimerList);

    /* initialize timer array */
    pstTMGR->u32TimerArraySize = u32MaxTimerNum;
    pstTMGR->pstTimerArray = (TIMER_UNIT_S *)pf_malloc(sizeof(TIMER_UNIT_S) * u32MaxTimerNum);

    if (NULL == pstTMGR->pstTimerArray)
    {
        pl_log(ERR, 
            "timer manager: failed to allocate memory for timer array, size: %u.",
            sizeof(TIMER_UNIT_S) * u32MaxTimerNum);
        
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_INIT_TIMERARRAY_NULL);
        return PF_TIMER_NO_RESOURCE;
    }
    
    pf_memset(pstTMGR->pstTimerArray, 0, sizeof(TIMER_UNIT_S) * u32MaxTimerNum);

    /* initialize each timer and setup freeTimerList */
    for (u32LoopIndex = 0; u32LoopIndex < u32MaxTimerNum; u32LoopIndex++)
    {
        pstTimer = pstTMGR->pstTimerArray + u32LoopIndex;
        pstTimer->stNode.ullData = (U64)pstTimer;
        pstTimer->stHashNode.ullData = (U64)pstTimer;
        pstTimer->stTmExpMsg.ulTimerId = u32LoopIndex & PF_TIMER_ARRAY_INDEX_MASK;
        DOUBLE_LIST_INSERT_HEAD(&pstTMGR->stFreeTimerList, &pstTimer->stNode);
    }

    /* initialize hash table */
    pstTMGR->u32HashTblLastIdx = u32HashTableSize - 1; /* shall be (2^n - 1) */
    pstTMGR->pstHashTable = (HASH_TABLE_UNIT_S *)pf_malloc(sizeof(HASH_TABLE_UNIT_S) * u32HashTableSize);
    if (NULL == pstTMGR->pstHashTable)
    {
        pl_log(ERR, 
            "timer manager: failed to allocate memory for hash table, size: %u.",
            sizeof(HASH_TABLE_UNIT_S) * u32HashTableSize);

        pf_free(pstTMGR->pstTimerArray);
        
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_INIT_HASHTABLE_NULL);
        return PF_TIMER_NO_RESOURCE;
    }
    pf_memset(pstTMGR->pstHashTable, 0, sizeof(HASH_TABLE_UNIT_S) * u32HashTableSize);
    
    for (u32LoopIndex = 0; u32LoopIndex < u32HashTableSize; u32LoopIndex++)
    {
        DOUBLE_LIST_INIT(&(pstTMGR->pstHashTable[u32LoopIndex].stActiveTimerList));
    }
    
    pl_log(INF, 
            "timer manager: initialize successfully. u32MaxTimerNum: %u, u32HashTableSize: %u, u32TickInterval: %u.",
            u32MaxTimerNum, 
            u32HashTableSize, 
            u32TickInterval);

    return PF_TIMER_SUCCESS;
}

/*******************************************************************************
*  Function:    tm_uninit_timer_manager
*
*  Description: Uninitialize a timer manager and cleanup resources. It is called
*               when this timer manager will not be used any more.
*
*  Input:       pstTMGR             pointer of timer manager to be uninitialized
*                                   
*
*  Output:      
*
*  Return:      
*               
*******************************************************************************/
void tm_uninit_timer_manager(TIMER_MANAGER_S * const pstTMGR)
{
    PF_MUTEX_DESTROY(&pstTMGR->stMutex);
 
    pf_free(pstTMGR->pstTimerArray);
    pf_free(pstTMGR->pstHashTable);

    return;
}

/*******************************************************************************
*  Function:    tm_set_timer
*
*  Description: Setup a timer to timer manager. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager that the timer belongs to.
*               u32ModuleId      application module ID. used as timer expire message receiver. 
*                               the caller shall make sure it is a valid value.
*               u32Duration     timer duration in ticks.
*               u32TypeId       application timer type ID. not used by timer manager.
*               u32Para         application timer parameter. not used by timer manager.
*               u8Cycle         TRUE for cycle-timer, and FALSE for one-shot timer.
*
*  Output:      pu32TimerId     generated timer ID, used for index a timer.
*
*  Return:      PF_TIMER_SUCCESS        timer setup succeeded.
*               PF_TIMER_NO_RESOURCE    no free timer resource. timer setup failed.
*
*******************************************************************************/
U32 tm_set_timer(
                TIMER_MANAGER_S * const pstTMGR,
                U32 u32ModuleId, 
                U32 u32Duration, 
                U32 u32TypeId, 
                U64 u64Para, 
                U8 u8Cycle, 
                U32* pu32TimerId)
{
    U32 u32Rslt;
    U32 u32CurrentTick;
    TIMER_UNIT_S *pstTimer;

    PF_MUTEX_LOCK(&pstTMGR->stMutex);
    
    u32Rslt = tm_alloc_timer(pstTMGR, &pstTimer);
    
    if (PF_TIMER_SUCCESS == u32Rslt)
    {
        u32CurrentTick = pstTMGR->u32CurrentTick;
        
        pstTimer->u32AbsExpTick = u32CurrentTick + u32Duration;
        pstTimer->u32Duration = u32Duration;

        pstTimer->u8Running = TRUE;
        pstTimer->u8Cycle = u8Cycle;
        pstTimer->u32ModuleId = u32ModuleId;

        pstTMGR->u32TimerCountHistory++;
        pstTimer->stTmExpMsg.ulTimerId = 
                     (pstTimer->stTmExpMsg.ulTimerId & PF_TIMER_ARRAY_INDEX_MASK) 
                     | (pstTMGR->u32TimerCountHistory << PF_TIMER_ARRAY_INDEX_BIT_LEN);
        *pu32TimerId = pstTimer->stTmExpMsg.ulTimerId;
        
        pstTimer->stTmExpMsg.ullPara = u64Para;
        pstTimer->stTmExpMsg.ulTypeId = u32TypeId;

        tm_insert_timer(pstTMGR, pstTimer);
    }
    else
    {
        u32Rslt = PF_TIMER_NO_RESOURCE;
    }/* end of if (u16Ret == PF_TIMER_SUCCESS) */
    
    PF_MUTEX_UNLOCK(&pstTMGR->stMutex);
    
#ifdef PF_TIMER_DEBUG
    /* Note: log print severely affect response time */
    pl_log(TRC, 
            "timer manager(%u): SetTimer: u32ModuleId %u, u32Duration %u, u32TypeId %u, u32Para 0x%llx, u8Cycle %u, pu32TimerId 0x%x, u32Rslt: %u.", 
            u32CurrentTick,            
            u32ModuleId,
            u32Duration,
            u32TypeId,
            u64Para,
            u8Cycle,
            *pu32TimerId,
            u32Rslt); 
#endif

    return u32Rslt;
}

/*******************************************************************************
*  Function:    tm_kill_timer
*
*  Description: kill a timer from timer manager. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager that the timer belongs to.
*               u32TimerId      timer ID. it must be the same one generated by tm_set_timer.
*               u32ModuleId      application module ID. it must be the same one 
*                               when setup this timer.
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        timer delete succeeded.
*               PF_TIMER_PARAM_ERROR    timer ID is invalid. probably random value.
*               PF_TIMER_NOT_PERMIT     timer ID is valid, but module ID is wrong.
*               PF_TIMER_NOT_EXIST      timer ID may be valid, but it is not running.
*
*******************************************************************************/
U32 tm_kill_timer(  TIMER_MANAGER_S * const pstTMGR,
                    U32 u32TimerId, 
                    U32 u32ModuleId)
{
    U32 u32Index;
    U32 u32Rslt;
    TIMER_UNIT_S *pstTimer;

    u32Index = u32TimerId & PF_TIMER_ARRAY_INDEX_MASK;
    
    if (u32Index >= pstTMGR->u32TimerArraySize)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_KILL_INDEX_FAIL);
        return PF_TIMER_PARAM_ERROR;
    }

    PF_MUTEX_LOCK(&pstTMGR->stMutex);
    
    pstTimer = pstTMGR->pstTimerArray + u32Index;
    
    if ((pstTimer->stTmExpMsg.ulTimerId != u32TimerId) 
        || (pstTimer->u8Running != TRUE))
    {
        u32Rslt = PF_TIMER_NOT_EXIST;
    }
    else if (pstTimer->u32ModuleId != u32ModuleId)
    {
        u32Rslt = PF_TIMER_NOT_PERMIT;
    }
    else
    {
        tm_remove_timer(pstTimer);
        tm_delete_timer(pstTMGR, pstTimer);
        u32Rslt = PF_TIMER_SUCCESS;
    }
    
    PF_MUTEX_UNLOCK(&pstTMGR->stMutex);

#ifdef PF_TIMER_DEBUG
    /* Note: log print severely affect response time */
    pl_log(TRC, 
            "timer manager(%u): KillTimer: u32TimerId: 0x%x, u32ModuleId: %u, u32Rslt: %u", 
            pstTMGR->u32CurrentTick, u32TimerId, u32ModuleId, u32Rslt);
#endif

    return u32Rslt;
}                

/*******************************************************************************
*  Function:    tm_is_set_timer
*
*  Description: query a timer from timer manager. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager that the timer belongs to.
*               u32TimerId      timer ID. it must be the same one generated by tm_set_timer.
*               u32ModuleId      application module ID. it must be the same one 
*                               when setup this timer.
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        query succeeded. timer is exist and running.
*               PF_TIMER_PARAM_ERROR    timer ID is invalid. probably random value.
*               PF_TIMER_NOT_PERMIT     timer ID is valid, but module ID is wrong.
*               PF_TIMER_NOT_EXIST      timer ID may be valid, but it is not running.
*
*******************************************************************************/
U32 tm_is_set_timer(TIMER_MANAGER_S * const pstTMGR,
                    U32 u32TimerId, 
                    U32 u32ModuleId)
{
    U32 u32Index;
    U32 u32Rslt;
    TIMER_UNIT_S *pstTimer;

    u32Index = u32TimerId & PF_TIMER_ARRAY_INDEX_MASK;
    
    if (u32Index >= pstTMGR->u32TimerArraySize)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TMSET_INDEX_FAIL);
        return PF_TIMER_PARAM_ERROR;
    }

    PF_MUTEX_LOCK(&pstTMGR->stMutex);
    
    pstTimer = pstTMGR->pstTimerArray + u32Index;
    
    if ((pstTimer->stTmExpMsg.ulTimerId != u32TimerId) 
        || (pstTimer->u8Running != TRUE))
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TMSET_RUNNING_FAIL);
        u32Rslt = PF_TIMER_NOT_EXIST;
    }
    else if (pstTimer->u32ModuleId != u32ModuleId)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TMSET_MODULEID_FAIL);
        u32Rslt = PF_TIMER_NOT_PERMIT;
    }
    else
    {
        u32Rslt = PF_TIMER_SUCCESS;
    }

    PF_MUTEX_UNLOCK(&pstTMGR->stMutex);

#ifdef PF_TIMER_DEBUG
    /* Note: log print severely affect response time */
    pl_log(TRC, 
            "timer manager: IsSetTimer: u32TimerId: 0x%x, u32ModuleId: %u, u32Rslt: %u", 
            u32TimerId, u32ModuleId, u32Rslt);
#endif

    return u32Rslt;
}                

/*******************************************************************************
*  Function:    tm_tick
*
*  Description: perform tick operation to timer manager.
*
*  Input:       pstTMGR         pointer of timer manager.
*
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void tm_tick(TIMER_MANAGER_S * const pstTMGR)
{
    U32 u32CurTick;
    U32 u32HashTblIdx;
    DOUBLE_LIST_S *pstHashNode;
    DOUBLE_LIST_S *pstListHead;
    TIMER_UNIT_S *pstTimer;
    
    PF_MUTEX_LOCK(&pstTMGR->stMutex);

    u32CurTick = ++pstTMGR->u32CurrentTick;
    
    u32HashTblIdx = u32CurTick & pstTMGR->u32HashTblLastIdx;
    pstListHead = &(pstTMGR->pstHashTable[u32HashTblIdx].stActiveTimerList);
    pstHashNode = DOUBLE_LIST_FIRST(pstListHead);

    while (pstHashNode != pstListHead)
    {
        pstTimer = LIST_NODE_TO_TIMER(pstHashNode);
        
        pstHashNode = DOUBLE_LIST_NEXT(pstHashNode);

        if (pstTimer->u32AbsExpTick == u32CurTick)
        {
            tm_expire_timer(pstTMGR, pstTimer);
        }
    }
    
    PF_MUTEX_UNLOCK(&pstTMGR->stMutex);

    return;
}

/*******************************************************************************
*  Function:    tm_get_timer_count
*
*  Description: get the number of running timers in timer manager.
*
*  Input:       pstTMGR         pointer of timer manager.
*
*  Output:      
*
*  Return:      the number of running timers
*
*******************************************************************************/
U32 tm_get_timer_count(TIMER_MANAGER_S * const pstTMGR)
{
    return pstTMGR->u32TimerCount;
}

/*******************************************************************************
*  Function:    tm_get_status
*
*  Description: get internal status of timer manager. used for debug and MT.
*
*  Input:       pstTMGR             pointer of timer manager.
*
*  Output:      pu32FreeListCount   pointer of free list count
*               pu32AllocListCount  pointer of allocated list count
*               pu32HashTableCount  pointer of running timer count in hash table
*
*  Return:      PF_TIMER_SUCCESS    internal status OK.
*               PF_TIMER_FAIL       detect internal status error.
*
*******************************************************************************/
U32 tm_get_status(TIMER_MANAGER_S * const pstTMGR, 
                    U32 *pu32FreeListCount,
                    U32 *pu32AllocListCount,
                    U32 *pu32HashTableCount)
{
    U32 u32LoopIndex;
    
    U32 u32FreeCount;
    U32 u32AllocCount;
    U32 u32HashCount;
    
    DOUBLE_LIST_S *pstNode;
    DOUBLE_LIST_S *pstHashNode;
    DOUBLE_LIST_S *pstListHead;
    
    U32 u32Rslt = PF_TIMER_SUCCESS;

    PF_MUTEX_LOCK(&pstTMGR->stMutex);

    /* 1, get free timer list count */
    u32FreeCount = 0;
    pstNode = DOUBLE_LIST_FIRST(&pstTMGR->stFreeTimerList);

    while (pstNode != &pstTMGR->stFreeTimerList)
    {
        pstNode = DOUBLE_LIST_NEXT(pstNode);
        u32FreeCount++;
        if (u32FreeCount > pstTMGR->u32TimerArraySize)
        {
            pl_log(ERR, 
                    "TimerManager: get status: freeTimerList status error!");
            PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_STATUS_FREELIST_COUNT_FAIL);
            u32Rslt = PF_TIMER_FAIL;
        }
    }
        
    /* 2, get allocated timer list count */
    u32AllocCount = 0;
    pstNode = DOUBLE_LIST_FIRST(&pstTMGR->stAllocTimerList);

    while (pstNode != &pstTMGR->stAllocTimerList)
    {
        pstNode = DOUBLE_LIST_NEXT(pstNode);
        u32AllocCount++;
        if (u32AllocCount > pstTMGR->u32TimerArraySize)
        {
            pl_log(ERR, 
                    "TimerManager: get status: freeTimerList status error!");
            PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_STATUS_ALLOCLIST_COUNT_FAIL);
            u32Rslt = PF_TIMER_FAIL;
        }
    }

    /* 3, get running timer count in hash table */
    u32HashCount = 0;
    
    for (u32LoopIndex = 0; u32LoopIndex <= pstTMGR->u32HashTblLastIdx; u32LoopIndex++)
    {
        pstListHead = &(pstTMGR->pstHashTable[u32LoopIndex].stActiveTimerList);
        pstHashNode = DOUBLE_LIST_FIRST(pstListHead);

        while (pstHashNode != pstListHead)
        {
            pstHashNode = DOUBLE_LIST_NEXT(pstHashNode);
            u32HashCount++;

            if (u32HashCount > pstTMGR->u32TimerArraySize)
            {
                pl_log(ERR, 
                        "TimerManager: get status: pstHashTable status error!");
                PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_STATUS_HASH_COUNT_FAIL);
                u32Rslt = PF_TIMER_FAIL;
                break;
            }
        }

        if (u32HashCount > pstTMGR->u32TimerArraySize)
        {
            break;
        }
    }

    PF_MUTEX_UNLOCK(&pstTMGR->stMutex);
    
    /* final check */
    if ((u32FreeCount + u32AllocCount) != pstTMGR->u32TimerArraySize)
    {
        pl_log(ERR, 
                "TimerManager: get status: total timer number wrong! u32FreeCount: %u, u32AllocCount: %u",
                u32FreeCount, u32AllocCount);
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_STATUS_FINAL_CHECK_FAIL);
        u32Rslt = PF_TIMER_FAIL;
    }

    if (u32AllocCount != u32HashCount)
    {
        pl_log(ERR, 
                "TimerManager: get status: running timer number wrong! u32HashCount: %u, u32AllocCount: %u",
                u32HashCount, u32AllocCount);
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TM_STATUS_ALLO_HASH_COUNT_FAIL);
        u32Rslt = PF_TIMER_FAIL;
    }
    
    *pu32FreeListCount = u32FreeCount;
    *pu32HashTableCount = u32HashCount;
    *pu32AllocListCount = u32AllocCount;

    return u32Rslt;
}


/*******************************************************************************
*  Function:    tm_restart_timer
*
*  Description: restart a timer from timer manager. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager that the timer belongs to.
*               u32TimerId      timer ID. it must be the same one generated by tm_set_timer.
*               u32ModuleId      application module ID. it must be the same one 
*                               when setup this timer.
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        timer restart succeeded.
*               PF_TIMER_PARAM_ERROR    timer ID is invalid. probably random value.
*               PF_TIMER_NOT_PERMIT     timer ID is valid, but module ID is wrong.
*               PF_TIMER_NOT_EXIST      timer ID may be valid, but it is not running.
*
*******************************************************************************/
U32 tm_restart_timer(  TIMER_MANAGER_S * const pstTMGR,
                    U32 u32TimerId, 
                    U32 u32ModuleId)
{
    U32 u32Index;
    U32 u32Rslt;
    TIMER_UNIT_S *pstTimer;

    u32Index = u32TimerId & PF_TIMER_ARRAY_INDEX_MASK;
    
    if (u32Index >= pstTMGR->u32TimerArraySize)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TMSET_INDEX_FAIL);
        return PF_TIMER_PARAM_ERROR;
    }

    PF_MUTEX_LOCK(&pstTMGR->stMutex);
    
    pstTimer = pstTMGR->pstTimerArray + u32Index;
    
    if ((pstTimer->stTmExpMsg.ulTimerId != u32TimerId) 
        || (pstTimer->u8Running != TRUE))
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TMSET_RUNNING_FAIL);
        u32Rslt = PF_TIMER_NOT_EXIST;
    }
    else if (pstTimer->u32ModuleId != u32ModuleId)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_TMSET_MODULEID_FAIL);
        u32Rslt = PF_TIMER_NOT_PERMIT;
    }
    else
    {
        tm_remove_timer(pstTimer);
        pstTimer->u32AbsExpTick =pstTMGR->u32CurrentTick + pstTimer->u32Duration; //update expiration timer tick
        tm_insert_timer(pstTMGR, pstTimer);
        u32Rslt = PF_TIMER_SUCCESS;
    }
    
    PF_MUTEX_UNLOCK(&pstTMGR->stMutex);

#ifdef PF_TIMER_DEBUG
    /* Note: log print severely affect response time */
    pl_log(TRC, 
            "timer manager(%u): RestartTimer: u32TimerId: 0x%x, u32ModuleId: %u, u32Rslt: %u", 
            pstTMGR->u32CurrentTick, u32TimerId, u32ModuleId, u32Rslt);
#endif

    return u32Rslt;
}
