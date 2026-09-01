/************************************************************
  Copyright (C), Innofidei Inc  2013
  FileName: pl.h
  Author: josephzhou    Version :  1.0   Date: 20130128
  Description:     平台接口单元测试用例头文件
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2013/1/28         1.0        初稿
***********************************************************/
#ifndef _UNIT_TEST_H
#define _UNIT_TEST_H
                              
#include <stdio.h>                    
#include <unistd.h>                   
#include <stdlib.h>                   
#include <string.h>                   
#include <fcntl.h>                    
#include <limits.h>                   
#include <sys/types.h>                
#include <sys/stat.h>                 
#include <sys/time.h>                 
#include <dlfcn.h>                    
#include <pthread.h>                  
#include <sys/sysinfo.h>


#define pl_dbg_ut printf 

extern volatile U32 test_assert;

#define TEST_ASSERT //{pl_dbg_ut("TEST FAILED ASSERT:%20s,%10d !!ASSERT!! msgQSendCountS %d\n",__FUNCTION__,__LINE__, performance.msgQSendCountS); test_assert=1;ASSERT(0);}

void test_case_result_output(char* testCaseName, U32 module, U32 result);

#undef FUNCTION_TRACE
#define FUNCTION_TRACE    printf("%12s,%20s,%4d\n", __FILE__, __FUNCTION__, __LINE__)

enum
{
    TEST_CASE_MODULE_THREADS,
    TEST_CASE_MODULE_MEMORY,
    TEST_CASE_MODULE_MESSAGE,
    TEST_CASE_MODULE_TIMER,
    TEST_CASE_MODULE_CYCLES,
    TEST_CASE_MODULE_LOG,
    TEST_CASE_MODULE_END
};

#endif
