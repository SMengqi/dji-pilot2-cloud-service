#define THIS_MODULE MODULE_PAYLOAD

#include "cameraController.h"
#include "pl_utils.h"
#include "trackMain.h"
#include "jsonUtil.h"

#include <string>
#include <chrono>
#include <thread>
#include <algorithm>

CameraController::CameraController(PayloadRequestManager& requestManager):
    m_typeHandler{
        {static_cast<int>(Type::ZOOM_OUT), [this](){ cameraZoomOut(); }},
        {static_cast<int>(Type::ZOOM_IN),  [this](){ cameraZoomIn(); }},
        {static_cast<int>(Type::ZOOM_MIN), [this](){ cameraZoomMin(); }},
        {static_cast<int>(Type::ZOOM_MAX), [this](){ cameraZoomMax(); }},
        {static_cast<int>(Type::ZOOM_IN_CONTINUE), [this](){ cameraZoomInContinue(); }},
        {static_cast<int>(Type::ZOOM_OUT_CONTINUE), [this](){ cameraZoomOutContinue(); }},
        {static_cast<int>(Type::ZOOM_STOP), [this](){ cameraZoomStop(); }}
    },
    m_currentZoomFactor(2.0f), // 默认缩放因子，实际应从当前状态获取
    m_requestManager(requestManager)
{}

/**
 * @brief 封装 相机变焦 (method: drc_camera_focal_length_set)
 *
 * payload_index每次都直接从TrackMain重新获取，不依赖类成员缓存——specifyZoomFator()
 * 由GimbalController的"看的清"组合功能(ANGLE_ZOOM/AIM_ZOOM)直接调用时，并不会经过
 * handleCameraControl()这个入口，之前m_payloadIndex只在handleCameraControl()里被赋值，
 * 导致走specifyZoomFator()这条路径时m_payloadIndex一直是空字符串、兜底成默认值"99-0-0"，
 * 跟真实负载对不上，被设备拒收(实测result:327010)。
 */
void CameraController::commonZoomDataPackage(dji_cloud::drc_up_down& message)
{
    dji_cloud::drc_data* p_data = message.mutable_data();

    message.set_method("drc_camera_focal_length_set");

    m_payloadIndex = TrackMain::getInstance().getPayloadIndex();
    if (m_payloadIndex.empty()) { m_payloadIndex = "99-0-0"; }
    p_data->set_camera_type("zoom");
    p_data->set_payload_index(m_payloadIndex);
    p_data->set_zoom_factor(m_currentZoomFactor);
}

void CameraController::controlResult(bool& ret, std::string& log, std::string& errResult)
{
    uint16_t code = STATE_CAMERA_ZOOM;
    std::string data = log + "成功";
    int result = 0;
    if (!ret) {
        code = STATE_CAMERA_ZOOM_FAILED;
        data = log + "失败" + errResult;
        result = 1;
        pl_log(ERR, "code: %d, %s", code, data.c_str());
    }

    handleUavResult(code, data, result);
}

/**
 * @brief 发送drc/down请求并等待drc/up回执（按method名匹配，见payloadRequestManager.h说明）
 *
 * seq仍照常设置，但不用它做请求-应答匹配——★已用真实抓包核实（2026-09-02）：
 * 部分method(如drc_camera_aim)的请求和回执都完全没有seq字段，用seq匹配必然失效。
 */
bool CameraController::sendRequest(dji_cloud::drc_up_down &message, std::string &log, bool is_up_result)
{
    message.set_seq(s_drcHeartbeatCount.fetch_add(1));
    std::string methodKey = message.method();
    uint32_t msgId = static_cast<uint32_t>(DJI_DRC_DOWN_PUBLISH_DATA_IND);
    std::string errResult;
    bool ret = m_requestManager.sendRequestAndWait(message, methodKey, msgId, log, errResult);
    if (is_up_result)
    {
        controlResult(ret, log, errResult);
    }

    return ret;
}

