#ifndef MESSAGE_DISPATCHER_H_
#define MESSAGE_DISPATCHER_H_

#include <unordered_map>
#include <functional>
#include <mutex>
#include <string>

/**
 * @brief 消息分发器
 * 
 * 提供线程安全的消息分发功能，支持注册消息ID到处理函数的映射。
 * 使用互斥锁保护内部数据结构，确保多线程环境下的安全性。
 * 
 * 使用示例：
 * @code
 * MessageDispatcher dispatcher;
 * dispatcher.registerHandler(MSG_ID, [](const std::string& msg) {
 *     // 处理消息
 * });
 * dispatcher.dispatch(MSG_ID, "消息内容");
 * @endcode
 */
class MessageDispatcher {
public:
    /**
     * @brief 消息处理函数类型
     */
    using HandleFunc = std::function<void(const std::string&)>;

    /**
     * @brief 注册消息处理器
     * 
     * 将消息ID与处理函数关联，后续调用dispatch时会自动调用对应的处理函数
     * 
     * @param msgId 消息ID
     * @param handler 处理函数
     */
    void registerHandler(uint32_t msgId, HandleFunc handler);

    /**
     * @brief 分发消息到对应的处理函数
     * 
     * 根据消息ID查找注册的处理函数并调用。如果未找到对应的处理函数，
     * 会记录错误日志并直接返回，不会抛出异常。
     * 
     * @param msgId 消息ID，用于查找对应的处理函数
     * @param msg 消息内容字符串
     * 
     * @note 线程安全，使用互斥锁保护处理函数映射表的访问
     */
    void dispatch(uint32_t msgId, const std::string& msg);

private:
    /**
     * @brief 消息ID到处理函数的映射表
     */
    std::unordered_map<uint32_t, HandleFunc> m_handlers;
    
    /**
     * @brief 保护m_handlers的互斥锁
     */
    mutable std::mutex m_mutex;
};

#endif // MESSAGE_DISPATCHER_H_
