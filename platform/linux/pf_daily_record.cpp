/************************************************************************************************************************
**                                                                                                                        
**  Copyright (c)  2009,  Innofidei, Inc.                                                                                 
**        All    Rights Reserved.                                                                                            
**                                                                                                                          
**  Subsystem    : LTE/SMALLCELL                                                                                             
**  File         : pf_daily_record.cpp                                                                                   
**  Created By    : josephzhou                                                                                              
**  Created On    : 1/5/2016
**                                                                                                                         
**  Purpose:                                                                                                             
**    low priority daily record task
**                                                                                                                         
**  History:                                                                                                             
**  Programmer        Date    Rev    Description                                                                                 
**  --------------- ---------- --------    ------------------------------                                                   
**
************************************************************************************************************************/
#define THIS_MODULE MODULE_DAILYREC

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
#include "os.h"
#include "osport.h"
#include "pf_daily_record.h"
#include "pf_timer.h"


/*Sat Feb 20 20:36:40 UTC 2016*/
#define PF_DAILY_RECORT_START_TIME          1456000000

/*Define Daily record default cycle time is 10 milliseconds*/
#define MAX_DAILY_CYCLE_NUMBER        100


static U8 aucDRecFileName[PF_DAILY_RECORD_IND_MAX][20] = {
                                    "dailyrec_ext.dat",         \
                                    "dailyrec_warn.dat",        \
                                    "dailyrec_event.dat"};

static U32 aulDRecFileLen[PF_DAILY_RECORD_IND_MAX] = {
                                    524288,                 \
                                    131072,                 \
                                     20480};

static U32 aulDRecMaxLen[PF_DAILY_RECORD_IND_MAX] = {
                                    40960,                  \
                                       32,                  \
                                     1024};

/*用于记录日志状态的变量*/
static U32 ulDRecTmpSta = 0;

/*用于记录日志占满状态的变量*/
static U32 ulDRecFullSta = 0;

/*用于记录定时器ID的变量*/
U32 ulDailyTimerId = 0;


extern PF_MUTEX_T g_stFileMutex;
extern FileInfoList g_stFileMapInfo;


/**********************************************************************************************
 * @API function  pf_daily_record_status_set
 * @brief         设置日志状态标识的接口
 * @input         ulDRecId      该标识为日志枚举值的宏定义
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_daily_record_status_set(U32 ulDRecId)
{
    if(ulDRecId < PF_DAILY_RECORD_IND_MAX)
    {
        ulDRecTmpSta |= (1<<ulDRecId);
    }
}

/**********************************************************************************************
 * @API function  pf_daily_record_status_clear
 * @brief         清空日志状态标识的接口
 * @input         ulDRecId      该标识为日志枚举值的宏定义
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_daily_record_status_clear(U32 ulDRecId)
{
    if(ulDRecId < PF_DAILY_RECORD_IND_MAX)
    {
        ulDRecTmpSta &= (~(1<<ulDRecId));
    }
}


/**********************************************************************************************
 * @API function  pf_daily_record_status_clear
 * @brief         清空日志状态标识的接口
 * @input         void
 * @output        void
 * @return        ulDRec        该标识中各个bit位与日志枚举值的宏定义一一对应
 *********************************************************************************************/
extern "C" U32 pf_daily_record_status_get(void)
{
    return ulDRecTmpSta;
}


/**********************************************************************************************
 * @API function  pf_daily_record_full_set
 * @brief         设置日志状态占满标识的接口
 * @input         ulDRecId      该标识为日志枚举值的宏定义
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_daily_record_full_set(U32 ulDRecId)
{
    if(ulDRecId < PF_DAILY_RECORD_IND_MAX)
    {
        ulDRecFullSta |= (1<<ulDRecId);
    }
}

/**********************************************************************************************
 * @API function  pf_daily_record_full_clear
 * @brief         清空日志占满状态标识的接口
 * @input         ulDRecId      该标识为日志枚举值的宏定义
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_daily_record_full_clear(U32 ulDRecId)
{
    if(ulDRecId < PF_DAILY_RECORD_IND_MAX)
    {
        ulDRecFullSta &= (~(1<<ulDRecId));
    }
}

/**********************************************************************************************
 * @function      pf_daily_record_timer_init
 * @brief         Daily buffer timer initialization interface
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_daily_record_timer_init(void)
{
    /*start cycle timer*/
    pf_timer_cycle_start(MODULE_DAILYREC, 
        MAX_DAILY_CYCLE_NUMBER, 
        0, 
        0, 
        &ulDailyTimerId);
}


