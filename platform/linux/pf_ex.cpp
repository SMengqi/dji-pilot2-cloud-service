/************************************************************
  Copyright (C), Innofidei Inc  2012
  FileName: pl.h
  Author: josephzhou    Version :  1.0   Date: 20121218
  Description:     ƽ̨�ӿ�ʵ��
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2011/12/18         1.0        ����
***********************************************************/

#define THIS_MODULE PLATFORM_EX
#include "pl.h"
#include "os.h"
#include "osport.h"
#include "pf_mbox.h"
#include "pf_timer_service.h"
#include "module.h"
#include "pl_comm.h"
#include "pf_upgrade.h"
#include "pf_daily_record.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> 
#include <net/if.h>  
#include <net/route.h> 
#include <execinfo.h>  
#include <signal.h>
#include <linux/watchdog.h>

#include "pf_stat.h"
#include "pf_crypt.h"
#include "json2pb.h"
#include "thread_cpucore_cfg.pb.h"
#include <sys/statfs.h>
#include <openssl/md5.h> 
#include "pf_rdkafka.h"
#include "pf_thread_mon.h"

#define PC_LINUX

#ifndef PC_LINUX
#include <linux/jiffies.h>
#endif

#define pl_dbg_memory       //pl_dbg


/*���嵱ǰЭ��ջ�̴߳��������������̺߳���Ҫ�޸ĺ궨��*/
#define PS_THREAD_NUMBER                17

static U64 ulTicksStartNs = 0;


extern char acSvnRevision[];
extern char acSvnModified[];
extern char acSvnDate[];
extern char acSvnRange[];
extern char acSvnMixed[];
extern char acSvnUrl[];
extern char acPsVersion[];

extern char acSvnUserName[];
extern char acSvnHostName[];
extern char acSysTime[];


#define MAX_VERSION_LENGTH      256
#define MAX_FILE_PATH_LENGTH    512

CHAR acPlatformVersion[] = "platform_drc-4.2.0_1294";

CHAR acShellVersion[MAX_VERSION_LENGTH];
CHAR acKernelVersion[MAX_VERSION_LENGTH] = {0};

CHAR acRootPath[MAX_VERSION_LENGTH] = ".";

static S32 get_script_version(char *buf);
static S32 get_kernel_version(char *buf);

extern U32 pf_get_thread_mid_num(void);
extern pf_handle_t  workerhandles[];
extern MODULE_ENTRY moduleArray[];

//#define PF_MEMPOOL_STATICS        //��memory pool ͳ��

#define MAX_STATICS_NUM         2
#define MAX_STATICS_OFFSET      1000

extern void stack_occupancy_maximum(void);

U32 g_mempool_warn = 0;
PF_MUTEX_T g_stFileMutex;
FileInfoList g_stFileMapInfo;

U32 pf_mempool_warn(void)
{
    if (g_mempool_warn)
        return TRUE;

    return FALSE;
}

void set_mempool_warn(void)
{
    if (g_mempool_warn)
        return;

    g_mempool_warn = 1;
}

void clear_mempool_warn(void)
{
    if (0 == g_mempool_warn)
        return;

    g_mempool_warn = 0;
}

U32 pf_sysmem_warn(void)
{
    struct sysinfo info;
    if (0 != sysinfo(&info))
    {
        pl_log(ERR, "sysinfo systemcall failed\n");
        return FALSE;
    }

    if (info.freeram < MEMORY_SIZES_LIMITED)
    {
        pl_log(WARN, "system free memory is %d\n", (int)info.freeram);
        return TRUE;
    }

    return FALSE;
}

typedef struct 
{
    U32 ulLenMin;
    U32 ulLenMax;
    U32 ulLength;
    U32 ulInCounts;
    U32 ulOutCounts;
    U32 aulInNums[MAX_STATICS_OFFSET];
}Statics;

Statics asSta[MAX_STATICS_NUM] = {0};

void statics_initial(U32 ulNum, U32 ulLength)
{
    if(ulNum < MAX_STATICS_NUM)
    {
        asSta[ulNum].ulLength = ulLength;
        asSta[ulNum].ulLenMax = 0;
        asSta[ulNum].ulLenMin = 0xFFFFFFFF;
    }
}

void statics_set(U32 ulNum, U32 ulLength)
{
    if(ulNum < MAX_STATICS_NUM)
    {
        U32 ulLen = asSta[ulNum].ulLength;
        if(asSta[ulNum].ulLenMax < ulLength)
        {
            asSta[ulNum].ulLenMax = ulLength;
        }

        if(asSta[ulNum].ulLenMin > ulLength)
        {
            asSta[ulNum].ulLenMin = ulLength;
        }

        if(ulLength >= ulLen*MAX_STATICS_OFFSET)
        {
            asSta[ulNum].ulOutCounts++;
        }
        else
        {
            U32 ulCycle = ulLength/ulLen;
            asSta[ulNum].aulInNums[ulCycle]++;
            asSta[ulNum].ulInCounts++;
        }
    }
}

void static_performance(U32 ulNum)
{
    if(ulNum < MAX_STATICS_NUM)
    {
        Statics* pstStat = &asSta[ulNum];
        U32 ulCounts = pstStat->ulOutCounts + pstStat->ulInCounts;
        U32 i;
        pl_log(WARN, "STATICS:%d,Len:%d,Min:%d,Max:%d,In:%d,Out:%d,Rate:%d",\
            ulNum,                                          \
            pstStat->ulLength,                              \
            pstStat->ulLenMin,                              \
            pstStat->ulLenMax,                              \
            pstStat->ulInCounts,                            \
            pstStat->ulOutCounts,                           \
            pstStat->ulInCounts*100/ulCounts);

        for(i=0; i<MAX_STATICS_OFFSET; i++)
        {
            if(pstStat->aulInNums[i] > 0)
            {
                pl_log(WARN, "ulInNums[%3d]=%5d,Rate:%2d", \
                    i, pstStat->aulInNums[i], pstStat->aulInNums[i]*100/ulCounts);
            }
        }
    }
}

/**********************************************************************************************
 * @API function  pf_show_performance
 * @brief         output of current system statistics
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_show_performance(void)
{
    pl_commbuf_statistic();
    pl_log(INF, "MemoryStatics:            malloc              free           realloc          MsgQSend            memset            memcpy");

    pl_log(INF, "Counts:     %12u(%6u)%12u(%4u)%12u(%4u)%10u(%6u)%10u(%6u)%10u(%6u)",                      \
           PS_CGet(CM_COM, CMCOM_ID_MALLOC_CNT),        PS_CGet(CM_COM, CMCOM_ID_MALLOC_CNT)  - PS_CGet(CM_LCOM, CMCOM_ID_MALLOC_CNT),          \
           PS_CGet(CM_COM, CMCOM_ID_FREE_CNT),          PS_CGet(CM_COM, CMCOM_ID_FREE_CNT)    - PS_CGet(CM_LCOM, CMCOM_ID_FREE_CNT),            \
           PS_CGet(CM_COM, CMCOM_ID_REALLOC_CNT),       PS_CGet(CM_COM, CMCOM_ID_REALLOC_CNT) - PS_CGet(CM_LCOM, CMCOM_ID_REALLOC_CNT),         \
           PS_CGet(CM_COM, CMCOM_ID_COPY_MSG_CNT),      PS_CGet(CM_COM, CMCOM_ID_COPY_MSG_CNT)- PS_CGet(CM_LCOM, CMCOM_ID_COPY_MSG_CNT),        \
           PS_CGet(CM_COM, CMCOM_ID_MEMSET_CNT),        PS_CGet(CM_COM, CMCOM_ID_MEMSET_CNT)  - PS_CGet(CM_LCOM, CMCOM_ID_MEMSET_CNT),          \
           PS_CGet(CM_COM, CMCOM_ID_MEMCPY_CNT),        PS_CGet(CM_COM, CMCOM_ID_MEMCPY_CNT)  - PS_CGet(CM_LCOM, CMCOM_ID_MEMCPY_CNT));
    pl_log(INF, "Sizes:      %12u(%6u)           0(   0)%12u(%4u)%10u(%6u)%10u(%6u)%10u(%6u)",             \
           PS_CGet(CM_COM, CMCOM_ID_MALLOC_SIZE),       PS_CGet(CM_COM, CMCOM_ID_MALLOC_SIZE)       - PS_CGet(CM_LCOM, CMCOM_ID_MALLOC_SIZE),       \
           PS_CGet(CM_COM, CMCOM_ID_REALLOC_SIZE),      PS_CGet(CM_COM, CMCOM_ID_REALLOC_SIZE)      - PS_CGet(CM_LCOM, CMCOM_ID_REALLOC_SIZE),      \
           PS_CGet(CM_COM, CMCOM_ID_COPY_MSG_SIZE_L),   PS_CGet(CM_COM, CMCOM_ID_COPY_MSG_SIZE_L)   - PS_CGet(CM_LCOM, CMCOM_ID_COPY_MSG_SIZE_L),   \
           PS_CGet(CM_COM, CMCOM_ID_MEMSET_SIZE),       PS_CGet(CM_COM, CMCOM_ID_MEMSET_SIZE)       - PS_CGet(CM_LCOM, CMCOM_ID_MEMSET_SIZE),       \
           PS_CGet(CM_COM, CMCOM_ID_MEMCPY_SIZE),       PS_CGet(CM_COM, CMCOM_ID_MEMCPY_SIZE)       - PS_CGet(CM_LCOM, CMCOM_ID_MEMCPY_SIZE));

    pl_log(INF, "FailCounts: %12u(      )%12u(    )%12u(    )%10u(      )",                                \
           PS_CGet(CM_PES, CMPES_ID_COMMBUF_MALLOC_NULL),       \
           PS_CGet(CM_PES, CMPES_ID_COMMBUF_FREE_NULL),         \
           PS_CGet(CM_PES, CMPES_ID_REALLOC_ADDRESS_NULL),      \
           PS_CGet(CM_PES, CMPES_ID_COPY_MSG_IN_FAIL) + PS_CGet(CM_PES, CMPES_ID_COPY_MSGQ_NULL) + PS_CGet(CM_PES, CMPES_ID_COPY_MSGQ_NULL) +\
           PS_CGet(CM_PES, CMPES_ID_MBOX_TRY_PUT_FULL) + PS_CGet(CM_PES, CMPES_ID_MBOX_TRY_PUT_COPY_FULL));

    pl_log(INF, "MemPoolMa: %8d,%8d,ComBufRate: %2d, Free: %4d,%8d",                                       \
            PS_CGet(CM_COM, CMCOM_ID_MALLOC_CNT),                                                           \
            PS_CGet(CM_COM, CMCOM_ID_COMMBUF_MALLOC_CNT),                                                   \
            PS_CGet(CM_COM, CMCOM_ID_COMMBUF_MALLOC_CNT)*100/(PS_CGet(CM_COM, CMCOM_ID_COMMBUF_MALLOC_CNT) + PS_CGet(CM_COM, CMCOM_ID_MALLOC_CNT)),     \
            PS_CGet(CM_COM, CMCOM_ID_FREE_CNT),                                                             \
            PS_CGet(CM_COM, CMCOM_ID_COMMBUF_FREE_CNT));

    pl_log(INF, "logCountS:%8lld(%8lld),logInfoS:%8lld(%8lld),dumpCntS:%8lld(%8lld),%8lld(%8lld)",                         \
            PS_CGet(CM_COM, CMCOM_ID_LOG_TOTAL_CNT),    PS_CGet(CM_COM, CMCOM_ID_LOG_TOTAL_CNT)-PS_CGet(CM_LCOM, CMCOM_ID_LOG_TOTAL_CNT),       \
            PS_CGet(CM_COM, CMCOM_ID_LOG_CNT),          PS_CGet(CM_COM, CMCOM_ID_LOG_CNT)-PS_CGet(CM_LCOM, CMCOM_ID_LOG_CNT),                   \
            PS_CGet(CM_COM, CMCOM_ID_DUMP_TOTAL_CNT),   PS_CGet(CM_COM, CMCOM_ID_DUMP_TOTAL_CNT)-PS_CGet(CM_LCOM, CMCOM_ID_DUMP_TOTAL_CNT),     \
            PS_CGet(CM_COM, CMCOM_ID_DUMP_RET_CNT),     PS_CGet(CM_COM, CMCOM_ID_DUMP_RET_CNT)-PS_CGet(CM_LCOM, CMCOM_ID_DUMP_RET_CNT));

    pl_log(INF, "   pf_log Send CountS:%8lld(%8lld), Failed:%8lld(        ), Return:%8lld(%8lld), Retctrl:%8lld, ReData:%8lld",                          \
            PS_CGet(CM_COM, CMCOM_ID_LOG_SEND_CNT),     PS_CGet(CM_COM, CMCOM_ID_LOG_SEND_CNT)-PS_CGet(CM_LCOM, CMCOM_ID_LOG_SEND_CNT),     \
            PS_CGet(CM_PES, CMPES_ID_PF_LOG_CTRL_FULL_FAIL) + PS_CGet(CM_PES, CMPES_ID_PF_LOG_DATA_FULL_FAIL),                               \
            PS_CGet(CM_COM, CMCOM_ID_LOG_RETURN_CNT),   PS_CGet(CM_COM, CMCOM_ID_LOG_RETURN_CNT)-PS_CGet(CM_LCOM, CMCOM_ID_LOG_RETURN_CNT),   \
            PS_CGet(CM_PES, CMPES_ID_PF_LOG_CTRL_FULL_FAIL),    \
            PS_CGet(CM_PES, CMPES_ID_PF_LOG_DATA_FULL_FAIL));

    pl_log(INF, "PF_MEMFAS:%8lld,%8lld,%8lld,%8lld,%8lld,%8lld,%8lld",       \
            PS_CGet(CM_PES, CMPES_ID_COMMBUF_MALLOC_IN_NULL),   \
            PS_CGet(CM_PES, CMPES_ID_REALLOC_IN_NULL),          \
            PS_CGet(CM_PES, CMPES_ID_MEMCPY_SRC_NULL),          \
            PS_CGet(CM_PES, CMPES_ID_MEMCPY_DST_NULL),          \
            PS_CGet(CM_PES, CMPES_ID_MEMCPY_SIZE_NULL),         \
            PS_CGet(CM_PES, CMPES_ID_MEMSET_ADDRESS_NULL),      \
            PS_CGet(CM_PES, CMPES_ID_MEMSET_SIZE_NULL));

    pl_log(INF, "MsgQSends:%8lld,Recvd:%11lld, Ma:%10lld, Fr:%10lld, Failed: copy %4lld,%4lld,%4lld trycopy:%4lld,%4lld,%4lld mbox:%4lld,%4lld,%4lld,%4lld",               \
            PS_CGet(CM_COM, CMCOM_ID_COPY_MSG_CNT) + PS_CGet(CM_COM, CMCOM_ID_COPY_TRY_MSG_CNT),         \
            PS_CGet(CM_COM, CMCOM_ID_GET_MSG_CNT),                      \
            PS_CGet(CM_COM, CMCOM_ID_COPY_MSG_MALLOC_CNT),              \
            PS_CGet(CM_PES, CMCOM_ID_COPY_MSG_FREE_CNT),                \
            PS_CGet(CM_PES, CMPES_ID_COPY_MSG_IN_FAIL),                 \
            PS_CGet(CM_PES, CMPES_ID_COPY_MSGQ_NULL),                   \
            PS_CGet(CM_PES, CMPES_ID_COPY_MSG_MALLOC_NULL),             \
            PS_CGet(CM_PES, CMPES_ID_COPY_TRY_MSG_IN_FAIL),             \
            PS_CGet(CM_PES, CMPES_ID_COPY_TRY_MSGQ_NULL),               \
            PS_CGet(CM_PES, CMPES_ID_COPY_TRY_MSG_MALLOC_NULL),         \
            PS_CGet(CM_PES, CMPES_ID_MBOX_TRY_PUT_FULL),                \
            PS_CGet(CM_PES, CMPES_ID_MBOX_TRY_PUT_COPY_FULL),           \
            PS_CGet(CM_PES, CMPES_ID_MBOX_TRY_LOCK_PUT_FAIL),           \
            PS_CGet(CM_PES, CMPES_ID_MBOX_TRY_LOCK_PUT_FULL_FAIL));

    pl_log(INF, "SocketSendCnt:%8lld(%8lld),HighSize:%4lld, Size:%10lld(%10lld), Failed send: %4lld,block:%4lld",     \
            PS_CGet(CM_COM, CMCOM_ID_SEND_TCP_CNT),                                                     \
            PS_CGet(CM_COM, CMCOM_ID_SEND_TCP_CNT) - PS_CGet(CM_LCOM, CMCOM_ID_SEND_TCP_CNT),           \
            PS_CGet(CM_COM, CMCOM_ID_SEND_TCP_SIZES_H),                                                 \
            PS_CGet(CM_COM, CMCOM_ID_SEND_TCP_SIZES_L),                                                 \
            PS_CGet(CM_COM, CMCOM_ID_SEND_TCP_SIZES_L) - PS_CGet(CM_LCOM, CMCOM_ID_SEND_TCP_SIZES_L),   \
            PS_CGet(CM_NES, CMNES_ID_NETLIB_SEND_FAIL),                                                 \
            PS_CGet(CM_NES, CMNES_ID_NETLIB_SEND_BLOCK_FAIL));
    pl_log(FATAL, "TIMER CopyErr:%6lld, AllocErr:%6lld, CommonMallocErr:%6lld, %6lld, LogFullErr: %6lld,%6lld, MboxFullErr:%6lld(msgId:%5lld, moduleId:%3lld) MboxMaxSizes:%6lld, NetSendBlock:%6lld, SendErr:%6lld, KafkaProducer HeaderErr:%6lld,SendErr:%6lld,QueueErr:%6lld, ConsumerErr:%6lld, AuthFlag:%2lld, AuthErr:%4lld", \
            PS_CGet(CM_PES, CMPES_ID_TIMER_TM_EXPIRE_COPY_FAIL),            \
            PS_CGet(CM_PES, CMPES_ID_TIMER_TM_ALLOC_FAIL),                  \
            PS_CGet(CM_PES, CMPES_ID_COMMBUF_GET_ADDRESS_FAIL),             \
            PS_CGet(CM_PES, CMPES_ID_COMMBUF_GET_FAIL),                     \
            PS_CGet(CM_PES, CMPES_ID_PF_LOG_CTRL_FULL_FAIL),                \
            PS_CGet(CM_PES, CMPES_ID_PF_LOG_DATA_FULL_FAIL),                \
            PS_CGet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_FAIL),           \
            PS_CGet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_MSGID),          \
            PS_CGet(CM_PES, CMPES_ID_MBOX_PUT_FULL_WAITING_DEST_MODULEID),  \
            PS_CGet(CM_PES, CMPES_ID_MBOX_PUT_MAX_SIZE),                    \
            PS_CGet(CM_NES, CMNES_ID_NETLIB_SEND_BLOCK_FAIL),               \
            PS_CGet(CM_NES, CMNES_ID_NETLIB_SEND_FAIL),                     \
            PS_CGet(CM_PES, CMPES_ID_RDKAFKA_PRODUCER_HEADER_FAIL),         \ 
            PS_CGet(CM_PES, CMPES_ID_RDKAFKA_PRODUCER_SEND_FAIL),           \
            PS_CGet(CM_PES, CMPES_ID_RDKAFKA_PRODUCER_QUEUE_FULL_FAIL),     \
            PS_CGet(CM_PES, CMPES_ID_RDKAFKA_COSUMER_THREAD_FAIL),          \
            PS_CGet(CM_PES, CMPES_ID_AUTH_CHECK_CNT),                       \
            PS_CGet(CM_PES, CMPES_ID_AUTH_CHECK_FAIL));


    for(int i = 0; i < MODULE_MAX; i++)
    {
        pl_log(UINF, "MOUDLE:%10s MsgQRcv:%8lld(%6lld), MsgQSend:%8lld(%6lld),MboxQueueSize:%8lld(%6lld),MsgQSendFail:%8lld(%6lld)", pf_get_module_name(i),             \
                PS_CGet(CM_RMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + i)),                                                       	    \
                PS_CGet(CM_RMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + i)) - PS_CGet(CM_LRMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + i)),   \
                PS_CGet(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + i)),                                                             \
                PS_CGet(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + i))-PS_CGet(CM_LSMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + i)),     \
                PS_CGet(CM_SMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + i))-PS_CGet(CM_RMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + i)),      \
                PS_CGet(CM_LSMSG, (CMSMSG_ID_SEND_MSGQ_MID_OFFSET + i))-PS_CGet(CM_LRMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + i)),    \
                PS_CGet(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + i)),                                                      \
                PS_CGet(CM_FMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + i)) - PS_CGet(CM_LFMSG, (CMFMSG_ID_SEND_FAILED_MSGQ_MID_OFFSET + i)));
    }

    if(pf_thread_mon_interval_debug_is_open())
    {
        for(int i = 0; i < MODULE_MAX; i++)
        {
            if(PS_CGet(CM_RMSG, i))    
		    {
                pl_log(INF, "MOUDLE:%10s MsgQRcv:%8lld, total:%8lld,Max:%8lld(ID:%6lld),average:%6dns", pf_get_module_name(i),             \
                    PS_CGet(CM_RMSG, i),                                                       	    \
                    PS_CGet(CM_MTT, i),                                                       	    \
                    PS_CGet(CM_MMAX, i),   \
                    PS_CGet(CM_MMID, i),   \
                    PS_CGet(CM_MTT, i)/PS_CGet(CM_RMSG, i));
            }
        }
    }

    pf_print_stack_stat();

#ifdef PF_MEMPOOL_STATICS
    static_performance(0);
#endif

    pf_print_timer_stat();
	
	pf_producer_rdkafka_throughput();
    pf_consumer_rdkafka_throughput();

}

#if DRC_PROCESS
extern CHAR *pf_get_fusion_version(void);
#endif
/**********************************************************************************************
 * @API function  pf_get_sc_version
 * @brief         Get version information of the current system
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_get_sc_version(void)
{
    char outString[2048] ="\r\n";
    S32 ret;
    sprintf(outString, "%sacSvnRevision: %s\r\n", outString, acSvnRevision);
    sprintf(outString, "%sacSvnModified: %s\r\n", outString, acSvnModified);
    sprintf(outString, "%sacSvnDate    : %s\r\n", outString, acSvnDate);
    sprintf(outString, "%sacSvnRange   : %s\r\n", outString, acSvnRange);
    sprintf(outString, "%sacSvnMixed   : %s\r\n", outString, acSvnMixed);
    sprintf(outString, "%sacSvnUrl     : %s\r\n", outString, acSvnUrl);
    sprintf(outString, "%sacPsVersion  : %s\r\n", outString, acPsVersion);
    sprintf(outString, "%sacPlatformVer: %s\r\n", outString, acPlatformVersion);

    sprintf(outString, "%sacSvnUserName: %s\r\n", outString, acSvnUserName);
    sprintf(outString, "%sacSvnHostName: %s\r\n", outString, acSvnHostName);
    sprintf(outString, "%sacSysTime    : %s\r\n", outString, acSysTime);

    ret = get_script_version(acShellVersion);
    if (ret < 0)
    {
        pl_log(ERR, "get_script_version fails, ret = %d\n", ret);
    }

    sprintf(outString, "%sacShellVersion    : %s\r\n", outString, acShellVersion);
    
    ret = get_kernel_version(acKernelVersion);
    if (ret < 0)
    {
        pl_log(ERR, "get_kernel_version fails, ret = %d\n", ret);
    }
    sprintf(outString, "%sacKernelVersion    : %s\r\n", outString, acKernelVersion);

#if DRC_PROCESS
//    sprintf(outString, "%sacFutionVer  : %s\r\n", outString, pf_get_fusion_version());
#endif

    pl_log(DO_NOT_USE, "%s", outString);
//    printf("\nTHe module version info is:\n%s\n", outString);


}


/**********************************************************************************************
 * @API function  pf_memcpy
 * @brief         copy memory from source address to destination address
 * @input         pDst            the destination address
                  pSrc            the source address
                  ulLength        the copy length
 * @output        void 
 * @return        the destination address
 *********************************************************************************************/
