#ifndef TRACKMAIN_H_
#define TRACKMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include <string>
#include <atomic>

struct Point {
    double lon;
    double lat;
    double alt;
    double course;
};

/**
 * @brief 飞行器遥测转发模块（Pilot2）
 *
 * 对应设计文档3.2节：解析飞行器 thing/product/{aircraft_sn}/osd 属性消息，转发给内部平台。
 * 字段名与机场3飞行器OSD基本一致，直接照搬机场3 TrackMain 的解析/转发逻辑（parseOsdMsg/
 * packFlightInfo/packPayloadParam），内部平台Topic不变。
 *
 * cameras[] 已用真实抓包核实为字面量数组（非机场3早期推测的动态命名struct），结构与机场3一致，
 * 可直接沿用机场3原有的"先在cameras[]里找payload_index/zoom_factor"逻辑；gimbal_pitch/gimbal_yaw
 * 大概率也在同一个数组元素里（跟机场3"取payload_index做动态key二次查找"的写法不同），但抓包时
 * 飞行器均未挂载负载、数组内部字段无法验证，挂载负载后需重新抓包确认（见设计文档第6节待确认事项2）。
 */
class TrackMain {
public:
    static TrackMain& getInstance() {
        static TrackMain instance;
        return instance;
    }

    TrackMain(const TrackMain&) = delete;
    TrackMain& operator=(const TrackMain&) = delete;

    void setUavId(const std::string& uavId);
    void handleOsdMsg(const char* msg);
    Point getUavPoint();

    std::string getPayloadIndex();
    float getZoomFactor();
    // 飞机飞行模式码（来自飞行器属性OSD，-1 表示未知）：用于判断是否被外部接管（返航/降落等）
    int getUavModeCode();

private:
    TrackMain();
    ~TrackMain() = default;

    // 解析飞行器属性OSD消息
    bool parseOsdMsg(const std::string& msg);
    // 封装flightInfo
    void packFlightInfo();
    void packPayloadParam();

private:
    struct BatteryInfo {
        int32_t capacityPercent = 0;
        int32_t remainFlightTime = 0;
    };

    struct CameraInfo {
        std::string payloadIndex = "99-0-0";
        float zoomFactor = 0.0f;
    };

    struct GimbalInfo {
        double pitch = 0.0;
        double yaw = 0.0;
    };

    struct FlightInfo {
        std::string deviceId;
        double longitude = 0.0;
        double latitude = 0.0;
        double seaaltitude = 0.0;
        double earthaltitude = 0.0;
        double speed = 0.0;
        double course = 0.0;
        BatteryInfo battery;
        GimbalInfo gimbal;
        CameraInfo camera;
    };

private:
    std::string m_uavId;
    FlightInfo m_flightInfo;
    // 飞机飞行模式码（原子，跨线程读写安全；新成员追加末尾，避免改变已有成员偏移）
    std::atomic<int> m_uavModeCode{-1};
};

#endif /*TRACKMAIN_H_*/
