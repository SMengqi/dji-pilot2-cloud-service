#define THIS_MODULE MODULE_PAYLOAD

/* Includes ------------------------------------------------------------------*/
#include "payloadMain.h"
#include "moduleConstants.h"

/* Private functions declaration ---------------------------------------------*/
PayloadMain::PayloadMain():
    BaseModule()
{
    // TODO: 注册消息处理器，参考机场3 PayloadMain 构造函数，例如：
    // getDispatcher()->registerHandler(PAYLOAD_RESULT_DATA_IND,
    //                 [this](const std::string& msg) { m_requestManager->handlePayloadResult(msg); });
    // getDispatcher()->registerHandler(PAYLOAD_GIMBAL_DATA_IND,
    //                 [this](const std::string& msg) { m_gimbalController->handleGimbalControl(msg); });
    // getDispatcher()->registerHandler(PAYLOAD_CAMERA_DATA_IND,
    //                 [this](const std::string& msg) { m_cameraController->handleCameraControl(msg); });
}

PayloadMain::~PayloadMain()
{
    stopThreadPool();
}

/**********************************************************************************************
* @function   : payload_init
**********************************************************************************************/
S32 payload_init(U32 ulModuleId)
{
    pl_log(INF, "payload init module ID is %d", ulModuleId);
    PayloadMain::getInstance().startThreadPool(ModuleConstants::ThreadPool::PAYLOAD_THREAD_COUNT);
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : payload_entry
**********************************************************************************************/
void payload_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId,
                               void* pcvMsg, U32 ulLength)
{
    std::string msg(static_cast<const char*>(pcvMsg), ulLength);
    switch (ulMsgId)
    {
        case PAYLOAD_RESULT_DATA_IND:
        case COMMON_REG_IND:
        case PAYLOAD_GIMBAL_DATA_IND:
        case PAYLOAD_CAMERA_DATA_IND: {
            PayloadMain::getInstance().postTask([=]() {
                PayloadMain::getInstance().dispatchMessage(ulMsgId, msg);
            });
        }
        break;

        default: { pl_log(ERR, "unexpected msg id %d(0x%x)", ulMsgId, ulMsgId); } break;
    }
}