/**********************************************************************************************
 * @function      daily_record_init
 * @brief         日志线程初始化
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
S32 daily_record_init(U32 ulModuleId)
{
	pf_daily_record_timer_init();

    pl_log(ERR, "%s:%d INIT %d", __FUNCTION__, __LINE__, ulModuleId);
	
    return PF_RET_SUCCESS;
}

U32 record_send_ftp_server(LOG_RECORD_FTP_CFG_REQ* pCfgMsg)
{
    S8 ascStartTime[MAX_DATE_AND_TIME_LEN];
    U8 strStartTime[MAX_DATE_AND_TIME_LEN];
    CHAR ascPath[1024];
    CHAR* pscRootPath = pf_get_root_path();
    
    struct timeval tv;
    pf_get_timeofday(&tv, NULL);
    struct tm info;
    pf_get_localtime_nolocks(&info, tv.tv_sec);
    
    OAM_FTP_PUT_NEW_FILE_REQ_MSG ftpFileMsg;
    pf_memset(&ftpFileMsg, 0, sizeof(ftpFileMsg));

    ftpFileMsg.ulProceType = PROCE_TYPE_COMMON_FILE;
    ftpFileMsg.ulCfgSrcMid = pCfgMsg->ulCfgSrcMid;
    pf_memcpy(ftpFileMsg.ascLoginName, pCfgMsg->ascLoginName, FTP_LOGIN_NAME_LEN);
    pf_memcpy(ftpFileMsg.ascPassword, pCfgMsg->ascPassword, FTP_LOGIN_PASSWORD_LEN);
    pf_memcpy(&ftpFileMsg.stFtpAddress, &pCfgMsg->stFtpAddress, sizeof(ftp_address));

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
    pf_read_flush_file((const S8 *)pf_get_log_path(), (const S8 *)ascStartTime, MAX_DATE_AND_TIME_LEN);
    pf_memcpy(&strStartTime[0], ascStartTime, 8);
    strStartTime[8] = ascStartTime[9];
    strStartTime[9] = ascStartTime[10];
    strStartTime[10] = ascStartTime[12];
    strStartTime[11] = ascStartTime[13];
    
    if(pf_is_file_exist((S8*)"./dailyrec_ext.dat"))
    {
       pf_read_flush_file((const S8 *)"./dailyrec_ext.dat", (const S8 *)ascStartTime, MAX_DATE_AND_TIME_LEN);
       pf_memcpy(&strStartTime[0], ascStartTime, 8);
       strStartTime[8] = ascStartTime[9];
       strStartTime[9] = ascStartTime[10];
       strStartTime[10] = ascStartTime[12];
       strStartTime[11] = ascStartTime[13];
       
       sprintf((CHAR*)ftpFileMsg.ascVersionNo, "%s_ext_%s_%04d%02d%02d%02d%02d.dat", pscDrName, strStartTime,    \
            info.tm_year + 1900, info.tm_mon + 1, info.tm_mday,                                 \
            info.tm_hour, info.tm_min);

        sprintf(ascPath, "%s/ftp/%s", pscRootPath, (CHAR*)ftpFileMsg.ascVersionNo);
        if(PF_RET_SUCCESS == pf_copy_flush_file(ascPath, "./dailyrec_ext.dat"))
        {
            pf_copy_msg(MODULE_DAILYREC, OAM_FTP_PUT_LOG_FILE_REQ, MODULE_FTP, &ftpFileMsg, sizeof(OAM_FTP_PUT_NEW_FILE_REQ_MSG));
            unlink("./dailyrec_ext.dat");
        }
    }
    
    if(pf_is_file_exist((S8*)"./dailyrec_warn.dat"))
    {
       pf_read_flush_file((const S8 *)"./dailyrec_warn.dat", (const S8 *)ascStartTime, MAX_DATE_AND_TIME_LEN);
       pf_memcpy(&strStartTime[0], ascStartTime, 8);
       strStartTime[8] = ascStartTime[9];
       strStartTime[9] = ascStartTime[10];
       strStartTime[10] = ascStartTime[12];
       strStartTime[11] = ascStartTime[13];
       
       sprintf((CHAR*)ftpFileMsg.ascVersionNo, "%s_war_%s_%04d%02d%02d%02d%02d.dat", pscDrName, strStartTime,      \
            info.tm_year + 1900, info.tm_mon + 1, info.tm_mday,                                 \
            info.tm_hour, info.tm_min);

        sprintf(ascPath, "%s/ftp/%s", pscRootPath, (CHAR*)ftpFileMsg.ascVersionNo);
        if(PF_RET_SUCCESS == pf_copy_flush_file(ascPath, "./dailyrec_warn.dat"))
        {
             pf_copy_msg(MODULE_DAILYREC, OAM_FTP_PUT_LOG_FILE_REQ, MODULE_FTP, &ftpFileMsg, sizeof(OAM_FTP_PUT_NEW_FILE_REQ_MSG));        
             unlink("./dailyrec_warn.dat");
        }
    }
    
    if(pf_is_file_exist((S8*)"./dailyrec_event.dat"))
    {
       pf_read_flush_file((const S8 *)"./dailyrec_event.dat", (const S8 *)ascStartTime, MAX_DATE_AND_TIME_LEN);
       pf_memcpy(&strStartTime[0], ascStartTime, 8);
       strStartTime[8] = ascStartTime[9];
       strStartTime[9] = ascStartTime[10];
       strStartTime[10] = ascStartTime[12];
       strStartTime[11] = ascStartTime[13];

       sprintf((CHAR*)ftpFileMsg.ascVersionNo, "%s_evt_%s_%04d%02d%02d%02d%02d.dat", pscDrName, strStartTime,      \
            info.tm_year + 1900, info.tm_mon + 1, info.tm_mday,                                 \
            info.tm_hour, info.tm_min);

        sprintf(ascPath, "%s/ftp/%s", pscRootPath, (CHAR*)ftpFileMsg.ascVersionNo);
        if(PF_RET_SUCCESS == pf_copy_flush_file(ascPath, "./dailyrec_event.dat"))
        {
            pf_copy_msg(MODULE_DAILYREC, OAM_FTP_PUT_LOG_FILE_REQ, MODULE_FTP, &ftpFileMsg, sizeof(OAM_FTP_PUT_NEW_FILE_REQ_MSG));
            unlink("./dailyrec_event.dat");
        }
    }
    return PF_RET_SUCCESS;
}

/**********************************************************************
Function:
    int daily_record_entry(U16 usSrcModuleId, U16 usMsgId, U16 usDstModuleId, 
                          void* pcvMsg, U16 usLength)
Description: 
    日志消息入口参数，由平台调用
Input:
    usSrcModuleId: source module identification
    usMsgId: message identification
    usDstModuleId: 目的模块标识号
    pcvMsg: 消息体地址 
    usLength: 消息体长度 
Output:
    void
Return: 
    0-success
    other-failure
Others:        
************************************************************************/
int daily_record_entry(U32 usSrcModuleId,
                U32 usMsgId,
                U32 usDstModuleId, 
                void* pcvMsg,
                U32 usLength)
{
    switch(usMsgId)
    {
        case PF_DAILY_RECORD_LOG_INFO:
        {
             S32 slRslt = PF_RET_SUCCESS;
            PF_DAILY_RECORD_S* pstDRec = (PF_DAILY_RECORD_S*)pcvMsg;
            S8 ascPath[50];
            U32 ulTmpLength;

            /*消息ID错误，返回失败信息*/
            if((PF_DAILY_RECORD_LOG_INFO != usMsgId) || (NULL == pstDRec))
            {
                pl_log(ERR, "src:%d, msg:0x%x,dst:%d,len:%d", \
                    usSrcModuleId,  \
                    usMsgId,        \
                    usDstModuleId,  \
                    usLength);
                return PF_RET_FAILURE;
            }
            
            U32 ulType = pstDRec->ulType;
            CHAR *  pcEventName = (CHAR *)pf_get_event_name(pstDRec->ulEventId);
            CHAR * pcEventNameTmp  = (CHAR *)pf_malloc(strlen(pcEventName)*2+pstDRec->ulLen);
            if(!pcEventNameTmp)
            {
                pl_log(ERR, "pcEventNameTmp malloc error");
                return PF_RET_FAILURE;
            }

            /*daily record data add event name*/
            pf_memcpy(pcEventNameTmp, pcEventName, strlen(pcEventName));
            CHAR * pcWrPtr = pcEventNameTmp+strlen(pcEventNameTmp);
            *(pcWrPtr++) = '\n';

            pf_memcpy(pcWrPtr,(CHAR *)pstDRec->aucData,pstDRec->ulLen);
            U32 ulLenTmp = strlen(pcEventName)+pstDRec->ulLen+1;
            pf_memcpy(pstDRec->aucData, pcEventNameTmp, ulLenTmp);
            pf_free(pcEventNameTmp);
            //pl_log(INF, "%s:%d start", __FUNCTION__, __LINE__);

            /*消息类型错误，返回失败信息*/
            if(ulType >= PF_DAILY_RECORD_IND_MAX)
            {
                pl_log(ERR, "daily record ulType %d exceed", ulType);
                return PF_RET_FAILURE;
            }
            /*文件写满情况*/
            if(1 == ((ulDRecFullSta>>ulType) & 1))
            {
                pl_log(WARN, "daily record %d full and clear" ,aulDRecFileLen[ulType]);
                pf_daily_record_clear(ulType);
            }

            U32 ulWriteLen =  ulLenTmp+24;
            S8 pscData[ulWriteLen];
            struct timeval tv;
            pf_get_timeofday(&tv, NULL);
            struct tm tm_info;
            struct tm *info = &tm_info;
            pf_get_localtime_nolocks(info, tv.tv_sec);
            pf_memset(ascPath, 0, sizeof(ascPath));
            pf_memset(pscData, 0, sizeof(pscData));
            sprintf((char*)ascPath, "%s%s",PF_DAILY_RECORD_ROOT_PATH, aucDRecFileName[ulType]);
            sprintf((char*)pscData,"%04d%02d%02d %02d:%02d %d %d %s",   \
                info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,                              \
                info->tm_hour, info->tm_min,pstDRec->ulEventId,pstDRec->ulLen,&pstDRec->aucData);
            
            /*写入临时文件*/
            slRslt = pf_write_endof_file(
                ascPath,                                \
                (const S8 *)pscData,         \
                strlen((char *)pscData));
            if(PF_RET_SUCCESS != slRslt)
            {
                pl_log(ERR, "write file failed %d", ulLenTmp);
                return PF_RET_FAILURE;
            }

           slRslt = pf_write_endof_file(
                ascPath,                                \
                (const S8 *)"\t\t\r\n5AA555AA",                     \
                8); 
            if(PF_RET_SUCCESS != slRslt)
            {
                pl_log(ERR, "write file end failed");
                return PF_RET_FAILURE;
            }

            if(PF_RET_SUCCESS != pf_get_file_length(ascPath, &ulTmpLength))
            {
                pl_log(ERR, "get file %s length failed", ascPath);
                return PF_RET_FAILURE;
            }

            if( ulTmpLength > aulDRecFileLen[ulType])
            {
                pl_log(WARN, "daily record %d full %d exceed %d", ulType, ulTmpLength, aulDRecFileLen[ulType]);
                /*置位占满标识*/
                pf_daily_record_full_set(ulType);
             }
             
            pf_daily_record_status_set(ulType);
            return slRslt;     
            break;
        }
        case LOG_RECORD_PUT_FTP_FILE_REQ:
        {
            LOG_RECORD_FTP_CFG_REQ *pstRecReq = (LOG_RECORD_FTP_CFG_REQ*)pcvMsg;
            record_send_ftp_server(pstRecReq);
            break;
        }

		case TIMER_EXPIRY_MSG:
		{
			U32 j;
			U32 k;

			//检查map中存在数据
			if(g_stFileMapInfo.size())
			{
				//拷贝map到本地，并释放公共map中的数据
				PF_MUTEX_LOCK(&g_stFileMutex);

				std::map<std::string, std::vector<std::shared_ptr<std::string>>> tmpMap = g_stFileMapInfo;
				g_stFileMapInfo.clear();
				PF_MUTEX_UNLOCK(&g_stFileMutex);
				
				std::map<std::string, std::vector<std::shared_ptr<std::string>>>::iterator it;
				U32 count = 0;

				for(it=tmpMap/*[i]*/.begin();it!=tmpMap/*[i]*/.end();it++)
				{
					count++;
					std::vector<std::shared_ptr<std::string>> tmpV = it->second;
					U32 ulSize = tmpV.size();

					//同一个文件超过3次后合并写入
					if(ulSize >= 3)
					{
						string strInfo = "";
						for(k=0; k<ulSize; k++)
						{
							//写入数据到文件中
							strInfo += *(tmpV[k].get());
						}						
						
						pf_write_endof_file((const S8 *)it->first.c_str(), (const S8 *)(strInfo.c_str()), strInfo.length());
						PS_CPlus(CM_COM, CMCOM_ID_COPYMSG_ENDOF_CNT);
						PS_CPlusV(CM_COM, CMCOM_ID_COPYMSG_ENDOF_SIZE, strInfo.length());
					}
					else
					{
						for(k=0; k<ulSize; k++)
						{
							//写入数据到文件中
							pf_write_endof_file((const S8 *)it->first.c_str(), (const S8 *)(tmpV[k].get()->c_str()), tmpV[k].get()->length());
							PS_CPlus(CM_COM, CMCOM_ID_COPYMSG_ENDOF_CNT);
							PS_CPlusV(CM_COM, CMCOM_ID_COPYMSG_ENDOF_SIZE, tmpV[k].get()->length());
						}						
					}

				}
				tmpMap.clear();

			}

			break;
		}		 

        
        default: 
            FUNCTION_TRACE;
            break;
    }
    return PF_RET_SUCCESS;
}



