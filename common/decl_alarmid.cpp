
#include "option.h"
#include "pl_type.h"

#define THIS_MODULE
#define DECL_ALARMID(x) #x
#define DECL_ALARMID_BEGIN(x,y) #x


const char *alarmid_name[] = 
{
    #include "decl_alarmid.h"
    "LAST_ALARMID"
};

#include "alarmid.h"
#include "pl.h"


#undef DECL_ALARMID
#undef DECL_ALARMID_BEGIN 
#define DECL_ALARMID(x) 0xFFFFFFFF
#define DECL_ALARMID_BEGIN(x,y) (y/ALARMID_SEG_LEN)

U32 gaulAlarmSegMap[ALARMID_SEG_LEN]={0};
U32 gaulAlarmSegLen[ALARMID_SEG_LEN]={0};
U32 gulAlarmNum = 0;
void init_alarmid_map()
{
    U32 aulAlarm[]={
    #include "decl_alarmid.h"
    };
    U32 aulAlarmFlag[ALARMID_SEG_LEN];
    U32 ulAlarmId = 0;
    gulAlarmNum = sizeof(aulAlarm)/sizeof(aulAlarm[0]);
    
    pf_memset(aulAlarmFlag, 0, sizeof(aulAlarmFlag));
    ASSERT(0xFFFFFFFF != aulAlarm[0]);

    for(U32 cnt=0; cnt<gulAlarmNum; cnt++)
    {
        if(aulAlarm[cnt] != 0xFFFFFFFF)
        {
            ulAlarmId = aulAlarm[cnt];
            if(ulAlarmId > 255)
            {
                ASSERT(0);
                return;
            }
                
            gaulAlarmSegMap[ulAlarmId] = cnt;
            gaulAlarmSegLen[ulAlarmId] = 1;
        }
        else
        {
            gaulAlarmSegLen[ulAlarmId]++;
        }

    }
}
const char*pf_get_alarmid_name(U32 ulAlarmId)
{
    if(0 == gulAlarmNum)
    {
        init_alarmid_map();
    }

    U32 ulSeg = ulAlarmId/ALARMID_SEG_LEN;
    U32 ulOffset = ulAlarmId%ALARMID_SEG_LEN;

    if(ulSeg >= ALARMID_SEG_LEN)
    {
        return "Unknown alarmid";
    }

    if(ulOffset >= gaulAlarmSegLen[ulSeg])
    {
        return "Unknown alarmid";
    }

    return alarmid_name[gaulAlarmSegMap[ulSeg]+ulOffset];
}

const S32 pf_get_alarmid_id(CHAR* pcErridName)
{
    if(0 == gulAlarmNum)
    {
        init_alarmid_map();
    }
    
    U32 ulSeg=0;
    U32 ulOffset=0;

    for(ulSeg = 0; ulSeg < ALARMID_SEG_LEN; ulSeg++)
    {
        for(ulOffset=0; ulOffset < gaulAlarmSegLen[ulSeg]; ulOffset++)
        {
            if(0 == strcmp(alarmid_name[gaulAlarmSegMap[ulSeg]+ulOffset] , pcErridName ))
            {
                return ulSeg*ALARMID_SEG_LEN + ulOffset;
            }
        }
    }

    return PF_RET_FAILURE;
}



