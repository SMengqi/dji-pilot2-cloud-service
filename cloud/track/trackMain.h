#ifndef TRACKMAIN_H_
#define TRACKMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include <string>

/**
 * @brief 飞行器遥测转发模块（Pilot2骨架）
 *
 * 对应机场3 TrackMain 的角色：接收飞行器 osd 属性消息，解析出经纬度/高度/速度/朝向/电量/
 * 云台角度/相机负载等字段，转发给内部控制平台（bxt/thing/product/osd/ 等）。
 * 字段名与机场3飞行器OSD基本一致，唯一结构性差异在 payload_index/zoom_factor/gimbal_pitch/
 * gimbal_yaw 这组字段（Pilot2下平铺在一个动态命名的 struct 里，机场3是 cameras数组+动态字段
 * 两段式），详见《Pilot2(RC Plus 2)官方接口清单》第九节。
 *
 * 当前为纯骨架：不走 BaseModule/线程池（机场3 TrackMain 同样是轻量单例+自由函数入口），
 * 具体的属性解析逻辑待实现。
 */
class TrackMain {
public:
    static TrackMain& getInstance() {
        static TrackMain instance;
        return instance;
    }

    TrackMain(const TrackMain&) = delete;
    TrackMain& operator=(const TrackMain&) = delete;

    // TODO: void handleOsdMsg(const char* msg);   解析飞行器osd属性消息
    // TODO: 对外暴露 payload_index / zoom_factor / mode_code 等查询接口

private:
    TrackMain() = default;
    ~TrackMain() = default;
};

#endif /*TRACKMAIN_H_*/
