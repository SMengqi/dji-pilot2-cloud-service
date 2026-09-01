/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    pf_timer_service.h

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-11-15
  
  Description:  This file declares timer service interfaces.

  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-11-15  1.0         First implementation of timer service

*******************************************************************************/
#ifndef _PF_TIMER_SERVICE_H
#define _PF_TIMER_SERVICE_H

#include "pf_timer_common.h"



#if DRC_PROCESS
#define THREAD_MONITOR_TIME_DEFAULT       DRC_THREAD_MONITOR_TIME_DEFAULT 
#define PF_MAX_TIMER_NUM_1MS              DRC_MAX_TIMER_NUM_1MS     /*1ms timer service parameters, must <= 16383, check PF_TIMER_ARRAY_MAX_SIZE */
#else
#define THREAD_MONITOR_TIME_DEFAULT       DRSU_THREAD_MONITOR_TIME_DEFAULT
#define PF_MAX_TIMER_NUM_1MS              DRSU_MAX_TIMER_NUM_1MS    /*1ms timer service parameters, must <= 16383, check PF_TIMER_ARRAY_MAX_SIZE */
#endif
/* 1ms timer service parameters */
#define PF_HASH_TABLE_SIZE_1MS     8192 /* must be 2^n */

#if 0
/* 10ms timer service parameters */
#define PF_MAX_TIMER_NUM_10MS      128
#define PF_HASH_TABLE_SIZE_10MS    1024

#define PF_TIMER_MIN_DURATION_1MS   1   /* minimum duration for 1ms timers */
#define PF_TIMER_MAX_DURATION_1MS   20  /* maximum duration for 1ms timers */

#define PF_TIMER_MIN_DURATION_10MS  10  /* minimum duration for 10ms timers */
#endif


/*******************************************************************************
*  Function:    pf_timer_init
*
*  Description: initialize timer service. initialize 10ms timer manager and
*               1ms timer manager. initialize kernel alarm and create timer task. 
*
*  Input:       
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_timer_init();

/*******************************************************************************
*  Function:    pf_print_timer_stat
*
*  Description: print timer manager statistics.
*
*  Input:        
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_print_timer_stat();

#endif /* #ifndef _PF_TIMER_SERVICE_H */
