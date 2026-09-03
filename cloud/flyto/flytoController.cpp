#define THIS_MODULE MODULE_FLYTO

#include "flytoController.h"
#include "deviceMain.h"

#include "pl.h"
#include "pf_map_block_3d.h"
#include "jsonUtil.h"
#include "pl_utils.h"
#include "bxt_cloud_common.pb.h"
#include "json.h"
#include "moveSpeedTable.h"

#include <thread>
#include <chrono>
#include <algorithm>
#include <array>
#include <limits>

/* Private constants ---------------------------------------------------------*/

/* Exported values -----------------------------------------------------------*/
extern bxt_cloud_common::common_message s_pbCommonCfg;

/* Private values ------------------------------------------------------------*/

namespace {
// 变焦倍数 -> YAW 角速度 (deg/s) 分段线性映射表
// 来源：固定下发 angle=10°、YAW_SPEED=23.5 时的实测转角折算（actual_rate = delta_angle * 2.35）
// 大疆官方说明：为保证画面稳定，变焦倍数越大，飞行器实际可达的转动角速度越低
struct ZoomSpeedPoint { float zoom; float speed; };
constexpr std::array<ZoomSpeedPoint, 10> kZoomYawSpeedTable = {{
    { 1.0f, 72.5f},
    { 2.0f, 36.7f},
    { 3.0f, 28.4f},
    { 4.0f, 22.8f},
    { 5.0f, 19.2f},
    { 6.0f, 16.5f},
    { 7.0f, 14.5f},
    { 8.0f, 12.9f},
    { 9.0f, 11.9f},
    {10.0f, 10.9f},
}};

// 在表内做分段线性插值；超出范围按端点裁剪
float yawSpeedForZoom(float zoom)
{
    const auto& table = kZoomYawSpeedTable;
    if (zoom <= table.front().zoom) return table.front().speed;
    if (zoom >= table.back().zoom)  return table.back().speed;
    for (size_t i = 1; i < table.size(); ++i) {
        if (zoom <= table[i].zoom) {
            float span = table[i].zoom - table[i-1].zoom;
            float t = (zoom - table[i-1].zoom) / span;
            return table[i-1].speed + t * (table[i].speed - table[i-1].speed);
        }
    }
    return table.back().speed;
}

// 大疆飞行器状态(mode_code)：我方持续移动(杆量)控制时飞机为 3(手动飞行)；手动中可能瞬时切到
// 避障模式 8(ADS-B躲避)/15(APAS) 再切回，这两者仍视为"我方控制中"。一旦离开这三者
// （返航9/降落10-12/航线5/失联14等），即说明被接管，应立即停车。
constexpr int UAV_MODE_MANUAL         = 3;   // 手动飞行——我方持续移动控制态
constexpr int UAV_MODE_ADSB_AVOID     = 8;   // ADS-B 躲避（手动中瞬时避障）
constexpr int UAV_MODE_APAS           = 15;  // APAS（手动中瞬时避障）
constexpr int UAV_MODE_COMMAND_FLIGHT = 17;  // 指令飞行——fly_to_point 执行态

// 飞机是否仍处于"我方手动控制"上下文（手动 或 手动中的瞬时避障）
inline bool isUavUnderManualControl(int modeCode)
{
    return modeCode == UAV_MODE_MANUAL ||
           modeCode == UAV_MODE_ADSB_AVOID ||
           modeCode == UAV_MODE_APAS;
}

// 按移动方向取速度上限（m/s）：升/降各自上限，其余为水平上限
float axisMaxSpeed(MoveMode mode)
{
    switch (mode) {
        case MoveMode::UP:   return flyto_speed::kAscendMaxSpeed;
        case MoveMode::DOWN: return flyto_speed::kDescendMaxSpeed;
        default:             return flyto_speed::kHorizontalMaxSpeed;
    }
}
} // namespace

/* Private functions declaration ---------------------------------------------*/

FlytoController::FlytoController(FlytoRequestManager& request)
    : m_actionHandlerMap{
        {static_cast<int>(Action::TAKEOFF), [this]() { handleTakeoff(); }},
        {static_cast<int>(Action::GOHOME),  [this]() { handleGohome(); }},
        {static_cast<int>(Action::MOVE),    [this](dji_cloud::flight_control_message& msg) { handleMove(msg); }},
        {static_cast<int>(Action::TURN),    [this](dji_cloud::flight_control_message& msg) { handleTurn(msg); }},
        {static_cast<int>(Action::FLYTO_POINT), [this](dji_cloud::flight_control_message& msg) { handleFlytoPoint(msg); }},
        {static_cast<int>(Action::CONTINUOUS_MOVE), [this](dji_cloud::flight_control_message& msg) { handleContinuousMove(msg); }},
        {static_cast<int>(Action::STOP_MOVE), [this](dji_cloud::flight_control_message& msg) { handleStopMove(msg); }}
    },
    m_requestManager(request)
{}

FlytoController::~FlytoController()
{
    if (m_continuousParams) {
        m_continuousParams->active.store(false);
    }

    if (m_continuousMoveThread && m_continuousMoveThread->joinable()) {
        m_continuousMoveThread->join();
    }
}

void FlytoController::sendResult(uint16_t successCode, uint16_t failedCode, bool ret,
                                 const std::string& log, const std::string& errResult)
{
    uint16_t code = successCode;
    std::string data = log + "成功";
    int result = 0;
    if (!ret) {
        code = failedCode;
        data = log + "失败" + errResult;
        result = 1;
        setUavControlMode(E_BxtUavControlMode::IDLE);
        pl_log(INF, "更新控制状态 | mode: %s", getControlModeDesc());
        pl_log(ERR, "code: %d, %s", code, data.c_str());
    }
    handleUavResult(code, data, result);
}

