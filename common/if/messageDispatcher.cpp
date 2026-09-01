#define THIS_MODULE MODULE_WAYPOINT

#include "messageDispatcher.h"
#include "pl.h"

void MessageDispatcher::registerHandler(uint32_t msgId, HandleFunc handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[msgId] = std::move(handler);
}

/**
 * @brief 分发消息到对应的处理函数
 * 
 * @param msgId 消息ID，用于查找对应的处理函数
 * @param msg 消息内容字符串
 * 
 * @note 线程安全，使用互斥锁保护处理函数映射表的访问
 * @warning 如果找不到对应msgId的处理函数，会记录错误日志并直接返回
 */
void MessageDispatcher::dispatch(uint32_t msgId, const std::string& msg) {
    HandleFunc handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(msgId);
        if (it == m_handlers.end()) {
            pl_log(ERR, "No handler registered for msgId: %u (0x%x)", msgId, msgId);
            return;
        }
        handler = it->second;
    }
    handler(msg);
}
