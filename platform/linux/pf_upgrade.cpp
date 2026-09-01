/*******************************************************************************************************************
 **                                                                                                                        
 **  Copyright (c)  2009,  Innofidei, Inc.                                                                                 
 **        All    Rights Reserved.                                                                                            
 **                                                                                                                          
 **  Subsystem     : LTE/SMALLCELL                                                                                             
 **  File          : pf_upgrade.cpp                                                                                       
 **  Created By    : josephzhou                                                                                              
 **  Created On    : 2013/10/31                                                                                                 
 **                                                                                                                         
 **  Purpose:                                                                                                             
 **    This file contains the platform api and main entry
 **                                                                                                                         
 **  History:                                                                                                             
 **  Programmer        Date    Rev    Description                                                                                 
 **  --------------- ---------- --------    ------------------------------                                                   
 **
 ******************************************************************************************************************/
#define THIS_MODULE PLATFORM_EX

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netinet/if_ether.h> 
#include <net/if_arp.h> 
#include <poll.h> 
#include <net/if.h>  

#include <pl.h>
#include "pf_thread_mon.h"
#include "pf_upgrade.h"
#include "event.h"


#if 0
#ifdef pl_log
#undef pl_log
#define pl_log(ucLogLevel, format, ...)     {printf(format, ## __VA_ARGS__); printf("\n");pf_usleep(10);}
#endif
#endif


//进程启动时间文件路径
#define TIME_DR_BOOTUP_FILE              "/log/time_dr_bootup.txt"
//进程重新启动时间文件路径
#define TIME_DR_REBOOT_FILE              "/log/time_dr_reboot.txt"
//startup.sh脚本启动进程时间文件路径
#define TIME_STARTUP_BOOTUP_FILE         "/log/time_startup_bootup.txt"
//monitor_dr.sh脚本启动进程时间文件路径
#define TIME_MONITOR_BOOTUP_FILE         "/log/time_monitor_bootup.txt"
//monitor_dr.sh脚本启动进程时间文件路径
#define TIME_MONITOR_CHECK_FILE          "/log/time_monitor_update.txt"
//monitor_dr.sh脚本监控间隔时间文件路径
#define TIME_MONITOR_INTERVAL_FILE       "/config/bootup/monitor_dr_time_interval"


//monitor_dr.sh脚本默认监控进程时间间隔
#define DEFAULT_MONITOR_TIME_INTERVAL    20 

//startup.sh script extarct the bin file of the *.gz
#define DEFAULT_STARTUP_TIME_EXTRACT     5


//arp包的格式,其中的数据格式都是宏定义值
enum  
{  
    ARP_MSG_SIZE = 0x2a  
};  


struct arpMsg  
{  
    /* Ethernet header */  
    uint8_t h_dest[6];      /* 00 destination ether addr */  
    uint8_t h_source[6];    /* 06 source ether addr */  
    uint16_t h_proto;       /* 0c packet type ID field */  
  
    /* ARP packet */  
    uint16_t htype;         /* 0e hardware type (must be ARPHRD_ETHER) */  
    uint16_t ptype;         /* 10 protocol type (must be ETH_P_IP) */  
    uint8_t hlen;           /* 12 hardware address length (must be 6) */  
    uint8_t plen;           /* 13 protocol address length (must be 4) */  
    uint16_t operation;     /* 14 ARP opcode */  
    uint8_t sHaddr[6];      /* 16 sender's hardware address */  
    uint8_t sInaddr[4];     /* 1c sender's IP address */  
    uint8_t tHaddr[6];      /* 20 target's hardware address */  
    uint8_t tInaddr[4];     /* 26 target's IP address */  
    uint8_t pad[18];        /* 2a pad for min. ethernet payload (60 bytes) */  
} PACKED;  

U32 m_ulBootupFlag = PF_BOOTUP_FLAG_UNKNOWN;

/**********************************************************************************************
 * @API function  safe_strncpy
 * @brief         字符串拷贝的接口
 * @input         src               源字符串地址
                  size              目的字符串最大长度
 * @output        dst               目的字符串地址
 * @return        ulRes             字符串拷贝的结果
                                    0 - success
                                    other  - failure
 *********************************************************************************************/
