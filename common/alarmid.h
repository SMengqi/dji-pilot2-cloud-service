/************************************************************
  Copyright (C), BroadXt Inc  2020
  FileName: alarmid.h
  Author: josephzhou    Version :  1.0   Date: 20200311
  Description:     header of alarm id
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2020/3/11         1.0        initial
***********************************************************/
#ifndef ALARMID_H
#define ALARMID_H

#include "option.h"
#include "pl_type.h"

/** 
*    Error id information by Protocol Stack
*/
#undef DECL_ALARMID
#undef DECL_ALARMID_BEGIN
#define DECL_ALARMID(x)     x
#define DECL_ALARMID_BEGIN(x,y)   x=y

#define ALARMID_SEG_LEN 256


enum
{
    #include "decl_alarmid.h"
    LAST_ALARMID
};


#ifdef __cplusplus
extern "C" {
#endif

const char*pf_get_alarmid_name(U32 ulErrId);
const S32 pf_get_alarmid_id(CHAR* pcErridName);

#ifdef __cplusplus
}
#endif

extern const char *alarmid_name[];

#endif
