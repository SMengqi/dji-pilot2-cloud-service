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
#define THIS_MODULE
/* include files*/
#include "../common/pl.h"
//#include "../common/types.h"
#include "../common/event.h"
#include "../common/module.h"
#include "../common/linuxport.h"
#include "osport.h"
#include "os.h"
#include "pf_mbox.h"

#include "unit_test.h"


typedef S32 (*PF_MSG)(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void *pData, U32 ulLength);

typedef void (*PF_THREAD_CREATE)(
        pf_addrword_t      sched_info,              /* scheduling info (eg pri)  */
        pf_thread_entry_t  *entry,                  /* entry point function      */
        pf_addrword_t      entry_data,              /* entry data                */
        char                *name,                  /* optional thread name      */
        void                *stack_base,            /* stack base, NULL = alloc  */
        unsigned int stack_size,                    /* stack size, 0 = default   */
        pf_handle_t        *handle,                 /* returned thread handle    */
        pf_thread_t          *thread                /* put thread here           */
        );


PF_MSG pf_copy_msg_right[] = 
{
    pf_copy_msg,                   \
    pf_copy_try_msg,               \
    pf_copy_urgent_msg,            \
    pf_copy_try_urgent_msg,        \
    pf_copy_msg
};

char pf_copy_case_input_error_name1[][40] = 
{
    {"pf_copy_msg inputERR1"},                   \
    {"pf_copy_try_msg inputERR1"},               \
    {"pf_copy_urgent_msg inputERR1"},               \
    {"pf_copy_try_urgent_msg inputERR1"},        \
    {"pf_copy_msg MAX inputERR1"}
};
char pf_copy_case_input_error_name2[][40] = 
{
    {"pf_copy_msg inputERR2"},                   \
    {"pf_copy_try_msg inputERR2"},               \
    {"pf_copy_urgent_msg inputERR2"},            \
    {"pf_copy_try_urgent_msg inputERR2"},        \
    {"pf_copy_msg MAX inputERR2"}
};
char pf_copy_case_input_error_name3[][40] = 
{
    {"pf_copy_msg inputERR3"},                   \
    {"pf_copy_try_msg inputERR3"},               \
    {"pf_copy_urgent_msg inputERR3"},            \
    {"pf_copy_try_urgent_msg inputERR3"},        \
    {"pf_copy_msg MAX inputERR3"}
};

char pf_copy_case_right_name[][40] = 
{
    {"MESRIGHT pf_copy_msg"},                   \
    {"MESRIGHT pf_copy_try_msg"},               \
    {"MESRIGHT pf_copy_urgent_msg"},            \
    {"MESRIGHT pf_copy_try_urgent_msg"},        \
    {"MESRIGHTMAX pf_copy_msg"}
};

char pf_copy_case_full_name[][40] = 
{
    {"MESFULL pf_copy_msg"},                   \
    {"MESFULL pf_copy_try_msg"},               \
    {"MESFULL pf_copy_urgent_msg"},            \
    {"MESFULL pf_copy_try_urgent_msg"},        \
    {"MESFULLMAX pf_copy_msg"}
};

char pf_copy_case_dual_name[][40] = 
{
    {"MESTIME pf_copy_msg"},                   \
    {"MESTIME pf_copy_try_msg"},               \
    {"MESTIME pf_copy_urgent_msg"},            \
    {"MESTIME pf_copy_try_urgent_msg"},        \
    {"MESTIMEMAX pf_copy_msg"}
};


enum
{
    TEST_STATE_COPY_MSG,
    TEST_STATE_COPY_TRY_MSG,
    TEST_STATE_COPY_URGENT_MSG,
    TEST_STATE_COPY_TRY_URGENT_MSG,
    TEST_STATE_END
}TEST_STATE_ENUM;


enum
{
    TESTCASE_BEGIN,
    TESTCASE_COPYMSG_RIGHT,    
    TESTCASE_COPYMSG_FULL,    
    TESTCASE_MESSAGE_DUALTHREAD_TIMING,    
    TESTCASE_END
}TESTCASE;


pthread_mutex_t qlock_ticks; 

#define FULL_SEQUENCE_ARRAY_NUMBER (MESSAGE_QUEUE_SIZE+1000)