char * safe_strncpy(char * dst, const char * src, size_t size)  
{  
    if (!size)  
        return dst;  
    dst[--size] = '/0';  
    return strncpy(dst, src, size);  
}  

/**********************************************************************************************
 * @API function  arpping
 * @brief         使用ARP检测IP address是否被占用的接口
 * @input         test_ip           待检测IP address
                  from_ip           当前IP address
                  from_mac          当前MAC地址
                  interface         网口名称
 * @output        void
 * @return        ulRes             IP address是否被占用的结果
                                    FALSE - IP address被占用
                                    TRUE  - 说明此ip可用  
 *********************************************************************************************/
BOOL arpping(U32 test_ip, U32 from_ip, U8 *from_mac, const char *interface)  
{  
    struct pollfd pfd[1];   /* 使用poll来检测句柄 */
    int rv = 1;             /* "no reply received" yet */  
    struct sockaddr addr;   /* for interface name */  
    struct arpMsg arp;  
    int const_int_1 = 1;  
    int r;  
    sigset_t origmask;
    struct timespec timeout_ts;
  
    //建立scoket.由于我们是要直接访问访问链路层并自己组arp包.因此我们使用PF_PACKET协议簇.socket类型为SOCK_PACKET.  
    pfd[0].fd = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));  
    if (pfd[0].fd == -1) 
    {  
        pl_log(ERR, "bb_msg_can_not_create_raw_socket");  
        return FALSE;  
    }  

    if (-1 == setsockopt(pfd[0].fd, SOL_SOCKET, SO_BROADCAST, &const_int_1, sizeof(const_int_1))) 
    {  
        pl_log(ERR, "cannot enable bcast on raw socket");  
        goto ret;  
    }  

    //进行组包，由于是要广播，因此目的mac地址为全0.  
    /* send arp request */  
    pf_memset(&arp, 0, sizeof(arp));  
    pf_memset(arp.h_dest, 0xff, 6);                     /* MAC DA */  
    pf_memcpy(arp.h_source, from_mac, 6);               /* MAC SA */  
    arp.h_proto = htons(ETH_P_ARP);                     /* protocol type (Ethernet) */  
    arp.htype = htons(ARPHRD_ETHER);                    /* hardware type */  
    arp.ptype = htons(ETH_P_IP);                        /* protocol type (ARP message) */  
    arp.hlen = 6;                                       /* hardware address length */  
    arp.plen = 4;                                       /* protocol address length */  
    arp.operation = htons(ARPOP_REQUEST);               /* ARP op code */  
    pf_memcpy(arp.sHaddr, from_mac, 6);                 /* source hardware address */  
    pf_memcpy(arp.sInaddr, &from_ip, sizeof(from_ip));  /* source IP address */  
    /* tHaddr is zero-fiiled */                         /* target hardware address */  
    pf_memcpy(arp.tInaddr, &test_ip, sizeof(test_ip));  /* target IP address */  
  
    pf_memset(&addr, 0, sizeof(addr));  
    safe_strncpy(addr.sa_data, interface, sizeof(addr.sa_data));  

    //广播arp包.  
    if (sendto(pfd[0].fd, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0) 
    {  
        // TODO: error message? caller didn't expect us to fail,  
        // just returning 1 "no reply received" misleads it.  
        pl_log(ERR, "ERR SEND");
        goto ret;  
    }  
  
    timeout_ts.tv_nsec = 0;
    timeout_ts.tv_sec = 2;
    pfd[0].events = POLLIN;  
    //这边他是害怕poll被信号打断，因此加了层循环，其实这边我们还可以使用ppoll的，就可以了。  
    r = ppoll(pfd, 1, &timeout_ts, &origmask);  
    if (r < 0)
    {  
        // TODO: error message? caller didn't expect us to fail,  
        // just returning 1 "no reply received" misleads it.  
        pl_log(ERR, "ERR POLL");
        goto ret;  
    }  

    if (r) 
    {  
        //读取返回数据.  
        r = read(pfd[0].fd, &arp, sizeof(arp));  

        //检测是否为应打包，发送ip是否为我们所请求的ip,这里是为了防止其他的数据包干扰我们检测。  
        if (r >= ARP_MSG_SIZE  
            && arp.operation == htons(ARPOP_REPLY)  
            /* don't check it: Linux doesn't return proper tHaddr (fixed in 2.6.24?) */  
            /* && memcmp(arp.tHaddr, from_mac, 6) == 0 */  
            && *((uint32_t *) arp.sInaddr) == test_ip) 
        {  
            //说明ip地址已被使用  
            rv = 0;  
        }  
    }  
  
 ret:  
    close(pfd[0].fd);  
    pl_log(INF, "%srp reply received for this address", rv ? "No a" : "A");  
    return rv;  
}  

