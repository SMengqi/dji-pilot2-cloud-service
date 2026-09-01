# Pilot2 接口移植设计文档

> 依据：《机场3官方接口清单》《Pilot2(RC Plus 2)官方接口清单》《机场3-Pilot2接口迁移对照表》
> （三份文档在 `dji_dock3_cloud_service/docs/pilot2_migration/`，本项目未复制，设计以其结论为准）

## 1. 范围

按迁移对照表的结论，只移植"一样"和"有变化"的功能，"不支持"的不做：

| 状态 | 功能 | 落在哪个模块 |
|---|---|---|
| 一样 | DRC进入/退出、心跳 | `cloud/flyto` |
| 一样 | 一键起飞、飞向目标点/更新、一键返航、杆量控制 | `cloud/flyto` |
| 有变化 | 权限抢占（cloud_control_auth_*） | `cloud/flyto` |
| 有变化 | 云台重置/拖拽/点控/框选变焦/相机变焦（drc_前缀） | `cloud/payload` |
| 有变化 | DRC链路状态、飞行器基础遥测（数据源换了） | `cloud/device`、`cloud/track` |
| 不支持 | 机场舱盖/开关机/远程调试、航点上报 | 不实现 |

不在本次范围：探照灯、喊话器（机场3现有清单里已明确不迁移）。

## 2. 总体架构

沿用机场3的 module/event 框架（`BaseModule` + `MessageDispatcher`），沿用现有 Topic 规则：

- `services` / `services_reply`：DRC进退、权限抢占、一键起飞/返航/飞向目标点——**tid/bid 匹配应答**，跟机场3完全一样
- `drc/down` / `drc/up`：杆量控制、云台/相机负载指令——**seq 顺序对应，无 tid/bid**，回执只有 `{result}`
- `thing/product/{rc_sn}/osd`：遥控器属性（`drc_state` 等）→ 路由给 `cloud/device`
- `thing/product/{aircraft_sn}/osd`：飞行器属性（经纬度/姿态/电量/云台/负载）→ 路由给 `cloud/track`

两种通道并存是这次移植跟机场3最大的结构差异：机场3的云台/相机指令走 `services`，Pilot2 要求走 `drc/down`。**这不是"把 tid/bid 换成 seq"这么简单，是两套完全独立的请求-应答匹配机制**，需要分别设计（见第4节）。

## 3. 模块设计

### 3.1 `cloud/device`（DeviceMain）—— 遥控器属性

只负责一件事：解析遥控器(网关) `osd` 属性，提供 `getDrcState()`。

- 新增 `RcProperties` 类（对应机场3 `Dock3Osd` 的角色，但解析对象是遥控器属性而不是机场OSD）
  - 输入：`thing/product/{rc_sn}/osd` 消息（`mqttsubMain.cpp` 里 `s_rcOsd` 分支已经路由到 `COMMON_REG_IND` → `MODULE_DEVICE`，无需改分发逻辑）
  - 消息体：走 JSON 直接解析（不像机场3那样有 protobuf，Pilot2 官方文档没给出 proto 定义，参照 `TrackMain` 现有的纯 JSON 解析方式）
  - 目前只需要取 `drc_state`(int, 0/1/2) 一个字段；`capacity_percent`/`wireless_link`等其余字段先不解析，用不到
- `DeviceMain` 对外只暴露 `uint16_t getDrcState()`，语义和取值范围与机场3 `DeviceMain::getDrcState()` 完全一致，`FlytoController`/`PayloadController` 不用感知数据来源变了

**不做**：机场3 `DeviceMain` 里的 `getDroneInDock()`/`getTaskStepCode()`/`getModeCode()`——机场3专属概念，Pilot2 没有对应属性，调用方需要相应调整（见3.3节）。

### 3.2 `cloud/track`（TrackMain）—— 飞行器遥测转发

对应机场3 `TrackMain` 的角色，解析飞行器 `osd` 属性并转发给内部平台。

- 字段名跟机场3飞行器OSD**基本一致**，可以照搬现有 `parseOsdMsg`/`packFlightInfo`/`packPayloadParam` 的解析和转发逻辑：
  - `longitude`/`latitude`/`height`/`elevation`/`horizontal_speed`/`attitude_head`/`mode_code`
  - `battery.capacity_percent`/`battery.remain_flight_time`
