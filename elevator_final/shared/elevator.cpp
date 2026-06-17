#include "elevator.h"
#include <cstdio>

Elevator::Elevator(int elevatorId, int cap, double floorTime, double stopT, double doorT)
    : id(elevatorId), currentFloor(1), direction(0), capacity(cap),
      floorTravelTime(floorTime), stopTime(stopT), doorTime(doorT),
      timeUntilFree(0.0), moveTimer(0.0),
      totalDistance(0.0), emptyDistance(0.0), loadedDistance(0.0),
      totalStops(0), totalBoarded(0),
      crowdedTime(0.0), emptyRunningTime(0.0)
{
}

void Elevator::addStop(int floor)
{
    if (std::find(targetFloors.begin(), targetFloors.end(), floor) == targetFloors.end())
    {
        targetFloors.push_back(floor);
    }
}

bool Elevator::hasStops() const
{
    return !targetFloors.empty();
}

int Elevator::nextTarget() const
{
    if (targetFloors.empty()) return currentFloor;
    return targetFloors.front();
}

double Elevator::estimatedTimeToFloor(int targetFloor) const
{
    if (timeUntilFree <= 0.0 && !hasStops())
    {
        return std::abs(targetFloor - currentFloor) * floorTravelTime;
    }

    double time = timeUntilFree;
    int lastFloor = currentFloor;
    if (!targetFloors.empty())
    {
        lastFloor = targetFloors.back();
    }
    time += std::abs(targetFloor - lastFloor) * floorTravelTime;

    if (targetFloors.empty())
        time += doorTime;
    else
        time += (double)targetFloors.size() * (stopTime + doorTime);

    return time;
}

int Elevator::stopsBetween(int from, int to) const
{
    if (targetFloors.empty()) return 0;
    int count = 0;
    int lo = from < to ? from : to;
    int hi = from < to ? to : from;
    for (int f : targetFloors)
        if (f > lo && f < hi) count++;
    return count;
}

double Elevator::totalWorkload() const
{
    if (targetFloors.empty()) return 0.0;
    double dist = 0.0;
    int prev = currentFloor;
    for (int f : targetFloors)
    {
        dist += std::abs(f - prev);
        prev = f;
    }
    return dist * floorTravelTime + (double)targetFloors.size() * (stopTime + doorTime);
}

int Elevator::pendingStopsCount() const
{
    return (int)targetFloors.size();
}

double Elevator::utilizationRate(double totalTime) const
{
    if (totalTime <= 0.0) return 0.0;
    return (totalDistance * floorTravelTime + totalStops * (stopTime + doorTime)) / totalTime * 100.0;
}

double Elevator::emptyRunRatio() const
{
    if (totalDistance <= 0.0) return 0.0;
    return emptyDistance / totalDistance;
}

void Elevator::stepUpdate(double dt)
{
    if (timeUntilFree > 0.0)
    {
        timeUntilFree -= dt;
        if (timeUntilFree < 0.0) timeUntilFree = 0.0;
        return;
    }

    if (targetFloors.empty())
    {
        direction = 0;
        return;
    }

    int target = targetFloors.front();

    if (currentFloor == target)
    {
        // 到达目标楼层
        targetFloors.erase(targetFloors.begin());
        timeUntilFree = stopTime + doorTime;
        totalStops++;
        // 记录停靠期间的拥挤时间
        if ((int)passengers.size() > capacity / 2)
            crowdedTime += stopTime + doorTime;
        if (targetFloors.empty())
            direction = 0;
        else
            determineDirection();
        return;
    }

    // 移动一层
    direction = (target > currentFloor) ? 1 : -1;
    currentFloor += direction;
    totalDistance += 1.0;
    if (passengers.empty())
    {
        emptyDistance += 1.0;
        emptyRunningTime += floorTravelTime;
    }
    else
        loadedDistance += 1.0;

    // 拥挤度追踪: >50%容量
    if ((int)passengers.size() > capacity / 2)
        crowdedTime += floorTravelTime;
}

std::vector<int> Elevator::unloadPassengers(double currentTime)
{
    std::vector<int> unloaded;
    auto it = passengers.begin();
    while (it != passengers.end())
    {
        if (it->targetFloor == currentFloor)
        {
            it->alightTime = currentTime;
            unloaded.push_back(it->id);
            it = passengers.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return unloaded;
}

void Elevator::boardPassenger(const PassengerRecord& p, double currentTime)
{
    PassengerRecord boarded = p;
    boarded.boardTime = currentTime;
    boarded.alightTime = 0.0;
    passengers.push_back(boarded);
    addStop(p.targetFloor);
    totalBoarded++;
}

void Elevator::determineDirection()
{
    if (targetFloors.empty())
    {
        direction = 0;
        return;
    }
    direction = (targetFloors.front() > currentFloor) ? 1 : -1;
}

void Elevator::printState() const
{
    printf("  Elevator %2d | Floor: %2d | Dir: %+2d | Pax: %2zu | Stops: %2zu | Busy: %.1fs\n",
           id, currentFloor, direction, passengers.size(), targetFloors.size(), timeUntilFree);
}

void Elevator::resetStats()
{
    totalDistance = 0.0;
    emptyDistance = 0.0;
    loadedDistance = 0.0;
    totalStops = 0;
    totalBoarded = 0;
    crowdedTime = 0.0;
    emptyRunningTime = 0.0;
}
