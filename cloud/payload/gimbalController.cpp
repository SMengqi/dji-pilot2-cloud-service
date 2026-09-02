#define THIS_MODULE MODULE_PAYLOAD

#include "gimbalController.h"

#include "pl.h"
#include "json2pb.h"
#include "jsonUtil.h"
#include "pl_utils.h"
#include "trackMain.h"

#include <thread>
#include <unordered_set>

GimbalController::GimbalController(PayloadRequestManager& requestManager, CameraController& cameraController):
    m_requestManager(requestManager),
    m_cameraController(cameraController),
    m_GimbalControlOffset(2.0f),
    m_typeHandlerMap{
        {static_cast<int>(GimbalController::GimbalControlType::ANGLE),
                                    [this](const dji_cloud::gimbal_control_message& msg) {handleCameraScreenDrag(msg);}},
        {static_cast<int>(GimbalController::GimbalControlType::CENTER),
                                    [this](const dji_cloud::gimbal_control_message& msg) {handleCameraAim(msg);}},
        {static_cast<int>(GimbalController::GimbalControlType::RECTANGLE),
                                    [this](const dji_cloud::gimbal_control_message& msg) {handleCameraFrameZoom(msg);}},
        {static_cast<int>(GimbalController::GimbalControlType::ANGLE_ZOOM),
                                    [this](const dji_cloud::gimbal_control_message& msg) {handleCameraAngleZoom(msg);}},
        {static_cast<int>(GimbalController::GimbalControlType::AIM_ZOOM),
                                    [this](const dji_cloud::gimbal_control_message& msg) {handleCameraAimZoom(msg);}}
    }
{}

/**
 * @brief 发送drc/down请求并等待drc/up回执（按seq匹配，见payloadRequestManager.h说明）
 */
bool GimbalController::sendRequest(dji_cloud::drc_up_down& message, const std::string& log, std::string& errResult)
{
    uint32_t currentSeq = s_drcHeartbeatCount.fetch_add(1);
    message.set_seq(currentSeq);
    std::string seqStr = std::to_string(currentSeq);
    uint32_t msgId = static_cast<uint32_t>(DJI_DRC_DOWN_PUBLISH_DATA_IND);
    return m_requestManager.sendRequestAndWait(message, seqStr, msgId, log, errResult);
}

void GimbalController::sendResult(bool ret, const std::string& log, const std::string& errResult)
{
    uint16_t code = STATE_GIMBAL_CONTROL;
    std::string data = log + "成功";
    int result = 0;
    if (!ret) {
        code = STATE_GIMBAL_CONTROL_FAILED;
        data = log + "失败" + errResult;
        result = 1;
        pl_log(ERR, "code: %d, %s", code, data.c_str());
    }

    handleUavResult(code, data, result);
}

bool GimbalController::sendRequestAndResult(dji_cloud::drc_up_down& message, const std::string& log)
{
    std::string errResult;
    bool ret = sendRequest(message, log, errResult);

    sendResult(ret, log, errResult);
    return ret;
}

/**
 * @brief 处理 云台重置 (method: drc_gimbal_reset)
 */
void GimbalController::gimbalReset(void)
{
    dji_cloud::drc_up_down message;
    dji_cloud::drc_data* p_data = message.mutable_data();

    message.set_method("drc_gimbal_reset");
    message.set_timestamp(get_milliseconds());

    p_data->set_payload_index(m_payloadIndex);
    p_data->set_reset_mode(2);

    std::string log = "云台重置";
    sendRequestAndResult(message, log);

    return;
}

GimbalControlOffset GimbalController::getGimbalAngleOffset(const int& direction)
{
    switch (direction) {
        case static_cast<int>(GimbalController::GimbalControlDirection::UP): {
            return GimbalControlOffset{0.0f, m_GimbalControlOffset};
        }
        case static_cast<int>(GimbalController::GimbalControlDirection::DOWN): {
            return GimbalControlOffset{0.0f, -m_GimbalControlOffset};
        }
        case static_cast<int>(GimbalController::GimbalControlDirection::LEFT): {
            return GimbalControlOffset{-m_GimbalControlOffset, 0.0f};
        }
        case static_cast<int>(GimbalController::GimbalControlDirection::RIGHT):{
            return GimbalControlOffset{m_GimbalControlOffset, 0.0f};
        }
        default:
            return GimbalControlOffset{0.0f, 0.0f};
    }
}

/**
 * @brief 封装 云台拖拽 (method: drc_camera_screen_drag)
 */
void GimbalController::gimbalAngleDataPackage(const GimbalControlOffset& offset,
                                              dji_cloud::drc_up_down& message)
{
    dji_cloud::drc_data* p_data = message.mutable_data();

    message.set_method("drc_camera_screen_drag");
    message.set_timestamp(get_milliseconds());

    p_data->set_payload_index(m_payloadIndex);
    p_data->set_locked(true);
    p_data->set_pitch_speed(offset.pitch);
    p_data->set_yaw_speed(offset.yaw);
}

/**
 * @brief 处理 云台拖拽控制
 * @param msg JSON格式的响应消息
 * @return void
 */
