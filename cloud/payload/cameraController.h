#ifndef CAMERA_CONTROLLER_H_
#define CAMERA_CONTROLLER_H_

#include "payloadRequestManager.h"

#include <functional>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include "dji_cloud_api.pb.h"

/**
 * @brief 相机变焦控制（Pilot2）
 *
 * 对应设计文档3.4节：camera_focal_length_set → drc_camera_focal_length_set
 * （机场3是services，Pilot2改成走drc/down/drc/up）。内部平台接入的
 * ZOOM_OUT/ZOOM_IN/ZOOM_MIN/ZOOM_MAX/ZOOM_IN_CONTINUE/ZOOM_OUT_CONTINUE/ZOOM_STOP
 * 这套内部action分发跟机场3完全一致，原样保留。
 */
class CameraController {
public:

    CameraController(PayloadRequestManager& requestManager);

    void handleCameraControl(const std::string& msg);
    void specifyZoomFator(float& zoomFactor);
private:
    void commonZoomDataPackage(dji_cloud::drc_up_down& message);
    void controlResult(bool& ret, std::string& log, std::string& errResult);
    bool sendRequest(dji_cloud::drc_up_down& message, std::string& log, bool is_up_result = true);
    bool cameraZoomOut();
    bool cameraZoomIn();
    bool cameraZoomMin();
    bool cameraZoomMax();
    void continuousZoom(bool isZoomIn, int durationMs, int intervalMs);
    void startZoomOperation(bool isZoomIn);
    bool cameraZoomInContinue();
    bool cameraZoomOutContinue();
    bool cameraZoomStop();

private:
    std::unordered_map<int, std::function<void()>> m_typeHandler;
    PayloadRequestManager& m_requestManager;

    const float m_zoomFactorMin = 2.0f;
    const float m_zoomFactorMax = 112.0f;

    float m_currentZoomFactor;
    std::string m_payloadIndex;
    std::atomic<bool> m_stopZoom = false;
    std::mutex m_zoomFactorMutex;          // 保护m_currentZoomFactor的访问
    std::mutex m_operationMutex;           // 保护持续变焦操作的启动和停止

private:
    enum class Type{
        ZOOM_OUT = 0, // 缩小
        ZOOM_IN,      // 放大
        ZOOM_MIN,     // 缩到最小
        ZOOM_MAX,     // 缩到最大
        ZOOM_IN_CONTINUE,  // 持续放大
        ZOOM_OUT_CONTINUE, // 持续缩小
        ZOOM_STOP     // 停止持续放大或缩小
    };

};


#endif  // CAMERA_CONTROLLER_H_