extern "C" void* pf_memcpy(void*pDst, const void*pSrc, U32 ulLength)
{
    if((ulLength==0) || (pDst==NULL) || (pSrc==NULL))
    {
        if(!pSrc)
        {
            PS_CPlus(CM_PES, CMPES_ID_MEMCPY_SRC_NULL);
        }
        if(!pDst)
        {
            PS_CPlus(CM_PES, CMPES_ID_MEMCPY_DST_NULL);
        }
        if(!ulLength)
        {
            PS_CPlus(CM_PES, CMPES_ID_MEMCPY_SIZE_NULL);
        }

        //pl_dbg("%s:pf_memcpy parameter failed:pDst=0x%x, pSrc=0x%x, ulLength=%d\n",__FUNCTION__,pDst,pSrc,ulLength);
        return NULL;
    }

    PS_CPlus(CM_COM, CMCOM_ID_MEMCPY_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_MEMCPY_SIZE, ulLength);

    return memcpy(pDst, pSrc, ulLength);
}

/**********************************************************************************************
 * @API function  pf_memset
 * @brief         Set all content of the memory to the specified value
 * @input         pData           the destination address
                  ucVal           the value of memory
                  ulLength        the assignment length
 * @output        void   
 * @return        the destination address    
 *********************************************************************************************/
extern "C" void* pf_memset(void*pData, U8 ucVal, U32 ulLength)
{
    if((ulLength==0) || (pData==NULL))
    {
        if(!pData)
        {
            PS_CPlus(CM_PES, CMPES_ID_MEMSET_ADDRESS_NULL);
        }
        
        if(!ulLength)
        {
            PS_CPlus(CM_PES, CMPES_ID_MEMSET_SIZE_NULL);
        }
        
        pl_dbg("%s:pf_memset parameter failed:pData=0x%x, ucVal=%d, ulLength=%d\n",__FUNCTION__,pData,ucVal,ulLength);
        return NULL;
    }

    PS_CPlus(CM_COM, CMCOM_ID_MEMSET_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_MEMSET_SIZE, ulLength);

    return memset(pData, ucVal, ulLength);
}

#ifndef MEMORY_IN_MEMPOOL

/**********************************************************************************************
 * @API function  pf_malloc
 * @brief         alloc dynamic memory
 * @input         ulSzie          the length of dynamic memory
 * @output        void
 * @return        the address of alloc memory
 *********************************************************************************************/
extern "C" void* pf_malloc(U32 ulSize)
{
    if(ulSize)
    {
        PS_CPlus(CM_COM, CMCOM_ID_MALLOC_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_MALLOC_SIZE, ulSize);

        void* pAddr = malloc(ulSize);
        //pl_dbg_memory("%s:0x%x\n", __FUNCTION__, (U32)pAddr);
        if(NULL == pAddr)
        {
            PS_CPlus(CM_PES, CMPES_ID_MALLOC_ADDRESS_NULL);
            return NULL;
        }

        return pAddr;
    }
    else
    {
        PS_CPlus(CM_PES, CMPES_ID_MALLOC_IN_NULL);
        //pl_dbg("%s:failed Size %d\n", __FUNCTION__, ulSize);
    }
    
    return 0;
}


/**********************************************************************************************
 * @API function  pf_realloc
 * @brief         realloc dynamic memory
 * @input         pSrcAddr        the address of realloc dynamic memory  
                  ulSize          the length of realloc dynamic memory  
 * @output        void
 * @return        the address of realloc memory
 *********************************************************************************************/
extern "C" void* pf_realloc(void* pSrcAddr, U32 ulSize)
{
    if((0 == ulSize) || (NULL == pSrcAddr))
    {
        PS_CPlus(CM_PES, CMPES_ID_REALLOC_IN_NULL);
        return NULL;
    }

    void* pData = realloc(pSrcAddr, ulSize);
    if(NULL == pData)
    {
        PS_CPlus(CM_PES, CMPES_ID_REALLOC_ADDRESS_NULL);
        return NULL;
    }
    else
    {
        PS_CPlus(CM_COM, CMCOM_ID_REALLOC_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_REALLOC_SIZE, ulSize);
        return pData;
    }
}

