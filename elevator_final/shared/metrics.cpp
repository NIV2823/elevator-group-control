#include "metrics.h"
#include <cstdio>
#include <cmath>
#include <sstream>
#include <algorithm>

MetricResult computeMetrics(
    const std::vector<PassengerRecord>& allPassengers,
    const std::vector<ElevatorMetrics>& elevDetails,
    double totalElevatorDistance,
    double simulationDuration,
    const std::string& algoName,
    const std::string& scenario)
{
    MetricResult m;
    m.algorithmName = algoName;
    m.scenarioName = scenario;
    m.simulationDuration = simulationDuration;
    m.totalPassengers = (int)allPassengers.size();
    m.servedPassengers = 0;
    m.totalEnergy = totalElevatorDistance;

    std::vector<double> waitTimes;
    std::vector<double> travelTimes;
    int longWaitCount = 0;
    int veryLongWaitCount = 0;

    for (const auto& p : allPassengers)
    {
        if (p.boardTime >= 0.0 && p.alightTime >= 0.0)
        {
            m.servedPassengers++;
            double wait = p.boardTime - p.arrivalTime;
            double travel = p.alightTime - p.boardTime;
            waitTimes.push_back(wait);
            travelTimes.push_back(travel);
            if (wait > 60.0) longWaitCount++;
            if (wait > 120.0) veryLongWaitCount++;
        }
    }

    // 基础统计
    if (m.servedPassengers > 0)
    {
        m.avgWaitingTime = meanValue(waitTimes);
        m.maxWaitingTime = *std::max_element(waitTimes.begin(), waitTimes.end());
        m.avgTravelTime = meanValue(travelTimes);
        m.avgSystemTime = m.avgWaitingTime + m.avgTravelTime;
        m.waitingTimeStdDev = standardDeviation(waitTimes, m.avgWaitingTime);
        m.p95WaitingTime = percentile(waitTimes, 95.0);
    }

    // 服务率
    m.serviceRate = (m.totalPassengers > 0)
        ? (double)m.servedPassengers / m.totalPassengers * 100.0 : 0.0;

    // 长等待率
    m.longWaitRate = (m.totalPassengers > 0)
        ? (double)longWaitCount / m.totalPassengers * 100.0 : 0.0;
    m.veryLongWaitRate = (m.totalPassengers > 0)
        ? (double)veryLongWaitCount / m.totalPassengers * 100.0 : 0.0;

    // 不满意指数 (平方惩罚: 对>30s的等待时间施加平方惩罚)
    {
        double dissatisfaction = 0.0;
        for (double w : waitTimes)
        {
            if (w > 30.0)
                dissatisfaction += (w - 30.0) * (w - 30.0);
        }
        m.dissatisfactionIndex = (m.servedPassengers > 0)
            ? dissatisfaction / m.servedPassengers : 0.0;
    }

    // 吞吐量
    m.throughput = (simulationDuration > 0)
        ? m.servedPassengers / (simulationDuration / 60.0) : 0.0;

    // 启停次数、空转时间、拥挤时间
    m.totalStops = 0;
    m.emptyRunningTime = 0.0;
    m.crowdingTime = 0.0;
    for (const auto& ed : elevDetails)
    {
        m.totalStops += ed.totalStops;
        m.emptyRunningTime += ed.emptyRunningTime;
        m.crowdingTime += ed.crowdedTime;
    }
    double totalElevatorTime = simulationDuration * (double)elevDetails.size();
    m.crowdingRate = (totalElevatorTime > 0)
        ? m.crowdingTime / totalElevatorTime * 100.0 : 0.0;

    // 负载均衡
    {
        std::vector<double> distances;
        double sumDist = 0.0;
        for (const auto& ed : elevDetails)
        {
            distances.push_back(ed.totalDistance);
            sumDist += ed.totalDistance;
        }
        double meanDist = distances.empty() ? 0.0 : sumDist / distances.size();
        double stdDevDist = standardDeviation(distances, meanDist);
        m.loadBalanceIndex = (meanDist > 0.01) ? stdDevDist / meanDist : 0.0;

        double sumUtil = 0.0;
        double maxUtil = 0.0;
        double minUtil = 1e9;
        for (const auto& ed : elevDetails)
        {
            sumUtil += ed.utilizationRate;
            if (ed.utilizationRate > maxUtil) maxUtil = ed.utilizationRate;
            if (ed.utilizationRate < minUtil) minUtil = ed.utilizationRate;
        }
        m.avgUtilization = elevDetails.empty() ? 0.0 : sumUtil / elevDetails.size();
        m.maxUtilizationDiff = maxUtil - minUtil;
    }

    m.elevatorDetails = elevDetails;

    // 综合评分
    m.comprehensiveScore = computeComprehensiveScore(m);

    return m;
}