/**
 * @brief 本地判断是否该停止发送控制指令（不涉及真正断开DRC链路，那是控制平台的事）
 *
 * 目前只保留"控制态已置空闲"这一条判断；机场3专属的"飞机在舱内"分支已去掉（Pilot2无此概念）。
 * 暂未挂到任何调用点，是否需要补充其它停发信号见设计文档第6节待确认事项3。
 */
bool FlytoController::shouldExitDrcMode()
{
    if (getUavControlMode() == E_BxtUavControlMode::IDLE) {
        pl_log(INF, "本地停止发送控制指令 | 原因: 控制态已置空闲");
        return true;
    }
    return false;
}

/**
 * @brief 处理 flyto 执行结果事件通知 (method: fly_to_point_progress)
 *
 * 函数功能：
 * 1. 解析 dock 在 /drc/up 上异步推送的飞向目标点执行进度事件
 * 2. 进度状态仅打印日志（执行中/到达/取消等）
 * 3. 仅当执行失败（status=wayline_failed 或 result!=0）时，上报失败结果，
 *    并在失败信息中带上具体状态
 *
 * 说明：该事件中各字段直接位于 data 下，且 status 为枚举字符串，
 *       与 proto reply_data.status(uint32) 字段名冲突，故直接用 jsoncpp 解析。
 *
 * @param msg fly_to_point_progress 事件的原始 JSON 字符串
 */
void FlytoController::handleFlytoProgress(const std::string& msg)
{
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    std::string errs;

    if (!reader->parse(msg.c_str(), msg.c_str() + msg.size(), &root, &errs) || !errs.empty()) {
        pl_log(ERR, "flyto进度事件JSON解析失败: %s | 原始数据: %.*s",
               errs.c_str(), static_cast<int>(msg.size()), msg.data());
        return;
    }

    if (!root.isMember("data") || !root["data"].isObject()) {
        pl_log(WARN, "flyto进度事件缺少data字段 | 原始数据: %.*s",
               static_cast<int>(msg.size()), msg.data());
        return;
    }

    const Json::Value& data = root["data"];
    std::string status        = data.get("status", "").asString();
    std::string flyToId       = data.get("fly_to_id", "").asString();
    int result                = data.get("result", 0).asInt();
    int wayPointIndex         = data.get("way_point_index", 0).asInt();
    double remainingDistance  = data.get("remaining_distance", 0.0).asDouble();
    double remainingTime      = data.get("remaining_time", 0.0).asDouble();

    // 缓存最近一次执行状态，供 handleFlytoPoint 每次调用时查询
    {
        std::lock_guard<std::mutex> lock(m_flytoStatusMutex);
        m_lastFlytoStatus            = status;
        if (!flyToId.empty()) {
            m_lastSentFlyToId = flyToId;   // 以事件携带的 fly_to_id 为准
        }
        m_lastFlytoResult            = result;
        m_lastFlytoWayPointIndex     = wayPointIndex;
        m_lastFlytoRemainingDistance = remainingDistance;
        m_lastFlytoRemainingTime     = remainingTime;
        m_lastFlytoProgressTime      = get_milliseconds();
    }

    // 进度状态仅打印日志
    pl_log(INF, "flyto执行进度 | status=%s, result=%d, fly_to_id=%s, way_point_index=%d, "
                "remaining_distance=%.1fm, remaining_time=%.1fs",
           status.c_str(), result, flyToId.c_str(), wayPointIndex,
           remainingDistance, remainingTime);

    // 仅在执行失败时上报，并在失败信息中带上状态
    if (status == "wayline_failed" || result != 0) {
        std::string errMsg = "飞向目标点失败, 状态:" + status + ", result:" + std::to_string(result);
        pl_log(ERR, "%s", errMsg.c_str());
        handleUavResult(static_cast<uint16_t>(STATE_FLYTO_POINT_FAILED), errMsg, 1);

        setUavControlMode(E_BxtUavControlMode::IDLE);
        pl_log(INF, "更新控制状态 | mode: %s", getControlModeDesc());
    }
}

/**
 * @brief 一键起飞 消息封装、发布
 */
void FlytoController::handleTakeoff()
{
    bxt_cloud_common::takeoff_message* p_takeoff_msg = s_pbCommonCfg.mutable_takeoff();
    bxt_cloud_common::simulate_message* p_simulateCfg = s_pbCommonCfg.mutable_simulate();

    dji_cloud::services_down message;
    dji_cloud::request_data* p_data = message.mutable_data();
    dji_cloud::simulate_message* p_simulate = p_data->mutable_simulate_mission();

    std::string tid = generate_uuid();
    message.set_tid(tid);
    message.set_bid(generate_uuid());
    message.set_timestamp(get_milliseconds());
    message.set_method("takeoff_to_point");

    p_data->set_target_latitude(p_simulateCfg->latitude());
    p_data->set_target_longitude(p_simulateCfg->longitude());
    p_data->set_target_height(p_takeoff_msg->commander_flight_height());
    p_data->set_security_takeoff_height(p_takeoff_msg->commander_flight_height());

    p_data->set_rth_mode(p_takeoff_msg->rth_mode());
    p_data->set_rth_altitude(p_takeoff_msg->rth_altitude());
    p_data->set_rc_lost_action(p_takeoff_msg->rc_lost_action());
    p_data->set_commander_mode_lost_action(p_takeoff_msg->commander_mode_lost_action());
    p_data->set_commander_flight_mode(p_takeoff_msg->commander_flight_mode());
    p_data->set_commander_flight_height(p_takeoff_msg->commander_flight_height());
    p_data->set_flight_id("01234567890");
    p_data->set_max_speed(p_takeoff_msg->max_speed());
    p_data->set_flight_safety_advance_check(p_takeoff_msg->flight_safety_advance_check());

    p_simulate->set_is_enable(p_simulateCfg->is_enable());
    p_simulate->set_latitude(p_simulateCfg->latitude());
    p_simulate->set_longitude(p_simulateCfg->longitude());

    std::string log = "一键起飞";
    uint32_t msgId = DJI_SERVICES_PUBLISH_DATA_IND;
    std::string errResult;
    bool ret = m_requestManager.sendRequestAndWait(message, tid, msgId, log, errResult);

    uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_TAKE_OFF);
    uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_TAKE_OFF_FAILED);
    sendResult(successCode, failedCode, ret, log, errResult);
}

