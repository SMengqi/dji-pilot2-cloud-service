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
        BXTDEMO INTER   0x6000~0x7FFF
              
        DRCRM<->DRC     0x8000~0x8FFF
        DRCRM INTER     0x9000~0xBFFF
        PLATFORM        0xFF00~0xFFFF
*******************************************************************/

/*----------DRF�ڲ�������Ϣ����---------*/
// DECL_BEGIN(FIVGA,         0, (EVENT_SEG_LEN*96)),//24576(0x6000)
//timer out type ID
// DECL_EVENT(CTRL_CUM_REG_TIMER_OUT,          0),//24577(0x6001)

//drf�е�commonģ��ע����Ϣ
// DECL_EVENT(DRF_COMMON_REG_IND, 0),//24581(0x6005)

//DATA
// DECL_EVENT(DRF_CSM_CUM_DATA_IND,   0),//24582(0x6006)
// DECL_EVENT(DRF_CUM_CFM_DATA_IND,   0),//24583(0x6007)
// DECL_EVENT(DRF_CFM_FBM_DATA_IND,   0),//24584(0x6008)

