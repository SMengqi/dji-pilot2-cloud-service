#ifndef PAYLOADMAIN_H_
#define PAYLOADMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include "baseModule.h"

/**
 * @brief 负载控制模块（Pilot2骨架）
 *
 * 对应《机场3-Pilot2接口迁移对照表》第五节"负载控制（云台/相机）"：
 * gimbal_reset→drc_gimbal_reset、camera_screen_drag→drc_camera_screen_drag、
 * camera_aim→drc_camera_aim、camera_frame_zoom→drc_camera_frame_zoom、
 * camera_focal_length_set→drc_camera_focal_length_set。
 * 字段名/取值枚举与机场3一致，差异仅在通道：机场3走 services(tid/bid匹配)，
 * Pilot2走 drc/down(seq顺序，回执只有{result})，详见清单文档。
 *
 * 机场3的探照灯(drc_light_*)、喊话器(speaker_control)不在本次迁移范围内，未包含在骨架里。
 *
 * 当前为纯骨架：只保留模块注册框架，具体的云台/相机控制器待实现。
 */
class PayloadMain : public BaseModule {
public:
    static PayloadMain& getInstance() {
        static PayloadMain instance;
        return instance;
    }

    PayloadMain(const PayloadMain&) = delete;
    void operator=(const PayloadMain&) = delete;

private:
    PayloadMain();
    ~PayloadMain();

    // TODO: std::unique_ptr<PayloadRequestManager> m_requestManager; DRC回执(result)匹配
    // TODO: std::unique_ptr<GimbalController> m_gimbalController;    云台重置/拖拽/点控/框选变焦
    // TODO: std::unique_ptr<CameraController> m_cameraController;   相机变焦
};

#endif /*PAYLOADMAIN_H_*/
