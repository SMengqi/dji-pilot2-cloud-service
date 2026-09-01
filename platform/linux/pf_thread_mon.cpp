/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    eCos_thread_mon.cpp

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-12-31
  
  Description:  This file implements thread monitor interfaces.

  Function List:
  pf_thread_mon_init: initialize thread monitor service.
  pf_thread_mon_update: update thread status for a module.
  pf_thread_mon_check: check status of all the modules.
  pl_thread_mon_abnormal_hdl: handle an abnormal module.

  
  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-12-31  1.0         First implementation of thread monitor

*******************************************************************************/
#define THIS_MODULE PLATFORM_EX

#include <os.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "pl.h"
#include "module.h"
#include "event.h"
#include "pf_thread_mon.h"
#include "pf_stat.h"
#include "pf_mbox.h"
#include "pf_upgrade.h"


typedef struct {
    U32 ulCurMsgCnt;
    U32 ulLastMsgCnt;
    U32 ulMsgSrc;
    U32 ulMsgId;
    U32 ulMsgDst;
    U32 ulMonFlag;
    U32 ulStackMaxOffset;   
    U32 ulStackUsedOffset;
    U32 ulLogAvaiMaxOffset;
    U32 ulLogWriteOffset;
    U32 ulLogReadOffset;
    U32 ulpadding;
    void* pucStackUsedAddr; 
    void* pucStackStartAddr; 
    void* pucStackEndAddr;   
    void* pucLogStartAddr;
    void* pucLogEndAddr;
    void* pucLogWriteAddr;
    void* pucLogReadAddr;
} THREAD_MON_S;

THREAD_MON_S gThreadMon[MODULE_TASK_MAX];

extern pf_handle_t workerhandles[];
extern "C" pf_mbox_t msgQArray[];
extern MODULE_ENTRY moduleArray[];

U32 ulDebugFileState;

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
void pf_thread_mon_init()
{
    U32 i;

    for (i = 0; i < MODULE_TASK_MAX; i++)
    {
        gThreadMon[i].ulCurMsgCnt = 0;
        gThreadMon[i].ulLastMsgCnt = 0;
        gThreadMon[i].ulMonFlag = TRUE;
    }

    /*/FTP thread interface execution time depends on the file length  
    stop the FTP thread monitor*/
    pf_thread_mon_stop(MODULE_FTP);
    pf_thread_mon_stop(MODULE_DAILYREC);

    /*set interval debug initial flag */
    pf_thread_mon_interval_debug_init(PLATFORM_INTERVAL_DEBUG_PATH);
}

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
void pf_thread_mon_stop(U32 ulMid)
{
    if (ulMid < MODULE_TASK_MAX)
    {
        gThreadMon[ulMid].ulMonFlag = FALSE;
    }
}

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
void pf_thread_mon_start(U32 ulMid)
{
    if (ulMid < MODULE_TASK_MAX)
    {
        gThreadMon[ulMid].ulMonFlag = TRUE;
    }
}


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
void pf_thread_mon_set_stack(U32 ulMid, void* pucStack, U32 ulStackSize, void* pLogStartAddr, U32 ulLogSize)
{
    if (ulMid < MODULE_TASK_MAX)
    {
        gThreadMon[ulMid].pucStackStartAddr = pucStack;
        gThreadMon[ulMid].pucStackEndAddr = pucStack + ulStackSize;
        gThreadMon[ulMid].pucStackUsedAddr = pucStack + ulStackSize;
        gThreadMon[ulMid].ulStackMaxOffset = ulStackSize;
        gThreadMon[ulMid].ulStackUsedOffset = 0;
        gThreadMon[ulMid].pucLogStartAddr = pLogStartAddr;
        gThreadMon[ulMid].pucLogWriteAddr = pLogStartAddr;
        gThreadMon[ulMid].pucLogReadAddr = pLogStartAddr;
        gThreadMon[ulMid].ulLogAvaiMaxOffset = ulLogSize;   
        gThreadMon[ulMid].ulLogWriteOffset= 0;
        gThreadMon[ulMid].ulLogReadOffset = 0;
    }
}

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
void pf_print_stack_stat(void)
{
    for(int mid = 0; mid < MODULE_TASK_MAX; mid++)
    {
        if(NULL != gThreadMon[mid].pucStackStartAddr)
        {
            pl_log(TRC, "MODLE:%10s stackStartAddr:%p, stackEndAddr:%p, stackUseAddr:%p, stackMaxSize:%d, stackUseSize:%d",pf_get_module_name(mid),\
                    gThreadMon[mid].pucStackStartAddr, gThreadMon[mid].pucStackEndAddr, gThreadMon[mid].pucStackUsedAddr, gThreadMon[mid].ulStackMaxOffset, gThreadMon[mid].ulStackUsedOffset);
        
        }
    }
}

