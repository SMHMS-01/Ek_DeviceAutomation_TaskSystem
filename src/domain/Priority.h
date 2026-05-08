#pragma once

namespace device_automation::domain
{

/**
 * @brief 任务优先级
 * @details 值越小，优先级越高
 */
enum class Priority : int
{
    Critical = 0,  ///< 关键任务，应立即执行
    High = 1,      ///< 高优先级
    Normal = 2,    ///< 普通优先级（默认）
    Low = 3,       ///< 低优先级
    Background = 4 ///< 后台任务
};

/**
 * @brief 比较优先级
 * @return 如果 a 优先级更高（值更小），返回 true
 */
inline bool operator<(Priority a, Priority b)
{
    return static_cast<int>(a) < static_cast<int>(b);
}

/**
 * @brief 比较优先级
 */
inline bool operator<=(Priority a, Priority b)
{
    return static_cast<int>(a) <= static_cast<int>(b);
}

/**
 * @brief 比较优先级
 */
inline bool operator>(Priority a, Priority b)
{
    return static_cast<int>(a) > static_cast<int>(b);
}

/**
 * @brief 比较优先级
 */
inline bool operator>=(Priority a, Priority b)
{
    return static_cast<int>(a) >= static_cast<int>(b);
}

} // namespace device_automation::domain
