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
#ifndef __PF_DAILY_RECORD_H__
#define __PF_DAILY_RECORD_H__

#include "../common/pl.h"


/*日志类型枚举定义*/
typedef enum
{
    PF_DAILY_RECORD_EXTERNAL_IND = 0,   /*外部事件日志*/
    PF_DAILY_RECORD_WARNING_IND,        /*设备告警日志*/
    PF_DAILY_RECORD_EVENT_IND,         /*重要事件日志*/
    PF_DAILY_RECORD_IND_MAX,            /*日志类型最大值*/
}PF_DAILY_RECORD_E;


/*日志存储接口结构体定义*/
typedef struct{
    U32 ulType;             /*日志类型*/
    U32 ulTime;             /*日志时间*/
    U32 ulEventId;        /*日志事件编号*/
    U32 ulLen;              /*日志参数长度*/
    U8 aucData[0];          /*日志参数内容*/
}PF_DAILY_RECORD_S;

/*平台文件存储接口结构体定义*/
typedef struct{
    U32 ulTime;             /*日志时间*/
    U32 ulEventId;        /*日志事件编号*/
    U32 ulLen;              /*日志参数长度*/
    U8 aucData[0];          /*日志参数内容*/
}PF_DAILY_RECORD_FILE_S;


#define PF_DAILY_RECORD_ROOT_PATH "./log/"



/**********************************************************************************************
 * @function      daily_record_init
 * @brief         日志线程初始化
 * @input         void
 * @output        void
 * @return        void
 *********************************************************************************************/
void daily_record_init();

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
int daily_record_entry(U16 usSrcModuleId,
                U16 usMsgId,
                U16 usDstModuleId, 
                void* pcvMsg,
                U16 usLength);

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************
 * @API function  pf_daily_record_set
 * @brief         设置日志类型的接口
 * @input         pstDRec       日志结构体
 * @output        void
 * @return        0      - success
                  other  - failure
 *********************************************************************************************/
S32 pf_daily_record_set(PF_DAILY_RECORD_S* pstDRec);

/**********************************************************************************************
 * @API function  pf_daily_record_is_full
 * @brief         检测日志是否占满的接口
 * @input         void
 * @output        pulDRec       标识中各个bit位与日志枚举值的宏定义一一对应
 * @return        0      - 未占满
                  1      - 已占满
 *********************************************************************************************/
BOOL pf_daily_record_is_full(U32* pulDRec);

/**********************************************************************************************
 * @API function  pf_daily_record_clear
 * @brief         清空日志的接口
 * @input         ulDRecId      该标识为日志枚举值的宏定义
 *                              大于等于最大值时，则清空所有日志文件
 * @output        void
 * @return        void
 *********************************************************************************************/
void pf_daily_record_clear(U32 ulDRecId);

#ifdef __cplusplus
}
#endif

#endif