/**********************************************************************
Function:
    U16 pf_show_platform_info(void)
Description: 
    platform���performanceѭ������ͳ�ƴ�ӡ
Input:
    void
Output:
    void
Return: 
    0-success
    other-failure
Others:        
************************************************************************/
U16 pf_show_platform_info(void)
{
    //int  lCpuload = 0;
    char outString[2048] ="\r\n";
    struct stru_mem_stat stMemStat = {0};
    S32 alCpuPerThread[MODULE_TASK_MAX] = {0};
    S32 lProcCpuMil = 0;
    F32 fProcCpu = 0;
    //pf_stat_get_cpu_load(&lCpuload);
    pf_stat_get_mem_info(&stMemStat);
    pf_stat_get_process_cpu(&lProcCpuMil);
    fProcCpu = (F32)lProcCpuMil / 10;
    
    /*pl_log(INF, "Cpuload=%d%c in %dms; Mem: Total=%u(MB), Used=%u(MB), Free=%u(MB)", 
        lCpuload, '%', MC_PERFORMANCE_CYCLE, stMemStat.mem_total/(1024*1024), 
        stMemStat.mem_used/(1024*1024), stMemStat.mem_free/(1024*1024));*/
    sprintf(outString, "ProcCpuload=%.1f%c, Mem: Total=%u(MB), Used=%u(MB), Free=%u(MB)", 
            fProcCpu, '%', 
            stMemStat.mem_total/(1024*1024), stMemStat.mem_used/(1024*1024), stMemStat.mem_free/(1024*1024));

    pf_stat_get_thread_cpu(MODULE_TIMER, &alCpuPerThread[MODULE_TIMER]);
    pf_stat_get_thread_cpu(MODULE_LOG, &alCpuPerThread[MODULE_LOG]);
    printf("%s\n", outString);
    pl_log(INF, "%s", outString);    
    sprintf(outString, "CpuloadPerThread: TIMER=%u,LOG=%u",
        alCpuPerThread[MODULE_TIMER], 
        alCpuPerThread[MODULE_LOG]);

    printf("%s\n", outString);    
    pl_log(INF, "%s", outString);

    return 0;
}

/*******************************************************************************
*  Function:    show_kernel_stack
*
*  Description: dump all the process kernel stack infomation.
*
*  Input:       pid    pid of the main process(thread leader).
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
S32 show_kernel_stack(pid_t ppid)
{
    char buff[200];
    char cmdBuff[200];
    char ch = 0;
    FILE *fd = NULL;
    dirent *ptr = NULL;
    DIR *dir = NULL;
    pid_t pid = getpid();
    U32 ulLen = 0;

    if(pid <= 0)
    {
        return PF_RET_FAILURE;
    }

    sprintf(buff, "/proc/%d/task/", pid);

    dir = opendir(buff);

    if(NULL != dir)
    {
        while((ptr = readdir(dir)) != NULL)
        {
            if(!strcmp(".", ptr->d_name) || !strcmp("..", ptr->d_name))
                continue;

            bzero(buff, sizeof(buff));
            sprintf(buff, "/proc/%d/task/", pid);    
            sprintf(buff, "%s%s/stack", buff, ptr->d_name);
            printf("kernel stack of %s\n", buff);

            /*读取各个线程的堆栈信�?*/
            if(PF_RET_SUCCESS == pf_get_file_length((S8 *)buff, &ulLen))
            {
                if(ulLen > 0)
                {
                    CHAR* pcStr = (CHAR*)pf_malloc(ulLen+1);

                    if(pcStr)
                    {
                        pf_read_flush_file((const S8 *)buff, (const S8 *)pcStr, ulLen);
                        pcStr[ulLen] = 0;
                        printf("kernel stack of %s info is\r\n%s\r\n", buff, pcStr);
                        pl_log(DO_NOT_USE, "kernel stack of %s info is\r\n%s\r\n", buff, pcStr);
                        pf_free((void *)pcStr);
                    }
                }
            }

            printf("kernel stack length %d\n", ulLen);
            //sprintf(cmdBuff, "cat %s >> %s", buff, pf_get_log_path());
            //pf_set_system_call((const S8 *) cmdBuff);

            sprintf(cmdBuff, "cat %s", buff);
            pf_set_system_call((const S8 *) cmdBuff);
            
        }

        closedir(dir);
    }

    return PF_RET_SUCCESS;
}

