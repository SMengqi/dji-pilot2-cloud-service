#define DECL_MODULE(x) #x
#define DECL_MODULE_BEGIN(x,y) #x
#define DECL_LOGMODULE(x) #x

#include <string.h>
#include "option.h"
#include "pl_type.h"

#if DRSU_PROCESS
const CHAR *module_name[] = 
{
    #include "decl_module.h"
    #include "decl_module_drsu.h"
    #include "decl_log_module.h"
};
#elif CLOUD_PROCESS
const CHAR *module_name[] = 
{
    #include "decl_module.h"
    #include "decl_module_cloud.h"
    #include "decl_log_module.h"
};
#elif CRM_PROCESS
const CHAR *module_name[] = 
{
    #include "decl_module.h"
    #include "decl_module_crm.h"
    #include "decl_log_module.h"
};
#elif ACU_PROCESS
const CHAR *module_name[] = 
{
    #include "decl_module.h"
    #include "decl_module_acu.h"
    #include "decl_log_module.h"
};
#else
/*DRC_PROCESS and UNIT_TEST*/
const CHAR *module_name[] = 
{
    #include "decl_module.h"
    #include "decl_module_drc.h"
    #include "decl_log_module.h"
};
#endif



#include "module.h"


extern "C" const CHAR* pf_get_module_name(U32 ulModuleId)
{
    if(ulModuleId <= MODULE_TOTAL_NUM)
    {
        return module_name[ulModuleId];
    }
    else if((ulModuleId >= MODULE_MAX) && (ulModuleId < LOG_MODULE_MAX))
    {
        return module_name[ulModuleId + MODULE_TOTAL_NUM + 1 - MODULE_MAX];
    }
    return "(x)";
}

extern "C" U32 pf_get_module_id(const CHAR* moduleName)
{
    U32 ulmid;
    U32 ulnum = LOG_MODULE_MAX + 1 + MODULE_TOTAL_NUM - MODULE_MAX;

    for(ulmid=0; ulmid<ulnum; ulmid++)
    {
        if(0 == memcmp(module_name[ulmid], moduleName, strlen(moduleName)))
        {
            if(ulmid > MODULE_TOTAL_NUM)
            {
                return (ulmid + MODULE_MAX - 1 - MODULE_TOTAL_NUM);
            }
            else
            {
                return ulmid;
            }
        }
    }

    return LOG_MODULE_MAX;
}