volatile U32 u32McNum = 0;
volatile U32 u32txrlcNum = 0;
volatile U32 u32macNum = 0;
volatile U32 u32rxrlcNum = 0;
volatile U32 test_msg_num = 1;

volatile U32 rxrlc_test_msg_num = 0;
volatile U32 txrlc_test_msg_num = 0;
volatile U32 mac_test_msg_num = 0;
volatile U32 ipgw_test_msg_num = 0;
volatile U32 test_right = 0;
volatile U32 test_state = 0;
volatile U32 test_case = 1;
volatile U32 auto_run = 1;
volatile U32 test_num = 1;
volatile U32 test_assert = 0;

U32 mcThreadNum[TESTCASE_END] = {0};
U32 macThreadNum[TESTCASE_END] = {0};
U32 txrlcThreadNum[TESTCASE_END] = {0};
U32 rxrlcThreadNum[TESTCASE_END] = {0};

#define pl_dbg_uint_test            pl_dbg_ut
#define pl_dbg_log_length           //pl_dbg_ut
#define pl_dbg_msg_entry            //pl_dbg_ut
#define pl_dbg_important_info       pl_dbg_ut/*pf_log_important_info*/
#define pl_dbg_msg_full             pl_dbg_ut

#define WHETHER_ASSERT_OR_NOT {if(test_assert) {pl_dbg_uint_test("STOP\n");TEST_ASSERT;while(1);}}

#define MAX_CYCLE_NUMBER 250
#define TESTCASE_COPYMSG_FULL_WAIT_CYCLE 1000


extern MODULE_ENTRY moduleArray[];
extern pf_thread_t  workerThreads[];
extern pf_handle_t  workerhandles[];
extern MODULE_INIT  moduleInitArray[];

U32  module_init_num[MODULE_MAX] = {0};
U32  module_entry_num[MODULE_MAX] = {0};
U32  msg_entry_num[MODULE_MAX] = {0};

extern void display_pthread_attr(pthread_attr_t *attr, char *prefix);

extern void delay_ms(U32 ulms);

extern void msg_entry(pf_addrword_t mid);




DECLTASK(test_main,     8388608)/*可创建堆栈最小值为128k字节*/

DECLMODULE(ipgw,        8388608)    
DECLMODULE(mac,         8388608)    
DECLMODULE(rxrlc,       8388608)    
DECLMODULE(txrlc,       8388608)    

DECLMODULE(rrc,         8388608)

void get_time(void)
{
    pl_dbg_uint_test("%s,ms:%lld,us:%lld,ns:%lld\n", __FUNCTION__, pf_get_ticks_ms(), pf_get_ticks_us(), pf_get_ticks_ns());
}

U32 pl_get_local_counts(void)
{
    static U32 num = 0;
    U32 ticks= 0;
    pthread_mutex_lock(&qlock_ticks);
    num++;
    ticks = num;
    pthread_mutex_unlock(&qlock_ticks);
    return ticks;
}

S32 ipgw_init(U32 ulModuleId)
{
    FUNCTION_TRACE;
    module_init_num[MODULE_IPGW]++;
    return PF_RET_SUCCESS;
}
S32 mac_init(U32 ulModuleId)
{
    FUNCTION_TRACE;
    module_init_num[MODULE_MAC]++;
    return PF_RET_SUCCESS;
}
S32 rxrlc_init(U32 ulModuleId)
{
    FUNCTION_TRACE;
    module_init_num[MODULE_RXRLC]++;
    return PF_RET_SUCCESS;
}
S32 txrlc_init(U32 ulModuleId)
{
    FUNCTION_TRACE;
    module_init_num[MODULE_TXRLC]++;
    return PF_RET_SUCCESS;
}
S32 test_main_init(U32 ulModuleId)
{
    FUNCTION_TRACE;
    return PF_RET_SUCCESS;
}
void clear_data(void)
{
    rxrlc_test_msg_num = 0;
    txrlc_test_msg_num = 0;
    mac_test_msg_num = 0;
    ipgw_test_msg_num = 0;    
    pf_memset(&mcThreadNum[0],    0, sizeof(mcThreadNum));
    pf_memset(&macThreadNum[0],  0, sizeof(macThreadNum));
    pf_memset(&txrlcThreadNum[0], 0, sizeof(txrlcThreadNum));
    pf_memset(&rxrlcThreadNum[0], 0, sizeof(rxrlcThreadNum));
}


