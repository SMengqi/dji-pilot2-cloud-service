#define THIS_MODULE MODULE_WAYPOINT

#include "baseModule.h"
#include "pl_utils.h"

BaseModule::BaseModule()
    : m_threadPoolManager()
    , m_dispatcher(std::make_unique<MessageDispatcher>())
{
}

void BaseModule::startThreadPool(size_t threadCount)
{
    m_threadPoolManager.start(threadCount);
}

void BaseModule::stopThreadPool()
{
    m_threadPoolManager.stop();
}

void BaseModule::postTask(std::function<void()> task)
{
    m_threadPoolManager.postTask(std::move(task));
}

void BaseModule::dispatchMessage(uint32_t msgId, const std::string& msg)
{
    try {
        m_dispatcher->dispatch(msgId, msg);
    } catch (const std::exception& e) {
        pl_log(ERR, "消息分发异常 | msgId: %u(0x%x) | 错误: %s", msgId, msgId, e.what());
    } catch (...) {
        pl_log(ERR, "消息分发未知异常 | msgId: %u(0x%x)", msgId, msgId);
    }
}
