/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    eCos_thread_mon.h

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-12-31
  
  Description:  This file declares thread monitor interfaces.

  Function List:
  pf_thread_mon_init: initialize thread monitor service.
  pf_thread_mon_update: update thread status for a module.
  pf_thread_mon_check: check status of all the modules.

  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-12-31  1.0         First implementation of thread monitor

*******************************************************************************/
#ifndef _ECOS_THREAD_MON_H
#define _ECOS_THREAD_MON_H

#include <pl.h>

//Thread monitoring time profile path
#define THREAD_MONITOR_TIME_FILE                "/config/bootup/thread_monitor_time"

#define DRSU_THREAD_MONITOR_TIME_DEFAULT        80000

#define DRC_THREAD_MONITOR_TIME_DEFAULT         10000


#define REMAINING_SPACE_SIZE         2148 

typedef enum
{
    SYMBOL_LOG_WRITE_COMPLETE        = 0x0E,
    SYMBOL_LOG_NOT_WRITTEN           = 0x0F,
    SYMBOL_USE_APPLICATION_SPACE     = 0xEE,
    SYMBOL_SET_USE_APPLICATION_SPACE = 0xEF,
    SYMBOL_USE_LOG_SPACE             = 0xFE,
    SYMBOL_SET_USE_LOG_SPACE         = 0xFF,
}PF_LOG_STATE_E;


/*******************************************************************************
*  Function:    pf_thread_mon_init
*
*  Description: initialize thread monitor service. 
*
*  Input:       
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_thread_mon_init();

/*******************************************************************************
*  Function:    pf_thread_mon_stop
*
*  Description: stop the thread module of monitor service. 
*
*  Input:       the module index
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_thread_mon_stop(U32 ulMid);

/*******************************************************************************
*  Function:    pf_thread_mon_start
*
*  Description: start the thread module of monitor service. 
*
*  Input:       the module index
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_thread_mon_start(U32 ulMid);


/*******************************************************************************
*  Function:    pf_thread_mon_update
*
*  Description: update thread status for a module.
*
*  Input:       mid    moudle ID of the thread to be updated.
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_thread_mon_update(U32 mid,U32 src,U32 msgID,U32 dst);


/*******************************************************************************
*  Function:    pf_thread_mon_update_count
*
*  Description: update thread status for a module.
*
*  Input:       mid    moudle ID of the thread to be updated.
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_thread_mon_update_count(U32 mid);


/*******************************************************************************
*  Function:    pf_thread_mon_check
*
*  Description: check status of all the modules 
*
*  Input:       
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_thread_mon_check();

#ifdef __cplusplus
extern "C" {
#endif
/*******************************************************************************
*  Function:    pf_thread_mon_get_count
*
*  Description: get thread counts for a module.
*
*  Input:       mid    moudle ID of the thread to be updated.
*
*  Output:      
*
*  Return:      count  the counts of the thread of the moudle ID
*
*******************************************************************************/
U32 pf_thread_mon_get_count(U32 mid);

#ifdef __cplusplus
}
#endif


/*******************************************************************************
*  Function:    pf_thread_mon_set_stack
*
*  Description: thread monitor set the value for the stack according to the module ID. 
*
*  Input:       ulMid          moudle ID of the abnormal thread.
*               pucStack       start address of stack
*               ulStackSize    stack size
*               pLogStartAddr  start address of log space
*               ulLogSize      log write size 
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_thread_mon_set_stack(U32 ulMid, void* pucStack, U32 ulStackSize, void* pLogStartAddr, U32 ulLogSize);


/*******************************************************************************
*  Function:    pf_print_stack_stat
*
*  Description: print stack manager statistics.
*
*  Input:        
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pf_print_stack_stat(void);

/*******************************************************************************
*  Function:    pf_thread_mon_check_stack_usage
*
*  Description: monitor stack usage
*
*  Input:       
*  Output:      
*  Return:      
*******************************************************************************/
void pf_thread_mon_check_stack_usage(void);

/*******************************************************************************
* Function:      function pf_thread_mon_interval_debug_init
*
* Description:   initial the interval debug module
*
* input:         scPath The file name of interval debug module
*
* output :
* return :
*******************************************************************************/
void pf_thread_mon_interval_debug_init(CHAR* pcDebugFileName);

/*******************************************************************************
*  Function:    pf_thread_mon_update
*
*  Description: whether the module of interval debug is open or not.
*
*  Input:      
*
*  Output:      
*
*  Return:      0: not exist
*               1: exist
*
*******************************************************************************/
BOOL pf_thread_mon_interval_debug_is_open(void);


/**********************************************************************************************
 * @function      pf_thread_mon_update_read_section
 * @brief         update some read information according to the mid
 * @input         mid             thread id
 *                ulLenStatistics length of a log message
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_thread_mon_update_read_section(U32 mid, U32 ulLenStatistics);

/**********************************************************************************************
 * @function      pf_thread_mon_get_log_info
 * @brief         get the address and writable size of the log space according to the mid
 * @input         ulMid             thread id
 *                
 * @output        pulogWriteAddr      write pointer of log space
 *                pulLogAvaiMaxOffset log available size
 * @return        void
 *********************************************************************************************/
void pf_thread_mon_get_log_info(U32 ulMid, void** pulogWriteAddr, U32* pulLogAvaiMaxOffset);

/**********************************************************************************************
 * @function      pf_thread_mon_update_write_section
 * @brief         update some write information according to the mid
 * @input         ulMid             thread id
 *                totaloffset       length of a log message
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_thread_mon_update_write_section(U32 ulMid, U32 totaloffset);

#endif /* #ifndef _ECOS_THREAD_MONITOR_H */
