/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    eCos_timer_trace.h

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-11-22
  
  Description:  This file declares and implements timer trace functions.

  Function List:
  PF_TIMER_TRACE_DECLARE(macro): declare timer trace global variables.
  PF_TIMER_TRACE_INIT(macro): initialize timer trace global variables.
  PF_SET_TIMER_TRACE(macro): trace set timer failure.
  PF_KILL_TIMER_TRACE(macro): trace kill timer failure.

  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-11-22  1.0         First implementation of timer trace functions.

*******************************************************************************/

#ifndef _PF_TIMER_TRACE_H
#define _PF_TIMER_TRACE_H

#include "pf_timer_common.h"

#ifdef PF_TIMER_DEBUG 

#define PF_TIMER_TRACE_RECORD_NUM 1024

typedef enum {
    PF_TIMER_TRACE_SET_FAIL = 0,    /* set timer failure */
    PF_TIMER_TRACE_KILL_FAIL,       /* kill timer failure */
} TIMER_TRACE_TYPE_E;

typedef struct {
    U8 u8TraceType;     /* TIMER_TRACE_TYPE_E */
    U8 u8ModuleId;      /* valid for all trace type */
    U16 u16TypeId;      /* only valid for PF_TIMER_TRACE_SET_FAIL */
    U32 u32Duration;    /* only valid for PF_TIMER_TRACE_SET_FAIL */
    U32 u32TimerId;     /* only valid for PF_TIMER_TRACE_KILL_FAIL */
    U16 u16Result;      /* valid for all trace type */
    U16 u16Reserved; 
} TIMER_TRACE_S;

extern cyg_mutex_t g_stTimerTraceMutex;
extern U32 g_u32TimerTraceArrayIndex;
extern TIMER_TRACE_S g_astTimerTraceArray[];

/* declare timer trace global variables. must call it once. */
#define PF_TIMER_TRACE_DECLARE \
    cyg_mutex_t g_stTimerTraceMutex; \
    U32 g_u32TimerTraceArrayIndex = 0; \
    TIMER_TRACE_S g_astTimerTraceArray[PF_TIMER_TRACE_RECORD_NUM];

/* initialize timer trace global variables. must call it once. */
#define PF_TIMER_TRACE_INIT \
{ \
    g_u32TimerTraceArrayIndex = 0; \
    cyg_mutex_init(&g_stTimerTraceMutex); \
    pf_memset(&g_astTimerTraceArray, 0, sizeof(g_astTimerTraceArray));\
}

/*******************************************************************************
*  Function:    PF_SET_TIMER_TRACE
*
*  Description:  trace set timer failure.
*
*  Input:       rslt        pointer of list head
*               moduleId    module ID of timer owner
*               typeId      timer type ID
*               duration    timer duration
*
*  Output:      
*
*  Return:      No Return Value
*
*******************************************************************************/
#define PF_SET_TIMER_TRACE(rslt, moduleId, typeId, duration) \
    if (rslt != PF_TIMER_SUCCESS) \
    { \
        U32 u32Index; \
        cyg_mutex_lock(&g_stTimerTraceMutex); \
        u32Index = g_u32TimerTraceArrayIndex; \
        g_u32TimerTraceArrayIndex = (g_u32TimerTraceArrayIndex + 1) \
                                    % PF_TIMER_TRACE_RECORD_NUM; \
        cyg_mutex_unlock(&g_stTimerTraceMutex); \
        g_astTimerTraceArray[u32Index].u8TraceType = PF_TIMER_TRACE_SET_FAIL; \
        g_astTimerTraceArray[u32Index].u8ModuleId = (moduleId); \
        g_astTimerTraceArray[u32Index].u16TypeId = (typeId); \
        g_astTimerTraceArray[u32Index].u32Duration = (duration); \
        g_astTimerTraceArray[u32Index].u16Result = (rslt); \
    } 

/*******************************************************************************
*  Function:    PF_KILL_TIMER_TRACE
*
*  Description:  trace kill timer failure.
*
*  Input:       rslt        pointer of list head
*               moduleId    module ID of timer owner
*               timerId     timer ID
*
*  Output:      
*
*  Return:      No Return Value
*
*******************************************************************************/
#define PF_KILL_TIMER_TRACE(rslt, moduleId, timerId) \
    if (rslt != PF_TIMER_SUCCESS) \
    { \
        U32 u32Index; \
        cyg_mutex_lock(&g_stTimerTraceMutex); \
        u32Index = g_u32TimerTraceArrayIndex; \
        g_u32TimerTraceArrayIndex = (g_u32TimerTraceArrayIndex + 1) \
                                    % PF_TIMER_TRACE_RECORD_NUM; \
        cyg_mutex_unlock(&g_stTimerTraceMutex); \
        g_astTimerTraceArray[u32Index].u8TraceType = PF_TIMER_TRACE_KILL_FAIL; \
        g_astTimerTraceArray[u32Index].u8ModuleId = (moduleId); \
        g_astTimerTraceArray[u32Index].u32TimerId = (timerId); \
        g_astTimerTraceArray[u32Index].u16Result = (rslt); \
    }

/*******************************************************************************
*  Function:    PF_RESTART_TIMER_TRACE
*
*  Description:  trace restart timer failure.
*
*  Input:       rslt        pointer of list head
*               moduleId    module ID of timer owner
*               timerId     timer ID
*
*  Output:      
*
*  Return:      No Return Value
*
*******************************************************************************/
#define PF_RESTART_TIMER_TRACE(rslt, moduleId, timerId) \
    if (rslt != PF_TIMER_SUCCESS) \
    { \
        U32 u32Index; \
        cyg_mutex_lock(&g_stTimerTraceMutex); \
        u32Index = g_u32TimerTraceArrayIndex; \
        g_u32TimerTraceArrayIndex = (g_u32TimerTraceArrayIndex + 1) \
                                    % PF_TIMER_TRACE_RECORD_NUM; \
        cyg_mutex_unlock(&g_stTimerTraceMutex); \
        g_astTimerTraceArray[u32Index].u8TraceType = PF_TIMER_TRACE_KILL_FAIL; \
        g_astTimerTraceArray[u32Index].u8ModuleId = (moduleId); \
        g_astTimerTraceArray[u32Index].u32TimerId = (timerId); \
        g_astTimerTraceArray[u32Index].u16Result = (rslt); \
    }
	
#else /* else of #ifdef PF_TIMER_DEBUG */

#define PF_TIMER_TRACE_DECLARE
#define PF_TIMER_TRACE_INIT
#define PF_SET_TIMER_TRACE 
#define PF_KILL_TIMER_TRACE
#define PF_RESTART_TIMER_TRACE

#endif /* end of #ifdef PF_TIMER_DEBUG */

#endif /* #ifndef _PF_TIMER_TRACE_H */