/**********************************************************************************************
 * @API function  pf_free
 * @brief         free dynamic memory
 * @input         pBuff           the free address of memory 
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_free(void* pBuff)
{
    if(pBuff)
    {
        PS_CPlus(CM_COM, CMCOM_ID_FREE_CNT);

        free(pBuff);
    }
    else
    {
        PS_CPlus(CM_PES, CMPES_ID_FREE_IN_NULL);
    }
}
#else

BUF_CTRL_S* pBufMem[MAX_MEMPOOL_NUMBER] = {0};
PF_MUTEX_T qlock_mem[MAX_MEMPOOL_NUMBER];
U8* pucMemPoolStart = NULL;
U8* pucMemPoolEnd = NULL;

U32 ulBlockPoolUnit = 0;
U32 aulBlockSize[MAX_MEMPOOL_NUMBER] = {   256,  1024, 1536, 4096, 8192, 64000};
U32 aulBlockNum[MAX_MEMPOOL_NUMBER]  = { 48000, 12000, 8000, 3000, 1500,   192};


U64 aulPoolAddrBegin[MAX_MEMPOOL_NUMBER];
U64 aulPoolAddrEnd[MAX_MEMPOOL_NUMBER];

U32 aulMallocBufOffset[] = {            \
    0,1,1,1,2,2,3,3,3,3,3,3,3,3,3,3,    \
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,    \
    5,5,5,5,5,5,5,5,5,5
};

/**********************************************************************************************
* @API function  pf_mem_pool_init
* @brief         memory pool initial function
* @input         void
* @output        void
* @return        0      - success
                 other  - failure
*********************************************************************************************/
U32 pf_mem_pool_init(void)
{
    U32 i;
    U32 ulTotalPoolSize;
    U32 ulCurPoolSize;
    U8* pucBufStart = NULL;

    ulTotalPoolSize = 0;

    for(i=0; i<MAX_MEMPOOL_NUMBER; i++)
    {
        ulTotalPoolSize += (aulBlockSize[i] * aulBlockNum[i]);
    }

    pucMemPoolStart = (U8*)malloc(ulTotalPoolSize);
    if(NULL == pucMemPoolStart)
    {
        ASSERT(0);
        pl_log(ERR, "%s MALLOC NULL", __FUNCTION__);
        PS_CPlus(CM_PES, CMPES_ID_POOL_INIT_MALLOC_NULL);
        return 1;        
    }

    memset(pucMemPoolStart, 0, ulTotalPoolSize);
    pucMemPoolEnd = pucMemPoolStart + ulTotalPoolSize;
    pl_log(INF, "%s MALLOC ADDRS 0x%x, ADDRE 0x%x, ulTotalPoolSize=%d\n", __FUNCTION__, pucMemPoolStart, pucMemPoolEnd, ulTotalPoolSize);

    pucBufStart = pucMemPoolStart;
    for(i=0; i<MAX_MEMPOOL_NUMBER; i++)
    {   
        char acName[20];
        sprintf(acName, "MemPool%d", i);
        ulCurPoolSize = aulBlockSize[i]*aulBlockNum[i];
        pBufMem[i] = pl_commbuf_create_with_address(pucBufStart, pucBufStart+ulCurPoolSize, aulBlockSize[i], aulBlockNum[i], acName);
        if(NULL == pBufMem[i])
        {
            ASSERT(0);
            pl_log(ERR, "%s commbuf Created %d Failed", __FUNCTION__, i);
            PS_CPlus(CM_PES, CMPES_ID_POOL_INIT_CREATE_NULL);
            return 1;        
        }

        aulPoolAddrBegin[i] = (U64)pucBufStart;
        aulPoolAddrEnd[i] = (U64)pucBufStart + ulCurPoolSize;

        pl_log(WARN, "%s commbuf Created %d BStart:0x%x, BEnd:0x%x, ulCurPoolSize=%d, ulBlockSize=%d, ulBlockNum=%d\n", __FUNCTION__, i, \
                aulPoolAddrBegin[i],                     \
                aulPoolAddrEnd[i],                       \
                ulCurPoolSize,                          \
                aulBlockSize[i],                        \
                aulBlockNum[i]);

        pucBufStart += ulCurPoolSize;

        ASSERT(ulCurPoolSize ==(aulBlockSize[0]*aulBlockNum[0]));

        PF_MUTEX_INIT(&qlock_mem[i]);

        pl_commbuf_mutex_init(pBufMem[i], &qlock_mem[i]);
    }

    ulBlockPoolUnit = aulBlockSize[0]*aulBlockNum[0];
    pl_log(DO_NOT_USE, "%s commbuf ulBlockPoolUnit %d, pucMemPoolStart=0x%x, pucMemPoolEnd=0x%x\n", __FUNCTION__, ulBlockPoolUnit, pucMemPoolStart, pucMemPoolEnd);

    pf_memset(&asSta[0], 0, sizeof(asSta));
#ifdef PF_MEMPOOL_STATICS
    statics_initial(0, 16);
#endif

    PF_MUTEX_INIT(&g_stFileMutex);

    return 0;
}


U32 pf_meminfo(MEMPOOL_INFO *info)
{
    int i = 0;
    if (NULL == info)
    {
        pl_log(ERR, "mempool info was NULL\n");
        return 1;
    }

    for(i = 0; i < MAX_MEMPOOL_NUMBER; i++)
    {
        info[i].size = aulBlockSize[i];
        info[i].total = aulBlockNum[i];
        info[i].avail = pl_commbuf_avail_num(pBufMem[i]);
        if (U32_INFINITY == info[i].avail)
            info[i].avail = 0;
    }

    if (TRUE == pf_mempool_warn())
        if (info[MAX_MEMPOOL_NUMBER -1].avail >= 10 )
            clear_mempool_warn();

    return 0;
}

/**********************************************************************************************
 * @API function  pf_malloc
 * @brief         alloc dynamic memory
 * @input         ulSzie          the length of dynamic memory
 * @output        void
 * @return        the address of alloc memory
 *********************************************************************************************/
extern "C" void* pf_malloc(U32 ulSize)
{
#ifdef PF_MEMPOOL_STATICS
    statics_set(0, ulSize);
#endif

    if(ulSize > aulBlockSize[MAX_MEMPOOL_NUMBER-1])
    {
        void* pAddr = pl_commbuf_malloc(ulSize);

        //pl_log(WARN, "%s ADDR:0x%x, ulSize=%d Length EXCEED MAX %d \n", __FUNCTION__, (U32)pAddr, ulSize, aulBlockSize[MAX_MEMPOOL_NUMBER-1]);

        return pAddr;
    }
    else if(ulSize > 0)
    {
        /* size is a multiple of MAX_MEMPOOL_BIT_OFFSET. 
        If it is an integer multiple and the same as poolsize, 
        it should be recorded in the poolsize corresponding pool.*/
        U32 ulSizeOffset = (ulSize - 1) >> MAX_MEMPOOL_BIT_OFFSET;  
        U32 ulMaxOffset = sizeof(aulMallocBufOffset)/sizeof(U32);
        U32 ulOffset = aulMallocBufOffset[ulSizeOffset];
        U32 i;
        void* pAddr = NULL;
        //ASSERT(ulSizeOffset < ulMaxOffset);

        for(i=ulOffset; i<MAX_MEMPOOL_NUMBER; i++)
        {
            pAddr = pl_commbuf_get(pBufMem[i]);

            /*If pAddr is not empty, apply for available nodes in the next heap pool*/
            if(NULL != pAddr)
            {
                //pl_log(INF, "%s pAddr=0x%x, ulSize=%d, ulOffset=%d, pMemOffset=%d\n", __FUNCTION__, pAddr, ulSize, ulOffset, i);
                return pAddr;
            }
        }

        if ( FALSE == pf_mempool_warn())
            set_mempool_warn();

        /*When all addresses are full, dynamic memory is requested*/
        pAddr = pl_commbuf_malloc(ulSize); 
        if(NULL == pAddr)
        {
            //pl_log(ERR, "%s Malloc Failed NULL ulSize=%d, ulOffset=%d, pMemOffset=%d\n", __FUNCTION__, ulSize, ulOffset, i);
            PS_CPlus(CM_PES, CMPES_ID_MALLOC_ADDRESS_NULL);
            return NULL;
        }
        else
        {
            //pl_log(WARN, "%s Malloc ADDR:0x%x, ulSize=%d, ulOffset=%d, pMemOffset=%d\n", __FUNCTION__, (U32)pAddr, ulSize, ulOffset, i);
            return pAddr;
        }

    }
    else
    {
        PS_CPlus(CM_PES, CMPES_ID_MALLOC_IN_NULL);
        return NULL;
    }
}


/**********************************************************************************************
 * @API function  pf_realloc
 * @brief         realloc dynamic memory
 * @input         pSrcAddr        the address of realloc dynamic memory  
                  ulSize          the length of realloc dynamic memory  
 * @output        void
 * @return        the address of realloc memory
 *********************************************************************************************/
extern "C" void* pf_realloc(void* pSrcAddr, U32 ulSize)
{
    U64 ullSrcAddr = (U64)pSrcAddr;

    if((0 == ulSize) || (NULL == pSrcAddr))
    {
        PS_CPlus(CM_PES, CMPES_ID_REALLOC_IN_NULL);
        return NULL;
    }

    if((ullSrcAddr >= (U64)pucMemPoolStart) && (ullSrcAddr < (U64)pucMemPoolEnd))
    {
        U64 ullBuffAddrOffset = ullSrcAddr - (U64)pucMemPoolStart;
        U64 ullBufOffset = ullBuffAddrOffset/ulBlockPoolUnit;
        U32 ulRet;

        ASSERT(ullBufOffset < MAX_MEMPOOL_NUMBER);
        ASSERT(0 == (ullBuffAddrOffset % (1 << MAX_MEMPOOL_BIT_OFFSET)));

        /*Length in heap space, no need to re-apply node*/
        if(ulSize <= aulBlockSize[ullBufOffset])
        {
            //pl_log(INF, "%s:DO NOT MALLOC ulSrcAddr:0x%x, ulBuffAddrOffset=%d, ulBufOffset=%d\n", __FUNCTION__, (U32)ulSrcAddr, ulBuffAddrOffset, ulBufOffset);
            PS_CPlus(CM_COM, CMCOM_ID_REALLOC_CNT);
            PS_CPlusV(CM_COM, CMCOM_ID_REALLOC_SIZE, ulSize);

            return pSrcAddr;
        }
        else
        {
            void* pData = pf_malloc(ulSize);
            if(NULL == pData)
            {
                //pl_log(ERR, "%s:REALLOC ERR ulSize:%d, pSrcAddr:0x%x, ulBuffAddrOffset=%d, ulBufOffset=%d\n", __FUNCTION__, ulSize, (U32)pSrcAddr, ulBuffAddrOffset, ulBufOffset);
                PS_CPlus(CM_PES, CMPES_ID_REALLOC_MALLOC_NULL);
                return NULL;
            }
            else
            {
                pf_memcpy(pData, pSrcAddr, aulBlockSize[ullBufOffset]);

                /*Release the original address*/
                ulRet = pl_commbuf_ret(pBufMem[ullBufOffset], pSrcAddr);
                ASSERT(0 == ulRet);

                //pl_log(INF, "%s:NEED MALLOC pData:0x%x,pSrcAddr:0x%x ulBuffAddrOffset=%d, ulBufOffset=%d\n", __FUNCTION__, (U32)pData, (U32)pSrcAddr, ulBuffAddrOffset, ulBufOffset);
                PS_CPlus(CM_COM, CMCOM_ID_REALLOC_CNT);
                PS_CPlusV(CM_COM, CMCOM_ID_REALLOC_SIZE, ulSize);
              
                return pData;
            }
        }
    }
    /*realloc address*/
    else
    {
        void* pData = realloc(pSrcAddr, ulSize);
        if(NULL == pData)
        {
            PS_CPlus(CM_PES, CMPES_ID_REALLOC_ADDRESS_NULL);
            return NULL;
        }
        else
        {
            PS_CPlus(CM_COM, CMCOM_ID_REALLOC_CNT);
            PS_CPlusV(CM_COM, CMCOM_ID_REALLOC_SIZE, ulSize);
            return pData;
        }
    }

}


/**********************************************************************************************
 * @API function  pf_free
 * @brief         free dynamic memory
 * @input         pBuff           the free address of memory 
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_free(void* pBuff)
{
    if((pBuff < pucMemPoolEnd) && (pBuff >= pucMemPoolStart))
    {
        U64 ulBuffAddrOffset = (U64)pBuff - (U64)pucMemPoolStart;
        U64 ulBufOffset = ulBuffAddrOffset/ulBlockPoolUnit;
        U32 ulRet;

        ASSERT(ulBufOffset < MAX_MEMPOOL_NUMBER);
        ASSERT(0 == ((U64)ulBuffAddrOffset % (1 << MAX_MEMPOOL_BIT_OFFSET)));
        
        ulRet = pl_commbuf_ret(pBufMem[ulBufOffset], pBuff);
        //ASSERT(0 == ulRet);

        //pl_log(INF, "%s:0x%x, ulBuffAddrOffset=%d, ulBufOffset=%d\n", __FUNCTION__, (U32)pBuff, ulBuffAddrOffset, ulBufOffset);
    }
    else if(pBuff)
    {
        pl_commbuf_free(pBuff);
        //pl_log(WARN, "%s:0x%x\n", __FUNCTION__, (U32)pBuff);
        return;
    }
    else
    {
        PS_CPlus(CM_PES, CMPES_ID_FREE_IN_NULL);
        return;
    }
}

#endif

/**********************************************************************************************
 * @API function  pf_ticks_init
 * @brief         get base time.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_ticks_init(void)
{
    timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    ulTicksStartNs = ((U64)tp.tv_sec * 1000000000) + (U64)tp.tv_nsec;
}

/**********************************************************************************************
 * @API function  pf_get_ticks_ms
 * @brief         get system time, the unit is milliseconds.
 * @input         void
 * @output        void
 * @return        system time
 *********************************************************************************************/
extern "C" U64 pf_get_ticks_ms(void)
{
    U64 u64ticks;
    timespec tp;

    if(clock_gettime(CLOCK_MONOTONIC, &tp))
    {
        PS_CPlus(CM_PES, CMPES_ID_TICKS_MS_FAIL);
    } 
    else
    {
        PS_CPlus(CM_COM, CMCOM_ID_GET_TICKS_MS_CNT);
    }

    u64ticks = (((U64)(tp.tv_sec) * 1000000000) + ((U64)(tp.tv_nsec) - ulTicksStartNs))/1000000;
    
    return u64ticks;
}

/**********************************************************************************************
 * @API function  pf_get_ticks_us
 * @brief         get system time, the unit is microseconds.
 * @input         void
 * @output        void
 * @return        system time
 *********************************************************************************************/
extern "C" U64 pf_get_ticks_us(void)
{
    U64 u64ticks;
    timespec tp;

    if(clock_gettime(CLOCK_MONOTONIC, &tp))
    {
        PS_CPlus(CM_PES, CMPES_ID_TICKS_US_FAIL);
    } 
    else
    {
        PS_CPlus(CM_COM, CMCOM_ID_GET_TICKS_US_CNT);
    }
    
    u64ticks = (((U64)tp.tv_sec * 1000000000) + ((U64)tp.tv_nsec - ulTicksStartNs))/1000;
    
    return u64ticks;
}


/**********************************************************************************************
 * @API function  pf_get_ticks_ns
 * @brief         get system time, the unit is nanoseconds.
 * @input         void
 * @output        void
 * @return        system time
 *********************************************************************************************/
extern "C" U64 pf_get_ticks_ns(void)
{
    U64 u64ticks;
    timespec tp;

    if(clock_gettime(CLOCK_MONOTONIC, &tp))
    {
        PS_CPlus(CM_PES, CMPES_ID_TICKS_NS_FAIL);
    }
    else
    {
        PS_CPlus(CM_COM, CMCOM_ID_GET_TICKS_NS_CNT);
    }

    u64ticks = ((U64)tp.tv_sec * 1000000000) + (U64)tp.tv_nsec - ulTicksStartNs;
    
    return u64ticks;
}

/**********************************************************************************************
 * @API function  pf_get_frame_time
 * @brief         get frame and subframe of system time
 * @input         void
 * @output        pulFrameTime      U32 Output frame time
                  pulSubFrameTime   U32 Output subframe time
 * @return        0      - success
                  other  - failure
 *********************************************************************************************/
extern U32 pf_get_frame_time(U32* pulFrameTime, U32* pulSubFrameTime)
{
    struct timeval tv;

    if(gettimeofday(&tv, NULL))
    {
        PS_CPlus(CM_PES, CMPES_ID_GET_FRAME_TIME_FAIL);
        return PF_RET_FAILURE;
    }
#ifdef PF_FRAME_PERIOD_SIZE
    *pulFrameTime = tv.tv_sec % PF_FRAME_PERIOD_SIZE;
#else
    *pulFrameTime = tv.tv_sec;
#endif
    *pulSubFrameTime = tv.tv_usec / 100000;
    
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_get_timeofday
 * @brief         Get the current exact time (UNIX to the present time)
 * @input         void
 * @output        struct  timeval* tv,
                  tv_sec            S32 The time obtained is accurate to seconds.
                  tv_usec           S32 The time obtained is accurate to microseconds��1e-6 s)
                  struct  timezone* tz
                  tz_minuteswest    S32 Number of Clocks Differential from Greenwich Time
                  tz_dsttime        S32 Summer Time Correction Type
 * @return        0      - success
                  other  - failure
 *********************************************************************************************/
