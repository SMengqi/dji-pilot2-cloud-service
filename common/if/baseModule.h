#ifndef BASE_MODULE_H_
#define BASE_MODULE_H_

#include "messageDispatcher.h"
#include "ThreadPoolManager.h"
#include "pl.h"

#include <string>
#include <functional>
#include <memory>

/**
 * @brief 模块基类
 * 
 * 提供所有业务模块的公共功能：
 * - 线程池管理
 * - 消息分发
 * - 异常处理
 * 
 * 使用方式：
 * @code
 * class MyModule : public BaseModule {
 * public:
 *     static MyModule& getInstance() {
 *         static MyModule instance;
 *         return instance;
 *     }
 * private:
 *     MyModule() : BaseModule() {
 *         // 注册消息处理器
 *         getDispatcher()->registerHandler(MSG_ID, [this](const std::string& msg) {
 *             // 处理消息
 *         });
 *     }
 * };
 * @endcode
 */
class BaseModule {
public:
    BaseModule();
    virtual ~BaseModule() = default;

    // 禁止拷贝构造和赋值
    BaseModule(const BaseModule&) = delete;
    BaseModule& operator=(const BaseModule&) = delete;

    /**
     * @brief 启动线程池
     * @param threadCount 线程数量
     */
    void startThreadPool(size_t threadCount);

    /**
     * @brief 停止线程池
     */
    void stopThreadPool();

    /**
     * @brief 提交任务到线程池
     * @param task 要执行的任务
     */
    void postTask(std::function<void()> task);

    /**
     * @brief 分发消息到注册的处理器
     * 
     * 该方法会自动处理异常，确保消息分发失败不会导致程序崩溃
     * 
     * @param msgId 消息ID
     * @param msg 消息内容
     */
    void dispatchMessage(uint32_t msgId, const std::string& msg);

    /**
     * @brief 获取消息分发器
     * @return MessageDispatcher的引用
     */
    MessageDispatcher* getDispatcher() { return m_dispatcher.get(); }

protected:
    /**
     * @brief 线程池管理器
     */
    ThreadPoolManager m_threadPoolManager;

    /**
     * @brief 消息分发器
     */
    std::unique_ptr<MessageDispatcher> m_dispatcher;
};

#endif /* BASE_MODULE_H_ */
