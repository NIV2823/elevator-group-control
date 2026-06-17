#ifndef TRAFFIC_H
#define TRAFFIC_H

#include "utils.h"

// ============================================================
// 交通流生成器 — 根据模式生成 (sourceFloor, targetFloor)
// ============================================================

class TrafficGenerator
{
public:
    int totalFloors;
    TrafficPattern pattern;

    TrafficGenerator(int floors, TrafficPattern tp = TrafficPattern::UNIFORM);

    // 生成一对 (source, target)
    void generate(int& sourceFloor, int& targetFloor) const;

    // 设置上行高峰参数
    void setUpPeakParams(double lobbyProb = 0.80);

    // 设置下行高峰参数
    void setDownPeakParams(double toLobbyProb = 0.80);

    // 设置层间随机参数
    void setInterFloorParams(int lowFloor, int highFloor, double interProb = 0.70);

    // 获取当前到达率乘数 (根据时间和配置)
    double getRateMultiplier(double currentTime,
                             double peakStart, double peakEnd,
                             double peakFactor) const;

private:
    // 上行高峰参数
    double upPeakLobbyProb;

    // 下行高峰参数
    double downPeakLobbyProb;

    // 层间参数
    int interLowFloor;
    int interHighFloor;
    double interFloorProb;

    int randomFloorUniform() const;
    int randomFloorExcept(int exclude) const;
};

#endif
