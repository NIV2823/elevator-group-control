#include "gwo.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdio>

using namespace std;

GWOScheduler::GWOScheduler(int popSize, int maxIter, int numElev)
    : populationSize(popSize), maxIterations(maxIter), numElevators(numElev),
    lowerBound(0.0), upperBound((double)numElev - 0.0001)
{
    alpha.fitness = numeric_limits<double>::max();
    beta.fitness = numeric_limits<double>::max();
    delta.fitness = numeric_limits<double>::max();
}

void GWOScheduler::initializePack(int dimension)
{
    pack.resize(populationSize);
    for (int i = 0; i < populationSize; i++)
    {
        pack[i].position.resize(dimension);
        for (int d = 0; d < dimension; d++)
        {
            pack[i].position[d] = randomDouble(lowerBound, upperBound);
        }
        pack[i].fitness = numeric_limits<double>::max();
    }
}

void GWOScheduler::evaluateFitness(
    const vector<HallCall>& calls,
    const vector<Elevator>& elevators,
    double floorTravelTime, double stopTime, double doorTime)
{
    for (int i = 0; i < populationSize; i++)
    {
        vector<int> assignment(calls.size());
        for (size_t j = 0; j < calls.size(); j++)
        {
            assignment[j] = clampInt((int)floor(pack[i].position[j]), 0, numElevators - 1);
        }
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

    if (beta.fitness == std::numeric_limits<double>::max())
    {
        beta = alpha;
    }
    if (delta.fitness == std::numeric_limits<double>::max())
    {
        delta = alpha;
    }
}

void GWOScheduler::updatePositions(double a, int dimension)
{
    for (int i = 0; i < populationSize; i++)
    {
        for (int d = 0; d < dimension; d++)
        {
            double A1_val = 2.0 * a * randomDouble(0.0, 1.0) - a;
            double C1_val = 2.0 * randomDouble(0.0, 1.0);
            double D_alpha = std::abs(C1_val * alpha.position[d] - pack[i].position[d]);
            double X1 = alpha.position[d] - A1_val * D_alpha;

            double A2_val = 2.0 * a * randomDouble(0.0, 1.0) - a;
            double C2_val = 2.0 * randomDouble(0.0, 1.0);
            double D_beta = std::abs(C2_val * beta.position[d] - pack[i].position[d]);
            double X2 = beta.position[d] - A2_val * D_beta;

            double A3_val = 2.0 * a * randomDouble(0.0, 1.0) - a;
            double C3_val = 2.0 * randomDouble(0.0, 1.0);
            double D_delta = std::abs(C3_val * delta.position[d] - pack[i].position[d]);
            double X3 = delta.position[d] - A3_val * D_delta;

            pack[i].position[d] = (X1 + X2 + X3) / 3.0;
            pack[i].position[d] = clamp(pack[i].position[d], lowerBound, upperBound);
        }
    }
}

double GWOScheduler::estimateFitness(
    const vector<int>& assignment,
    const vector<HallCall>& calls,
    const vector<Elevator>& elevators,
    double floorTravelTime, double stopTime, double doorTime)
{
    double totalWaitTime = 0.0;
    double totalTravelTime = 0.0;
    double totalEnergy = 0.0;

    std::vector<double> elevatorExtraTime(numElevators, 0.0);
    std::vector<int> elevatorLastFloor(numElevators);
    for (int e = 0; e < numElevators; e++)
    {
        elevatorExtraTime[e] = elevators[e].timeUntilFree;
        elevatorLastFloor[e] = elevators[e].currentFloor;
    }

    for (size_t i = 0; i < calls.size(); i++)
    {
        int elevatorId = assignment[i];
        const HallCall& call = calls[i];

        double travelToCall = std::abs(elevatorLastFloor[elevatorId] - call.floor)
            * floorTravelTime;
        double waitTime = elevatorExtraTime[elevatorId] + travelToCall;
        totalWaitTime += waitTime;

        double travelDist = std::abs(call.targetFloor - call.floor);
        double travelTime = travelDist * floorTravelTime + stopTime + doorTime;
        totalTravelTime += travelTime;

        elevatorExtraTime[elevatorId] = waitTime + stopTime + doorTime;
        elevatorLastFloor[elevatorId] = call.targetFloor;

        totalEnergy += travelToCall + travelDist;
    }

    double maxElevatorLoad = 0.0;
    for (int e = 0; e < numElevators; e++)
    {
        if (elevatorExtraTime[e] > maxElevatorLoad)
            maxElevatorLoad = elevatorExtraTime[e];
    }

    double w1 = 0.35;
    double w2 = 0.25;
    double w3 = 0.20;
    double w4 = 0.20;

    double fitness = w1 * totalWaitTime + w2 * totalTravelTime
        + w3 * totalEnergy + w4 * maxElevatorLoad;

    return fitness;
}

std::vector<int> GWOScheduler::optimize(
    const std::vector<HallCall>& calls,
    const std::vector<Elevator>& elevators,
    double floorTravelTime,
    double stopTime,
    double doorTime)
{
    int dimension = (int)calls.size();

    if (dimension == 0)
    {
        return std::vector<int>();
    }

    if (numElevators == 1)
    {
        return std::vector<int>(dimension, 0);
    }

    alpha.fitness = std::numeric_limits<double>::max();
    beta.fitness = std::numeric_limits<double>::max();
    delta.fitness = std::numeric_limits<double>::max();
    alpha.position.clear();
    beta.position.clear();
    delta.position.clear();

    initializePack(dimension);
    evaluateFitness(calls, elevators, floorTravelTime, stopTime, doorTime);
    updateHierarchy();

    for (int iter = 0; iter < maxIterations; iter++)
    {
        double a = 2.0 - 2.0 * ((double)iter / (double)maxIterations);

        updatePositions(a, dimension);
        evaluateFitness(calls, elevators, floorTravelTime, stopTime, doorTime);
        updateHierarchy();
    }

    std::vector<int> bestAssignment(dimension);
    for (int i = 0; i < dimension; i++)
    {
        bestAssignment[i] = clampInt((int)std::floor(alpha.position[i]), 0, numElevators - 1);
    }

    return bestAssignment;
}