/*
The file provides stat service to others:
1. CPU load
2. Memory information

*/
#define THIS_MODULE PLATFORM_EX
#define PF_STAT_CPP

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pl.h>
#include <osport.h>

#include <sys/shm.h>
#include <pl_counts.h>
#include <pf_stat.h>
#include <pf_upgrade.h>


# define LINE_BUF_SIZE 128

static char proc_entry[32];
static U32 ticks_per_second;
static pid_t process_id;
typedef struct jiffy_counts_t {
    unsigned long long usr, nic, sys, idle;
    unsigned long long iowait, irq, softirq, steal;
    unsigned long long total;
    unsigned long long busy;
} jiffy_counts_t;

static jiffy_counts_t j_now, j_prev;

static unsigned long process_total_prev;
static U64 wall_clock_time_prev;

static S32 slCurrentCpu = 0;

jiffy_counts_t pre_cpu_info[CPU_CORE_NUM_MAX] = {0};


cpu_load_st gst_cpu_load;
thread_load_st gst_thread_load;


static U32 gu32devstarttime = 0;   

U32 pf_set_system_starttime(void)
{
    struct timeval tv;
    if(pf_get_timeofday(&tv, NULL))
    {
        return 0;
    }
    gu32devstarttime = tv.tv_sec;
    
    printf("\r\n system start at %d  \r\n",gu32devstarttime);

    return  gu32devstarttime;
}


U32 pf_get_system_runtime_sec(void)
{
    struct timeval tv;
    if(pf_get_timeofday(&tv, NULL))
    {
        return 0;
    }

    return (tv.tv_sec - gu32devstarttime);
}


void pf_stat_thread_name_init(char* name, U32 threadId)
{
    CHAR acPath[FILE_PATH_LEN];
    snprintf((CHAR*)acPath, FILE_PATH_LEN, "%s/ps.pid", pf_get_root_path());

    pf_set_config_integer((const S8 *)acPath, threadId);

    strcpy(gst_thread_load.thread_info[gst_thread_load.thread_num].thread_name,name);
    gst_thread_load.thread_info[gst_thread_load.thread_num].thread_id = threadId;

    gst_thread_load.thread_num++;  
}


void pf_stat_set_thread_name(char* name)
{
    strcpy(gst_thread_load.thread_info[gst_thread_load.thread_num].thread_name,name);

    gst_thread_load.thread_num++;  
}

 
int pf_stat_update_thread_info(thread_load_st** p_thread_load)
{
    FILE *fp;
    int ret = 0;
    int i = 0;
    int id = 0;
    float cpuload = 0;
    float memload = 0;
    char line_buf[LINE_BUF_SIZE];

    char cmd[100] = {0};
    sprintf(cmd,"ps -eLo pid,lwp,%%cpu,%%mem |grep %d > thread_info.txt",gst_thread_load.thread_info[0].thread_id);
    system(cmd);

    fp = fopen("thread_info.txt", "r");
    if (fp == NULL)
    {
        return -1;
    }

    while(1)
    {
        if (!fgets(line_buf, LINE_BUF_SIZE, fp)) 
        {
            break;
        }

        cpuload = 0;
        memload = 0;
        id = 0;
        ret = sscanf(line_buf, "%llu %llu %f %f ",
                &id,&id, &cpuload , &memload);

        gst_thread_load.thread_info[i].thread_id = id;
        gst_thread_load.thread_info[i].thread_cpu_load = cpuload*10;
        gst_thread_load.thread_info[i].thread_mem_load = memload*10;

        i++;
    }
    
    fclose(fp);
    
    if(p_thread_load != NULL)
    {
        *p_thread_load = &gst_thread_load;
    }
    
    return 0;
}



thread_load_st* pf_stat_get_thread_info(void)
{
    return &gst_thread_load;
}


