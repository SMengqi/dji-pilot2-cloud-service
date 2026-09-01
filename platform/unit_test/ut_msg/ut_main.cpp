/*******************************************************************************************************************
 **                                                                                                                        
 **  Copyright (c)  2009,  Innofidei, Inc.                                                                                 
 **        All    Rights Reserved.                                                                                            
 **                                                                                                                          
 **  Subsystem    : LTE/UE                                                                                             
 **  File        : main.c                                                                                       
 **  Created By    : Wenson                                                                                              
 **  Created On    : 09/12/24                                                                                                 
 **                                                                                                                         
 **  Purpose:                                                                                                             
 **    This file    contains the platform api and main entry
 **                                                                                                                         
 **  History:                                                                                                             
 **  Programmer        Date    Rev    Description                                                                                 
 **  --------------- ---------- --------    ------------------------------                                                   
 **
 ******************************************************************************************************************/
#define THIS_MODULE PLATFORM_EX
/* include files*/
#include "../common/pl.h"
#include "../common/pl_comm.h"
#include "../common/event.h"
#include "../common/module.h"
#include "../common/errid.h"
#include "../common/option.h"
#include "osport.h"
#include "os.h"
#include "pf_mbox.h"

#include "unit_test.h"
#include "pf_stat.h"
#include "network/net_buffer.h"

#include <sys/time.h>
#include <string.h>
#include "pf_rdkafka.h"
#include  <iostream> 


using namespace std;

#define pl_dbg_ut_main  pl_dbg_ut

#define DEFAULT_BUFFER_SIZE 33500000 

uint8_t u8Buf[DEFAULT_BUFFER_SIZE];

void buf_right_test()
{
    U32 i=1;
    uint32_t size;
    uint8_t* buf;
    int ret;
    int count;
    
    CBuffer* m_sendBuffer = new(std::nothrow) CBuffer(1000);
    pl_dbg_ut_main("-------------%s  START -----------------\n", __FUNCTION__);

    for(i=1;i<20;i++)
    {
        pf_memset(u8Buf,i,sizeof(u8Buf));
        m_sendBuffer->AddTcpBuffer((const uint8_t *) u8Buf, 50);
        m_sendBuffer->printInfo("Add80 50", i);
    }

    for(i = 1;i < 20 ;i++)
    {
        m_sendBuffer->printInfo("debug", i);
        size = m_sendBuffer->GetTcpLength();
        buf = m_sendBuffer->GetTcpBuffer();
        pf_memset(u8Buf,i,sizeof(u8Buf));
        ret = memcmp(buf,u8Buf,50);  
        if(ret != PF_RET_SUCCESS)
        {
            printf("buf_right_test error  i:%d %s %d \n",i,__FUNCTION__,__LINE__);        
        }
        m_sendBuffer->ReleaseTcpBuffer(50);
    }

    for(i=1;i<20;i++)
    {
        pf_memset(u8Buf,i,sizeof(u8Buf));
        m_sendBuffer->AddTcpBuffer((const uint8_t *) u8Buf, 50);
        m_sendBuffer->printInfo("Add80 50", i);
    }
    
    for(i = 1;i < 20 ;i++)
    {
        m_sendBuffer->printInfo("debug", i);
        size = m_sendBuffer->GetTcpLength();
        buf = m_sendBuffer->GetTcpBuffer();
        pf_memset(u8Buf,i,sizeof(u8Buf));
        ret = memcmp(buf,u8Buf,50);  
        if(ret != PF_RET_SUCCESS)
        {
            printf("buf_right_test error  i:%d %s %d \n",i,__FUNCTION__,__LINE__);        
        }
        m_sendBuffer->ReleaseTcpBuffer(50);
    }
    
}

