/************************************************************
  Copyright (C), BroadXt Inc  2019
  FileName: pl.h
  Author: josephzhou    Version :  1.0   Date: 20190617
  Description:     header of platform
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2019/6/17         1.0        initial
***********************************************************/
#ifndef _PL_H
#define _PL_H

#include "pl_type.h"
#include "module.h"
#include "option.h"
#include "pl_counts.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "if_pl_ftp.h"


#include <map>
#include <vector>
#include <string>
#include <memory>
using std::string;


/*All compiler switches are placed in option.h. Don't put them anywhere else*/
typedef enum
{
    PF_RET_FAILURE = -1,
    PF_RET_SUCCESS = 0,
}PF_RETURN_VALUE_E;


typedef enum
{
    DO_NOT_USE,     //Log at this level is currently designed to be closed by void method. Don't use this level.
    FATAL,          //fatal error
    ERR,            //error
    WARN,           //warning
    INF,            //Information for displaying context, global variables, values of statistical variables, etc. Control Plane
    TRC,            //Used to indicate the arrival and delivery of some messages, including some field values in the display, etc. Control Plane
    UINF,           //User plane LOG��By default, this level of LOG is turned off.
}LOG_OUTPUT_LEVEL_E;

typedef enum
{
    WARM_RESET_PROCESS,         //reset the process
    WARM_RESET_SYSTEM,          //reboot the system
    WARM_RESET_MAX,             //max ID
}WARM_RESET_FLAG_E;


typedef struct
{
    U32 size;                   // the size of the buff
    U32 total;                  // the total num of buff
    U32 avail;                  // the num which is available for malloc
}MEMPOOL_INFO;


struct stCpuCoreMask
{
    U32     mask0;        //  cpu0 ~ cpu31
    U32     mask1;        //  cpu32 ~ cpu63
    U32     mask2;        //  cpu64 ~ cpu95
    U32     mask3;        //  cpu96 ~ cpu127
};

typedef enum
{
    DISK_INFO_UNIT_KB,         
    DISK_INFO_UNIT_MB,          
    DISK_INFO_UNIT_GB,             
}PF_DISK_INFO_UNIT;

#undef NULL
#define NULL 0

#define LonLat_Unit_Accuracy 10000000

#define UNSOCKET_BUFFER_LENGTH      65000
/*The maximum length of UDP package is 65535, and the length of header is 20; The maximun length is 65515*/
#define MAX_UDP_SOCKET_LENGTH       65500

/*the maximum length of received message*/
#define MAX_MSG_SIZE                65535

#define PROCESS_MAX                 8


#ifndef THIS_MODULE
#error "MUST DEFINE THIS_MODULE"
#endif

#ifndef THIS_LOG_MODULE 
#define THIS_LOG_MODULE THIS_MODULE
#endif

#ifndef MIN_INT
#define MIN_INT(var1,var2) (((var1)>(var2))?(var2):(var1))
#endif

#ifndef MAX_INT
#define MAX_INT(var1,var2) (((var1)<(var2))?(var2):(var1))
#endif


/*Define root path information for the digital rail file*/
#define DR_ROOT_PATH                    "."

/*Define path information for the configuration file*/
#define MAP_BLOCK_FILE_PATH             "/config/bootup/hdmap.mapblock"
#define CONFIG_BOOTUP_PATH              "/config/bootup/"
#define CONFIG_CURRENT_PATH             "/config/current/"
#define CONFIG_REBOOT_PATH              "/config/reboot/"

/*Define path information for the script file*/
#define SCRIPT_ROOT_PATH                "/script/"
#define SCRIPT_BOOTUP_PATH              "/script/bootup/"
#define SCRIPT_BOOTUP_BEFORE_PATH       "/script/bootup/before/"
#define SCRIPT_BOOTUP_AFTER_PATH        "/script/bootup/after/"

/*Define path information for the binary file*/
#define BIN_ROOT_PATH                   "/bin/"

/*Define path information for the log file*/
#define LOG_ROOT_PATH                   "/log/"

