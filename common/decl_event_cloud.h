/********************************************************************
  Copyright (C), broadxt Inc  2019
  FileName: decl_event_drc.h
  Author: josephzhou      Version : v0.1         Date:2019-5-24
  Description:     define message Identities
  History:     
  <author>       <time>         <version >      <desc>
  josephzhou     2019/05/24     v0.1            build this moudle 
  josephzhou     2019/06/13     v0.1            add DRC&DRSU, DRC&DRCRM message ID 
  josephzhou     2022/11/01     v0.2            add DRF message ID 
  note��only support three kinds of message: _REQ, _CFM, _IND, _RSP

        DRC<->ACU       0x0   ~0x3FF
        DRC<->DRSU      0x400 ~0x7FF
        DRC<->DRCRM     0x800 ~0xBFF
        DRC INTER       0x1000~0x3FFF
              
        DRSU<->DRC      0x4000~0x43FF
        DRSU INTER      0x5000~0x6FFF
        BXRDEMO INTER   0x6000~0x7FFF
              
        DRCRM<->DRC     0x8000~0x8FFF
        DRCRM INTER     0x9000~0xBFFF
        PLATFORM        0xFF00~0xFFFF
*******************************************************************/

/*----------BXTDEMO�ڲ�������Ϣ����---------*/
DECL_BEGIN(INTER_BEGIN_MSG,         0, (EVENT_SEG_LEN*96)),//24576(0x6000)
//timer out type ID
// DECL_EVENT(CTRL_CUM_REG_TIMER_OUT,          0),//24577(0x6001)

//cloud common
DECL_EVENT(COMMON_REG_IND, 0),//24581(0x6002)

//DATA
DECL_EVENT(FLYTO_RESULT_DATA_IND,           0),
DECL_EVENT(FLYTO_RESULT_HEARTBEAT_DATA_IND, 0),
DECL_EVENT(FLYTO_DRC_OSD_DATA_IND,              0),

DECL_EVENT(PAYLOAD_RESULT_DATA_IND,         0),
DECL_EVENT(PAYLOAD_GIMBAL_DATA_IND,         0),
DECL_EVENT(PAYLOAD_CAMERA_DATA_IND,         0),
DECL_EVENT(PAYLOAD_LIGHT_DATA_IND,          0),

DECL_EVENT(WAYPOINT_RESULT_DATA_IND,        0),


DECL_EVENT(TRACK_PUBLISH_DATA_IND,          0),
DECL_EVENT(DJI_SERVICES_PUBLISH_DATA_IND,   0),
DECL_EVENT(DJI_REAUESTS_REPLY_PUBLISH_DATA_IND,   0),
DECL_EVENT(DJI_DRC_DOWN_PUBLISH_DATA_IND,   0),
DECL_EVENT(BXT_UAV_STATUS_PUBLISH_DATA_IND, 0),
DECL_EVENT(BXT_UAV_RESULT_PUBLISH_DATA_IND, 0),
DECL_EVENT(PAYLOAD_PARAM_PUBLISH_DATA_IND,  0),

DECL_EVENT(KMZ_REQUEST_PUBLISH_DATA_IND,    0),
DECL_EVENT(KMZ_RESPONSE_DATA_IND,           0),
DECL_EVENT(WAYPOINT_TASK_READY_DATA_IND,    0),
DECL_EVENT(WAYPOINT_RESOURCE_GET_DATA_IND,  0),
DECL_EVENT(WAYPOINT_TASK_PROGRESS_DATA_IND, 0),

DECL_EVENT(DEVICES_RESULT_DATA_IND,         0),
DECL_EVENT(REMOTE_DEBUG_DATA_IND,           0),

// 新增事件必须追加在末尾：事件 ID 按位置顺序在段内自动编号，
// 中间插入会整体后移 DJI_SERVICES_PUBLISH_DATA_IND / DJI_DRC_DOWN_PUBLISH_DATA_IND 等的数值，
// 一旦各模块未完全一致重编译，模块间消息路由(pf_copy_msg)就会错乱，导致控制消息发不出去。
DECL_EVENT(FLYTO_PROGRESS_DATA_IND,         0),
DECL_EVENT(TRACK_STATE_DATA_IND,            0),

