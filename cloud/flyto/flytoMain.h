#ifndef FLYTOMAIN_H_
#define FLYTOMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include <string>
#include <memory>

#include "baseModule.h"
#include "flytoRequestManager.h"
#include "flytoController.h"

/**
 * @brief 飞行控制模块（Pilot2）
 *
 * 对应设计文档3.3节：takeoff_to_point / fly_to_point / fly_to_point_update / return_home /
 * stick_control（字段与机场3基本一致），以及内部平台接入的MOVE/TURN/CONTINUOUS_MOVE/STOP_MOVE
 * action码。DRC链路管理(drc_mode_enter/exit/heart_beat)和权限抢占(cloud_control_auth_*)
 * 不在本项目范围，由控制平台另一组实现（见设计文档第1节范围收窄说明），本模块不涉及。
 *
 * fly_to_point_stop/return_home_cancel（机场3没有的新方法）暂不实现，留到其它部分完成后再补充。
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

    std::unique_ptr<FlytoRequestManager> m_requestManager; // services请求/应答匹配(tid)
    std::unique_ptr<FlytoController> m_flytoController;    // 飞行控制/杆量控制实现
};

#endif /*FLYTOMAIN_H_*/