/*******************************************************************************
*  Function:    pl_thread_mon_abnormal_dump
*
*  Description: dump all the abnormal process info, as cpuload/leftmesg etc.
*
*  Input:       mid    moudle ID of the abnormal thread.
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pl_thread_mon_abnormal_dump(U32 mid)
{
    printf("ThreadMonitor: Fatal! abnormal moduleID=%u<%s>, threadID=0x%x.\n", 
           mid, pf_get_module_name(mid),
           workerhandles[mid]);

    char outString[512] ="\r\n";

    sprintf(outString, "%s ThreadMonitor: Fatal! abnormal moduleID=%u<%s>, threadID=0x%x.,Src=%u<%s>,MsgId=%u<%s>,Dst=%u<%s>,MboxSize=%d\n", 
           outString,
           mid, 
           pf_get_module_name(mid),
           workerhandles[mid],
           gThreadMon[mid].ulMsgSrc,
           pf_get_module_name(gThreadMon[mid].ulMsgSrc),
           gThreadMon[mid].ulMsgId,
           pf_get_event_name(gThreadMon[mid].ulMsgId),
           gThreadMon[mid].ulMsgDst,
           pf_get_module_name(gThreadMon[mid].ulMsgDst),
           pf_mbox_peek(msgQArray[mid]));
    printf("%s\n", outString);
    
    pl_log(DO_NOT_USE, "%s", outString);
    
}

/*******************************************************************************
*  Function:    pl_thread_mon_dump
*
*  Description: dump all the process info, as cpuload/leftmesg etc.
*
*  Input:       mid    moudle ID of the thread.
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pl_thread_mon_dump(U32 mid)
{
    printf("ThreadMonitor: Fatal! abnormal moduleID=%u<%s>, threadID=0x%x.\n", 
           mid, pf_get_module_name(mid),
           workerhandles[mid]);

    char outString[512] ="\r\n";

    sprintf(outString, "%s ThreadMonitor: moduleID=%u<%s>, threadID=0x%x.,Src=%u<%s>,MsgId=%u<%s>,Dst=%u<%s>,MboxSize=%d\n", 
           outString,
           mid, 
           pf_get_module_name(mid),
           workerhandles[mid],
           gThreadMon[mid].ulMsgSrc,
           pf_get_module_name(gThreadMon[mid].ulMsgSrc),
           gThreadMon[mid].ulMsgId,
           pf_get_event_name(gThreadMon[mid].ulMsgId),
           gThreadMon[mid].ulMsgDst,
           pf_get_module_name(gThreadMon[mid].ulMsgDst),
           pf_mbox_peek(msgQArray[mid]));
    printf("%s\n", outString);

    pl_log(DO_NOT_USE, "%s", outString);
    
}

/*******************************************************************************
*  Function:    pl_thread_mon_abnormal_hdl
*
*  Description: handle an abnormal module. suspend the thread and print stack.
*
*  Input:       mid    moudle ID of the abnormal thread.
*
*  Output:      
*
*  Return:      
*
*******************************************************************************/
void pl_thread_mon_abnormal_hdl(U32 mid)
{
    pl_thread_mon_dump(mid);

    // shall not delete thread now. otherwise it may disappear in coredump.
    // DO NOT call pf_thread_delete(workerhandles[mid]);
    pid_t mainPid = 0;
    mainPid = getpid();
    show_kernel_stack(mainPid);
    printf("kernel stack dump completed\n");

}

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
void pf_thread_mon_check()
{
    U32 i;
    U32 mid;
    /* flag of thread monitor status. TRUE for normal, and FALSE for abnormal. */
    static U32 gulThreadMonSuccFlag = TRUE;

    
#if 0 // enable coredump until PS is stable enough.
    if (TRUE != gulThreadMonSuccFlag)
    {
        printf("ThreadMonitor: get coredump now.\n");

        *(int*)0=0; /* ThreadMonitor: force coredump. */
        return;
    }
#endif

    U32 firstmid = 0;

    for (mid = 0; mid < MODULE_TASK_MAX; mid++)
    {        
        if (!gThreadMon[mid].ulMonFlag)
        {
            continue;
        }

        if ((gThreadMon[mid].ulCurMsgCnt & 1) == 0)
        {
            continue;
        }

        if (gThreadMon[mid].ulLastMsgCnt != gThreadMon[mid].ulCurMsgCnt)
        {
            //gThreadMon[mid].ulLastMsgCnt = gThreadMon[mid].ulCurMsgCnt;
            continue;
        }
        
        /* thread deadlock detected! */
        gulThreadMonSuccFlag = FALSE;
        if (firstmid == 0)
        {
            firstmid = mid;
        }

        pl_thread_mon_abnormal_dump(mid);
    }
    
    if (gulThreadMonSuccFlag == FALSE)
    {
        S8 ascPath[PS_UPDATE_NAME_LENGTH];

        snprintf((CHAR*)ascPath, PS_UPDATE_NAME_LENGTH, "%s%s%s", pf_get_root_path(), CONFIG_BOOTUP_PATH, "dr_ulimit_config");
        pf_show_platform_info();
        /* dump all thread's info*/
        for(int j = 0; j < MODULE_TASK_MAX; j++)
        {
            pl_thread_mon_dump(j);
            printf("mid:%d LastMsgCnt=%d, CurMsgCnt=%d\n", j,
                gThreadMon[j].ulLastMsgCnt, 
                gThreadMon[j].ulCurMsgCnt);

            pl_log(DO_NOT_USE, "mid:%d LastMsgCnt=%d, CurMsgCnt=%d\n", j,
                gThreadMon[j].ulLastMsgCnt, 
                gThreadMon[j].ulCurMsgCnt);
        }
        pl_thread_mon_abnormal_hdl(firstmid);

        CHAR* pcBootupFlag = "BOOTUP_FLAG_THREAD_MONITOR";
        
        /*writing reboot info into the file*/
        pf_write_root_path_file((const S8 *)PF_BOOTUP_FILE_PATH, (const S8 *)pcBootupFlag, strlen(pcBootupFlag));
        
        pl_log(DO_NOT_USE, "BOOTUP_FLAG_THREAD_MONITOR");
        pf_log_write_file();
        for(int i = 0;i < MODULE_TASK_MAX;i++)
        {
            msgQArray[i] = NULL;
            moduleArray[i] = NULL;
        }

        if(pf_is_file_exist(ascPath))
        {
            printf("file %s exit and coredump\r\n", ascPath);
            /*检测dr_ulimit_config文件存在触发coredump异常*/
            CHAR* pAddr = NULL;
            /*延迟2秒等待日志更新完�?/
            //pf_usleep(2000000);
            /*触发coredump异常*/
            *pAddr = 128;
        }
        
        _exit(0);

    }
    else
    {
        for (mid = 0; mid < MODULE_TASK_MAX; mid++)
        {
            gThreadMon[mid].ulLastMsgCnt = gThreadMon[mid].ulCurMsgCnt;
        }
    }
#if 0
    if (TRUE != gulThreadMonSuccFlag)
    {
        //todo: system level recovery strategy
    }
#endif
}

