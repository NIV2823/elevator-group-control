#ifndef PASSENGER_H
#define PASSENGER_H

#include <vector>
#include "utils.h"
#include "traffic.h"

class PassengerGenerator
{
public:
    int totalFloors;
    double arrivalRate;
    int nextId;
    TrafficGenerator trafficGen;

    PassengerGenerator(int floors, double rate,
                       TrafficPattern tp = TrafficPattern::UNIFORM);

    // 根据dt和当前时间生成乘客
    std::vector<PassengerRecord> generatePassengers(double currentTime, double dt);

    // 批量生成
    std::vector<PassengerRecord> generateBatch(int count, double currentTime);

    // 设置交通模式
    void setTrafficPattern(TrafficPattern tp);
    TrafficPattern getTrafficPattern() const { return trafficGen.pattern; }

private:
    int randomFloorExcept(int exclude) const;
};

#endif
