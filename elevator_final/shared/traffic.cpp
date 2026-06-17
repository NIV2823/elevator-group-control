#include "traffic.h"
#include <cmath>

TrafficGenerator::TrafficGenerator(int floors, TrafficPattern tp)
    : totalFloors(floors), pattern(tp),
      upPeakLobbyProb(0.80),
      downPeakLobbyProb(0.80),
      interLowFloor(3), interHighFloor(floors),
      interFloorProb(0.70)
{
    if (interHighFloor < 3) interHighFloor = floors;
}

int TrafficGenerator::randomFloorUniform() const
{
    return randomInt(1, totalFloors);
}

int TrafficGenerator::randomFloorExcept(int exclude) const
{
    if (totalFloors <= 1) return 1;
    int floor;
    do
    {
        floor = randomInt(1, totalFloors);
    } while (floor == exclude);
    return floor;
}

void TrafficGenerator::setUpPeakParams(double lobbyProb)
{
    upPeakLobbyProb = clamp(lobbyProb, 0.0, 1.0);
}

void TrafficGenerator::setDownPeakParams(double toLobbyProb)
{
    downPeakLobbyProb = clamp(toLobbyProb, 0.0, 1.0);
}

void TrafficGenerator::setInterFloorParams(int lowFloor, int highFloor, double interProb)
{
    interLowFloor = clampInt(lowFloor, 1, totalFloors);
    interHighFloor = clampInt(highFloor, interLowFloor, totalFloors);
    interFloorProb = clamp(interProb, 0.0, 1.0);
}

void TrafficGenerator::generate(int& sourceFloor, int& targetFloor) const
{
    switch (pattern)
    {
    case TrafficPattern::UNIFORM:
    {
        sourceFloor = randomFloorUniform();
        targetFloor = randomFloorExcept(sourceFloor);
        break;
    }

    case TrafficPattern::UP_PEAK:
    {
        // 80%从1楼出发去各楼层, 20%均匀随机
        if (randomDouble() < upPeakLobbyProb)
        {
            sourceFloor = 1;
            targetFloor = randomInt(2, totalFloors);
        }
        else
        {
            sourceFloor = randomFloorUniform();
            targetFloor = randomFloorExcept(sourceFloor);
        }
        break;
    }

    case TrafficPattern::DOWN_PEAK:
    {
        // 80%从各楼层去1楼, 20%均匀随机
        if (randomDouble() < downPeakLobbyProb)
        {
            sourceFloor = randomInt(2, totalFloors);
            targetFloor = 1;
        }
        else
        {
            sourceFloor = randomFloorUniform();
            targetFloor = randomFloorExcept(sourceFloor);
        }
        break;
    }

    case TrafficPattern::BIDIRECTIONAL:
    {
        // 50%上行高峰 + 50%下行高峰
        if (randomDouble() < 0.50)
        {
            sourceFloor = 1;
            targetFloor = randomInt(2, totalFloors);
        }
        else
        {
            sourceFloor = randomInt(2, totalFloors);
            targetFloor = 1;
        }
        break;
    }

    case TrafficPattern::INTER_FLOOR:
    {
        // 主要在interLowFloor~interHighFloor之间移动
        if (randomDouble() < interFloorProb)
        {
            sourceFloor = randomInt(interLowFloor, interHighFloor);
            targetFloor = randomFloorExcept(sourceFloor);
            // 限制目标也在中高层
            if (randomDouble() < 0.60)
            {
                targetFloor = randomInt(interLowFloor, interHighFloor);
                if (targetFloor == sourceFloor)
                    targetFloor = sourceFloor + (sourceFloor < interHighFloor ? 1 : -1);
            }
        }
        else
        {
            sourceFloor = randomFloorUniform();
            targetFloor = randomFloorExcept(sourceFloor);
        }
        break;
    }

    case TrafficPattern::CUSTOM:
    default:
    {
        sourceFloor = randomFloorUniform();
        targetFloor = randomFloorExcept(sourceFloor);
        break;
    }
    }
}

double TrafficGenerator::getRateMultiplier(double currentTime,
                                            double peakStart, double peakEnd,
                                            double peakFactor) const
{
    if (peakStart <= 0.0 || peakEnd <= 0.0 || peakEnd <= peakStart)
        return 1.0;

    if (currentTime >= peakStart && currentTime <= peakEnd)
    {
        // 高斯形高峰期曲线
        double mid = (peakStart + peakEnd) / 2.0;
        double sigma = (peakEnd - peakStart) / 4.0;
        double t = (currentTime - mid) / sigma;
        double base = std::exp(-0.5 * t * t);
        return 1.0 + (peakFactor - 1.0) * base;
    }
    return 1.0;
}
