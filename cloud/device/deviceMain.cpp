#define THIS_MODULE MODULE_DEVICE

/* Includes ------------------------------------------------------------------*/
#include "deviceMain.h"
#include "moduleConstants.h"

#include <string>

/* Private functions declaration ---------------------------------------------*/
DeviceMain::DeviceMain() :
    BaseModule(),
    m_rcProperties(std::make_unique<RcProperties>())
{
    getDispatcher()->registerHandler(COMMON_REG_IND, [this](const std::string& msg) {
        // 处理遥控器(网关)属性；mqttsubMain.cpp 里 thing/product/{rc_sn}/osd 已路由到本模块
        m_rcProperties->parseRcOsdData(msg);
    });
}

DeviceMain::~DeviceMain()
{
    stopThreadPool();
}

uint16_t DeviceMain::getDrcState()
{
    return m_rcProperties->getDrcState();
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