extern "C" S32 pf_get_timeofday(struct timeval *tv, struct timezone *tz)
{
    S32 ret = gettimeofday(tv, tz);
    if(ret)
    {
        PS_CPlus(CM_PES, CMPES_ID_GET_TIMEOFDAY_FAIL);
    } 

    return ret;
}

/**********************************************************************************************
 * @API function  pf_usleep
 * @brief         Delay Interface at Microsecond Level
 * @input         ulUs          delayed microseconds
 * @output        void
 * @return        0      - success
                  other  - failure
 *********************************************************************************************/
extern "C" U32 pf_usleep(U32 ulUs)
{
    struct timespec interval;
    U32 ulTime_sec = ulUs/1000000;
    U32 ulTime_nsec = (ulUs%1000000) * 1000;
    
    int ret;

    PS_CPlus(CM_COM, CMCOM_ID_USLEEP_TOTAL_CNT);
    if (clock_gettime(CLOCK_REALTIME, &interval) < 0)
    {
        PS_CPlus(CM_PES, CMPES_ID_USLEEP_GET_FAIL);
    }

    ulTime_nsec += interval.tv_nsec;
    if (ulTime_nsec > 999999999 )
    {
        interval.tv_nsec = ulTime_nsec - 1000000000;
        interval.tv_sec += 1 + ulTime_sec;
    }
    else
    {
        interval.tv_nsec = ulTime_nsec;
        interval.tv_sec += ulTime_sec;
    }

    /* Implement DELAY sleep */
    ret = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &interval, NULL);

    if (0 != ret)
    {
        PS_CPlus(CM_PES, CMPES_ID_USLEEP_NANO_FAIL);
        return ret;
    }

    return ret;
}


/**********************************************************************************************
 * @API function  pf_get_big_endian_flag
 * @brief         Get the byte order of the current system, big-endian or little-endian
 * @input         void
 * @output        void
 * @return        FALSE  - little endian
                  TRUE   - big endian
 *********************************************************************************************/
extern "C" BOOL pf_get_big_endian_flag(void)
{
   int c = 1;
   if ((*(char *)&c) == 1)  
   {
       return FALSE;
   }
   else
   {
       return TRUE;
   }
}



void display_pthread_attr(pthread_attr_t *attr, char *prefix)
{
    int ret, i;
    size_t v;
    void *stkaddr;
    struct sched_param sp;

    ret = pthread_attr_getdetachstate(attr, &i);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_getdetachstate ret=%d", ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_ATTR_FAIL);
    }

    pl_log(INF,"%sDetach state        = %s", prefix,
            (i == PTHREAD_CREATE_DETACHED) ? ((char*)"PTHREAD_CREATE_DETACHED") :
            (i == PTHREAD_CREATE_JOINABLE) ? ((char*)"PTHREAD_CREATE_JOINABLE") :
            ((char*)"UNKNOWN"));

    ret = pthread_attr_getscope(attr, &i);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_getscope ret=%d", ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_ATTR_FAIL);
    }

    pl_log(INF,"%sScope               = %s", prefix,
           (i == PTHREAD_SCOPE_SYSTEM)  ? ((char*)"PTHREAD_SCOPE_SYSTEM") :
           (i == PTHREAD_SCOPE_PROCESS) ? ((char*)"PTHREAD_SCOPE_PROCESS") :
           ((char*)"UNKNOWN"));

    ret = pthread_attr_getinheritsched(attr, &i);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_getinheritsched ret=%d", ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_ATTR_FAIL);
    }
   
    pl_log(INF,"%sInherit scheduler   = %s", prefix,
            (i == PTHREAD_INHERIT_SCHED)  ? ((char*)"PTHREAD_INHERIT_SCHED") :
            (i == PTHREAD_EXPLICIT_SCHED) ? ((char*)"PTHREAD_EXPLICIT_SCHED") :
            ((char*)"UNKNOWN"));

    ret = pthread_attr_getschedpolicy(attr, &i);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_getschedpolicy ret=%d", ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_ATTR_FAIL);
    }

    pl_log(INF,"%sScheduling policy   = %s", prefix,
            (i == SCHED_OTHER) ? ((char*)"SCHED_OTHER") :
            (i == SCHED_FIFO)  ? ((char*)"SCHED_FIFO") :
            (i == SCHED_RR)    ? ((char*)"SCHED_RR") :
           ((char*)"UNKNOWN"));

    ret = pthread_attr_getschedparam(attr, &sp);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_getschedparam ret=%d", ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_ATTR_FAIL);
    }

    pl_log(INF,"%sScheduling priority = %d", prefix, sp.sched_priority);

    ret = pthread_attr_getguardsize(attr, &v);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_getguardsize ret=%d", ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_ATTR_FAIL);
    }

    pl_log(INF,"%sGuard size          = %d bytes", prefix, v);

    ret = pthread_attr_getstack(attr, &stkaddr, &v);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_getstack ret=%d", ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_ATTR_FAIL);
    }

    pl_log(INF,"%sStack address       = %p", prefix, stkaddr);
    pl_log(INF,"%sStack size          = 0x%x bytes", prefix, v);
}

void pf_thread_create(
        pf_addrword_t       sched_info,             /* scheduling info (eg pri)  */
        pf_thread_entry_t   *entry,                 /* entry point function      */
        pf_addrword_t       entry_data,             /* entry data                */
        char                *name,                  /* optional thread name      */
        void                *stack_base,            /* stack base, NULL = alloc  */
        unsigned int        stack_size,             /* stack size, 0 = default   */
        pf_handle_t         *handle,                /* returned thread handle    */
        pf_thread_t         *thread                 /* put thread here           */
        ) 
{
    pthread_attr_t attr;
    sched_param param; 
    int ret;

#if DRC_PROCESS
    PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_CNT);
    if(PF_RET_SUCCESS != pf_check_auth_state())
    {
        PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_FAIL);
    }
#endif

#if UNIT_TEST
    PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_CNT);
    if(PF_RET_SUCCESS != pf_check_auth_state())
    {
        PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_FAIL);
    }
#endif

    PS_CPlus(CM_COM, CMCOM_ID_THREAD_CREATE_TOTAL_CNT);
    ret = pthread_attr_init(&attr);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_init ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }

    pf_memset(stack_base, 0x5A, stack_size);

//#ifndef RUN_ON_PC
#if 0
    if(0 != sched_info)
    {
        ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (0 != ret)
        {
            pl_log(ERR, "pthread_attr_setdetachstate ret=%d\n",ret);
            PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
        }

        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); // ʵʱlinux�����ô��ֵ��ȷ�ʽ
        if (0 != ret)
        {
            pl_log(ERR, "pthread_attr_setinheritsched ret=%d\n",ret);
            PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
        }

        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);  
        if (0 != ret)
        {
            pl_log(ERR, "pthread_attr_setscope ret=%d\n",ret);
            PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
        }
        dbgline;

        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);  
        if (0 != ret)
        {
            pl_log(ERR, "pthread_attr_setschedpolicy ret=%d\n",ret);
            PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
        }

        param.sched_priority = sched_info;  
        ret = pthread_attr_setschedparam(&attr, &param);  
        if (0 != ret)
        {
            pl_log(ERR, "pthread_attr_setschedparam ret=%d\n",ret);
            PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
        }
    }

    pl_log(ERR, "pthread_create stack_base=0x%x, stack_size=0x%x\n",stack_base,stack_size);
    ret = pthread_attr_setstacksize(&attr, stack_size);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_setstacksize ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }

    ret = pthread_attr_setstack(&attr, stack_base, stack_size);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_setstack ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }
#endif

    ret = pthread_create(handle, &attr, (void*(*)(void*))entry,(void*)entry_data);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_create error ret=%d",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }


    ret = pthread_setname_np(*handle, name);
    if (0 != ret)
    {
        printf("pthread_setname_np error ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }
    
    pf_stat_set_thread_name(name);

    display_pthread_attr(&attr, name);
    printf("\r\npf_thread_create  handle name:%10s, Add:0x%llx\n", name, *handle);

    return;
}


void pf_thread_create_mid(
        pf_addrword_t       sched_info,             /* scheduling info (eg pri)      */
        pf_thread_entry_t   *entry,                 /* entry point function          */
        pf_addrword_t       entry_data,             /* entry data                    */
        char                *name,                  /* optional thread name          */
        void                *stack_base,            /* stack base, NULL = alloc      */
        U32                 stack_size,             /* stack size, 0 = default       */
        U32                 mid,                    /* module thread handle id       */
        U32                 log_size                /* log size,default is LOGMAXSIZE*/
        ) 
{
    pthread_attr_t attr;
    sched_param param; 
    int ret;
    
#if DRC_PROCESS
    PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_CNT);
    if(PF_RET_SUCCESS != pf_check_auth_state())
    {
        PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_FAIL);
    }
#endif

#if UNIT_TEST
    PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_CNT);
    if(PF_RET_SUCCESS != pf_check_auth_state())
    {
        PS_CPlus(CM_PES, CMPES_ID_AUTH_CHECK_FAIL);
    }