/*******************************************************************************
*  Function:    pf_thread_mon_check_stack_usage
*
*  Description: monitor stack usage
*
*  Input:       
*  Output:      
*  Return:      
*******************************************************************************/
void pf_thread_mon_check_stack_usage(void)
{
    U32 mid;
    U32 stackMaxOffset = 0;
    U32 stackUsedOffset = 0;
    U64* pu64Stack = NULL;
    U64* pu64StackUsedMddr = NULL;

    for (mid = 0; mid < MODULE_TASK_MAX; mid++)
    {
        U32 flag = 1;
        U32 count = 1024;
        pu64Stack = (U64*)gThreadMon[mid].pucStackUsedAddr;
        if(NULL == pu64Stack)
        {
            continue;
        }
        
        stackMaxOffset = gThreadMon[mid].ulStackMaxOffset/8;
        stackUsedOffset = gThreadMon[mid].ulStackUsedOffset/8;
        for(int i = stackUsedOffset; i < stackMaxOffset; i++)
        {
            pu64Stack --;
            if(0x5A5A5A5A5A5A5A5A == *pu64Stack)
            {  
                if(flag)
                {
                    pu64StackUsedMddr = pu64Stack;
                    stackUsedOffset = ((U64*)gThreadMon[mid].pucStackEndAddr - pu64StackUsedMddr - 1)*8;
                    flag = 0;
                }
                else if(i < (stackMaxOffset - 1))
                {
                    count--;
                    if(!count)
                    {
                        gThreadMon[mid].pucStackUsedAddr = pu64StackUsedMddr;
                        gThreadMon[mid].ulStackUsedOffset = stackUsedOffset;
                        break;
                    }
                }
                else
                {
                    gThreadMon[mid].pucStackUsedAddr = pu64StackUsedMddr;
                }
            }
            else if(!flag)
            {
                flag = 1;
                count = 1024;
            }
        }
    }
}

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
void pf_thread_mon_update(U32 mid, U32 src, U32 msgID, U32 dst)
{
    gThreadMon[mid].ulCurMsgCnt++;
    gThreadMon[mid].ulMsgSrc=src;
    gThreadMon[mid].ulMsgId=msgID;
    gThreadMon[mid].ulMsgDst=dst;
}

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
void pf_thread_mon_update_count(U32 mid)
{
    gThreadMon[mid].ulCurMsgCnt++;
}


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
extern "C" U32 pf_thread_mon_get_count(U32 mid)
{
    if(mid >= MODULE_MAX)
    {
        return 0;
    }
    U32 ulCounts = gThreadMon[mid].ulCurMsgCnt + gThreadMon[mid].ulLastMsgCnt;

    return ulCounts;
}

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
void pf_thread_mon_interval_debug_init(CHAR* pcDebugFileName)
{
    CHAR ascLocalFilePath[512];
    sprintf((CHAR*)ascLocalFilePath, "%s%s", pf_get_root_path(), pcDebugFileName);

    if(0 == access(ascLocalFilePath, F_OK))
    {
        ulDebugFileState = 1;
    }
    else 
    {
        ulDebugFileState = 0;
    }
}

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
BOOL pf_thread_mon_interval_debug_is_open(void)
{
    if(1 == ulDebugFileState)
    {
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}


/**********************************************************************************************
 * @function      pf_thread_mon_update_read_section
 * @brief         update some read information according to the mid
 * @input         ulMid             thread id
 *                ulLenStatistics length of a log message
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_thread_mon_update_read_section(U32 ulMid, U32 ulLenStatistics)
{
    if (ulMid < MODULE_TASK_MAX)
    {
        gThreadMon[ulMid].ulLogReadOffset += ulLenStatistics;
        gThreadMon[ulMid].pucLogReadAddr += ulLenStatistics;

        /*judge_read_size judge whether the log space has been read*/
        if(REMAINING_SPACE_SIZE + gThreadMon[ulMid].ulLogReadOffset >= LOG_STACK_MAX_SIZE)//ulReadOffset
        {
            gThreadMon[ulMid].pucLogReadAddr = gThreadMon[ulMid].pucLogStartAddr;
            gThreadMon[ulMid].ulLogReadOffset = 0;
            gThreadMon[ulMid].ulLogAvaiMaxOffset = LOG_STACK_MAX_SIZE - gThreadMon[ulMid].ulLogWriteOffset;
        }
    }
}

