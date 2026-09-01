/************************************************************
  Copyright (C), BroadXt Inc  2019
  FileName: module.h
  Author: josephzhou    Version :  1.0   Date: 20190617
  Description:     header of module
  Function List:    
    1. -------
  History:          
  <author>      <time>          <version >   <desc>
  josephzhou    2019/6/17         1.0        initial
***********************************************************/

#ifndef MODULE_H
#define MODULE_H

#include "option.h"
#include "pl_type.h"

#define MAX_DRSU_MODULE 90
#define MAX_DRC_MODULE  128

#undef DECL_MODULE
#define DECL_MODULE(x) MODULE_##x

#undef DECL_MODULE_BEGIN
#define DECL_MODULE_BEGIN(x,y)   MODULE_##x=y


#undef DECL_LOGMODULE
#define DECL_LOGMODULE(x) x

#if DRSU_PROCESS
enum DrsuModuleId
{
    #include "decl_module.h"
    #include "decl_module_drsu.h"
    #include "decl_log_module.h"
};
#elif  CRM_PROCESS
enum CrmModuleId
{
    #include "decl_module.h"
    #include "decl_module_crm.h"
    #include "decl_log_module.h"
};
#elif  CLOUD_PROCESS
enum CloudModuleId
{
    #include "decl_module.h"
    #include "decl_module_cloud.h"
    #include "decl_log_module.h"
};
#elif ACU_PROCESS
enum SaasModuleId
{
    #include "decl_module.h"
    #include "decl_module_acu.h"
    #include "decl_log_module.h"
};
#else
/*DRC_PROCESS and UNIT_TEST*/
enum DrcModuleId
{
    #include "decl_module.h"
    #include "decl_module_drc.h"
    #include "decl_log_module.h"
};
#endif


#ifdef __cplusplus
extern "C" {
#endif
const CHAR* pf_get_module_name(U32 ulModuleId);

U32 pf_get_module_id(const CHAR* moduleName);

#ifdef __cplusplus
}
#endif

extern const CHAR *module_name[];

#endif
