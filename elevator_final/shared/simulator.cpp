#include "simulator.h"
#include "../algo_b/nearest.h"
#include <cstdio>
#include <cmath>

Simulator::Simulator(const SimulationConfig& cfg,
                     std::unique_ptr<IScheduler> sched)
    : config(cfg),
      generator(cfg.totalFloors, cfg.passengerArrivalRate, cfg.trafficPattern),
      totalElevatorDistance(0.0), currentTime(0.0), lastScheduleTime(0.0)
{
    for (int i = 0; i < config.numElevators; i++)
    {
        elevators.emplace_back(i, config.elevatorCapacity,
                               config.floorTravelTime, config.stopTime, config.doorTime);
    }

    if (sched)
        scheduler = std::move(sched);
    else
        scheduler = std::make_unique<NearestScheduler>();
}

void Simulator::stepInit()
{
    currentTime = 0.0;
    lastScheduleTime = 0.0;
    totalElevatorDistance = 0.0;
    allPassengers.clear();
    waitingPassengers.clear();
    for (auto& e : elevators)
        e.resetStats();
}

double Simulator::getCurrentArrivalRate() const
{
    return config.passengerArrivalRate
           * generator.trafficGen.getRateMultiplier(
               currentTime, config.peakStart, config.peakEnd, config.peakFactor);
}

void Simulator::run(bool verbose)
{
    double dt = 0.1;
    double reportInterval = 60.0;
    double nextReport = reportInterval;

    if (verbose)
    {
        printf("\n");
        printf("================================================================\n");
        printf("  电梯群控仿真 - 算法: %s\n", scheduler->name().c_str());
        printf("================================================================\n");
        printf("  楼层: %d | 电梯: %d | 容量: %d | 交通模式: %s\n",
               config.totalFloors, config.numElevators, config.elevatorCapacity,
               trafficPatternName(config.trafficPattern));
        printf("  时长: %.0fs | 到达率: %.2f pax/s\n",
               config.simulationDuration, config.passengerArrivalRate);
        printf("----------------------------------------------------------------\n");
    }

    while (currentTime < config.simulationDuration)
    {
        generateNewPassengers();

        if (currentTime - lastScheduleTime >= config.gwoInterval)
        {
            processHallCalls();
            lastScheduleTime = currentTime;
        }

        updateElevators(dt);
        boardWaitingPassengers();
        currentTime += dt;

        if (verbose && currentTime >= nextReport)
        {
            int served = 0;
            for (const auto& p : allPassengers)
                if (p.alightTime >= 0.0) served++;
            printf("  [t=%.0fs] 等待: %2zu | 已服务: %4d | 总: %4zu\n",
                   currentTime, waitingPassengers.size(), served, allPassengers.size());
            nextReport += reportInterval;
        }
    }

    if (verbose)
    {
        printf("----------------------------------------------------------------\n");
        printf("  仿真完成 at t=%.0fs\n", currentTime);
    }
}

bool Simulator::stepUpdate(double dt)
{
    if (currentTime >= config.simulationDuration)
        return false;

    generateNewPassengers();

    if (currentTime - lastScheduleTime >= config.gwoInterval)
    {
        processHallCalls();
        lastScheduleTime = currentTime;
    }

    updateElevators(dt);
    boardWaitingPassengers();
    currentTime += dt;

    return currentTime < config.simulationDuration;
}

void Simulator::generateNewPassengers()
{
    // 使用动态到达率
    double effectiveRate = getCurrentArrivalRate();
    double dt = 0.1;

    // 临时修改generator的到达率
    double savedRate = generator.arrivalRate;
    generator.arrivalRate = effectiveRate;

    auto newPax = generator.generatePassengers(currentTime, dt);
    for (auto& p : newPax)
    {
        allPassengers.push_back(p);
        waitingPassengers.push_back(p);
    }

    generator.arrivalRate = savedRate;
}

HallCall Simulator::passengerToHallCall(const PassengerRecord& p) const
{
    HallCall call;
    call.floor = p.sourceFloor;
    call.direction = (p.targetFloor > p.sourceFloor) ? 1 : -1;
    call.passengerId = p.id;
    call.targetFloor = p.targetFloor;
    call.arrivalTime = p.arrivalTime;
    return call;
}

void Simulator::processHallCalls()
{
    if (waitingPassengers.empty()) return;

    std::vector<HallCall> calls;
    for (const auto& p : waitingPassengers)
        calls.push_back(passengerToHallCall(p));

    std::vector<int> assignment = scheduler->dispatch(calls, elevators, config);

    for (size_t i = 0; i < assignment.size(); i++)
    {
        int elevId = assignment[i];
        waitingPassengers[i].assignedElevator = elevId;
        elevators[elevId].addStop(waitingPassengers[i].sourceFloor);
    }
}