double computeComprehensiveScore(const MetricResult& m)
{
    // 归一化评分：每个指标满分100，加权求和
    // 权重: 候梯30%, 乘梯15%, 服务率15%, 能耗10%, 负载均衡10%, 不满意10%, 长等待10%

    auto score = [](double val, double worst, double best) -> double
    {
        if (worst <= best) return 100.0;
        return 100.0 * (worst - val) / (worst - best);
    };

    // 各指标的最差-最优范围（经验值）
    double sWait = score(clamp(m.avgWaitingTime, 5.0, 60.0), 60.0, 5.0);
    double sTravel = score(clamp(m.avgTravelTime, 5.0, 80.0), 80.0, 5.0);
    double sService = m.serviceRate; // 越高越好
    double sEnergy = score(clamp(m.energyPerPax, 2.0, 20.0), 20.0, 2.0);
    double sLoadBal = score(clamp(m.loadBalanceIndex, 0.0, 1.0), 1.0, 0.0);
    double sDissat = score(clamp(m.dissatisfactionIndex, 0.0, 2000.0), 2000.0, 0.0);
    double sLongWait = score(clamp(m.longWaitRate, 0.0, 30.0), 30.0, 0.0);

    return 0.30 * sWait + 0.15 * sTravel + 0.15 * sService
         + 0.10 * sEnergy + 0.10 * sLoadBal + 0.10 * sDissat + 0.10 * sLongWait;
}

void MetricResult::print() const
{
    printf("\n");
    printf("============================================================\n");
    printf("  电梯群控综合评价报告 - %s\n", algorithmName.c_str());
    if (!scenarioName.empty())
        printf("  场景: %s\n", scenarioName.c_str());
    printf("============================================================\n");
    printf("\n  --- 服务质量指标 ---\n");
    printf("  总乘客数:              %6d\n", totalPassengers);
    printf("  已服务乘客数:          %6d\n", servedPassengers);
    printf("  服务完成率:            %6.1f %%\n", serviceRate);
    printf("  平均候梯时间:          %8.2f s\n", avgWaitingTime);
    printf("  最大候梯时间:          %8.2f s\n", maxWaitingTime);
    printf("  95分位候梯时间:        %8.2f s\n", p95WaitingTime);
    printf("  候梯时间标准差:        %8.2f s\n", waitingTimeStdDev);
    printf("  平均乘梯时间:          %8.2f s\n", avgTravelTime);
    printf("  平均系统时间:          %8.2f s\n", avgSystemTime);
    printf("  长等待率(>60s):        %7.1f %%\n", longWaitRate);
    printf("  极长等待率(>120s):     %7.1f %%\n", veryLongWaitRate);
    printf("  不满意指数:            %8.1f\n", dissatisfactionIndex);

    printf("\n  --- 效率指标 ---\n");
    printf("  总运行距离:            %8.1f 层\n", totalEnergy);
    printf("  人均能耗:              %8.2f 层/人\n", energyPerPax);
    printf("  吞吐量:                %8.1f 人/分钟\n", throughput);

    printf("\n  --- 负载均衡指标 ---\n");
    printf("  负载均衡指数:          %8.4f\n", loadBalanceIndex);
    printf("  平均利用率:            %7.1f %%\n", avgUtilization);
    printf("  最大利用率差异:        %7.1f %%\n", maxUtilizationDiff);

    if (!elevatorDetails.empty())
    {
        printf("\n  --- 各电梯详细指标 ---\n");
        printf("  %-4s %10s %10s %10s %8s %8s\n",
               "ID", "距离", "空驶", "载客", "停靠", "登梯");
        for (const auto& ed : elevatorDetails)
        {
            printf("  %-4d %10.1f %10.1f %10.1f %8d %8d\n",
                   ed.elevatorId, ed.totalDistance, ed.emptyDistance,
                   ed.loadedDistance, ed.totalStops, ed.totalBoarded);
        }
    }

    printf("\n  >>> 综合得分: %.1f / 100 <<<\n", comprehensiveScore);
    printf("============================================================\n");
}