- **唯一结构性差异**（务必注意）：机场3是"`cameras[]`数组（拿 `payload_index`/`zoom_factor`）+ 按负载编号动态命名的顶层字段（拿 `gimbal_pitch`/`gimbal_yaw`）"两段式；Pilot2 是**单个动态命名的 struct**（key=payload_index的值），`payload_index`/`zoom_factor`/`gimbal_pitch`/`gimbal_roll`/`gimbal_yaw` 全部平铺在里面。解析代码要相应改写，不能照搬机场3的两段式查找逻辑。
  - 处理方式：先找到 `data` 下唯一的、值是 struct 且不是已知固定字段名的那个 key（即 `type_subtype_gimbalindex` 那一行说的动态命名字段），把它当作 payload_index 的值本身；从这个 struct 里取 `zoom_factor`/`gimbal_pitch`/`gimbal_yaw`。
  - **实现前必须先用真实 Pilot2 飞行器 osd 报文核对一遍字段结构**（目前这部分设计是基于官方文档表格推断的，没有实测报文验证过，参见待确认事项）。
- 输出转发的内部平台 Topic 复用现有的：`bxt/thing/product/osd/`（飞行状态）、`bxt/cloud/gimbal/camera/param/`（云台/相机参数）——不用改内部接口，只改数据来源解析。

### 3.3 `cloud/flyto`（FlytoMain）—— DRC链路管理 + 权限抢占 + 飞行控制

这是移植量最大的模块，拆成三块：

#### 3.3.1 DRC链路管理（`drc_mode_enter`/`drc_mode_exit`/`heart_beat`）

字段和机场3完全一样（`mqtt_broker{address,client_id,username,password,expire_time,enable_tls}`/`osd_frequency`/`hsi_frequency`），照搬 `pl_utils.cpp` 里 `drcModeEnter`/`drcModeExit`/心跳发送的公共构造逻辑即可，`FlytoRequestManager` 走 `services`/`services_reply`，tid 匹配方式不变。

**需要改的地方**：`ensureFlightControlAndMode()`/`ensureDrcMode()` 现在判断"是否已进入指令飞行模式"要改成查 `DeviceMain::getDrcState()`（数据源是遥控器属性），机场3原来查的是 `DeviceMain::getDrcState()` 背后的机场OSD——**调用方代码不用改，只是 `DeviceMain` 内部实现换了数据源**（3.1节已经保证了这个接口兼容）。

#### 3.3.2 权限抢占（`cloud_control_auth_request`/`cloud_control_release`/`cloud_control_auth_notify`）

机场3的 `payload_authority_grab`/`flight_authority_grab` 两个方法从未被实际调用过，这次是**新实现**，不是照搬：

- `cloud_control_auth_request`（services下行）：请求体 `{user_id, user_callsign, control_keys:["flight"]}`，回包只有 `{result}`
- `cloud_control_release`（services下行）：请求体 `{control_keys:["flight"]}`
- `cloud_control_auth_notify`（events上行，异步通知）：遥控器弹窗被用户处理后的结果 `{result, output.status}`（`ok`/`failed`/`canceled`）——这是一条**独立于请求-应答之外的异步事件**，需要单独注册一个 handler（不是走 `FlytoRequestManager` 的 tid 匹配，因为它跟 `cloud_control_auth_request` 的 tid 不一定对应，是遥控器用户交互完成后才触发的）

**待确认（见第6节）**：什么时机调用 `cloud_control_auth_request`——是每次要控制前都申请，还是像 `drc_mode_enter` 一样在 `ensureFlightControlAndMode()` 里做成一个前置门槛？申请后要等多久、弹窗超时怎么处理？这些机场3没有先例可参考，需要跟业务方定。

#### 3.3.3 飞行控制（`takeoff_to_point`/`fly_to_point`/`fly_to_point_update`/`fly_to_point_stop`/`return_home`/`return_home_cancel`/`stick_control`）

- `takeoff_to_point`/`fly_to_point`/`fly_to_point_update`/`return_home`：字段跟机场3 `flytoController.cpp` 现有实现完全一致，直接照搬 `request_data` 里已有的字段（`target_latitude`等），改动量很小
- `fly_to_point_stop`/`return_home_cancel`：机场3没有对应方法，这次新增，`data` 为空，走 `services`/`services_reply`，模式跟 `return_home`（无参数指令）一样简单
- `stick_control`：跟机场3 `commonControlSend` 完全一致（`drc/down` 下行，`{roll,pitch,throttle,yaw}`，无回包），照搬即可

**需要改的地方**：`shouldExitDrcMode()` 里"飞机在舱内(`droneInDock`)则退出DRC"这一条，Pilot2 没有"舱"这个概念——这条判断在 Pilot2 下永远不会为真（`getDroneInDock()`本来就不该再被调用），需要去掉这个判断分支，只保留"控制态已置空闲"这一条退出条件（另一条被注释掉的"`drcState==INACTIVE`"分支要不要恢复，见第6节待确认）。

### 3.4 `cloud/payload`（PayloadMain）—— 云台/相机负载控制

5个方法字段跟机场3完全一致，唯一改动是通道：`services`(tid/bid) → `drc/down`(seq，回执只有`{result}`)。