/**
 * @brief 处理一键返航请求
 *
 * 函数功能：
 * 1. 创建返航消息对象
 * 2. 生成唯一事务ID和批次ID
 * 3. 设置时间戳和方法名
 * 4. 发送返航请求并等待响应
 */
void FlytoController::handleGohome()
{
    dji_cloud::services_down message;
    std::string tid = generate_uuid();
    message.set_tid(tid);
    message.set_bid(generate_uuid());
    message.set_timestamp(get_milliseconds());
    message.set_method("return_home");

    dji_cloud::request_data* empty_data = message.mutable_data();

    std::string log = "一键返航";
    uint32_t msgId = DJI_SERVICES_PUBLISH_DATA_IND;
    std::string errResult;
    bool ret = m_requestManager.sendRequestAndWait(message, tid, msgId, log, errResult);

    uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_GO_HOME);
    uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_GO_HOME_FAILED);
    sendResult(successCode, failedCode, ret, log, errResult);

    setUavControlMode(E_BxtUavControlMode::IDLE);
    pl_log(INF, "更新控制状态 | mode: %s", getControlModeDesc());
}

/**
 * @brief 获取指定移动模式的偏移量
 */
FlightControlOffset FlytoController::getMoveOffset(MoveMode mode)
{
    int step = m_stickConfig.moveStep;
    switch(mode) {
        case MoveMode::UP:       return { step, 0, 0, 0};
        case MoveMode::DOWN:     return {-step, 0, 0, 0};
        case MoveMode::LEFT:     return {0, -step, 0, 0};
        case MoveMode::RIGHT:    return {0,  step, 0, 0};
        case MoveMode::FORWARD:  return {0, 0,  step, 0};
        case MoveMode::BACKWARD: return {0, 0, -step, 0};
        default: return {0, 0, 0, 0};
    }
}

FlightControlOffset FlytoController::calculateMoveOffset(MoveMode mode, float speed)
{
    switch (mode) {
        case MoveMode::UP:
            return { flyto_speed::speedToOffset(speed, flyto_speed::kAscendSpeedMap), 0, 0, 0};
        case MoveMode::DOWN:
            return {-flyto_speed::speedToOffset(speed, flyto_speed::kDescendSpeedMap), 0, 0, 0};
        case MoveMode::LEFT:
            return {0, -flyto_speed::speedToOffset(speed, flyto_speed::kHorizontalSpeedMap), 0, 0};
        case MoveMode::RIGHT:
            return {0,  flyto_speed::speedToOffset(speed, flyto_speed::kHorizontalSpeedMap), 0, 0};
        case MoveMode::FORWARD:
            return {0, 0,  flyto_speed::speedToOffset(speed, flyto_speed::kHorizontalSpeedMap), 0};
        case MoveMode::BACKWARD:
            return {0, 0, -flyto_speed::speedToOffset(speed, flyto_speed::kHorizontalSpeedMap), 0};
        default:
            return {0, 0, 0, 0};
    }
}

/**
 * @brief 发送飞行控制指令（stick_control，走 drc/down，跟机场3完全一致）
 */
bool FlytoController::commonControlSend(const FlightControlOffset& offset)
{
    dji_cloud::services_down message;
    dji_cloud::request_data* p_data = message.mutable_data();

    message.set_seq(++m_controlSeq);
    message.set_method("stick_control");

    int center = m_stickConfig.stickControlCenter;
    p_data->set_throttle(center + offset.throttle);
    p_data->set_roll(center + offset.roll);
    p_data->set_pitch(center + offset.pitch);
    p_data->set_yaw(center + offset.yaw);
    p_data->set_gimbal_pitch(center);

    std::string jsonStr;
    jsonStr = pb2json(message);
    if (jsonStr.empty()) {
        pl_log(ERR, "Protobuf to JSON 失败 | method=%s", message.method().c_str());
        return false;
    }

    pl_log(INF, "发送控制指令 | data=%.*s", static_cast<int>(jsonStr.size()), jsonStr.c_str());
    pf_copy_msg(THIS_MODULE, DJI_DRC_DOWN_PUBLISH_DATA_IND, MODULE_MQTTPUB, (void*)jsonStr.c_str(), jsonStr.size());
    return true;
}

