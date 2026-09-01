#define THIS_MODULE
#define DECL_EVENT(x,y) #x
#define DECL_BEGIN(x,y,z) #x

#include "option.h"

#if DRC_PROCESS
const char *event_name[] = 
{
    #include "decl_event_drc_acu.h"
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drc_crm_r4.h"
    #include "decl_event_drc.h"
	#include "decl_event_drc_banma.h"
    #include "decl_event_pl.h"
    "LAST_EVENT"
};
#elif DRSU_PROCESS
const char *event_name[] = 
{
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drsu.h"
    #include "decl_event_pl.h"
    "LAST_EVENT"
};
#elif CLOUD_PROCESS
const char *event_name[] = 
{
    #include "decl_event_cloud.h"
    #include "decl_event_drc_crm_r4.h"
    #include "decl_event_drc.h"
    #include "decl_event_pl.h"
    "LAST_EVENT"
};
#else
const char *event_name[] = 
{
    #include "decl_event_drc_acu.h"
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drc_crm.h"
    #include "decl_event_drc.h"
    #include "decl_event_drc_banma.h"
    #include "decl_event_drsu.h"
    #include "decl_event_crm.h"
    #include "decl_event_pl.h"
    "LAST_EVENT"
};
#endif


#include "event.h"
#include "pl.h"


#undef DECL_EVENT
#undef DECL_BEGIN 
#define DECL_EVENT(x,y) 0xFFFFFFFF
#define DECL_BEGIN(x,y,z) (z/EVENT_SEG_LEN)

U32 gaulEventSegMap[256]={0};
U32 gaulEventSegLen[256]={0};
U32 gulEventNum = 0;
void init_event_map()
{
#if DRC_PROCESS
    U32 aulArr[]={
    #include "decl_event_drc_acu.h"
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drc_crm_r4.h"
    #include "decl_event_drc.h"
	#include "decl_event_drc_banma.h"
    #include "decl_event_pl.h"
    };
#elif DRSU_PROCESS
    U32 aulArr[]={
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drsu.h"
    #include "decl_event_pl.h"
    };
#elif CLOUD_PROCESS
    U32 aulArr[]={
    #include "decl_event_cloud.h"
    #include "decl_event_drc_crm_r4.h"
    #include "decl_event_drc.h"
    #include "decl_event_pl.h"
    };
#else
    U32 aulArr[]={
    #include "decl_event_drc_acu.h"
    #include "decl_event_drc_drsu.h"
    #include "decl_event_drc_crm.h"
    #include "decl_event_drc.h"
    #include "decl_event_drc_banma.h"
    #include "decl_event_drsu.h"
    #include "decl_event_crm.h"
    #include "decl_event_pl.h"
    };
#endif

    U32 ulArrId = 0;
    gulEventNum = sizeof(aulArr)/sizeof(aulArr[0]);
    
//    pf_memset(aulArrFlag, 0, sizeof(aulArrFlag));
    ASSERT(0xFFFFFFFF != aulArr[0]);

    for(U32 cnt=0; cnt<gulEventNum; cnt++)
    {
        if(aulArr[cnt] != 0xFFFFFFFF)
        {
            ulArrId = aulArr[cnt];
            if(ulArrId > 255)
            {
                ASSERT(0);
                return;
            }
                
            gaulEventSegMap[ulArrId] = cnt;
            gaulEventSegLen[ulArrId] = 1;
        }
        else
        {
            gaulEventSegLen[ulArrId]++;
        }

    }
}
extern "C" const char*pf_get_event_name(U32 ulEventId)
{
    if(0 == gulEventNum)
    {
        init_event_map();
    }

    U32 ulSeg = ulEventId/EVENT_SEG_LEN;
    U32 ulOffset = ulEventId%EVENT_SEG_LEN;

    if(ulSeg >= 256)
    {
        return "Unknown event";
    }

    if(ulOffset >= gaulEventSegLen[ulSeg])
    {
        return "Unknown event";
    }

    return event_name[gaulEventSegMap[ulSeg]+ulOffset];
}


extern "C" S32 pf_get_event_id(CHAR* pcEventName)
{
    if(0 == gulEventNum)
    {
        init_event_map();
    }
    
    U32 ulSeg=0;
    U32 ulOffset=0;

    for(ulSeg = 0; ulSeg < 256; ulSeg++)
    {
        for(ulOffset=0; ulOffset < gaulEventSegLen[ulSeg]; ulOffset++)
        {
            if(0 == strcmp(event_name[gaulEventSegMap[ulSeg]+ulOffset] , pcEventName ))
            {
                return ulSeg*EVENT_SEG_LEN + ulOffset;
            }
        }
    }

    return PF_RET_FAILURE;
}