void bufTest()
{
    U32 i=1;
    CBuffer* m_sendBuffer = new(std::nothrow) CBuffer( DEFAULT_BUFFER_SIZE );
    pl_dbg_ut_main("-------------%s  START -----------------\n", __FUNCTION__);

    for(i=1;i<80;i++)
    {
        pf_memset(u8Buf,i,sizeof(u8Buf));
        m_sendBuffer->AddTcpBuffer((const uint8_t *) u8Buf, 500000);
        m_sendBuffer->printInfo("Add80 500000", i);
    }

    for(i=1;i<50;i++)
    {
    	m_sendBuffer->ReleaseTcpBuffer(500000);
    	m_sendBuffer->printInfo("Rel50 100000", i);
    }

    for(i=1;i<60;i++)
    {
        pf_memset(u8Buf,i,sizeof(u8Buf));
        m_sendBuffer->AddTcpBuffer((const uint8_t *) u8Buf, 500000);
        m_sendBuffer->printInfo("Add60 500000", i);
    }

    for(i=1;i<68;i++)
    {
    	 m_sendBuffer->ReleaseTcpBuffer(500000);
        m_sendBuffer->printInfo("Rel68 500000", i);
    }

    for(i=1;i<20;i++)
    {
    	m_sendBuffer->AddTcpBuffer((const uint8_t *) u8Buf, 500000);
    	m_sendBuffer->printInfo("Add20 500000", i);
    }

}

extern void message_test_main(void);
extern void get_time(void);
extern void flyto_controller_test_main(void);

pid_t mainPid = 0;

DECLTASK(test_thread, 131072)/*���������128k��*/

void test_thread_init(void)
{
    pl_dbg_ut_main("-------------%s START-----------------\n", __FUNCTION__);
    pl_dbg_ut_main("-------------%s  END -----------------\n", __FUNCTION__);
}

static U32 totalCase = 0;
static U32 totalCaseSuc = 0;
static U32 totalCaseFail = 0;

void test_case_result_output(char* testCaseName, U32 module, U32 result)
{
    char localModuleName[TEST_CASE_MODULE_END][40] = {
        "THREADS UNITTEST",
        "MEMORY  UNITTEST",
        "MESSAGE UNITTEST",
        "TIMER   UNITTEST",
        "CYCLES  UNITTEST",
        "LOG     UNITTEST"
    };
    totalCase++;
    if(result)
    {
        totalCaseFail++;
        pl_dbg_ut_main("%s:TESTCASENAME(%2d):%20s:%40s FAILED!!!\n", __FUNCTION__, module, localModuleName[module], testCaseName);
    }
    else
    {
        totalCaseSuc++;
        pl_dbg_ut_main("%s:TESTCASENAME(%2d):%20s:%40s SUCCESS!!\n", __FUNCTION__, module, localModuleName[module], testCaseName);
    }

    pl_dbg_ut_main("%s:TOTALCASE:%d, FAILED:%d(rate:%d), SUCCUSS:%d(rate:%d)!\n", __FUNCTION__, totalCase, totalCaseFail, totalCaseFail*100/totalCase, totalCaseSuc, totalCaseSuc*100/totalCase);
}