static int stat_snap_shot(jiffy_counts_t *p_jif)
{
    FILE *fp;
    int ret = 0;
    static const char fmt[] = "cpu %llu %llu %llu %llu %llu %llu %llu %llu";
    char line_buf[LINE_BUF_SIZE];

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        return -1;
    }

    if (!fgets(line_buf, LINE_BUF_SIZE, fp) || line_buf[0] != 'c' /* not "cpu" */)
    {
        fclose(fp);
        return -1;
    }

    ret = sscanf(line_buf, fmt,
            &p_jif->usr, &p_jif->nic, &p_jif->sys, &p_jif->idle,
            &p_jif->iowait, &p_jif->irq, &p_jif->softirq,
            &p_jif->steal);

    if (ret >= 4) {
        p_jif->total = p_jif->usr + p_jif->nic + p_jif->sys + p_jif->idle
            + p_jif->iowait + p_jif->irq + p_jif->softirq + p_jif->steal;
        /* procps 2.x does not count iowait as busy time */
        p_jif->busy = p_jif->total - p_jif->idle - p_jif->iowait;
    }

    if (fp)
    {
        fclose(fp); 
    }

    return 0;
}


/*
安装步骤：
sudo apt-get install lm-sensors
sudo sensors-detect 
service module-init-tools start
如果想省略第2，3步可以直接使用命令：
sudo modprobe coretemp
到此为止，安装结束；
*/

int pf_stat_get_cpu_coretemp(S16* pcoretemp)
{
    FILE *fp;
    int ret = -1;
    S16 s16ret = -1;
    float cpunum = 0;
    float temp = 0;
    char line_buf[LINE_BUF_SIZE];

    char cmd[LINE_BUF_SIZE] = {0};
    snprintf(cmd, LINE_BUF_SIZE, "sensors |grep Core > cpu_coretemp.txt");
    pf_set_system_call((const S8 *)cmd);

    fp = fopen("cpu_coretemp.txt", "r");
    if (fp == NULL)
    {
        return -1;
    }
    
    while(1)
    {
        char tmpstr[3][128] = {0};
    
        if (!fgets(line_buf, LINE_BUF_SIZE, fp)) 
        {
            break;
        }
//printf("\r\n\r\n%s",line_buf);

        cpunum = 0;
        temp = 0;

        s16ret = sscanf(line_buf, "%s %s %s", tmpstr[0] , tmpstr[1], tmpstr[2]);
        if(s16ret > 1)
        {
            if(0 == strcmp(tmpstr[0],"Core"))
            {
                S16 tmps16 = 0;
                s16ret--;
                if((U8(tmpstr[s16ret][strlen(tmpstr[s16ret])-1]) == 0x43)
                    &&(U8(tmpstr[s16ret][strlen(tmpstr[s16ret])-2]) == 0xb0)
                    &&(U8(tmpstr[s16ret][strlen(tmpstr[s16ret])-3]) == 0xc2))
                {
                    if(tmpstr[s16ret][0] == '+')
                    {
                        tmps16 = 1;
                    }
                    else if(tmpstr[s16ret][0] == '-')
                    {
                        tmps16 = -1;
                    }
                    
                    if(1 == sscanf(&(tmpstr[s16ret][1]), "%f", &temp ))
                    {
                        *pcoretemp = (S16)(temp*tmps16);
                        pl_log(INF, "cpu_coretemp %d !\n", *pcoretemp);
                        ret = 0;
                        break;
                    }
                }
            }
        }
    }
    
    if (fp)
    {
        fclose(fp);
    }

    if(ret != 0)
    {
        pl_log(ERR, "%s @ line%d   failed !\n", __FUNCTION__, __LINE__);
    }
    
    return ret;
}