/**********************************************************************************************
 * @API function  pf_daily_record_set
 * @brief         设置日志类型的接口
 * @input         pstDRec       日志结构体
 * @output        void
 * @return        0      - success
                  other  - failure
 *********************************************************************************************/
extern "C" S32 pf_daily_record_set(PF_DAILY_RECORD_S* pstDRec)
{
    S32 slRslt = PF_RET_SUCCESS;
    time_t now;   
  
    if(NULL == pstDRec)
    {
        return PF_RET_FAILURE;
    }

    if((pstDRec->ulType >= PF_DAILY_RECORD_IND_MAX) || (pstDRec->ulLen >= aulDRecMaxLen[pstDRec->ulType]))
    {
        pl_log(ERR, "daily record ulType %d or len %d exceed", pstDRec->ulType, pstDRec->ulLen);
        return PF_RET_FAILURE;
    }

    /*获取当前绝对时间*/
 //   time(&now);
  //  pstDRec->ulTime = (U32)now;

    pf_copy_msg(MODULE_DAILYREC,            \
        PF_DAILY_RECORD_LOG_INFO,           \
        MODULE_DAILYREC,                    \
        (void *)pstDRec,                    \
        sizeof(PF_DAILY_RECORD_S) + pstDRec->ulLen);

    return slRslt;
}


