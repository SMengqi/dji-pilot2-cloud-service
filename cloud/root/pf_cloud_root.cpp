/*******************************************************************************************************************
 Copyright (C), BroadXt Inc  2019
         All    Rights Reserved.
 FileName: pf_root.cpp
 Author: josephzhou    Version :  1.0    Date: 20190617
 Description:      contains the platform api and main entry
 Function List:
   1. -------
 History:
 <author>       <time>           <version >    <desc>
 josephzhou    2019/6/17         1.0        initial
 ******************************************************************************************************************/
#define THIS_MODULE PLATFORM_ROOT
/* include files*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <stdarg.h>


#include "../../common/pl.h"
#include "../../common/pl_comm.h"
#include "../../common/event.h"
#include "../../common/module.h"
#include "../../common/if_pl/pf_daily_record.h"
#include "osport.h"
#include "os.h"
#include "pf_thread_mon.h"
#include "pf_mbox.h"
#include "pf_upgrade.h"

#include "pf_stat.h"
#include <signal.h>
#include "pl.h"
#include "pf_map_block.h"

pid_t mainPid = 0;
extern pf_mbox_t msgQArray[];
extern MODULE_ENTRY moduleArray[];
extern MODULE_INIT moduleInitArray[];
extern pf_thread_t  workerThreads[];
extern pf_handle_t  workerhandles[];

void Signal_handler(int signal)
{
    pf_sig_handler(signal);
    _exit(0);
}

DECLMODULE_OWNERENTRY(log,  8388608)

DECLTASK(pf_timer,          8388608)
DECLTASK(network,           8388608)
DECLMODULE(daily_record,    8388608)

DECLMODULE(lcf,             8388608)
DECLTASK(mqttsub,           8388608)
DECLMODULE(mqttpub,         8388608)
DECLMODULE(device,          8388608)
DECLMODULE(flyto,           8388608)
DECLMODULE(track,           8388608)
DECLMODULE(payload,         8388608)

extern void thread_mid_init(void);
extern void NetlibInit(void);


//#define MODULE_COMBINED_OPEN    //���̺߳ϲ�����

/**********************************************************************************************
 * @API function  main
 * @brief         the entry of main
                  main�����������̣�
                  log->modules��MAC/RLC/PDCP/S1AP/RRC/RRM/OAM��->timer->MC->l1drv
 * @input         argc          Ϊ������API�ӿڵĲ���argv�е��ַ�������
                  argv          �ַ�������ָ�룬���������δ�����е��ַ���ָ��
 * @output        void
 * @return        0
 *********************************************************************************************/
extern "C" int main(int argc, char *argv[])
{
    U32 ulTicks;
    
    if(argc > 1)
    {
        pf_set_root_path((const S8 *)argv[1]);
    }
    printf("module max %d, module total num %d\n", MODULE_MAX, MODULE_TOTAL_NUM);
    if(MODULE_MAX <= MODULE_TOTAL_NUM)
    {
        printf("MODULE_MAX=%d <= MODULE_TOTAL_NUM=%d, please rebuild platform lib\r\n", MODULE_MAX, MODULE_TOTAL_NUM);
        ASSERT(0);
    }

    pf_sharedMemory_init();
    thread_mid_init();
    pf_ticks_init();
    pf_thread_mon_init();
    PS_CInit;
    ulTicks = pf_get_ticks_us();
    PS_CSet(CM_TMP, 0, ulTicks);
    mainPid = getpid();
    pf_stat_thread_name_init("DrPsDrc", mainPid);
    pl_commbuf_init();
    NetlibInit();
    pf_get_thread_cpucore_cfg((const S8*)THREAD_CPUCORE_CFG_PATH);

#ifdef MEMORY_IN_MEMPOOL
    U32 ret = pf_mem_pool_init();
    if(0 != ret)
    {
        ASSERT(0);
    }
#endif

    CREATETASK(70,      TIMER,     pf_timer);
    //dont forget call STARTMODULE_OWNERENTRY
    CREATEMODULE_OWNERENTRY(0, LOG, log); // ���ģ�鲻ʹ����ͨ��msg_entry
    STARTMODULE_OWNERENTRY(LOG);
    signal(SIGTERM, Signal_handler);
    pf_set_sys_init();

    pf_set_module_flag(PLATFORM_EX,     WARN);
    pf_set_module_flag(MODULE_NETWORK,  WARN);

    pf_set_module_flag(MODULE_LCF,      INF);
    pf_set_module_flag(MODULE_MQTTSUB,  INF);
    pf_set_module_flag(MODULE_MQTTPUB,  INF);
    
    pf_set_module_flag(MODULE_DEVICE,   INF);
    pf_set_module_flag(MODULE_FLYTO,    INF);
    pf_set_module_flag(MODULE_TRACK,    INF);
    pf_set_module_flag(MODULE_PAYLOAD,  TRC);

    pf_get_sc_version();
    //CREATEMODULE(0,     FTP,           ftp);
    CREATEMODULE(0,     DAILYREC,  daily_record);
    CREATETASK(20,      NETWORK,   network);

    CREATEMODULE(0,     LCF,         lcf);
    pf_usleep(1500000);
    CREATEMODULE(2,     MQTTPUB,     mqttpub);
    CREATEMODULE(3,     DEVICE,      device);
    CREATEMODULE(4,     FLYTO,       flyto);
    CREATEMODULE(5,     TRACK,       track);
    CREATEMODULE(6,     PAYLOAD,     payload);
    CREATETASK(1,       MQTTSUB,     mqttsub);

  
  //main thread
    while (1)
    {
        pf_usleep(3600000000);
    }

    return 0;

}



