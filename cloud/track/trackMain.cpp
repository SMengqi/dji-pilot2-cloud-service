#define THIS_MODULE MODULE_TRACK

/* Includes ------------------------------------------------------------------*/
#include "trackMain.h"

/**********************************************************************************************
* @function   : track_init
**********************************************************************************************/
S32 track_init(U32 ulModuleId)
{
    pl_log(INF, "track init module ID is %d", ulModuleId);
    // TODO: 参考机场3 track_init，从配置里取遥控器/飞行器 SN 并保存
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : track_entry
**********************************************************************************************/
void track_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId,
                               void* pcvMsg, U32 ulLength)
{
    switch (ulMsgId)
    {
        case COMMON_REG_IND: {
            // TODO: TrackMain::getInstance().handleOsdMsg(static_cast<const char*>(pcvMsg));
        }
        break;

        default: { pl_log(ERR, "unexpected msg id %d(0x%x)", ulMsgId, ulMsgId); } break;
    }
}
