#ifndef REQUEST_CONTEXT_MANAGER_H_
#define REQUEST_CONTEXT_MANAGER_H_

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <iostream>
#include <condition_variable>

#include "pl.h"

#include "dji_cloud_api.pb.h"
#include "json2pb.h"
#include "moduleConstants.h"

/**
 * @brief 请求上下文结构体
 * 
 * 用于管理异步请求-响应模式中的等待上下文
 */
struct RequestContext {
    std::string tid;                    ///< 事务ID
    bool finished = false;              ///< 是否完成标志
    int result = -1;                   ///< 请求结果（0=成功，非0=失败）
    std::condition_variable cv;         ///< 条件变量，用于等待响应
    std::mutex mtx;                     ///< 互斥锁，保护finished和result字段
};

/**
 * @brief 请求上下文管理器
 * 
 * 管理所有待响应的请求上下文，提供请求-响应同步机制。
 * 支持超时控制和结果解析。
 */
class RequestContextManager {
public:

    /**
     * @brief 构造函数
     * 
     * 初始化通用结果映射表
     */
    RequestContextManager();

    /**
     * @brief 查找并更新请求上下文
     * 
     * 根据事务ID查找对应的请求上下文，更新其结果状态并通知等待线程
     * 
     * @param tid 事务ID
     * @param result 请求处理结果（0=成功，非0=失败）
     * @param[out] outCtx 输出参数，存储找到的请求上下文
     * @return true 如果找到并成功更新请求上下文
     * @return false 如果未找到对应的请求上下文
     */
    bool findAndUpdateRequest(const std::string& tid, int result, std::shared_ptr<RequestContext>& outCtx);

    /**
     * @brief 发送请求并等待响应
     * 
     * 模板方法，支持任意Protobuf消息类型。执行流程：
     * 1. 创建请求上下文并注册
     * 2. 序列化消息并发送
     * 3. 等待响应（带超时）
     * 4. 解析结果并返回
     * 
     * @tparam MessageType Protobuf消息类型
     * @param message 要发送的消息（会被修改，添加tid等字段）
     * @param tid 事务ID（输入输出参数）
     * @param msgId 消息ID（用于发送）
     * @param log 日志描述信息
     * @param errResult 错误结果描述（输出参数）
     * @param timeoutMs 超时时间（毫秒），默认8秒
     * @return true 请求成功并收到成功响应
     * @return false 请求失败、超时或收到失败响应
     */
    template<typename MessageType>
    bool sendRequestAndWait(
        MessageType& message, std::string& tid,
        uint32_t& msgId, const std::string& log,
        std::string &errResult, int timeoutMs = ModuleConstants::Timeout::DEFAULT_REQUEST_TIMEOUT)
    {
        auto reqCtx = std::make_shared<RequestContext>();
        reqCtx->tid = tid;

        // 注册等待
        {
            std::lock_guard<std::mutex> lock(m_requestMutex);
            m_pendingRequests[tid] = reqCtx;
        }

        // 发送前清理 lambda
        auto cleanup = [this, &tid]()
        {
            std::lock_guard<std::mutex> lock(m_requestMutex);
            m_pendingRequests.erase(tid);
        };

        std::string jsonStr;
        jsonStr = pb2json(message);
        if (jsonStr.empty())
        {
            cleanup();
            pl_log(ERR, "pb2json failed, msg type: %s", log.c_str());
            errResult = "(pb2json failed)";
            return false;
        }
        pl_log(INF, "发布:%s | data=%.*s", 
               log.c_str(), static_cast<int>(jsonStr.size()), jsonStr.data());

        pf_copy_msg(THIS_MODULE, msgId, MODULE_MQTTPUB, (void *)jsonStr.c_str(), jsonStr.size());

        // 等待响应
        std::unique_lock<std::mutex> lock(reqCtx->mtx);
        bool finished = reqCtx->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                            [&]{ return reqCtx->finished; });

        if (!finished)
        {
            // 超时，从 pending 中移除请求
            cleanup();
            pl_log(ERR, "发布%s超时 | tid=%s", log.c_str(), tid.c_str());
            errResult = "(超时)";
            return false;
        }

        if (reqCtx->result != 0) { 
            errResult = "(" + std::to_string(reqCtx->result) + ")"; 
            pl_log(ERR, "发布%s失败 | result=%d", log.c_str(), reqCtx->result);
            return false;
        }

        return reqCtx->result == 0;
    }
    
    /**
     * @brief 等待机场通知
     * 
     * 用于等待机场发送的通知消息（如任务就绪、资源获取等）
     * 
     * @param tid 事务ID（这里使用flight_id）
     * @param log 日志描述信息
     * @param timeoutMs 超时时间（毫秒）
     * @return true 成功收到通知
     * @return false 超时或失败
     */
    bool waitForDockNotice(std::string tid, std::string& log, const int& timeoutMs);

    /**
     * @brief 处理通用结果消息
     * 
     * 解析响应消息并更新对应的请求上下文
     * 
     * @param msg JSON格式的响应消息
     */
    void handleCommonResult(const std::string& msg);

    /**
     * @brief 解析结果消息
     * 
     * 从JSON响应消息中提取method、tid和result
     * 
     * @param msg JSON格式的响应消息
     * @param[out] method 方法名（输出参数）
     * @param[out] tid 事务ID（输出参数）
     * @return int 结果码（0=成功，非0=失败），-1表示解析失败
     */
    int parseResult(const std::string& msg, std::string& method, std::string& tid);

    /**
     * @brief 解析 drc/up 回执消息（drc_up_down 格式：{seq, method, data:{result}, timestamp}）
     *
     * 与 parseResult() 的区别：drc/up 回执没有 tid/bid 字段，请求-应答用 seq 顺序对应
     * （见接口迁移设计文档第4节）。用 sendRequestAndWait() 发起drc/down请求时若把
     * seq的字符串形式当tid传入，收到回执后应调用本方法解析、而不是parseResult()——
     * 后者按services_reply解析，drc_up_down里没有tid字段，会一直解析出空字符串，
     * 导致findAndUpdateRequest()永远匹配不到、请求必然超时。
     *
     * @param msg drc/up 回执的原始 JSON 字符串
     * @param[out] method 方法名（输出参数）
     * @param[out] seq seq的字符串形式（输出参数），需与发起请求时sendRequestAndWait()的tid参数一致
     * @return int 结果码（0=成功，非0=失败），-1表示解析失败
     */
    int parseDrcResult(const std::string& msg, std::string& method, std::string& seq);

private:
    // 当前等待中的请求
    std::unordered_map<std::string, std::shared_ptr<RequestContext>> m_pendingRequests;
    mutable std::mutex m_requestMutex; // 保护 m_pendingRequests
    // 航线控制通用接口响应容器
    std::unordered_map<std::string, std::string> m_commonResultMap;
};

#endif // REQUEST_CONTEXT_MANAGER_H_
