#define THIS_MODULE MODULE_FLYTO

#include "flytoRequestManager.h"

// drc_mode_enter/drc_mode_exit/cloud_control_auth_*（DRC链路管理/权限抢占）不归本模块发送，
// 由控制平台另一组实现，故不在结果描述表里列出（见设计文档第1节范围收窄说明）。
FlytoRequestManager::FlytoRequestManager() :
    m_resultMap{
        {"takeoff_to_point",      "起飞响应"},
        {"return_home",           "返航响应"},
        {"fly_to_point",          "飞向目标点响应"},
        {"fly_to_point_update",   "更新飞向目标点响应"}
    }
{}

/**
 * @brief 处理请求的结果
 *
 * 解析请求的JSON结果消息，更新对应的请求上下文，并记录相应日志。
 *
 * @param msg 包含请求结果的JSON字符串消息
 */
void FlytoRequestManager::handleFlytoResult(const std::string& msg)
{
    std::string jsonStr(msg);
    std::string method;
    std::string tid;
    int result = parseResult(jsonStr, method, tid);

    // 查找对应的描述信息，如果不存在则使用method本身
    auto it = m_resultMap.find(method);
    std::string data = (it != m_resultMap.end()) ? it->second : method;

    pl_log(INF, "收到%s响应 | result: %d | data:%s", method.c_str(), result, data.c_str());

    // 在 pendingRequests 中查找对应请求
    std::shared_ptr<RequestContext> reqctx;
    bool ret = findAndUpdateRequest(tid, result, reqctx);
    if (!ret) {
        pl_log(WARN, "未找到对应的请求 | tid=%s ", tid.c_str());
        return;
    }

    if (result == 0) {
        pl_log(TRC, "%s : 成功 | result: %d", data.c_str(), result);
    } else {
        pl_log(ERR, "%s : 失败 | result: %d, 原始数据: %.*s",
               data.c_str(), result, static_cast<int>(jsonStr.size()), jsonStr.data());
    }
}
