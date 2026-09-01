/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    eCos_timer_service.cpp

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-11-15
  
  Description:  This file implements timer service interfaces.

  Function List:
  pf_timer_init: initialize platform timer service.
  pf_set_timer: setup a timer in 10ms timer service.
  pf_set_cycle_timer: setup a cycle timer in 10ms timer service.
  pf_kill_timer: kill a timer in 10ms timer service.
  pf_is_set_timer_internal: query a running timer in 10ms timer sercie.
  pf_set_utimer: setup a timer in 1ms timer service.
  pf_set_cycle_utimer: setup a cycle timer in 1ms timer service.
  pf_kill_utimer: kill a timer in 1ms timer service.
  pf_is_set_utimer_internal: query a running timer in 1ms timer sercie.
  pf_utimer_tick: 1ms timer service tick maintenance.
  pf_prepare_calibrate_timer: prepare timer calibration before DRX sleep.
  pf_calibrate_timer: calibrate timers after DRX wakeup.
  pf_print_timer_stat: print timer service statistics.
  
  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-11-15  1.0         First implementation of timer service 

*******************************************************************************/

#define THIS_MODULE MODULE_TIMER

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/timerfd.h>

#include <pl.h>

#include "pf_timer_manager.h"
#include "pf_timer_trace.h"
#include "pf_timer_service.h"
#include "pf_thread_mon.h"
#include "pf_upgrade.h"

/* flag of whether pl timer serivce is available or not. TRUE or FALSE */
U32 g_u32TimerServiceEnabled = FALSE;

/* 1ms timer manager*/
TIMER_MANAGER_S g_stTimerManager1ms;  

/* posix timer FD */
int gTimerFd;

std::vector<U32> g_stTimerFrameVector;


PF_TIMER_TRACE_DECLARE; /*lint !e19 */

