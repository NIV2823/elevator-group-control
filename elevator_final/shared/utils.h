#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>

// ============================================================
// 基础数据结构
// ============================================================

struct HallCall
{
    int floor;              // 呼梯楼层
    int direction;          // 1=上行, -1=下行
    int passengerId;        // 乘客ID
    int targetFloor;        // 目标楼层
    double arrivalTime;     // 到达时间
};

struct PassengerRecord
{
    int id;
    int sourceFloor;
    int targetFloor;
    double arrivalTime;
    double boardTime;       // -1表示未登梯
    double alightTime;      // -1表示未下梯
    int assignedElevator;   // -1表示未分配
};

// ============================================================
// 交通模式枚举
// ============================================================

enum class TrafficPattern
{
    UNIFORM,        // 均匀随机
    UP_PEAK,        // 上行高峰 (早高峰)
    DOWN_PEAK,      // 下行高峰 (晚高峰)
    BIDIRECTIONAL,  // 双向高峰 (午餐时间)
    INTER_FLOOR,    // 层间随机
    CUSTOM          // 自定义
};

inline const char* trafficPatternName(TrafficPattern tp)
{
    switch (tp)
    {
    case TrafficPattern::UNIFORM:       return "均匀随机";
    case TrafficPattern::UP_PEAK:       return "上行高峰";
    case TrafficPattern::DOWN_PEAK:     return "下行高峰";
    case TrafficPattern::BIDIRECTIONAL: return "双向高峰";
    case TrafficPattern::INTER_FLOOR:   return "层间随机";
    case TrafficPattern::CUSTOM:        return "自定义";
    default: return "未知";
    }
}

// ============================================================
// 仿真配置
// ============================================================

struct SimulationConfig
{
    int totalFloors = 15;
    int numElevators = 4;
    int elevatorCapacity = 10;
    double simulationDuration = 600.0;    // 秒
    double passengerArrivalRate = 0.5;    // 人/秒
    double floorTravelTime = 2.0;         // 每层行程时间(秒)
    double stopTime = 3.0;                // 停靠时间(秒)
    double doorTime = 2.0;                // 开关门时间(秒)
    double gwoInterval = 5.0;             // GWO调度间隔(秒)
    int gwoPopulation = 30;
    int gwoIterations = 50;
    TrafficPattern trafficPattern = TrafficPattern::UNIFORM;
    double peakFactor = 2.0;              // 高峰期到达率倍率
    double peakStart = 0.0;               // 高峰期开始(秒)
    double peakEnd = 0.0;                 // 高峰期结束(秒), 0=无高峰
};

// ============================================================
// 随机数工具函数
// ============================================================

double randomDouble(double minVal, double maxVal);
int randomInt(int minVal, int maxVal);
int poissonRandom(double lambda);
double clamp(double val, double lo, double hi);
int clampInt(int val, int lo, double hi);
double randomDouble();
void reseedRandom(unsigned int seed);

// ============================================================
// 统计工具
// ============================================================

inline double percentile(std::vector<double> data, double pct)
{
    if (data.empty()) return 0.0;
    std::sort(data.begin(), data.end());
    int idx = (int)(pct / 100.0 * (data.size() - 1));
    idx = clampInt(idx, 0, (int)data.size() - 1);
    return data[idx];
}

inline double standardDeviation(const std::vector<double>& data, double mean)
{
    if (data.size() <= 1) return 0.0;
    double sumSq = 0.0;
    for (double v : data) sumSq += (v - mean) * (v - mean);
    return std::sqrt(sumSq / data.size());
}

inline double meanValue(const std::vector<double>& data)
{
    if (data.empty()) return 0.0;
    double sum = 0.0;
    for (double v : data) sum += v;
    return sum / data.size();
}

#endif
