#ifndef PAYLOADMAIN_H_
#define PAYLOADMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include "baseModule.h"
#include "payloadRequestManager.h"
#include "gimbalController.h"
#include "cameraController.h"
#include "ThreadPoolManager.h"

#include <memory>

/**
 * @brief 负载控制模块（Pilot2）
 *
 * 对应《机场3-Pilot2接口迁移对照表》第五节"负载控制（云台/相机）"：
 * gimbal_reset→drc_gimbal_reset、camera_screen_drag→drc_camera_screen_drag、
 * camera_aim→drc_camera_aim、camera_frame_zoom→drc_camera_frame_zoom、
 * camera_focal_length_set→drc_camera_focal_length_set。
 * 字段名/取值枚举与机场3一致，差异仅在通道：机场3走 services(tid/bid匹配)，
 * Pilot2走 drc/down(seq顺序，回执只有{result})，详见清单文档。
 *
 * 机场3的探照灯(drc_light_*)、喊话器(speaker_control)不在本次迁移范围内，未包含在本模块里。
 *
 * 使用独立响应线程池处理drc/up回执，避免请求发送线程（阻塞等待sendRequestAndWait）
 * 和回执处理线程是同一个池导致死锁，跟机场3PayloadMain的设计一致。
 */
class PayloadMain : public BaseModule {
public:
    static PayloadMain& getInstance() {
        static PayloadMain instance;
        return instance;
    }

    PayloadMain(const PayloadMain&) = delete;
    void operator=(const PayloadMain&) = delete;

    void startResponsePool(size_t threadCount);
    void stopResponsePool();
    void postResponseTask(std::function<void()> task);

private:
    PayloadMain();
    ~PayloadMain();

    std::unique_ptr<PayloadRequestManager> m_requestManager;
    std::unique_ptr<CameraController> m_cameraController;
    std::unique_ptr<GimbalController> m_gimbalController;

    ThreadPoolManager m_responsePoolManager;
};

#endif /*PAYLOADMAIN_H_*/
