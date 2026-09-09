# dji_pilot2_cloud_service（骨架）

用于 DJI Pilot2 (RC Plus 2) 上云自定义接口开发，架构参考 `dji_dock3_cloud_service`（机场3云端服务）。

当前状态：`device`/`flyto`/`track`/`payload` 四个模块的业务逻辑均已实现并经过真机联调（详见
`docs/design/interface-porting-design.md`），进入收尾阶段——剩余问题见下方"待办"。

## 与机场3项目的关系

- `platform/`、`common/`、`drc/`、`build_scripts/`、`test/` 是从 `dji_dock3_cloud_service` **原样复制**过来的
  通用框架代码（module/event 分发框架、pf_ 平台函数、MQTT 客户端、proto/json 互转工具、第三方库），
  两边各自独立演化，以后框架层的改动需要分别同步。
- `cloud/` 是本项目自己重新搭的业务模块目录，**没有整体复制**机场3的 `cloud/`：
  - 保留：`root`（入口）、`lcf`（配置加载，原样复制）、`mqtt`（MQTT收发，做了精简）、
    `device`、`flyto`、`track`、`payload`（这四个是全新实现，业务逻辑已完成）
  - 删除：`waypoint`（机场3的KMZ航线任务模块）——因为 Pilot2 官方接口没有开放云端可管理航线任务的能力，
    详见迁移分析文档，不在本项目职责范围内

## 各模块职责（对照迁移文档）

| 模块 | 职责 | 对应机场3 |
|---|---|---|
| `cloud/device` | 遥控器(网关)属性 + 飞行器属性解析（osd/state通道） | 机场3 DeviceMain 的属性解析部分（不含舱盖/开关机/远程调试，Pilot2不支持） |
| `cloud/flyto` | DRC链路管理(drc_mode_enter/exit/heart_beat) + 权限抢占(cloud_control_auth_*) + 飞行控制(takeoff_to_point/fly_to_point*/return_home*/stick_control) | 机场3 FlytoMain |
| `cloud/track` | 飞行器OSD遥测转发给内部平台 | 机场3 TrackMain |
| `cloud/payload` | 云台/相机负载控制（drc_gimbal_reset / drc_camera_screen_drag / drc_camera_aim / drc_camera_frame_zoom / drc_camera_focal_length_set） | 机场3 PayloadMain 的云台/相机部分（不含探照灯、喊话器，本次不迁移） |
| `cloud/mqtt` | MQTT收发与method分发，topic规则与机场3一致（services/services_reply/events/requests/drc/up/osd/state） | 机场3 mqttsubMain/mqttpubMain，已按上面的范围裁剪 `m_topicHandlers` |
| `cloud/lcf` | 配置加载 | 原样复制，暂未按Pilot2调整config schema |

## 功能范围依据

三份分析文档（在 `dji_dock3_cloud_service/docs/pilot2_migration/` 下，本项目未复制）：
1. 《机场3官方接口清单》—— 机场3当前实际用到的官方接口
2. 《Pilot2(RC Plus 2)官方接口清单》—— Pilot2官方开放的接口全貌
3. 《机场3-Pilot2接口迁移对照表》—— 逐功能标注"一样/有变化/不支持"，是本骨架取舍的依据

## 待办

四个业务模块已实现完成并经过真机联调（详见 `docs/design/interface-porting-design.md` 第7/8节），剩余待办：

- [ ] **恢复 `control_source` 云端控制权校验**：`cloud/flyto/flytoController.cpp`、
      `cloud/payload/gimbalController.cpp`、`cloud/payload/cameraController.cpp` 里的
      `isCloudControlActive()` 检查目前被临时注释——真机测试发现 broker 不推送
      `thing/product/{aircraft_sn}/state` topic（DJI/broker 侧问题，非本项目代码问题），
      导致该检查永远拿不到值。等 broker 侧问题解决后需取消注释恢复三处检查。
- [ ] `fly_to_point_stop`/`return_home_cancel` 未实现：内部平台目前没有触发这两个方法的 action 码，
      待其余部分稳定后再补充
- [ ] `drc_camera_screen_drag` 的 `locked` 写死 `true`（官方示例为 `false`）、相机变焦上限写死 112
      （官方范围可见光为 2~200），两处沿用机场3原有取值，是否需要调整待确认
- [ ] `common/proto/bxt_cloud_common.proto` 里 `dock_sn` 字段的语义需要按Pilot2重新梳理（遥控器SN替代机场SN），
      目前 `cloud/mqtt/mqttsubMain.cpp` 里仍暂用这个字段名，已标 TODO
- [ ] 未纳入 SVN，确定要长期维护时再决定入库路径
