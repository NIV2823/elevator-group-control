#include "scheduler.h"
#include "../algo_b/nearest.h"
#include "../algo_a/gwo.h"

std::unique_ptr<IScheduler> createScheduler(SchedulerType type, const SimulationConfig& cfg)
{
    switch (type)
    {
    case SchedulerType::NEAREST:
        return std::make_unique<NearestScheduler>();

    case SchedulerType::GWO:
        return std::make_unique<GWOScheduler>(
            cfg.gwoPopulation, cfg.gwoIterations, cfg.numElevators);

    default:
        return std::make_unique<NearestScheduler>();
    }
}
