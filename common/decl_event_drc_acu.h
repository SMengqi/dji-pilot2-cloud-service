/********************************************************************
  Copyright (C), broadxt Inc  2019
  FileName: decl_event_drc.h
  Author: josephzhou      Version : v0.1         Date:2019-5-24
  Description:     define message Identities
  History:     
  <author>       <time>         <version >      <desc>
  josephzhou     2019/05/24     v0.1            build this moudle 
  josephzhou     2019/06/13     v0.1            add DRC&DRSU, DRC&DRCRM message ID 
  note：only support three kinds of message: _REQ, _CFM, _IND, _RSP

        DRC<->ACU       0x0   ~0x3FF
        DRC<->DRSU      0x400 ~0x7FF
        DRC<->DRCRM     0x800 ~0xBFF
        DRC INTER       0x1000~0x3FFF
              
        DRSU<->DRC      0x4000~0x43FF
        DRSU INTER      0x5000~0x7FFF
              
        DRCRM<->DRC     0x8000~0x8FFF
        DRCRM INTER     0x9000~0xBFFF
        PLATFORM        0xFF00~0xFFFF
*******************************************************************/


/*----------DRC和ACU交互消息定义---------*/
DECL_BEGIN(DRC_ACU_BEGIN_MSG,           0, 0), // 0(0x0)
DECL_EVENT(ACU_MSG_TYPE_CONNECT_REQ,         0),//1(0x1)
DECL_EVENT(ACU_MSG_TYPE_CONNECT_RSP,         0),//2(0x2)
DECL_EVENT(ACU_MSG_TYPE_DISCONNECT_REQ,      0),//3(0x3)
DECL_EVENT(ACU_MSG_TYPE_DISCONNECT_RSP,      0),//4(0x4)
DECL_EVENT(ACU_MSG_TYPE_VEHICLE_STATE,       0),//5(0x5)
DECL_EVENT(ACU_MSG_TYPE_RESERVED,            0),//6(0x6)
DECL_EVENT(ACU_MSG_TYPE_RESERVED1,           0),//7(0x7)
DECL_EVENT(ACU_MSG_TYPE_BUS_INFO,            0),//8(0x8)
DECL_EVENT(ACU_MSG_TYPE_ALARM_RPT,           0),//9(0x9)
DECL_EVENT(ACU_MSG_TYPE_ALARM_CFM,           0),//10(0xA)
DECL_EVENT(ACU_MSG_TYPE_ALARM_CLEAN_RPT,     0),//11(0xB)
DECL_EVENT(ACU_MSG_TYPE_ALARM_CLEAN_CFM,     0),//12(0xC)
DECL_EVENT(ACU_MSG_TYPE_MAP_UPDATE_REQ,      0),//13(0xD)
DECL_EVENT(ACU_MSG_TYPE_MAP_UPDATE_CFM,      0),//14(0xE)
DECL_EVENT(ACU_MSG_TYPE_MAP_UPDATE_IND,      0),//15(0xF)
DECL_EVENT(ACU_MSG_TYPE_SW_UPDATE_REQ,       0),//16(0x10)
DECL_EVENT(ACU_MSG_TYPE_SW_UPDATE_CFM,       0),//17(0x11)
DECL_EVENT(ACU_MSG_TYPE_SW_UPDATE_IND,       0),//18(0x12)
DECL_EVENT(ACU_MSG_TYPE_ALIVE_REQ,           0),//19(0x13)
DECL_EVENT(ACU_MSG_TYPE_ALIVE_RSP,           0),//20(0x14)
DECL_EVENT(ACU_MSG_TYPE_SCHEDULE_REQ,        0),//21(0x15)
DECL_EVENT(ACU_MSG_TYPE_SCHEDULE_CFM,        0),//22(0x16)
DECL_EVENT(ACU_MSG_TYPE_DATA,                0),//23(0x17)
DECL_EVENT(ACU_MSG_TYPE_CONFIG_REQ,          0),//24(0x18)
DECL_EVENT(ACU_MSG_TYPE_CONFIG_CFM,          0),//25(0x19)
DECL_EVENT(ACU_MSG_TYPE_OBSTACLE_RPT,        0),//26(0x1A)
DECL_EVENT(ACU_MSG_TYPE_BUS_CTRL_RPT,        0),//27(0x1B)
DECL_EVENT(ACU_MSG_TYPE_BUS_FUNCTION_DISABLE_RPT,        0),//28(0x1C)
DECL_EVENT(ACU_MSG_TYPE_VIDEO_DATA_RPT,        0),//29(0x1D)

