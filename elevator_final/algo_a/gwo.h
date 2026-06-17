#ifndef GWO_H
#define GWO_H

#include "../shared/scheduler.h"

struct Wolf
{
    std::vector<double> position;
    double fitness;
};

class GWOScheduler : public IScheduler
{
public:
    int populationSize;
    int maxIterations;
    int numElevators;
    double lowerBound;
    double upperBound;

    Wolf alpha, beta, delta;
    std::vector<Wolf> pack;

    GWOScheduler(int popSize = 30, int maxIter = 50, int numElev = 4);

    std::vector<int> dispatch(
        const std::vector<HallCall>& calls,
        const std::vector<Elevator>& elevators,
        const SimulationConfig& cfg) override;

    std::string name() const override { return "GWO灰狼优化"; }
    std::string description() const override { return "模拟灰狼群体狩猎行为，Alpha/Beta/Delta引导搜索，平衡全局探索与局部开发"; }
    void reset() override;

private:
    void initializePack(int dimension);
    void evaluateFitness(
        const std::vector<HallCall>& calls,
        const std::vector<Elevator>& elevators,
        double floorTravelTime, double stopTime, double doorTime);
    void updateHierarchy();
    void updatePositions(double a, int dimension);
    double estimateFitness(
        const std::vector<int>& assignment,
        const std::vector<HallCall>& calls,
        const std::vector<Elevator>& elevators,
        double floorTravelTime, double stopTime, double doorTime);
};

#endif
