/************************************************************
  Copyright (C), BroadXt Inc  2019
  FileName: errid.h
  Author: josephzhou    Version :  1.0   Date: 20190617
  Description:     header of error id
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2019/6/17         1.0        initial
***********************************************************/
#ifndef ERRID_H
#define ERRID_H

#include "option.h"
#include "pl_type.h"

/** 
*    Error id information by Protocol Stack
*/
#undef DECL_ERRID
#undef DECL_ERRID_BEGIN
#define DECL_ERRID(x)     x
#define DECL_ERRID_BEGIN(x,y)   x=y

#define ERRID_SEG_LEN 256


enum
{
    #include "decl_errid.h"
    LAST_ERRID
};


#ifdef __cplusplus
extern "C" {
#endif

const char*pf_get_errid_name(U32 ulErrId);
const S32 pf_get_errid_id(CHAR* pcErridName);

#ifdef __cplusplus
}
#endif

extern const char *errid_name[];

#endif
