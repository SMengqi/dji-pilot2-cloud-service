#ifndef MODULE_CONSTANTS_H_
#define MODULE_CONSTANTS_H_

#include <cstdint>

/**
 * @file moduleConstants.h
 * @brief 模块常量定义
 * 
 * 统一管理所有模块使用的常量，避免魔法数字
 */

namespace ModuleConstants {

/**
 * @brief 超时时间常量（单位：毫秒）
 */
namespace Timeout {
    constexpr int DEFAULT_REQUEST_TIMEOUT = 8000;      ///< 默认请求超时时间（8秒）
    constexpr int WAYPOINT_TASK_TIMEOUT = 12000;        ///< 航线任务超时时间（12秒）
    constexpr int DRC_CONTROL_TIMEOUT = 60000;         ///< DRC控制超时时间（60秒）
    constexpr int KMZ_REQUEST_TIMEOUT = 8000;          ///< KMZ文件请求超时时间（8秒）
    constexpr int MQTT_CONNECT_TIMEOUT = 2000;         ///< MQTT连接超时时间（2秒）
    constexpr int MAIN_LOOP_SLEEP = 3600000000;        ///< 主循环休眠时间（微秒，1小时）
}

/**
 * @brief 线程池大小常量
 */
namespace ThreadPool {
    constexpr size_t FLYTO_THREAD_COUNT = 3;           ///< Flyto模块线程数
    constexpr size_t WAYPOINT_THREAD_COUNT = 4;        ///< Waypoint模块线程数
    constexpr size_t PAYLOAD_THREAD_COUNT = 2;         ///< Payload模块线程数
    constexpr size_t PAYLOAD_RESPONSE_THREAD_COUNT = 2;///< Payload模块响应线程数
    constexpr size_t DEVICE_THREAD_COUNT = 2;          ///< Device模块线程数
}

/**
 * @brief 控制模式相关常量
 */
namespace ControlMode {
    constexpr uint16_t TASK_STEP_GROUND = 5;           ///< 地面任务步骤码
    constexpr uint16_t TASK_STEP_AIR = 1;             ///< 空中任务步骤码
    constexpr uint16_t DRC_STATE_INACTIVE = 0;        ///< DRC状态：未激活
}

/**
 * @brief 时间相关常量
 */
namespace Time {
    constexpr int DRC_WAIT_RETRY_COUNT = 20;           ///< DRC模式等待重试次数
    constexpr int DRC_WAIT_INTERVAL_MS = 500;         ///< DRC模式等待间隔（毫秒）
    constexpr int TASK_SWITCH_DELAY_MS = 100;         ///< 任务切换延迟（毫秒）
}

/**
 * @brief 距离相关常量（单位：米）
 */
namespace Distance {
    constexpr float SHORT_DISTANCE_MIN = 2.5f;         ///< 短距离最小值
    constexpr float SHORT_DISTANCE_MAX = 6.0f;        ///< 短距离最大值
    constexpr float LONG_DISTANCE_THRESHOLD = 6.0f;    ///< 长距离阈值
    constexpr float DISTANCE_ADJUSTMENT = 3.0f;        ///< 距离调整值
}

/**
 * @brief 控制周期相关常量
 */
namespace ControlCycle {
    constexpr int CYCLE_DURATION_MS = 200;             ///< 控制周期时长（毫秒）
    constexpr int CHECK_INTERVAL_MS = 100;             ///< 检查间隔（毫秒）
    constexpr int CHECK_COUNT_PER_CYCLE = 2;           ///< 每个周期的检查次数
}

} // namespace ModuleConstants

#endif /* MODULE_CONSTANTS_H_ */