int pf_stat_update_cpu_info(cpu_load_st** p_cpu_load)
{
    FILE *fp;
    int ret = 0;
    int i = 0;
    static const char fmt[] = "%s %llu %llu %llu %llu %llu %llu %llu %llu";
    char line_buf[LINE_BUF_SIZE];
    char name[16]={0};
    
    jiffy_counts_t cur_jif = {0};

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
	{
        return -1;
	}
	
    while(1)
    {
        if (!fgets(line_buf, LINE_BUF_SIZE, fp) || line_buf[0] != 'c' /* not "cpu" */)
        {
            break;
        }
//printf("\r\n\r\n%s",line_buf);
        memset(&cur_jif,0,sizeof(jiffy_counts_t));
        
        ret = sscanf(line_buf, fmt, name,
                &cur_jif.usr, &cur_jif.nic, &cur_jif.sys, &cur_jif.idle,
                &cur_jif.iowait, &cur_jif.irq, &cur_jif.softirq,
                &cur_jif.steal);

        cur_jif.total = cur_jif.usr + cur_jif.nic + cur_jif.sys + cur_jif.idle
            + cur_jif.iowait + cur_jif.irq + cur_jif.softirq + cur_jif.steal;
        /* procps 2.x does not count iowait as busy time */
        cur_jif.busy = cur_jif.total - cur_jif.idle - cur_jif.iowait;


//printf("\r\n%d: %s %ld %ld %ld %ld %ld %ld %ld %ld  busy_%ld  total_%ld ",i,name,cur_jif.usr, cur_jif.nic, cur_jif.sys, cur_jif.idle,
//                cur_jif.iowait, cur_jif.irq, cur_jif.softirq,cur_jif.steal,cur_jif.busy,cur_jif.total);


        if(cur_jif.total > pre_cpu_info[i].total)
        {
            unsigned long long tmptotal = (cur_jif.total - pre_cpu_info[i].total);
            gst_cpu_load.cpu_info[i].idle    = (cur_jif.idle - pre_cpu_info[i].idle) * 1000 / tmptotal;
            gst_cpu_load.cpu_info[i].iowait  = (cur_jif.iowait - pre_cpu_info[i].iowait) * 1000 / tmptotal;
            gst_cpu_load.cpu_info[i].irq     = (cur_jif.irq - pre_cpu_info[i].irq) * 1000 / tmptotal;
            gst_cpu_load.cpu_info[i].nic     = (cur_jif.nic - pre_cpu_info[i].nic) * 1000 / tmptotal;
            gst_cpu_load.cpu_info[i].softirq = (cur_jif.softirq - pre_cpu_info[i].softirq) * 1000 / tmptotal;
            gst_cpu_load.cpu_info[i].steal   = (cur_jif.steal - pre_cpu_info[i].steal) * 1000 / tmptotal;
            gst_cpu_load.cpu_info[i].sys     = (cur_jif.sys - pre_cpu_info[i].sys) * 1000 / tmptotal;
            gst_cpu_load.cpu_info[i].usr     = (cur_jif.usr - pre_cpu_info[i].usr) * 1000 / tmptotal;

            if(i == 0)
            {
                gst_cpu_load.cpu_load = (cur_jif.busy - pre_cpu_info[i].busy) * 1000 / tmptotal;
            }
        }

        pre_cpu_info[i] = cur_jif;

        i++;

    }

    gst_cpu_load.cpu_core_num = i;
    
    if (fp)
    {
        fclose(fp); 
    }

    if(p_cpu_load != NULL)
    {
        *p_cpu_load = &gst_cpu_load;
    }

    return 0;
}


cpu_load_st* pf_stat_get_cpu_info(void)
{
    return &gst_cpu_load;
}


void pf_sharedMemory_init(void)
{
    pf_set_system_starttime();
}


/*
@ pf_stat_get_cpu_load:
Input:
Output:  p_pointer, which ready to get cpuload ( 0 ~ 100):
          cpuload:  Total CPU load
          sysload:   cpu load costs by system (driver, deamon, syscall etc)
          
Return:  0: success ; 
    -EBUSY : Call the function too frequently.
    -EBADF : Can't open /proc/stat, it's just so bad.
*/
S32 pf_stat_get_cpu_load(S32 *p_cpuload, S32 *p_sysload)
{
    S32 ret = 0;

    if (0 == stat_snap_shot(&j_now))
    {
        if (j_now.total - j_prev.total)
        {
            *p_cpuload = (j_now.busy - j_prev.busy) * 100 / (j_now.total - j_prev.total);
            *p_sysload = (j_now.sys - j_prev.sys) * 100 / (j_now.total - j_prev.total);
            j_prev = j_now;
        }
        else
        {
            ret = -EBUSY;
        }
    }
    else
    {
        ret = -EBADF;
    }
    
    return ret; /* Success */
}

