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

/*----------DRCRM内部交互消息定义---------*/
DECL_BEGIN(DRCRM_INTER_BEGIN_MSG,       0, (EVENT_SEG_LEN*144)),//36864


