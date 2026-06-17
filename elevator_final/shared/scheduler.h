#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <string>
#include <memory>
#include "utils.h"
#include "elevator.h"

// ============================================================
// 调度器抽象接口
// ============================================================

class IScheduler
{
public:
    virtual ~IScheduler() = default;

    // 核心调度：为每个呼梯请求分配电梯ID
    virtual std::vector<int> dispatch(
        const std::vector<HallCall>& calls,
        const std::vector<Elevator>& elevators,
        const SimulationConfig& cfg) = 0;

    virtual std::string name() const = 0;
    virtual std::string description() const { return ""; }

    virtual void reset() {}
};

// ============================================================
// 两种电梯调度算法
// ============================================================

enum class SchedulerType
{
    NEAREST,    // 最近电梯（基准算法）
    GWO         // 灰狼优化算法
};

inline const char* schedulerTypeName(SchedulerType t)
{
    switch (t)
    {
    case SchedulerType::NEAREST: return "最近电梯调度";
    case SchedulerType::GWO:     return "GWO灰狼优化";
    default: return "未知";
    }
}

inline const char* schedulerTypeShortName(SchedulerType t)
{
    switch (t)
    {
    case SchedulerType::NEAREST: return "最近电梯";
    case SchedulerType::GWO:     return "GWO";
    default: return "未知";
    }
}

std::unique_ptr<IScheduler> createScheduler(SchedulerType type, const SimulationConfig& cfg);

#endif