#endif

    PS_CPlus(CM_COM, CMCOM_ID_THREAD_CREATE_MID_TOTAL_CNT);
    ret = pthread_attr_init(&attr);
    if (0 != ret)
    {
        pl_log(ERR, "pthread_attr_init ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }

    pf_memset(stack_base, 0x5A, stack_size);
    
    printf("\npthread_create stack_base=0x%x, stack_size=0x%x\n",stack_base,stack_size);
    ret = pthread_attr_setstacksize(&attr, stack_size);
    if (0 != ret)
    {
        printf("pthread_attr_setstacksize ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }

    ret = pthread_attr_setstack(&attr, stack_base, stack_size);
    if (0 != ret)
    {
        printf("pthread_attr_setstack ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }

    ret = pthread_create(&workerhandles[mid], &attr, (void*(*)(void*))entry,(void*)entry_data);
    if (0 != ret)
    {
        printf("pthread_create error ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }
	
	/*set thread mid first before msg_entry initial*/
    pf_set_thread_mid(mid);
    pf_thread_mon_set_stack(mid, stack_base, stack_size, stack_base+stack_size, log_size);
    
    ret = pthread_setname_np(workerhandles[mid], name);
    if (0 != ret)
    {
        printf("pthread_setname_np error ret=%d\n",ret);
        PS_CPlus(CM_PES, CMPES_ID_PTHREAD_CREATE_FAIL);
    }
    pf_stat_set_thread_name(name);

    display_pthread_attr(&attr, name);
    printf("pf_thread_create_mid  handle name:%10s, Add:0x%llx\n", name, workerhandles[mid]);

    return;
}



void pf_thread_delete(pf_handle_t handle)
{
    pthread_cancel(handle);

    return;
}

static S32 get_script_version(char *buf)
{
    int fd = 0;
    char read_buf[128];
    char tmp_buf[16];
    char *p = NULL;
    int ret = 0;

    if (!buf)
    {
        return -EINVAL;
    }

    strcpy(buf, "Script is invalid");

    fd = open("/tmp/start_app.sh", O_RDONLY); /*MR 11274: the file name was changed to "start_app.sh"*/
    if (fd < 0)
    {
        fd = open("/mnt/tmpdisk/INNO_APP/start_app.sh",O_RDONLY);
        if (fd < 0)
        {
            strcpy(buf, "NO SCRIPT");
            return 0; /*return "NO SCRIPT" is valid */
        }
    }

    ret = read(fd, read_buf, sizeof(read_buf));
    if (0 > ret)
    {
        close(fd);
        return ret;
    }

    p = strstr(read_buf, "SPT");
    if (!p)
    {
        close(fd);
        return -EIO;
    }
    
    strncpy(tmp_buf, p, sizeof(tmp_buf));
    
    p = strstr(tmp_buf, ")");
    if (!p)
    {
        close(fd);
        return -EIO;
    }
    
    *p = 0; /*End of a string*/
        
    if (strlen(tmp_buf) < sizeof(tmp_buf))
    {
        strcpy(buf, tmp_buf);
    }
    else
    {
        close(fd);
        return -EIO;
    }

    return 0;
}

static S32 get_kernel_version(char *buf)
{
    int ret = pf_read_flush_file((const S8 *)"/proc/version", (const S8 *)buf, MAX_VERSION_LENGTH);

    if(PF_RET_SUCCESS != ret)
    {
        pl_log(ERR, "%s:%d GET KERNEL VERSION FAILED", __FUNCTION__, __LINE__);
        strcpy(buf, "UNKNOWN");
        PS_CPlus(CM_PES, CMPES_ID_KERNEL_VERSION_FAIL);
        return PF_RET_FAILURE;
    }

    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_set_thread_cpucore
 * @brief         set the thread run in the specified CPU core 
 * @input         thread_id         the thread id;   0: current thread  
 * @input         cpucore_mask      the cpu core mask; 
                                    bit0 as cpu0 ;    0: mask           1: allow  
                                    ... 
                                    bit_n as cpu_n;   0: mask           1: allow 
 * @output        void       
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" int pf_set_thread_cpucore(int thread_id, stCpuCoreMask cpucore_mask)
{
    int cpus = 0;
    int  i = 0;
    cpu_set_t mask;
    cpu_set_t get;

    cpus = sysconf(_SC_NPROCESSORS_CONF);

    CPU_ZERO(&mask);    /* Initialize mask to null*/

    for(i = 0; i < cpus; i++) 
    {
        if(i < 32)
        {
            if (cpucore_mask.mask0&(0x00000001<<i)) 
            { 
                CPU_SET(i, &mask); 
            }    

        }
        else if(i < 64)
        {
            if (cpucore_mask.mask1&(0x00000001<<(i-32)))
            { 
                CPU_SET(i, &mask); 
            }    
        }
        else if(i < 96)
        {
            if (cpucore_mask.mask2&(0x00000001<<(i-64)))
            { 
                CPU_SET(i, &mask); 
            }    
        }
        else if(i < 128)
        {
            if (cpucore_mask.mask3&(0x00000001<<(i-96)))
            { 
                CPU_SET(i, &mask); 
            }    
        }
        
    }

    /*set cpu affinity*/
    if (sched_setaffinity(thread_id, sizeof(mask), &mask) == -1)
    {
        pl_log(ERR, "Set CPU affinity failue, thread_id:%d , ERROR:%s",thread_id, strerror(errno));
        return PF_RET_FAILURE; 
    }   
    
    pl_log(INF, " cpu core num is %d ,  thread_id_%d ,  mask_set: mask0_0x%08x, mask1_0x%08x, mask2_0x%08x, mask3_0x%08x ", 
            cpus, thread_id, cpucore_mask.mask0, cpucore_mask.mask1, cpucore_mask.mask2, cpucore_mask.mask3);
    
    return PF_RET_SUCCESS;
}


/**********************************************************************************************
 * @API function  pf_get_thread_cpucore
 * @brief         get cpu affinity of the thread
 * @input         thread_id         the thread id;   0: current thread  
 * @output        cpucore_mask      the cpu core mask; 
                                    bit0 as cpu0 ;    0: mask           1: allow  
                                    ... 
                                    bit_n as cpu_n;   0: mask           1: allow 
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" int pf_get_thread_cpucore(int thread_id, stCpuCoreMask* cpucore_mask)
{
    int cpus = 0;
    int  i = 0;
    cpu_set_t get;
    int coremask = 0;

    cpus = sysconf(_SC_NPROCESSORS_CONF);

    CPU_ZERO(&get); // ��ʼ��get������get��Ϊ0
    ///* �鿴���� cpu �׺Ͷ� */
    if (sched_getaffinity(thread_id, sizeof(get), &get) == -1) 
    {
        pl_log(ERR, "Get CPU affinity failue, thread_id:%d ,  ERROR:%s", thread_id, strerror(errno));
        return PF_RET_FAILURE; 
    }   

    for(i = 0; i < cpus; i++) 
    {
        if(i < 32)
        {
            if (CPU_ISSET(i, &get)) // �鿴cpu i �Ƿ��� get ���ϵ��� 
            { 
                cpucore_mask->mask0 |= (0x00000001<<i);
            }    

        }
        else if(i < 64)
        {
            if (CPU_ISSET(i, &get)) 
            { 
                cpucore_mask->mask1 |= (0x00000001<<(i-32));
            }    
        }
        else if(i < 96)
        {
            if (CPU_ISSET(i, &get)) 
            { 
                cpucore_mask->mask2 |= (0x00000001<<(i-64));
            }    
        }
        else if(i < 128)
        {
            if (CPU_ISSET(i, &get)) 
            { 
                cpucore_mask->mask3 |= (0x00000001<<(i-96));
            }    
        }
        
    }

    
    pl_log(INF, " cpu core num is %d ,  thread_id_%d ,  mask_get: mask0_0x%08x, mask1_0x%08x, mask2_0x%08x, mask3_0x%08x",
        cpus, thread_id, cpucore_mask->mask0,cpucore_mask->mask1,cpucore_mask->mask2,cpucore_mask->mask3);

    return PF_RET_SUCCESS;
}




/**********************************************************************************************
 * @API function  pf_get_inet_aton
 * @brief         whether the input IP address format is correct
 * @input         pcAddr          �����IP address   
 * @output        void
 * @return        0 - correct IP address format
                  other - wrong IP address correct
 *********************************************************************************************/
extern "C" S32 pf_get_inet_aton(char *pcAddr)
{
    U32 i;
    U32 ulStrNum = 0;
    U32 ulStrLen = strlen(pcAddr);
    struct in_addr addr;
    if(0 == inet_aton(pcAddr, &addr))
    {
        pl_log(ERR, "%s:%d PARAM NOT IPADDR %s", __FUNCTION__, __LINE__, pcAddr);
        PS_CPlus(CM_PES, CMPES_ID_INET_ATON_FUNC_FAIL);
        return PF_RET_FAILURE;
    }

    for(i = 1; i < ulStrLen; i++)
    {
        if('.' == pcAddr[i])
        {
            ulStrNum++;
        }
    }

    if(3 != ulStrNum)
    {
        pl_log(ERR, "%s:%d IPADDR FORMAT %s ERROR NUM %d", __FUNCTION__, __LINE__, pcAddr, ulStrNum);
        PS_CPlus(CM_PES, CMPES_ID_INET_ATON_STR_FAIL);
        return PF_RET_FAILURE;
    }
            
    return PF_RET_SUCCESS;
}


/**********************************************************************************************
 * @API function  pf_get_ifconfig_addr
 * @brief         querying current IP address and subnet mask
 * @input         ifname            network name
 * @output        pulIpaddr         return current IP address
                  pulNetmask        return current subnet mask address 
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_get_ifconfig_addr(const S8 *ifname, U32 *pulIpaddr, U32 *pulNetmask)
{
    struct ifreq ifr;
    int fd;
    struct sockaddr_in *addr;

    pf_memset(&ifr, 0, sizeof(struct ifreq));
    if(NULL == ifname)
    {
        pl_log(ERR, "ifname INPUT NULL");
        PS_CPlus(CM_PES, CMPES_ID_IFCONFIG_IN_NULL);
        return PF_RET_FAILURE;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(PF_RET_FAILURE == fd)
    {
        pl_log(ERR, "Not create network socket connection");
        PS_CPlus(CM_PES, CMPES_ID_IFCONFIG_SOCKET_FAIL);
        return PF_RET_FAILURE;
    }

    strncpy(ifr.ifr_name, (char*)ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    /*��ȡ��ǰIP address*/
    if(NULL != pulIpaddr)
    {
        if(ioctl(fd, SIOCGIFADDR, &ifr) < 0)
        {
            pl_log(ERR, "ifname %s Not setup interface for IPADDR", ifname);
            PS_CPlus(CM_PES, CMPES_ID_IFCONFIG_IOCTL_FAIL);
            close(fd);
            return PF_RET_FAILURE;
        }
        addr = (struct sockaddr_in *)&(ifr.ifr_addr);
        *pulIpaddr = addr->sin_addr.s_addr;
        pl_log(INF, "ifname %s IPADDR is %s(0x%x)", ifname, inet_ntoa(addr->sin_addr), (U32)*pulIpaddr);
    }

    /*��ȡ��ǰ���������ַ*/
    if(NULL != pulNetmask)
    {
        if(ioctl(fd, SIOCGIFNETMASK, &ifr) < 0)
        {
            pl_log(ERR, "ifname %s Not setup interface for NETMASK", ifname);
            close(fd);
            PS_CPlus(CM_PES, CMPES_ID_IFCONFIG_IOCTL_FAIL);
            return PF_RET_FAILURE;
        }
        
        addr = (struct sockaddr_in *)&(ifr.ifr_addr);
        *pulNetmask = addr->sin_addr.s_addr;
        pl_log(INF, "ifname %s NETMASK is %s(0x%x)", ifname, inet_ntoa(addr->sin_addr), (U32)*pulNetmask);
    }

    close(fd);

    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_get_ifconfig_mac_addr
 * @brief         ��ѯС��վ��ǰMAC��ַ�Ľӿ�
 * @input         ifname            ��������
                  ulLen             return current MAC��ַ����󳤶�
 * @output        pulIpaddr         return current MAC��ַ
 * @return        ulRes             result
                                    0    - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_get_ifconfig_mac_addr(const S8 *ifname, U8 *pulIpaddr, U32 ulLen)
{
    struct ifreq req;
    int err;

    if((NULL == ifname) || (NULL == pulIpaddr) || (ETH_ALEN > ulLen))
    {
        pl_log(ERR, "INPUT NULL");    
        PS_CPlus(CM_PES, CMPES_ID_IFCONFIG_MAC_IN_NULL);
        return PF_RET_FAILURE;
    }

    int s = socket(AF_INET, SOCK_DGRAM, 0); //internetЭ��������ݱ������׽ӿ�

    strcpy(req.ifr_name, (char*)ifname); //���豸����Ϊ�����������

    err = ioctl(s, SIOCGIFHWADDR, &req); //ִ��ȡMAC��ַ����

    close(s);

    if(PF_RET_FAILURE != err)
    { 
        memcpy(pulIpaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); //ȡ�����MAC��ַ

        return PF_RET_SUCCESS;
    }
    else
    {
        pl_log(ERR, "Failed get MAC Address");    
        PS_CPlus(CM_PES, CMPES_ID_IFCONFIG_IOCTL_FAIL);
        return PF_RET_FAILURE;
    }
}

/**********************************************************************************************
 * @API function  pf_get_ifconfig_info
 * @brief         querying current  all network cards name
 * @input        
 * @output       ifname            network name                 
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_get_ifconfig_info(const S8 *ifname)
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[2048];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == PF_RET_FAILURE) 
    {
        pl_log(ERR, "socket error");    
        return PF_RET_FAILURE;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == PF_RET_FAILURE) 
    {
        pl_log(ERR, "ioctl error");  
        close(sock);
        return PF_RET_FAILURE;
    }
    
    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    int count = 0;
    
    for (; it != end; ++it)
    {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
        {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) 
            { // don't count loopback
                sprintf((char *)ifname, "%s  %s", ifname, it->ifr_name);
            }
        }
        else
        {
            pl_log(ERR, "flags error");    
            close(sock);
            return PF_RET_FAILURE;
        }
    }

    close(sock);
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_set_ip_addr
 * @brief         Setting the current IP address interface
 * @input         ifname            network card name 
 * @input         scIpAddr          IP address to be set
 * @output        void
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_set_ip_addr(S8* ifname,S8* scIpAddr)
{
    struct ifreq ifr;    
    char *dev = (char*)ifname;
    S32 slRet = pf_get_inet_aton((char*)scIpAddr);
    if(PF_RET_FAILURE == slRet)
    {
        pl_log(ERR, "dest IP Addr=%s WRONG", scIpAddr);
        PS_CPlus(CM_PES, CMPES_ID_IPADDR_SET_FAIL);
        return PF_RET_FAILURE;
    }
    
    pf_memset(&ifr,0,sizeof(ifr));    
    if( strlen(dev) >= IFNAMSIZ)
    {
        pl_log(ERR, "device name error.");
        PS_CPlus(CM_PES, CMPES_ID_IPADDR_IFNAMSIZ_FAIL);
        return PF_RET_FAILURE;
    }

    strcpy( ifr.ifr_name, dev);
    
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(PF_RET_FAILURE == sockfd)
    {
        pl_log(ERR, "Not create network socket connection");
        PS_CPlus(CM_PES, CMPES_ID_IPADDR_SET_FAIL);
        return PF_RET_FAILURE;
    }

    //get inet addr
    if( ioctl( sockfd, SIOCGIFADDR, &ifr) == -1)
    {
        pl_log(ERR, "IOCTRL GET IFADDR error.");
        PS_CPlus(CM_PES, CMPES_ID_IPADDR_SIOCGIFADDR1_FAIL);
        close(sockfd);
        return PF_RET_FAILURE;
    }
    
    struct sockaddr_in *addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    char * address = inet_ntoa(addr->sin_addr);

    pl_log(UINF, "current inet addr: %s",address);

    //set inet addr
    struct sockaddr_in *p = (struct sockaddr_in *)&(ifr.ifr_addr);

    p->sin_family = AF_INET;
    inet_aton( (char*)scIpAddr, &(p->sin_addr));

    if( ioctl( sockfd, SIOCSIFADDR, &ifr) == -1)
    {
        pl_log(ERR, "CURIP %s SET IFADDR %s error.", address, scIpAddr);
        PS_CPlus(CM_PES, CMPES_ID_IPADDR_SIOCGIFADDR2_FAIL);
        close(sockfd);
        return PF_RET_FAILURE;
    }
    
    pl_log(INF, "change inet addr to: %s", scIpAddr);
    close(sockfd);
    
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_set_default_gw
 * @brief         Setting the current gateway address interface
 * @input         scGwAddr          Gateway address to be set
 * @output        void
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_set_default_gw(S8* scGwAddr)
{
    int skfd;
    struct rtentry route;
    int err;
    //char *dev = "eth0";
    S32 slRet = pf_get_inet_aton((char*)scGwAddr);
    if(PF_RET_FAILURE == slRet)
    {
        pl_log(ERR, "dest GW IP Addr=%s WRONG", scGwAddr);
        PS_CPlus(CM_PES, CMPES_ID_GW_SET_FAIL);
        return PF_RET_FAILURE;
    }
    
    skfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        pl_log(ERR, "GW IP SOCKET %s CREATE FAILED", scGwAddr);
        PS_CPlus(CM_PES, CMPES_ID_GW_SET_SOCKET_FAIL);
        return PF_RET_FAILURE;
    }

    /* Delete existing defalt gateway */
    pf_memset(&route, 0, sizeof(route));

    route.rt_dst.sa_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = 0;

    route.rt_genmask.sa_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = 0;

    route.rt_flags = RTF_UP;

    err = ioctl(skfd, SIOCDELRT, &route);

    if ((err == 0 || errno == ESRCH) && scGwAddr) 
    {
        /* Set default gateway */
        memset(&route, 0, sizeof(route));

        route.rt_dst.sa_family = AF_INET;
        ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = 0;

        route.rt_gateway.sa_family = AF_INET;
        ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr = inet_addr((const char*)scGwAddr);

        route.rt_genmask.sa_family = AF_INET;
        ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = 0;

        route.rt_flags = RTF_UP | RTF_GATEWAY;

        err = ioctl(skfd, SIOCADDRT, &route);
        
        close(skfd);
        return err;
    }
    else
    {
        close(skfd);
        pl_log(ERR, "GW IP SOCKET %s CREATE FAILED", scGwAddr);
        PS_CPlus(CM_PES, CMPES_ID_GW_SET_SIOCDELRT_FAIL);
        return PF_RET_FAILURE;
    }

}

/**********************************************************************************************
 * @API function  pf_set_net_mask
 * @brief         used to set subnet mask
 * @input         ifname            network card name 
 * @input         netmask           subnet mask address to be set
 * @output        void
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_set_net_mask(S8* ifname,const S8 *netmask)
{
    struct sockaddr_in sin;
    struct ifreq ifr;
    S32 fd;
    char *dev = (char*)ifname;

    S32 slRet = pf_get_inet_aton((char*)netmask);
    if(PF_RET_FAILURE == slRet)
    {
        pl_log(ERR, "dest NET MASK Addr WRONG");
        PS_CPlus(CM_PES, CMPES_ID_NETMASK_SET_FAIL);
        return PF_RET_FAILURE;
    }

    pf_memset(&ifr,0,sizeof(ifr));    
    if( strlen(dev) >= IFNAMSIZ)
    {
        pl_log(ERR, "device name error FOR NETMASK");
        PS_CPlus(CM_PES, CMPES_ID_NETMASK_SET_DEV_FAIL);
        return PF_RET_FAILURE;
    }
    else
    {
        strcpy( ifr.ifr_name, dev);
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(PF_RET_FAILURE == fd)
    {
        pl_log(ERR, "Not create network socket connection");
        PS_CPlus(CM_PES, CMPES_ID_NETMASK_SET_SOCKET_FAIL);
        return PF_RET_FAILURE;
    }

    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr((char*)netmask);
    memcpy(&ifr.ifr_addr, &sin, sizeof(sin));

    if(ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
    {
        pl_log(ERR, "net mask ioctl error");
        close(fd);
        PS_CPlus(CM_PES, CMPES_ID_NETMASK_SET_SIOCSIFNETMASK_FAIL);
        return PF_RET_FAILURE;
    }

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0)
    {
        pl_log(ERR, "SIOCSIFFLAGS FAILED");
        close(fd);
        PS_CPlus(CM_PES, CMPES_ID_NETMASK_SET_SIOCSIFFLAGS_FAIL);
        return PF_RET_FAILURE;
    }

    close(fd);

    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_set_ip_addr_net_mask
 * @brief         used to set IP address and subnet mask
 * @input         ifname            network card name 
 * @input         scIpAddr          IP address to be set
 * @input         netmask           subnet mask address to be set
 * @output        void
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_set_ip_addr_net_mask(S8* ifname,S8* scIpAddr,S8 *netmask)
{
    struct ifreq ifr;    
    char *dev = (char*)ifname;
    
    S32 slRet = pf_get_inet_aton((char*)scIpAddr);
    if(PF_RET_FAILURE == slRet)
    {
        pl_log(ERR, "dest IP Addr=%s WRONG", scIpAddr);
 //       PS_CPlus(CM_PES, CMPES_ID_IPMASK_IP_FAIL);
        return PF_RET_FAILURE;
    }

    slRet = pf_get_inet_aton((char*)netmask);
    if(PF_RET_FAILURE == slRet)
    {
        pl_log(ERR, "dest NET MASK Addr=%s WRONG",netmask);
//        PS_CPlus(CM_PES, CMPES_ID_IPMASK_MASK_FAIL);
        return PF_RET_FAILURE;
    }
    
    pf_memset(&ifr,0,sizeof(ifr));    
    if( strlen(dev) >= IFNAMSIZ)
    {
        pl_log(ERR, "device name error.");
  //      PS_CPlus(CM_PES, CMPES_ID_IPMASK_IFNAMSIZ_FAIL);
        return PF_RET_FAILURE;
    }
    else
    {
        strcpy( ifr.ifr_name, dev);
    }
    
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);

    if(PF_RET_FAILURE == sockfd)
    {
        pl_log(ERR, "Not create network socket connection");
//        PS_CPlus(CM_PES, CMPES_ID_IPMASK_SET_SOCKET_FAIL);
        return PF_RET_FAILURE;
    }


    //set inet addr
    struct sockaddr_in *p = (struct sockaddr_in *)&(ifr.ifr_addr);

    p->sin_family = AF_INET;
    inet_aton( (char*)scIpAddr, &(p->sin_addr));

    if( ioctl( sockfd, SIOCSIFADDR, &ifr) == -1)
    {
        pl_log(ERR, "SET IFADDR %s error.", scIpAddr);
 //       PS_CPlus(CM_PES, CMPES_ID_IPMASK_SIOCGIFADDR2_FAIL);
 
        close(sockfd);
        return PF_RET_FAILURE;
    }

    inet_aton( (char*)netmask, &(p->sin_addr));
    if(ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0)
    {
        pl_log(ERR, "SET NETMASK %s error",netmask);
        close(sockfd);
 //       PS_CPlus(CM_PES, CMPES_ID_IPMASK_SET_SIOCSIFNETMASK_FAIL);
        return PF_RET_FAILURE;
    }
    
    close(sockfd);
    pl_log(INF, "SET  %s  IP and NETMASK to:  %s  %s ",dev, scIpAddr,netmask);
    return PF_RET_SUCCESS;
}


/**********************************************************************************************
 * @API function  pf_enable_networkcard
 * @brief         used to enable network card 
 * @input         ifname            network card name 
 * @output        void
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_enable_networkcard(S8* ifname)
{
    S32 fd;
    struct ifreq ifr;
    char *dev = (char*)ifname;

    pf_memset(&ifr,0,sizeof(ifr));    
    if( strlen(dev) >= IFNAMSIZ)
    {
        pl_log(ERR, "device name error FOR NETENABLE");
//        PS_CPlus(CM_PES, CMPES_ID_NETENABLE_SET_DEV_FAIL);
        return PF_RET_FAILURE;
    }
    else
    {
        strcpy( ifr.ifr_name, dev);
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(PF_RET_FAILURE == fd)
    {
        pl_log(ERR, "NETENABLE  create network socket connection fail");
//        PS_CPlus(CM_PES, CMPES_ID_NETENABLE_SET_SOCKET_FAIL);
        return PF_RET_FAILURE;
    }

    //get current status
    if(ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
    {
        pl_log(ERR, "NETENABLE ioctl SIOCGIFFLAGS error");
        close(fd);
//        PS_CPlus(CM_PES, CMPES_ID_NETENABLE_GET_SIOCGIFFLAGS_FAIL);
        return PF_RET_FAILURE;
    }

    //let net work up
    ifr.ifr_flags |= IFF_UP ;
    
    //change status
    if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0)
    {
        pl_log(ERR, "NETENABLE ioctl SIOCSIFFLAGS error");
        close(fd);
//        PS_CPlus(CM_PES, CMPES_ID_NETENABLE_SET_SIOCSIFFLAGS_FAIL);
        return PF_RET_FAILURE;
    }
    
    close(fd);
    
    return PF_RET_SUCCESS;
}



/**********************************************************************************************
 * @API function  pf_disable_networkcard
 * @brief         used to disable network card 
 * @input         ifname            network card name 
 * @output        void
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
extern "C" S32 pf_disable_networkcard(S8* ifname)
{
    S32 fd;
    struct ifreq ifr;
    char *dev = (char*)ifname;

    pf_memset(&ifr,0,sizeof(ifr));    
    if( strlen(dev) >= IFNAMSIZ)
    {
        pl_log(ERR, "device name error FOR NETDISABLE");
//        PS_CPlus(CM_PES, CMPES_ID_NETDISABLE_SET_DEV_FAIL);
        return PF_RET_FAILURE;
    }
    else
    {
        strcpy( ifr.ifr_name, dev);
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(PF_RET_FAILURE == fd)
    {
        pl_log(ERR, "NETDISABLE  create network socket connection fail");
//        PS_CPlus(CM_PES, CMPES_ID_NETDISABLE_SET_SOCKET_FAIL);
        return PF_RET_FAILURE;
    }



    //get current status
    if(ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
    {
        pl_log(ERR, "NETDISABLE ioctl SIOCGIFFLAGS error");
        close(fd);
//        PS_CPlus(CM_PES, CMPES_ID_NETDISABLE_GET_SIOCGIFFLAGS_FAIL);
        return PF_RET_FAILURE;
    }

    //let net work down
    ifr.ifr_flags &= ~IFF_UP;
    
    //change status
    if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0)
    {
        pl_log(ERR, "NETDISABLE ioctl SIOCSIFFLAGS error");
        close(fd);
//        PS_CPlus(CM_PES, CMPES_ID_NETDISABLE_SET_SIOCSIFFLAGS_FAIL);
        return PF_RET_FAILURE;
    }
    
    close(fd);
    
    return PF_RET_SUCCESS;
}



/**********************************************************************************************
 * @API function  pf_set_system_call
 * @brief         call system fuction directly
 * @input         scCmd  The command to call system fuction
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_set_system_call(const S8* scCmd)
{
    pid_t status;  
    if(NULL == scCmd)
    {
        pl_log(ERR, "INPUT shell script NULL");  
        PS_CPlus(CM_PES, CMPES_ID_SYSTEM_CALL_IN_NULL);
        return PF_RET_FAILURE;  
    }

    pl_log(INF, "INPUT shell script: %s", scCmd);  

    if(strstr((const char*)scCmd, "reboot"))
    {
        pl_log(WARN, "ps send %s command", scCmd);  

        /*Synchronized disk data*/
        sync();

        pf_usleep(2000000);

        printf("pf_set_system_call %s\n", scCmd);
    }
    
    status = system((char*)scCmd);  
    if (PF_RET_FAILURE == status)  
    {  
        pl_log(ERR, "scCmd=%s system call error, script exit code: %d!", scCmd, WEXITSTATUS(status));  
        PS_CPlus(CM_PES, CMPES_ID_SYSTEM_CALL_FUNC_FAIL);
        return PF_RET_FAILURE;
    }  
    else  
    {  
        pl_log(INF, "exit status value = [0x%x]", status);  
  
        if (WIFEXITED(status))  
        {  
            if (0 == WEXITSTATUS(status))  
            {  
                pl_log(INF, "run shell script successfully.");  
                return PF_RET_SUCCESS;
            }  
            else  
            {  
                PS_CPlus(CM_PES, CMPES_ID_SYSTEM_CALL_EXIT_CODE_FAIL);
                pl_log(ERR, "run shell script fail, script exit code: %d", WEXITSTATUS(status));  
            }  
        }  
        else  
        {  
            PS_CPlus(CM_PES, CMPES_ID_SYSTEM_CALL_EXIT_STATUS_FAIL);
            pl_log(ERR, "system call exit status = [%d]", WEXITSTATUS(status));  
        }  
    }  
  
    return status;  
}

/**********************************************************************************************
 * @API function  pf_write_flush_file
 * @brief         write and flush the file
 * @input         scPath    The path of file
                  scStr     The string writen to file
                  ulLength  The length of string writen to file
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_write_flush_file(const S8* scPath, const S8* scStr, U32 ulLength)
{
    FILE* fpFile = NULL;
    size_t ulWrLen = 0;
    U32 i;

    PS_CPlus(CM_COM, CMCOM_ID_WRITE_FLUSH_TOTAL_CNT);
    if((NULL == scPath) || (NULL == scStr))
    {
        pl_log(ERR, "input NULL");  
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_IN_NULL);
        return PF_RET_FAILURE;
    }
	U32 ulLen = strlen((CHAR*)scPath);
    if(ulLen >= MAX_FILE_PATH_LENGTH)
    {
        pl_log(ERR, "input ulLen %d exceed %d", ulLen, MAX_FILE_PATH_LENGTH);  
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_IN_NULL);
        return PF_RET_FAILURE;
    }
	
    CHAR str[MAX_FILE_PATH_LENGTH];    
    pf_memcpy(str, scPath, ulLen+1);
    for( i=1; i<ulLen; i++ )
    {
        if( str[i] == '/' )
        {
            str[i] = '\0';
            if(0 != access(str, F_OK))
            {
                if(0 != mkdir(str, 0755))
                {
                    pl_log(ERR, "mkdir %s failed", str);      
                }
            }
            str[i] = '/';
        }
    }
	
    /*open in write mode, set the file length to 0*/
    fpFile = fopen((const char*)scPath, "w");
    if(NULL != fpFile)
    {
        ulWrLen = fwrite((void*)scStr, ulLength, 1, fpFile);
#if 0
        if(ulLength != ulWrLen)
        {
            pl_log(WARN, "write file %s(%d)(%d) NOT SAME!", scStr, ulLength, ulWrLen);
        }
#endif        
        fflush(fpFile);
        fsync(fileno(fpFile));
        fclose(fpFile);
        PS_CPlus(CM_COM, CMCOM_ID_WRITE_FLUSH_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_WRITE_FLUSH_SIZE, ulLength);
        return PF_RET_SUCCESS;  
    }  

    pl_log(ERR, "write_file failed");  
    PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_OPEN_FAIL);
    return PF_RET_FAILURE;  
}

/**********************************************************************************************
 * @API function  pf_write_endof_file
 * @brief         write to the end of file
 * @input         scPath    The path of file
                  scStr     The string writen to file
                  ulLength  The length of string writen to file
 *     
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_write_endof_file(const S8* scPath, const S8* scStr, U32 ulLength)
{
    FILE* fpFile = NULL;
    U32 i;

    PS_CPlus(CM_COM, CMCOM_ID_WRITE_ENDOF_TOTAL_CNT);
    if((NULL == scPath) || (NULL == scStr))
    {
        PS_CPlus(CM_PES, CMPES_ID_WRITE_ENDOF_IN_NULL);
        pl_log(ERR, "input NULL");  
        return PF_RET_FAILURE;
    }

	U32 ulLen = strlen((CHAR*)scPath);
    if(ulLen >= MAX_FILE_PATH_LENGTH)
    {
        pl_log(ERR, "input ulLen %d exceed %d", ulLen, MAX_FILE_PATH_LENGTH);  
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_IN_NULL);
        return PF_RET_FAILURE;
    }

    CHAR str[MAX_FILE_PATH_LENGTH];    
    pf_memcpy(str, scPath, ulLen+1);
    for( i=1; i<ulLen; i++ )
    {
        if( str[i] == '/' )
        {
            str[i] = '\0';
            if(0 != access(str, F_OK))
            {
                if(0 != mkdir(str, 0755))
                {
                    pl_log(ERR, "mkdir %s failed", str);      
                }
            }
            str[i] = '/';
        }
    }

    /*open in write mode, keep the original file length*/
    fpFile = fopen((const char*)scPath, "a");
    if(NULL != fpFile)
    {
        fwrite((void*)scStr, ulLength, 1, fpFile);
        fflush(fpFile);
        fsync(fileno(fpFile));
        fclose(fpFile);
        PS_CPlus(CM_COM, CMCOM_ID_WRITE_ENDOF_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_WRITE_ENDOF_SIZE, ulLength);
        return PF_RET_SUCCESS;  
    }  

    PS_CPlus(CM_PES, CMPES_ID_WRITE_ENDOF_OPEN_FAIL);
    return PF_RET_FAILURE;  
}

/**********************************************************************************************
 * @API function  pf_copy_msg_file
 * @brief         copy content to platform thread for writing the end of file
 * @input         fileName          The path of file, should not be NULL
                  strContentPtr     The string writen to file, create file only if content is NULL.
 *     
 * @output        none
 * @return        true: ok
                  false: failure
 *********************************************************************************************/
BOOL pf_copy_msg_file(std::string &fileName, std::shared_ptr<std::string> strContentPtr)
{
    PS_CPlus(CM_COM, CMCOM_ID_COPYMSG_ENDOF_TOTAL_CNT);
    if(true == fileName.empty())
    {
        PS_CPlus(CM_PES, CMPES_ID_WRITE_ENDOF_IN_NULL);
        return FALSE;
    }

    PF_MUTEX_LOCK(&g_stFileMutex);
    g_stFileMapInfo[fileName].push_back(strContentPtr);
    PF_MUTEX_UNLOCK(&g_stFileMutex);
    
    return TRUE;  
}


/**********************************************************************************************
 * @API function  pf_read_flush_file
 * @brief         read the file
 * @input         scPath    The path of file
                  scStr     The string read from the file
                  ulLength  The length of string read from file
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_read_flush_file(const S8* scPath, const S8* scStr, U32 ulLength)
{
    FILE* fpFile = NULL;
    size_t ulRdLen = 0;

    PS_CPlus(CM_COM, CMCOM_ID_READ_FLUSH_TOTAL_CNT);
    if((NULL == scPath) || (NULL == scStr))
    {
        pl_log(ERR, "input NULL");  
        PS_CPlus(CM_PES, CMPES_ID_READ_FILE_IN_NULL);
        return PF_RET_FAILURE;
    }

    /*open in read mode*/
    fpFile = fopen((const char*)scPath, "r");
    if(NULL != fpFile)
    {
        pf_memset((void*)scStr, 0, ulLength);

        ulRdLen = fread((void*)scStr, ulLength, 1, fpFile);
        fclose(fpFile);
        PS_CPlus(CM_COM, CMCOM_ID_READ_FLUSH_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_READ_FLUSH_SIZE, ulRdLen);
        return PF_RET_SUCCESS;  
    }  

    pl_log(ERR, "read_file failed");  
    PS_CPlus(CM_PES, CMPES_ID_READ_FILE_OPEN_FAIL);
    return PF_RET_FAILURE;  
}


