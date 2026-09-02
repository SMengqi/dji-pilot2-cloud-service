#define THIS_MODULE MODULE_PAYLOAD

#include "payloadRequestManager.h"
#include "pl.h"

PayloadRequestManager::PayloadRequestManager():
    m_resultMap{
        {"drc_gimbal_reset",            "云台重置"},
        {"drc_camera_screen_drag",      "云台转动"},
        {"drc_camera_aim",              "云台点控"},
        {"drc_camera_frame_zoom",       "框选变焦"},
        {"drc_camera_focal_length_set", "相机变焦"}
    }
{}

/**
 * @brief 处理 负载控制 响应消息（drc/up 回执，见 payloadRequestManager.h 说明）
 * @param msg JSON格式的响应消息
 * @return void
 */
void PayloadRequestManager::handlePayloadResult(const std::string& msg)
{
    std::string jsonStr(msg);
    std::string method;
    int result = parseDrcResult(jsonStr, method);

    auto it = m_resultMap.find(method);
    std::string data = (it != m_resultMap.end()) ? it->second : method;
    if (data.empty()) return;

    pl_log(INF, "收到%s响应 | result: %d | data:%s", method.c_str(), result, data.c_str());
    // 在 pendingRequests 中查找对应请求（key为发起请求时的method名，不是tid/seq——
    // 真实抓包证实seq并非所有method都会回显，按method名匹配才可靠，见requestContextManager.h说明）
    std::shared_ptr<RequestContext> reqCtx;
    if (!findAndUpdateRequest(method, result, reqCtx))
    {
        pl_log(WARN, "未找到对应的请求 | method=%s ", method.c_str());
        return;
    }

    if (result == 0) {
        pl_log(INF, "%s成功 | result: %d", data.c_str(), result);
    } else {
        pl_log(ERR, "%s失败 | result: %d, 原始数据: %.*s",
               data.c_str(), result, static_cast<int>(jsonStr.size()), jsonStr.data());
    }
}