/*******************************************************************************
*  Function:    pf_timer_entry
*
*  Description: timer task entry function. it wait on g_stTimerAlarmSem semaphore.
*               once triggered by semaphore, do 10ms timer manager tick maintenance.
*
*  Input:       pMsgQ    pointer of timer task message queue. currently it is not used.
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_timer_entry(pf_addrword_t pMsgQ)
{
    U32 i;
    U32 j;
    U32 count = 0xFFFFFFFF;
    U64 exp;
    struct timeval tv;
    FRAME_REPORT_IND_S stFrameExpectInd;
    pl_log(INF, "timer servcie: timer task started.");

    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH]; 
    S32 slTime;

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), THREAD_MONITOR_TIME_FILE);

    slTime = pf_get_config_integer(aucStrBuf);

    //monitor文件不存在，进程未启动
    if(slTime <= 0)
    {
        slTime = THREAD_MONITOR_TIME_DEFAULT;
    }

    for (i = 1;;i++)
    {
        j = read(gTimerFd, &exp, sizeof(exp));

#if DRC_PROCESS
        U32 u32Size = g_stTimerFrameVector.size();
        if(u32Size)
        {
            if(0xFFFFFFFF == count)
            {
                if(pf_get_timeofday(&tv, NULL))
                {
                    PS_CPlus(CM_PES, CMPES_ID_TIMER_ENTRY_TIMEOFDAY_FAIL);
                }
                else
                {
                    /*Guarantee startup in 0ms*/
                    if(tv.tv_usec % 100000 < 1000)
                    {
                        /*start to send the frame time*/
                        count = 1;
#ifdef PF_FRAME_PERIOD_SIZE
                        stFrameExpectInd.ulFrameTimer = tv.tv_sec % PF_FRAME_PERIOD_SIZE;
#else
                        stFrameExpectInd.ulFrameTimer = tv.tv_sec;
#endif
                        stFrameExpectInd.ulSubFrameTimer = tv.tv_usec / 100000;
						for (U32 idx = 0; idx < u32Size; idx++)
						{
							pf_copy_msg(
								MODULE_TIMER, 
								FRAME_REPORT_IND, 
								g_stTimerFrameVector[idx], 
								&stFrameExpectInd, 
								sizeof(FRAME_REPORT_IND_S));
						}
    
                        pl_log(INF, "timer servcie: timer start info %d,%d(%d,%d) u32Size=%d id=%d,%d", tv.tv_sec, tv.tv_usec, stFrameExpectInd.ulFrameTimer, stFrameExpectInd.ulSubFrameTimer, u32Size, g_stTimerFrameVector[0], g_stTimerFrameVector[u32Size-1]);
    
                        /*set the expect frame time*/
                        stFrameExpectInd.ulSubFrameTimer++;
    
                        if(10 == stFrameExpectInd.ulSubFrameTimer)
                        {
                            stFrameExpectInd.ulSubFrameTimer = 0;
                            stFrameExpectInd.ulFrameTimer++;
#ifdef PF_FRAME_PERIOD_SIZE
                            stFrameExpectInd.ulFrameTimer %= PF_FRAME_PERIOD_SIZE;
#endif
                        }
                    }
                }
            }
            else
            {
                if(pf_get_timeofday(&tv, NULL))
                {
                    PS_CPlus(CM_PES, CMPES_ID_TIMER_ENTRY_TIMEOFDAY_FAIL);
                }
                else
                {
                    U32 ulFlag = 0;
                    if(stFrameExpectInd.ulSubFrameTimer == tv.tv_usec/100000)
                    {
                        /**/
                        ulFlag = 1;
                    }
                    else if(0 == count%100)
                    {
                        ulFlag = 1;
                        PS_CPlus(CM_PES, CMPES_ID_TIMER_ENTRY_TIMEINFO_FAIL);
                    }
    
                    if(ulFlag)
                    {
						for (U32 idx = 0; idx < u32Size; idx++)
						{
							pf_copy_msg(
								MODULE_TIMER, 
								FRAME_REPORT_IND, 
								g_stTimerFrameVector[idx], 
								&stFrameExpectInd, 
								sizeof(FRAME_REPORT_IND_S));
						}
    
                        pl_log(INF, "timer servcie: timer cycle info %d,%d(%d,%d) u32Size=%d id=%d,%d", tv.tv_sec, tv.tv_usec, stFrameExpectInd.ulFrameTimer, stFrameExpectInd.ulSubFrameTimer, u32Size, g_stTimerFrameVector[0], g_stTimerFrameVector[u32Size-1]);
    
                        count = 0;
                        /*set the expect frame time*/
                        stFrameExpectInd.ulSubFrameTimer++;
    
                        if(10 == stFrameExpectInd.ulSubFrameTimer)
                        {
                            stFrameExpectInd.ulSubFrameTimer = 0;
                            stFrameExpectInd.ulFrameTimer++;
#ifdef PF_FRAME_PERIOD_SIZE
                            stFrameExpectInd.ulFrameTimer %= PF_FRAME_PERIOD_SIZE;
#endif
                        }
                    }
                }
    
                count++;
            }

        }

