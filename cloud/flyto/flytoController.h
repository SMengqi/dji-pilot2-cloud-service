#ifndef FLYTO_CONTROLLER_H_
#define FLYTO_CONTROLLER_H_

#include <unordered_map>
#include <functional>
#include <variant>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <cstdint>

#include <iostream>
#include <cmath>
#include <iomanip>
#include <stdexcept>

#include "trackMain.h"
#include "flytoRequestManager.h"
#include "deviceMain.h"
#include "dji_cloud_api.pb.h"
#include "moduleConstants.h"

// 使用variant定义处理器类型，支持无参数和带参数的函数
using ActionHandler = std::variant<
    std::function<void()>,
    std::function<void(dji_cloud::flight_control_message&)>
>;

struct FlightControlOffset {
    int32_t throttle = 0;     // + 上 - 下
    int32_t roll = 0;         // + 右 - 左
    int32_t pitch = 0;        // + 前 - 后
    int32_t yaw = 0;          // + 顺时针 - 逆时针
};

struct stickControlConfig {
    int stickControlCenter = 1024;    // 摇杆中位值
    int moveStep = 400;               // 移动基础步长
    int rotateStep = 205;             // 旋转基础步长
    float moveSpeed = 2.0;            // m/s
    float rotateSpeed = 5.0;          // deg/s
};

struct flightControlConfig {
    float base;
    float speed;
    bool moveFlag;
    // 移动前的坐标点
    Point startPoint;
};

enum class Action {
    TAKEOFF = 1,
    LAND,
    GOHOME,
    MOVE,
    TURN = 6,
    FLYTO_POINT,
    CONTINUOUS_MOVE = 8,
    STOP_MOVE
};

enum class MoveMode {
    UP = 1,   // 向上
    DOWN,     // 向下
    LEFT,     // 向左
    RIGHT,    // 向右
    FORWARD,  // 向前
    BACKWARD, // 向后
};

enum class TurnMode {
    CLOCKWISE = 1,    // 逆时针
    COUNTERCLOCKWISE, // 顺时针
};

/**
 * @brief 飞行控制模块实现（Pilot2）
 *
 * 对应设计文档3.3节：takeoff_to_point/fly_to_point/fly_to_point_update/return_home/
 * stick_control 五个DJI官方method，字段跟机场3 flytoController.cpp 基本一致，直接照搬。
 *
 * MOVE/TURN/CONTINUOUS_MOVE/STOP_MOVE 这套"action码"是内部平台(flight/control topic)
 * 接入协议，非DJI官方接口，本次移植原样保留，内部仍是通过 stick_control 下发。
 *
 * 跟机场3版本的关键差异（均因设计文档第1节"范围收窄"——DRC链路管理归控制平台另一组实现，
 * 本项目不重复开发）：
 * - 去掉了 enterDrcControl()/心跳线程(startHeartbeatThread等)：本模块不再主动建立/维持DRC链路，
 *   ensureFlightControlAndMode() 改成只读查 DeviceMain::getDrcState()。
 * - 去掉了 handleOsdData()/m_currentPoint（对应 drc/up 的 osd_info_push 事件）：该事件目前未启用
 *   （mqttsubMain.cpp 里对应 topic handler 仍是注释状态），飞行器实时位置改用 TrackMain::getUavPoint()。
 * - shouldExitDrcMode() 去掉了机场3专属的"飞机在舱内"分支，只保留"控制态已置空闲"这一条本地判断
 *   （不涉及真正断开DRC链路，那是控制平台的事），目前暂未挂到任何调用点上
 *   （设计文档第6节待确认事项3：是否需要补充新的停发信号，待定）。
 *
 * fly_to_point_stop/return_home_cancel（机场3没有的新方法）暂不实现，留到其它部分完成后再补充
 * （内部平台目前也没有触发这两个方法的action码）。
 */
class FlytoController {
public:
    FlytoController(FlytoRequestManager& request);
    ~FlytoController();

    void handleFlightControl(const std::string& msg);
    void handleFlytoProgress(const std::string& msg);

private:
    friend class FlytoControllerTestProxy;
    void sendResult(uint16_t successCode, uint16_t failedCode, bool ret,
                    const std::string& log, const std::string& errResult);
    // 处理起飞、返航指令
    void handleTakeoff();
    void handleGohome();

    void calculateTimingParameters(float distance, float speed,
                                   int& totalMs, int& fullCycles, int& remainingMs);
    bool executeCycleControl(const FlightControlOffset& offset, int fullCycles);
    bool executeTimedControl(const FlightControlOffset& offset, float distance, float speed);

    FlightControlOffset getMoveOffset(MoveMode mode);
    FlightControlOffset calculateMoveOffset(MoveMode mode, float speed);
    bool commonControlSend(const FlightControlOffset& offset);
    void handleMove(dji_cloud::flight_control_message& msg);
    FlightControlOffset getTurnOffset(TurnMode mode);
    void handleTurn(dji_cloud::flight_control_message& msg);

    void handleContinuousMove(dji_cloud::flight_control_message& msg);
    void handleStopMove(dji_cloud::flight_control_message& msg);

    bool validateCoordinates(Point p1, Point p2);
    double calculateDistance(Point p1, Point p2);
    double calculate_target_course(const Point& current, const Point& target);
    void handleFlytoPoint(dji_cloud::flight_control_message& msg);

    // 只读查询 DeviceMain::getDrcState()，不再主动建链（详见类注释）
    bool ensureFlightControlAndMode();

    // 目前暂未挂到任何调用点（设计文档第6节待确认事项3）
    bool shouldExitDrcMode();

private:
    FlytoRequestManager& m_requestManager;

    // 航线控制动作处理容器
    std::unordered_map<int, ActionHandler> m_actionHandlerMap;
    // 杆量控制配置
    stickControlConfig m_stickConfig;

    // 控制指令计数
    std::atomic<U64> m_controlSeq{0};
    // 超时时间（使用常量定义）
    static constexpr int m_timeoutMs = ModuleConstants::Timeout::DEFAULT_REQUEST_TIMEOUT;

    // 持续飞行状态标志
    std::atomic<bool> m_isContinuousMoving{false};
    // 持续飞行线程
    std::unique_ptr<std::thread> m_continuousMoveThread;
    // 持续飞行参数（线程间共享）
    struct ContinuousMoveParams {
        FlightControlOffset offset;
        int direction;
        std::mutex mutex;
        std::atomic<bool> active{false};
    };

    // 持续飞行参数（线程间共享）
    std::shared_ptr<ContinuousMoveParams> m_continuousParams;

    // 地球平均半径（WGS84），单位：米
    const double EARTH_RADIUS = 6371008.8;
    /**
     * @brief 将角度转换为弧度
     */
    inline double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }

    // flyto 执行状态缓存：由 fly_to_point_progress 事件更新，
    // 供 handleFlytoPoint 每次调用时查询并打印（仅记录日志，不影响下发）。
    // 注意：新成员一律追加在类末尾，避免改变已有成员偏移（与事件 ID 同理）。
    std::mutex  m_flytoStatusMutex;
    std::string m_lastSentFlyToId;            // 最近一次下发/收到的 fly_to_id
    std::string m_lastFlytoStatus;            // 最近一次执行状态(status)
    int         m_lastFlytoResult{0};
    int         m_lastFlytoWayPointIndex{0};
    double      m_lastFlytoRemainingDistance{0.0};
    double      m_lastFlytoRemainingTime{0.0};
    U64         m_lastFlytoProgressTime{0};   // 最近一次状态更新时间(ms)
};

#endif /* FLYTO_CONTROLLER_H_ */
