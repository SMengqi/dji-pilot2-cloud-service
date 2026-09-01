#define THIS_MODULE MODULE_FLYTO

/* Includes ------------------------------------------------------------------*/
#include "flytoMain.h"
#include "moduleConstants.h"

/* Private functions declaration ---------------------------------------------*/
FlytoMain::FlytoMain() :
    BaseModule()
{
    // TODO: 注册消息处理器，参考机场3 FlytoMain 构造函数，例如：
    // getDispatcher()->registerHandler(FLYTO_RESULT_DATA_IND,
    //             [this](const std::string& msg) { m_requestManager->handleFlytoResult(msg); });
    // getDispatcher()->registerHandler(COMMON_REG_IND,
    //             [this](const std::string& msg) { m_flytoController->handleFlightControl(msg); });
}

FlytoMain::~FlytoMain()
{
    stopThreadPool();
}

/**********************************************************************************************
* @function   : flyto_init
**********************************************************************************************/
S32 flyto_init(U32 ulModuleId)
{
    pl_log(INF, "flyto init module ID is %d", ulModuleId);
    FlytoMain::getInstance().startThreadPool(ModuleConstants::ThreadPool::FLYTO_THREAD_COUNT);
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : flyto_entry
**********************************************************************************************/
void flyto_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId,
                               void* pcvMsg, U32 ulLength)
{
    std::string msg(static_cast<const char*>(pcvMsg), ulLength);
    switch (ulMsgId)
    {
        case FLYTO_RESULT_DATA_IND:
        case FLYTO_RESULT_HEARTBEAT_DATA_IND:
        case FLYTO_DRC_OSD_DATA_IND:
        case FLYTO_PROGRESS_DATA_IND:
        case COMMON_REG_IND: {
            FlytoMain::getInstance().postTask([ulMsgId, msg]() {
                FlytoMain::getInstance().dispatchMessage(ulMsgId, msg);
            });
        }
        break;

        default: { pl_log(ERR, "unexpected msg id %d(0x%x)", ulMsgId, ulMsgId); } break;
    }
}