std::string MetricResult::toCSVHeader() const
{
    return "Algorithm,Scenario,TotalPax,ServedPax,ServiceRate,AvgWait,MaxWait,"
           "P95Wait,WaitStdDev,AvgTravel,AvgSystem,LongWaitRate,VeryLongWaitRate,"
           "Dissatisfaction,TotalEnergy,EnergyPerPax,Throughput,LoadBalanceIndex,"
           "AvgUtilization,MaxUtilDiff,ComprehensiveScore";
}

std::string MetricResult::toCSV() const
{
    char buf[512];
    snprintf(buf, sizeof(buf),
             "%s,%s,%d,%d,%.1f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f,%.1f,%.2f,%.1f,%.4f,%.1f,%.1f,%.1f",
             algorithmName.c_str(), scenarioName.c_str(),
             totalPassengers, servedPassengers, serviceRate,
             avgWaitingTime, maxWaitingTime, p95WaitingTime, waitingTimeStdDev,
             avgTravelTime, avgSystemTime, longWaitRate, veryLongWaitRate,
             dissatisfactionIndex, totalEnergy, energyPerPax, throughput,
             loadBalanceIndex, avgUtilization, maxUtilizationDiff,
             comprehensiveScore);
    return std::string(buf);
}

std::string MetricResult::toShortReport() const
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "%s: 得分=%.1f 等待=%.1fs 系统=%.1fs 服务率=%.1f%% 长等待=%.1f%% 吞吐=%.1f",
             algorithmName.c_str(), comprehensiveScore,
             avgWaitingTime, avgSystemTime, serviceRate, longWaitRate, throughput);
    return std::string(buf);
}

void printComparisonReport(
    const std::vector<MetricResult>& results,
    const std::vector<std::string>& algoNames)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                        算法性能对比报告\n");
    printf("================================================================================\n");
    printf("%-16s %8s %8s %8s %8s %8s %8s %8s\n",
           "算法", "综合得分", "平均等待", "平均系统", "服务率%", "长等待%", "人均能耗", "吞吐量");
    printf("--------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < results.size(); i++)
    {
        const auto& m = results[i];
        printf("%-16s %8.1f %8.1fs %8.1fs %7.1f %7.1f %8.2f %7.1f\n",
               m.algorithmName.c_str(),
               m.comprehensiveScore,
               m.avgWaitingTime,
               m.avgSystemTime,
               m.serviceRate,
               m.longWaitRate,
               m.energyPerPax,
               m.throughput);
    }
    printf("================================================================================\n");

    // 找出最优
    int bestIdx = 0;
    for (size_t i = 1; i < results.size(); i++)
        if (results[i].comprehensiveScore > results[bestIdx].comprehensiveScore)
            bestIdx = (int)i;

    printf("  最佳算法: %s (综合得分: %.1f)\n",
           results[bestIdx].algorithmName.c_str(),
           results[bestIdx].comprehensiveScore);
    printf("================================================================================\n");
}