| 机场3(services) | Pilot2(drc/down) | 字段(不变) |
|---|---|---|
| `gimbal_reset` | `drc_gimbal_reset` | `payload_index`/`reset_mode` |
| `camera_screen_drag` | `drc_camera_screen_drag` | `payload_index`/`locked`/`pitch_speed`/`yaw_speed` |
| `camera_aim` | `drc_camera_aim` | `payload_index`/`camera_type`/`locked`/`x`/`y` |
| `camera_frame_zoom` | `drc_camera_frame_zoom` | `payload_index`/`camera_type`/`locked`/`x`/`y`/`width`/`height` |
| `camera_focal_length_set` | `drc_camera_focal_length_set` | `payload_index`/`zoom_factor` |

`camera_type` 在 Pilot2 的 `drc_camera_aim`/`drc_camera_frame_zoom` 里多支持一个 `"ir"`（红外）取值，机场3现有代码写死 `wide`/`zoom`，移植时要把这个限制放开（改成透传输入参数，而不是写死字符串）。

`payload_index` 依然从 `TrackMain::getPayloadIndex()` 动态获取（3.2节的飞行器OSD解析结果），这条链路机场3已经验证过是对的，原样保留。

## 4. 通道适配层设计（关键新增）

机场3现有 `PayloadRequestManager`/`FlytoRequestManager` 是围绕 `tid` 做请求-应答匹配的（发请求时记一个 pending map，`services_reply` 回来按 `tid` 查表回填结果）。`drc/down`/`drc/up` 没有 `tid`，不能直接套用。

机场3代码里**已经有一个可以参考的样板**：`cloud/payload/lightController.cpp`（探照灯）就是完全按 `drc_up_down`/`drc_data`（`seq`+`method`）实现的，虽然探照灯本身不在这次移植范围内，但它的"怎么发 drc/down、怎么等 drc/up 回执"这套写法可以直接复用到 `drc_gimbal_reset` 等5个方法上：

- 请求-应答匹配方式：由于 `drc/down`/`drc/up` 上同一时刻通常只有一条指令在途，可以用"发送后同步等待下一条同 method 名的 drc/up 消息"的方式（`mqttsubMain.cpp` 的 `onRead()` 已经是按 method 名分发的，天然支持这个模式），不需要像 `tid` 那样维护一个多路复用的 pending map。
- 新增一个轻量的 `PayloadRequestManager::sendDrcRequestAndWait(method, data, timeout)` 之类的方法，内部用条件变量等 `mqttsubMain` 回调把结果写回来，接口形状可以模仿现有 `sendRequestAndWait(message, tid, ...)`，只是去掉 tid 参数、换成按 method 等待。

## 5. 状态依赖设计（汇总）

| 查询接口 | 机场3数据源 | Pilot2数据源 | 调用方要不要改 |
|---|---|---|---|
| `getDrcState()` | 机场OSD.`drc_state` | 遥控器属性.`drc_state` | 不用改，`DeviceMain`内部换源 |
| `getDroneInDock()` | 机场OSD.`drone_in_dock` | 无 | 要改：`shouldExitDrcMode()`去掉这个判断分支 |
| `getTaskStepCode()`/`getModeCode()`(机场状态) | 机场OSD | 无 | 不需要迁移（原调用点`waypointIndexInit`/`飞行任务结束`上报本身也不在移植范围） |
| `getUavModeCode()`(飞行器状态) | 飞行器OSD.`mode_code` | 飞行器属性.`mode_code` | 不用改，字段名/取值一致 |

## 6. 待确认事项（需要业务方/你确认后才能定稿）

1. **权限抢占的调用时机**：`cloud_control_auth_request` 什么时候发（每次控制前 vs 建会话时一次性申请）？弹窗无响应/超时怎么处理？
2. **DRC链路建立方（复述之前的结论）**：`drc_mode_enter` 里的 `mqtt_broker` 信息具体由谁在什么时机下发，机场3是"按需进入"，Pilot2 场景需要重新确认触发时机。
3. **飞行器OSD的动态负载结构**：3.2节的解析设计目前只是基于官方文档表格推断，需要真实报文核对确认。
4. **`shouldExitDrcMode()` 去掉"舱内退出"分支后，要不要补一个新的退出条件**（比如`mode_code`进入某个特定状态就退出DRC）？还是保持"只有控制态置空闲才退出"这一条即可？

## 7. 落地顺序建议

1. `cloud/device`（`RcProperties`/`getDrcState()`）—— 其它模块的基础依赖
2. `cloud/track`（飞行器OSD解析）—— `payload_index`/`mode_code`等要靠它，且能独立测试
3. `cloud/flyto`（DRC链路管理 → 权限抢占 → 飞行控制，按这个子顺序）
4. `cloud/payload`（依赖1和"通道适配层"，复用`lightController`的drc/down模式）

每一步做完建议单独编译+连真实环境验证一次，不要攒到最后一起联调。