/*
@ pf_stat_get_mem_info
Input:
Output: pmem_stat: struct pointer read to get memory state (memory counts in Byte)
Return: 0: success
    -EFAULT : May system function fails
*/
S32 pf_stat_get_mem_info(struct stru_mem_stat* pmem_stat)
{
    struct sysinfo info;

    if (0 != sysinfo(&info))
    {
        return -EFAULT;
    }
    
    pmem_stat->mem_total = info.totalram;
    pmem_stat->mem_used  = info.totalram - info.freeram;
    pmem_stat->mem_free  = info.freeram;

    return 0;   
}

/* Struct and functions for calculating "real working time" of each thread */

typedef struct stru_thread_cputime_info {
    U64 time_tick_prev;  /* last time the function was called, unit is ns*/
    U64 time_work_prev;  /* thread working time since thread starts, unit is ns*/
} stru_thread_cputime_info;

static stru_thread_cputime_info thread_worktime_info[MODULE_TASK_MAX];
extern pf_handle_t workerhandles[];
//static stru_thread_cputime_info process_worktime_info; /*Whole process*/

/*
@ pf_stat_get_thread_cpu
Input: mid: module id (e.g MAC, MC ...)
Output: p_tcpu_load: struct pointer read to get CPU load
Return: 0: success
    -EFAULT : May system function fails
    -EINVAL : Wrong parameter input
    -ERANGE : Calculte result is wrong ( out of  0 ~ 100)
    Other : Wrong results returned by pthread_getcpuclockid.
*/

