#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <vector>
#include <queue>
#include <memory>
#include "utils.h"
#include "elevator.h"
#include "passenger.h"
#include "scheduler.h"
#include "metrics.h"

class Simulator
{
public:
    SimulationConfig config;
    std::vector<Elevator> elevators;
    PassengerGenerator generator;
    std::unique_ptr<IScheduler> scheduler;

    std::vector<PassengerRecord> allPassengers;
    std::vector<PassengerRecord> waitingPassengers;
    double totalElevatorDistance;
    double currentTime;
    double lastScheduleTime;

    Simulator(const SimulationConfig& cfg,
              std::unique_ptr<IScheduler> sched = nullptr);

    // 运行完整仿真
    void run(bool verbose = true);

    // 步进仿真 (用于GUI实时更新)
    void stepInit();
    bool stepUpdate(double dt);

    // 获取当前指标
    MetricResult getMetrics() const;

    // 获取当前各电梯统计
    std::vector<ElevatorMetrics> getElevatorDetails() const;

    // 公开方法（GUI用）
    void generateNewPassengers();
    void processHallCalls();
    void updateElevators(double dt);
    void boardWaitingPassengers();

    // 动态到达率调整
    double getCurrentArrivalRate() const;

private:
    HallCall passengerToHallCall(const PassengerRecord& p) const;
};

#endif