/**********************************************************************************************
 * @API function  pf_daily_record_is_full
 * @brief         检测日志是否占满的接口
 * @input         void
 * @output        pulDRec       标识中各个bit位与日志枚举值的宏定义一一对应
 * @return        0      - 未占满
                  1      - 已占满
 *********************************************************************************************/
extern "C" BOOL pf_daily_record_is_full(U32* pulDRec)
{
    /*如果需要输出值则置初始值为0*/
    if(NULL != pulDRec)
    {
        *pulDRec = ulDRecFullSta;
    }

    if(ulDRecFullSta)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**********************************************************************************************
 * @API function  pf_daily_record_clear
 * @brief         清空日志的接口
 * @input         ulDRecId      该标识为日志枚举值的宏定义
 *                              大于等于最大值时，则清空所有日志文件
 * @output        void
 * @return        void
 *********************************************************************************************/
extern "C" void pf_daily_record_clear(U32 ulDRecId)
{
    U8 ascPath[50];
    U32 i;
    U32 ulStrLen = strlen(PF_DAILY_RECORD_ROOT_PATH);

    pf_memset(ascPath, 0, sizeof(ascPath));
    sprintf((char*)ascPath, "%s", PF_DAILY_RECORD_ROOT_PATH);

    if(PF_DAILY_RECORD_IND_MAX > ulDRecId)
    {
        pf_memcpy(ascPath+ulStrLen, aucDRecFileName[ulDRecId], strlen((char*)aucDRecFileName[ulDRecId]));

        unlink((char*)ascPath);

        /*清空占满标识*/
        pf_daily_record_full_clear(ulDRecId);
    }
    else
    {
        /*清空所有日志文件*/
        for(i=0; i<PF_DAILY_RECORD_IND_MAX; i++)
        {
            pf_memcpy(ascPath+ulStrLen, aucDRecFileName[i], strlen((char*)aucDRecFileName[i]));

            unlink((char*)ascPath);

            /*清空占满标识*/
            pf_daily_record_full_clear(ulDRecId);
        }
    }
}

