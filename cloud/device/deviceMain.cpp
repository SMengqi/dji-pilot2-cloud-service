#define THIS_MODULE MODULE_DEVICE

/* Includes ------------------------------------------------------------------*/
#include "deviceMain.h"
#include "moduleConstants.h"

#include <string>

/* Private functions declaration ---------------------------------------------*/
DeviceMain::DeviceMain() :
    BaseModule()
{
    // TODO: 注册属性上报消息处理器，参考机场3 DeviceMain 构造函数的写法，例如：
    // getDispatcher()->registerHandler(COMMON_REG_IND, [this](const std::string& msg) {
    //     m_rcProperties->parse(msg);         // 或按 topic 区分转给 m_aircraftProperties
    // });
}

DeviceMain::~DeviceMain()
{
    stopThreadPool();
}

/**********************************************************************************************
* @function   : device_init
**********************************************************************************************/
S32 device_init(U32 ulModuleId)
{
    pl_log(INF, "device init module ID is %d", ulModuleId);
    DeviceMain::getInstance().startThreadPool(ModuleConstants::ThreadPool::DEVICE_THREAD_COUNT);
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : device_entry
**********************************************************************************************/
void device_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId,
                               void* pcvMsg, U32 ulLength)
{
    std::string msg(static_cast<const char*>(pcvMsg), ulLength);
    switch (ulMsgId)
    {
        case COMMON_REG_IND: {
            DeviceMain::getInstance().postTask([=]() {
                DeviceMain::getInstance().dispatchMessage(ulMsgId, msg);
            });
        }
        break;

        default: { pl_log(ERR, "unexpected msg id %d(0x%x)", ulMsgId, ulMsgId); } break;
    }
}
