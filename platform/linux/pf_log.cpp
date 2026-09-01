/************************************************************************************************************************
**                                                                                                                        
**  Copyright (c)  2009,  Innofidei, Inc.                                                                                 
**        All    Rights Reserved.                                                                                            
**                                                                                                                          
**  Subsystem    : LTE/SMALLCELL                                                                                             
**  File        : pf_log.cpp                                                                                   
**  Created By    : josephzhou                                                                                              
**  Created On    : 12/10/2012
**                                                                                                                         
**  Purpose:                                                                                                             
**    low priority log task
**                                                                                                                         
**  History:                                                                                                             
**  Programmer        Date    Rev    Description                                                                                 
**  --------------- ---------- --------    ------------------------------                                                   
**
************************************************************************************************************************/
#define THIS_MODULE PLATFORM_LOG

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "pl.h"
#include "event.h"
#include "linuxport.h"
#include "pf_upgrade.h"
#include "pf_timer.h"
#include "module.h"
#include "os.h"
#include "osport.h"

#include "json2pb.h"
#include "log_level.pb.h"
#include <pf_stat.h>
#include "../../platform/network/Linux/platform_socket.h"
#include "pf_nacos_wrapper.h"
#include <vector>
#include "pf_thread_mon.h"
#include "pf_rdkafka.h"
#include "kafka_config.pb.h"

#define SINGLE_LOG_SIZE             1024 
#define LOG_UNIT_SIZE               14000                           
#define DUMP_DATA_BYTE_PER_LINE     32                              //The number of bytes displayed per line in binary printing
#define DUMP_DATA_CHAR_PER_LINE     (DUMP_DATA_BYTE_PER_LINE*3)     //The number of characters displayed per line in binary printing


/*Define LOG default cycle time is 10 milliseconds*/
#define MAX_LOG_CYCLE_NUMBER        100

// MR1513: Improve nacos log level management code
#define NACOS_CONF_COMMON_FILE      "common"

//65536=CM_MAX*CI_MAX*sizeof(U64) g_ulPSCnts
#define PSCNT_TOTAL_LENGTH          65536

//8192=4*CI_MAX*sizeof(U64)
#define PSCNT_COPY_LENGTH           8192

/*maximum kafka send data size is 999700*/
#define MAX_SIZE_LOG_WRITE_KAFKA    999700

/*define log buffer structer*/
typedef struct{
    pthread_mutex_t mlock;      /*memory mutex*/
    U32 ulSeqNum;               /*log number*/
    U32 ulNewSeqNum;            /*NewMap log number*/
}PFLOG_BUFFER_INFO_S;

typedef enum
{   
    /*only writing log into files*/
    LOG_WRITE_FILE = 0x01,
    /*only kafka is written*/
    LOG_WRITE_KAFKA = 0x02,
}PF_LOG_WRITE_FLAG_E;

/*The buffer pointer of log*/
PFLOG_BUFFER_INFO_S* pstBufInfo = NULL;

/*Log buffer backup pointer for temporarily closing log level when cpuload is full*/
PFLOG_BUFFER_INFO_S* pstBufInfoTmp = NULL;

static const char hex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
 
typedef std::map<int, std::vector<char *>> LogInfoList;

LogInfoList g_stLogMapInfo;
LogInfoList g_stLocalLogMapInfo;
PF_MUTEX_T g_stLogMutex;

const CHAR *loglevel[]=  {
    "DNU",
    "FAT",
    "ERR",
    "WRN",
    "INF",
    "TRC",
    "UIF"
};

U32 aulModuleLogFlag[LOG_MODULE_MAX] = {0};
U32 ulLogTimerId = 0;
static CHAR acLogPath[SINGLE_LOG_SIZE] = {0};

/*default : 1 only writing log into files, If 2, only kafka is written; If 3, both are written*/
U32 ulLogWriteFlag = LOG_WRITE_FILE;

kafka_producer *glLogKafkaProduce = NULL; 

int my_log_handle_mq_send_err(int err)
{
    if(err != RdKafka::ERR_NO_ERROR)
    { 
        pl_log(ERR, "rdkafka: err is %s", RdKafka::err2str((RdKafka::ErrorCode)err).c_str());    
    }
    return PF_RET_SUCCESS;
}
/**********************************************************************************************
 * @function      pf_log_buffer_init
 * @brief         Log buffer initialization interface
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_log_buffer_init()
{
    pthread_mutexattr_t inherit_attr;    
    /*Applying dynamic memory for log information logging, do not release*/
    pstBufInfoTmp = (PFLOG_BUFFER_INFO_S*)pf_malloc(sizeof(PFLOG_BUFFER_INFO_S));
    pf_memset(pstBufInfoTmp, 0, sizeof(PFLOG_BUFFER_INFO_S));

    /*initial buffer pointer*/
    pstBufInfo = pstBufInfoTmp;
}

/**********************************************************************************************
 * @function      pf_log_timer_init
 * @brief         Log buffer timer initialization interface
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_log_timer_init(void)
{
    /*start cycle timer*/
    pf_timer_cycle_start(MODULE_LOG, 
        MAX_LOG_CYCLE_NUMBER, 
        0, 
        0, 
        &ulLogTimerId);
}

/**********************************************************************************************
 * @function      log_write_kafka
 * @brief         Internal function to write log information to kafka
 * @input         strInfo    string to store log information
 * @output        void
 * @return        void
 *********************************************************************************************/