void CameraController::specifyZoomFator(float& zoomFactor)
{
    m_currentZoomFactor = zoomFactor;
    dji_cloud::drc_up_down message;
    commonZoomDataPackage(message);

    std::string log = "相机焦距设置为: " + std::to_string(zoomFactor);
    // is_up_result默认true——这一步（"看的清"组合动作里的变焦部分）现在会单独上报成功/失败，
    // 不再被静默吞掉（之前用false会导致瞄准成功但变焦失败时，内部平台完全收不到变焦失败的结果）。
    sendRequest(message, log);
}


bool CameraController::cameraZoomOut()
{
    // 缩小逻辑实现
    if (m_currentZoomFactor <= m_zoomFactorMin)
    {
        std::string data = "当前已是最小焦距,无法继续缩小.";
        pl_log(ERR, "%s", data.c_str());
        uint16_t code = static_cast<uint16_t>(STATE_CAMERA_ZOOM_FAILED);
        handleUavResult(code, data, 1);
        return false;
    }
    float step = std::max(0.1f, m_currentZoomFactor * 0.2f);
    m_currentZoomFactor = std::max(m_currentZoomFactor - step, m_zoomFactorMin);

    dji_cloud::drc_up_down message;
    commonZoomDataPackage(message);

    std::string log = "相机焦距缩小";
    sendRequest(message, log);

    return true;
}

bool CameraController::cameraZoomIn()
{
    // 放大逻辑实现
    if (m_currentZoomFactor >= m_zoomFactorMax) {
        std::string data = "当前已是最大焦距,无法继续放大.";
        pl_log(ERR, "%s", data.c_str());
        uint16_t code = static_cast<uint16_t>(STATE_CAMERA_ZOOM_FAILED);
        handleUavResult(code, data, 1);
        return false;
    }
    float step = std::max(0.1f, m_currentZoomFactor * 0.2f);
    m_currentZoomFactor = std::min(m_currentZoomFactor + step, m_zoomFactorMax);

    dji_cloud::drc_up_down message;
    commonZoomDataPackage(message);

    std::string log = "相机焦距放大";
    sendRequest(message, log);

    return true;
}
bool CameraController::cameraZoomMin()
{
    m_currentZoomFactor = m_zoomFactorMin;

    dji_cloud::drc_up_down message;
    commonZoomDataPackage(message);

    std::string log = "相机焦距缩小到最小值";
    sendRequest(message, log);

    return true;
}
bool CameraController::cameraZoomMax()
{
    m_currentZoomFactor = m_zoomFactorMax;

    dji_cloud::drc_up_down message;
    commonZoomDataPackage(message);

    std::string log = "相机焦距放大到最大值";
    sendRequest(message, log);

    return true;
}