void print_thread_info(U8 testCase, U32 time_start)
{
    U32 time_end = pf_get_ticks_ms();
    sched_param param;
    int priority;
    int policy;
    int ret;
    ret = pthread_getschedparam (workerhandles[MODULE_IPGW], &policy, &param);
    if(0 != ret)
    {
        TEST_ASSERT;
    }
    priority = param.sched_priority; 
    pl_dbg_uint_test("MODULE_IPGW cheduling policy   = %s, priority=%d\n", 
           (policy == SCHED_OTHER) ? "SCHED_OTHER" :
           (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
           (policy == SCHED_RR)    ? "SCHED_RR" :
           "???", priority);

    /* scheduling parameters of target thread */
    ret = pthread_getschedparam (workerhandles[MODULE_MAC], &policy, &param);
    if(0 != ret)
    {
        TEST_ASSERT;
    }
    priority = param.sched_priority; 
    pl_dbg_uint_test("MODULE_MAC cheduling policy   = %s, priority=%d\n", 
           (policy == SCHED_OTHER) ? "SCHED_OTHER" :
           (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
           (policy == SCHED_RR)    ? "SCHED_RR" :
           "???", priority);

    pl_dbg_uint_test("u32McNum:%d(0x%x),u32atmtNum:%d(0x%x) u32atpdpNum:%d(0x%x)\n", u32McNum, &u32McNum, u32macNum, &u32macNum, u32txrlcNum, &u32txrlcNum);
    pl_dbg_uint_test("u32McNum:%d,TS:%d,TE:%d,TL:%d,TA%d\n", u32McNum, time_start, time_end, time_end-time_start, u32McNum/(time_end-time_start));
}


void print_msgQ_statics(void)
{    
#if 0	
    pl_dbg_uint_test("performance    : msgQSendCountS %8d, msgQSendFailsAllCountS %8d,  msgQSendFailsModeNULL %8d,      msgQSendFailsMalloc %8d, msgQSendFailsModuleMax %6d\n",\
            (S32)performance.msgQSendCountS,            \
            (S32)performance.msgQSendFailsAllCountS,    \
            (S32)performance.msgQSendFailsModuleNULL,   \
            (S32)performance.msgQSendFailsMalloc,       \
            (S32)performance.msgQSendFailsModuleMax);
    pl_dbg_uint_test("performance  msgQReceivedCountS %8d\n",\
            (S32)performance.msgQReceivedCountS);
    pl_dbg_uint_test("           msgQSendMallocCountS %8d\n",\
            (S32)performance.msgQSendMallocCountS);
    pl_dbg_uint_test("          msgQSendMsgFreeCountS %8d,msgQSendFalseFreeCountS %8d,  msgQSendFalseCountS   %8d\n",\
            (S32)performance.msgQSendMsgFreeCountS,    \
            (S32)performance.msgQSendFalseFreeCountS,   \
            (S32)performance.msgQSendFalseCountS);
    pl_dbg_uint_test("performance  msgQSendCopyCountS %8d,  msgQSendCopyUrgCountS %8d,  msgQSendCopyTryCountS %8d, msgQSendCopyTryUrgCountS %8d\n",\
            (S32)performance.msgQSendCopyCountS,        \
            (S32)performance.msgQSendCopyUrgCountS,     \
            (S32)performance.msgQSendCopyTryCountS,     \
            (S32)performance.msgQSendCopyTryUrgCountS);
    pl_dbg_uint_test("performance  msgQSendDataCountS %8d,  msgQSendDataUrgCountS %8d,  msgQSendDataTryCountS %8d, msgQSendDataTryUrgCountS %8d\n",\
            (S32)performance.msgQSendDataCountS,        \
            (S32)performance.msgQSendDataUrgCountS,     \
            (S32)performance.msgQSendDataTryCountS,     \
            (S32)performance.msgQSendDataTryUrgCountS);
    pl_dbg_uint_test("performance   msgQSendOamCountS %8d,       msgQSendOamSizeS %8d\n",\
            (S32)performance.msgQSendOamCountS,         \
            (S32)performance.msgQSendOamSizeS);
    pl_dbg_uint_test("performance        memsetCountS %8d,          memsetSizeS 0x%8x,           memcpyCountS %8d,             memcpySizeS %8d\n",\
            (S32)performance.memsetCountS,              \
            (S32)performance.memsetSizeS,               \
            (S32)performance.memcpyCountS,              \
            (S32)performance.memcpySizeS);
    pl_dbg_uint_test("performance memcpyFailsSrCountS %8d,  memcpyFailsDstCountS 0x%8x,  memcpyFailsSizeCountS %8d\n",\
            (S32)performance.memcpyFailsSrcCountS,              \
            (S32)performance.memcpyFailsDstCountS,              \
            (S32)performance.memcpyFailsSizeCountS);
    pl_dbg_uint_test("performance memsetFailAddCountS %8d,  memsetFailSizeCountS 0x%8x\n",\
            (S32)performance.memsetFailAddrCountS,              \
            (S32)performance.memsetFailSizeCountS);
     pl_dbg_uint_test("performance        mallocCountS %8d,          mallocSizeS 0x%8x,   mallocFailsOutCountS %8d,     mallocFailsOutSizeS %8d,     mallocFailsInCountS %d, \n",\
            (S32)performance.mallocCountS,              \
            (S32)performance.mallocSizeS,               \
            (S32)performance.mallocFailsOutCountS,      \
            (S32)performance.mallocFailsOutSizeS,       \
            (S32)performance.mallocFailsInCountS);
    pl_dbg_uint_test("performance          freeCountS %8d,        freeFailsCountS %8d\n",\
            (S32)performance.freeCountS,                \
            (S32)performance.freeFailsCountS);
    pl_dbg_uint_test("performance           logCountS %8d,             dumpCountS %8d           logInfoCountS %8d\n",  \ 
            (S32)performance.logCountS,                 \
            (S32)performance.dumpCountS,                \
            (S32)performance.logInfoCountS);            
    pl_dbg_uint_test("performance      logFailsCountS %8d,        dumpFailsCountS %8d\n",\ 
            (S32)performance.logFailsCountS,            \
            (S32)performance.dumpFailsCountS);            
    if(performance.msgQSendMallocCountS != performance.msgQSendMsgFreeCountS + performance.msgQSendFalseFreeCountS)
    {
        pl_dbg_uint_test("%s  msgQ memory NOT EQUAL, mallocCs=%8d, FreeCs=%8d\n", __FUNCTION__, performance.msgQSendMallocCountS, performance.msgQSendMsgFreeCountS + performance.msgQSendFalseFreeCountS);
    }
    if(performance.msgQSendCountS != performance.msgQReceivedCountS)
    {
        pl_dbg_uint_test("%s  msgQ SEND   NOT EQUAL,   SendCs=%8d,  RevCs=%8d\n", __FUNCTION__, performance.msgQSendCountS, performance.msgQReceivedCountS);
    }
    for(S32 i=0; i<MODULE_MAX; i++)   
    {        
        if(performance.msgQSendFailsCountS[i] > 0)           
            pl_dbg_uint_test("MODULE NAME I=%d: msgQSendFailsCountS %d\n", i, (S32)performance.msgQSendFailsCountS[i]);   
    }    
    for(S32 i=0; i<TESTCASE_END; i++)   
    {        
        pl_dbg_uint_test("TESTCASE I=%8d MC:%8d,MAC:%8d,TXRLC:%8d,RXRLC:%8d\n", i, mcThreadNum[i], macThreadNum[i], txrlcThreadNum[i], rxrlcThreadNum[i]);   
    }    
    pl_dbg_uint_test("TESTCASE ALL MC:%d,MAC:%d,TXRLC:%d,RXRLC:%d\n", ipgw_test_msg_num, mac_test_msg_num, txrlc_test_msg_num, rxrlc_test_msg_num);   
#endif
}



void test_dual_thread_timing(U8 *Data, U16 length, U32 pri, U32 j)
{
    U32 time_start;
    U32 i;
    pf_memset(&mcThreadNum[0],   0, sizeof(mcThreadNum));
    pf_memset(&macThreadNum[0],  0, sizeof(macThreadNum));
    u32McNum = 0;
    if(length)
    {
        pf_memset(Data, length, length);
    }
    pl_dbg_uint_test("%s:TEST DUAL THREAD SAME PRIORITY:%d,length=%d\n", __FUNCTION__, pri, length);
    time_start = pf_get_ticks_ms();
    test_state = j;
    for(i=0; i<test_msg_num; i++)
    {
        dbgline;
        test_num++;
        pf_copy_msg_right[test_state](MODULE_IPGW, test_case, MODULE_IPGW, Data, length);
    }
    dbgline;
    delay_ms(TESTCASE_COPYMSG_FULL_WAIT_CYCLE);
    test_state = TEST_STATE_END;
    print_thread_info(test_case, time_start);
    print_msgQ_statics();
    delay_ms(1000);

    test_case_result_output(pf_copy_case_dual_name[j], TEST_CASE_MODULE_MESSAGE, 1);
    WHETHER_ASSERT_OR_NOT;
}

void set_dual_pthread_priority(U32 pri1, U32 pri2)
{
    sched_param param;
    int policy = SCHED_RR;
    int ret;

    pl_dbg_msg_entry("%s: PID: %d, pri1: %d, pri2: %d\n", __FUNCTION__, getpid(), pri1, pri2);

    param.sched_priority = pri1; 
    ret = pthread_setschedparam (workerhandles[MODULE_IPGW], policy, &param);
    if(0 != ret)
    {
        pl_dbg_msg_entry(" MC pri1:%d, ret=%d\n", pri1, ret);
    }

    param.sched_priority = pri2; 
    ret = pthread_setschedparam (workerhandles[MODULE_MAC], policy, &param);
    if(0 != ret)
    {
        pl_dbg_msg_entry("MAC pri2:%d, ret=%d\n", pri2, ret);
    }
}

void testcase_copymsg_right(void)
{
    U32 i;
    U32 malloc_length;
    U8 tmpData[8192];
    test_right = 1;
    for(i=0; i<TEST_STATE_END; i++)
    {
        FUNCTION_TRACE;
        print_msgQ_statics();
        clear_data();
        print_msgQ_statics();
        get_time();
        WHETHER_ASSERT_OR_NOT;
        malloc_length = (pl_get_local_counts() % MAX_CYCLE_NUMBER) + 1;
        pf_memset((void*)tmpData, malloc_length, malloc_length);
        pl_dbg_uint_test("TestMain: TESTCASE_COPYMSG_RIGHT tmpData 0x%x, malloc_length %d\n", tmpData[0], malloc_length);
        test_state = i;
        pf_copy_msg(malloc_length, malloc_length, MODULE_IPGW, (void*)tmpData, malloc_length);
        print_msgQ_statics();
        FUNCTION_TRACE;
        delay_ms(TESTCASE_COPYMSG_FULL_WAIT_CYCLE);
        FUNCTION_TRACE;
        test_state = TEST_STATE_END;
        FUNCTION_TRACE;
        delay_ms(3000);
        FUNCTION_TRACE;
        print_msgQ_statics();
#if 0
        if(/*(performance.mallocCountS < performance.freeCountS) (performance.msgQSendMallocCountS != (performance.msgQSendMsgFreeCountS+performance.msgQSendFalseFreeCountS)) \
                   ||*/ (performance.mallocFailsInCountS)         \
                   || (performance.mallocFailsOutCountS)        \
                   || (performance.freeFailsCountS))
        {
            test_case_result_output(pf_copy_case_right_name[i], TEST_CASE_MODULE_MESSAGE, NOT_OK);
        }
        else
        {
            test_case_result_output(pf_copy_case_right_name[i], TEST_CASE_MODULE_MESSAGE, OK);
        }
#endif        
        pl_dbg_uint_test("\n\n\n\n\n\n********TESTCASE_COPYMSG_RIGHT %d TEST END***********\n\n\n\n\n",i);
        pl_dbg_important_info("\n*****TESTCASE_COPYMSG_RIGHT VIP INFO %d TEST END******\n\n\n\n\n",i);
    }
    test_right = 0;
}



void testcase_copymsg_full(void)
{
    U32 i;
    U32 j;
    U32 localNum=0;
    U32 malloc_length;
    U8 tmpData[16384*2];
    //U8 tmpData[8192];

    //clear_msg_full_params();
    test_right = 1;
    for(i=0; i<TEST_STATE_END; i++)
    {
        clear_data();
        get_time();
        WHETHER_ASSERT_OR_NOT;
        FUNCTION_TRACE;
        test_state = i;
        for(j=0; j<FULL_SEQUENCE_ARRAY_NUMBER; j++)
        {
            malloc_length = j + 1;
            pf_memset((void*)tmpData, malloc_length, malloc_length);
            pf_copy_msg_right[test_state](malloc_length, malloc_length, MODULE_MC, (void*) tmpData, malloc_length);
            pl_dbg_msg_full("TESTCASE_COPYMSG_FULL:%d,%d, tmpData 0x%x, malloc_length %d\n", i, j, tmpData[0], malloc_length);
        }

        FUNCTION_TRACE;
        delay_ms(TESTCASE_COPYMSG_FULL_WAIT_CYCLE);
        delay_ms(TESTCASE_COPYMSG_FULL_WAIT_CYCLE);
        delay_ms(TESTCASE_COPYMSG_FULL_WAIT_CYCLE);
        FUNCTION_TRACE;

        test_state = TEST_STATE_END;
        FUNCTION_TRACE;
        delay_ms(TESTCASE_COPYMSG_FULL_WAIT_CYCLE);
        FUNCTION_TRACE;
        print_msgQ_statics();

        //print_msgQ_full_result(i);

        pl_dbg_uint_test("\n\n\n\n\n\n********TESTCASE_COPYMSG_FULL %d TEST END***********\n\n\n\n\n",i);

        pl_dbg_important_info("\n*****TESTCASE_COPYMSG_FULL VIP INFO %d TEST END******\n\n\n\n\n",i);

    }
    test_right = 0;

}




void testcase_message_dualthread_timing(void)
{
    U32 i;
    U32 state;
    U32 localNum=0;
    U32 malloc_length;
    U8 tmpData[8192];
    pl_dbg_uint_test("-------------%s START-----------------\n", __FUNCTION__);
    for(state=0; state<TEST_STATE_END; state++)
    {
        for(i=1; i<5; i++)
        {
            set_dual_pthread_priority(i, 1);
            pl_dbg_uint_test("test_case_result_output: %s NULL length= 0, pri1=%d, pri2=1\n", __FUNCTION__, i);
            //pf_log_important_info("test_case_result_output: %s NULL length= 0, pri1=%d, pri2=1\n", __FUNCTION__, i);
            test_dual_thread_timing(NULL, 0, i, state);
            delay_ms(1000ULL);
            pl_dbg_uint_test("test_case_result_output: %s ADDR:0x%x length=48, pri1=%d, pri2=1\n", __FUNCTION__, tmpData, i);
            //pf_log_important_info("test_case_result_output: %s ADDR:0x%x length=48, pri1=%d, pri2=1\n", __FUNCTION__, tmpData, i);
            test_dual_thread_timing(tmpData, 48, i, state);
            delay_ms(1000ULL);
            pl_dbg_uint_test("test_case_result_output: %s ADDR:0x%x length=49, pri1=%d, pri2=%d\n", __FUNCTION__, tmpData, i, i);
            //pf_log_important_info("test_case_result_output: %s ADDR:0x%x length=49, pri1=%d, pri2=%d\n", __FUNCTION__, tmpData, i, i);
            test_dual_thread_timing(tmpData, 49, i, state);
            delay_ms(1000ULL);
            WHETHER_ASSERT_OR_NOT;
        }

    }
    pl_dbg_uint_test("-------------%s  END -----------------\n", __FUNCTION__);
}




void test_main_entry (pf_addrword_t data)
{
    //pf_memset(&performance, 0, sizeof(Performence));
    FUNCTION_TRACE;
    pl_dbg_uint_test("%s: PID: %d\n", __FUNCTION__, getpid());
    while(1)
    {
        FUNCTION_TRACE;
        switch (test_case)
        {

            case TESTCASE_COPYMSG_RIGHT:
            {
                FUNCTION_TRACE;
                testcase_copymsg_right();
                break;
            }

            case TESTCASE_COPYMSG_FULL:
            {
                FUNCTION_TRACE;
                testcase_copymsg_full();
                break;
            }
            case TESTCASE_MESSAGE_DUALTHREAD_TIMING:
            {
                FUNCTION_TRACE;
                testcase_message_dualthread_timing();
                break;    
            }

            default:
                FUNCTION_TRACE;
                break;
        }
        
        if(test_case > 0)
        {
            pl_dbg_uint_test("auto_run=%d,Address:%d(0x%x)\n",auto_run,(U64)&auto_run,(U64)&auto_run);
            pl_dbg_uint_test("test_case=%d,Address:%d(0x%x)\n",test_case,(U64)&test_case,(U64)&test_case);
            pl_dbg_uint_test("test_state=%d,Address:%d(0x%x)\n",test_state,(U64)&test_state,(U64)&test_state);
            pl_dbg_uint_test("test_msg_num=%d,Address:%d(0x%x)\n",test_msg_num,(U64)&test_msg_num,(U64)&test_msg_num);
            pl_dbg_uint_test("test_right=%d,Address:%d(0x%x)\n",test_right,(U64)&test_right,(U64)&test_right);
            pl_dbg_uint_test("test_num=%d,Address:%d(0x%x)\n",test_num,(U64)&test_num,(U64)&test_num);
            //pl_dbg_uint_test("performance.msgQSendFailsAllCountS=%d,Address:%d(0x%x)\n",performance.msgQSendFailsAllCountS,(U64)&performance.msgQSendFailsAllCountS,(U64)&performance.msgQSendFailsAllCountS);
            //pl_dbg_uint_test("performance Address:%d(0x%x)\n",(U64)&performance,(U64)&performance);
            //pl_dbg_uint_test("performance msgQSendCountS:%d, msgQReceiveddCountS %d\n",(U32)performance.msgQSendCountS,performance.msgQReceivedCountS);
            pl_dbg_uint_test("TEST CASE %d END\n", test_case);  

            if(auto_run == 1)
            {
                test_case++;
                test_case %= TESTCASE_END;
            }
            else if(auto_run)
            {
                test_case++;
                test_case %= TESTCASE_END;
                if(test_case == 0)
                {
                    test_case = 1;    
                }
            }                
            else
            {
                test_case = 0;
            }

        }
        else
        {
            return;        
        }
    }
}


void call_random_copy_msg_length(U16 usModuleId)
{
    U16 randomLength = pf_get_ticks_ns();
    U16 randomfunc = pf_get_ticks_ns()%TEST_STATE_END;

    if(randomLength)
    {
        void* pmData = pf_malloc(randomLength);
        ASSERT(pmData);
        pf_memset(pmData, randomLength, randomLength);
        pf_copy_msg_right[randomfunc](randomLength, randomLength, usModuleId, (void*)pmData, randomLength);                    
        pf_free(pmData);
    }
    else
    {
        pf_copy_msg_right[randomfunc](0, 0, usModuleId, NULL, 0);                    
    }
}
void rxrlc_entry(U32 src, U32 msgId, U32 dst, void * data, U32 length)
{
    U32 i;
    U32 u32MsgNum = /*pl_get_local_counts()%10 + */1;
    U32 malloc_length;
    u32rxrlcNum++;
    module_entry_num[MODULE_RXRLC]++;
    pl_dbg_msg_entry("%s: PID: %d\n", __FUNCTION__, getpid());
    if(test_right)
    {
        char* cData = (char*)data;
        char c = length%256;
        for(i=0; i<length; i++)
        {
            if(c != *(cData+i))    
            {
                pl_dbg_uint_test("mac_entry Failed i=%d,length=%d,data=%d(0x%x)\n", i, length, *(cData+i), data);
                TEST_ASSERT;
            }
        }
    }

   
    rxrlcThreadNum[test_case%TESTCASE_END]++;
    rxrlcThreadNum[TESTCASE_BEGIN]++;
    return ;
}
void txrlc_entry(U32 src, U32 msgId, U32 dst, void * data, U32 length)
{
    U32 i;
    U32 u32MsgNum = /*pl_get_local_counts()%10 + */1;
    U32 malloc_length;
    u32txrlcNum++;
    module_entry_num[MODULE_TXRLC]++;
    pl_dbg_msg_entry("%s: PID: %d\n", __FUNCTION__, getpid());
    if(test_right)
    {
        char* cData = (char*)data;
        char c = length%256;
        for(i=0; i<length; i++)
        {
            if(c != *(cData+i))    
            {
                pl_dbg_uint_test("mac_entry Failed i=%d,length=%d,data=%d(0x%x)\n", i, length, *(cData+i), (U64)data);
                TEST_ASSERT;
            }
        }
    }

    txrlcThreadNum[test_case%TESTCASE_END]++;
    txrlcThreadNum[TESTCASE_BEGIN]++;
    return ;
}

void mac_entry(U32 src, U32 msgId, U32 dst, void * data, U32 length)
{
    U32 i;
    U32 u32MsgNum = /*pl_get_local_counts()%10 + */1;
    u32macNum++;
    module_entry_num[MODULE_MAC]++;
    pl_dbg_msg_entry("%s: PID: %d\n", __FUNCTION__, getpid());
    if(TESTCASE_MESSAGE_DUALTHREAD_TIMING == test_case)
    {
        if(TEST_STATE_END != test_state )
        {
            pf_copy_msg_right[test_state](MODULE_MAC, msgId, MODULE_IPGW, (void*)data, length);                    
        }
        macThreadNum[test_case%TESTCASE_END]++;
        macThreadNum[TESTCASE_BEGIN]++;
        //pf_log(0, MODULE_MAC, "mac_entry test log info %d %d total %d %d", test_state, test_case, macThreadNum[test_case%TESTCASE_END], macThreadNum[TESTCASE_BEGIN]);
        return;
    }
    
    macThreadNum[test_case%TESTCASE_END]++;
    macThreadNum[TESTCASE_BEGIN]++;
    return ;
}

void ipgw_entry(U32 src, U32 msgId, U32 dst, void * data, U32 length)
{
    U32 i;
    U32 u32MsgNum = /*pl_get_local_counts()%10 + */1;
    U32 malloc_length;
    u32McNum++;
    module_entry_num[MODULE_IPGW]++;
    pl_dbg_msg_entry("%s: PID: %d\n", __FUNCTION__, getpid());
    if(TESTCASE_MESSAGE_DUALTHREAD_TIMING == test_case)
    {
        if(TEST_STATE_END != test_state )
        {
            pf_copy_msg_right[test_state](MODULE_IPGW, msgId, MODULE_MAC, (void*)data, length);
        }
        mcThreadNum[test_case%TESTCASE_END]++;
        mcThreadNum[TESTCASE_BEGIN]++;
        //pf_log(0, MODULE_IPGW, "ipgw_entry test log info %d %d total %d %d", test_state, test_case, mcThreadNum[test_case%TESTCASE_END], mcThreadNum[TESTCASE_BEGIN]);
        return;
    }
    
        
    mcThreadNum[test_case%TESTCASE_END]++;
    mcThreadNum[TESTCASE_BEGIN]++;
    return ;
}

void rrc_init(void)
{
    FUNCTION_TRACE;
    module_init_num[MODULE_RRC]++;
    return;
}

void rrc_entry(U32 src, U32 msgId, U32 dst, void * data, U32 length)
{
    FUNCTION_TRACE;
    module_entry_num[MODULE_RRC]++;
    return ;
}


int message_test_main(void)
{
    U32 i;
    FUNCTION_TRACE;
    qlock_ticks = PTHREAD_MUTEX_INITIALIZER;
    get_time();
    pl_dbg_uint_test("=======================================================\n");
    pl_dbg_uint_test("%s L:%d TEST\n", __FUNCTION__, __LINE__);
    pl_dbg_uint_test("=======================================================\n");


    CREATEMODULE(11, IPGW, ipgw); 
    delay_ms(1000);
    CREATEMODULE(10, MAC, mac);

    CREATEMODULE(0, RXRLC, rxrlc);
    delay_ms(1000);
    CREATEMODULE(0, TXRLC, txrlc);
    delay_ms(1000);

    CREATETASK(99, UNSOCK_SERVER, test_main);
    pf_set_sys_init();

    while(test_case)
    {
        i++;
        pl_dbg_uint_test("%s L:%d TEST i = %d\n", __FUNCTION__, __LINE__, i);
        pf_copy_msg(MODULE_DAILYREC, 0, MODULE_LOG, 0, 0);
        get_time();
        delay_ms(60000);
        get_time();
    }

    get_time();
    pl_dbg_uint_test("%s L:%d TEST\n", __FUNCTION__, __LINE__);
    FUNCTION_TRACE;
    get_time();
}
