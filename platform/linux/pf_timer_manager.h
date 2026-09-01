/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    eCos_timer.h

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-11-15
  
  Description:  This file declares timer manager data structure and interfaces.

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
  
  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-11-15  1.0         First implementation of timer manager

*******************************************************************************/

#ifndef _PF_TIMER_MANAGER_H
#define _PF_TIMER_MANAGER_H

#include "double_list.h"
#include "pf_timer_common.h"

/* timer array parameters. Note: 0xFFFFFFFF is reserved for invalid timer id */
#define PF_TIMER_ARRAY_INDEX_BIT_LEN  14
#define PF_TIMER_ARRAY_INDEX_MASK     ((1 << PF_TIMER_ARRAY_INDEX_BIT_LEN) - 1)  /* 0x3FF */
#define PF_TIMER_ARRAY_MAX_SIZE       PF_TIMER_ARRAY_INDEX_MASK /* 0xFF (1023), max number of timers  */

/* macro function: get list node data and convert it to timer pointer */
#define LIST_NODE_TO_TIMER(pnode) (TIMER_UNIT_S*)((DOUBLE_LIST_S*)(pnode)->ullData)

typedef struct {
    DOUBLE_LIST_S stActiveTimerList;      /* active timer list for current hash unit */
} HASH_TABLE_UNIT_S;

typedef struct {
    DOUBLE_LIST_S stNode;       /* list node for: 1, free timer list; 2, allocated timer list for traversal. */
    DOUBLE_LIST_S stHashNode;   /* list node for: 1, hash table active timer list. */

    U32 u32Duration;     /* original timer tick, used by periodic timer. */
    U32 u32AbsExpTick;   /* absolute expiration timer tick. */

    U8  u8Running;       /* this timer is active or not, PF_TRUE or PF_FALSE. */
    U8  u8Cycle;         /* this timer is periodic or not. PF_TRUE or PF_FALSE */
    U16 u16Padding;      /* padding */

    U32 u32ModuleId;     /* target module ID for timer expiration message.*/
    
    TIMER_EXPIRE_MSG_S stTmExpMsg; /* timer expiration message. */
} TIMER_UNIT_S;

typedef struct {
    U32 u32TickInterval;
    U32 u32CurrentTick;

    PF_MUTEX_T stMutex;
    
    U32 u32TimerArraySize;
    TIMER_UNIT_S *pstTimerArray;

    U32 u32HashTblLastIdx;
    HASH_TABLE_UNIT_S *pstHashTable;

    DOUBLE_LIST_S stFreeTimerList;  /* list of all the free timers */
    DOUBLE_LIST_S stAllocTimerList; /* list of all the allocated timers, used for fast traverse running timers in DRX calibration.*/

    U32 u32TimerCount;          /* current running timer count */
    U32 u32TimerCountHistory;   /* history timer count, always increament */
    U32 u32TimerMsgFailCount;   /* timer expire message send fail count */
} TIMER_MANAGER_S;

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
*               PF_TIMER_PARAM_ERROR    one of the input parameter is wrong. check logs for details.
*               PF_TIMER_NO_RESOURCE    insufficient system heap memories.
*
*******************************************************************************/
U32 tm_init_timer_manager(
                        TIMER_MANAGER_S * const pstTMGR, 
                        U32 u32MaxTimerNum, 
                        U32 u32HashTableSize, 
                        U32 u32TickInterval);

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
void tm_uninit_timer_manager(TIMER_MANAGER_S * const pstTMGR);

/*******************************************************************************
*  Function:    tm_set_timer
*
*  Description: Setup a timer to timer manager. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager that the timer belongs to.
*               u8ModuleId      application module ID. used as timer expire message receiver. 
*                               the caller shall make sure it is a valid value.
*               u32Duration     timer duration in ticks.
*               u16TypeId       application timer type ID. not used by timer manager.
*               u32Para         application timer parameter. not used by timer manager.
*               u8Cycle         PF_TRUE for cycle-timer, and PF_FALSE for one-shot timer.
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
                U32* pu32TimerId);

/*******************************************************************************
*  Function:    tm_kill_timer
*
*  Description: kill a timer from timer manager. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager that the timer belongs to.
*               u32TimerId      timer ID. it must be the same one generated by tm_set_timer.
*               u8ModuleId      application module ID. it must be the same one 
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
                    U32 u32ModuleId);

/*******************************************************************************
*  Function:    tm_is_set_timer
*
*  Description: query a timer from timer manager. Return error code.
*
*  Input:       pstTMGR         pointer of timer manager that the timer belongs to.
*               u32TimerId      timer ID. it must be the same one generated by tm_set_timer.
*               u8ModuleId      application module ID. it must be the same one 
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
                    U32 u32ModuleId);

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
void tm_tick(TIMER_MANAGER_S *pstTM);

/*******************************************************************************
*  Function:    tm_get_timer_count
*
*  Description: get the number of running timers in timer manager.
*
*  Input:       pstTMGR         pointer of timer manager.
*
*
*  Output:      
*
*  Return:      the number of running timers
*
*******************************************************************************/
U32 tm_get_timer_count(TIMER_MANAGER_S * const pstTMGR);

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
                    U32 *pu32HashTableCount);

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
                    U32 u32ModuleId);
                    
#endif /* #ifndef _PF_TIMER_MANAGER_H */