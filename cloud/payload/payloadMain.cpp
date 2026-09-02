#define THIS_MODULE MODULE_PAYLOAD

/* Includes ------------------------------------------------------------------*/
#include "payloadMain.h"
#include "moduleConstants.h"

/* Private functions declaration ---------------------------------------------*/
PayloadMain::PayloadMain():
    BaseModule(),
    m_requestManager(std::make_unique<PayloadRequestManager>()),
    m_cameraController(std::make_unique<CameraController>(*m_requestManager)),
    m_gimbalController(std::make_unique<GimbalController>(*m_requestManager, *m_cameraController))
{
    getDispatcher()->registerHandler(PAYLOAD_RESULT_DATA_IND,
                    [this](const std::string& msg) { m_requestManager->handlePayloadResult(msg); });
    getDispatcher()->registerHandler(PAYLOAD_GIMBAL_DATA_IND,
                    [this](const std::string& msg) { m_gimbalController->handleGimbalControl(msg); });
    getDispatcher()->registerHandler(PAYLOAD_CAMERA_DATA_IND,
                    [this](const std::string& msg) { m_cameraController->handleCameraControl(msg); });
}

PayloadMain::~PayloadMain()
{
    stopResponsePool();
    stopThreadPool();
}

void PayloadMain::startResponsePool(size_t threadCount)
{
    m_responsePoolManager.start(threadCount);
    pl_log(INF, "响应线程池启动 | 线程数: %zu", threadCount);
}

void PayloadMain::stopResponsePool()
{
    m_responsePoolManager.stop();
    pl_log(INF, "响应线程池已停止");
}

void PayloadMain::postResponseTask(std::function<void()> task)
{
    m_responsePoolManager.postTask(std::move(task));
}

/**********************************************************************************************
* @function   : payload_init
**********************************************************************************************/
S32 payload_init(U32 ulModuleId)
{
    pl_log(INF, "payload init module ID is %d", ulModuleId);
    PayloadMain::getInstance().startThreadPool(ModuleConstants::ThreadPool::PAYLOAD_THREAD_COUNT);
    PayloadMain::getInstance().startResponsePool(ModuleConstants::ThreadPool::PAYLOAD_RESPONSE_THREAD_COUNT);
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
        // drc/up 回执走独立响应线程池：请求发送线程(下面的postTask)会阻塞在
        // sendRequestAndWait()等回执，若跟回执处理用同一个池会死锁。
        case PAYLOAD_RESULT_DATA_IND: {
            PayloadMain::getInstance().postResponseTask([=]() {
                PayloadMain::getInstance().dispatchMessage(ulMsgId, msg);
            });
        }
        break;

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
