/*
pl_stat.h: "S32" define in <pl.h>
*/

#ifndef PF_STAT_H
#define PF_STAT_H

#define CPU_CORE_NUM_MAX  128



typedef struct cpu_info_st {
    U16  usr, nic, sys, idle;    //  n/1000
    U16  iowait, irq, softirq, steal;
} cpu_info_st;


typedef struct cpu_load_st {
    U16  cpu_load;    //  n/1000
    U16  cpu_core_num;
    S16  cpu_coretemp;
    U16  rsvu16;
    cpu_info_st cpu_info[CPU_CORE_NUM_MAX];
} cpu_load_st;



typedef struct thread_info_st {
    U32  thread_id;
    U16  thread_cpu_load;
    U16  thread_mem_load;
    char thread_name[32];   
} thread_info_st;

typedef struct thread_load_st {
    U16  thread_num;
    U16  u16rsv;    //  n/1000
    U32  u32rsv;
    thread_info_st thread_info[MODULE_TASK_MAX];
} thread_load_st;


typedef struct stru_mem_stat {
    unsigned long mem_total;
    unsigned long mem_used;
    unsigned long mem_free;
} stru_mem_stat;





typedef struct shared_mem_st {

	cpu_load_st    cpu_load;
	thread_load_st thread_load;
	stru_mem_stat  mem_state;
//#ifdef MEMORY_STATICS 
	U32 au32PSCnts[CM_MAX][CI_MAX]; 
//#endif
} shared_mem_st;






U32 pf_set_system_starttime(void);

U32 pf_get_system_runtime_sec(void);



void pf_sharedMemory_init(void);

void pf_stat_thread_name_init(char* name, U32 threadId);
void pf_stat_set_thread_name(char* name);

int pf_stat_update_thread_info(thread_load_st** p_thread_load);

int pf_stat_update_cpu_info(cpu_load_st** p_cpu_load);

/**********************************************************************************************
 * @API function  pf_stat_get_cpu_coretemp
 * @brief		  get current cpu temperature
 * @input		  void
 * @output		  the temperature of cpu (unit:centigrade)
 * @return        0                 succuss
                  other             failure 
 依赖工具安装步骤说明：
 sudo apt-get install lm-sensors
 sudo sensors-detect 
 service module-init-tools start
 如果想省略第2，3步可以直接使用命令：
 sudo modprobe coretemp
 到此为止，安装结束；
 *********************************************************************************************/
int pf_stat_get_cpu_coretemp(S16* pcoretemp);

thread_load_st* pf_stat_get_thread_info(void);
cpu_load_st* pf_stat_get_cpu_info(void);

/*
@ pf_stat_get_cpu_load:
Input:
Output:  p_pointer, which ready to get cpuload ( 0 ~ 100):
          cpuload:  Total CPU load
          sysload:   cpu load costs by system (driver, deamon, syscall, etc)
          
Return:  0: success ; 
    -EBUSY : Call the function too frequently.
    -EBADF : Can't open /proc/stat, it's just so bad.
*/
S32 pf_stat_get_cpu_load(S32 *p_cpuload, S32 *p_sysload);

/*
@ pf_stat_get_mem_info
Input:
Output: pmem_stat: struct pointer read to get memory state (memory counts in Byte)
Return: 0: success
    -EFAULT : Can system function fails
*/

S32 pf_stat_get_mem_info(struct stru_mem_stat* pmem_stat);

/*
@ pf_stat_get_thread_cpu
Input: mid: module id (e.g MAC, MC ...)
Output: pmem_stat: struct pointer read to get memory state (memory counts in Byte)
Return: 0: success
    -EFAULT : May system function fails
    -EINVAL : Wrong parameter input
    -ERANGE : Calculte result is wrong ( out of  0 ~ 100)
    Other : Wrong ret return by pthread_getcpuclockid.
*/

S32 pf_stat_get_thread_cpu(U16 mid, S32 *p_tcpu_load);

#ifdef __cplusplus
extern "C" {
#endif
/*
@ pf_stat_get_prccess_cpu
Input: NONE
Output: pmem_stat: struct pointer read to get cpu load
Return: 0: success
    -EFAULT : May system function fails
    -EINVAL : Wrong parameter input
    -ERANGE : Calculte result is wrong ( out of  0 ~ 1000: 0.0% ~ 100.0%)
    Other : Wrong results returned by pthread_getcpuclockid.
*/
S32 pf_stat_get_process_cpu(S32 *p_pcpu_load);

/**********************************************************************************************
 * @API function  pf_stat_get_process_cycle_cpuload
 * @brief         获取小基站固定周期内的CPULOAD的接口
 * @input         无
 * @output        无
 * @return        slRes             cpuload的结果
 *********************************************************************************************/
S32 pf_stat_get_process_cycle_cpuload(void);
#ifdef __cplusplus
}
#endif

#endif