#endif

        if (j == sizeof(exp))
        {
            for (j = 0; j < (U32)exp; j++)
            {
                tm_tick(&g_stTimerManager1ms); // normal tick maintanence
            }
#ifdef PF_TIMER_DEBUG
            if (j > 1)
            {
                pl_log(ERR, "timer overrun: %u\n", j);
            }
#endif
        }
        else
        {
            PS_CPlus(CM_PES, CMPES_ID_TIMER_ENTRY_READ_FAIL);
        }

        // here we do some maintanence tasks every 10s.
        if (i == slTime)
        {
#ifdef PF_TIMER_DEBUG
            //keep alive print
            struct timeval tv;
            gettimeofday(&tv, NULL);
            pl_log(INF, "TimerService:1s, sec=%u, usec=%u.\n", tv.tv_sec, tv.tv_usec);
#endif
            pf_thread_mon_check();
            i = 0;
        }

    }

    return;     /*lint !e527 */
} /* lint !e550 */

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
S32 pf_timer_init(U32 ulModuleId)
{

    U32 u32Rslt;
    struct itimerspec val;

    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH]; 
    S32 slTime = 0;

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), MAX_TIMER_NUMBER_FILE);

    slTime = pf_get_config_integer(aucStrBuf);

    //monitor文件不存在，进程未启动
    if(slTime <= 0)
    {
        slTime = PF_MAX_TIMER_NUM_1MS;
    }

    PF_TIMER_TRACE_INIT;

    g_u32TimerServiceEnabled = FALSE;

    u32Rslt = tm_init_timer_manager(&g_stTimerManager1ms, 
                                    slTime,
                                    PF_HASH_TABLE_SIZE_1MS,
                                    PF_TIMER_TICK_INTERVAL_1MS);
    if (PF_TIMER_SUCCESS != u32Rslt)
    {
        pl_log(ERR, 
                "timer servcie: initialize 1ms Timer Manager failed, result: %u.", 
                u32Rslt);
        PS_CPlus(CM_PES, CMPES_ID_TIMER_INIT_TM_FAIL);
        return PF_RET_FAILURE;
    }

    gTimerFd = timerfd_create(CLOCK_REALTIME, 0);
    if (gTimerFd == -1)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_INIT_CREATE_FAIL);
        return PF_RET_FAILURE;
    }

    val.it_value.tv_sec = 0;
    val.it_value.tv_nsec = 1000000; // 1ms
    val.it_interval = val.it_value;
    if (timerfd_settime(gTimerFd, 0, &val, NULL) == -1)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_INIT_TIME_FAIL);
        return PF_RET_FAILURE;
    }
   
    g_u32TimerServiceEnabled = TRUE;

    pl_log(INF, "timer service: initialize successfully %d.", ulModuleId);

    return PF_RET_SUCCESS;
}



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
void pf_print_timer_stat()
{
    pl_log(INF, 
            "timer servcie: 1ms timer count: = %u", 
            tm_get_timer_count(&g_stTimerManager1ms)); 
}

/*******************************************************************************
*  Function:    pf_timer_start
*
*  Description: Setup a one-shot timer to timer manager. Return error code.
*
*  Input:       ulSrcModuleId   application module ID. used as timer expire message receiver. 
*                               the caller shall make sure it is a valid value.
*               ulDuration      timer duration in millisec.
*               ulTypeId        application timer type ID. not used by timer manager.
*               ulParam         application timer parameter. not used by timer manager.
*
*  Output:      pulTimerId      generated timer ID, used for index a timer.
*
*  Return:      PF_TIMER_SUCCESS        timer setup succeeded.
*               PF_TIMER_NO_RESOURCE    no free timer resource. timer setup failed.
*               PF_TIMER_OUT_OF_SERVICE timer service is not available.
*               PF_TIMER_PARAM_ERROR    input parameter error.
*
*******************************************************************************/
U32 pf_timer_start(U32 ulSrcModuleId, U32 ulDuration, U32 ulTypeId, U64 ullParam,U32* pulTimerId)
{
    U32 u32Rslt;
    
    if (FALSE == g_u32TimerServiceEnabled)
    {
        u32Rslt = PF_TIMER_OUT_OF_SERVICE;
        PF_SET_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTypeId, ulDuration); /*lint !e505 !e522 */
        PS_CPlus(CM_PES, CMPES_ID_TIMER_START_ENABLE_FAIL);
        return u32Rslt;
    }

    if ((ulSrcModuleId >= MODULE_MAX) 
        || (NULL == pulTimerId)) /* parameter check */
    {
        pl_log(ERR, 
                "timer servcie: wrong input parameter for 10ms timer!"
                "u8ModuleId: %u, u32Duration: %u, u16TypeId: %u, u32Para: %llu, pu32TimerId: %p.",
                ulSrcModuleId,
                ulDuration,
                ulTypeId,
                ullParam,
                pulTimerId);
        
        u32Rslt = PF_TIMER_PARAM_ERROR;
        PF_SET_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTypeId, ulDuration); /*lint !e505 !e522 */
        PS_CPlus(CM_PES, CMPES_ID_TIMER_START_PARAM_FAIL);
        return u32Rslt;
    }

    u32Rslt = tm_set_timer( &g_stTimerManager1ms, 
                            ulSrcModuleId, 
                            ulDuration + 1, // compensate the first tick by adding 1 
                            ulTypeId, 
                            ullParam, 
                            FALSE, 
                            pulTimerId);

    PF_SET_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTypeId, ulDuration); /*lint !e505 !e522 */
    PS_CPlus(CM_COM, CMCOM_ID_TIMER_START_CNT);
    
    return u32Rslt;
}

