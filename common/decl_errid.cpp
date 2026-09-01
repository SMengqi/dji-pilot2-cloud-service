
#include "option.h"
#include "pl_type.h"

#define THIS_MODULE
#define DECL_ERRID(x) #x
#define DECL_ERRID_BEGIN(x,y) #x


const char *errid_name[] = 
{
    #include "decl_errid.h"
    "LAST_ERRID"
};

#include "errid.h"
#include "pl.h"


#undef DECL_ERRID
#undef DECL_ERRID_BEGIN 
#define DECL_ERRID(x) 0xFFFFFFFF
#define DECL_ERRID_BEGIN(x,y) (y/ERRID_SEG_LEN)

U32 gaulErrSegMap[256]={0};
U32 gaulErrSegLen[256]={0};
U32 gulErrNum = 0;
void init_errid_map()
{
    U32 aulArr[]={
    #include "decl_errid.h"
    };
    U32 aulArrFlag[256];
    U32 ulArrId = 0;
    gulErrNum = sizeof(aulArr)/sizeof(aulArr[0]);
    
    pf_memset(aulArrFlag, 0, sizeof(aulArrFlag));
    ASSERT(0xFFFFFFFF != aulArr[0]);

    for(U32 cnt=0; cnt<gulErrNum; cnt++)
    {
        if(aulArr[cnt] != 0xFFFFFFFF)
        {
            ulArrId = aulArr[cnt];
            if(ulArrId > 255)
            {
                ASSERT(0);
                return;
            }
                
            gaulErrSegMap[ulArrId] = cnt;
            gaulErrSegLen[ulArrId] = 1;
        }
        else
        {
            gaulErrSegLen[ulArrId]++;
        }

    }
}
const char*pf_get_errid_name(U32 ulErrId)
{
    if(0 == gulErrNum)
    {
        init_errid_map();
    }

    U32 ulSeg = ulErrId/ERRID_SEG_LEN;
    U32 ulOffset = ulErrId%ERRID_SEG_LEN;

    if(ulSeg >= 256)
    {
        return "Unknown errid";
    }

    if(ulOffset >= gaulErrSegLen[ulSeg])
    {
        return "Unknown errid";
    }

    return errid_name[gaulErrSegMap[ulSeg]+ulOffset];
}

const S32 pf_get_errid_id(CHAR* pcErridName)
{
    if(0 == gulErrNum)
    {
        init_errid_map();
    }
    
    U32 ulSeg=0;
    U32 ulOffset=0;

    for(ulSeg = 0; ulSeg < 256; ulSeg++)
    {
        for(ulOffset=0; ulOffset < gaulErrSegLen[ulSeg]; ulOffset++)
        {
            if(0 == strcmp(errid_name[gaulErrSegMap[ulSeg]+ulOffset] , pcErridName ))
            {
                return ulSeg*ERRID_SEG_LEN + ulOffset;
            }
        }
    }

    return PF_RET_FAILURE;
}