void FlytoController::calculateTimingParameters(float distance, float speed,
                                               int& totalMs, int& fullCycles, int& remainingMs)
{
    totalMs = std::round(distance / speed * 1000.0);
    fullCycles = totalMs / ModuleConstants::ControlCycle::CYCLE_DURATION_MS;
    remainingMs = totalMs % ModuleConstants::ControlCycle::CYCLE_DURATION_MS;
}

bool FlytoController::executeCycleControl(const FlightControlOffset& offset, int fullCycles)
{
    for (int i = 0; i < fullCycles; ++i) {
        if (!commonControlSend(offset)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(ModuleConstants::ControlCycle::CYCLE_DURATION_MS));
    }
    return true;
}

bool FlytoController::executeTimedControl(const FlightControlOffset& offset, float distance, float speed)
{
    pl_log(INF, "执行定时控制指令 | distance = %.2fm, speed = %.2fm/s", distance, speed);

    int totalMs, fullCycles, remainingMs;
    calculateTimingParameters(distance, speed, totalMs, fullCycles, remainingMs);

    pl_log(INF, "时间参数 | totalMs = %dms, fullCycles = %d, remainingMs = %dms",
            totalMs, fullCycles, remainingMs);

    if (!executeCycleControl(offset, fullCycles)) {
        pl_log(ERR, "周期控制执行失败");
        return false;
    }

    if (remainingMs > 0) {
        if (!commonControlSend(offset)) {
            pl_log(ERR, "剩余周期控制发送失败");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(remainingMs));
    }

    if (!commonControlSend({0, 0, 0, 0})) {
        pl_log(ERR, "发送回中指令失败");
        return false;
    }

    return true;
}

/**
 * @brief 处理飞行器移动指令(内部平台action=MOVE)
 */
void FlytoController::handleMove(dji_cloud::flight_control_message& msg)
{
    constexpr uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_POSITION_MOVE);
    constexpr uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_POSITION_MOVE_FAILED);

    if (!ensureFlightControlAndMode()) {
        sendResult(successCode, failedCode, false, "位置移动", "飞行模式检查失败");
        return;
    }

    int direction;
    float distance;
    float speed = 3.0f;
    try {
        direction = std::stoi(msg.direction());
        distance = std::stof(msg.distance());
    } catch (const std::exception& e) {
        pl_log(ERR, "参数转换失败: %s", e.what());
        sendResult(successCode, failedCode, false, "位置移动", "参数转换失败");
        return;
    }

    if (direction < static_cast<int>(MoveMode::UP) ||
        direction > static_cast<int>(MoveMode::BACKWARD)) {
        pl_log(ERR, "非法方向值: %d", direction);
        sendResult(successCode, failedCode, false, "位置移动", "非法方向值");
        return;
    }

    if (distance <= 0.0f) {
        pl_log(ERR, "非法距离值: %f", distance);
        sendResult(successCode, failedCode, false, "位置移动", "非法距离值");
        return;
    }

    float vmax = axisMaxSpeed(static_cast<MoveMode>(direction));
    if (speed <= 0.0f || speed > vmax) {
        pl_log(ERR, "非法速度值: %f (有效范围: 0-%.1fm/s)", speed, vmax);
        sendResult(successCode, failedCode, false, "位置移动", "非法速度值");
        return;
    }

    auto offset = calculateMoveOffset(static_cast<MoveMode>(direction), speed);
    bool ret = executeTimedControl(offset, distance, speed);
    sendResult(successCode, failedCode, ret, "位置移动", ret ? "" : "控制指令发送失败");
    if (ret) {
        setUavControlMode(E_BxtUavControlMode::IDLE);
        pl_log(INF, "更新控制状态 | mode: %s", getControlModeDesc());
    }
}

/**
 * @brief 获取指定转向模式的偏移量
 */
FlightControlOffset FlytoController::getTurnOffset(TurnMode mode)
{
    constexpr int YAW_OFFSET = 660;
    switch(mode) {
        case TurnMode::CLOCKWISE:        return { 0, 0, 0, -YAW_OFFSET};
        case TurnMode::COUNTERCLOCKWISE: return { 0, 0, 0, YAW_OFFSET };
        default: return {0,0,0,0};
    }
}

/**
 * @brief 处理飞行器转向指令(内部平台action=TURN)
 */
void FlytoController::handleTurn(dji_cloud::flight_control_message& msg)
{
    if (!ensureFlightControlAndMode()) {
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_TRUN);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_TRUN_FAILED);
        sendResult(successCode, failedCode, false, "转向", "飞行模式检查失败");
        return;
    }

    int direction;
    float angle;
    try {
        direction = std::stoi(msg.direction());
        angle = std::stof(msg.angle());
    } catch (const std::exception& e) {
        pl_log(ERR, "参数转换失败: %s", e.what());
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_TRUN);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_TRUN_FAILED);
        sendResult(successCode, failedCode, false, "转向", "参数转换失败");
        return;
    }

    if (direction < static_cast<int>(TurnMode::CLOCKWISE) ||
        direction > static_cast<int>(TurnMode::COUNTERCLOCKWISE)) {
        pl_log(ERR, "非法方向值: %d", direction);
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_TRUN);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_TRUN_FAILED);
        sendResult(successCode, failedCode, false, "转向", "非法方向值");
        return;
    }

    if (angle <= 0.0f) {
        pl_log(ERR, "非法角度值: %f", angle);
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_TRUN);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_TRUN_FAILED);
        sendResult(successCode, failedCode, false, "转向", "非法角度值");
        return;
    }

    const auto offset = getTurnOffset(static_cast<TurnMode>(direction));
    // 根据实时 OSD 变焦倍数动态计算 YAW 角速度
    // 大疆官方：变焦越大，飞行器实际可达的转动角速度越低，固定速度会导致计时不准
    float zoomFactor = TrackMain::getInstance().getZoomFactor();
    if (zoomFactor < 1.0f) {
        pl_log(WARN, "读取到的变焦倍数无效(%.2f)，回退到 3x 默认", zoomFactor);
        zoomFactor = 3.0f;
    }
    float yawSpeed = yawSpeedForZoom(zoomFactor);
    pl_log(INF, "YAW 动态速度 | zoom=%.2f, yawSpeed=%.2f deg/s, angle=%.2f",
           zoomFactor, yawSpeed, angle);
    bool ret = executeTimedControl(offset, angle, yawSpeed);

    uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_TRUN);
    uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_TRUN_FAILED);
    sendResult(successCode, failedCode, ret, "转向", ret ? "" : "控制指令发送失败");
    if (ret) {
        setUavControlMode(E_BxtUavControlMode::IDLE);
        pl_log(INF, "更新控制状态 | mode: %s", getControlModeDesc());
    }
}