/**********************************************************************************************
 * @API function  pf_get_file_length
 * @brief         get length of the file
 * @input         scPath     The path of file
 * @output        pulFileLen Length of the file
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_get_file_length(S8* scPath, U32* pulFileLen)
{
    U32 filesize = 0;
    FILE *fp;    
    PS_CPlus(CM_COM, CMCOM_ID_GET_FILE_TOTAL_CNT);
    fp = fopen((char*)scPath, "r");    
    if(NULL == fp)    
    {
        *pulFileLen = 0;
        pl_log(ERR, "The file %s NOT EXIT!", scPath);
        PS_CPlus(CM_PES, CMPES_ID_GET_FILE_LENGTH_IN_NULL);
        return PF_RET_FAILURE;    
    }
    
    fseek(fp, 0L, SEEK_END);    
    filesize = ftell(fp);    
    fclose(fp);    
    *pulFileLen = filesize;
    PS_CPlus(CM_COM, CMCOM_ID_GET_FILE_CNT);
    return PF_RET_SUCCESS;    
}


/**********************************************************************************************
 * @API function  pf_is_file_equal
 * @brief         whether the source and the destination are the same
 * @input         scDstPath     The dest path of file
 *                scSrcPath     The source path of file
 * @output        none
 * @return        0: different
                  1: same
 *********************************************************************************************/
