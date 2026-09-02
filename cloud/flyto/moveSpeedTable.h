#ifndef MOVE_SPEED_TABLE_H_
#define MOVE_SPEED_TABLE_H_

#include <array>
#include <cstddef>

namespace flyto_speed {

struct SpeedOffsetPoint { float speed; int offset; };

// 水平(roll/pitch)——实测标定，数值不变
constexpr std::array<SpeedOffsetPoint, 15> kHorizontalSpeedMap = {{
    {1,165},{2,220},{3,260},{4,295},{5,330},{6,370},{7,410},{8,450},
    {9,490},{10,530},{11,570},{12,600},{13,625},{14,645},{15,660}
}};

// 垂直爬升(throttle +)——实测标定（送N→约N m/s；满杆660≈6.0m/s；升降实测一致）
constexpr std::array<SpeedOffsetPoint, 6> kAscendSpeedMap = {{
    {1,272},{2,378},{3,463},{4,536},{5,603},{6,660}
}};

// 垂直下降(throttle -)——实测标定（与爬升一致；如后续发现降速更软再单独调）
constexpr std::array<SpeedOffsetPoint, 6> kDescendSpeedMap = {{
    {1,272},{2,378},{3,463},{4,536},{5,603},{6,660}
}};

constexpr float kHorizontalMaxSpeed = 15.0f;
constexpr float kAscendMaxSpeed     = 6.0f;   // 满杆 660 实测垂直 ≈6.0m/s
constexpr float kDescendMaxSpeed    = 6.0f;   // 满杆 660 实测垂直 ≈6.0m/s

// 通用查表：升序表中取首个 speed<=表项的 offset；
// 低于首档取首档，高于末档兜底为末档(最大)。等价于原 calculateMoveOffset 行为。
inline int speedToOffset(float speed, const SpeedOffsetPoint* table, std::size_t n) {
    if (n == 0) return 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (speed <= table[i].speed) return table[i].offset;
    }
    return table[n - 1].offset;
}

template <std::size_t N>
inline int speedToOffset(float speed, const std::array<SpeedOffsetPoint, N>& t) {
    return speedToOffset(speed, t.data(), t.size());
}

} // namespace flyto_speed

#endif /* MOVE_SPEED_TABLE_H_ */
