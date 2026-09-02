#ifndef PAYLOAD_REQUEST_MANAGER_H_
#define PAYLOAD_REQUEST_MANAGER_H_

#include "requestContextManager.h"
#include <unordered_map>

/**
 * Pilot2的5个负载方法(drc_gimbal_reset/drc_camera_screen_drag/drc_camera_aim/
 * drc_camera_frame_zoom/drc_camera_focal_length_set)全部走drc/down/drc/up，
 * 跟机场3(services/services_reply)不同，回执解析用parseDrcResult()（按seq匹配，
 * 而不是parseResult()按tid匹配——drc_up_down没有tid字段，用parseResult()解析
 * 永远匹配不到、请求必然超时，见requestContextManager.h的说明）。
 */
class PayloadRequestManager : public RequestContextManager {
public:
    PayloadRequestManager();
    ~PayloadRequestManager() = default;

    void handlePayloadResult(const std::string& msg);

private:
    std::unordered_map<std::string, std::string> m_resultMap;
};

#endif  // PAYLOAD_REQUEST_MANAGER_H_