void FlytoController::handleContinuousMove(dji_cloud::flight_control_message& msg)
{
    if (!ensureFlightControlAndMode()) {
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE_FAILED);
        sendResult(successCode, failedCode, false, "持续移动", "飞行模式检查失败");
        return;
    }

    int direction;
    float speed = 5.0f;
    try {
        direction = std::stoi(msg.direction());
        if (msg.has_max_speed()) {
            speed = static_cast<float>(msg.max_speed());
        }
    } catch (const std::exception& e) {
        pl_log(ERR, "参数转换失败: %s", e.what());
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE_FAILED);
        sendResult(successCode, failedCode, false, "持续移动", "参数转换失败");
        return;
    }

    if (direction < static_cast<int>(MoveMode::UP) ||
        direction > static_cast<int>(MoveMode::BACKWARD)) {
        pl_log(ERR, "非法方向值: %d", direction);
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE_FAILED);
        sendResult(successCode, failedCode, false, "持续移动", "非法方向值");
        return;
    }

    float vmax = axisMaxSpeed(static_cast<MoveMode>(direction));
    if (speed <= 0.0f || speed > vmax) {
        pl_log(ERR, "非法速度值: %f (有效范围: 0-%.1fm/s)", speed, vmax);
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE_FAILED);
        sendResult(successCode, failedCode, false, "持续移动", "非法速度值");
        return;
    }

    FlightControlOffset offset = calculateMoveOffset(static_cast<MoveMode>(direction), speed);

    if (m_continuousParams && m_continuousParams->active.load() &&
        m_continuousParams->direction == direction) {
        std::lock_guard<std::mutex> lock(m_continuousParams->mutex);
        m_continuousParams->offset = offset;
        pl_log(INF, "更新持续飞行速度 | direction=%d, speed=%.1fm/s", direction, speed);
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE_FAILED);
        sendResult(successCode, failedCode, true, "持续移动", "");
        return;
    }

    // 停止之前的持续飞行线程
    if (m_continuousMoveThread && m_continuousMoveThread->joinable()) {
        if (m_continuousParams) {
            m_continuousParams->active.store(false);
        }
        m_continuousMoveThread->join();
        commonControlSend({0, 0, 0, 0});
    }

    // 创建新的持续飞行参数
    m_continuousParams = std::make_shared<ContinuousMoveParams>();
    m_continuousParams->offset = offset;
    m_continuousParams->direction = direction;
    m_continuousParams->active.store(true);

    // 启动新的持续飞行线程
    m_continuousMoveThread = std::make_unique<std::thread>([this]() {
        // 已确认进入"手动飞行(3)"后，才据其离开判定接管，避免进入期(模式尚未切到3)误退出
        bool enteredManual = false;
        while (m_continuousParams->active.load()) {
            // 自检1：UAV 控制模式不再是 FLYTO_MODE 时主动退出
            // （航线模块接管/IDLE 切换等场景，避免向 Dock 发送已被拒收的杆量）
            if (getUavControlMode() != E_BxtUavControlMode::FLYTO_MODE) {
                pl_log(INF, "持续移动检测到 mode != FLYTO_MODE，主动退出");
                m_continuousParams->active.store(false);
                break;
            }

            // 自检2：飞机离开"手动飞行(3)"上下文(8/15 瞬时避障除外) → 被返航/降落/航线/失联等接管，立即停车
            int uavMode = TrackMain::getInstance().getUavModeCode();
            if (uavMode == UAV_MODE_MANUAL) {
                enteredManual = true;
            }
            if (enteredManual && !isUavUnderManualControl(uavMode)) {
                pl_log(WARN, "持续移动检测到飞机离开手动飞行(mode_code=%d)，疑似返航/降落/接管，主动退出", uavMode);
                m_continuousParams->active.store(false);
                setUavControlMode(E_BxtUavControlMode::IDLE);
                break;
            }

            FlightControlOffset currentOffset;
            {
                std::lock_guard<std::mutex> lock(m_continuousParams->mutex);
                currentOffset = m_continuousParams->offset;
            }

            if (!commonControlSend(currentOffset)) {
                m_continuousParams->active.store(false);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(ModuleConstants::ControlCycle::CYCLE_DURATION_MS));
        }
    });

    pl_log(INF, "开始持续飞行 | direction=%d, speed=%.1fm/s", direction, speed);

    uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE);
    uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_CONTINUOUS_MOVE_FAILED);
    sendResult(successCode, failedCode, true, "持续移动", "");
}

