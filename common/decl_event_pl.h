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
*******************************************************90************/



/*----------平台内部交互消息定义---------*/
DECL_BEGIN(PLATFORM_BEGIN_MSG,       0, (EVENT_SEG_LEN*255)),//65280(0xFF00)
DECL_EVENT(PF_MC_READY_IND,          0),//65281(0xFF01)
DECL_EVENT(TIMER_EXPIRY_MSG,         0),//65282(0xFF02)
DECL_EVENT(FRAME_REPORT_IND,         0), //platform to MODULE_DRC_RCII 65283(0xFF03)
DECL_EVENT(TRACE_PC_LOG_INFO_IND,         0),//65284(0xFF04)
DECL_EVENT(GTEST_SOCKET_MSG_IND,          0),  // for gtest socket send msg 65285(0xFF05)
DECL_EVENT(OAM_FTP_GET_NEW_VERSION_REQ,   0),//65286(0xFF06)
DECL_EVENT(FTP_OAM_GET_NEW_VERSION_RSP,   0),//65287(0xFF07)
DECL_EVENT(OAM_FTP_GET_NEW_FILE_REQ,          0),//65288(0xFF08)
DECL_EVENT(FTP_OAM_GET_NEW_FILE_RSP,          0),//65289(0xFF09)
DECL_EVENT(OAM_PLATFORM_LOG_PARA_CFG_CMD,     0),//65290(0xFF0A)
DECL_EVENT(OAM_PLATFORM_LOG_SWITCH_CTRL_CMD,  0),//65291(0xFF0B)
DECL_EVENT(PLATFORM_OAM_LOG_UPLOAD_IND,       0),//65292(0xFF0C)
DECL_EVENT(OAM_FTP_PUT_NEW_FILE_REQ,       0),//65293(0xFF0D)
DECL_EVENT(FTP_OAM_PUT_NEW_FILE_RSP,       0),//65294(0xFF0E)
DECL_EVENT(LOG_RECORD_PUT_FTP_FILE_REQ,       0),//65295(0xFF0F)
DECL_EVENT(OAM_PLATFORM_LOG_UPLOAD_NOW_REQ,       0),//65296(0xFF10)
DECL_EVENT(OAM_FTP_PUT_NEW_FILE_NOW_REQ,       0),//65297(0xFF11)
DECL_EVENT(FTP_OAM_PUT_NEW_FILE_NOW_RSP,       0),//65298(0xFF12)
DECL_EVENT(OAM_FTP_PUT_LOG_FILE_REQ,       0),//65299(0xFF13)
DECL_EVENT(PF_DAILY_RECORD_LOG_INFO,       0),//65300(0xFF14)
DECL_EVENT(DRTS_CRM_SIMULATION_DATA_INFO,       0),//65301(0xFF15)
DECL_EVENT(PF_LOG_KAFKA_INFO,       0),//65302(0xFF16)



