#ifndef GIMBAL_CONTROLLER_H_
#define GIMBAL_CONTROLLER_H_

#include <functional>
#include <unordered_map>
#include <string>

#include "payloadRequestManager.h"
#include "cameraController.h"

#include "dji_cloud_api.pb.h"

struct GimbalControlOffset {
    float yaw;
    float pitch;
};

/**
 * @brief 云台控制（Pilot2）
 *
 * 对应设计文档3.4节：gimbal_reset/camera_screen_drag/camera_aim/camera_frame_zoom
 * 四个方法（机场3是services，Pilot2改成drc_gimbal_reset/drc_camera_screen_drag/
 * drc_camera_aim/drc_camera_frame_zoom，走drc/down/drc/up）。内部平台接入的type分发
 * （ANGLE/CENTER/RECTANGLE/ANGLE_ZOOM/AIM_ZOOM）跟机场3完全一致，原样保留。
 *
 * camera_type（wide/zoom/ir）改成透传internal gimbal_control_message.camera_type字段，
 * 不再像机场3那样写死"wide"/"zoom"（Pilot2的drc_camera_aim/drc_camera_frame_zoom多支持
 * 一个"ir"取值，见设计文档3.4节）；内部平台未下发时按机场3原有默认值回退。
 */
class GimbalController {
public:
    GimbalController(PayloadRequestManager& requestManager, CameraController& cameraController);
    ~GimbalController() = default;

    void handleGimbalControl(const std::string& msg);

private:
    bool sendRequest(dji_cloud::drc_up_down& message, const std::string& log, std::string& errResult);
    void sendResult(bool ret, const std::string& log, const std::string& errResult);
    bool sendRequestAndResult(dji_cloud::drc_up_down& message, const std::string& log);

    void gimbalReset(void);
    GimbalControlOffset getGimbalAngleOffset(const int& direction);
    void gimbalAngleDataPackage(const GimbalControlOffset& offset, dji_cloud::drc_up_down& message);
    // 云台拖拽转动
    void handleCameraScreenDrag(const dji_cloud::gimbal_control_message& msg);
    // 云台点控转动
    bool handleCameraAim(const dji_cloud::gimbal_control_message& msg);
    // 云台框选变焦
    void handleCameraFrameZoom(const dji_cloud::gimbal_control_message& msg);
    // 云台角度转动与变焦 (看的清功能)
    void handleCameraAngleZoom(const dji_cloud::gimbal_control_message& msg);
    // 云台点控 + 变焦 (看的清功能)
    void handleCameraAimZoom(const dji_cloud::gimbal_control_message& msg);
private:
    CameraController& m_cameraController;

    std::unordered_map<int, std::function<void(const dji_cloud::gimbal_control_message&)>> m_typeHandlerMap;
    PayloadRequestManager& m_requestManager;

    // 云台控制偏移量 degree/s
    const float m_GimbalControlOffset;

    std::string m_payloadIndex;

private:
    enum class GimbalControlDirection {
        UP = 0,
        DOWN,
        LEFT,
        RIGHT,
        FLIP_LEFT,
        FLIP_RIGHT,
        RESET,
        STOP
    };
    enum class GimbalControlType {
        ANGLE = 1000,
        CENTER,
        RECTANGLE,
        ANGLE_ZOOM,
        AIM_ZOOM
    };

};


#endif /*** GIMBAL_CONTROLLER_H_ ***/