void FlytoController::handleStopMove(dji_cloud::flight_control_message& msg)
{
    (void)msg;

    if (m_continuousParams) {
        m_continuousParams->active.store(false);
    }

    if (m_continuousMoveThread && m_continuousMoveThread->joinable()) {
        m_continuousMoveThread->join();
    }

    if (!commonControlSend({0, 0, 0, 0})) {
        pl_log(ERR, "停止持续飞行时发送回中指令失败");
        uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_STOP_MOVE);
        uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_STOP_MOVE_FAILED);
        sendResult(successCode, failedCode, false, "停止移动", "发送回中指令失败");
        return;
    }

    pl_log(INF, "停止持续飞行");

    uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_STOP_MOVE);
    uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_STOP_MOVE_FAILED);
    sendResult(successCode, failedCode, true, "停止移动", "");
}

/**
 * @brief 验证经纬度是否在合法范围内
 */
bool FlytoController::validateCoordinates(Point p1, Point p2)
{
    if (p1.lat < -90.0 || p1.lat > 90.0 || p2.lat < -90.0 || p2.lat > 90.0) {
        pl_log(WARN, "纬度必须在 -90 到 90 之间, p1.lat=%f, p2.lat=%f", p1.lat, p2.lat);
        return false;
    }
    if (p1.lon < -180.0 || p1.lon > 180.0 || p2.lon < -180.0 || p2.lon > 180.0) {
        pl_log(WARN, "经度必须在 -180 到 180 之间, p1.lon=%f, p2.lon=%f", p1.lon, p2.lon);
        return false;
    }

    return true;
}

/**
 * @brief 使用 Haversine 公式计算两个经纬度点之间的地表距离（单位：米）
 */
