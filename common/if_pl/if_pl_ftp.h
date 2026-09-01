#ifndef IF_OAM_FTP_H
#define IF_OAM_FTP_H

/************************************************************
  Copyright (C), Innofidei Inc  2012
  FileName   :if_pl_ftp.h
  Author     :yueminjuan     
  Version    :v0.1         
  Date       :2019-07-19
  Description:the interface between ACSM mode and OAM mode
  History:
  <author>       <time>       <version >   <desc>
  yueminjuan      2019-07-19   v0.1         create 
***********************************************************/


#define FTP_DOMAIN_NAME_LEN 64
#define FTP_PATH_LEN 64

#define FTP_VERSION_NUMBER_LEN 64

#define FTP_LOGIN_NAME_LEN 16
#define FTP_LOGIN_PASSWORD_LEN 16

#define FILE_PATH_LEN 256

#define MAX_DATE_LEN 16
#define MAX_TIME_LEN 16
#define MAX_DATE_AND_TIME_LEN 32

typedef enum
{
    LOG_TYPE_DEBUG = 1,
    LOG_TYPE_EVENT,
    LOG_TYPE_IMPORTANT,
    LOG_TYPE_ALARM
}DR_LOG_TYPE_E;

typedef enum
{
    SWITCH_OFF = 0,
    SWITCH_ON = 1
}FUNCTION_SWITCH_E;

typedef struct  
{
    S8 ascDomainName[FTP_DOMAIN_NAME_LEN];
    U32 ulPortId;
    S8 ascPath[FTP_PATH_LEN];
    U32 ulPadding;
}ftp_address;

typedef struct
{
    ftp_address stFtpAddress;
    U8 ascLoginName[FTP_LOGIN_NAME_LEN];
    U8 ascPassword[FTP_LOGIN_PASSWORD_LEN];
    U32 ulTimeCycle;  //unit is DAY
	U32 ulDevUploadOffset;   //S
	U8 strTartDate[MAX_DATE_LEN];  //YYYYMMDD	
	U8 strTime[MAX_TIME_LEN];      //HHmmss	
}dr_log_upload_para;

typedef struct
{
    U8 strStartTime[MAX_DATE_AND_TIME_LEN];//YYYYMMDDHHmm  the time log file was created
    U8 strEndTime[MAX_DATE_AND_TIME_LEN];  //YYYYMMDDHHmm  the time log file was modified lasted
    U8 strLogFileName[FTP_VERSION_NUMBER_LEN];   //log name:GLOBAL ID_event_YYYYMMDDHHmm.log;GLOBAL ID_event_ YYYYMMDDHHmm.log    
    U8 strFilePath[FTP_PATH_LEN];    //file path where the log updated
    U32 eFileType;   //DR_LOG_TYPE_E
    U32 padding;
}dr_log_upload_ind;



typedef enum
{
    PROCE_TYPE_DRC_SW = 0,
    PROCE_TYPE_DRSU_SW,
    PROCE_TYPE_DRSU_MAP,
    PROCE_TYPE_ACU_SW,
    PROCE_TYPE_ACU_MAP,
    PROCE_TYPE_COMMON_FILE,
}PROCE_TYPE_E;

typedef struct
{
    U32 ulCfgSrcMid;
    U32 ulPadding;
    ftp_address stFtpAddress;
    U8 ascLoginName[FTP_LOGIN_NAME_LEN];
    U8 ascPassword[FTP_LOGIN_PASSWORD_LEN];
}LOG_RECORD_FTP_CFG_REQ,FILE_NAME_MODIFY_FTP_REQ;


typedef struct
{
    U32 ulProceType;   //PROCE_TYPE_E
    U32 ulPadding;
    ftp_address stFtpAddress;
    S8 ascLoginName[FTP_LOGIN_NAME_LEN];
    S8 ascPassword[FTP_LOGIN_PASSWORD_LEN];
    S8 ascVersionNo[FTP_VERSION_NUMBER_LEN];  //file name
}OAM_FTP_GET_NEW_FILE_REQ_MSG;

typedef struct
{
    U32 ulProceType;   //PROCE_TYPE_E
    U32 ulCfgSrcMid;
    ftp_address stFtpAddress;
    S8 ascLoginName[FTP_LOGIN_NAME_LEN];
    S8 ascPassword[FTP_LOGIN_PASSWORD_LEN];
    S8 ascVersionNo[FTP_VERSION_NUMBER_LEN];  //file name
}OAM_FTP_PUT_NEW_FILE_REQ_MSG;


typedef struct
{
    S32 slRlt;
    U32 ulProceType;   //PROCE_TYPE_E
    S8 ascPath[FTP_PATH_LEN];
    S8 ascVersionNo[FTP_VERSION_NUMBER_LEN]; //file name
}FTP_OAM_GET_NEW_FILE_RSP_MSG,FTP_OAM_PUT_NEW_FILE_RSP_MSG;



typedef struct
{
    dr_log_upload_para stLogUploadPara;
}OAM_PLATFORM_LOG_PARA_CFG_CMD_MSG;


typedef struct
{
    U32 eLogOnOffFlag;//FUNCTION_SWITCH_E
}OAM_PLATFORM_LOG_SWITCH_CTRL_CMD_MSG;

typedef struct
{
    dr_log_upload_ind stUploadInd;
}PLATFORM_OAM_LOG_UPLOAD_IND_MSG,RECORD_LOG_FTP_CFG_RSP;

typedef struct
{
   U32 ulTransId;
   U32 ulPadding;
   ftp_address stFtpAddress;
   S8 ascLoginName[FTP_LOGIN_NAME_LEN];
   S8 ascPassword[FTP_LOGIN_PASSWORD_LEN];
}OAM_PLATFORM_UPLOAD_LOG_NOW_REQ_MSG;

typedef struct
{
   U32 ulTransId;
   U32 ulCfgSrcMid;
   ftp_address stFtpAddress;
   S8 ascLoginName[FTP_LOGIN_NAME_LEN];
   S8 ascPassword[FTP_LOGIN_PASSWORD_LEN];
   S8 ascVersionNo[FTP_VERSION_NUMBER_LEN];  //file name
}FTP_UPLOAD_LOG_NOW_REQ_MSG;

typedef struct
{
   U32 ulTransId;
   U32 ulResult; //0:success
}PLATFORM_OAM_UPLOAD_LOG_NOW_RSP_MSG;

#endif