void CameraController::continuousZoom(bool isZoomIn, int durationMs, int intervalMs)
{
    pl_log(INF, "************ %s start ************", __FUNCTION__);
    auto startTime = std::chrono::steady_clock::now();
    std::string directionStr = isZoomIn ? "放大" : "缩小";
    const int maxIterations = 20;
    bool isUpResult = true;
    int iterations = 0;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        // 检查停止条件和超时
        if (elapsed >= durationMs || m_stopZoom) {
            break;
        }

        float currentZoomFactor;
        {
            std::lock_guard<std::mutex> zoomLock(m_zoomFactorMutex);
            currentZoomFactor = m_currentZoomFactor;
        }

        if ((isZoomIn && currentZoomFactor >= m_zoomFactorMax) ||
            (!isZoomIn && currentZoomFactor <= m_zoomFactorMin)) {
            std::string data = std::string("已到达") + (isZoomIn ? "最大" : "最小") + "焦距";
            pl_log(INF, "%s", data.c_str());
            uint16_t code = static_cast<uint16_t>(STATE_CAMERA_ZOOM_FAILED);
            handleUavResult(code, data, 1);

            // 确保边界状态被更新
            {
                std::lock_guard<std::mutex> zoomLock(m_zoomFactorMutex);
                if (isZoomIn) {
                    m_currentZoomFactor = m_zoomFactorMax;
                } else {
                    m_currentZoomFactor = m_zoomFactorMin;
                }
            }
            break;
        }

        float step = std::max(0.1f, currentZoomFactor * 0.1f);
        float newZoomFactor = currentZoomFactor + (isZoomIn ? step : -step);

        // 更新共享变量
        {
            std::lock_guard<std::mutex> zoomLock(m_zoomFactorMutex);
            if (isZoomIn) {
                m_currentZoomFactor = std::min(newZoomFactor, m_zoomFactorMax);
            } else {
                m_currentZoomFactor = std::max(newZoomFactor, m_zoomFactorMin);
            }
        }

        dji_cloud::drc_up_down message;
        commonZoomDataPackage(message);
        std::string log = "相机焦距持续" + directionStr;
        if (iterations != 0) { isUpResult = false; }

        bool ret = sendRequest(message, log, isUpResult);
        if (!ret) { break; }

        // 控制间隔时间
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));

        if (++iterations >= maxIterations) {
            pl_log(WARN, "持续操作达到最大循环次数:%d,停止执行", maxIterations);
            break;
        }
    }
    pl_log(INF, "************ %s END ************", __FUNCTION__);

}

void CameraController::startZoomOperation(bool isZoomIn)
{
    // 立即设置停止标志，确保之前的操作能够尽快退出
    m_stopZoom = true;

    // 获取操作锁，确保同一时间只有一个持续变焦操作能够执行
    std::lock_guard<std::mutex> lock(m_operationMutex);

    // 清除停止标志，准备开始新操作
    m_stopZoom = false;

    // 执行实际的变焦操作
    continuousZoom(isZoomIn, 5000, 200);
}

bool CameraController::cameraZoomInContinue()
{
    startZoomOperation(true);
    return true;
}

bool CameraController::cameraZoomOutContinue()
{
    startZoomOperation(false);
    return true;
}

bool CameraController::cameraZoomStop()
{
    // 原子操作设置停止标志
    m_stopZoom = true;
    std::string data = "停止持续变焦";
    pl_log(INF, "%s", data.c_str());
    uint16_t code = static_cast<uint16_t>(STATE_CAMERA_ZOOM_FAILED);
    handleUavResult(code, data, 0);
    return true;
}

void CameraController::handleCameraControl(const std::string& msg)
{
    dji_cloud::camera_control_message camera_msg;
    if (!json_to_proto(msg, camera_msg)) {
        std::string data = "解析相机控制消息失败";
        pl_log(ERR, "%s | 原始数据: %.*s", data.c_str(), static_cast<int>(msg.size()), msg.data());
        uint16_t code = static_cast<uint16_t>(STATE_CAMERA_ZOOM_FAILED);
        handleUavResult(code, data, 1);
        return;
    }
    updateTransId(camera_msg.trans_id());

    pl_log(INF, "收到相机控制消息 | 类型: %d", camera_msg.camera_zoom_direction());

    m_currentZoomFactor = TrackMain::getInstance().getZoomFactor();
    if (m_currentZoomFactor < 1.0f) {
        std::string data = "获取当前焦距因子异常(小于1.0)，无法进行相机变焦控制操作";
        pl_log(ERR, "%s", data.c_str());
        uint16_t code = static_cast<uint16_t>(STATE_CAMERA_ZOOM_FAILED);
        handleUavResult(code, data, 1);
        return;
    }

    auto it = m_typeHandler.find(camera_msg.camera_zoom_direction());
    if (it != m_typeHandler.end()) {
        it->second();
    } else {
        std::string data = "未知的相机控制类型: " + std::to_string(camera_msg.camera_zoom_direction());
        pl_log(WARN, "%s", data.c_str());
        handleUavResult(static_cast<uint16_t>(STATE_CAMERA_ZOOM_FAILED), data, 1);
    }
}
