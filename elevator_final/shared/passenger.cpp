#include "passenger.h"

PassengerGenerator::PassengerGenerator(int floors, double rate, TrafficPattern tp)
    : totalFloors(floors), arrivalRate(rate), nextId(0), trafficGen(floors, tp)
{
}

std::vector<PassengerRecord> PassengerGenerator::generatePassengers(double currentTime, double dt)
{
    double lambda = arrivalRate * dt;
    int count = poissonRandom(lambda);
    return generateBatch(count, currentTime);
}

std::vector<PassengerRecord> PassengerGenerator::generateBatch(int count, double currentTime)
{
    std::vector<PassengerRecord> result;
    for (int i = 0; i < count; i++)
    {
        PassengerRecord p;
        p.id = nextId++;
        p.arrivalTime = currentTime + randomDouble(0.0, 0.5);
        p.boardTime = -1.0;
        p.alightTime = -1.0;
        p.assignedElevator = -1;

        // 使用TrafficGenerator生成起止楼层
        trafficGen.generate(p.sourceFloor, p.targetFloor);

        result.push_back(p);
    }
    return result;
}

void PassengerGenerator::setTrafficPattern(TrafficPattern tp)
{
    trafficGen.pattern = tp;
}

int PassengerGenerator::randomFloorExcept(int exclude) const
{
    int floor;
    do
    {
        floor = randomInt(1, totalFloors);
    } while (floor == exclude);
    return floor;
}