extern "C" BOOL pf_is_file_equal(S8* scDstPath, S8* scSrcPath)
{
    U32 ulDstSize = -1;    
    U32 ulSrcSize = -1;    
    S8* pscDstData = NULL;
    S8* pscSrcData = NULL;
    BOOL bret = TRUE;
    
    if(PF_RET_SUCCESS != pf_get_file_length(scDstPath, &ulDstSize))
    {
        pl_log(ERR, "The dest file %s get length error!", scDstPath);
        return FALSE;    
    }

    if(PF_RET_SUCCESS != pf_get_file_length(scSrcPath, &ulSrcSize))
    {
        pl_log(ERR, "The source file %s get length error!", scSrcPath);
        return FALSE;    
    }

    if(ulDstSize != ulSrcSize)
    {
        pl_log(ERR, "The file %s and %s length differ %d,%d!", scSrcPath, scDstPath, ulSrcSize, ulDstSize);
        return FALSE;    
    }

    pscDstData = (S8*)pf_malloc(ulDstSize + 1);
    if(NULL == pscDstData)
    {
        pl_log(ERR, "get Dest length %d error!", ulDstSize);
        return FALSE;    
    }
        
    pscSrcData = (S8*)pf_malloc(ulDstSize + 1);
    if(NULL == pscSrcData)
    {
        pl_log(ERR, "get src length %d error!", ulDstSize);
        pf_free(pscDstData);
        return FALSE;    
    }

    if(PF_RET_SUCCESS != pf_read_flush_file(scDstPath, pscDstData, ulDstSize))
    {
        pl_log(ERR, "read dst %s length %d error!", scDstPath, ulDstSize);
        bret = FALSE;    
    }

    if(PF_RET_SUCCESS != pf_read_flush_file(scSrcPath, pscSrcData, ulDstSize))
    {
        pl_log(ERR, "read dst %s length %d error!", scSrcPath, ulDstSize);
        bret = FALSE;    
    }

    if(0 == memcmp(pscDstData, pscSrcData, ulDstSize))
    {
        pl_log(INF, "The file %s and %s length %d equal!", scSrcPath, scDstPath, ulSrcSize);
        bret = TRUE;    
    }
    else
    {
        pl_log(ERR, "The file %s and %s length %d content not equal!", scSrcPath, scDstPath, ulSrcSize);
        bret = FALSE;    
    }

    /*�ͷ�����the address of memory*/
    pf_free(pscSrcData);
    pf_free(pscDstData);
    return bret;
}


/**********************************************************************************************
 * @API function  pf_is_file_exist
 * @brief         whether the file exists or not
 * @input         scSrcPath     The source path of file
 * @output        none
 * @return        0: not exist
                  1: exist
 *********************************************************************************************/
extern "C" BOOL pf_is_file_exist(S8* scSrcPath)
{
    if(scSrcPath)
    {
        if(PF_RET_SUCCESS == access((CHAR*)scSrcPath, F_OK))
        {   
            return TRUE;   
        }   
    }

    return FALSE;
}

/**********************************************************************************************
 * @API function  pf_copy_flush_file
 * @brief         copy and flush the file from source path to destnation path.
 * @input         scDstPath The destination path of the file
                  scDstPath The source path of the file
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_copy_flush_file(const CHAR* scDstPath, const CHAR* scSrcPath)
{
    S8* pscData = NULL;
    size_t ulWrLen = 0;
    U32 ulLength;

    PS_CPlus(CM_COM, CMCOM_ID_WRITE_FLUSH_TOTAL_CNT);
    if((NULL == scDstPath) || (NULL == scSrcPath))
    {
        pl_log(ERR, "input NULL");  
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_IN_NULL);
        return PF_RET_FAILURE;
    }

    /*open in write mode, set the file length to 0*/
    if(PF_RET_SUCCESS != pf_get_file_length((S8*)scSrcPath, &ulLength))
    {
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_OPEN_FAIL);
        return PF_RET_FAILURE;
    }  

    pscData = (S8*)pf_malloc(ulLength);
    if(!pscData)
    {
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_OPEN_FAIL);
        return PF_RET_FAILURE;
    }  

    if(PF_RET_SUCCESS != pf_read_flush_file((const S8*)scSrcPath, (const S8 *) pscData, ulLength))
    {
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_OPEN_FAIL);
        pf_free((void*)pscData);
        return PF_RET_FAILURE;

    }

    if(PF_RET_SUCCESS != pf_write_flush_file((const S8*)scDstPath, (const S8 *) pscData, ulLength))
    {
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_OPEN_FAIL);
        pf_free((void*)pscData);
        return PF_RET_FAILURE;
    }

    pf_free((void*)pscData);
    return PF_RET_SUCCESS;  
}

  

#define MAX_CHARACTOR_LENGTH_NUMBER     32
#define MAX_CHARACTOR_LENGTH_IN_LINE    50
#define TMP_SYSTEM_MEMINFO_PATH    "/tmp/meminfo"
#define TMP_PROC_MEMINFO_PATH      "/tmp/status"
/**********************************************************************************************
 * @API function  pf_read_flush_file_in_line
 * @brief         read line from the file
 * @input         scPath    The path of file
                  scSrcStr  The string need to search in the file
                  scLine    The string line read from the file
                  ulLength  The length of string read from file
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_read_flush_file_in_line(const S8* scPath, const S8* scSrcStr, U32 ulNum, S8* scLine, U32 ulLength)
{
    U32 ulFileLen = 0;
    S8* pscData = NULL;
    S8* pscLineAddr = NULL;
    S8* pscTmpData = NULL;
    U32 i;
    U32 j;
    U32 ulLineLen;
    U32 ulTotalLen = 0;

    if((NULL == scPath) || (NULL == scSrcStr) || (NULL == scLine) || (0 == ulLength))
    {
        pl_log(ERR, "input NULL");  
        PS_CPlus(CM_PES, CMPES_ID_READ_FILE_INLINE_IN_NULL);
        return PF_RET_FAILURE;
    }

    if(PF_RET_SUCCESS != pf_get_file_length((S8*)scPath, &ulFileLen))
    {
        pl_log(ERR, "pf_read_file_line get file length error %d", ulFileLen);  
        return PF_RET_FAILURE;
    }

    pscData = (S8*)pf_malloc(ulFileLen+1);
    if(NULL == pscData)
    {
        pl_log(ERR, "pf_read_file_line get memory length %d failed", ulFileLen);  
        return PF_RET_FAILURE;
    }
    pf_memset(pscData, 0, ulFileLen+1);

    if(PF_RET_SUCCESS != pf_read_flush_file(scPath, pscData, ulFileLen))
    {
        pl_log(ERR, "pf_read_file_line get file length error %d", ulFileLen);  
        return PF_RET_FAILURE;
    }

    for(j=0; j<ulNum; j++)
    {
        /*get the address of character string*/
        pscLineAddr = (S8*)strstr((char*)pscData, (char*)(scSrcStr + j*MAX_CHARACTOR_LENGTH_IN_LINE));    
        if(NULL == pscLineAddr)
        {
            pl_log(ERR, "pf_read_file_line get no %s string", scSrcStr + j*MAX_CHARACTOR_LENGTH_IN_LINE);  
            continue;
        }

        pscTmpData = pscLineAddr;
        i = pscLineAddr - pscData;
        while(i < ulFileLen)
        {
            i++;
            if('\n' == *pscTmpData)
            {
                break;
            }
            pscTmpData++;
        }

        ulLineLen = i - (pscLineAddr - pscData);
        
        if(ulLength < ulLineLen + ulTotalLen)
        {
            pl_log(ERR, "pf_read_file_line output error for length %d limited %d %d", ulLength, ulLineLen, ulTotalLen);  
            continue;
        }

        pf_memcpy(scLine+ulTotalLen, pscLineAddr, ulLineLen);

        ulTotalLen += ulLineLen;

        pl_log(INF, "pf_read_file_line %d total %d", ulLineLen, ulTotalLen);  
    }

    *(scLine + ulTotalLen) = 0;
    pf_free(pscData);

    return PF_RET_SUCCESS;  
}


/**********************************************************************************************
 * @API function  pf_set_root_path
 * @brief         set the root path
 * @input         scPath    The root path
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_set_root_path(const S8* scPath)
{
    if(NULL == scPath)
    {
        return PF_RET_FAILURE;
    }

    if((scPath[0] != '/') || (strlen((CHAR*)scPath) >= sizeof(acRootPath)))
    {
        return PF_RET_FAILURE;
    }


    sprintf((CHAR*)acRootPath, "%s", scPath);
    
    return PF_RET_SUCCESS;  
}


/**********************************************************************************************
 * @API function  pf_get_root_path
 * @brief         get the root path
 * @input         void
 * 
 * @output        none
 * @return        the bootup path of the process 
 *********************************************************************************************/
extern "C" CHAR* pf_get_root_path(void)
{
    return (CHAR*)acRootPath;  
}


/**********************************************************************************************
 * @API function  pf_write_root_path_file
 * @brief         write and flush the file of relative root path
 * @input         scPath    The relative path of file
                  scStr     The string writen to file
                  ulLength  The length of string writen to file
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_write_root_path_file(const S8* scPath, const S8* scStr, U32 ulLength)
{
    FILE* fpFile = NULL;
    size_t ulWrLen = 0;
    CHAR acPath[512];

    PS_CPlus(CM_COM, CMCOM_ID_WRITE_FLUSH_TOTAL_CNT);
    if((NULL == scPath) || (NULL == scStr))
    {
        pl_log(ERR, "pf_write_file input NULL");  
        PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_IN_NULL);
        return PF_RET_FAILURE;
    }

    sprintf((CHAR*)acPath, "%s%s", acRootPath, scPath);

    /*open in write mode, set the file length to 0*/
    fpFile = fopen((const char*)acPath, "w");
    if(NULL != fpFile)
    {
        ulWrLen = fwrite((void*)scStr, ulLength, 1, fpFile);

        fflush(fpFile);
        fsync(fileno(fpFile));
        fclose(fpFile);
        PS_CPlus(CM_COM, CMCOM_ID_WRITE_FLUSH_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_WRITE_FLUSH_SIZE, ulLength);
        return PF_RET_SUCCESS;  
    }  

    pl_log(ERR, "write_file failed");  
    PS_CPlus(CM_PES, CMPES_ID_WRITE_FILE_OPEN_FAIL);
    return PF_RET_FAILURE;  
}


/**********************************************************************************************
 * @API function  pf_read_root_path_file
 * @brief         read the file of relative root path
 * @input         scPath    The relative path of file
                  scStr     The string read from the file
                  ulLength  The length of string read from file
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_read_root_path_file(const S8* scPath, const S8* scStr, U32 ulLength)
{
    FILE* fpFile = NULL;
    size_t ulRdLen = 0;
    CHAR acPath[512];

    PS_CPlus(CM_COM, CMCOM_ID_READ_FLUSH_TOTAL_CNT);
    if((NULL == scPath) || (NULL == scStr))
    {
        pl_log(ERR, "pf_read_file input NULL");  
        PS_CPlus(CM_PES, CMPES_ID_READ_FILE_IN_NULL);
        return PF_RET_FAILURE;
    }
    sprintf((CHAR*)acPath, "%s%s", acRootPath, scPath);

    /*open in read mode*/
    fpFile = fopen((const char*)acPath, "r");
    if(NULL != fpFile)
    {
        pf_memset((void*)scStr, 0, ulLength);

        ulRdLen = fread((void*)scStr, ulLength, 1, fpFile);
        fclose(fpFile);
        PS_CPlus(CM_COM, CMCOM_ID_READ_FLUSH_CNT);
        PS_CPlusV(CM_COM, CMCOM_ID_READ_FLUSH_SIZE, ulRdLen);
        return PF_RET_SUCCESS;  
    }  

    pl_log(ERR, "read_file failed");  
    PS_CPlus(CM_PES, CMPES_ID_READ_FILE_OPEN_FAIL);
    return PF_RET_FAILURE;  
}

