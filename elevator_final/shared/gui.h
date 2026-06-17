#ifndef GUI_H
#define GUI_H

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <vector>
#include <memory>
#include <string>
#include "simulator.h"
#include "scheduler.h"
#include "metrics.h"

class ElevatorGUI
{
public:
    ElevatorGUI(HINSTANCE hInst);
    ~ElevatorGUI();

    bool init(int nCmdShow);
    int run();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static ElevatorGUI* s_instance;

    LRESULT handleCreate(HWND hWnd);
    LRESULT handleCommand(WPARAM wParam, LPARAM lParam);
    LRESULT handleTimer(WPARAM wParam);
    LRESULT handlePaint();
    void handleResize();

    void createControls();
    void showParamPanel();
    void showSimPanel();
    void showResultPanel();
    void readParameters();

    void startSimulation();
    void stopSimulation();
    void collectStats();

    // 绘制
    void drawParamPanel(HDC hdc, RECT& client);
    void drawSimulation(HDC hdc, RECT& client);
    void drawResults(HDC hdc, RECT& client);

    // 计算各楼层等待人数
    std::vector<int> countWaitingPerFloor() const;

    HINSTANCE m_hInstance;
    HWND m_hWnd;

    // 参数控件
    HWND m_hFloorsEdit, m_hElevEdit, m_hDurEdit, m_hCapEdit;
    HWND m_hArrivalEdit;
    HWND m_hTrafficCombo;
    HWND m_hAlgoCombo;      // 算法选择(2选1)
    HWND m_hStartBtn, m_hStopBtn, m_hResultBtn, m_hRestartBtn;

    // 字体
    HFONT m_hFontTitle, m_hFontNormal, m_hFontSmall, m_hFontTiny;

    // 颜色
    COLORREF m_elevColors[8];

    // 状态
    enum class UIState { PARAM, SIMULATING, RESULTS };
    UIState m_state;

    // 仿真参数
    int m_numFloors, m_numElevators, m_simDuration, m_capacity;
    double m_arrivalRate;
    TrafficPattern m_trafficPattern;
    SchedulerType m_selectedAlgo;

    // 仿真
    std::unique_ptr<Simulator> m_sim;
    MetricResult m_result;
    double m_currentTime;
    int m_servedCount, m_totalPassengers, m_waitingCount;
    double m_avgWait, m_avgTravel, m_maxWait;

    // 窗口尺寸
    int m_winW, m_winH;
};

int runGUI(HINSTANCE hInstance, int nCmdShow);

#endif