void test_thread_entry(U32 priority)
{
    FUNCTION_TRACE;
    pl_dbg_ut_main("=======================================================\n");
    pl_dbg_ut_main("MAIN ULCONT  UNIT TEST\n");
    pl_dbg_ut_main("=======================================================\n");

    get_time();
    message_test_main();
    flyto_controller_test_main();
    get_time();

}
/*
string m_status;
U32 ulLen = 0;
S8 ascPath[200];
S8 *pscData = NULL;
string stats_test;
long time_once_total_now = 0, time_once_avg_now = 0;
long time_ten_total_now = 0, time_ten_avg_now = 0;
long time_tsz_total_now = 0, time_tsz_avg_now = 0;
long time_one_thousand_total_now = 0, time_one_thousand_avg_now = 0;
long time_esfz_total_now = 0, time_esfz_avg_now = 0;

long time_once_total_pre = 0, time_once_avg_pre = 0;
long time_ten_total_pre = 0, time_ten_avg_pre = 0;
long time_tsz_total_pre = 0, time_tsz_avg_pre = 0;
long time_one_thousand_total_pre = 0, time_one_thousand_avg_pre = 0;
long time_esfz_total_pre = 0, time_esfz_avg_pre = 0;

static int pl_debug_rdproducer_stats_full_log_on = 0;

static int print_producer_throughput_now(std::string str_stats)
{  
    if (pl_debug_rdproducer_stats_full_log_on)
    {
        pl_log(INF, "--- rd producer statistics: %s", str_stats.c_str());
        return 0;
    }   
     m_status = str_stats;
}

static int print_producer_throughput_pre(std::string str_stats)
{  
    if (pl_debug_rdproducer_stats_full_log_on)
    {
        pl_log(INF, "--- rd producer statistics: %s", str_stats.c_str());
        return 0;
    } 
    
    string::size_type p;
    unsigned long long txmsgs;
    std::string topic_name;
    std::string producer_name;
    std::string str_tmp; 

    if ((p = str_stats.find("\"topic\":\"")) != std::string::npos)
    {
        str_tmp = str_stats.substr(p + strlen("\"topic\":\""));
        topic_name = str_tmp.substr(0, str_tmp.find("\""));
    }

    if ((p = str_stats.find("\"client_id\":")) != std::string::npos)
    {
        str_tmp = str_stats.substr(p + strlen("\"client_id\":"));
        producer_name = str_tmp.substr(0, str_tmp.find(","));
    }

    // 1. Total send messages
    if((p = str_stats.rfind("\"txmsgs\":")) != 0)
    {
        txmsgs = strtoull(str_stats.c_str() + p + strlen("\"txmsgs\":"), NULL, 10);
        pl_log(INF, "%s[ %s]: txmsgs ( %lld)", producer_name.c_str(), topic_name.c_str(), txmsgs);
    }

    // 2. Total send bytes
    unsigned long long txmsg_bytes;
    if((p = str_stats.rfind("\"txmsg_bytes\":")) != 0)
    {
        txmsg_bytes = strtoull(str_stats.c_str() + p + strlen("\"txmsg_bytes\":"), NULL, 10);
        pl_log(INF, "%s[ %s]: txmsg_bytes ( %lld)", producer_name.c_str(), topic_name.c_str(), txmsg_bytes);
    }
	   
    return 0;
}


void test_print_producer_throughput(void)
{
    struct timeval tv;
    long time_nex = 0, time_now = 0;
    long time_total = 0;
    int i =0;

    cout <<  endl << ">>>Call once" << endl;
    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 1; i++)
    {    
        print_producer_throughput_pre(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_once_total_pre += time_total;
    cout << "print_producer_throughput_pre time consumed is "<< time_total << "(us)" << endl;

    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 1; i++)
    {    
        print_producer_throughput_now(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_once_total_now += time_total;
    cout << "print_producer_throughput_now time consumed is "<< time_total << "(us)" << endl;

    cout <<  endl << ">>>Call 10 times" << endl;
    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 10; i++)
    {    
        print_producer_throughput_pre(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_ten_total_pre+= time_total;
    cout << "print_producer_throughput_pre time consumed is "<< time_total << "(us)" << endl;

    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 10; i++)
    {    
        print_producer_throughput_now(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_ten_total_now += time_total;
    cout << "print_producer_throughput_now time consumed is "<< time_total << "(us)" << endl;

    cout << endl << ">>>Call 360 times" << endl;
    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 360; i++)
    {    
        print_producer_throughput_pre(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_tsz_total_pre += time_total;
    cout << "print_producer_throughput_pre time consumed is "<< time_total << "(us)" << endl;

    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 360; i++)
    {    
        print_producer_throughput_now(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_tsz_total_now += time_total;
    cout << "print_producer_throughput_now time consumed is "<< time_total << "(us)" << endl;

    cout <<  endl << ">>>Call 1000 times" << endl;
    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 1000; i++)
    {    
        print_producer_throughput_pre(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_one_thousand_total_pre += time_total;
    cout << "print_producer_throughput_pre time consumed is "<< time_total << "(us)" << endl;

    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 1000; i++)
    {    
        print_producer_throughput_now(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_one_thousand_total_now += time_total;
    cout << "print_producer_throughput_now time consumed is "<< time_total << "(us)" << endl;

    cout <<  endl << ">>>Call 8640 times" << endl;
    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 8640; i++)
    {    
        print_producer_throughput_pre(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_esfz_total_pre += time_total;
    cout << "print_producer_throughput_pre time consumed is "<< time_total << "(us)" << endl;

    gettimeofday(&tv, NULL);
    time_nex = tv.tv_sec * 1000000 + tv.tv_usec;
    for(i = 0; i < 8640; i++)
    {    
        print_producer_throughput_now(stats_test);
    }
    gettimeofday(&tv, NULL);
    time_now = tv.tv_sec * 1000000 + tv.tv_usec;
    time_total = time_now - time_nex;
    time_esfz_total_now += time_total;
    cout << "print_producer_throughput_now time consumed is "<< time_total << "(us)" << endl;

}

*/