/*Define path information for the factory file*/
#define FACTORY_ROOT_PATH               "/factory/"

/*Define path information for the ftp file*/
#define FTP_ROOT_PATH                  "/ftp/"

/*Define path information for the database file*/
#define DATABASE_ROOT_PATH             "/database/"

/*Define path information for the system infomation file*/
#define SYSINFO_ROOT_PATH              "/sysinfo/"

 
/*Define path information for the log level of each module*/
#define LOG_MODULE_FILE_PATH            CONFIG_BOOTUP_PATH"log_config.dat"
#define LOG_MODULE_TMP_FILE_PATH        CONFIG_BOOTUP_PATH"log_config.tmp"
#define LOG_MODULE_CURRENT_FILE_PATH    CONFIG_CURRENT_PATH"log_config.dat"

/*Define path information for fixing the thread on the CPU cores*/
#define THREAD_CPUCORE_CFG_PATH         CONFIG_BOOTUP_PATH"thread_cpucore_config.dat"

/*Define platform interval debug path for DEBUGGING*/
#define PLATFORM_INTERVAL_DEBUG_PATH    CONFIG_BOOTUP_PATH"platform_interval_debug"

#define LOG_FILE_MAX_SIZE               10485760 /*10M 10x1024x1024*/


typedef S32 (*MODULE_INIT)(U32 ulModuleId);
typedef void (*MODULE_ENTRY)(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength);
typedef std::map<std::string, std::vector<std::shared_ptr<std::string>>> FileInfoList;



#include "linuxport.h"

#   define pl_memcmp        memcmp
#   define pl_memcpy        pf_memcpy
#   define pl_memset        pf_memset
#   define pl_malloc        pf_malloc
#   define pl_free          pf_free

#ifdef WITH_LOG
extern U32 aulModuleLogFlag[];

