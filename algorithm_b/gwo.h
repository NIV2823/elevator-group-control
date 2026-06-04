#ifndef GWO_H
#define GWO_H

#include <vector>
#include "utils.h"
#include "elevator.h"

struct Wolf
{
    std::vector<double> position;
    double fitness;
};

class GWOScheduler
{
public:
    int populationSize;
    int maxIterations;
    int numElevators;
    double lowerBound;
    double upperBound;

    Wolf alpha;
    Wolf beta;
    Wolf delta;
    std::vector<Wolf> pack;

    GWOScheduler(int popSize, int maxIter, int numElev);

    std::vector<int> optimize(
        const std::vector<HallCall>& calls,
        const std::vector<Elevator>& elevators,
        double floorTravelTime,
        double stopTime,
        double doorTime);

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