S32 pf_stat_get_thread_cpu(U16 mid, S32 *p_tcpu_load)
{
    clockid_t cid;
    U64 time_system;
    U64 time_thread;
    U64 u64ticks;
    timespec tp;

    int ret = 0;

    if (p_tcpu_load)
    {
        *p_tcpu_load = 0;
    }
    else
    {
        pl_log(INF, "Error! %s:%d input is NULL\n", __FUNCTION__, mid);
        return -EINVAL;
    }

    if(mid > MODULE_TASK_MAX || (NULL == workerhandles[mid])) 
    {
        pl_log(INF, "Error! %s:%d input is wrong\n", __FUNCTION__, mid);
        return -EINVAL;
    }

    ret = clock_gettime(CLOCK_MONOTONIC, &tp);

    if (ret != 0)
    {
        pl_log(INF, "%s @ line%d Clock_gettime failed !\n", __FUNCTION__, __LINE__);
        return ret;
    }
    /* Get system delta tick:  time_of_now - time_of prev */
    time_system = (U64)(tp.tv_sec) * 1000000000 + (U64)tp.tv_nsec; /* system ticks*/
    u64ticks    = time_system;
    time_system -= thread_worktime_info[mid].time_tick_prev; /* delta time*/
    thread_worktime_info[mid].time_tick_prev = u64ticks; /* tick-now save to prev*/

    ret = pthread_getcpuclockid((pthread_t)workerhandles[mid], &cid);

    if( ret != 0 )
    {
        pl_log(INF, "Error! %s:%d pthread_getcpuclockid failed\n", __FILE__, mid);
        return ret;
    }
    else
    {
        ret = clock_gettime(cid, &tp);
        if (ret != 0)
        {
            pl_log(INF, "%s @ line%d Clock_gettime failed !\n", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    /* Get thread delta tick: this_tick - prev_tick*/
    time_thread = (U64)(tp.tv_sec) * 1000000000 + (U64)tp.tv_nsec; /* thread ticks */
    u64ticks    = time_thread;
    time_thread -= thread_worktime_info[mid].time_work_prev; /* get delta */
    thread_worktime_info[mid].time_work_prev = u64ticks; /* tick-now save to prev*/

    /* Get per-thread cpu load (0 ~ 100)*/
    if (time_system == 0 || time_thread > time_system )
    {
        return -ERANGE;
    }

    *p_tcpu_load = (S32)(time_thread * 100 / time_system);
    
    return 0;
}

/*
@ pf_stat_get_prccess_cpu_init
Input: None
Output: None
Return: None
MUST be called before calling pf_stat_get_process_cpu()
*/
static void pf_stat_get_process_cpu_init(void)
{
    process_id = getpid();
    sprintf(proc_entry, "/proc/%u/stat", process_id );
    ticks_per_second = sysconf(_SC_CLK_TCK); 
    printf("Get PID is %d, ticks is %lu\n", process_id, ticks_per_second);
}

static int process_stat_snap_shot(U32 *p_total)
{
    char buf[256];
    //int pos_count = 0;
    ssize_t ret = -1;
    U32 total_time, utime, ustime, cutime, cstime;
    int fd = open(proc_entry,O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    ret = read(fd, buf, sizeof(buf));
    buf[ret > 0 ? ret : 0] = 0;

    close(fd);

    sscanf(buf, "%*s %*s "
                "%*s %*s "               /* state, ppid */
                "%*s %*s %*s %*s "        /* pgid, sid, tty, tpgid */
                "%*s %*s %*s %*s %*s " /* flags, min_flt, cmin_flt, maj_flt, cmaj_flt */
                "%lu %lu "             /* utime, stime */
                "%lu %lu %*s "         /* cutime, cstime, priority */
                ,&utime, &ustime, &cutime, &cstime);
    total_time = utime + ustime + cutime + cstime;
    *p_total = total_time;

    return 0;
}


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
extern "C" S32 pf_stat_get_process_cpu(S32 *p_pcpu_load)
{
    U32 p_time_now,p_time_delta;
    U64 u64_ptime_delta_ns;
    U64 u64_wallclock_now;
    U64 u64_wallclock_delta;  
    //U32 u32_wallclock_ticks;
    int ret;
    timespec tp;
    static int trigger = 0;

    if (trigger == 0)
    {
        pf_stat_get_process_cpu_init();
        trigger = 1;
    }

    ret = clock_gettime(CLOCK_MONOTONIC, &tp);

    if (ret != 0)
    {
     pl_log(INF, "%s @ line%d Clock_gettime failed !\n", __FUNCTION__, __LINE__);
     return ret;
    }

    /* Get system delta tick:  time_of_now - time_of prev */
    u64_wallclock_now = (U64)(tp.tv_sec) * 1000000000 + (U64)tp.tv_nsec; /* system ticks*/
    
    u64_wallclock_delta = u64_wallclock_now - wall_clock_time_prev; /* delta time*/
    wall_clock_time_prev = u64_wallclock_now;

    process_stat_snap_shot(&p_time_now);

    p_time_delta = p_time_now - process_total_prev;
    process_total_prev = p_time_now;

    u64_ptime_delta_ns = (U64)p_time_delta * 1000000000LLU / (U64)ticks_per_second;

    *p_pcpu_load = (S32)(u64_ptime_delta_ns  * 1000 / u64_wallclock_delta);

    /* As a work-around, SMP will output CPU-load bigger than 100. 1-core still keeps in range of 100 */
    //if (!pf_get_kernel_smp_flag())
    {
        if (*p_pcpu_load > 1000) /*MR 10447: cpu load (1-core) should not bigger than 100% */
        {
            *p_pcpu_load = 1000;
        }
    }

    slCurrentCpu = *p_pcpu_load;
    
    return 0;
}

/**********************************************************************************************
 * @API function  pf_stat_get_process_cycle_cpuload
 * @brief         get current process cycle cpuload
 * @input         void
 * @output        void
 * @return        slRes             cpuload result
 *********************************************************************************************/
extern "C" S32 pf_stat_get_process_cycle_cpuload(void)
{
    return slCurrentCpu;
}

#undef PF_STAT_CPP



