/*******************************************************************************************************************
 **                                                                                                                        
 **  Copyright (c)  2019,  BroadXT, Inc.                                                                                 
 **        All    Rights Reserved.                                                                                            
 **                                                                                                                          
 **  Subsystem     : digital rail                                                                                             
 **  File          : pf_upgrade.h                                                                                      
 **  Created By    : josephzhou                                                                                              
 **  Created On    : 2019/7/7                                                                                                 
 **                                                                                                                         
 **  Purpose:                                                                                                             
 **    This file    contains the file upgrade api
 **                                                                                                                         
 **  History:                                                                                                             
 **  Programmer        Date    Rev    Description                                                                                 
 **  --------------- ---------- --------    ------------------------------                                                   
 **
 ******************************************************************************************************************/
#ifndef _PF_UPGRADE_H
#define _PF_UPGRADE_H


#define PS_UPDATE_NAME_LENGTH                   256
#define PS_UPDATE_CFGSTR_LENGTH                 32

#define PF_BOOTUP_FILE_PATH                     "/log/dr_bootup_flag.txt"


typedef enum  
{
    PF_BOOTUP_FLAG_UNKNOWN,         //initial unknown state
    PF_BOOTUP_FLAG_NORMAL,          //normal boot
    PF_BOOTUP_FLAG_REBOOT,          //reboot 
    PF_BOOTUP_FLAG_MONITOR,         //monitor boot 
    PF_BOOTUP_FLAG_UPGRADE_REBOOT,  //upgrade version boot 
    PF_BOOTUP_FLAG_PROCESS_EXIT,    //process exit boot  
    PF_BOOTUP_FLAG_CORE_DUMP,       //core dump boot  
    PF_BOOTUP_FLAG_KILL,            //kill boot  
    PF_BOOTUP_FLAG_THREAD_MONITOR,  //thread monitor error boot  
    PF_BOOTUP_FLAG_OTHER,           //other boot  
    PF_BOOTUP_FLAG_MAX,             //max ID
}PF_BOOTUP_FLAG_E;


#ifdef __cplusplus
extern "C" {
#endif

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
S32 pf_set_config_integer(const S8* scPath, U32 ulVal);

/**********************************************************************************************
 * @API function  pf_get_config_integer
 * @brief         Get the integer of the configuration file.
 * @input         scPath            the path of configuration file
 * @output        void
 * @return        other - succuss
                  0xFFFFFFFF - failure
 *********************************************************************************************/
S32 pf_get_config_integer(const S8* scPath);

/**********************************************************************************************
 * @API function  pf_arpping_ipaddr_usable
 * @brief         Detection of IP Address Occupancy
 * @input         scIpAddr          IP address to be detect
                  scEthName         name of testing ethernet
 * @output        void
 * @return        ulRes             0 - IP address is available
                                    other - IP address is occupied
 *********************************************************************************************/
S32 pf_arpping_ipaddr_usable(char* scIpAddr, char* scEthName);

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
 * @API function  pf_set_default_gw
 * @brief         Setting the current gateway address interface
 * @input         scGwAddr          Gateway address to be set
 * @output        void
 * @return        ulRes             result
                                    0      - success
                                    other  - failure
 *********************************************************************************************/
S32 pf_set_default_gw(S8* scGwAddr);

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
S32 pf_set_net_mask(S8* ifname, const S8 *netmask);

/**********************************************************************************************
 * @API function  pf_set_bootup_time
 * @brief         Set the current time into the file.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_set_bootup_time(void);

/**********************************************************************************************
 * @API function  pf_set_reboot_time
 * @brief         Set the reboot time into the file.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_set_reboot_time(void);


/**********************************************************************************************
 * @API function  pf_get_script_monitor_status
 * @brief         Get the bootup time in the file of monitor script.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
U32 pf_get_bootup_flag(void);

/**********************************************************************************************
 * @API function  pf_is_monitor_script_running
 * @brief         Whether the monitor script is running or not.
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
BOOL pf_is_monitor_script_running(void);


#ifdef __cplusplus
}
#endif

#endif//_PF_UPGRADE_H
