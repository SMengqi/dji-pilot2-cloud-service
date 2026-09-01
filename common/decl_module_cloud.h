
/************************************************************
  Copyright (C), BroadXt Inc  2019
  FileName: decl_module_drc.h
  Author: josephzhou    Version :  1.0   Date: 20190617
  Description:     header of DRC module
  Function List:
    1. -------
  History:
  <author>      <time>          <version >   <desc>
  josephzhou    2019/6/17         1.0        initial
***********************************************************/


// lint -e835 -e845 -e831
//////////////////////BXTDEMO MODULE DEFINE////////////////////

              //system module
DECL_MODULE(LCF),            
DECL_MODULE(MQTTSUB),
DECL_MODULE(MQTTPUB),
DECL_MODULE(DEVICE),
DECL_MODULE(WAYPOINT),
DECL_MODULE(FLYTO),
DECL_MODULE(TRACK),
DECL_MODULE(PAYLOAD),

DECL_MODULE(PL_UTILS),

// latest module thread
DECL_MODULE(TOTAL_NUM),

// DRC maximum module thread
DECL_MODULE_BEGIN(MAX, MAX_DRSU_MODULE),

DECL_MODULE(TIMER),
DECL_MODULE(NETWORK),
DECL_MODULE(UNSOCK_SERVER),


// latest task thread
DECL_MODULE(TASK_MAX),