void Simulator::updateElevators(double dt)
{
    for (auto& elev : elevators)
    {
        if (elev.timeUntilFree > 0.0)
        {
            elev.timeUntilFree -= dt;
            if (elev.timeUntilFree < 0.0) elev.timeUntilFree = 0.0;
            elev.moveTimer = 0.0;
            continue;
        }

        if (!elev.hasStops())
        {
            elev.direction = 0;
            elev.moveTimer = 0.0;
            continue;
        }

        int target = elev.nextTarget();

        if (elev.currentFloor == target)
        {
            // --- 到达目标楼层 ---
            auto unloadedIds = elev.unloadPassengers(currentTime);
            for (int pid : unloadedIds)
            {
                for (auto& p : allPassengers)
                {
                    if (p.id == pid)
                    {
                        p.alightTime = currentTime;
                        break;
                    }
                }
            }

            // 算法追踪: 启停次数
            elev.totalStops++;
            // 算法追踪: 停靠期间的拥挤时间
            if ((int)elev.passengers.size() > elev.capacity / 2)
                elev.crowdedTime += config.stopTime + config.doorTime;

            elev.targetFloors.erase(elev.targetFloors.begin());
            elev.timeUntilFree = config.stopTime + config.doorTime;
            elev.moveTimer = 0.0;
            elev.determineDirection();
        }
        else
        {
            elev.direction = (target > elev.currentFloor) ? 1 : -1;
            elev.moveTimer += dt;
            if (elev.moveTimer >= config.floorTravelTime)
            {
                elev.moveTimer -= config.floorTravelTime;
                elev.currentFloor += elev.direction;

                // 算法追踪: 各电梯运行距离
                elev.totalDistance += 1.0;
                totalElevatorDistance += 1.0;

                bool isEmpty = elev.passengers.empty();
                if (isEmpty)
                {
                    elev.emptyDistance += 1.0;
                    elev.emptyRunningTime += config.floorTravelTime;  // 空转时间
                }
                else
                {
                    elev.loadedDistance += 1.0;
                }

                // 算法追踪: 拥挤度>50%容量
                if ((int)elev.passengers.size() > elev.capacity / 2)
                    elev.crowdedTime += config.floorTravelTime;
            }
        }
    }
}

void Simulator::boardWaitingPassengers()
{
    auto it = waitingPassengers.begin();
    while (it != waitingPassengers.end())
    {
        if (it->assignedElevator < 0)
        {
            ++it;
            continue;
        }

        Elevator& elev = elevators[it->assignedElevator];

        if (elev.timeUntilFree > 0.0) { ++it; continue; }

        if (elev.currentFloor == it->sourceFloor &&
            (int)elev.passengers.size() < elev.capacity)
        {
            bool directionMatch = (elev.direction == 0) ||
                (elev.direction == 1 && it->targetFloor > it->sourceFloor) ||
                (elev.direction == -1 && it->targetFloor < it->sourceFloor);

            if (directionMatch || elev.passengers.empty())
            {
                elev.boardPassenger(*it, currentTime);

                for (auto& p : allPassengers)
                {
                    if (p.id == it->id)
                    {
                        p.boardTime = currentTime;
                        p.assignedElevator = it->assignedElevator;
                        break;
                    }
                }

                it = waitingPassengers.erase(it);
                continue;
            }
        }
        ++it;
    }
}

std::vector<ElevatorMetrics> Simulator::getElevatorDetails() const
{
    std::vector<ElevatorMetrics> details;
    for (const auto& elev : elevators)
    {
        ElevatorMetrics em;
        em.elevatorId = elev.id;
        em.totalDistance = elev.totalDistance;
        em.emptyDistance = elev.emptyDistance;
        em.loadedDistance = elev.loadedDistance;
        em.emptyRunRatio = elev.emptyRunRatio();
        em.emptyRunningTime = elev.emptyRunningTime;
        em.crowdedTime = elev.crowdedTime;
        em.totalStops = elev.totalStops;
        em.totalBoarded = elev.totalBoarded;
        em.currentPax = (int)elev.passengers.size();
        em.utilizationRate = elev.utilizationRate(currentTime);
        details.push_back(em);
    }
    return details;
}

MetricResult Simulator::getMetrics() const
{
    return computeMetrics(allPassengers, getElevatorDetails(),
                          totalElevatorDistance, config.simulationDuration,
                          scheduler ? scheduler->name() : "未知",
                          trafficPatternName(config.trafficPattern));
}