void delay_ms(U32 ulms)
{
    usleep(ulms*1000);    
}

extern U32 pf_mem_pool_init(void);
extern void thread_mid_init(void);
extern void pf_thread_mon_init(void);

extern MODULE_ENTRY moduleArray[];
extern int pro_main(int argc, char* argv[]);
extern int con_main(int argc, char* argv[]);

extern int pro_group_main(int argc, char* argv[]);
extern int con_group_main(void);

DECLMODULE_OWNERENTRY(log,8388608);
DECLTASK(pf_timer,8388608);


int main(int argc, char* argv[])
{
    int ulConut = 0;
    int flag = 0;
    
    U32 ulTicks;
    char* pro_argv = "producer";
    char* con_argv = "consumer"; 
    char* pro_argv_group = "producer_group";
    char* con_argv_group = "consumer_group"; 
//    int l = 0;

    if(argc == 2)
    {
        if(strcmp(argv[argc-1], con_argv_group))
        {
            pl_dbg_ut_main("Wrong input of command line parameters\nIt should be entered like this:\n./XXX consumer_group");
        }
    }

    if(argc >2)
    {
        if(strcmp(argv[argc-1], pro_argv_group))
        {
            pl_dbg_ut_main("Wrong input of command line parameters\nIt should be entered like this:\n./XXX record-count=XXX producer_group\n");
            return PF_RET_FAILURE;
        }
    }
    
    if(argc > 3)
    {
        if(strcmp(argv[argc-1], pro_argv) && strcmp(argv[argc-1], con_argv))
        {
            pl_dbg_ut_main("Wrong input of command line parameters\nIt should be entered like this:\n./XXX record-count=XXX record-size=XXX topic=XXX brokers=XXX.XX.XX.XX:XXXX <producer or consumer>\n");
            return PF_RET_FAILURE;
        }
    }
  /*  
    pf_memset(ascPath, 0, sizeof(ascPath));
    sprintf((char*)ascPath, "/home/songmengqi/projects/kafka-test/MR1484_distributed_nacosAPI_restful_dr_drc_3.7.1456_Release-build-3744-pl/platform/unit_test/ut_msg/test/produce.txt");

    pf_get_file_length((S8 *)ascPath, &ulLen);
    pscData = (S8*)pf_malloc(ulLen);
    pf_read_flush_file((const S8 *)ascPath, (const S8 *)pscData, ulLen);
    //cout << ulLen  << endl;
    stats_test = (char *)pscData;

    if(!strcmp(argv[1], "time"))
    {
        for(l = 0; l < 20; l++)
        {
            test_print_producer_throughput();
        }

        time_once_avg_pre = time_once_total_pre/20;
        time_once_avg_now = time_once_total_now/20;
        cout <<  endl << "Execute 11 times>>>Call once" << endl; 
        cout << "print_producer_throughput_pre average loss time : " << time_once_avg_pre << "(us)" << endl;
        cout << "print_producer_throughput_now average loss time : " << time_once_avg_now << "(us)" << endl;
        
        time_ten_avg_pre = time_ten_total_pre/20;
        time_ten_avg_now = time_ten_total_now/20;
        cout <<  endl << "Execute 11 times>>>Call 10 times" << endl;
        cout << "print_producer_throughput_pre average loss time : " << time_ten_avg_pre << "(us)" << endl;
        cout << "print_producer_throughput_now average loss time : " << time_ten_avg_now << "(us)" << endl;
        
        time_tsz_avg_pre = time_tsz_total_pre/20;
        time_tsz_avg_now = time_tsz_total_now/20;
        cout <<  endl << "Execute 11 times>>>Call 360 times" << endl;
        cout << "print_producer_throughput_pre average loss time : " << time_tsz_avg_pre << "(us)" << endl;
        cout << "print_producer_throughput_now average loss time : " << time_tsz_avg_now << "(us)" << endl;
        
        time_one_thousand_avg_pre = time_one_thousand_total_pre/20;
        time_one_thousand_avg_now = time_one_thousand_total_now/20;
        cout <<  endl << "Execute 11 times>>>Call 1000 times" << endl;
        cout << "print_producer_throughput_pre average loss time : " << time_one_thousand_avg_pre << "(us)" << endl;
        cout << "print_producer_throughput_now average loss time : " << time_one_thousand_avg_now << "(us)" << endl;
        
        time_esfz_avg_pre = time_esfz_total_pre/20;
        time_esfz_avg_now = time_esfz_total_now/20;
        cout <<  endl << "Execute 11 times>>>Call 8640 times" << endl;
        cout << "print_producer_throughput_pre average loss time : " << time_esfz_avg_pre << "(us)" << endl;
        cout << "print_producer_throughput_now average loss time : " << time_esfz_avg_now << "(us)" << endl;
        
        return PF_RET_SUCCESS;
    }
  */
    
    pf_sharedMemory_init();
    thread_mid_init();
    /*Get the initial time of the system*/
    pf_ticks_init();
    /*initialize thread monitor service*/
    pf_thread_mon_init();
    /*Initialize counters for all modules*/
    PS_CInit;
    ulTicks = pf_get_ticks_us();
    /*The value of index of module mid is set to value*/   
    PS_CSet(CM_TMP, 0, ulTicks);
    mainPid = getpid();
    /*Initialize global resources of commbuffer*/
    pl_commbuf_init();

#ifdef MEMORY_IN_MEMPOOL
    /*memory pool initial function*/
    U32 ret = pf_mem_pool_init();
    FUNCTION_TRACE;
    if(0 != ret)
    {
        ASSERT(0);
    }
#endif

    
    CREATETASK(70, TIMER, pf_timer);
    //dont forget call STARTMODULE_OWNERENTRY
    CREATEMODULE_OWNERENTRY(30, LOG, log); // ����������msg_entry
    STARTMODULE_OWNERENTRY(LOG);

    pl_dbg_ut_main("-------------%s START-----------------\n", __FUNCTION__);
/*
    pf_get_ticks_ns();
    pl_commbuf_init(); 
#ifdef MEMORY_IN_MEMPOOL    
    pf_mem_pool_init();
    FUNCTION_TRACE;
#endif    
*/
    get_time();
    pf_set_module_flag(PLATFORM_EX, UINF);

    /*Get the byte order of the current system, big-endian or little-endian*/
    flag = pf_get_big_endian_flag();
    if(flag)
    {
        pl_dbg_ut_main("CURRENT PLATFORM IS BIG ENDIAN\n");
    }
    else
    {
        pl_dbg_ut_main("CURRENT PLATFORM IS LITTLE ENDIAN\n");
    }
    
    if(argc > 3)
    {
        if(!strcmp(argv[argc-1], pro_argv))
        {
            pl_dbg_ut_main("-------------rd_producer START-----------------\n");
            pro_main(argc-1, argv);
            while(1);
            return PF_RET_SUCCESS;
        }
        else if(!strcmp(argv[argc-1], con_argv))
        {
            pl_dbg_ut_main("-------------rd_consumer START-----------------\n");
            con_main(argc-1, argv);
            while(1);
            return PF_RET_SUCCESS;
        }
    }
    if(argc >= 2)
    {
        if(!strcmp(argv[argc-1], pro_argv_group))
        {
            pl_dbg_ut_main("-------------producer_group START-----------------\n");
            pro_group_main(argc-1, argv);
            while(1);
            return PF_RET_SUCCESS;
        }
        else if(!strcmp(argv[1], con_argv_group))
        {
            pl_dbg_ut_main("-------------consumer_group START-----------------\n");
            con_group_main();
            while(1);
            return PF_RET_SUCCESS;
        }
    }

//    pl_dbg_ut_main("--------------TEST END---------------\n");
//    return PF_RET_SUCCESS;
    
   // bufTest();
    buf_right_test();
    
    pl_dbg_ut_main("event ID:%5d(0x%4x), info:%s\n", DRC_ACU_BEGIN_MSG,     DRC_ACU_BEGIN_MSG,      pf_get_event_name(DRC_ACU_BEGIN_MSG));
    pl_dbg_ut_main("event ID:%5d(0x%4x), info:%s\n", DRC_DRSU_BEGIN,        DRC_DRSU_BEGIN,         pf_get_event_name(DRC_DRSU_BEGIN));
    pl_dbg_ut_main("event ID:%5d(0x%4x), info:%s\n", DRC_DRSU_REG_RSP,      DRC_DRSU_REG_RSP,       pf_get_event_name(DRC_DRSU_REG_RSP));
    //pl_dbg_ut_main("event ID:%5d(0x%4x), info:%s\n", CBMP_INTER_BEGIN_MSG,  CBMP_INTER_BEGIN_MSG,   pf_get_event_name(CBMP_INTER_BEGIN_MSG));
    pl_dbg_ut_main("event ID:%5d(0x%4x), info:%s\n", PF_MC_READY_IND,       PF_MC_READY_IND,        pf_get_event_name(PF_MC_READY_IND));

    pl_dbg_ut_main("error ID:%5d(0x%4x), info:%s\n", RET_SUCCESS,                  RET_SUCCESS,                   pf_get_errid_name(RET_SUCCESS));
    pl_dbg_ut_main("error ID:%5d(0x%4x), info:%s\n", ERRID_OS_OPERATION_NOT_PERMITTED,  ERRID_OS_OPERATION_NOT_PERMITTED,   pf_get_errid_name(ERRID_OS_OPERATION_NOT_PERMITTED));
    pl_dbg_ut_main("error ID:%5d(0x%4x), info:%s\n", ERRID_OS_NO_SUCH_FILE_OR_DIRECORY, ERRID_OS_NO_SUCH_FILE_OR_DIRECORY,  pf_get_errid_name(ERRID_OS_NO_SUCH_FILE_OR_DIRECORY));
    pl_dbg_ut_main("error ID:%5d(0x%4x), info:%s\n", ERRID_DRC_ACU_BEGIN,               ERRID_DRC_ACU_BEGIN,                pf_get_errid_name(ERRID_DRC_ACU_BEGIN));
    pl_dbg_ut_main("error ID:%5d(0x%4x), info:%s\n", ERRID_DRC_DRSU_BEGIN,              ERRID_DRC_DRSU_BEGIN,               pf_get_errid_name(ERRID_DRC_DRSU_BEGIN));
    pl_dbg_ut_main("error ID:%5d(0x%4x), info:%s\n", ERRID_DRC_DRSU_ALARM_BEGIN,        ERRID_DRC_DRSU_ALARM_BEGIN,         pf_get_errid_name(ERRID_DRC_DRSU_ALARM_BEGIN));
    pl_dbg_ut_main("error ID:%5d(0x%4x), info:%s\n", ERRID_DRC_CRM_BEGIN,               ERRID_DRC_CRM_BEGIN,                pf_get_errid_name(ERRID_DRC_CRM_BEGIN));

    pl_dbg_ut_main("module ID:%5d(0x%4x), info:%s\n", MODULE_DAILYREC,  MODULE_DAILYREC,    pf_get_module_name(MODULE_DAILYREC));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info:%s\n", MODULE_LOG,       MODULE_LOG,         pf_get_module_name(MODULE_LOG));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info:%s\n", MODULE_TIMER,     MODULE_TIMER,       pf_get_module_name(MODULE_TIMER));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info:%s\n", MODULE_NETWORK,   MODULE_NETWORK,     pf_get_module_name(MODULE_NETWORK));

    pl_dbg_ut_main("module ID:%5d(0x%4x), info id:%d\n", MODULE_NETWORK,   MODULE_NETWORK,  pf_get_module_id("MODULE_NETWORK"));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info id:%d\n", MODULE_NETWORK,   MODULE_NETWORK,  pf_get_module_id("NETWORK"));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info id:%d\n", MODULE_NETWORK,   MODULE_NETWORK,  pf_get_module_id("E_NETWORK"));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info id:%d\n", MODULE_LOG,       MODULE_LOG,      pf_get_module_id("LOG"));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info id:%d\n", MODULE_LOG,       MODULE_LOG,      pf_get_module_id("MODULE_LOG"));
    pl_dbg_ut_main("module ID:%5d(0x%4x), info id:%d\n", MODULE_LOG,       MODULE_LOG,      pf_get_module_id("E_LOG"));

	
    ASSERT(PF_RET_SUCCESS == pf_write_flush_file((const S8*)"./f/b/c/d", (const S8*)"scFl1", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_flush_file((const S8*)"./f/b/c/d", (const S8*)"scFl2", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_flush_file((const S8*)"f/b/c/d", (const S8*)"scFl3", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_flush_file((const S8*)"./fab", (const S8*)"scFls", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_flush_file((const S8*)"fac", (const S8*)"scFls", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_flush_file((const S8*)"c", (const S8*)"scFls", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_flush_file((const S8*)".c", (const S8*)"scFls", 6));
	
    ASSERT(PF_RET_SUCCESS == pf_write_endof_file((const S8*)"./e/c/d/e", (const S8*)"scEn1", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_endof_file((const S8*)"./e/c/d/e", (const S8*)"scEn2", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_endof_file((const S8*)"e/c/d/e", (const S8*)"scEn3", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_endof_file((const S8*)"./eab", (const S8*)"scFl3", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_endof_file((const S8*)"eac", (const S8*)"scFls", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_endof_file((const S8*)"a", (const S8*)"scFls", 6));
    ASSERT(PF_RET_SUCCESS == pf_write_endof_file((const S8*)".a", (const S8*)"scFls", 6));

    ASSERT(PF_RET_FAILURE == pf_write_flush_file((const S8*)"./a/b/c/dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd", (const S8*)"scFls", 6));
    ASSERT(PF_RET_FAILURE == pf_write_endof_file((const S8*)"./a/b/c/dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd", (const S8*)"scFls", 6));
    ASSERT(PF_RET_FAILURE == pf_write_flush_file(NULL, (const S8*)"scFls", 6));
    ASSERT(PF_RET_FAILURE == pf_write_flush_file((const S8*)"./a", NULL, 6));
    ASSERT(PF_RET_FAILURE == pf_write_endof_file(NULL, (const S8*)"scFls", 6));
    ASSERT(PF_RET_FAILURE == pf_write_endof_file((const S8*)"./a", NULL, 6));
    ASSERT(PF_RET_FAILURE == pf_write_flush_file((const S8*)"", (const S8*)"scFls", 6));
    ASSERT(PF_RET_FAILURE == pf_write_endof_file((const S8*)"", (const S8*)"scFls", 6));
    ASSERT(PF_RET_FAILURE == pf_write_endof_file((const S8*)"/", (const S8*)"scFls", 6));
    ASSERT(PF_RET_FAILURE == pf_write_endof_file((const S8*)"//", (const S8*)"scFls", 6));

    //ASSERT(0);

    test_thread_entry((U32)0);
    FUNCTION_TRACE;
    get_time();

    pl_dbg_ut_main("-------------%s  END -----------------\n", __FUNCTION__);

    while(1)
    {
        FUNCTION_TRACE;
        get_time();
        ulConut++;
        pl_dbg_ut_main("main ulConut = %u\n", ulConut);
        delay_ms(100000);
    };
}