/**********************************************************************************************
 * @function      pf_thread_mon_get_log_info
 * @brief         get the address and writable size of the log space according to the mid
 * @input         ulMid             thread id
 *                
 * @output        pulogWriteAddr      write pointer of log space
 *                pulLogAvaiMaxOffset log available size
 * @return        void
 *********************************************************************************************/
void pf_thread_mon_get_log_info(U32 ulMid, void** pulogWriteAddr, U32* pulLogAvaiMaxOffset)
{
    if (ulMid < MODULE_TASK_MAX && NULL != gThreadMon[ulMid].pucLogStartAddr)
    {
        if(gThreadMon[ulMid].pucLogReadAddr > gThreadMon[ulMid].pucLogWriteAddr)
        {
            gThreadMon[ulMid].ulLogAvaiMaxOffset = gThreadMon[ulMid].ulLogReadOffset - gThreadMon[ulMid].ulLogWriteOffset - 2;
            *pulLogAvaiMaxOffset = gThreadMon[ulMid].ulLogAvaiMaxOffset;
        }
        
        if(gThreadMon[ulMid].ulLogAvaiMaxOffset <= REMAINING_SPACE_SIZE ) 
        {
            *pulogWriteAddr = (char *)pf_malloc(LOG_STACK_MAX_SIZE);
            if(NULL == *pulogWriteAddr)
            {
                return ;
            }
            
            *(U8*)(*pulogWriteAddr) = SYMBOL_SET_USE_APPLICATION_SPACE;
            *((U8*)(*pulogWriteAddr)+1) = (U8)ulMid;
            PS_CPlus(CM_PES, CMPES_ID_INSUFFICIENT_FREE_SPACE_FAIL); 
            
            if(((gThreadMon[ulMid].ulLogWriteOffset + REMAINING_SPACE_SIZE) >= LOG_STACK_MAX_SIZE) && (gThreadMon[ulMid].ulLogReadOffset > REMAINING_SPACE_SIZE))   
            {
                gThreadMon[ulMid].pucLogWriteAddr = gThreadMon[ulMid].pucLogStartAddr;
                gThreadMon[ulMid].ulLogAvaiMaxOffset = gThreadMon[ulMid].ulLogReadOffset;
                gThreadMon[ulMid].ulLogWriteOffset = 0;
            }
        }
        else
        {
            *pulogWriteAddr = gThreadMon[ulMid].pucLogWriteAddr;
            *(U8*)(*pulogWriteAddr) = SYMBOL_SET_USE_LOG_SPACE;
            *((U8*)(*pulogWriteAddr) + 1) = (U8)ulMid; 
            *pulLogAvaiMaxOffset = gThreadMon[ulMid].ulLogAvaiMaxOffset;
        }    
    }
    else
    {
        *pulogWriteAddr = (char *)pf_malloc(LOG_STACK_MAX_SIZE);
        if(NULL == *pulogWriteAddr)
        {
            return ;
        }
            
        *(U8*)(*pulogWriteAddr) = SYMBOL_SET_USE_APPLICATION_SPACE;
        *((U8*)(*pulogWriteAddr)+1) = (U8)ulMid;
    }
}

/**********************************************************************************************
 * @function      pf_thread_mon_update_write_section
 * @brief         update some write information according to the mid
 * @input         ulMid             thread id
 *                totaloffset       length of a log message
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_thread_mon_update_write_section(U32 ulMid, U32 totaloffset)
{
    if (ulMid < MODULE_TASK_MAX)
    {
        gThreadMon[ulMid].ulLogAvaiMaxOffset -= totaloffset;
        gThreadMon[ulMid].ulLogWriteOffset += totaloffset;
        gThreadMon[ulMid].pucLogWriteAddr += totaloffset;
    }
}

