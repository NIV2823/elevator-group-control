#ifndef NEAREST_H
#define NEAREST_H

#include "../shared/scheduler.h"

class NearestScheduler : public IScheduler
{
public:
    std::vector<int> dispatch(
        const std::vector<HallCall>& calls,
        const std::vector<Elevator>& elevators,
        const SimulationConfig& cfg) override;

    std::string name() const override { return "最近电梯"; }
    std::string description() const override { return "将每个呼梯请求分配给预计到达时间最短的电梯"; }
};

#endif