/**********************************************************************************************
 * @API function  pf_get_root_path_file_length
 * @brief         get length of the root path file
 * @input         scPath     The path of file
 * @output        pulFileLen Length of the file
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_get_root_path_file_length(const S8* scPath, U32* pulFileLen)
{
    U32 filesize = 0;
    FILE *fp;    
    CHAR acPath[512];
    sprintf((CHAR*)acPath, "%s%s", acRootPath, scPath);

    PS_CPlus(CM_COM, CMCOM_ID_GET_FILE_TOTAL_CNT);
    fp = fopen((char*)acPath, "r");    
    if(NULL == fp)    
    {
        *pulFileLen = 0;
        pl_log(ERR, "The file %s NOT EXIT!", acPath);
        PS_CPlus(CM_PES, CMPES_ID_GET_FILE_LENGTH_IN_NULL);
        return PF_RET_FAILURE;    
    }
    
    fseek(fp, 0L, SEEK_END);    
    filesize = ftell(fp);    
    fclose(fp);    
    *pulFileLen = filesize;
    PS_CPlus(CM_COM, CMCOM_ID_GET_FILE_CNT);
    return PF_RET_SUCCESS;    
}


/**********************************************************************************************
 * @API function  pf_get_sysinfo
 * @brief         get system info 
 * @input         ulLength  The length of max string 
 * 
 * @output        scInfo    The string of system info
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" S32 pf_get_sysinfo(S8* scInfo, U32 ulLength)
{
    U32 ulTotalLen = 0;
    S8 ascPath[MAX_CHARACTOR_LENGTH_IN_LINE];
    S8 ascMemInfo[][MAX_CHARACTOR_LENGTH_IN_LINE] = {
            "MemTotal",
            "MemFree",
            "Buffers",
            "Cached",
            "SwapCached",
            "Active",
            "Inactive",
            "SwapTotal",
            "SwapFree",
            "VmallocTotal",
            "VmallocUsed",
            "VmallocChunk"
    }; 

    S8 ascProcInfo[][MAX_CHARACTOR_LENGTH_IN_LINE] = {
            "VmRSS",
            "VmData",
            "VmStk",
            "VmExe",
            "VmLib",
            "VmPTE"
    }; 

    if((NULL == scInfo) || (0 == ulLength))
    {
        pl_log(ERR, "pf_get_sysinfo input NULL");  
        PS_CPlus(CM_PES, CMPES_ID_GET_SYSINFO_IN_NULL);
        return PF_RET_FAILURE;
    }

    pf_set_system_call((const S8*)"cat /proc/meminfo > /tmp/meminfo");

    pf_memset(scInfo, 0, ulLength);

    if(PF_RET_SUCCESS != pf_read_flush_file_in_line(        \
            (const S8*)TMP_SYSTEM_MEMINFO_PATH,             \
            (const S8*)ascMemInfo,                          \
            sizeof(ascMemInfo)/MAX_CHARACTOR_LENGTH_IN_LINE,\
            (S8*)scInfo,                                    \
            ulLength))
    {
        pl_log(ERR, "pf_get_sysinfo meminfo error");  
        PS_CPlus(CM_PES, CMPES_ID_GET_SYSINFO_READ_MEMINFO_FAIL);
        return PF_RET_FAILURE;
    }

    ulTotalLen = strlen((char*)scInfo);
    
    pf_memset(ascPath, 0, sizeof(ascPath));
    sprintf((char*)ascPath, "cat /proc/%d/status > /tmp/status", getpid());

    pf_set_system_call(ascPath);

    //printf("ascPath is %s\npid is %d and ppid is %d\n", ascPath, getpid(), getppid());

    if(PF_RET_SUCCESS != pf_read_flush_file_in_line(            \
            (const S8*)TMP_PROC_MEMINFO_PATH,                   \
            (const S8*)ascProcInfo,                             \
            sizeof(ascProcInfo)/MAX_CHARACTOR_LENGTH_IN_LINE,   \
            (S8*)(scInfo + ulTotalLen),                         \
            ulLength - ulTotalLen))
    {
        pl_log(ERR, "pf_get_sysinfo procinfo error");  
        PS_CPlus(CM_PES, CMPES_ID_GET_SYSINFO_READ_STATUS_FAIL);
        return PF_RET_FAILURE;
    }

    //pl_log(UINF, "pf_get_sysinfo is %s", scInfo);  

    unlink("/tmp/meminfo");
    unlink("/tmp/status");

    return PF_RET_SUCCESS;  
}

/**********************************************************************************************
 * @API function  pf_auth_get_id_info
 * @brief         ��ȡ��վ��Ȩ״̬�Ľӿ�
 * @input         ulAuthInfoId    ����ѯ��Ȩ��Ϣ������ֵ
 * @output        void
 * @return        0-δ��Ȩ
                  other-����Ȩ
 *********************************************************************************************/
extern "C" U32 pf_auth_get_id_info(U32 ulAuthInfoId)
{
    U32 ulAuthInfo = TRUE;
#ifdef PS_AUTHORIZATION_ENABLE
    U32 ulLevel = 0;
    U32 ulRet = pf_auth_get_level_info(&ulLevel);
    if(PF_RET_SUCCESS != ulRet)
    {
        pl_log(ERR, "%s: SC NO AUTHORIZATION", __FUNCTION__);
        return FALSE;
    }
    
    if(ulAuthInfoId >= PF_AUTH_INFO_MAX)
    {
        pl_log(ERR, "%s: authorization id %d exceed", __FUNCTION__, ulAuthInfoId);
        return FALSE;
    }

    ulAuthInfo = (ulLevel >> ulAuthInfoId) & 1;
#endif
    pl_log(TRC, "%s: authorization id %d info is %d", __FUNCTION__, ulAuthInfoId, ulAuthInfo);
    
    return ulAuthInfo;
}



void pf_set_reboot_system()
{
    CHAR* pcBootupFlag = "BOOTUP_FLAG_REBOOT";
    if (geteuid() == 0)//Judge current user authority  0:root
    {
        printf("Run as root, euid:%ld\n", (long) geteuid());  

        /*write the memory log into the file*/
        pf_log_write_file();

        /*writing reboot info into the file*/
        pf_write_root_path_file((const S8 *)PF_BOOTUP_FILE_PATH, (const S8 *)pcBootupFlag, strlen(pcBootupFlag));
        pf_set_system_call((const S8*)"reboot");
    }
    else
    {
        CHAR acEncPath[MAX_VERSION_LENGTH];
        CHAR acSrcPath[MAX_VERSION_LENGTH];
        S8 pscData[MAX_VERSION_LENGTH];
        S8 pscCMD[MAX_VERSION_LENGTH];
        U32 ulLength;
        
        pf_memset(acEncPath, 0, MAX_VERSION_LENGTH);
        pf_memset(acSrcPath, 0, MAX_VERSION_LENGTH);
        pf_memset(pscData, 0, MAX_VERSION_LENGTH);
        pf_memset(pscCMD, 0, MAX_VERSION_LENGTH);
        
        sprintf((CHAR*)acEncPath, "%s/config/bootup/user_password_crypt", acRootPath);
        sprintf((CHAR*)acSrcPath, "%s/config/bootup/user_password", acRootPath);


        if(pf_is_file_exist((S8 *)acEncPath))
        {
            if( TRUE !=pf_cipher_decrypt_file(acEncPath,acSrcPath))
            {
                pl_log(ERR,"cipher decrypt file fail %s",acEncPath);
            }
            if(PF_RET_SUCCESS != pf_get_file_length((S8*)acSrcPath, &ulLength))
            {
                 pl_log(ERR, "get file %s length fail",acSrcPath);
            }
                           
            if( PF_RET_SUCCESS !=pf_read_flush_file((const S8 *)acSrcPath, pscData,  ulLength))
            {
                pl_log(ERR,"read file fail %s",acSrcPath);
            }

            /*write the memory log into the file*/
            pf_log_write_file();

            /*writing reboot info into the file*/
            pf_write_root_path_file((const S8 *)PF_BOOTUP_FILE_PATH, (const S8 *)pcBootupFlag, strlen(pcBootupFlag));

            unlink(acSrcPath);
            sprintf((CHAR*)pscCMD, "echo %s |sudo -S reboot", pscData);
            if( PF_RET_SUCCESS !=pf_set_system_call(pscCMD))
            {
                pl_log(ERR,"reboot fail");
            }
        }
        else 
        {
            if(pf_is_file_exist((S8 *)acSrcPath))
            {
                if(PF_RET_SUCCESS != pf_get_file_length((S8*)acSrcPath, &ulLength))
                {
                    pl_log(ERR, "get file %s length fail",acSrcPath);
                }  
            
                if( PF_RET_SUCCESS !=pf_read_flush_file((const S8 *)acSrcPath, pscData,  ulLength))
                {
                    pl_log(ERR,"read file fail %s",acSrcPath);
                }

                if( TRUE !=pf_cipher_encrypt_file(acSrcPath,acEncPath))
                {
                    pl_log(ERR,"cipher encrypt file fail %s",acEncPath);
                }

                /*write the memory log into the file*/
                pf_log_write_file();

                /*writing reboot info into the file*/
                pf_write_root_path_file((const S8 *)PF_BOOTUP_FILE_PATH, (const S8 *)pcBootupFlag, strlen(pcBootupFlag));

                unlink(acSrcPath);
                
                sprintf((CHAR*)pscCMD, "echo %s |sudo -S reboot", pscData);
                if( PF_RET_SUCCESS !=pf_set_system_call(pscCMD))
                {
                    pl_log(ERR,"reboot fail");
                }                   
            }
            pl_log(ERR,"no user_password file so can not reboot as user");
            
        }
    }
}

S32 pf_set_reboot_watchdog()
{
    int fd;
    int value;
    int timeout;
    
    fd = open("/dev/watchdog",O_RDWR);/*when open watchdog , enable watchdog*/
    if(fd < 0) 
    {      
        pl_log(ERR,"watchdog reboot fail ,start system reboot");
        return PF_RET_FAILURE;
    }

    ioctl(fd , WDIOC_GETTIMEOUT,&timeout);    

    timeout = 60;
    if(ioctl(fd , WDIOC_SETTIMEOUT ,&timeout) != 0) 
    {       
        perror("SET");     
        close (fd);    
        return PF_RET_FAILURE;
    }      

    close (fd);
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_set_warm_reset
 * @brief         the interface of system reboot
 * @input         ulFlag        the flag of reset
                                0 - reset the process
                                1 - reboot the system
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_set_warm_reset(U32 ulFlag)
{
    pf_set_reboot_time();

    if(WARM_RESET_SYSTEM == ulFlag)
    {
        pf_set_reboot_watchdog();
        pf_set_reboot_system();
    }
    else
    {
        CHAR* pcBootupFlag = "BOOTUP_FLAG_PROCESS_EXIT";

        /*writing reboot info into the file*/
        pf_write_root_path_file((const S8 *)PF_BOOTUP_FILE_PATH, (const S8 *)pcBootupFlag, strlen(pcBootupFlag));

        pl_log(INF,"WARM_RESET_PROCESS");
        pf_log_write_file();
        for(int i = 0;i < MODULE_TASK_MAX;i++)
        {
            msgQArray[i] = NULL;
            moduleArray[i] = NULL;
        }
        _exit(0);
    }
}


extern stCpuCoreMask aulModuleCpucoreMask[];

/**********************************************************************************************
 * @API function  pf_get_thread_cpucore_cfg
 * @brief         get thread cpucore config from the input file 
 * @input         scPath     The path of file
 * @output        gloable array  aulModuleCpucoreMask
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
extern "C" int pf_get_thread_cpucore_cfg(const S8* scPath)
{
    //read config file
    U32 ulFileLen = 0;
    if(PF_RET_SUCCESS != pf_get_root_path_file_length(scPath,&ulFileLen))
    {
        pl_log(INF,"get config file(%s) len failed ",scPath);
        return PF_RET_FAILURE;
    }

    if(0 != ulFileLen)
    {
        S8* pTmpMemory = (S8*)pf_malloc(ulFileLen);
        thread_cpucore_cfg::config_list CpucoreConfigList;
        U32 ulMid;
        if(PF_RET_SUCCESS != pf_read_root_path_file(scPath,(const S8*)pTmpMemory,ulFileLen))
        {
            pl_log(ERR,"read config file(%s) failed ",scPath);
            return PF_RET_FAILURE;
        }

        json2pb(CpucoreConfigList, (const char*)pTmpMemory, ulFileLen);
        pf_free(pTmpMemory);
        pTmpMemory = NULL;
                                                

        for(U32 idx = 0; idx < CpucoreConfigList.astcpucpremasklist_size(); idx++)
        {
            thread_cpucore_cfg::config_data* pCfg = CpucoreConfigList.mutable_astcpucpremasklist(idx);
            ulMid = pf_get_module_id(pCfg->aucmodulename().c_str());
            
            if(pCfg->has_ulcpucoremask0())
            {
                aulModuleCpucoreMask[ulMid].mask0 = pCfg->ulcpucoremask0();
            }
            else
            {
                aulModuleCpucoreMask[ulMid].mask0 = 0;
            }

            if(pCfg->has_ulcpucoremask1())
            {
                aulModuleCpucoreMask[ulMid].mask1 = pCfg->ulcpucoremask1();
            }
            else
            {
                aulModuleCpucoreMask[ulMid].mask1 = 0;
            }

            if(pCfg->has_ulcpucoremask2())
            {
                aulModuleCpucoreMask[ulMid].mask2 = pCfg->ulcpucoremask2();
            }
            else
            {
                aulModuleCpucoreMask[ulMid].mask2 = 0;
            }

            if(pCfg->has_ulcpucoremask3())
            {
                aulModuleCpucoreMask[ulMid].mask3 = pCfg->ulcpucoremask3();
            }
            else
            {
                aulModuleCpucoreMask[ulMid].mask3 = 0;
            }

        }

    }

    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_sig_handler
 * @brief         Used to handle signal and output backtrace 
 * @input         none
 * @output        none
 * @return        none                  
 *********************************************************************************************/
extern "C" void pf_sig_handler(int sig)
{
    int  nptrs;
    void *buffer[MAX_VERSION_LENGTH];
    int fd;

    CHAR* pcBootupFlag = "BOOTUP_FLAG_KILL";

    /*writing reboot info into the file*/
    pf_write_root_path_file((const S8 *)PF_BOOTUP_FILE_PATH, (const S8 *)pcBootupFlag, strlen(pcBootupFlag));

    pl_log(DO_NOT_USE, "RECEIVED signal ID is %d", sig);

    pf_log_write_file();

    fd = open(pf_get_log_path(),O_WRONLY | O_CREAT | O_APPEND,0600);
    nptrs = backtrace(buffer, MAX_VERSION_LENGTH);
    printf("backtrace() returned %d addresses\n", nptrs);

    backtrace_symbols_fd(buffer,nptrs,fd);
    close(fd);

}

void pf_sig_handler_exit(int sig)
{
   pf_sig_handler(sig);
   _exit(0);
}

/**********************************************************************************************
 * @API function  pf_signal_backtrace
 * @brief         start to set signal and output backtrace function
 * @input         none
 * @output        none
 * @return        none
                  
 *********************************************************************************************/
extern "C" void pf_signal_backtrace()
{
    //signal(SIGSEGV, pf_sig_handler_exit);

    signal(SIGPIPE, SIG_IGN);
}


/**********************************************************************************************
 * @API function  pf_set_sys_init
 * @brief         initialization interface of platform called after start-up
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_set_sys_init(void)
{
//    pf_daily_record_set_booting();
    
    pf_set_bootup_time();
    pf_signal_backtrace();
}


/**********************************************************************************************
 * @API function  pf_get_diskinfo_size
 * @brief         get disk totalsize and free size 
 * @input         scPath     disk path
                  pulTotal   disk total size
                  pulFree    disk free size
                  unit       unit for disk     eg:DISK_INFO_UNIT_KB/DISK_INFO_UNIT_MB/DISK_INFO_UNIT_GB
 * @output        pulTotal   disk total size
                  pulFree    disk free size
 * @return        0: ok
                  other: failure
 *********************************************************************************************/

S32 pf_get_diskinfo_size(const S8* scPath,U64 *pulTotal,U64 *pulFree,U32 unit)
{
    struct statfs diskInfo;  
    U64 totalBlocks;
    U64 totalSize;
    U64 freeDisk;
    
    if(statfs((char *)scPath, &diskInfo) < 0)
    {
        pl_log(ERR,"statfs error");
        return PF_RET_FAILURE;
    }
    
    totalBlocks = diskInfo.f_bsize;  
    totalSize = totalBlocks * diskInfo.f_blocks;  
    freeDisk = diskInfo.f_bavail*totalBlocks;  

    if(unit == DISK_INFO_UNIT_KB)
    {
         *pulTotal = totalSize/1024;
         *pulFree = freeDisk/1024;
    }
    else if(unit == DISK_INFO_UNIT_MB)
    {
         *pulTotal = totalSize/1048576;
         *pulFree = freeDisk/1048576;

    }
    else if(unit == DISK_INFO_UNIT_GB)
    {
         *pulTotal = totalSize/1073741824;
         *pulFree = freeDisk/1073741824;
    }
    
    return PF_RET_SUCCESS;
}