/*******************************************************************************
*  Function:    pf_timer_cycle_start
*
*  Description: Setup a cycle timer to timer manager. Return error code.
*
*  Input:       ulSrcModuleId   application module ID. used as timer expire message receiver. 
*                               the caller shall make sure it is a valid value.
*               ulDuration      timer duration in millisec.
*               ulTypeId        application timer type ID. not used by timer manager.
*               ulParam         application timer parameter. not used by timer manager.
*
*  Output:      pulTimerId      generated timer ID, used for index a timer.
*
*  Return:      PF_TIMER_SUCCESS        timer setup succeeded.
*               PF_TIMER_NO_RESOURCE    no free timer resource. timer setup failed.
*               PF_TIMER_OUT_OF_SERVICE timer service is not available.
*               PF_TIMER_PARAM_ERROR    input parameter error.
*
*******************************************************************************/
U32 pf_timer_cycle_start(U32 ulSrcModuleId, U32 ulDuration, U32 ulTypeId, U64 ullParam,U32* pulTimerId)
{
    U32 u32Rslt;
    if (FALSE == g_u32TimerServiceEnabled)
    {
        u32Rslt = PF_TIMER_OUT_OF_SERVICE;
        PF_SET_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTypeId, ulDuration); /*lint !e505 !e522 */
        PS_CPlus(CM_PES, CMPES_ID_TIMER_CYCLE_START_ENABLE_FAIL);
        return u32Rslt;
    }

    if ((ulSrcModuleId >= MODULE_MAX) 
        || (NULL == pulTimerId) 
        || (0 == ulDuration)) /* parameter check */
    {
        pl_log(ERR, 
                "timer servcie: wrong input parameter for 10ms timer!"
                "u8ModuleId: %u, u32Duration: %u, u16TypeId: %u, u32Para: %llu, pu32TimerId: %p.",
                ulSrcModuleId,
                ulDuration,
                ulTypeId,
                ullParam,
                pulTimerId);
        
        u32Rslt = PF_TIMER_PARAM_ERROR;
        PF_SET_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTypeId, ulDuration); /*lint !e505 !e522 */
        PS_CPlus(CM_PES, CMPES_ID_TIMER_CYCLE_START_PARAM_FAIL);
        return u32Rslt;
    }

    u32Rslt = tm_set_timer( &g_stTimerManager1ms, 
                            ulSrcModuleId, 
                            ulDuration, 
                            ulTypeId, 
                            ullParam, 
                            TRUE, 
                            pulTimerId);

    PF_SET_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTypeId, ulDuration); /*lint !e505 !e522 */
    PS_CPlus(CM_COM, CMCOM_ID_TIMER_CYCLE_START_CNT);
    
    return u32Rslt;
}


/*******************************************************************************
*  Function:    pf_timer_stop
*
*  Description: kill a timer from timer manager. Return error code.
*
*  Input:       ulTimerId       timer ID. it must be the same one generated by 
*                               pf_timer_start or pf_timer_cycle_start.
*               ulSrcModuleId   application module ID. it must be the same one 
*                               when setup this timer.
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        timer delete succeeded.
*               PF_TIMER_PARAM_ERROR    timer ID is invalid. probably random value.
*               PF_TIMER_NOT_PERMIT     timer ID is valid, but module ID is wrong.
*               PF_TIMER_NOT_EXIST      timer ID may be valid, but it is not running.
*               PF_TIMER_OUT_OF_SERVICE timer service is not available.
*
*******************************************************************************/
U32 pf_timer_stop(U32 ulTimerId, U32 ulSrcModuleId)
{
    U32 u32Rslt;
    
    if (FALSE == g_u32TimerServiceEnabled)
    {
        u32Rslt = PF_TIMER_OUT_OF_SERVICE;
        PF_KILL_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTimerId); /*lint !e505 !e522 */
        PS_CPlus(CM_PES, CMPES_ID_TIMER_STOP_ENABLE_FAIL);
        return u32Rslt;
    }

    u32Rslt = tm_kill_timer(&g_stTimerManager1ms, ulTimerId, ulSrcModuleId);
    PF_KILL_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTimerId); /*lint !e505 !e522 */
    PS_CPlus(CM_COM, CMCOM_ID_TIMER_STOP_CNT);
    
    return u32Rslt;
}

