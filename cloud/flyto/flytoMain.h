#ifndef FLYTOMAIN_H_
#define FLYTOMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include <string>

#include "baseModule.h"

/**
 * @brief 飞行控制模块（Pilot2骨架）
 *
 * 对应《机场3-Pilot2接口迁移对照表》里"一样"和"有变化"的三大块：
 * - 二、DRC链路管理：drc_mode_enter / drc_mode_exit / heart_beat（services + drc/down，字段与机场3一致）
 * - 三、权限抢占：cloud_control_auth_request / cloud_control_release / cloud_control_auth_notify
 *   （services + events，取代机场3声明未用的 payload_authority_grab / flight_authority_grab）
 * - 四、飞行控制：takeoff_to_point / fly_to_point / fly_to_point_update / fly_to_point_stop /
 *   return_home / return_home_cancel / stick_control（字段与机场3基本一致，参见清单文档）
 *
 * 当前为纯骨架：只保留模块注册框架，具体的请求管理器/控制器待实现。
 */
class FlytoMain : public BaseModule
{
public:
    static FlytoMain& getInstance() {
        static FlytoMain instance;
        return instance;
    }

    FlytoMain(const FlytoMain&) = delete;
    FlytoMain& operator=(const FlytoMain&) = delete;

private:
    FlytoMain();
    ~FlytoMain();

    // TODO: std::unique_ptr<FlytoRequestManager> m_requestManager; services请求/应答匹配(tid)
    // TODO: std::unique_ptr<FlytoController> m_flytoController;    DRC链路管理/权限抢占/飞行控制/杆量控制实现
};

#endif /*FLYTOMAIN_H_*/
