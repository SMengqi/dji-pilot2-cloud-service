#ifndef DEVICEMAIN_H_
#define DEVICEMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include "baseModule.h"
#include "rcProperties.h"

#include <cstdint>
#include <memory>

/**
 * @brief 设备属性模块（Pilot2）
 *
 * 对应《Pilot2(RC Plus 2)官方接口清单》第九节"设备属性"：
 * - 遥控器(网关)属性：live_capacity / capacity_percent / wireless_link / cloud_control_auth / drc_state 等
 * - 飞行器属性：mode_code / mode_code_reason / control_source / battery / type_subtype_gimbalindex 等
 * 均走 osd(0.5Hz定频) / state(变化时) 两个通道，具体见清单文档。
 *
 * 机场3体系里本模块承担的"舱盖开关(cover_open/close) / 开关机(drone_open/close) / 远程调试
 * (debug_mode_open/close)"能力，在 Pilot2 官方接口里没有对应能力，不属于本模块职责范围
 * （见《机场3-Pilot2接口迁移对照表》第一节"设备管理"）。
 *
 * 目前只实现遥控器属性解析（RcProperties/getDrcState()）；飞行器属性解析待实现。
 */
class DeviceMain : public BaseModule {
public:
    static DeviceMain& getInstance() {
        static DeviceMain instance;
        return instance;
    }

    DeviceMain(const DeviceMain&) = delete;
    void operator=(const DeviceMain&) = delete;

    // 供 flyto/payload 模块判断"是否已建立DRC链路"，语义/取值范围与机场3 DeviceMain::getDrcState()一致
    uint16_t getDrcState();

    // TODO: 对外暴露 getControlSource() 等，供 flyto/payload 判断"云端是否持有控制权"
    //       （见设计文档第6节待确认事项1）。

private:
    DeviceMain();
    ~DeviceMain();

    std::unique_ptr<RcProperties> m_rcProperties;
    // TODO: std::unique_ptr<AircraftProperties> m_aircraftProperties; 飞行器属性解析器
};

#endif /*DEVICEMAIN_H_*/
