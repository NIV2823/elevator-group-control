#ifndef METRICS_H
#define METRICS_H

#include <vector>
#include <string>
#include "utils.h"

// ============================================================
// 各电梯详细指标
// ============================================================

struct ElevatorMetrics
{
    int elevatorId;
    double totalDistance;
    double emptyDistance;
    double loadedDistance;
    double emptyRunRatio;        // 空驶率(距离比)
    double emptyRunningTime;     // 空载运行时间(秒)
    double crowdedTime;          // 拥挤时间(>50%容量)(秒)
    int totalStops;
    int totalBoarded;
    int currentPax;              // 当前载客人数
    double utilizationRate;      // 利用率%
};

// ============================================================
// 综合评价指标结果
// ============================================================

struct MetricResult
{
    // ---- 服务质量指标 ----
    double avgWaitingTime;       // 平均候梯时间(s)
    double maxWaitingTime;       // 最大候梯时间(s)
    double avgTravelTime;        // 平均乘梯时间(s)
    double avgSystemTime;        // 平均系统时间(s)
    double waitingTimeStdDev;    // 候梯时间标准差

    int totalPassengers;         // 总乘客数
    int servedPassengers;        // 成功运送人数
    double serviceRate;          // 服务完成率(%)
    double longWaitRate;         // 长等待率(>60s)(%)
    double veryLongWaitRate;     // 极长等待率(>120s)(%)
    double p95WaitingTime;       // 95分位候梯时间(s)
    double dissatisfactionIndex; // 不满意指数

    // ---- 效率指标 ----
    double totalEnergy;          // 总运行距离(楼层数)
    double energyPerPax;         // 人均能耗(楼层/人)
    double throughput;           // 吞吐量(人/分钟)
    int totalStops;              // 总启停次数
    double emptyRunningTime;     // 总空转时间(秒)
    double crowdingTime;         // 拥挤时间(>50%容量)(秒)
    double crowdingRate;         // 拥挤时间占比(%)

    // ---- 负载均衡指标 ----
    double loadBalanceIndex;     // 负载均衡指数
    double avgUtilization;       // 平均电梯利用率(%)
    double maxUtilizationDiff;   // 最大利用率差异(%)

    // ---- 各电梯详情 ----
    std::vector<ElevatorMetrics> elevatorDetails;

    // ---- 综合评分 ----
    double comprehensiveScore;   // 综合得分(0-100)

    // ---- 元信息 ----
    std::string algorithmName;
    std::string scenarioName;
    double simulationDuration;

    void print() const;
    std::string toCSVHeader() const;
    std::string toCSV() const;
    std::string toShortReport() const;
};

MetricResult computeMetrics(
    const std::vector<PassengerRecord>& allPassengers,
    const std::vector<ElevatorMetrics>& elevDetails,
    double totalElevatorDistance,
    double simulationDuration,
    const std::string& algoName = "",
    const std::string& scenario = "");

double computeComprehensiveScore(const MetricResult& m);

void printComparisonReport(
    const std::vector<MetricResult>& results,
    const std::vector<std::string>& algoNames);

#endif
