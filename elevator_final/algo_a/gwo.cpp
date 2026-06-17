#include "gwo.h"
#include <cmath>
#include <limits>
#include <cstdio>

GWOScheduler::GWOScheduler(int popSize, int maxIter, int numElev)
    : populationSize(popSize), maxIterations(maxIter), numElevators(numElev),
      lowerBound(0.0), upperBound((double)numElev - 0.0001)
{
    alpha.fitness = std::numeric_limits<double>::max();
    beta.fitness = std::numeric_limits<double>::max();
    delta.fitness = std::numeric_limits<double>::max();
}

void GWOScheduler::reset()
{
    alpha.fitness = std::numeric_limits<double>::max();
    beta.fitness = std::numeric_limits<double>::max();
    delta.fitness = std::numeric_limits<double>::max();
    alpha.position.clear();
    beta.position.clear();
    delta.position.clear();
    pack.clear();
}

void GWOScheduler::initializePack(int dimension)
{
    pack.resize(populationSize);
    for (int i = 0; i < populationSize; i++)
    {
        pack[i].position.resize(dimension);
        for (int d = 0; d < dimension; d++)
            pack[i].position[d] = randomDouble(lowerBound, upperBound);
        pack[i].fitness = std::numeric_limits<double>::max();
    }
}

void GWOScheduler::evaluateFitness(
    const std::vector<HallCall>& calls,
    const std::vector<Elevator>& elevators,
    double floorTravelTime, double stopTime, double doorTime)
{
    for (int i = 0; i < populationSize; i++)
    {
        std::vector<int> assignment(calls.size());
        for (size_t j = 0; j < calls.size(); j++)
            assignment[j] = clampInt((int)std::floor(pack[i].position[j]), 0, numElevators - 1);
        pack[i].fitness = estimateFitness(assignment, calls, elevators,
                                           floorTravelTime, stopTime, doorTime);
    }
}

void GWOScheduler::updateHierarchy()
{
    for (int i = 0; i < populationSize; i++)
    {
        if (pack[i].fitness < alpha.fitness)
        {
            delta = beta;
            beta = alpha;
            alpha = pack[i];
        }
        else if (pack[i].fitness < beta.fitness)
        {
            delta = beta;
            beta = pack[i];
        }
        else if (pack[i].fitness < delta.fitness)
        {
            delta = pack[i];
        }
    }
    if (beta.fitness == std::numeric_limits<double>::max()) beta = alpha;
    if (delta.fitness == std::numeric_limits<double>::max()) delta = alpha;
}

void GWOScheduler::updatePositions(double a, int dimension)
{
    for (int i = 0; i < populationSize; i++)
    {
        for (int d = 0; d < dimension; d++)
        {
            double A1 = 2.0 * a * randomDouble() - a;
            double C1 = 2.0 * randomDouble();
            double D_alpha = std::abs(C1 * alpha.position[d] - pack[i].position[d]);
            double X1 = alpha.position[d] - A1 * D_alpha;

            double A2 = 2.0 * a * randomDouble() - a;
            double C2 = 2.0 * randomDouble();
            double D_beta = std::abs(C2 * beta.position[d] - pack[i].position[d]);
            double X2 = beta.position[d] - A2 * D_beta;

            double A3 = 2.0 * a * randomDouble() - a;
            double C3 = 2.0 * randomDouble();
            double D_delta = std::abs(C3 * delta.position[d] - pack[i].position[d]);
            double X3 = delta.position[d] - A3 * D_delta;

            pack[i].position[d] = (X1 + X2 + X3) / 3.0;
            pack[i].position[d] = clamp(pack[i].position[d], lowerBound, upperBound);
        }
    }
}

double GWOScheduler::estimateFitness(
    const std::vector<int>& assignment,
    const std::vector<HallCall>& calls,
    const std::vector<Elevator>& elevators,
    double floorTravelTime, double stopTime, double doorTime)
{
    double totalWaitTime = 0.0;
    double totalTravelTime = 0.0;
    double totalEnergy = 0.0;

    std::vector<double> elevatorExtraTime(numElevators, 0.0);
    std::vector<int> elevatorLastFloor(numElevators);
    std::vector<int> elevatorLoad(numElevators, 0);
    for (int e = 0; e < numElevators; e++)
    {
        elevatorExtraTime[e] = elevators[e].timeUntilFree;
        elevatorLastFloor[e] = elevators[e].currentFloor;
        elevatorLoad[e] = (int)elevators[e].passengers.size();
    }

    for (size_t i = 0; i < calls.size(); i++)
    {
        int eid = assignment[i];
        const HallCall& call = calls[i];

        double travelToCall = std::abs(elevatorLastFloor[eid] - call.floor) * floorTravelTime;
        double waitTime = elevatorExtraTime[eid] + travelToCall;
        totalWaitTime += waitTime;

        double travelDist = std::abs(call.targetFloor - call.floor);
        double travelTime = travelDist * floorTravelTime + stopTime + doorTime;
        totalTravelTime += travelTime;

        elevatorExtraTime[eid] = waitTime + stopTime + doorTime;
        elevatorLastFloor[eid] = call.targetFloor;
        elevatorLoad[eid]++;

        totalEnergy += travelToCall + travelDist;
    }

    // 负载均衡惩罚
    double loadMean = 0.0;
    for (int e = 0; e < numElevators; e++) loadMean += elevatorLoad[e];
    loadMean /= numElevators;
    double loadVar = 0.0;
    for (int e = 0; e < numElevators; e++)
        loadVar += (elevatorLoad[e] - loadMean) * (elevatorLoad[e] - loadMean);
    double loadBalance = std::sqrt(loadVar / numElevators);

    double w1 = 0.35, w2 = 0.25, w3 = 0.20, w4 = 0.20;
    return w1 * totalWaitTime + w2 * totalTravelTime + w3 * totalEnergy + w4 * loadBalance * 10.0;
}

std::vector<int> GWOScheduler::dispatch(
    const std::vector<HallCall>& calls,
    const std::vector<Elevator>& elevators,
    const SimulationConfig& cfg)
{
    int dimension = (int)calls.size();
    if (dimension == 0) return {};
    if (cfg.numElevators == 1) return std::vector<int>(dimension, 0);

    // 使用配置中的种群参数（如果已设置）
    if (cfg.gwoPopulation > 0) populationSize = cfg.gwoPopulation;
    if (cfg.gwoIterations > 0) maxIterations = cfg.gwoIterations;
    numElevators = cfg.numElevators;
    upperBound = (double)numElevators - 0.0001;

    alpha.fitness = std::numeric_limits<double>::max();
    beta.fitness = std::numeric_limits<double>::max();
    delta.fitness = std::numeric_limits<double>::max();
    alpha.position.clear();
    beta.position.clear();
    delta.position.clear();

    initializePack(dimension);
    evaluateFitness(calls, elevators, cfg.floorTravelTime, cfg.stopTime, cfg.doorTime);
    updateHierarchy();

    for (int iter = 0; iter < maxIterations; iter++)
    {
        double a = 2.0 - 2.0 * ((double)iter / (double)maxIterations);
        updatePositions(a, dimension);
        evaluateFitness(calls, elevators, cfg.floorTravelTime, cfg.stopTime, cfg.doorTime);
        updateHierarchy();
    }

    std::vector<int> bestAssignment(dimension);
    for (int i = 0; i < dimension; i++)
        bestAssignment[i] = clampInt((int)std::floor(alpha.position[i]), 0, numElevators - 1);

    return bestAssignment;
}
