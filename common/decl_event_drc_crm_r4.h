/********************************************************************
  Copyright (C), broadxt Inc  2019
  FileName: decl_event_drc_crm.h
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


/*----------DRC和DRCRM交互消息定义---------*/
DECL_BEGIN(DRC_CRM_BEGIN_MSG,         0, (EVENT_SEG_LEN*8)),//2048(0x800)
DECL_EVENT(DRC_CRM_EVENT_IND,          0),//2049(0x801)
DECL_EVENT(CRM_DRC_EVENT_RSP,          0),//2050(0x802)
DECL_EVENT(DRC_CRM_TCI_IND,         0),//2051(0x803)
DECL_EVENT(DRC_CRM_FUSED_TRAFFIC_DATA_IND,         0),//2052(0x804)
DECL_EVENT(DRC_CRM_TRAFFICLIGHT_DATA_IND,     0),//2053(0x805)
DECL_EVENT(DRC_CRM_ACU_STATE_IND,     0),//2054(0x806)
DECL_EVENT(DRC_CRM_TRAFFIC_INFO_IND,          0),//2055(0x807)
DECL_EVENT(DRC_CRM_VT_POSITION_IND,          0),//2056(0x808)