double FlytoController::calculateDistance(Point p1, Point p2)
{
    // 转换为弧度
    double lat1_rad = toRadians(p1.lat);
    double lon1_rad = toRadians(p1.lon);
    double lat2_rad = toRadians(p2.lat);
    double lon2_rad = toRadians(p2.lon);

    // 差值
    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;

    // Haversine 公式
    double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
               std::cos(lat1_rad) * std::cos(lat2_rad) *
               std::sin(dlon / 2) * std::sin(dlon / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    return EARTH_RADIUS * c;
}

/**
 * @brief 高精度计算目标点相对于当前点的course方位角（正东0°，逆时针0~180°，顺时针0~-180°）
 */
double FlytoController::calculate_target_course(const Point& current, const Point& target)
{
    // WGS84椭球参数（地球标准模型）
    const double a = 6378137.0;        // 长半轴（米）
    const double b = 6356752.314245;   // 短半轴（米）
    const double f = (a - b) / a;      // 扁率
    const double pi = M_PI;
    const double rad = pi / 180.0;     // 度转弧度
    const double deg = 180.0 / pi;     // 弧度转角度

    // 步骤1：将经纬度转换为弧度
    double lat1 = current.lat * rad;
    double lon1 = current.lon * rad;
    double lat2 = target.lat * rad;
    double lon2 = target.lon * rad;

    // 步骤2：计算经纬度差值
    double delta_lon = lon2 - lon1;

    // 步骤3：Vincenty公式核心计算（初始方位角，正北为0°，顺时针0~360°）
    double U1 = atan((1 - f) * tan(lat1));
    double U2 = atan((1 - f) * tan(lat2));
    double sinU1 = sin(U1), cosU1 = cos(U1);
    double sinU2 = sin(U2), cosU2 = cos(U2);

    double lambda = delta_lon;
    double lambda_p = 2 * pi;
    double iter_limit = 100;
    double sin_lambda, cos_lambda;
    double sin_sigma, cos_sigma, sigma;
    double sin_alpha, cos2_alpha;
    double C;

    // 迭代计算（精度1e-12）
    while (fabs(lambda - lambda_p) > 1e-12 && --iter_limit > 0) {
        sin_lambda = sin(lambda);
        cos_lambda = cos(lambda);
        sin_sigma = sqrt(
            (cosU2 * sin_lambda) * (cosU2 * sin_lambda) +
            (cosU1 * sinU2 - sinU1 * cosU2 * cos_lambda) * (cosU1 * sinU2 - sinU1 * cosU2 * cos_lambda)
        );
        if (sin_sigma == 0) return 0.0;  // 两点重合
        cos_sigma = sinU1 * sinU2 + cosU1 * cosU2 * cos_lambda;
        sigma = atan2(sin_sigma, cos_sigma);
        sin_alpha = cosU1 * cosU2 * sin_lambda / sin_sigma;
        cos2_alpha = 1 - sin_alpha * sin_alpha;
        double cos2_sigma_m = cos_sigma - 2 * sinU1 * sinU2 / cos2_alpha;
        if (isnan(cos2_sigma_m)) cos2_sigma_m = 0;  // 赤道附近修正
        C = f / 16 * cos2_alpha * (4 + f * (4 - 3 * cos2_alpha));
        lambda_p = lambda;
        lambda = delta_lon + (1 - C) * f * sin_alpha *
            (sigma + C * sin_sigma * (cos2_sigma_m + C * cos_sigma * (-1 + 2 * cos2_sigma_m * cos2_sigma_m)));
    }

    // 步骤4：计算初始方位角（正北为0°，顺时针0~360°）
    double alpha1 = atan2(cosU2 * sin_lambda, cosU1 * sinU2 - sinU1 * cosU2 * cos_lambda);
    alpha1 = alpha1 * deg;  // 转换为角度

    // 步骤5：归一化到 [-180, 180]（匹配attitude_head格式：正北0°，逆时针0~-180，顺时针0~180）
    double ah_norm = fmod(alpha1, 360.0);
    if (ah_norm > 180.0) ah_norm -= 360.0;
    if (ah_norm < -180.0) ah_norm += 360.0;

    // 步骤6：转换为course格式（正东0°，逆时针0~180°，顺时针0~-180°）
    // 校准参数：+3.25° 匹配你的实测值（因无人机/设备的方位角基准微小偏移）
    double course = 90.0 - ah_norm - 0.781551;

    // 步骤7：最终归一化到 [-180, 180]
    if (course > 180.0) course -= 360.0;
    if (course < -180.0) course += 360.0;

    return course;
}

void FlytoController::handleFlytoPoint(dji_cloud::flight_control_message& msg)
{
    // 每次调用先查询上一次 flyto 执行状态（来自 fly_to_point_progress 事件）。
    // 若上一段飞行仍在执行(wayline_progress)，本次改用 fly_to_point_update 更新目标点。
    bool flytoInProgress = false;
    std::string inProgressFlyToId;
    {
        std::lock_guard<std::mutex> lock(m_flytoStatusMutex);
        flytoInProgress   = (m_lastFlytoStatus == "wayline_progress");
        inProgressFlyToId = m_lastSentFlyToId;

        // 用飞机实际飞行模式校正陈旧状态：fly_to_point 执行时飞机为 17(指令飞行)。
        if (flytoInProgress) {
            int uavMode = TrackMain::getInstance().getUavModeCode();
            if (uavMode != UAV_MODE_COMMAND_FLIGHT) {
                pl_log(WARN, "flyto状态卡在wayline_progress但飞机已离开指令飞行(mode_code=%d)，判定已结束并重置状态", uavMode);
                m_lastFlytoStatus = "ok";
                flytoInProgress   = false;
            }
        }

        if (m_lastSentFlyToId.empty() && m_lastFlytoStatus.empty()) {
            pl_log(INF, "查询flyto执行状态 | 暂无历史记录(本次为首次调用或尚未收到进度事件)");
        } else {
            U64 age = (m_lastFlytoProgressTime > 0) ? (get_milliseconds() - m_lastFlytoProgressTime) : 0;
            pl_log(INF, "查询flyto执行状态 | last_fly_to_id=%s, status=%s, result=%d, way_point_index=%d, "
                        "remaining_distance=%.1fm, remaining_time=%.1fs, 距上次进度=%llums",
                   m_lastSentFlyToId.c_str(), m_lastFlytoStatus.c_str(), m_lastFlytoResult,
                   m_lastFlytoWayPointIndex, m_lastFlytoRemainingDistance, m_lastFlytoRemainingTime,
                   (unsigned long long)age);
        }
    }

    double longitude;
    double latitude;
    float height;
    uint32_t max_speed;
    constexpr uint16_t successCode = static_cast<uint16_t>(STATE_FLYTO_POINT);
    constexpr uint16_t failedCode = static_cast<uint16_t>(STATE_FLYTO_POINT_FAILED);
    try {
        longitude = msg.longitude();
        latitude = msg.latitude();
        height = msg.height();
        max_speed = msg.max_speed();
    } catch (const std::exception& e) {
        pl_log(ERR, "flyto_point参数转换失败: %s", e.what());
        sendResult(successCode, failedCode, false, "飞向目标点", "参数转换失败");
        return;
    }

    // 获取当前飞行器位置信息
    Point currentPos = TrackMain::getInstance().getUavPoint();
    // 飞行器坐标点
    Point currentPoint = {currentPos.lon, currentPos.lat};
    // 目标坐标点
    Point targetPoint = {longitude, latitude};
    // 验证目标点坐标合法性
    bool res = validateCoordinates(currentPoint, targetPoint);
    if (!res) {
        pl_log(ERR, "flyto_point坐标验证失败 | currentPoint: (%f,%f), targetPoint: (%f,%f)",
                currentPoint.lon, currentPoint.lat, targetPoint.lon, targetPoint.lat);
        sendResult(successCode, failedCode, false, "飞向目标点", "坐标验证失败");
        return;
    }
    // 计算目标点与当前点的距离
    double distance_m = calculateDistance(currentPoint, targetPoint);
    pl_log(INF, "flyto_point | currentPoint: (%f,%f), targetPoint: (%f,%f). distance: %f m",
            currentPoint.lon, currentPoint.lat, targetPoint.lon, targetPoint.lat, distance_m);

    double course = calculate_target_course(currentPoint, targetPoint);

    std::string log = "飞向目标点";
    uint32_t msgId = DJI_SERVICES_PUBLISH_DATA_IND;
    std::string errResult;
    double safe_dis;
    bool ret;

    S32 result = check_block_around_line_lonlat(currentPos.lat, currentPos.lon, currentPos.alt, course, distance_m, 5.0, 1.0, &safe_dis);
    if (result == RET_ERR) {
        log = "fly_to_point移动失败. 坐标信息错误";
        pl_log(ERR, "%s", log.c_str());
        ret = false;
        errResult = "坐标信息错误";
    } else if (result == RET_OK) {
        log = "fly_to_point移动失败. 目标位置有障碍物";
        pl_log(WARN, "目标位置有障碍物，可移动安全距离:%f", safe_dis);
        ret = false;
        errResult = "目标位置有障碍物";
    } else {
        pl_log(INF, "目标位置无障碍物, 可安全移动");
        dji_cloud::services_down message;
        dji_cloud::request_data* p_data = message.mutable_data();
        dji_cloud::points_array* p_point = p_data->add_points();

        std::string tid = generate_uuid();
        message.set_tid(tid);
        message.set_bid(generate_uuid());
        message.set_timestamp(get_milliseconds());

        if (flytoInProgress) {
            // 上一段飞行仍在执行(wayline_progress)：改用 fly_to_point_update 更新目标点。
            message.set_method("fly_to_point_update");
            p_data->set_max_speed(max_speed);
            log = "更新飞向目标点";
            pl_log(INF, "上一段飞行仍在执行(wayline_progress)，改用 fly_to_point_update 更新目标点 | fly_to_id=%s",
                   inProgressFlyToId.c_str());
        } else {
            // 常规下发：fly_to_point，生成新的 fly_to_id
            message.set_method("fly_to_point");
            std::string flyToId = generate_uuid();
            p_data->set_fly_to_id(flyToId);
            p_data->set_max_speed(max_speed);

            // 记录本次下发的 fly_to_id，供 fly_to_point_progress 匹配及下次调用时查询
            {
                std::lock_guard<std::mutex> lock(m_flytoStatusMutex);
                m_lastSentFlyToId       = flyToId;
                m_lastFlytoStatus       = "sent";   // 已下发，等待执行进度事件
                m_lastFlytoProgressTime = get_milliseconds();
            }
        }

        p_point->set_longitude(longitude);
        p_point->set_latitude(latitude);
        p_point->set_height(height);

        ret = m_requestManager.sendRequestAndWait(message, tid, msgId, log, errResult);
    }

    sendResult(successCode, failedCode, ret, log, errResult);
}

/**
 * @brief 确保指令飞行模式（只读检查，不再主动建链）
 *
 * 设计文档第3.3节"需要改的地方"：DRC链路的建立/维持归控制平台另一组负责，
 * 本模块只查 DeviceMain::getDrcState()，已连接就直接发指令，未连接直接失败返回，
 * 不再自己尝试进入DRC模式(enterDrcControl)或启动心跳线程。
 */
bool FlytoController::ensureFlightControlAndMode()
{
    if (DeviceMain::getInstance().getDrcState() == ModuleConstants::ControlMode::DRC_STATE_INACTIVE) {
        pl_log(WARN, "DRC链路未就绪，无法下发飞行控制指令");
        return false;
    }
    // control_source不是"A"/"B"(物理设备)才代表云端持有控制权，见设计文档第6节待确认事项1
    if (!TrackMain::getInstance().isCloudControlActive()) {
        pl_log(WARN, "云端尚未持有飞行控制权(control_source非云端)，无法下发飞行控制指令");
        return false;
    }
    return true;
}

/**
 * @brief 处理飞行控制指令总入口
 *
 * 使用json_to_proto工具将JSON转换为protobuf格式；
 * 通过std::visit和std::variant实现多态函数调用；
 * action取值范围：TAKEOFF=1, GOHOME=3, MOVE=4, TURN=6, FLYTO_POINT=7, CONTINUOUS_MOVE=8, STOP_MOVE=9
 * （机场3的LAND=2这次不启用——大疆官方接口本身不支持独立降落指令）。
 *
 * @param msg 包含飞行控制指令的JSON字符串
 */
void FlytoController::handleFlightControl(const std::string& msg)
{
    constexpr uint16_t failedCode = static_cast<uint16_t>(STATE_FLIGHT_FLYTO_WARN_INVALID_PARAM);

    // 解析飞行控制消息
    std::string jsonStr(msg);
    dji_cloud::flight_control_message message;
    if (!json_to_proto(jsonStr, message)) {
        pl_log(ERR, "JSON to proto 转换失败 | 原始数据: %.*s",
               static_cast<int>(jsonStr.size()), jsonStr.c_str());
        handleUavResult(failedCode, "飞行控制失败JSON解析失败", 1);
        return;
    }

    updateTransId(message.trans_id());

    int action;
    try {
        action = std::stoi(message.action());
    } catch (const std::exception& e) {
        pl_log(ERR, "action参数转换失败: %s", e.what());
        handleUavResult(failedCode, "飞行控制失败action参数转换失败", 1);
        return;
    }

    constexpr std::array<int, 7> valid_actions = {1, 3, 4, 6, 7, 8, 9};
    if (std::find(valid_actions.begin(), valid_actions.end(), action) == valid_actions.end()) {
        pl_log(ERR, "非法action值: %d", action);
        handleUavResult(failedCode, "飞行控制失败非法action值", 1);
        return;
    }

    auto it = m_actionHandlerMap.find(action);
    if (it != m_actionHandlerMap.end())
    {
        setUavControlMode(E_BxtUavControlMode::FLYTO_MODE);
        pl_log(INF, "更新控制状态 | mode: %s", getControlModeDesc());

        // 根据variant类型调用不同的处理器
        std::visit([&message](auto&& handler) {
            using HandlerType = std::decay_t<decltype(handler)>;
            if constexpr (std::is_invocable_v<HandlerType, dji_cloud::flight_control_message&>) {
                handler(message);
            } else if constexpr (std::is_invocable_v<HandlerType>) {
                handler();
            }
        }, it->second);
    }
    else
    {
        pl_log(ERR, "未知飞行控制指令 | action: %d", action);
        handleUavResult(failedCode, "飞行控制失败未知飞行控制指令", 1);
    }
}
