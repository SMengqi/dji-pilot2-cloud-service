#define THIS_MODULE MODULE_WAYPOINT

#include "requestContextManager.h"
#include "jsonUtil.h"

RequestContextManager::RequestContextManager()
    :m_commonResultMap{
        {"flighttask_prepare",        "下发任务响应"},
        {"flighttask_execute",        "执行任务响应"},
        {"flighttask_pause",          "暂停任务响应"},
        {"flighttask_recovery",       "恢复任务响应"},
        {"flighttask_undo",           "取消任务响应"},
        {"in_flight_wayline_cancel",  "取消空中任务响应"},
        {"in_flight_wayline_stop",    "暂停空中任务响应"},
        {"in_flight_wayline_recover", "恢复空中任务响应"},
        {"in_flight_wayline_deliver", "空中下发航线"}
    }
{}

/**
 * @brief 查找并更新请求上下文
 *
 * 根据事务ID查找对应的请求上下文，更新其结果状态并通知等待线程
 *
 * @param tid 事务ID
 * @param result 请求处理结果
 * @param[out] outCtx 输出参数，存储找到的请求上下文
 * @return true 如果找到并成功更新请求上下文
 * @return false 如果未找到对应的请求上下文
 */
bool RequestContextManager::findAndUpdateRequest(
    const std::string& tid, int result,
    std::shared_ptr<RequestContext>& outCtx)
{
    std::lock_guard<std::mutex> lock(m_requestMutex);
    auto it = m_pendingRequests.find(tid);
    if (it == m_pendingRequests.end()) {
        return false;
    }

    outCtx = it->second;
    {
        std::lock_guard<std::mutex> ctxLock(outCtx->mtx);
        outCtx->result = result;
        outCtx->finished = true;
    }

    pl_log(TRC, "找到对应的请求并更新结果 | tid=%s ", tid.c_str());
    m_pendingRequests.erase(it);
    outCtx->cv.notify_one();
    return true;
}


/**
 * @brief 地面任务准备2\4 - 订阅任务就绪通知\任务资源获取请求
 * #param tid （这里使用的是flight_id)
 * @param log 日志信息
 * @return true 响应\资源获取等待成功 false 失败
 */
bool RequestContextManager::waitForDockNotice(std::string tid, std::string& log, 
                                                             const int& timeoutMs)
{
    pl_log(INF, "等待%s, tid = %s", log.c_str(), tid.c_str());

    auto req_ctx = std::make_shared<RequestContext>();
    req_ctx->tid = tid;
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_pendingRequests[tid] = req_ctx;
    }
    std::unique_lock<std::mutex> lock2(req_ctx->mtx);
    bool finished = req_ctx->cv.wait_for(lock2, std::chrono::milliseconds(timeoutMs),
                                         [&]{ return req_ctx->finished; });

    if (!finished)
    {
        // 超时，从 pending 中移除请求
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_pendingRequests.erase(tid);
        pl_log(ERR, "等待%s超时 | flight_id=%s", log.c_str(), tid.c_str());
        return false;
    }
    return true;
}


/**
 * @brief 解析响应消息
 * @param [in] msg 响应消息
 * @param [out] method 方法名
 * @param [out] tid 请求 ID
 * @return 0 成功 -1 失败
 */
int RequestContextManager::parseResult(const std::string &msg, std::string &method, std::string &tid)
{
    dji_cloud::services_reply message;
    if (!json_to_proto(msg, message))
    {
        pl_log(ERR, "JSON to proto 转换失败 | 原始数据: %.*s",
               static_cast<int>(msg.size()), msg.data());
        return -1;
    }

    dji_cloud::reply_data *p_data = message.mutable_data();
    if (!p_data)
    {
        pl_log(ERR, "Protobuf data字段为空 | 原始数据: %.*s",
               static_cast<int>(msg.size()), msg.data());
        return -1;
    }

    method = message.method();
    tid = message.tid();
    return p_data->result();
}

/**
 * @brief 解析 drc/up 回执消息（drc_up_down 格式，没有tid/bid，用seq顺序对应）
 * @param [in] msg drc/up 回执的原始 JSON 字符串
 * @param [out] method 方法名
 * @param [out] seq seq的字符串形式，需与sendRequestAndWait()发起请求时传入的tid参数一致
 * @return 0 成功 -1 失败
 */
int RequestContextManager::parseDrcResult(const std::string &msg, std::string &method, std::string &seq)
{
    dji_cloud::drc_up_down message;
    if (!json_to_proto(msg, message))
    {
        pl_log(ERR, "JSON to proto 转换失败(drc_up_down) | 原始数据: %.*s",
               static_cast<int>(msg.size()), msg.data());
        return -1;
    }

    const dji_cloud::drc_data& data = message.data();

    method = message.method();
    seq = std::to_string(message.seq());
    return data.result();
}

/**
 * @brief 处理公共的响应消息
 * @param  msg 响应消息
 * @return void
 */
void RequestContextManager::handleCommonResult(const std::string& msg)
{
    std::string method;
    std::string tid;
    int result = parseResult(msg, method, tid);
    std::string data = m_commonResultMap[method];
    
    pl_log(INF, "收到%s响应 | result: %d", method.c_str(), result);
    // 在 pendingRequests 中查找对应请求
    std::shared_ptr<RequestContext> req_ctx;
    if (!findAndUpdateRequest(tid, result, req_ctx))
    {
        return;
    }

    if (result == 0)
    {
        pl_log(INF, "%s:成功 | result: %d", data.c_str(), result);
    }
    else
    {
        pl_log(ERR, "%s:失败 | result: %d, 原始数据: %.*s",
               data.c_str(), result, static_cast<int>(msg.size()), msg.data());
    }
}