S32 log_write_kafka(std::string &strInfo)
{
    std::string strLogHeaderName = "MsgId";
    std::string strLogHeaderVal = std::to_string(PF_LOG_KAFKA_INFO);
    U32 count = 0;
    S32 strlen = strInfo.length();
    
    if(strlen > MAX_SIZE_LOG_WRITE_KAFKA)
    {
        while(strlen > 0)
        {
            std::string frontstr = strInfo.substr(MAX_SIZE_LOG_WRITE_KAFKA*count, MAX_SIZE_LOG_WRITE_KAFKA);
            if(glLogKafkaProduce->send((S8*)frontstr.c_str(), frontstr.length(), (S8*)strLogHeaderName.c_str(), strLogHeaderName.size(), (S8*)strLogHeaderVal.c_str(), strLogHeaderVal.size()))
            {
                pl_log(ERR, "log send kafka failed");
                return PF_RET_FAILURE;
            }
            count++;
            strlen -= MAX_SIZE_LOG_WRITE_KAFKA;
        }
    } 
    else
    {
        if(glLogKafkaProduce->send((S8*)strInfo.c_str(), strInfo.length(), (S8*)strLogHeaderName.c_str(), strLogHeaderName.size(), (S8*)strLogHeaderVal.c_str(), strLogHeaderVal.size()))
        {
            pl_log(ERR, "log send kafka failed");
            return PF_RET_FAILURE;
        }
    }
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @function      write_file_and_statistics
 * @brief         Internal function to write log information to files and make statistics
 * @input         strInformation    string to store log information
 * @output        void
 * @return        void
 *********************************************************************************************/
void write_file_and_statistics(string &strInformation)
{        
    U32 flag = 0;
    
    if(LOG_WRITE_KAFKA == (ulLogWriteFlag & LOG_WRITE_KAFKA))
    {
        if(log_write_kafka(strInformation))
        {
            flag = 1;
        }
    }
    
    if((LOG_WRITE_FILE == (ulLogWriteFlag & LOG_WRITE_FILE)) || (1 == flag))
    {
        pf_write_endof_file((const S8 *)acLogPath, (const S8 *)(strInformation.c_str()), strInformation.length());
    }
    
    PS_CPlus(CM_COM, CMCOM_ID_COPYMSG_ENDOF_CNT);
    PS_CPlusV(CM_COM, CMCOM_ID_COPYMSG_ENDOF_SIZE, strInformation.length());
}

/**********************************************************************************************
 * @function      traversal_log_info
 * @brief         Internal function for traversing log information
 * @input         newTmpV          store the value of the public map
 *                flag             Flag to determine whether the log has been write finish
 *                strInformation   store log information
 *                logReadSize      size of read log space
 * @output        void
 * @return        0                SUCCESS
 *                -1               non conformance, need to continue
 *********************************************************************************************/
U32 traversal_log_info(U32 mid, void* newTmpV, U32* flag, string &strInformation)
{   
    U32 ulLenStatistics = 0;
    
    if(*flag)
    {
        if(SYMBOL_LOG_NOT_WRITTEN == (*(U8*)newTmpV&0x0F))
        {
            g_stLocalLogMapInfo[pstBufInfo->ulNewSeqNum].push_back((char*)newTmpV); 
            pstBufInfo->ulNewSeqNum++;
            *flag = 0;
            PS_CPlus(CM_PES, CMPES_ID_LOG_NOT_WRITE_COMPLETED_CNT_FAIL);
        }
        else if(SYMBOL_LOG_WRITE_COMPLETE == (*(U8*)newTmpV&0x0F))
        {
            strInformation += ((char*)newTmpV + 2);
            if(SYMBOL_USE_LOG_SPACE == *(U8*)newTmpV)
            {
                ulLenStatistics = strlen((char*)newTmpV) + 1;
                pf_thread_mon_update_read_section(mid, ulLenStatistics);
            }
            else if(SYMBOL_USE_APPLICATION_SPACE == *(U8*)newTmpV)
            {
                PS_CPlus(CM_COM, CMCOM_ID_LOG_FREE_CNT);
                pf_free((char*)newTmpV);
                newTmpV = NULL;
            }
            else
            {
                PS_CPlus(CM_PES, CMPES_ID_EXCEPTION_LOG_INFORMATION_CNT_FAIL);
                return PF_RET_FAILURE;
            }
        }
        else
        {
            PS_CPlus(CM_PES, CMPES_ID_EXCEPTION_LOG_INFORMATION_CNT_FAIL);
            return PF_RET_FAILURE;
        }
    }
    else if(0 == (*flag))
    {
        g_stLocalLogMapInfo[pstBufInfo->ulNewSeqNum].push_back((char*)newTmpV);
        pstBufInfo->ulNewSeqNum++;
    }
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @function      pf_log_send
 * @brief         sending log function
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_log_send(void)
{
    U32 k = 0;
    U32 flag = 1;
    U32 backValue = 0;
    string strInfo = "";
    string strNewInfo = "";

    if(g_stLocalLogMapInfo.size())
    {
        std::map<int, std::vector<char *>> NewMap = g_stLocalLogMapInfo;
        g_stLocalLogMapInfo.clear();
        
        std::map<int, std::vector<char *>>::iterator newit;
        for(newit=NewMap.begin();newit!=NewMap.end();newit++)
        {
            std::vector<char *> newTmpV = newit->second;
            U32 mid = *((U8*)newTmpV[k]+1);
            
            /*traverse a log message*/
            backValue = traversal_log_info(mid, newTmpV[k], &flag, strNewInfo);
            if(-1 == backValue)
            {
                continue;
            }
            
        }
        
        /*write logs to files and make statistics*/
        write_file_and_statistics(strNewInfo);
        NewMap.clear();
    }
    
    if(g_stLogMapInfo.size())
    {   
        PF_MUTEX_LOCK(&g_stLogMutex);
        std::map<int, std::vector<char *>> tmpMap = g_stLogMapInfo;
        g_stLogMapInfo.clear();
        PF_MUTEX_UNLOCK(&g_stLogMutex);
        
        std::map<int, std::vector<char *>>::iterator it;
        for(it=tmpMap.begin();it!=tmpMap.end();it++)
        {
            std::vector<char *> tmpV = it->second;
            U32 mid = *((U8*)tmpV[k]+1);
            
            /*traverse a log message*/
            backValue = traversal_log_info(mid, tmpV[k], &flag, strInfo);
            if(-1 == backValue)
            {
                continue;
            }
            
        }
        
        /*write logs to files and make statistics*/
        write_file_and_statistics(strInfo);
        tmpMap.clear();
    }
}


/**********************************************************************************************
 * @function      pf_log_write_file
 * @brief         Used to write the log of memory into the file.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_log_write_file(void)
{
    CHAR ucInfo[SINGLE_LOG_SIZE] = {0};

    pf_timer_stop(ulLogTimerId, MODULE_LOG);
    ulLogTimerId = 0;

    /*write log info into file*/
    pf_log_send();

    sprintf(ucInfo, "pf_log_write_file WIRTE ALL LOGINFO INTO FILE FINISHED\r\n");
    
    /*write log info into file*/
    pf_write_endof_file((const S8 *)acLogPath, (const S8 *)ucInfo, strlen(ucInfo));

    /*restart the log timer*/
    pf_log_timer_init();
}


/**********************************************************************************************
 * @API function  pf_set_module_flag
 * @brief         This interface is used to set the output printing level of the module.
 * @input         ulMid              module ID
                  ulFlag             module log level
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_set_module_flag(U32 ulMid, U32 ulFlag)
{
    if((ulMid < LOG_MODULE_MAX) && (ulFlag <= UINF))
    {
        aulModuleLogFlag[ulMid] = ulFlag;
    }    
}

/**********************************************************************************************
 * @API function  pf_get_module_flag
 * @brief         This interface is used to get the output printing level of the module.
 * @input         ulMid              module ID
 * @output        void
 * @return        ulFlag             module log level
 *********************************************************************************************/
extern "C" U32 pf_get_module_flag(U32 ulMid)
{
    if(ulMid < LOG_MODULE_MAX)
    {
        return aulModuleLogFlag[ulMid] ;
    }

    return 0;
}


/**********************************************************************************************
 * @API function  pf_log_module_flag_close
 * @brief         This interface is used to close the output printing level of the module.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_log_module_flag_close(void)
{
    pstBufInfo = NULL;
}


/**********************************************************************************************
 * @API function  pf_log_module_flag_close
 * @brief         This interface is used to opem the output printing level of the module.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_log_module_flag_open(void)
{
    pstBufInfo = pstBufInfoTmp;
}

bool is_leap_year(U32 year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

/**********************************************************************************************
 * @API function  pf_get_localtime_nolocks
 * @brief         change the localtime to tm structer
 * @input         t           The localtime in second
 * @output        tmp         The structer tm of the localtime
 * @return        none
 *********************************************************************************************/
extern "C" void pf_get_localtime_nolocks(struct tm *tmp, time_t t/*, time_t tz, int dst*/) 
{
    const time_t secs_min = 60;
    const time_t secs_hour = 3600;
    const time_t secs_day = 3600*24;

#if 0
    t -= tz;                            /* Adjust for timezone. */ 
    t += 3600*dst;                      /* Adjust for daylight time. */ 
#else
    t += 28800;                      /* Adjust for daylight time. */ 
#endif

    time_t days = t / secs_day;         /* Days passed since epoch. */ 
    time_t seconds = t % secs_day;      /* Remaining seconds. */
    
    tmp->tm_isdst = 0/*dst*/;
    tmp->tm_hour = seconds / secs_hour;
    tmp->tm_min = (seconds % secs_hour) / secs_min;
    tmp->tm_sec = (seconds % secs_hour) % secs_min;
    
    /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure * where sunday = 0, so to calculate the day of the week we have to add 4 * and take the modulo by 7. */ 
    tmp->tm_wday = (days+4)%7;
    /* Calculate the current year. */ 
    tmp->tm_year = 1970;
    while(1) 
    {
        /* Leap years have one day more. */ 
        time_t days_this_year = 365 + is_leap_year(tmp->tm_year);
        if (days_this_year > days) break; 
        days -= days_this_year;
        tmp->tm_year++;
    }
    tmp->tm_yday = days;/* Number of day of the current year. */
    
    /* We need to calculate in which month and day of the month we are. To do * so we need to skip days according to how many days there are in each * month, and adjust for the leap year that has one more day in February. */ 
    int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    mdays[1] += is_leap_year(tmp->tm_year);
    
    tmp->tm_mon = 0; 
    while(days >= mdays[tmp->tm_mon]) 
    {
        days -= mdays[tmp->tm_mon];
        tmp->tm_mon++; 
    }
    
    tmp->tm_mday = days+1;/* Add 1 since our 'days' is zero-based. */ 
    tmp->tm_year -= 1900; /* Surprisingly tm_year is year-1900. */
} 

/**********************************************************************************************
 * @API function  pf_get_localtime
 * @brief         Used to get local time.
 * @input         void
 * @output        pstTmInfo         output the tm struct of local time
                  pulTime           output the number of local time
 * @return        0                 succuss
                  other             failure (if pstTmInfo and pulTime are both NULL )
 *********************************************************************************************/
S32 pf_get_localtime(struct tm* pstTmInfo, U32* pulTime) 
{
    if(NULL == pulTime)
    {
        if(NULL == pstTmInfo)
        {
            return PF_RET_FAILURE;
        }
        time_t t;
        time(&t);

        pf_get_localtime_nolocks(pstTmInfo, t);
    }
    else
    {
        time_t t;
        time(&t);
        *pulTime = t;
        if(pstTmInfo)
        {
            pf_get_localtime_nolocks(pstTmInfo, *pulTime);
        }
    }
    
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
 * @API function  pf_log
 * @brief         used to output LOG information of the system.
 * @input         ucLogLevel      level of log infomation
                  usModuleId      source module identification
                  format          the address of output infomation
 * @output        void
 * @return        void
 *********************************************************************************************/
#ifdef PF_LOG_WITH_FUNCTION
extern "C" void pf_log(U32 ulModuleId, U32 ulLogLevel, const CHAR* pscFuncName, U32 ulLine, const CHAR* format, ...)
#else
extern "C" void pf_log(U32 ulModuleId, U32 ulLogLevel, const CHAR* format, ...)
#endif
{
    PS_CPlus(CM_COM, CMCOM_ID_LOG_TOTAL_CNT); 
#ifndef WITH_LOG
    return;
#endif
    
    if(NULL == pstBufInfo)
    {
        FUNCTION_TRACE;
        PS_CPlus(CM_COM, CMCOM_ID_LOG_NULL_CNT);
        return;
    }

    long time_before = pf_get_ticks_ns();
    long time_now = 0;
    long time = 0;
    
    struct timeval tv;
    pf_get_timeofday(&tv, NULL);
#if 0
    time_t curr = tv.tv_sec;
    struct tm *info = localtime(&curr);
#else
    struct tm tm_info;
    struct tm *info = &tm_info;
    pf_get_localtime_nolocks(info, tv.tv_sec);
#endif
    U32 ulSpLen = 0;
    U32 ulSnpLen = 0;
    U32 ulMaxLen = 0;
    U32 ulThreadId = 0;
    U32 totaloffset = 0;
    U32 ulAvaiMaxOffset = 0;
    U32 ulSeqNum = 0;                   /*local log number*/
    void* pulogWriteAddr = NULL;
    
    ulThreadId = pf_get_thread_mid();
    pf_thread_mon_get_log_info(ulThreadId, &pulogWriteAddr, &ulAvaiMaxOffset);
   
    PF_MUTEX_LOCK(&g_stLogMutex); 
    pstBufInfo->ulSeqNum++;  
    ulSeqNum = pstBufInfo->ulSeqNum;
    g_stLogMapInfo[ulSeqNum].push_back((char*)pulogWriteAddr);     
    PF_MUTEX_UNLOCK(&g_stLogMutex);
    
    va_list args;
    va_start(args, format);
	
#ifdef PF_LOG_WITH_FUNCTION
    CHAR* pcModName = (CHAR*)pf_get_module_name(ulModuleId);
    U32 ulModLen = strlen(pcModName);
    if(ulModLen > 9)
    {
        pcModName += (ulModLen - 9);
    }

    ulSpLen = sprintf(((char*)pulogWriteAddr+2), "%04d%02d%02d %02d:%02d:%02d.%06d[%03u]%9s:%s:L%04d:%20s:", \
        info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,                              \
        info->tm_hour, info->tm_min, info->tm_sec, tv.tv_usec,                              \
        ulSeqNum,                                                                           \
        pcModName,                                                                          \
        loglevel[ulLogLevel],                                                               \
        ulLine,                                                                             \
        pscFuncName);   
#else
    /*output log Auxiliary information*/
    ulSpLen = sprintf(((char*)pulogWriteAddr+2), "%04d%02d%02d %02d:%02d:%02d.%06d[%s][%03u]%s:",          \
        info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,                              \
        info->tm_hour, info->tm_min, info->tm_sec, tv.tv_usec,                              \
        loglevel[ulLogLevel],                                                               \
        ulSeqNum,                                                                           \
        pf_get_module_name(ulThreadId));
#endif

    if(0xFF == *(U8*)pulogWriteAddr)
    {
        ulMaxLen = ulAvaiMaxOffset - ulSpLen - 6; 
    }
    else 
    {
        ulMaxLen = LOG_STACK_MAX_SIZE - ulSpLen - 6;
    }

    ulSnpLen = vsnprintf((char *)pulogWriteAddr + 2 + ulSpLen, ulMaxLen, format, args);

    //snprintf((char*)pulogWriteAddr + strlen((char*)pulogWriteAddr), 3, "\r\n");
	ulMaxLen = strlen((char*)pulogWriteAddr);
	*((char*)pulogWriteAddr + ulMaxLen) = '\r';
	*((char*)pulogWriteAddr + ulMaxLen + 1) = '\n';
	*((char*)pulogWriteAddr + ulMaxLen + 2) = 0;

    //mark that a log has been written
    *(U8*)pulogWriteAddr -= 1;

    //length of a complete log message
    totaloffset = ulMaxLen + 3;

    //update log space information
    if(SYMBOL_USE_LOG_SPACE == *(U8*)pulogWriteAddr)
    {  
        pf_thread_mon_update_write_section(ulThreadId, totaloffset);
    }
    else 
    {
        //*(U8*)pulogWriteAddr 0xEE
        PS_CPlus(CM_COM, CMCOM_ID_LOG_MALLOC_CNT);
    }
    
    va_end(args);
    PS_CPlus(CM_COM, CMCOM_ID_LOG_CNT);

    if(pf_thread_mon_interval_debug_is_open())
    {
        time_now = pf_get_ticks_ns();
        
        time = time_now - time_before;
        if(time > PS_CGet(CM_MMAX, MODULE_GTEST))
        {
            PS_CSet(CM_MMAX, MODULE_GTEST, time);    
        }
        PS_CPlusV(CM_MTT, MODULE_GTEST, time);
        PS_CPlus(CM_RMSG, MODULE_GTEST);
    }
    return;
}

/**********************************************************************************************
 * @API function  write_log_buf
 * @brief         Converting the memory information in the pf_dump interface into hexadecimal values
 * @input         pByteIn           input address
                  dataLengthInByte  Length of memory remaining to be output
                  pByteOut          output address
                  maxLength         maximum output length
 * @output        void
 * @return        outOffset         offset 
 * @date          2012/11/16
 *********************************************************************************************/
U32 write_log_buf(CHAR* pByteIn, U32* ulDataLengthInByte, CHAR* pByteOut, U32 ulMaxLength)
{
    /*output string offset*/
    U32 outOffset = 0;    
    /*input string index*/
    U32 ulindex = 0;        
    U32 leftOutLen = ulMaxLength;
    U32 leftInLen = *ulDataLengthInByte;
    CHAR tempVal; 

    FUNCTION_TRACE;
    while((leftOutLen >= 3) && (leftInLen > 0))
    {
        if(0 == (outOffset%DUMP_DATA_CHAR_PER_LINE))
        {
            /*Output newline character*/
            pByteOut[outOffset++] = '\n'; 
        }
        else
        {
            /*Output comma*/
            pByteOut[outOffset++] = ',';
        }

        tempVal = pByteIn[ulindex];
        pByteOut[outOffset++] = hex[((tempVal&0xf0)>>4)];
        pByteOut[outOffset++] = hex[(tempVal&0x0f)];

        ulindex++;
        leftOutLen -= 3;
        leftInLen--;
    }

    /*Output newline character*/
    pByteOut[outOffset++] = '\n'; 
    *ulDataLengthInByte = leftInLen;

    return outOffset;
}

static std::string nacos_namespace;
static std::string nacos_group;
static std::string nacos_ip;

static std::string nacos_content; // save the all level configs in json format

static std::vector <std::string> dataIDs; // save all dataID name

static platform_log::log_config_list common_log_config_list;

void set_common_list(void)
{
    std::string str_json;
    str_json = pl_nacos_getconfig(NACOS_CONF_COMMON_FILE, nacos_namespace, nacos_group, nacos_ip);
    if (!str_json.empty())
    {
        json2pb(common_log_config_list, str_json.c_str(), str_json.size());
    }
}

S32 find_and_replace(platform_log::log_config_list& list , std::string key, platform_log::LOG_LEVEL_E value)
{
    S32 find = 0;
    platform_log::log_config_data* pLogCfg;

    /* 1. If aucmodulename exists, then replace the the old ulmodulelevel with new ulmodulelevel */
    for(U32 idx = 0; idx < list.astloglevellist_size(); idx++)
    {
        pLogCfg = list.mutable_astloglevellist(idx);

        /*check whether pLogCfg is NULL or not*/
        if(pLogCfg)
        {
            if(pLogCfg->has_aucmodulename())
            {
                if(pLogCfg->aucmodulename().compare(key.c_str()) == 0)
                {
                    if(pLogCfg->has_ulmodulelevel())
                   {
                       pLogCfg->set_ulmodulelevel(value);
                       find = 1;
                       break;
                   }
                }
            }
        } 
    }

    /* 2. If find none, this key-value is a new entry, just add to the list */
    if(!find)
    {
        pLogCfg = list.add_astloglevellist();
        pLogCfg->set_aucmodulename(key);
        pLogCfg->set_ulmodulelevel(value);
    }
    
    return 0;
}

S32 get_content_from_nacos(std::string &content_combine)
{
    /* check if user had provided the naos cluster's information */
    if (nacos_namespace.empty() || nacos_group.empty() || nacos_ip.empty())
    {
        return PF_NACOS_INFO_HAS_NOT_SET;
    }

    std::string str_json;

    platform_log::log_config_list compose_log_config_list = common_log_config_list;
    platform_log::log_config_list tmp_log_config_list;

    /* Find all dataID, and compose parts into one list */
    for (U32 i = 0; i < dataIDs.size(); i++)
    {
        str_json = pl_nacos_getconfig(dataIDs[i], nacos_namespace, nacos_group, nacos_ip);
        json2pb(tmp_log_config_list, str_json.c_str(), str_json.size());

        for(U32 idx = 0; idx < tmp_log_config_list.astloglevellist_size(); idx++)
        {
            platform_log::log_config_data* pLogCfg = tmp_log_config_list.mutable_astloglevellist(idx);
    
            /*check whether pLogCfg is NULL or not*/
            if(pLogCfg)
            {
                /*check whether pLogCfg has module name or not*/
                if(pLogCfg->has_aucmodulename() && pLogCfg->has_ulmodulelevel())
                {
                    find_and_replace(compose_log_config_list, pLogCfg->aucmodulename(),  pLogCfg->ulmodulelevel());
                }
            } 
        } 
        
    }// end of compose proto
        
    content_combine = pb2json(compose_log_config_list);

    return PF_RET_SUCCESS;
}

/* pl_set_nacos_log_info:
 * Description: set nacos key parameters: namespace, group, and IP of nacos-cluster(or standalone server)
 * Input:
 *      log_namespace: nacos namespace, e.g: "production_env"
 *      log_group: nacos group name, e.g: "DRSU"
 *      log_ip: nacos IP, e.g: "172.16.8.4"
 * Output:
 *       PF_RET_SUCCESS: ok
 */
S32 pl_set_nacos_log_info(std::string &log_namespace, std::string &log_group, std::string &log_ip)
{
    nacos_namespace = log_namespace;
    nacos_group = log_group;
    nacos_ip = log_ip;

    //MR1513: Improve nacos log level management code: read out common configure and set log levels@ this function
    set_common_list();

    return PF_RET_SUCCESS;
}

/* pl_get_nacos_log_info:
 * Description: get all moduleName-logLevels from nacos
 * output:
 *   string& info: string reference to accept the moduleName-logLevels in json format
 * NOTE:
 *    If NO one calls "pl_set_nacos_log_info" and "pl_set_nacos_log_dataID" firstly, it may returns an empty string.
 * return:
 *   PF_RET_SUCCESS: ok
 * 
 */ 
S32 pl_get_nacos_log_info(std::string &info)
{
    info = nacos_content;
    return PF_RET_SUCCESS;
}

/*  pl_set_nacos_log_dataID:
 *  Description: user could set self-defined dataID which declares moduleName-logLevel pairs in json format.
 *  NOTE:
 *      Caller should invoke "pl_set_nacos_log_info" before calling this function.
 *  This funciton will do:
 *  1. Check if nacos key parameters exists: ip, namespace, group. If one of those does Not exist, returns error
 *  2. Check the if the dataID is a new item, if so, inserts to the list(vector)
 *  3. Calling "get_content_from_nacos" according to nacos-ip\nacos-namespace\nacos-group\dataID, retrieves moduleName-logLevels
 *     and save to nacos_content (global variable)
 * 
 *  Input: string dataID, e.g "my_module_loglevel"
 *  return:
 *       PF_RET_SUCCESS: Ok
 *       PF_NACOS_INFO_HAS_NOT_SET: Caller has not called "pl_set_nacos_log_info" yet, caller MUST call that function before calling this one.
 */
S32 pl_set_nacos_log_dataID(std::string &log_dataID)
{
    /* check if user had provided the naos cluster's information */
    if (nacos_namespace.empty() || nacos_group.empty() || nacos_ip.empty())
    {
        return PF_NACOS_INFO_HAS_NOT_SET;
    }

    U32 j = 0;
    
    for (j = 0; j < dataIDs.size(); j++)
    {
        if (dataIDs[j] == log_dataID)
        {
            break; // find the entry
        }
    }

    if (j == dataIDs.size())
    {// No entry is found, it's a new dataID, insert to vector
        dataIDs.push_back(log_dataID);
    }

    return get_content_from_nacos(nacos_content);
}

/* assign_log_level (internal funciton)
 * Description: accept the input string, and get out moduleName-logLevel pairs, then set modules' level by calling "pf_set_module_flag",
 * 
 *  Input: content, which is a json format string, looks like: {"astLogLevelList": [{"aucModuleName": "DRC_VTSM", "ulModuleLevel": 5}]}
 *  Return:
 *       0: success
 */     

S32 assign_log_level(std::string &content)
{
    platform_log::log_config_list gastLogConfigList;
    U32 ulMid = 0;
    
    json2pb(gastLogConfigList, content.c_str(), content.size());

    for(U32 idx = 0; idx < gastLogConfigList.astloglevellist_size(); idx++)
    {
        platform_log::log_config_data* pLogCfg = gastLogConfigList.mutable_astloglevellist(idx);

        /*check whether pLogCfg is NULL or not*/
        if(pLogCfg)
        {
            /*check whether pLogCfg has module name or not*/
            if(pLogCfg->has_aucmodulename())
            {
                ulMid = pf_get_module_id(pLogCfg->aucmodulename().c_str());
                /*check whether pLogCfg has module level or not*/
                if(pLogCfg->has_ulmodulelevel())
                {
                    pf_set_module_flag(ulMid, pLogCfg->ulmodulelevel());
                    //printf("--> %s, %d \n", pLogCfg->aucmodulename().c_str(), pLogCfg->ulmodulelevel());
                }
            }
        }
    }

    return 0;
}

/* write_to_current_file: (internal function)
 * Description: 
 *   write moduleName-logLevel pairs to a specific file(current/log_config.dat)
 * Input:
 *      str_json: string reference of moduleName-logLeves. e.g: {"astLogLevelList": [{"aucModuleName": "DRC_VTSM", "ulModuleLevel": 5}]}
 *      NOTE:
 *          If logLevel is "DO_NOT_USE"(value = 0), this pair will not be write to the file
 * Return:
 *      PF_RET_SUCCESS: ok
 */ 
S32 write_to_current_file(std::string &str_json)
{
    CHAR acDstPath[SINGLE_LOG_SIZE];
    
    platform_log::log_config_list config_list;

    json2pb(config_list, str_json.c_str(), str_json.size());
            
    for(U32 idx = 0; idx < config_list.astloglevellist_size(); idx++)
    {
        platform_log::log_config_data* pLogCfg = config_list.mutable_astloglevellist(idx);

        /*check whether pLogCfg is NULL or not*/
        if(pLogCfg)
        {
            /*check whether pLogCfg has module name or not*/
            if(pLogCfg->has_aucmodulename())
            {
                // If level is NOT DO_NOT_USE (0), remove the according name-level pair
                if(pLogCfg->has_ulmodulelevel() && (pLogCfg->ulmodulelevel()) == platform_log::LOG_LEVEL_E::DO_NOT_USE)
                {
                      pLogCfg->clear_aucmodulename();
                      pLogCfg->clear_ulmodulelevel();
                }
            }
        } 
    } 

    std::string content = pb2json(config_list);
    // MR1513: Improve nacos log level management code: file name will be log_config.dat.lastDataID
    std::string last_dataID = dataIDs[dataIDs.size()-1];
    sprintf((CHAR*)acDstPath, "%s%s.%s", pf_get_root_path(), LOG_MODULE_CURRENT_FILE_PATH, last_dataID.c_str());

    pf_write_flush_file((const S8*)acDstPath, (const S8*)content.c_str(), content.size());
    
    return PF_RET_SUCCESS;
}

S32 log_write_kafka_init(CHAR* pcFilePath)
{
    //read log config file
    U32 ulFileLen = 0;
    U32 idy = 0;
    CHAR acPath[SINGLE_LOG_SIZE];
    
    sprintf((CHAR*)acPath, "%s%s", pf_get_root_path(), pcFilePath);

    if(!pf_is_file_exist((S8 *) acPath))
    {
        return PF_RET_FAILURE;
    }

    if(PF_RET_SUCCESS != pf_get_file_length((S8*)acPath, &ulFileLen))
    {
        pl_log(INF,"get log config file len failed,config file may don't exist");
        return PF_RET_FAILURE;
    }

    if(0 != ulFileLen)
    {
        S8* pTmpMemory = (S8*)pl_malloc(ulFileLen);
        if(NULL == pTmpMemory)
        {
            pl_log(ERR, "malloc failed %s %d", __FUNCTION__, __LINE__);
            return PF_RET_FAILURE;
        }
        platform_log::log_config_list gastLogConfigList;
        
        if(PF_RET_SUCCESS != pf_read_flush_file((const S8*)acPath,(const S8*)pTmpMemory,ulFileLen))
        {
            pl_log(ERR,"read acu config file failed");
            pl_free(pTmpMemory);
            pTmpMemory = NULL;
            return PF_RET_FAILURE;
        }
        
        json2pb(gastLogConfigList, (const char*)pTmpMemory, ulFileLen);
        pl_free(pTmpMemory);
        pTmpMemory = NULL;
        
        platform_log::log_config_list pLogCfg = gastLogConfigList;
        
        /*check whether pLogCfg has  log write flag or not*/
        if(pLogCfg.has_u32logwriteflag())
        {
            /*check whether pLogCfg has log producer or not*/
            if(pLogCfg.has_stlogproducer())
            {                
                kafka_config::kafka_producer_init_list* pKafkaCfg  = gastLogConfigList.mutable_stlogproducer();
                if(pKafkaCfg)
                {
                    if(pKafkaCfg->has_brokers())
                    {
                        glLogKafkaProduce = new kafka_producer;
                        
                        if(glLogKafkaProduce->configure(pKafkaCfg->brokers(), pKafkaCfg->topics(), &my_log_handle_mq_send_err, pKafkaCfg->idempotent()))
                        {   
                            pl_log(ERR, "pKafkaCfg configure ERROR configure brokers %s topics %s", pKafkaCfg->brokers().c_str(), pKafkaCfg->topics().c_str());
                            return PF_RET_FAILURE;
                        }
                        
                        if(pKafkaCfg->has_sec_config())
                        {
                            kafka_config::kafka_init_security* pKafkaSec = pKafkaCfg->mutable_sec_config();
                            if(glLogKafkaProduce->configure_acl(pKafkaSec->security_protocol(), pKafkaSec->sasl_mechanism(), pKafkaSec->user_name(), pKafkaSec->user_passwd()))
                            {
                                pl_log(ERR, "pKafkaCfg configure acl protocol %s mechanism %s name %s password %s failed\r\n", \
                                       pKafkaSec->security_protocol().c_str(), pKafkaSec->sasl_mechanism().c_str(), pKafkaSec->user_name().c_str(), pKafkaSec->user_passwd().c_str());
                                return PF_RET_FAILURE; 
                            }
                        }        

                        for(idy = 0; idy < pKafkaCfg->kafka_init_list_size(); idy++)
                        {            
                            kafka_config::kafka_init_data* pInitList = pKafkaCfg->mutable_kafka_init_list(idy);    
                            if (glLogKafkaProduce->set_config( pInitList->config_key(), pInitList->config_value()))    
                            {            
                                pl_log(ERR, "pKafkaCfg set config %s to %s failed", pInitList->config_key().c_str(), pInitList->config_value().c_str());
                                return PF_RET_FAILURE; 
                            }
                        }

                        if (glLogKafkaProduce->create_producer())    
                        {         
                            pl_log(ERR, "pKafkaCfg create_producer  ERROR ");
                            return PF_RET_FAILURE;      
                        }  

                        if(pKafkaCfg->has_send_api_key())
                        {
                            if(glLogKafkaProduce->set_config("custom.key", pKafkaCfg->send_api_key()))
                            {
                                pl_log(ERR, "pKafkaCfg set config send_api_key %s failed\r\n", pKafkaCfg->send_api_key().c_str());
                                return PF_RET_FAILURE; 
                            }
                        }
                        
                        ulLogWriteFlag = pLogCfg.u32logwriteflag();
                    }
                } 
            }
        }
        
        return PF_RET_SUCCESS;
    }
    
    return PF_RET_FAILURE;
}

S32 log_module_init(CHAR* pcFilePath)
{
    /* MR 1492: set log levels /current-log-conif-file via nacos */
    std::string content;

    pl_log(INF, "Try to get log level config from nacos");   
    
    if (PF_RET_SUCCESS == get_content_from_nacos(content))
    {
        assign_log_level(content);
    
        write_to_current_file(content);
        
        return PF_RET_SUCCESS;
    }
    
    //read log module config file
    U32 ulFileLen = 0;

    CHAR acPath[SINGLE_LOG_SIZE];
    
    sprintf((CHAR*)acPath, "%s%s", pf_get_root_path(), pcFilePath);

    if(!pf_is_file_exist((S8 *) acPath))
    {
        return PF_RET_FAILURE;
    }

    if(PF_RET_SUCCESS != pf_get_file_length((S8*)acPath, &ulFileLen))
    {
        pl_log(INF,"get log config file len failed,config file may don't exist");
        return PF_RET_FAILURE;
    }
    
    if(0 != ulFileLen)
    {
        S8* pTmpMemory = (S8*)pl_malloc(ulFileLen);
        platform_log::log_config_list gastLogConfigList;
        U32 ulMid;
        if(PF_RET_SUCCESS != pf_read_flush_file((const S8*)acPath,
                                            (const S8*)pTmpMemory,ulFileLen))
        {
            pl_log(ERR,"read acu config file failed");
            return PF_RET_FAILURE;
        }
    
        json2pb(gastLogConfigList, (const char*)pTmpMemory, ulFileLen);
        pl_free(pTmpMemory);
        pTmpMemory = NULL;
    
        for(U32 idx = 0; idx < gastLogConfigList.astloglevellist_size(); idx++)
        {
            platform_log::log_config_data* pLogCfg = gastLogConfigList.mutable_astloglevellist(idx);

            /*check whether pLogCfg is NULL or not*/
            if(pLogCfg)
            {
                /*check whether pLogCfg has module name or not*/
                if(pLogCfg->has_aucmodulename())
                {
                    ulMid = pf_get_module_id(pLogCfg->aucmodulename().c_str());
                    /*check whether pLogCfg has module level or not*/
                    if(pLogCfg->has_ulmodulelevel())
                    {
                        pf_set_module_flag(ulMid, pLogCfg->ulmodulelevel());
                    }
                }
            }
        }

        CHAR acDstPath[SINGLE_LOG_SIZE];
        sprintf((CHAR*)acDstPath, "%s%s", pf_get_root_path(), LOG_MODULE_CURRENT_FILE_PATH);
        
        pf_copy_flush_file((const CHAR *)acDstPath, (const CHAR *)acPath);

        return PF_RET_SUCCESS;
    }
    
    return PF_RET_FAILURE;
}

void log_module_reinit(void)
{
    if(PF_RET_SUCCESS == log_module_init(LOG_MODULE_TMP_FILE_PATH))
    {
        CHAR acPath[SINGLE_LOG_SIZE];
        
        sprintf((CHAR*)acPath, "%s%s", pf_get_root_path(), LOG_MODULE_TMP_FILE_PATH);

        unlink(acPath);
    }
}


/**********************************************************************************************
 * @function      log_init
 * @brief         log thread initial
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
S32 log_init(U32 ulModuleId)
{
    S32 ret;
    U32 i;
    U32 ulLen;

    /*MR10919 for commercial ONLY*/
    for(i=0; i<LOG_MODULE_MAX; i++)
    {
        aulModuleLogFlag[i] = DO_NOT_USE;
    }

    /*log buffer initial*/
    pf_log_buffer_init();

    sprintf(acLogPath, "%s/log.txt", pf_get_root_path());

    if(PF_RET_SUCCESS == pf_get_file_length((S8 *)acLogPath, &ulLen))
    {
        if(ulLen)
        {
            CHAR acCmdInfo[SINGLE_LOG_SIZE];
            S32 slRet;
            U32 ulLen;
            struct timeval tv;
            pf_get_timeofday(&tv, NULL);
            struct tm tm_info;
            struct tm *info = &tm_info;
            pf_get_localtime_nolocks(info, tv.tv_sec);
            
            slRet = snprintf(acCmdInfo, SINGLE_LOG_SIZE, "mv %s %s/log/%04d%02d%02d_%02d%02d%02d_log.txt",          \
                acLogPath, pf_get_root_path(),
                info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,                                \
                info->tm_hour, info->tm_min, info->tm_sec);

            pf_set_system_call((const S8 *) acCmdInfo);
            // printf("log file exists, %s\r\n", acCmdInfo);
            pl_log(INF, "log file exists, %s", acCmdInfo);
        }
    }

    //read log module config file
    ret = log_module_init(LOG_MODULE_FILE_PATH);

    log_write_kafka_init(LOG_MODULE_FILE_PATH);
        
    return ret;
}

/**********************************************************************************************
 * @API function  pf_get_log_path
 * @brief         get the log path
 * @input         void
 * 
 * @output        none
 * @return        the log path of the process 
 *********************************************************************************************/
extern "C" CHAR* pf_get_log_path(void)
{
    return (CHAR*)acLogPath;  
}

/**********************************************************************************************
 * @API function  pf_set_log_path
 * @brief         set the log path, should be called after the log_init function
 * @input         pcLogPath       the destination path of log file
 * 
 * @output        none
 * @return        0                     succuss
                  other                 failure
 *********************************************************************************************/
extern "C" S32 pf_set_log_path(CHAR* pcLogPath)
{
    if(!pcLogPath)
    {
        return PF_RET_FAILURE;
    }

    CHAR acLogInfo[SINGLE_LOG_SIZE];
    S32 slRet;
    U32 ulLen;
    struct timeval tv;
    pf_get_timeofday(&tv, NULL);
    struct tm tm_info;
    struct tm *info = &tm_info;
    pf_get_localtime_nolocks(info, tv.tv_sec);

    /*check the length of the path*/
    if(strlen(pcLogPath) >= SINGLE_LOG_SIZE)
    {
        printf("LOG path %s length is %d, is more than 1024 Bytes \r\n", pcLogPath, strlen(pcLogPath));
        return PF_RET_FAILURE;
    }

    slRet = snprintf(acLogInfo, SINGLE_LOG_SIZE, "%04d%02d%02d %02d:%02d:%02d.%06d: write into %s\r\n",          \
        info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,                              \
        info->tm_hour, info->tm_min, info->tm_sec, tv.tv_usec, pcLogPath);

    /*check the result of snprintf*/
    if((PF_RET_FAILURE == slRet) || (slRet >= SINGLE_LOG_SIZE))
    {
        printf("LOG INFO %s ERROR, MAYBE more than 1024 Bytes %d \r\n", acLogInfo, slRet);
        slRet = SINGLE_LOG_SIZE;
        //return PF_RET_FAILURE;
    }

    /*check the file path is valid or not*/
    if(PF_RET_SUCCESS == pf_write_endof_file((const S8 *)pcLogPath, (const S8 *)acLogInfo, strlen(acLogInfo)))
    {
        //pf_log_send();

        //pthread_mutex_lock(&(pstBufInfo->mlock));
        if(PF_RET_SUCCESS == pf_get_file_length((S8 *)acLogPath, &ulLen))
        {
            if(ulLen)
            {
                S8* pscData = (S8*)pf_malloc(ulLen);

                if(pscData)
                {
                    pf_read_flush_file((const S8 *) acLogPath, (const S8 *)pscData, ulLen);
                    pf_write_endof_file((const S8 *) pcLogPath, (const S8 *)pscData, ulLen);
                    unlink(acLogPath);
                    pf_free((void *)pscData);
                }
            }
        }

        //pthread_mutex_unlock(&(pstBufInfo->mlock));

        printf("THE PATH of LOG FILE is changed from %s to %s\r\n", acLogPath, pcLogPath);
        pf_memset(acLogPath, 0, sizeof(acLogPath));
        
        snprintf(acLogPath, SINGLE_LOG_SIZE, "%s", pcLogPath);
        return PF_RET_SUCCESS;
    }

    printf("LOG FILE PATH %s is writed ERROR\r\n", pcLogPath);
    return PF_RET_FAILURE;  
}



/**********************************************************************************************
 * @API function  log_upload_time
 * @brief         This interface is used to set the output printing level of the module.
 * @input         pCfgMsg            module ID
 * @output        void
 * @return        void
 *********************************************************************************************/
U32 log_upload_time(OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG* pCfgMsg)
{
    U32 day; 
    U32 sec;
    U32 ttime;//target time
    U32 ctime;//current time
    U32 ulIntTime = pCfgMsg->stLogUploadPara.ulDevUploadOffset;
    U32 ulTimeCycle = pCfgMsg->stLogUploadPara.ulTimeCycle;
    U32 tmp;
    struct tm stTmInfo;

    ctime = (U32)time(NULL);

    if(ulTimeCycle)
    {
        day = atoi((CHAR*)pCfgMsg->stLogUploadPara.strTartDate);
        sec = atoi((CHAR*)pCfgMsg->stLogUploadPara.strTime);

        stTmInfo.tm_year = day/10000 - 1900;
        stTmInfo.tm_mon = ((day/100)%100) - 1;
        stTmInfo.tm_mday = day%100;

        stTmInfo.tm_hour = sec/10000;
        stTmInfo.tm_min = (sec/100) % 100;
        stTmInfo.tm_sec = sec%100;

        stTmInfo.tm_isdst = 0/*dst*/;

        ttime = (U32)mktime(&stTmInfo);

        if(ttime + ulIntTime >= ctime)
        {
            tmp = ttime + ulIntTime - ctime;
            tmp /= ulTimeCycle;
            return ttime + ulIntTime + (tmp*ulTimeCycle);
        }
        else
        {
            tmp = ctime - ttime - ulIntTime;
            tmp /= ulTimeCycle;
            return ttime + ulIntTime + ((tmp + 1) * ulTimeCycle);
        }
    }
            
    return ctime;
}


U32 log_send_to_ftp_server(OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG* pCfgMsg, U32 ulTime,U32 ulCfgSrcMid)
{
    S8 ascStartTime[MAX_DATE_AND_TIME_LEN];
    U8 strStartTime[MAX_DATE_AND_TIME_LEN];
    CHAR ascPath[SINGLE_LOG_SIZE];
    CHAR* pscRootPath = pf_get_root_path();
    struct tm info;
    pf_get_localtime_nolocks(&info, ulTime);
    
    OAM_FTP_PUT_NEW_FILE_REQ_MSG ftpFileMsg;
    pf_memset(&ftpFileMsg, 0, sizeof(ftpFileMsg));

    ftpFileMsg.ulProceType = PROCE_TYPE_COMMON_FILE;
    ftpFileMsg.ulCfgSrcMid = ulCfgSrcMid;
    pf_memcpy(ftpFileMsg.ascLoginName, pCfgMsg->stLogUploadPara.ascLoginName, FTP_LOGIN_NAME_LEN);
    pf_memcpy(ftpFileMsg.ascPassword, pCfgMsg->stLogUploadPara.ascPassword, FTP_LOGIN_PASSWORD_LEN);
    pf_memcpy(&ftpFileMsg.stFtpAddress, &pCfgMsg->stLogUploadPara.stFtpAddress, sizeof(ftp_address));

    CHAR* pscDrName = strstr(pscRootPath, "dr");
    if(NULL == pscDrName)
    {
        pscDrName = pscRootPath;
    }
    else
    {
        pscDrName = strstr((pscDrName+2), "dr");
        if(!pscDrName)
        {
            pscDrName = strstr(pscRootPath, "dr");
        }
    }
    
    pf_memset(strStartTime,0,MAX_DATE_AND_TIME_LEN);   
    pf_read_flush_file((const S8 *)acLogPath, (const S8 *)ascStartTime, MAX_DATE_AND_TIME_LEN);
    pf_memcpy(&strStartTime[0], ascStartTime, 8);
    strStartTime[8] = ascStartTime[9];
    strStartTime[9] = ascStartTime[10];
    strStartTime[10] = ascStartTime[12];
    strStartTime[11] = ascStartTime[13];

    sprintf((CHAR*)ftpFileMsg.ascVersionNo, "%s_run_%s_%04d%02d%02d%02d%02d.log", pscDrName, strStartTime,   \
            info.tm_year + 1900, info.tm_mon + 1, info.tm_mday,                                 \
            info.tm_hour, info.tm_min);

    sprintf(ascPath, "%s/ftp/%s", pscRootPath, (CHAR*)ftpFileMsg.ascVersionNo);
    if(PF_RET_SUCCESS != pf_copy_flush_file(ascPath, acLogPath))
    {
        return PF_RET_FAILURE;
    }
    unlink(acLogPath);

    pf_copy_msg(MODULE_LOG, OAM_FTP_PUT_LOG_FILE_REQ, MODULE_FTP, &ftpFileMsg, sizeof(OAM_FTP_PUT_NEW_FILE_REQ_MSG));

    return PF_RET_SUCCESS;
}

U32 daily_send_to_ftp_server(OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG* pCfgMsg,U32 ulCfgSrcMid)
{
    LOG_RECORD_FTP_CFG_REQ stRecReq;

    stRecReq.ulCfgSrcMid = ulCfgSrcMid;
    pf_memcpy(&stRecReq.stFtpAddress, &pCfgMsg->stLogUploadPara.stFtpAddress, sizeof(ftp_address));
    pf_memcpy(stRecReq.ascLoginName, pCfgMsg->stLogUploadPara.ascLoginName, FTP_LOGIN_NAME_LEN);
    pf_memcpy(stRecReq.ascPassword, pCfgMsg->stLogUploadPara.ascPassword, FTP_LOGIN_PASSWORD_LEN);

    pf_copy_msg(MODULE_LOG, 
            LOG_RECORD_PUT_FTP_FILE_REQ, 
            MODULE_DAILYREC, 
            &stRecReq, 
            sizeof(LOG_RECORD_FTP_CFG_REQ));
    return PF_RET_SUCCESS;
}

U32 log_send_to_ftp_server_now(OAM_PLATFORM_UPLOAD_LOG_NOW_REQ_MSG* pLogUploadNow,U32 ulCfgSrcMid)
{
    S8 ascStartTime[MAX_DATE_AND_TIME_LEN];
    U8 strStartTime[MAX_DATE_AND_TIME_LEN];
    CHAR ascPath[SINGLE_LOG_SIZE];
    CHAR* pscRootPath = pf_get_root_path();
    U32 ctime = (U32)time(NULL);//current time
    
    struct tm info;
    pf_get_localtime_nolocks(&info, ctime);
    
    FTP_UPLOAD_LOG_NOW_REQ_MSG ftpFileMsg;
    pf_memset(&ftpFileMsg, 0, sizeof(ftpFileMsg));

    ftpFileMsg.ulTransId= pLogUploadNow->ulTransId;
    ftpFileMsg.ulCfgSrcMid = ulCfgSrcMid;
    pf_memcpy(ftpFileMsg.ascLoginName, pLogUploadNow->ascLoginName, FTP_LOGIN_NAME_LEN);
    pf_memcpy(ftpFileMsg.ascPassword, pLogUploadNow->ascPassword, FTP_LOGIN_PASSWORD_LEN);
    pf_memcpy(&ftpFileMsg.stFtpAddress, &pLogUploadNow->stFtpAddress, sizeof(ftp_address));

    CHAR* pscDrName = strstr(pscRootPath, "dr");
    if(NULL == pscDrName)
    {
        pscDrName = pscRootPath;
    }
    else
    {
        pscDrName = strstr((pscDrName+2), "dr");
        if(!pscDrName)
        {
            pscDrName = strstr(pscRootPath, "dr");
        }
    }

    pf_memset(strStartTime,0,MAX_DATE_AND_TIME_LEN);   
    pf_read_flush_file((const S8 *)acLogPath, (const S8 *)ascStartTime, MAX_DATE_AND_TIME_LEN);
    pf_memcpy(&strStartTime[0], ascStartTime, 8);
    strStartTime[8] = ascStartTime[9];
    strStartTime[9] = ascStartTime[10];
    strStartTime[10] = ascStartTime[12];
    strStartTime[11] = ascStartTime[13];

    sprintf((CHAR*)ftpFileMsg.ascVersionNo, "%s_run_%s_%04d%02d%02d%02d%02d.log", pscDrName, strStartTime,   \
            info.tm_year + 1900, info.tm_mon + 1, info.tm_mday,                                 \
            info.tm_hour, info.tm_min);

    sprintf(ascPath, "%s/ftp/%s", pscRootPath, (CHAR*)ftpFileMsg.ascVersionNo);
    if(PF_RET_SUCCESS != pf_copy_flush_file(ascPath, acLogPath))
    {
        return PF_RET_FAILURE;
    }
    unlink(acLogPath);

    pf_copy_msg(MODULE_LOG, OAM_FTP_PUT_NEW_FILE_NOW_REQ, MODULE_FTP, &ftpFileMsg, sizeof(FTP_UPLOAD_LOG_NOW_REQ_MSG));

    return PF_RET_SUCCESS;

}

extern int pf_get_message(void* pMsgQ, stMboxMessage& pstMsg);

extern int pf_mbox_peek(void* pMsgQ);

#ifdef GET_NACOS_CONFIG_BY_LISTEN
S32 handler_of_common(std::string info)
{
    pl_log(INF, "Change of common: %s", info.c_str());
    
    assign_log_level(info);

    std::string content;
    
    if (PF_RET_SUCCESS == get_content_from_nacos(content))
    {
        write_to_current_file(content);
    }
    
    return 0;
}

S32 handler_of_service(std::string info)
{
    pl_log(INF, "Change of service: %s", info.c_str());
    
    assign_log_level(info);

    std::string content;
    
    if (PF_RET_SUCCESS == get_content_from_nacos(content))
    {
        write_to_current_file(content);
    }
    
    return 0;
}
#endif

void log_entry(pf_addrword_t mid)
{
    stMboxMessage msg;
    U32 ulQueueSize = 0;
    U32 trace_seq = 0;
    U32 count = 0;
    OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG log_msg;
    U32 ulDevId = 0;
    U32 ulTimer = 0;//log upload time, 0: do not upload
    U32 ulCfgSrcMid = 0; 
    U32 ctime = 0; //current time
    S8 scPath[SINGLE_LOG_SIZE];
    long time_switch_bef = 0;
    long time_switch_now = 0;
    long time_switch = 0;
    BOOL bDebugFlag = pf_thread_mon_interval_debug_is_open();
    
    sprintf((CHAR*)scPath, "%s/statics.bin", pf_get_root_path());
    CHAR* pcDevId = strstr(pf_get_root_path(), "_");
    if(pcDevId)
    {
        ulDevId = atoi(pcDevId+1);
    }

    if(pf_get_big_endian_flag())
    {
        // printf("CURRENT PLATFORM IS BIG ENDIAN\n");
        pl_log(UINF, "CURRENT PLATFORM IS BIG ENDIAN");
    }
    else
    {
        // printf("CURRENT PLATFORM IS LITTLE ENDIAN\n");
        pl_log(UINF, "CURRENT PLATFORM IS LITTLE ENDIAN");
    }
    // printf("msg id %lld %llx msgQArray %lld %llx\n", msgQArray[(int)mid], msgQArray[(int)mid], msgQArray[MODULE_LOG], msgQArray[MODULE_LOG]);        
    pl_log(INF, "msg id %lld %llx msgQArray %lld %llx", \
                           msgQArray[(int)mid], msgQArray[(int)mid], msgQArray[MODULE_LOG], msgQArray[MODULE_LOG]);
    /*log timer initial*/
    pf_log_timer_init();
    
#ifdef GET_NACOS_CONFIG_BY_LISTEN
    pl_nacos_listener log_common_monitor;
    pl_nacos_listener log_service_monitor;
    //S32 listen_to_key(std::string key, std::string group, std::string nacos_ip, HANDLE_KEY_CHANGED callback);
    // typedef S32 (*HANDLE_KEY_CHANGED)(std::string);
    log_common_monitor.listen_to_key("common", "platform.log", "172.16.8.4", handler_of_common);
    log_service_monitor.listen_to_key("service.log", "platform.log", "172.16.8.4", handler_of_service);
#endif
    
    while(1)
    {
        do
        {
            pf_get_message((void*)msgQArray[(int)mid], msg);

            /*This is about execution time test*/
            time_switch_bef = pf_get_ticks_ns();
            
            switch(msg.ulMsgId)
            {
                case TIMER_EXPIRY_MSG:
                {
                    TIMER_EXPIRE_MSG_S* tmpTimerMsg = (TIMER_EXPIRE_MSG_S*)(msg.pucData);

                    if(TIMER_INVALID_SOCKET_DELETE_REQ == tmpTimerMsg->ulTypeId)
                    {
                        if(tmpTimerMsg->ullPara != 0)
                        {
                            int socketFd = ((PlatformSocket*)(tmpTimerMsg->ullPara))->GetSocket();

                            delete (PlatformSocket*)(tmpTimerMsg->ullPara);
                            pl_log( WARN," delete platformSocket_0x%08x , socketFd_%d , timerid_%d ",tmpTimerMsg->ullPara,socketFd,tmpTimerMsg->ulTimerId);

//                            pf_log(THIS_LOG_MODULE, WARN, __FUNCTION__, __LINE__," delete platformSocket_0x%08x , socketFd_%d , timerid_%d ",tmpTimerMsg->ullPara,socketFd,tmpTimerMsg->ulTimerId);
                        }
                        break;
                    }

                    FUNCTION_TRACE;
                    pf_log_send();
                    FUNCTION_TRACE;
                    count++;
                    if(0 == count % 100)
                    {
                        /*set log level of each module in the temp file*/
                        log_module_reinit();
                        /*show the performance information of platform*/
                        // pf_show_performance();
                        /*monitor stack usage*/
                        pf_thread_mon_check_stack_usage();
                        /*update the common statics into last area*/
                        if(bDebugFlag)
                        {
                            pf_write_flush_file((const S8 *)scPath, (const S8 *)g_ulPSCnts, PSCNT_TOTAL_LENGTH);//65536=CM_MAX*CI_MAX*sizeof(U64)
                        }
                        pf_memcpy(&(g_ulPSCnts[CM_LCOM][0]), &(g_ulPSCnts[CM_COM][0]), PSCNT_COPY_LENGTH);//8192=4*CI_MAX*sizeof(U64)
                        
                        //check the status of log switch 
                        if(ulTimer)
                        {
                            ctime = (U32)time(NULL);
                            /*If the current time is greater than or equal to the log upload time, 
                            the log will be uploaded to the ftp server*/
                            if(ctime >= ulTimer)
                            {
                                log_send_to_ftp_server(&log_msg, ulTimer,ulCfgSrcMid);
                                daily_send_to_ftp_server(&log_msg,ulCfgSrcMid);

                                //if ulTimeCycle is 0, send log file only once
                                if(log_msg.stLogUploadPara.ulTimeCycle)
                                {
                                    ulTimer += log_msg.stLogUploadPara.ulTimeCycle;
                                }
                                else
                                {
                                    ulTimer = 0;
                                }
                            }
                        }
#if DRC_PROCESS
                        pf_stat_update_thread_info(NULL);
#endif                        
                    }
                    break;
                }

                case OAM_PLATFORM_LOG_PARA_CFG_CMD:
                {
                    OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG* pLogMsg = (OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG*)msg.pucData; 
                    pf_memcpy(&log_msg, (const void *)pLogMsg, sizeof(OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG));
                    ulCfgSrcMid = msg.ulSrcModuleId;

                    /*The interval time is used to ensure that different devices upload logs at different time points, 
                    so as to realize the load flow balance of the system*/
                    log_msg.stLogUploadPara.ulDevUploadOffset = (ulDevId%256) * pLogMsg->stLogUploadPara.ulDevUploadOffset + 1;

#ifdef UNIT_TEST
                    log_msg.stLogUploadPara.ulTimeCycle = pLogMsg->stLogUploadPara.ulTimeCycle * 180;//using for test
#else
                    log_msg.stLogUploadPara.ulTimeCycle = pLogMsg->stLogUploadPara.ulTimeCycle * 86400;//
#endif

#ifdef GTEST_EN
                    log_msg.stLogUploadPara.ulTimeCycle = pLogMsg->stLogUploadPara.ulTimeCycle * 180;//using for gtest
#endif
                    ulTimer = log_upload_time(&log_msg);
                    //printf("OAM_PLATFORM_LOG_PARA_CFG_CMD ulDevId=%d iteme = %d ulTimer=%d\r\n", ulDevId, log_msg.stLogUploadPara.ulTimeCycle, ulTimer);
                    break;
                }
                
                case OAM_PLATFORM_LOG_SWITCH_CTRL_CMD:
                {
                    OAM_PLATFORM_LOG_SWITCH_CTRL_CMD_MSG* pLogCmd = (OAM_PLATFORM_LOG_SWITCH_CTRL_CMD_MSG*)msg.pucData; 
                    if(SWITCH_ON == pLogCmd->eLogOnOffFlag)
                    {
                        //if the log upload switch on, set upload time to ulTimer
                        ulTimer = log_upload_time(&log_msg);
                    }
                    else
                    {
                        //if the log upload switch off, set upload time to 0
                        ulTimer = 0;
                    }
                        
                    break;
                }
                case OAM_PLATFORM_LOG_UPLOAD_NOW_REQ:
                {
                    OAM_PLATFORM_UPLOAD_LOG_NOW_REQ_MSG* pLogUploadNow = (OAM_PLATFORM_UPLOAD_LOG_NOW_REQ_MSG*)msg.pucData; 
                    log_send_to_ftp_server_now(pLogUploadNow,msg.ulSrcModuleId);                       
                }
                 
                default: 
                    FUNCTION_TRACE;
                    break;
            }

            if(bDebugFlag)
            {
                time_switch_now = pf_get_ticks_ns();
        
                time_switch = time_switch_now - time_switch_bef;
                if(time_switch > PS_CGet(CM_MMAX, MODULE_LOG))
                {
                    PS_CSet(CM_MMID, MODULE_LOG, msg.ulMsgId);
                    PS_CSet(CM_MMAX, MODULE_LOG, time_switch);    
                }
                
                PS_CPlusV(CM_MTT, MODULE_LOG, time_switch);
            }
            
            PS_CPlus(CM_RMSG, (CMRMSG_ID_RCVD_MSGQ_MID_OFFSET + MODULE_LOG));
            msg.free();
            ulQueueSize = pf_mbox_peek((void*)msgQArray[(int)mid]);
        } while(ulQueueSize);

    }

}