void GimbalController::handleCameraScreenDrag(const dji_cloud::gimbal_control_message& msg)
{
    int direction = msg.gimbal_direction();
    if (direction == static_cast<int>(GimbalController::GimbalControlDirection::RESET)) {
        pl_log(INF, "云台重置指令");
        gimbalReset();
        return;
    }

    const std::unordered_set<int> valid_actions = {4,5,7};
    if (valid_actions.count(direction)) {
        pl_log(ERR, "暂不支持的云台转向指令 | direction: %d", direction);
        return;
    }

    dji_cloud::drc_up_down message;
    GimbalControlOffset offset = getGimbalAngleOffset(direction);
    gimbalAngleDataPackage(offset, message);

    std::string log = "云台转向";
    sendRequestAndResult(message, log);
}

/**
 * @brief 处理 云台中心点控制 (method: drc_camera_aim)
 * @param msg JSON格式的响应消息
 * @return void
 */
bool GimbalController::handleCameraAim(const dji_cloud::gimbal_control_message& msg)
{
    dji_cloud::drc_up_down message;
    dji_cloud::drc_data* p_data = message.mutable_data();

    message.set_method("drc_camera_aim");
    message.set_timestamp(get_milliseconds());

    // camera_type透传内部平台下发的字段，未下发时按机场3原有默认值"wide"回退
    // （Pilot2多支持一个"ir"取值，见设计文档3.4节）
    p_data->set_camera_type(msg.has_camera_type() ? msg.camera_type() : "wide");
    p_data->set_locked(false);
    p_data->set_payload_index(m_payloadIndex);
    p_data->set_x(msg.gimbal_centerx());
    p_data->set_y(msg.gimbal_centery());

    std::string log = "点控云台";

    return sendRequestAndResult(message, log);
}

/**
 * @brief 处理 云台框选变焦 (method: drc_camera_frame_zoom)
 * @param msg JSON格式的响应消息
 * @return void
 */
void GimbalController::handleCameraFrameZoom(const dji_cloud::gimbal_control_message& msg)
{
    dji_cloud::drc_up_down message;
    dji_cloud::drc_data* p_data = message.mutable_data();

    message.set_method("drc_camera_frame_zoom");
    message.set_timestamp(get_milliseconds());

    p_data->set_payload_index(m_payloadIndex);
    // camera_type透传内部平台下发的字段，未下发时按机场3原有默认值"zoom"回退
    p_data->set_camera_type(msg.has_camera_type() ? msg.camera_type() : "zoom");
    p_data->set_locked(true);
    p_data->set_x(msg.gimbal_centerx() - msg.gimbal_width() / 2);
    p_data->set_y(msg.gimbal_centery() - msg.gimbal_height() / 2);
    p_data->set_width(msg.gimbal_width());
    p_data->set_height(msg.gimbal_height());

    std::string log = "框选变焦";
    sendRequestAndResult(message, log);
    return;
}

/**
 * @brief 处理 云台转向+变焦 (看得清功能)
 * @param msg JSON格式的响应消息
 * @return void
*/
void GimbalController::handleCameraAngleZoom(const dji_cloud::gimbal_control_message& msg)
{
    dji_cloud::drc_up_down message;
    GimbalControlOffset offset = { msg.yaw(), -msg.pitch() };
    gimbalAngleDataPackage(offset, message);

    bool ret = true;
    std::string errResult;
    std::string log = "云台转向";

    for (int i = 0; i < 4; i++) {
        ret = sendRequest(message, log, errResult);
        if (!ret) {
            pl_log(ERR, "云台转动失败,不做变焦操作");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    sendResult(ret, log, errResult);

    if (ret) {
        float zoomFactor = msg.zoom_factor();
        m_cameraController.specifyZoomFator(zoomFactor);
    }
}

/**
 * @brief 处理 云台点控+变焦 (看得清功能)
 * @param msg JSON格式的响应消息
 * @return void
*/
void GimbalController::handleCameraAimZoom(const dji_cloud::gimbal_control_message& msg)
{
    if (handleCameraAim(msg)) {
        float zoomFactor = msg.zoom_factor();
        m_cameraController.specifyZoomFator(zoomFactor);
    }
}

/**
 * @brief 处理 云台控制指令分发
 * @param msg JSON格式的响应消息
 * @return void
 */
void GimbalController::handleGimbalControl(const std::string& msg)
{
    dji_cloud::gimbal_control_message gimbal_msg;
    if (!json_to_proto(msg, gimbal_msg)) {
        pl_log(ERR, "解析云台控制消息失败 | 原始数据: %.*s", static_cast<int>(msg.size()), msg.data());
        return;
    }

    updateTransId(gimbal_msg.trans_id());

    m_payloadIndex = TrackMain::getInstance().getPayloadIndex();
    if (m_payloadIndex.empty()) { m_payloadIndex = "99-0-0"; }

    pl_log(INF, "收到云台控制消息 | 类型: %d", gimbal_msg.type());
    auto it = m_typeHandlerMap.find(gimbal_msg.type());
    if (it != m_typeHandlerMap.end()) {
        it->second(gimbal_msg);
    } else {
        pl_log(WARN, "未知的云台控制类型: %d", gimbal_msg.type());
    }
}
