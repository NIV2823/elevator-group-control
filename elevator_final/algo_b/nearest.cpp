#include "nearest.h"
#include <limits>

std::vector<int> NearestScheduler::dispatch(
    const std::vector<HallCall>& calls,
    const std::vector<Elevator>& elevators,
    const SimulationConfig& cfg)
{
    std::vector<int> assignment(calls.size());
    int numElev = (int)elevators.size();

    for (size_t i = 0; i < calls.size(); i++)
    {
        int bestElev = 0;
        double bestTime = std::numeric_limits<double>::max();

        for (int e = 0; e < numElev; e++)
        {
            // 检查容量
            if ((int)elevators[e].passengers.size() >= cfg.elevatorCapacity)
                continue;

            double time = elevators[e].estimatedTimeToFloor(calls[i].floor);

            // 方向一致性加分（同向电梯优先级略高）
            int callDir = calls[i].direction;
            int elevDir = elevators[e].direction;
            if (elevDir != 0 && callDir != elevDir)
                time += 5.0; // 反向惩罚

            if (time < bestTime)
            {
                bestTime = time;
                bestElev = e;
            }
        }
        assignment[i] = bestElev;
    }
    return assignment;
}
