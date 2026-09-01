#ifndef PL_UTILS_
#define PL_UTILS_

#include "pl.h"
#include "event.h"
#include "dji_cloud_api.pb.h"

#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>

/*timer duration in millisec*/
#define MAX_HEARTBEAT_CYCLE_NUMBER 5000

extern std::atomic<uint32_t> s_drcHeartbeatCount;
extern U32 ulHeartBeatTimerId;
extern U64 s_lastDrcControlTime;

void updateTransId(const std::string& transId);
std::string generate_uuid(void);
uint64_t get_milliseconds(void);

const char* getMsgIdDesc(uint32_t ulMsgId);

/************** 控制响应 **************/
// 飞行器状态上报
bool handleFlightStatus(uint16_t code, const std::string& msg, bool isAck = true);
// 飞行器控制结果上报
bool handleFlightResult(const std::string& result);
bool handleUavResult(uint16_t code, const std::string& msg, int result);

/******* UAV CONTROL STATE MACHINE *******/
typedef enum {
   IDLE,          // 空闲
   WAYPOINT_MODE, // 航线模式
   FLYTO_MODE     // 飞控模式
} E_BxtUavControlMode;

E_BxtUavControlMode getUavControlMode(void);

bool setUavControlMode(E_BxtUavControlMode mode);
const char* getControlModeDesc(void);
/******** UAV CONTROL STATE MACHINE ********/

/************** 指令飞行模式 进入、退出 **************/
void drcModeEnter(dji_cloud::services_down& message);
bool drcModeExit(void);

void drcUpHeartbeat(const std::string& msg);
bool drcDownHeartbeat(void);
bool drcDownHeartbeatWithSeq(uint32_t seq);


typedef enum {
    STATE_UAV_READY = 1,      // 飞行器控制就绪

    STATE_WAYPOINT_EXECUTE,   // 执行航线
    STATE_WAYPOINT_PAUSE,     // 暂停航线
    STATE_WAYPOINT_RESTORE,   // 恢复航线
    STATE_WAYPOINT_UNDO,      // 停止航线
    STATE_WAYPOINT_INDEX,     // 航线索引
    STATE_WAYPOINT_INTERRUPT, // 中断航线
    STATE_WAYPOINT_END,       // 航线结束
    STATE_WAYPOINT_THROW,     // 抛投
    STATE_WAYPOINT_THROW_END_TURN_BACK, // 抛投后返航

    STATE_FLYTO_TAKE_OFF = 50, // 起飞
    STATE_FLYTO_LANDING,       // 降落
    STATE_FLYTO_GO_HOME,       // 返航
    STATE_FLYTO_POSITION_MOVE, // 位置移动
    STATE_FLYTO_TRUN,          // 转弯
    STATE_FLYTO_POINT,         // 飞向目标点
    STATE_FLYTO_CONTINUOUS_MOVE,                  // 持续移动
    STATE_FLYTO_STOP_MOVE,                        // 停止移动

    STATE_CAMERA_ZOOM = 60,      // 相机变焦

    STATE_GIMBAL_CONTROL = 65,   // 云台控制

    STATE_WAYPOINT_WARN_INVALID_OPERATE = 100,    // 航线控制无效操作警告
    STATE_WAYPOINT_WARN_REPEAT_EXECUTE_OPERATE,   // 航线控制重复执行操作警告
    STATE_WAYPOINT_WARN_REPEAT_PAUSE_OPERATE,     // 航线控制重复暂停操作警告
    STATE_WAYPOINT_WARN_INVALID_PARAM,            // 航线控制无效参数警告
    STATE_FLIGHT_FLYTO_WARN_INVALID_OPERATE,      // 飞行控制无效操作警告
    STATE_FLIGHT_FLYTO_WARN_INVALID_PARAM,        // 飞行控制无效参数警告

    STATE_WAYPOINT_ERROR_LOAD_KMZ_FAILED = 200,   // 航线控制加载KMZ失败
    STATE_WAYPOINT_ERROR_UPLOAD_KMZ_FAILED,       // 航线控制上传KMZ失败
    STATE_WAYPOINT_ERROR_PAUSE_FAILED,            // 航线控制暂停失败
    STATE_WAYPOINT_ERROR_RESTORE_FAILED,          // 航线控制恢复失败
    STATE_WAYPOINT_ERROR_UNDO_FAILED,             // 航线控制停止失败
    STATE_WAYPOINT_ERROR_ACTION_FAILED,           // 航线控制执行失败
    STATE_PAYLOAD_ERROR_LIGHT_CONTROL_FAILED,     // 负载控制灯光失败

    STATE_FLYTO_TAKE_OFF_FAILED = 300,            // 起飞失败
    STATE_FLYTO_LANDING_FAILED,                   // 降落失败
    STATE_FLYTO_GO_HOME_FAILED,                   // 返航失败
    STATE_FLYTO_POSITION_MOVE_FAILED,             // 位置移动失败
    STATE_FLYTO_TRUN_FAILED,                      // 转弯失败
    STATE_FLYTO_OBTAIN_JOYSTATICK_CTRL_AUTHORITY_FAILED, // 获取遥控器控制权限失败
    STATE_FLYTO_POINT_FAILED,                     // 飞向目标点失败
    STATE_FLYTO_CONTINUOUS_MOVE_FAILED,           // 持续移动失败
    STATE_FLYTO_STOP_MOVE_FAILED,                 // 停止移动失败

    STATE_CAMERA_ZOOM_FAILED = 350,               // 相机变焦失败

    STATE_GIMBAL_CONTROL_FAILED = 360,            // 云台控制失败
    
} E_BxtStateMoudle;

#endif /* PL_UTILS_ */