/**********************************************************************************************
 * @API function  read_interface
 * @brief         获取网卡基本信息的接口
 * @input         interface         网口名称
 * @output        ifindex           待输出的网卡索引地址
                  addr              待输出的网卡IP address
                  arp               待输出的网卡MAC地址
 * @return        ulRes             0-succuss，other-failure
 *********************************************************************************************/
S32 read_interface(const char* interface, U32* ifindex, U32* addr, U8* arp)  
{  
    int fd;  
    struct ifreq ifr;  
    struct sockaddr_in * our_ip;  
  
    pf_memset(&ifr, 0, sizeof(ifr));  
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);  
  
    ifr.ifr_addr.sa_family = AF_INET;  
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);  
    
    if (addr)  
    {  
        if (ioctl(fd, SIOCGIFADDR, &ifr) != 0)  
        {  
            perror("ioctl");  
            close(fd);  
            return PF_RET_FAILURE;  
        }  
        our_ip = (struct sockaddr_in *) &ifr.ifr_addr;  
        *addr = our_ip->sin_addr.s_addr;  
        pl_log(INF, "ip of %s = %s ", interface, inet_ntoa(our_ip->sin_addr));  
    }  
  
    if (ifindex)  
    {  
        if (ioctl(fd, SIOCGIFINDEX, &ifr) != 0)  
        {  
            close(fd);  
            return PF_RET_FAILURE;  
        }  
        pl_log(INF, "adapter index %d", ifr. ifr_ifindex);  
        *ifindex = ifr. ifr_ifindex;  
    }  
  
    if (arp)  
    {  
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) != 0)  
        {  
            close(fd);  
            return PF_RET_FAILURE;  
        }  
        pf_memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);  
        pl_log(INF, "adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x",  
                arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);  
    }  
    close(fd);  
    return PF_RET_SUCCESS;  
}  

/**********************************************************************************************
 * @API function  pf_arpping_ipaddr_usable
 * @brief         Detection of IP Address Occupancy
 * @input         scIpAddr          IP address to be detect
                  scEthName         name of testing ethernet
 * @output        void
 * @return        ulRes             0 - IP address is available
                                    other - IP address is occupied
 *********************************************************************************************/
extern "C" S32 pf_arpping_ipaddr_usable(char* scIpAddr, char* scEthName)  
{  
    U32 TEST_IP;  
    U32 ip;  
    U8 mac[6];  

    if(PF_RET_FAILURE == pf_get_inet_aton(scIpAddr))
    {
        pl_log(ERR, "error IP ADDR %s", scIpAddr);
        return PF_RET_FAILURE;
    }

    TEST_IP = inet_addr(scIpAddr);  
    
    if(PF_RET_SUCCESS != read_interface(scEthName, NULL, &ip, mac))
    {
        pl_log(ERR, "ETH NAME NO EXIST %s", scEthName);
        return PF_RET_FAILURE;
    }
  
    if(FALSE == arpping(TEST_IP, ip, mac, scEthName))
    {
        pl_log(ERR, "IP ADDR IS USED NOW %s", scIpAddr);
        return PF_RET_FAILURE;
    }

    pl_log(INF, "IP ADDR CAN BE USED NOW %s", scIpAddr);
    return PF_RET_SUCCESS;  
}  

/**********************************************************************************************
 * @API function  pf_set_config_integer
 * @brief         Set the integer to the configuration file.
 * @input         scPath            the path of configuration file
                  ulVal             the setting value
 * @output        void
 * @return        ulRes             result
                                    0          - success
                                    0xFFFFFFFF - failure
 *********************************************************************************************/
