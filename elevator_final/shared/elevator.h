#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <vector>
#include <algorithm>
#include "utils.h"

class Elevator
{
public:
    int id;
    int currentFloor;
    int direction;           // 1=上, -1=下, 0=静止
    int capacity;
    double floorTravelTime;
    double stopTime;
    double doorTime;
    double timeUntilFree;    // 剩余忙碌时间
    double moveTimer;        // 楼层间移动计时器

    std::vector<int> targetFloors;             // 停靠队列
    std::vector<PassengerRecord> passengers;   // 当前载客

    // 统计数据
    double totalDistance;        // 总运行距离(楼层数)
    double emptyDistance;        // 空载运行距离
    double loadedDistance;       // 载客运行距离
    int totalStops;              // 总停靠次数
    int totalBoarded;            // 总登梯人数
    double crowdedTime;          // 拥挤时间(>50%容量)(秒)
    double emptyRunningTime;     // 空载运行时间(秒)

    Elevator(int elevatorId, int cap, double floorTime, double stopT, double doorT);

    void addStop(int floor);
    bool hasStops() const;
    int nextTarget() const;
    double estimatedTimeToFloor(int targetFloor) const;
    int stopsBetween(int from, int to) const;
    double totalWorkload() const;
    int pendingStopsCount() const;
    double utilizationRate(double totalTime) const;
    double emptyRunRatio() const;

    void stepUpdate(double dt);
    std::vector<int> unloadPassengers(double currentTime);
    void boardPassenger(const PassengerRecord& p, double currentTime);
    void determineDirection();
    void printState() const;
    void resetStats();
};

#endif
