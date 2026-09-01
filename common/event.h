/************************************************************
  Copyright (C), BroadXt Inc  2019
  FileName: event.h
  Author: josephzhou    Version :  1.0   Date: 20190617
  Description:     header of event
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2019/6/17         1.0        initial
***********************************************************/

#ifndef EVENT_H
#define EVENT_H


/** 
*    Message Number Processed by Protocol Stack
*/
#include "option.h"
#include "pl_type.h"
#undef DECL_EVENT
#undef DECL_BEGIN
#define DECL_EVENT(x,y)     x
#define DECL_BEGIN(x,y,z)   x=z

#define EVENT_SEG_LEN 256

#if DRC_PROCESS
enum
{
    #include "decl_event_drc_acu.h"
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drc_crm_r4.h"
    #include "decl_event_drc.h"
    #include "decl_event_drc_banma.h"
    #include "decl_event_pl.h"
    LAST_EVENT
};
#elif DRSU_PROCESS
enum
{
    #include "decl_event_drc_drsu.h"
    #include "decl_event_pl.h"
    LAST_EVENT
};
#elif CLOUD_PROCESS
enum
{
    #include "decl_event_cloud.h"
    #include "decl_event_drc_crm_r4.h"
    #include "decl_event_drc.h"
    #include "decl_event_pl.h"
    LAST_EVENT
};
#else
enum
{
    #include "decl_event_drc_acu.h"
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drc_crm_r4.h"
    #include "decl_event_drc.h"
    #include "decl_event_crm.h"
    #include "decl_event_pl.h"
    LAST_EVENT
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
const char*pf_get_event_name(U32 ulEventId);
S32 pf_get_event_id(CHAR* pcEventName);

#ifdef __cplusplus
}
#endif
extern const char *event_name[];

#endif