extern "C" S32 pf_set_config_integer(const S8* scPath, U32 ulVal)
{
    U32 ulCfgValue;
    S8 aucStrBuf[PS_UPDATE_CFGSTR_LENGTH];        
    pf_memset(aucStrBuf, 0, PS_UPDATE_CFGSTR_LENGTH);

    sprintf((char*)aucStrBuf, "%d", ulVal);

    if(PF_RET_SUCCESS != pf_write_flush_file(scPath, aucStrBuf, strlen((char*)aucStrBuf)))
    {
        pl_log(ERR, "set CONFIG %s value %d FAILED", aucStrBuf, ulVal);        
        return PF_RET_FAILURE;
    }
   
    return PF_RET_SUCCESS;
}


/**********************************************************************************************
 * @API function  pf_get_config_integer
 * @brief         Get the integer of the configuration file.
 * @input         scPath            the path of configuration file
 * @output        void
 * @return        other - succuss
                  0xFFFFFFFF - failure
 *********************************************************************************************/
extern "C" S32 pf_get_config_integer(const S8* scPath)
{
    S32 slCfgValue;
    S8 aucStrBuf[PS_UPDATE_CFGSTR_LENGTH];        

    if(PF_RET_SUCCESS != pf_read_flush_file(scPath, aucStrBuf, sizeof(aucStrBuf)))
    {
        pl_log(ERR, "GET IPADDR STRING CONFIG %s", scPath);        
        return PF_RET_FAILURE;
    }

    slCfgValue = atoi((char*)aucStrBuf);

    return slCfgValue;
}