#ifdef PF_LOG_WITH_FUNCTION
#   define pl_log(ulLogLevel, format, ...)                              \
        if(aulModuleLogFlag[THIS_LOG_MODULE] >= ulLogLevel)             \
        {                                                               \
            pf_log(THIS_LOG_MODULE, ulLogLevel, __FUNCTION__, __LINE__, \
                format, ## __VA_ARGS__);                                \
        }                                                               \
        else                                                            \
        {                                                               \
            PS_CPlus(CM_COM, CMCOM_ID_LOG_RETURN_CNT);                  \
        }

#   define pl_dumpData(ulLogLevel, pData, dataLengthInByte, format, ...)\
        if(aulModuleLogFlag[THIS_LOG_MODULE] >= ulLogLevel)             \
        {                                                               \
            pf_dump(THIS_LOG_MODULE, ulLogLevel, __FUNCTION__, __LINE__,\
                pData, dataLengthInByte, format, ## __VA_ARGS__);       \
        }                                                               \
        else                                                            \
        {                                                               \
            PS_CPlus(CM_COM, CMCOM_ID_DUMP_RET_CNT);                    \
        }
#else
#   define pl_log(ulLogLevel, format, ...)                              \
        if(aulModuleLogFlag[THIS_LOG_MODULE] >= ulLogLevel)             \
        {                                                               \
            pf_log(THIS_LOG_MODULE, ulLogLevel, format, ## __VA_ARGS__);\
        }                                                               \
        else                                                            \
        {                                                               \
            PS_CPlus(CM_COM, CMCOM_ID_LOG_RETURN_CNT);                  \
        }

#   define pl_dumpData(ulLogLevel, pData, dataLengthInByte, format, ...)\
        if(aulModuleLogFlag[THIS_LOG_MODULE] >= ulLogLevel)             \
        {                                                               \
            pf_dump(THIS_LOG_MODULE, ulLogLevel, pData,                 \
                dataLengthInByte, format, ## __VA_ARGS__);              \
        }                                                               \
        else                                                            \
        {                                                               \
            PS_CPlus(CM_COM, CMCOM_ID_DUMP_RET_CNT);                    \
        }
#endif

#else
#   define pl_log(...)
#   define pl_dumpData(...)
#endif

#define pl_dbg  //printf

S32 show_kernel_stack(pid_t pid);

#define ASSERT(X)  \
    do {\
        if(!(X))                            \
            show_kernel_stack(0);     \
        assert((X));                        \
    }while(0)

#define FUNCTION_TRACE    //printf("%12s,%20s,%4d\n", __FILE__, __FUNCTION__, __LINE__)

/**********************************************************************************************
 * @API function  pf_copy_msg_file
 * @brief         copy content to platform thread for writing the end of file
 * @input         fileName      The path of file, should not be NULL
                  strContentPtr The string writen to file, create file only if content is NULL.
 *     
 * @output        none
 * @return        true: ok
                  false: failure
 *********************************************************************************************/
BOOL pf_copy_msg_file(std::string &fileName, std::shared_ptr<std::string> strContentPtr);

#ifdef __cplusplus
extern "C" {
#endif
/**********************************************************************************************
 * @API function  pf_memcpy
 * @brief         copy memory from source address to destination address
 * @input         pDst            the destination address
                  pSrc            the source address
                  ulLength        the copy length
 * @output        void 
 * @return        the destination address
 *********************************************************************************************/
void* pf_memcpy(void*pDst, const void*pSrc, U32 ulLength);

/**********************************************************************************************
 * @API function  pf_memset
 * @brief         Set all content of the memory to the specified value
 * @input         pData           the destination address
                  ucVal           the value of memory
                  ulLength        the assignment length
 * @output        void   
 * @return        the destination address    
 *********************************************************************************************/
void* pf_memset(void*pData, U8 ucVal, U32 ulLength);

#ifdef MEMORY_IN_MEMPOOL
/**********************************************************************************************
* @API function  pf_mem_pool_init
* @brief         memory pool initial function
* @input         void
* @output        void
* @return        0        - success
                 other     - failure
*********************************************************************************************/
 U32 pf_mem_pool_init(void);

U32 pf_mempool_warn(void);
void set_mempool_warn(void);
void clear_mempool_warn(void);

U32 pf_meminfo(MEMPOOL_INFO *);

#endif

/**********************************************************************************************
 * @API function  pf_malloc
 * @brief         alloc dynamic memory
 * @input         ulSzie          the length of dynamic memory
 * @output        void
 * @return        the address of alloc memory
 *********************************************************************************************/
void* pf_malloc(U32 ulSzie);


/**********************************************************************************************
 * @API function  pf_realloc
 * @brief         realloc dynamic memory
 * @input         pSrcAddr        the address of realloc dynamic memory  
                  ulSize          the length of realloc dynamic memory  
 * @output        void
 * @return        the address of realloc memory
 *********************************************************************************************/
void* pf_realloc(void* pSrcAddr, U32 ulSize);

/**********************************************************************************************
 * @API function  pf_free
 * @brief         free dynamic memory
 * @input         pBuff           the free address of memory 
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_free(void* pBuff);

/**********************************************************************************************
 * @API function  pf_show_performance
 * @brief         output of current system statistics
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_show_performance(void);

/**********************************************************************************************
 * @API function  pf_copy_msg
 * @brief         Transfer general message interface function
                  Messages will not be lost
 * @input         usSrcModuleId         source module identification
 *                usMsgId               message identification
 *                usDstModuleId         destination module identification
 *                pData                 the address of message
 *                usLength              the length of message
 * @output        void
 * @return        0                     succuss
                  other                 failure
 * @date          2012/11/16
 *********************************************************************************************/
S32 pf_copy_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void *pData, U32 ulLength);
S32 pf_copy_msg2(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void *pData, U32 ulLength ,void *pData2, U32 ulLength2);

/**********************************************************************************************
 * @API function  pf_copy_try_msg
 * @brief         Try to transfer general message interface function
                  Messages may be lost
 * @input         ulSrcModuleId         source module identification
 *                ulMsgId               message identification
 *                ulDstModuleId         destination module identification
 *                pData                 the address of message
 *                ulLength              the length of message
 * @output        void
 * @return        0                     succuss
                  other                 failure
 * @date          2012/11/16
 *********************************************************************************************/
S32 pf_copy_try_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void *pData, U32 ulLength);

/**********************************************************************************************
 * @API function  pf_copy_urgent_msg
  * @brief        Transfer urgent message interface function, put the message ahead of queue
                  stMboxMessages will not be lost
  * @input        ulSrcModuleId        source module identification
  *               ulMsgId              message identification
  *               ulDstModuleId        destination module identification
  *               pData                the address of message
  *               ulLength             the length of message
  * @output       void
  * @return       0                    succuss
                  other                failure
 *********************************************************************************************/
S32 pf_copy_urgent_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength);

/**********************************************************************************************
 * @API function  pf_copy_try_urgent_msg
 * @brief         Try to transfer urgent message interface function, put the message ahead of queue
                  stMboxMessages may be lost for the full of queue
 * @input         usSrcModuleId         source module identification
 *                usMsgId               message identification
 *                usDstModuleId         destination module identification
 *                pData                 the address of message
 *                usLength              the length of message
 * @output        void
 * @return        0                     succuss
                  other                 failure
 *********************************************************************************************/
S32 pf_copy_try_urgent_msg(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, void* pData, U32 ulLength);


 /**********************************************************************************************
 * @API function  pf_set_thread_mid
 * @brief         Set the identity of the module where the thread is located
 * @input         ulMid                 the identity of the module
 * @output        void
 * @return        void 
 *********************************************************************************************/
void pf_set_thread_mid(U32 ulMid);
 
 /**********************************************************************************************
 * @API function  pf_get_thread_mid
 * @brief         get the identity of the module where the thread is located
 * @input         void
 * @output        void
 * @return        ulMid                 the identity of the module where the thread is located
 *********************************************************************************************/
U32 pf_get_thread_mid(void);

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
void pf_log(U32 ulModuleId, U32 ulLogLevel, const CHAR* pscFuncName, U32 ulLine, const CHAR* format, ...);
#else
void pf_log(U32 ulModuleId, U32 ulLogLevel, const CHAR* format, ...);
#endif

/**********************************************************************************************
 * @function      pf_log_timer_init
 * @brief         Log buffer timer initialization interface
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_log_timer_init(void);

/**********************************************************************************************
 * @API function  pf_log_module_flag_close
 
 * @brief         This interface is used to close the output printing level of the module.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_log_module_flag_close(void);

/**********************************************************************************************
 * @API function  pf_log_module_flag_close
 * @brief         This interface is used to open the output printing level of the module.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_log_module_flag_open(void);


/**********************************************************************************************
 * @API function  pf_log_important_info
 * @brief         ���ӿ�����ϵͳ�����Ҫ�Ĵ�ӡ��Ϣ�����ڼ�¼�����е�ĳһ�ؼ����裬
                  ֧�ֱ��浽�����ļ����߷��͸�OAM��ʾ�������ʽ��
 * @input         format          �������Ϣ��ַ 
 * @output        void
 * @return        void
 *********************************************************************************************/
#define pf_log_important_info() 

#define pf_log_command_line                pf_log_important_info



/**********************************************************************************************
 * @API function  pf_dump
 * @brief         Output hexadecimal memory information
 * @input         ucLogLevel        output level
                  usModuleId        source module identification
                  pData             output the address of memory 
                  dataLengthInByte  the length of output memory
                  format            the address of output memory
 * @output        void
 * @return        size              return length
 * @date          2019/06/17
 *********************************************************************************************/
#ifdef PF_LOG_WITH_FUNCTION
void pf_dump(U32 ulModuleId, U32 ulLogLevel, const CHAR* func, U32 ulLine, void* pData, U32 ulDataLengthInByte, CHAR* format, ...);
#else
void pf_dump(U32 ulModuleId, U32 ulLogLevel, void* pData, U32 ulDataLengthInByte, CHAR* format, ...);
#endif

/**********************************************************************************************
 * @API function  pf_set_module_flag
 * @brief         This interface is used to set the output printing level of the module.
 * @input         ulMid              module ID
                  ulFlag             module log level
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_set_module_flag(U32 ulMid, U32 ulFlag);

/**********************************************************************************************
 * @API function  pf_get_module_flag
 * @brief         This interface is used to get the output printing level of the module.
 * @input         ulMid              module ID
 * @output        void
 * @return        ulFlag             module log level
 *********************************************************************************************/
U32 pf_get_module_flag(U32 ulMid);

/**********************************************************************************************
 * @API function  pf_get_localtime
 * @brief         Used to get local time.
 * @input         void
 * @output        pstTmInfo         output the tm struct of local time
                  pulTime           output the number of local time
 * @return        0                 succuss
                  other             failure (if pstTmInfo and pulTime are both NULL )
 *********************************************************************************************/
S32 pf_get_localtime(struct tm* pstTmInfo, U32* pulTime);

/**********************************************************************************************
 * @function      pf_log_write_file
 * @brief         Used to write the log of memory into the file.
 * @input         void
 * @output        void
 * @return        
 *********************************************************************************************/
void pf_log_write_file(void);

/**********************************************************************************************
 * @API function  pf_get_log_path
 * @brief         get the log path
 * @input         void
 * 
 * @output        none
 * @return        the log path of the process 
 *********************************************************************************************/
CHAR* pf_get_log_path(void);

/**********************************************************************************************
 * @API function  pf_set_log_path
 * @brief         set the log path, should be called after the function of log_init 
 * @input         pcLogPath       the destination path of log file
 * 
 * @output        none
 * @return        0                     succuss
                  other                 failure
 *********************************************************************************************/
S32 pf_set_log_path(CHAR* pcLogPath);


/**********************************************************************************************
 * @API function  pf_ticks_init
 * @brief         get base time.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_ticks_init(void);

/**********************************************************************************************
 * @API function  pf_get_ticks_ms
 * @brief         get system time, the unit is milliseconds.
 * @input         void
 * @output        void
 * @return        system time
 *********************************************************************************************/
U64 pf_get_ticks_ms(void);

/**********************************************************************************************
 * @API function  pf_get_ticks_us
 * @brief         get system time, the unit is microseconds.
 * @input         void
 * @output        void
 * @return        system time
 *********************************************************************************************/
U64 pf_get_ticks_us(void);

/**********************************************************************************************
 * @API function  pf_get_ticks_ns
 * @brief         get system time, the unit is nanoseconds.
 * @input         void
 * @output        void
 * @return        system time
 *********************************************************************************************/
U64 pf_get_ticks_ns(void);

/**********************************************************************************************
 * @API function  pf_get_frame_time
 * @brief         get frame and subframe of system time
 * @input         void
 * @output        pulFrameTime      U32 Output frame time
                  pulSubFrameTime   U32 Output subframe time
 * @return        0      - success
                  other  - failure
 *********************************************************************************************/
U32 pf_get_frame_time(U32* pulFrameTime, U32* pulSubFrameTime);

#include <sys/time.h>
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
S32 pf_get_timeofday(struct timeval *tv, struct timezone *tz );

/**********************************************************************************************
 * @API function  pf_usleep
 * @brief         Delay Interface at Microsecond Level
 * @input         ulUs          delayed microseconds
 * @output        void
 * @return        0      - success
                  other  - failure
 *********************************************************************************************/
U32 pf_usleep(U32 ulUs);

/**********************************************************************************************
 * @API function  pf_get_localtime_nolocks
 * @brief         change the localtime to tm structer
 * @input         t           The localtime in second
 * @output        tmp         The structer tm of the localtime
 * @return        none
 *********************************************************************************************/
void pf_get_localtime_nolocks(struct tm *tmp, time_t t);

/**********************************************************************************************
 * @API function  pf_get_big_endian_flag
 * @brief         Get the byte order of the current system, big-endian or little-endian
 * @input         void
 * @output        void
 * @return        FALSE  - little endian
                  TRUE   - big endian
 *********************************************************************************************/
BOOL pf_get_big_endian_flag(void);

/**********************************************************************************************
 * @API function  pf_get_sc_version
 * @brief         Get version information of the current system
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_get_sc_version(void);

/**********************************************************************************************
 * @API function  pf_get_ifconfig_info
 * @brief         querying current  all network cards name
 * @input        
 * @output       ifname            network name                 
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
 S32 pf_get_ifconfig_info(const S8 *ifname);

/**********************************************************************************************
 * @API function  pf_get_inet_aton
 * @brief         whether the input IP address format is correct
 * @input         pcAddr          �����IP address   
 * @output        void
 * @return        0 - correct IP address format
                  other - wrong IP address correct
 *********************************************************************************************/
S32 pf_get_inet_aton(char *pcAddr);

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
S32 pf_get_ifconfig_addr(const S8 *ifname, U32 *pulIpaddr, U32 *pulNetmask);

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
S32 pf_set_ip_addr(S8* ifname,S8* scIpAddr);

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
 S32 pf_set_net_mask(S8* ifname,const S8 *netmask);

 
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
 S32 pf_set_ip_addr_net_mask(S8* ifname,S8* scIpAddr,S8 *netmask);



 /**********************************************************************************************
  * @API function  pf_enable_networkcard
  * @brief         used to enable network card 
  * @input         ifname            network card name 
  * @output        void
  * @return        ulRes             result
                                     0      - success
                                     other  - failure
  *********************************************************************************************/
S32 pf_enable_networkcard(S8* ifname);

 /**********************************************************************************************
  * @API function  pf_disable_networkcard
  * @brief         used to disable network card 
  * @input         ifname            network card name 
  * @output        void
  * @return        ulRes             result
                                     0      - success
                                     other  - failure
  *********************************************************************************************/
 S32 pf_disable_networkcard(S8* ifname);

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
int pf_set_thread_cpucore(int thread_id, stCpuCoreMask cpucore_mask);
int pf_get_thread_cpucore(int thread_id, stCpuCoreMask* cpucore_mask);

/**********************************************************************************************
 * @API function  pf_get_thread_cpucore_cfg
 * @brief         get thread cpucore config from the input file 
 * @input         scPath     The path of file
 * @output        gloable array  aulModuleCpucoreMask
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
int pf_get_thread_cpucore_cfg(const S8* scPath);

/**********************************************************************************************
 * @API function  pf_set_system_call
 * @brief         call system fuction directly
 * @input         scCmd  The command to call system fuction

 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
S32 pf_set_system_call(const S8* scCmd);

/**********************************************************************************************
 * @API function  pf_write_flush_file
 * @brief         write and flush the file
 * @input         scPath    The path of file
                  scStr     The string writen to file
                  ulLength  The length of string writen to file

 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
 S32 pf_write_flush_file(const S8* scPath, const S8* scStr, U32 ulLength);

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
S32 pf_write_endof_file(const S8* scPath, const S8* scStr, U32 ulLength);

/**********************************************************************************************
 * @API function  pf_read_flush_file
 * @brief         read the file
 * @input         scPath    The path of file
                  scStr     The string read from the file
                  ulLength  The length of string read from file

 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
S32 pf_read_flush_file(const S8* scPath, const S8* scStr, U32 ulLength);

/**********************************************************************************************
 * @API function  pf_get_file_length
 * @brief         get length of the file
 * @input         scPath     The path of file
 * @output        pulFileLen Length of the file
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
S32 pf_get_file_length(S8* scPath, U32* pulFileLen);

/**********************************************************************************************
 * @API function  pf_is_file_equal
 * @brief         whether the source and the destination are the same
 * @input         scDstPath     The dest path of file
 *                scSrcPath     The source path of file
 * @output        none
 * @return        0: different
                  1: same
 *********************************************************************************************/
BOOL pf_is_file_equal(S8* scDstPath, S8* scSrcPath);

/**********************************************************************************************
 * @API function  pf_is_file_exist
 * @brief         whether the file exists or not
 * @input         scSrcPath     The source path of file
 * @output        none
 * @return        0: not exist
                  1: exist
 *********************************************************************************************/
BOOL pf_is_file_exist(S8* scSrcPath);

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
S32 pf_copy_flush_file(const CHAR* scDstPath, const CHAR* scSrcPath);


/**********************************************************************************************
 * @API function  pf_set_root_path
 * @brief         set the root path
 * @input         scPath    The root path
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
S32 pf_set_root_path(const S8* scPath);

/**********************************************************************************************
 * @API function  pf_get_root_path
 * @brief         get the root path
 * @input         void
 * 
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
CHAR* pf_get_root_path(void);  

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
S32 pf_write_root_path_file(const S8* scPath, const S8* scStr, U32 ulLength);  

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
S32 pf_read_root_path_file(const S8* scPath, const S8* scStr, U32 ulLength);  

/**********************************************************************************************
 * @API function  pf_get_root_path_file_length
 * @brief         get length of the root path file
 * @input         scPath     The relative path of file
 * @output        pulFileLen Length of the file
 * @return        the length of the relative path file
 *********************************************************************************************/
S32 pf_get_root_path_file_length(const S8* scPath, U32* pulFileLen);


/**********************************************************************************************
 * @API function  pf_get_sysinfo
 * @brief         get system info 
 * @input         ulLength  The length of max string 
 * 
 * @output        scInfo    The string of system info
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
S32 pf_get_sysinfo(S8* scInfo, U32 ulLength);

/**********************************************************************************************
 * @API function  pf_set_warm_reset
 * @brief         the interface of system reboot
 * @input         ulFlag        the flag of reset
                                0 - reset the process
                                1 - reboot the system
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_set_warm_reset(U32 ulFlag);

/**********************************************************************************************
 * @API function  pf_sig_handler
 * @brief         Used to handle signal and output backtrace 
 * @input         none
 * @output        none
 * @return        none                  
 *********************************************************************************************/
void pf_sig_handler(int sig);

/**********************************************************************************************
 * @API function  pf_check_auth_state
 * @brief         Check the authorization status, waiting for authorization until successfully
 * @input         none
 * @output        none
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
S32 pf_check_auth_state(void);

void pf_interChip_msg_init(U32 ulPid, const char* pcPath);
void pf_interChip_msg_connect(U32 ulPid, const char* pcPath);



/**********************************************************************************************
 * @API function  pf_set_sys_init
 * @brief         initialization interface of platform called after start-up
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_set_sys_init(void);

/**********************************************************************************************
 * @API function  pf_get_diskinfo_size
 * @brief         get disk totalsize and free size 
 * @input         scPath     disk path
				  pulTotal   disk total size
				  pulFree	 disk free size
				  unit		unit for disk 	eg:DISK_INFO_UNIT_KB/DISK_INFO_UNIT_MB/DISK_INFO_UNIT_GB
 * @output        pulTotal   disk total size
				  pulFree	 disk free size
 * @return        0: ok
                  other: failure
 *********************************************************************************************/
S32 pf_get_diskinfo_size(const S8* scPath,U64 *pulTotal,U64 *pulFree,U32 unit);

/**********************************************************************************************
 * @API function  pf_get_service_name
 * @brief         get drc service name
 * @input         void
 * @output        string
 * @return        
 *********************************************************************************************/
std::string pf_get_service_name(void);

#define MEMORY_SIZES_LIMITED    52428800    //50M

/*the number of memory pool*/
#define MAX_MEMPOOL_NUMBER      6

/*the maximum bit offset of memory pool*/
#define MAX_MEMPOOL_BIT_OFFSET  8

U32 pf_sysmem_warn(void);




#ifdef __cplusplus
}
#endif

#if DRSU_PROCESS
# define MESSAGE_QUEUE_SIZE 128    
# define SHORT_PACK_SIZE    40932  /*40960-28*/
#else
# define MESSAGE_QUEUE_SIZE 16384    
# define SHORT_PACK_SIZE    100
#endif

/**********************************************************************************************
 * @struct  Message
 *
 * @brief   Structural definition of message delivery between threads
 *
 * @date    2012/11/16
 **********************************************************************************************/
struct stMboxMessage
{
    U32 ulSrcModuleId;
    U32 ulDstModuleId;
    U32 ulLength;       /*the actual length of the pointer pucData*/
    U32 ulMsgId;
    U8* pucData;        /*can pass a pointer or an integer. Specifically, it is negotiated between the sending and receiving sides of the message. But when passing a non-pointer, length must be zero*/
    U32 shortpayload[SHORT_PACK_SIZE>>2];
    U32 ulTick;
#ifdef __cplusplus
    void restore()
    {
        if(ulLength && (ulLength <=SHORT_PACK_SIZE))
        {
            pucData = (unsigned char*)shortpayload;
        }
    }

    void free(void)
    {
        if((ulLength > SHORT_PACK_SIZE) && pucData)
        {
            PS_CPlus(CM_COM, CMCOM_ID_COPY_MSG_FREE_CNT);
            pf_free(pucData);
        }
    }

    stMboxMessage()
    {
        ulSrcModuleId = 0;
        ulDstModuleId = 0;
        ulMsgId = 0;
        pucData = 0;
        ulLength = 0;
        shortpayload[SHORT_PACK_SIZE>>2] = {0};
        ulTick = 0;
    }    
#endif
};


/**********************************************************************************************
 * @struct  stMsgHeader
 *
 * @brief   Structural definition of message delivery between subsystems.
 *
 * @date    2019/06/17
 **********************************************************************************************/
typedef struct stMsgHeader
{
    U32 ulSendSubSysId;         /*the subsystem identify of sender*/
    U32 ulRcvdSubSysId;         /*the subsystem identify of receiver*/
    U32 ulMsgLength;            /*the length of message, not include the length of stMsgHeader*/
    U32 ulMsgType;              /*the type of message*/
}stMsgHeader;





struct PacketHead
{
    stMsgHeader msgHead;
    U32     packetID;        // �ܰ����к�
    U32     totalPacketNum;  // �ְ��ܸ��� 
    U32     packetIndex;     // ��ǰ�ְ����
    U32     packetOffset;    // ��ǰ������ƫ�Ƶ�ַ
    U32     packetLen;       // ��ǰ���ݰ����ݳ���
    U32     padding;
};




#define PACKET_ENABLE   (0x80000000)

#define MEMBER_SIZE_BYTE      sizeof(U32)
#define SEND_SYSID           (0)
#define RECV_SYSID           (4)
#define MSG_LENGTH           (8)
#define MSG_TYPE             (12)
#define PACKET_ID            (16)
#define SUBPACKET_TOTALNUM   (20)
#define SUBPACKET_INDEX      (24)
#define SUBPACKET_OFFSET     (28)
#define SUBPACKET_LEN        (32)






typedef enum
{
    PACKET_NULL = -2,       // �û�û������bufָ�룬�޴����
    PACKET_RECEIVING = -1,  // ���������
    PACKET_SUCCESS = 0,     // �����ɣ����ݾ������ָ���λ��
    PACKET_ONLY_ONE = 1,    // �û�û������bufָ�룬��ǰ��Ϊ������������Ҫ���

}PF_PACKET_RESULT;







#define TIMER_INVALID_SOCKET_DELETE_MS   5000
#define TIMER_INVALID_SOCKET_DELETE_REQ  1234


/* pl_set_nacos_log_info:
 * Description: set nacos key parameters: namespace, group, and IP of nacos-cluster(or standalone server)
 * Input:
 *      log_namespace: nacos namespace, e.g: "production_env"
 *      log_group: nacos group name, e.g: "DRSU"
 *      log_ip: nacos IP, e.g: "172.16.8.4"
 * Output:
 *       PF_RET_SUCCESS: ok
 */
S32 pl_set_nacos_log_info(std::string &log_namespace, std::string &log_group, std::string &log_ip);

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
S32 pl_set_nacos_log_dataID(std::string &log_dataID);

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
S32 pl_get_nacos_log_info(std::string &info);

#define PF_NACOS_INFO_HAS_NOT_SET -100

#endif