/*******************************************************************************
*  Function:    pf_is_set_timer_internal
*
*  Description: query a timer from 10ms timer manager. Return error code.
*               this function is only for debug and MT.
*
*  Input:       u32TimerId      timer ID. it must be the same one generated by tm_set_timer.
*               u32ModuleId     application module ID. it must be the same one 
*                               when setup this timer.
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        timer query succeeded. the timer is running.
*               PF_TIMER_PARAM_ERROR    timer ID is invalid. probably random value.
*               PF_TIMER_NOT_PERMIT     timer ID is valid, but module ID is wrong.
*               PF_TIMER_NOT_EXIST      timer ID may be valid, but it is not running.
*               PF_TIMER_OUT_OF_SERVICE timer service is not available.
*
*******************************************************************************/
U32 pf_is_set_timer_internal(U32 u32TimerId, U32 u32ModuleId)
{
    U32 u32Rslt;

    if (FALSE == g_u32TimerServiceEnabled)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_INTERNAL_ENABLE_FAIL);
        return PF_TIMER_OUT_OF_SERVICE;
    }

    u32Rslt = tm_is_set_timer(&g_stTimerManager1ms, u32TimerId, u32ModuleId);
 
    return u32Rslt;
}

/*******************************************************************************
*  Function:    pf_timer_restart
*
*  Description:Used to restart the specified timer
*
*  Input:       ulTimerId       timer ID. it must be the same one generated by 
*                               pf_timer_start or pf_timer_cycle_start.
*               ulSrcModuleId   application module ID. it must be the same one 
*                               when setup this timer.
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        timer restart succeeded.
*               PF_TIMER_PARAM_ERROR    timer ID is invalid. probably random value.
*               PF_TIMER_NOT_PERMIT     timer ID is valid, but module ID is wrong.
*               PF_TIMER_NOT_EXIST      timer ID may be valid, but it is not running.
*               PF_TIMER_OUT_OF_SERVICE timer service is not available.
*
*******************************************************************************/
U32 pf_timer_restart(U32 ulTimerId, U32 ulSrcModuleId)
{
    U32 u32Rslt;
    
    if (FALSE == g_u32TimerServiceEnabled)
    {
        u32Rslt = PF_TIMER_OUT_OF_SERVICE;
        PF_RESTART_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTimerId); /*lint !e505 !e522 */
        PS_CPlus(CM_PES, CMPES_ID_TIMER_RESTART_FAIL);
        return u32Rslt;
    }

    u32Rslt = tm_restart_timer(&g_stTimerManager1ms, ulTimerId, ulSrcModuleId);
    PF_RESTART_TIMER_TRACE(u32Rslt, ulSrcModuleId, ulTimerId); /*lint !e505 !e522 */    
    PS_CPlus(CM_COM, CMCOM_ID_TIMER_RESTART_CNT);
    
    return u32Rslt;
}


/*******************************************************************************
*  Function:    pf_timer_frame_data_start
*
*  Description: query a timer which output the frame number to the modules every 100ms 
*
*  Input:       aulDstModuleId  the vector of module index.
*
*  Output:      
*
*  Return:      PF_TIMER_SUCCESS        timer query succeeded. the timer is running.
*               PF_TIMER_PARAM_ERROR    timer ID is invalid. probably random value.
*               PF_TIMER_OUT_OF_SERVICE timer service is not available.
*
*******************************************************************************/
U32 pf_timer_frame_data_start(std::vector<U32> aulDstModuleId)
{
    U32 u32Rslt;
    U32 u32Size = aulDstModuleId.size();

    if (FALSE == g_u32TimerServiceEnabled)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_INTERNAL_ENABLE_FAIL);
        return PF_TIMER_OUT_OF_SERVICE;
    }

    if (0 == u32Size)
    {
        PS_CPlus(CM_PES, CMPES_ID_TIMER_INTERNAL_ENABLE_FAIL);
        return PF_TIMER_PARAM_ERROR;
    }

    for (U32 idx = 0; idx < u32Size; idx++)
    {
        g_stTimerFrameVector.push_back(aulDstModuleId[idx]);
    }
 
    return PF_TIMER_SUCCESS;
}