/**********************************************************************************************
 * @API function  pf_set_bootup_time
 * @brief         Set the current time into the file.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_set_bootup_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH];        
    U32 ulTime = (U32)time(NULL);

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_DR_BOOTUP_FILE);

    pf_set_config_integer(aucStrBuf, ulTime);
}


/**********************************************************************************************
 * @API function  pf_set_reboot_time
 * @brief         Set the reboot time into the file.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_set_reboot_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH];        
    U32 ulTime = (U32)time(NULL);

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_DR_REBOOT_FILE);

    pf_set_config_integer(aucStrBuf, ulTime);
}

/**********************************************************************************************
 * @API function  pf_get_reboot_time
 * @brief         Get the reboot time in the file.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" U32 pf_get_bootup_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH];        

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_DR_BOOTUP_FILE);

    return (U32)pf_get_config_integer(aucStrBuf);
}


/**********************************************************************************************
 * @API function  pf_get_reboot_time
 * @brief         Get the reboot time in the file.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" U32 pf_get_reboot_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH];        

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_DR_REBOOT_FILE);

    return (U32)pf_get_config_integer(aucStrBuf);
}

/**********************************************************************************************
 * @API function  pf_get_startup_script_time
 * @brief         Get the bootup time of the startup script.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" U32 pf_get_startup_script_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH];        

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_STARTUP_BOOTUP_FILE);

    return (U32)pf_get_config_integer(aucStrBuf);
}

/**********************************************************************************************
 * @API function  pf_get_monitor_script_time
 * @brief         Get the bootup time of the monitor script.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" U32 pf_get_monitor_script_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH]; 
    S32 slTime;

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_MONITOR_BOOTUP_FILE);

    slTime = pf_get_config_integer(aucStrBuf);

    //monitor文件不存在，进程未启动
    if(PF_RET_FAILURE == slTime)
    {
        return 0;
    }

    return (U32)slTime;
}

/**********************************************************************************************
 * @API function  pf_get_monitor_update_time
 * @brief         Get the update time of the monitor script.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" U32 pf_get_monitor_update_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH]; 
    S32 slTime;

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_MONITOR_CHECK_FILE);

    slTime = pf_get_config_integer(aucStrBuf);

    //monitor文件不存在，进程未启动
    if(PF_RET_FAILURE == slTime)
    {
        return 0;
    }

    return (U32)slTime;
}

/**********************************************************************************************
 * @API function  pf_get_monitor_interval_time
 * @brief         Get the interval time of the monitor script.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" U32 pf_get_monitor_interval_time(void)
{
    S8 aucStrBuf[PS_UPDATE_NAME_LENGTH];        
    S32 slTime;

    sprintf((CHAR*)aucStrBuf, "%s%s", pf_get_root_path(), TIME_MONITOR_INTERVAL_FILE);
    slTime = pf_get_config_integer(aucStrBuf);
    if(PF_RET_FAILURE == slTime)
    {
        slTime = DEFAULT_MONITOR_TIME_INTERVAL;
    }

    return (U32)slTime;
}

/**********************************************************************************************
 * @API function  pf_get_script_monitor_status
 * @brief         Get the bootup time in the file of monitor script.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" U32 pf_get_bootup_flag(void)
{
	if(PF_BOOTUP_FLAG_UNKNOWN == m_ulBootupFlag)
	{
	    CHAR aucBootupInfo[PS_UPDATE_CFGSTR_LENGTH];
		CHAR acPath[PS_UPDATE_NAME_LENGTH];
		
		sprintf(acPath, "%s%s", pf_get_root_path(), PF_BOOTUP_FILE_PATH);

	    S32 slRet = pf_read_flush_file((const S8 *)acPath, (const S8 *)aucBootupInfo, PS_UPDATE_CFGSTR_LENGTH);

	    /*bootup for monitor script*/
	    if(PF_RET_SUCCESS == slRet)
	    {
	        if(strstr(aucBootupInfo, "BOOTUP_FLAG_NORMAL"))
	    	{
				m_ulBootupFlag = PF_BOOTUP_FLAG_NORMAL;
			}
			else if(strstr(aucBootupInfo, "BOOTUP_FLAG_REBOOT"))
			{
				m_ulBootupFlag = PF_BOOTUP_FLAG_REBOOT;
			}
			else if(strstr(aucBootupInfo, "BOOTUP_FLAG_MONITOR"))
			{
				m_ulBootupFlag = PF_BOOTUP_FLAG_MONITOR;
			}
			else if(strstr(aucBootupInfo, "BOOTUP_FLAG_UPGRADE_REBOOT"))
			{
				m_ulBootupFlag = PF_BOOTUP_FLAG_UPGRADE_REBOOT;
			}
			else if(strstr(aucBootupInfo, "BOOTUP_FLAG_PROCESS_EXIT"))
			{
				m_ulBootupFlag = PF_BOOTUP_FLAG_PROCESS_EXIT;
			}
			else if(strstr(aucBootupInfo, "BOOTUP_FLAG_CORE_DUMP"))
			{
				m_ulBootupFlag = PF_BOOTUP_FLAG_CORE_DUMP;
			}
			else if(strstr(aucBootupInfo, "BOOTUP_FLAG_KILL"))
			{
				m_ulBootupFlag = PF_BOOTUP_FLAG_KILL;
			}
			else if(strstr(aucBootupInfo, "BOOTUP_FLAG_THREAD_MONITOR"))
			{
				m_ulBootupFlag = PF_BOOTUP_FLAG_THREAD_MONITOR;
			}
			else
			{
				pl_log(ERR, "pf_get_bootup_flag other reason: %s", aucBootupInfo); 	   
				m_ulBootupFlag = PF_BOOTUP_FLAG_OTHER;
			}

			unlink(acPath);
	    }
		/*monitor bootup*/
		else
		{
			m_ulBootupFlag = PF_BOOTUP_FLAG_MONITOR;
		}
	}

	return m_ulBootupFlag;
}


/**********************************************************************************************
 * @API function  pf_is_monitor_script_running
 * @brief         Whether the monitor script is running or not.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" BOOL pf_is_monitor_script_running(void)
{
    U32 ulMonTime = pf_get_monitor_update_time();
    U32 ulCurTime = (U32)time(NULL);
    U32 ulInterVal = pf_get_monitor_interval_time();

    /*如果脚本执行时间与间隔时间的和超过了当前时间，说明监控进程在运行*/
    if(ulMonTime + ulInterVal >= ulCurTime)
    {
        return TRUE;
    }

    return FALSE;
}


