#define _CRT_SECURE_NO_WARNINGS

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define NOMINMAX

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <vector>
#include <string>
#include <memory>

#include "shared/simulator.h"
#include "shared/scheduler.h"
#include "shared/metrics.h"
#include "shared/gui.h"

// ============================================================
// 控制台批量分析模式
// ============================================================

SimulationConfig makeConfig(int floors, int elevators, int capacity,
                             double duration, double arrivalRate,
                             TrafficPattern traffic,
                             int gwoPop = 30, int gwoIter = 50)
{
    SimulationConfig cfg;
    cfg.totalFloors = floors;
    cfg.numElevators = elevators;
    cfg.elevatorCapacity = capacity;
    cfg.simulationDuration = duration;
    cfg.passengerArrivalRate = arrivalRate;
    cfg.floorTravelTime = 2.0;
    cfg.stopTime = 3.0;
    cfg.doorTime = 2.0;
    cfg.gwoInterval = 5.0;
    cfg.gwoPopulation = gwoPop;
    cfg.gwoIterations = gwoIter;
    cfg.trafficPattern = traffic;
    return cfg;
}

void runComparison(const SimulationConfig& cfg, const char* scenarioName,
                    std::vector<MetricResult>& allResults,
                    std::vector<std::string>& allNames)
{
    printf("\n");
    printf("################################################################\n");
    printf("  场景: %s\n", scenarioName);
    printf("################################################################\n");

    std::vector<MetricResult> results;
    std::vector<std::string> algoNames;

    // 两种算法对比: Nearest(基准) vs GWO(优化)
    SchedulerType types[] = { SchedulerType::NEAREST, SchedulerType::GWO };

    for (auto type : types)
    {
        auto sched = createScheduler(type, cfg);
        printf("\n  --- %s ---\n", schedulerTypeName(type));

        auto sim = std::make_unique<Simulator>(cfg, std::move(sched));
        sim->run(false);

        MetricResult m = sim->getMetrics();
        m.print();
        results.push_back(m);
        algoNames.push_back(schedulerTypeName(type));
        allResults.push_back(m);
        std::string fullName = std::string(scenarioName) + " - " + schedulerTypeName(type);
        allNames.push_back(fullName);
    }

    printf("\n  >>> 对比总结:\n");
    if (results.size() >= 2)
    {
        double improvement = results[1].comprehensiveScore - results[0].comprehensiveScore;
        printf("  GWO综合得分: %.1f | 最近电梯综合得分: %.1f\n",
               results[1].comprehensiveScore, results[0].comprehensiveScore);
        printf("  GWO候梯时间: %.1fs | 最近电梯候梯时间: %.1fs\n",
               results[1].avgWaitingTime, results[0].avgWaitingTime);
        printf("  GWO长等待率: %.1f%% | 最近电梯长等待率: %.1f%%\n",
               results[1].longWaitRate, results[0].longWaitRate);
        if (improvement > 0)
            printf("  GWO相比最近电梯提升了 %.1f 分\n", improvement);
        else
            printf("  GWO与最近电梯得分接近 (差 %.1f 分)\n", improvement);
    }
}

void exportCSV(const std::vector<MetricResult>& results,
               const std::vector<std::string>& names,
               const char* filename)
{
    FILE* f = fopen(filename, "w");
    if (!f)
    {
        printf("  警告: 无法写入 %s\n", filename);
        return;
    }
    fprintf(f, "Scenario,%s\n", results[0].toCSVHeader().c_str());
    for (size_t i = 0; i < results.size(); i++)
        fprintf(f, "%s,%s\n", names[i].c_str(), results[i].toCSV().c_str());
    fclose(f);
    printf("\n  结果已导出到: %s\n", filename);
}

void runBatchMode()
{
    printf("============================================================\n");
    printf("  电梯群控调度 — 批量性能分析\n");
    printf("  算法对比: GWO灰狼优化 vs 最近电梯调度(基准)\n");
    printf("============================================================\n\n");

    std::vector<MetricResult> allResults;
    std::vector<std::string> allNames;

    // 场景1: 小型建筑
    {
        SimulationConfig cfg = makeConfig(5, 2, 8, 600.0, 0.3, TrafficPattern::UNIFORM);
        runComparison(cfg, "小型建筑(5F,2E)", allResults, allNames);
    }

    // 场景2: 中型办公楼
    {
        SimulationConfig cfg = makeConfig(15, 4, 10, 1200.0, 0.6, TrafficPattern::UNIFORM);
        runComparison(cfg, "中型办公(15F,4E)", allResults, allNames);
    }

    // 场景3: 高层建筑
    {
        SimulationConfig cfg = makeConfig(30, 6, 12, 1800.0, 0.9, TrafficPattern::UNIFORM);
        runComparison(cfg, "高层建筑(30F,6E)", allResults, allNames);
    }

    // 场景4: 上行高峰
    {
        SimulationConfig cfg = makeConfig(15, 4, 10, 900.0, 0.8, TrafficPattern::UP_PEAK);
        runComparison(cfg, "上行高峰(15F,4E)", allResults, allNames);
    }

    // 场景5: 下行高峰
    {
        SimulationConfig cfg = makeConfig(15, 4, 10, 900.0, 0.8, TrafficPattern::DOWN_PEAK);
        runComparison(cfg, "下行高峰(15F,4E)", allResults, allNames);
    }

    // 场景6: 双向高峰
    {
        SimulationConfig cfg = makeConfig(15, 4, 10, 900.0, 0.8, TrafficPattern::BIDIRECTIONAL);
        runComparison(cfg, "双向高峰(15F,4E)", allResults, allNames);
    }

    // 场景7: 层间随机
    {
        SimulationConfig cfg = makeConfig(15, 4, 10, 900.0, 0.6, TrafficPattern::INTER_FLOOR);
        runComparison(cfg, "层间随机(15F,4E)", allResults, allNames);
    }

    // 导出CSV
    exportCSV(allResults, allNames, "elevator_results.csv");

    printf("\n============================================================\n");
    printf("  批量分析完成! 共运行 %zu 个场景.\n", allResults.size());
    printf("  结果已保存到 elevator_results.csv\n");
    printf("============================================================\n\n");
    printf("按 Enter 键退出...\n");
    getchar();
}

// ============================================================
// 主入口
// ============================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    srand((unsigned int)time(nullptr));

    std::string cmdLine(lpCmdLine);

    if (cmdLine.find("--help") != std::string::npos ||
        cmdLine.find("-h") != std::string::npos)
    {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        printf("电梯群控调度动态演示系统\n\n");
        printf("用法:\n");
        printf("  elevator_final.exe              启动GUI动态演示模式\n");
        printf("  elevator_final.exe --batch       运行批量分析(控制台)\n");
        printf("  elevator_final.exe --help        显示帮助\n\n");
        printf("支持算法: GWO灰狼优化 / 最近电梯调度(基准)\n");
        printf("支持交通: 均匀随机/上行高峰/下行高峰/双向高峰/层间随机\n\n");
        printf("按 Enter 键退出...\n");
        getchar();
        FreeConsole();
        return 0;
    }

    if (cmdLine.find("--batch") != std::string::npos)
    {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        runBatchMode();
        FreeConsole();
        return 0;
    }

    return runGUI(hInstance, nCmdShow);
}
