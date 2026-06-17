#include "gui.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// 控件ID
enum CtrlID
{
    IDC_FLOORS = 1001, IDC_ELEVATORS = 1002, IDC_DURATION = 1003,
    IDC_CAPACITY = 1004, IDC_ARRIVAL = 1005, IDC_TRAFFIC = 1006,
    IDC_ALGO = 1007,
    IDC_START = 1008, IDC_STOP = 1009, IDC_RESTART = 1010,
    IDC_FLOORS_LBL = 1021, IDC_ELEV_LBL = 1022, IDC_DUR_LBL = 1023,
    IDC_CAP_LBL = 1024, IDC_ARR_LBL = 1025, IDC_TRAF_LBL = 1026,
    IDC_ALGO_LBL = 1027,
    ID_TIMER = 2001
};

ElevatorGUI* ElevatorGUI::s_instance = nullptr;

// 将std::string转为wstring（解决%S兼容问题）
static std::wstring toWide(const std::string& s)
{
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

ElevatorGUI::ElevatorGUI(HINSTANCE hInst)
    : m_hInstance(hInst), m_hWnd(nullptr),
      m_hFloorsEdit(nullptr), m_hElevEdit(nullptr), m_hDurEdit(nullptr),
      m_hCapEdit(nullptr), m_hArrivalEdit(nullptr),
      m_hTrafficCombo(nullptr), m_hAlgoCombo(nullptr),
      m_hStartBtn(nullptr), m_hStopBtn(nullptr), m_hResultBtn(nullptr),
      m_hRestartBtn(nullptr),
      m_hFontTitle(nullptr), m_hFontNormal(nullptr), m_hFontSmall(nullptr),
      m_hFontTiny(nullptr),
      m_state(UIState::PARAM),
      m_numFloors(10), m_numElevators(3), m_simDuration(600), m_capacity(10),
      m_arrivalRate(0.5), m_trafficPattern(TrafficPattern::UNIFORM),
      m_selectedAlgo(SchedulerType::GWO),
      m_currentTime(0), m_servedCount(0), m_totalPassengers(0),
      m_waitingCount(0), m_avgWait(0), m_avgTravel(0), m_maxWait(0),
      m_winW(1200), m_winH(820)
{
    m_elevColors[0] = RGB(66, 133, 244);
    m_elevColors[1] = RGB(219, 68, 55);
    m_elevColors[2] = RGB(244, 160, 0);
    m_elevColors[3] = RGB(15, 157, 88);
    m_elevColors[4] = RGB(156, 39, 176);
    m_elevColors[5] = RGB(0, 188, 212);
    m_elevColors[6] = RGB(255, 87, 34);
    m_elevColors[7] = RGB(139, 195, 74);
    s_instance = this;
}

ElevatorGUI::~ElevatorGUI()
{
    DeleteObject(m_hFontTitle);
    DeleteObject(m_hFontNormal);
    DeleteObject(m_hFontSmall);
    DeleteObject(m_hFontTiny);
}

bool ElevatorGUI::init(int nCmdShow)
{
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ElevatorSimFinal";
    RegisterClass(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - m_winW) / 2;
    int posY = (screenH - m_winH) / 2;

    m_hWnd = CreateWindow(
        L"ElevatorSimFinal", L"电梯群控调度动态演示系统",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        posX, posY, m_winW, m_winH,
        nullptr, nullptr, m_hInstance, nullptr);

    if (!m_hWnd) return false;
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
    return true;
}

int ElevatorGUI::run()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK ElevatorGUI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (s_instance)
    {
        switch (msg)
        {
        case WM_CREATE:  return s_instance->handleCreate(hWnd);
        case WM_COMMAND: return s_instance->handleCommand(wParam, lParam);
        case WM_TIMER:   return s_instance->handleTimer(wParam);
        case WM_PAINT:   return s_instance->handlePaint();
        case WM_SIZE:    s_instance->handleResize(); break;
        case WM_DESTROY: KillTimer(hWnd, ID_TIMER); PostQuitMessage(0); break;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================
// 创建控件 — 字体统一放大
// ============================================================

void ElevatorGUI::createControls()
{
    // 初始界面字体: title=30, normal=21, small=18, tiny=15
    m_hFontTitle = CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
    m_hFontNormal = CreateFont(21, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
    m_hFontSmall = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
    m_hFontTiny = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");

    int labelX = 30, editX = 230, editW = 160, editH = 30;
    int startY = 130, gap = 50;

    auto makeLabel = [&](const wchar_t* text, int y, int id)
    {
        HWND h = CreateWindow(L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
                              labelX, y, 185, 28, m_hWnd, (HMENU)(INT_PTR)id,
                              m_hInstance, nullptr);
        HFONT f = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        SendMessage(h, WM_SETFONT, (WPARAM)f, TRUE);
    };

    makeLabel(L"楼层数 (1-30):", startY, IDC_FLOORS_LBL);
    makeLabel(L"电梯数量 (1-8):", startY + gap, IDC_ELEV_LBL);
    makeLabel(L"模拟时长 (秒):", startY + gap * 2, IDC_DUR_LBL);
    makeLabel(L"电梯容量 (人/部):", startY + gap * 3, IDC_CAP_LBL);
    makeLabel(L"到达率 (人/秒):", startY + gap * 4, IDC_ARR_LBL);
    makeLabel(L"交通模式:", startY + gap * 5, IDC_TRAF_LBL);
    makeLabel(L"调度算法:", startY + gap * 6, IDC_ALGO_LBL);

    auto makeEdit = [&](const wchar_t* def, int y, int id)
    {
        HWND h = CreateWindow(L"EDIT", def,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
                              editX, y, editW, editH, m_hWnd, (HMENU)(INT_PTR)id,
                              m_hInstance, nullptr);
        SendMessage(h, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);
        return h;
    };

    m_hFloorsEdit  = makeEdit(L"10",  startY, IDC_FLOORS);
    m_hElevEdit    = makeEdit(L"3",   startY + gap, IDC_ELEVATORS);
    m_hDurEdit     = makeEdit(L"600", startY + gap * 2, IDC_DURATION);
    m_hCapEdit     = makeEdit(L"10",  startY + gap * 3, IDC_CAPACITY);
    m_hArrivalEdit = makeEdit(L"0.5", startY + gap * 4, IDC_ARRIVAL);

    m_hTrafficCombo = CreateWindow(L"COMBOBOX", nullptr,
                                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                    editX, startY + gap * 5, editW + 20, 200,
                                    m_hWnd, (HMENU)(INT_PTR)IDC_TRAFFIC,
                                    m_hInstance, nullptr);
    SendMessage(m_hTrafficCombo, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);
    SendMessage(m_hTrafficCombo, CB_ADDSTRING, 0, (LPARAM)L"均匀随机");
    SendMessage(m_hTrafficCombo, CB_ADDSTRING, 0, (LPARAM)L"上行高峰");
    SendMessage(m_hTrafficCombo, CB_ADDSTRING, 0, (LPARAM)L"下行高峰");
    SendMessage(m_hTrafficCombo, CB_ADDSTRING, 0, (LPARAM)L"双向高峰");
    SendMessage(m_hTrafficCombo, CB_ADDSTRING, 0, (LPARAM)L"层间随机");
    SendMessage(m_hTrafficCombo, CB_SETCURSEL, 0, 0);

    m_hAlgoCombo = CreateWindow(L"COMBOBOX", nullptr,
                                 WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                 editX, startY + gap * 6, editW + 20, 200,
                                 m_hWnd, (HMENU)(INT_PTR)IDC_ALGO,
                                 m_hInstance, nullptr);
    SendMessage(m_hAlgoCombo, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);
    SendMessage(m_hAlgoCombo, CB_ADDSTRING, 0, (LPARAM)L"GWO灰狼优化");
    SendMessage(m_hAlgoCombo, CB_ADDSTRING, 0, (LPARAM)L"最近电梯调度");
    SendMessage(m_hAlgoCombo, CB_SETCURSEL, 0, 0);

    m_hStartBtn = CreateWindow(L"BUTTON", L"开始仿真",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                editX, startY + gap * 7 + 30, 180, 48,
                                m_hWnd, (HMENU)(INT_PTR)IDC_START,
                                m_hInstance, nullptr);
    SendMessage(m_hStartBtn, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    m_hStopBtn = CreateWindow(L"BUTTON", L"停止仿真",
                               WS_CHILD | BS_PUSHBUTTON,
                               20, 10, 120, 38, m_hWnd,
                               (HMENU)(INT_PTR)IDC_STOP, m_hInstance, nullptr);
    SendMessage(m_hStopBtn, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    m_hRestartBtn = CreateWindow(L"BUTTON", L"重新设置",
                                  WS_CHILD | BS_PUSHBUTTON,
                                  m_winW - 170, 15, 140, 42, m_hWnd,
                                  (HMENU)(INT_PTR)IDC_RESTART, m_hInstance, nullptr);
    SendMessage(m_hRestartBtn, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);
}

LRESULT ElevatorGUI::handleCreate(HWND hWnd) { m_hWnd = hWnd; createControls(); showParamPanel(); return 0; }

void ElevatorGUI::handleResize()
{
    RECT client; GetClientRect(m_hWnd, &client);
    m_winW = client.right; m_winH = client.bottom;
    if (m_hRestartBtn) SetWindowPos(m_hRestartBtn, nullptr, m_winW - 170, 15, 140, 42, SWP_NOZORDER);
}

// ============================================================
// 面板切换
// ============================================================

void ElevatorGUI::showParamPanel()
{
    m_state = UIState::PARAM;
    int edits[] = { IDC_FLOORS, IDC_ELEVATORS, IDC_DURATION, IDC_CAPACITY, IDC_ARRIVAL,
                    IDC_TRAFFIC, IDC_ALGO, IDC_START };
    for (int id : edits) ShowWindow(GetDlgItem(m_hWnd, id), SW_SHOW);
    int labels[] = { IDC_FLOORS_LBL, IDC_ELEV_LBL, IDC_DUR_LBL, IDC_CAP_LBL,
                     IDC_ARR_LBL, IDC_TRAF_LBL, IDC_ALGO_LBL };
    for (int id : labels) ShowWindow(GetDlgItem(m_hWnd, id), SW_SHOW);
    ShowWindow(m_hStopBtn, SW_HIDE); ShowWindow(m_hRestartBtn, SW_HIDE);
    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void ElevatorGUI::showSimPanel()
{
    m_state = UIState::SIMULATING;
    int edits[] = { IDC_FLOORS, IDC_ELEVATORS, IDC_DURATION, IDC_CAPACITY, IDC_ARRIVAL,
                    IDC_TRAFFIC, IDC_ALGO, IDC_START };
    for (int id : edits) ShowWindow(GetDlgItem(m_hWnd, id), SW_HIDE);
    int labels[] = { IDC_FLOORS_LBL, IDC_ELEV_LBL, IDC_DUR_LBL, IDC_CAP_LBL,
                     IDC_ARR_LBL, IDC_TRAF_LBL, IDC_ALGO_LBL };
    for (int id : labels) ShowWindow(GetDlgItem(m_hWnd, id), SW_HIDE);
    ShowWindow(m_hStopBtn, SW_SHOW); ShowWindow(m_hRestartBtn, SW_HIDE);
    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void ElevatorGUI::showResultPanel()
{
    m_state = UIState::RESULTS;
    ShowWindow(m_hStopBtn, SW_HIDE); ShowWindow(m_hRestartBtn, SW_SHOW);
    InvalidateRect(m_hWnd, nullptr, TRUE);
}

// ============================================================
// 参数读取
// ============================================================

void ElevatorGUI::readParameters()
{
    WCHAR buf[64];
    GetWindowText(m_hFloorsEdit, buf, 64); int v = _wtoi(buf);
    if (v >= 1 && v <= 30) m_numFloors = v;
    GetWindowText(m_hElevEdit, buf, 64); v = _wtoi(buf);
    if (v >= 1 && v <= 8) m_numElevators = v;
    GetWindowText(m_hDurEdit, buf, 64); v = _wtoi(buf);
    if (v > 0) m_simDuration = v;
    GetWindowText(m_hCapEdit, buf, 64); v = _wtoi(buf);
    if (v > 0) m_capacity = v;
    GetWindowText(m_hArrivalEdit, buf, 64); double dv = _wtof(buf);
    if (dv > 0.0) m_arrivalRate = dv;
    int sel = (int)SendMessage(m_hTrafficCombo, CB_GETCURSEL, 0, 0);
    m_trafficPattern = (TrafficPattern)sel;
    int algoSel = (int)SendMessage(m_hAlgoCombo, CB_GETCURSEL, 0, 0);
    m_selectedAlgo = (algoSel == 1) ? SchedulerType::NEAREST : SchedulerType::GWO;
}

// ============================================================
// 仿真控制
// ============================================================

void ElevatorGUI::startSimulation()
{
    readParameters();
    m_currentTime = 0; m_servedCount = 0; m_totalPassengers = 0;
    m_waitingCount = 0; m_avgWait = 0; m_avgTravel = 0; m_maxWait = 0;
    SimulationConfig cfg;
    cfg.totalFloors = m_numFloors; cfg.numElevators = m_numElevators;
    cfg.elevatorCapacity = m_capacity; cfg.simulationDuration = (double)m_simDuration;
    cfg.passengerArrivalRate = m_arrivalRate;
    cfg.floorTravelTime = 2.0; cfg.stopTime = 3.0; cfg.doorTime = 2.0;
    cfg.gwoInterval = 5.0; cfg.gwoPopulation = 25; cfg.gwoIterations = 40;
    cfg.trafficPattern = m_trafficPattern;
    m_sim = std::make_unique<Simulator>(cfg, createScheduler(m_selectedAlgo, cfg));
    SetTimer(m_hWnd, ID_TIMER, 40, nullptr);
    showSimPanel();
}

void ElevatorGUI::stopSimulation()
{
    KillTimer(m_hWnd, ID_TIMER);
    if (m_sim) { while (m_sim->stepUpdate(0.1)) {} m_result = m_sim->getMetrics(); }
    collectStats();
    showResultPanel();
}

std::vector<int> ElevatorGUI::countWaitingPerFloor() const
{
    std::vector<int> counts(m_numFloors + 1, 0);
    if (!m_sim) return counts;
    for (const auto& p : m_sim->waitingPassengers)
        if (p.sourceFloor >= 1 && p.sourceFloor <= m_numFloors) counts[p.sourceFloor]++;
    return counts;
}

void ElevatorGUI::collectStats()
{
    if (!m_sim) return;
    int served = 0; double totalWait = 0, totalTravel = 0, maxW = 0; int st = 0;
    for (const auto& p : m_sim->allPassengers)
    {
        if (p.alightTime >= 0.0 && p.boardTime >= 0.0)
        {
            served++;
            double wait = p.boardTime - p.arrivalTime;
            double travel = p.alightTime - p.boardTime;
            totalWait += wait; totalTravel += travel;
            if (wait > maxW) maxW = wait; st++;
        }
    }
    m_servedCount = served; m_totalPassengers = (int)m_sim->allPassengers.size();
    m_waitingCount = (int)m_sim->waitingPassengers.size();
    m_maxWait = maxW; m_currentTime = m_sim->currentTime;
    if (st > 0) { m_avgWait = totalWait / st; m_avgTravel = totalTravel / st; }
}

// ============================================================
// 消息处理
// ============================================================

LRESULT ElevatorGUI::handleCommand(WPARAM wParam, LPARAM lParam)
{
    int id = LOWORD(wParam), code = HIWORD(wParam);
    if (id == IDC_START && code == BN_CLICKED) startSimulation();
    else if (id == IDC_STOP && code == BN_CLICKED) stopSimulation();
    else if (id == IDC_RESTART && code == BN_CLICKED)
    { KillTimer(m_hWnd, ID_TIMER); m_sim.reset(); showParamPanel(); }
    return 0;
}

LRESULT ElevatorGUI::handleTimer(WPARAM wParam)
{
    if (wParam == ID_TIMER && m_state == UIState::SIMULATING && m_sim)
    {
        if (!m_sim->stepUpdate(0.1)) { stopSimulation(); return 0; }
        collectStats();
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
    return 0;
}

// ============================================================
// 绘制
// ============================================================

LRESULT ElevatorGUI::handlePaint()
{
    PAINTSTRUCT ps; HDC hdc = BeginPaint(m_hWnd, &ps);
    RECT client; GetClientRect(m_hWnd, &client);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
    switch (m_state)
    {
    case UIState::PARAM:    drawParamPanel(memDC, client); break;
    case UIState::SIMULATING: drawSimulation(memDC, client); break;
    case UIState::RESULTS:  drawResults(memDC, client); break;
    }
    BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp); DeleteObject(memBmp); DeleteDC(memDC);
    EndPaint(m_hWnd, &ps);
    return 0;
}

// ============================================================
// 初始参数面板 — 标题大一码，说明文字统一放大，删除底部预览框
// ============================================================

void ElevatorGUI::drawParamPanel(HDC hdc, RECT& client)
{
    HBRUSH bg = CreateSolidBrush(RGB(245, 247, 250));
    FillRect(hdc, &client, bg); DeleteObject(bg);
    SetBkMode(hdc, TRANSPARENT);

    // 标题 (32号 > 说明文字)
    HFONT titleFont = CreateFont(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
    SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(25, 35, 55));
    RECT tr = {30, 18, 700, 60};
    DrawText(hdc, L"电梯群控调度动态演示系统", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(titleFont);

    // 副标题 (21号)
    SelectObject(hdc, m_hFontNormal);
    SetTextColor(hdc, RGB(80, 90, 115));
    RECT sr = {30, 62, 700, 92};
    DrawText(hdc, L"设置仿真参数，选择调度算法，点击「开始仿真」", -1, &sr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    HPEN sep = CreatePen(PS_SOLID, 1, RGB(210, 215, 225));
    SelectObject(hdc, sep);
    MoveToEx(hdc, 30, 110, nullptr); LineTo(hdc, client.right - 30, 110);
    DeleteObject(sep);

    // 右侧说明区域 — 字体统一放大
    int ix = 460, iy = 128;

    // "算法说明" 标题 (21号)
    SelectObject(hdc, m_hFontNormal);
    SetTextColor(hdc, RGB(28, 38, 58));
    RECT ir1 = {ix, iy, client.right - 30, iy + 35};
    DrawText(hdc, L"算法说明", -1, &ir1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // 算法说明内容 (18号 — small, 之前是tiny)
    SelectObject(hdc, m_hFontSmall);
    SetTextColor(hdc, RGB(55, 65, 85));

    RECT ir2 = {ix + 12, iy + 42, client.right - 35, iy + 130};
    DrawText(hdc, L"GWO灰狼优化算法:\n  模拟灰狼群体狩猎行为，由Alpha/Beta/Delta三只领导狼\n  引导种群搜索，平衡全局探索与局部开发，综合优化候梯\n  时间、乘梯时间、能耗和负载均衡。",
             -1, &ir2, DT_LEFT | DT_WORDBREAK);

    RECT ir3 = {ix + 12, iy + 148, client.right - 35, iy + 225};
    DrawText(hdc, L"最近电梯调度(基准算法):\n  将每个呼梯请求分配给预计到达时间最短的电梯，\n  同时考虑方向一致性和当前载客量，作为性能对比基准。",
             -1, &ir3, DT_LEFT | DT_WORDBREAK);

    // "评价指标说明" 标题 (21号)
    SelectObject(hdc, m_hFontNormal);
    SetTextColor(hdc, RGB(28, 38, 58));
    RECT ir4t = {ix, iy + 235, client.right - 30, iy + 270};
    DrawText(hdc, L"评价指标说明", -1, &ir4t, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // 评价指标内容 (18号)
    SelectObject(hdc, m_hFontSmall);
    SetTextColor(hdc, RGB(55, 65, 85));
    RECT ir4 = {ix + 12, iy + 275, client.right - 35, iy + 440};
    DrawText(hdc, L"  ■ 服务质量: 平均候梯时间、最大候梯时间、平均乘梯时间\n      长等待率(>60s)、服务完成率、不满意指数\n  ■ 运行效率: 电梯启停次数、电梯空转时间、人均能耗\n  ■ 拥挤管理: 拥挤度>50%%时间占比、各电梯当前载客\n  ■ 负载均衡: 负载均衡指数、各电梯利用率差异\n  ■ 综合评分: 加权归一化得分(0-100)",
             -1, &ir4, DT_LEFT | DT_WORDBREAK);

    // (不再绘制底部的深色预览框)
}

// ============================================================
// 仿真界面 — 字体总体调大2号，删除与停止按钮重叠部分+底部交通模式文字
// ============================================================

void ElevatorGUI::drawSimulation(HDC hdc, RECT& client)
{
    HBRUSH dbg = CreateSolidBrush(RGB(20, 24, 33));
    FillRect(hdc, &client, dbg); DeleteObject(dbg);
    if (!m_sim) return;

    int numFloors = m_numFloors, numElevators = m_numElevators;
    SetBkMode(hdc, TRANSPARENT);

    // 仿真用大号字体
    HFONT simTitle = CreateFont(26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
    HFONT simNormal = CreateFont(23, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
    HFONT simSmall = CreateFont(19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
    HFONT simTiny = CreateFont(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");

    // === 左侧面板 ===
    int panelW = 300, panelX = 15;

    // 算法名 (删除与停止按钮重叠的文字冲突 — 把y下移，不再靠近按钮区域)
    std::wstring algoW = toWide(m_sim->scheduler ? m_sim->scheduler->name() : "");
    SelectObject(hdc, simNormal);
    SetTextColor(hdc, RGB(100, 200, 255));
    RECT algoR = {panelX, 55, panelX + panelW, 85};
    WCHAR algoLabel[128];
    swprintf(algoLabel, 128, L"算法: %s", algoW.c_str());
    DrawText(hdc, algoLabel, -1, &algoR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // 实时统计卡片 (先用18号/tiny=17号)
    int cardW = 138, cardH = 56, cardGapX = 10, cardGapY = 8;
    int cardsY = 95;

    struct { const wchar_t* label; WCHAR val[32]; COLORREF c; } cards[] = {
        {L"等待人数", L"", RGB(244, 160, 0)},
        {L"已运送",   L"", RGB(66, 133, 244)},
        {L"平均等待", L"", RGB(219, 68, 55)},
        {L"平均乘梯", L"", RGB(15, 157, 88)},
        {L"总乘客数", L"", RGB(156, 39, 176)},
        {L"最大等待", L"", RGB(0, 188, 212)},
    };
    swprintf(cards[0].val, 32, L"%d", m_waitingCount);
    swprintf(cards[1].val, 32, L"%d", m_servedCount);
    swprintf(cards[2].val, 32, L"%.1fs", m_avgWait);
    swprintf(cards[3].val, 32, L"%.1fs", m_avgTravel);
    swprintf(cards[4].val, 32, L"%d", m_totalPassengers);
    swprintf(cards[5].val, 32, L"%.1fs", m_maxWait);

    for (int i = 0; i < 6; i++)
    {
        int col = i % 2, row = i / 2;
        int cx = panelX + col * (cardW + cardGapX);
        int cy = cardsY + row * (cardH + cardGapY);

        HBRUSH cb = CreateSolidBrush(RGB(33, 38, 48));
        HPEN cp = CreatePen(PS_SOLID, 2, cards[i].c);
        SelectObject(hdc, cp); SelectObject(hdc, cb);
        RoundRect(hdc, cx, cy, cx + cardW, cy + cardH, 8, 8);
        DeleteObject(cb); DeleteObject(cp);

        SelectObject(hdc, simSmall);
        SetTextColor(hdc, cards[i].c);
        RECT vr = {cx + 4, cy + 2, cx + cardW - 4, cy + 30};
        DrawText(hdc, cards[i].val, -1, &vr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, simTiny);
        SetTextColor(hdc, RGB(135, 145, 160));
        RECT nr = {cx + 4, cy + 30, cx + cardW - 4, cy + cardH - 2};
        DrawText(hdc, cards[i].label, -1, &nr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // === 各楼层等待人数 ===
    int waitPanelY = cardsY + 3 * (cardH + cardGapY) + 12;
    SelectObject(hdc, simSmall);
    SetTextColor(hdc, RGB(200, 210, 220));
    RECT waitTitleR = {panelX, waitPanelY, panelX + panelW, waitPanelY + 26};
    DrawText(hdc, L"各楼层等待人数", -1, &waitTitleR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    std::vector<int> floorWaits = countWaitingPerFloor();
    int listY = waitPanelY + 30;
    int listH = client.bottom - listY - 10;
    int rowH = std::min(24, listH / std::max(1, numFloors));

    HBRUSH listBg = CreateSolidBrush(RGB(28, 32, 42));
    HPEN listPen = CreatePen(PS_SOLID, 1, RGB(50, 55, 65));
    SelectObject(hdc, listPen); SelectObject(hdc, listBg);
    Rectangle(hdc, panelX, listY, panelX + panelW, listY + listH);
    DeleteObject(listBg); DeleteObject(listPen);

    SelectObject(hdc, simTiny);
    int maxWaitF = 1;
    for (int f = 1; f <= numFloors; f++)
        if (floorWaits[f] > maxWaitF) maxWaitF = floorWaits[f];

    for (int f = 1; f <= numFloors; f++)
    {
        int ry = listY + 4 + (f - 1) * rowH;
        if (ry + rowH > listY + listH - 2) break;

        WCHAR fl[8]; swprintf(fl, 8, L"F%d", f);
        SetTextColor(hdc, (f == 1) ? RGB(100, 200, 130) : RGB(140, 150, 165));
        RECT fr = {panelX + 8, ry, panelX + 42, ry + rowH};
        DrawText(hdc, fl, -1, &fr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        int wCount = floorWaits[f];
        SetTextColor(hdc, wCount > 0 ? RGB(255, 180, 40) : RGB(80, 85, 95));
        WCHAR wc[8]; swprintf(wc, 8, L"%d人", wCount);
        RECT wr = {panelX + 46, ry, panelX + 95, ry + rowH};
        DrawText(hdc, wc, -1, &wr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        int barX = panelX + 100, barW = panelW - barX - 10, barH = rowH - 4, barY = ry + 2;
        HBRUSH barBg = CreateSolidBrush(RGB(45, 50, 60));
        RECT barBgR = {barX, barY, barX + barW, barY + barH};
        FillRect(hdc, &barBgR, barBg); DeleteObject(barBg);

        if (wCount > 0 && maxWaitF > 0)
        {
            int fillW = (int)((double)wCount / maxWaitF * barW);
            if (fillW < 4) fillW = 4;
            COLORREF bc = (wCount <= 2) ? RGB(100, 200, 130)
                         : (wCount <= 5) ? RGB(244, 160, 0) : RGB(219, 68, 55);
            HBRUSH bf = CreateSolidBrush(bc);
            RECT bfr = {barX + 1, barY + 1, barX + fillW, barY + barH - 1};
            FillRect(hdc, &bfr, bf); DeleteObject(bf);
        }
    }

    // === 右侧: 电梯井道 ===
    int shaftAreaX = panelX + panelW + 20;
    int shaftAreaY = 8, shaftAreaW = client.right - shaftAreaX - 10;
    int shaftAreaH = client.bottom - shaftAreaY - 8;
    int floorH = shaftAreaH / numFloors;
    if (floorH < 18) floorH = 18;
    int shaftW = std::min(110, (shaftAreaW - 60) / numElevators - 12);
    int shaftGap = (numElevators <= 4) ? 28 : 8;
    int totalSW = numElevators * shaftW + (numElevators - 1) * shaftGap;
    int shaftStartX = shaftAreaX + (shaftAreaW - totalSW) / 2;

    // 电梯实时状态标题
    SelectObject(hdc, simSmall);
    SetTextColor(hdc, RGB(180, 190, 205));
    WCHAR shaftTitle[64];
    swprintf(shaftTitle, 64, L"电梯实时状态 (F1=底部  F%d=顶部)", numFloors);
    RECT stR = {shaftAreaX, shaftAreaY, shaftAreaX + shaftAreaW, shaftAreaY + 24};
    DrawText(hdc, shaftTitle, -1, &stR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    int aFY = shaftAreaY + 30, aFH = shaftAreaH - 32;
    int afH = aFH / numFloors;

    SelectObject(hdc, simTiny);
    for (int f = 0; f < numFloors; f++)
    {
        int y = aFY + (numFloors - 1 - f) * afH;
        WCHAR fl[16]; swprintf(fl, 16, L"F%d", f + 1);
        SetTextColor(hdc, (f == 0) ? RGB(100, 220, 140) : RGB(120, 130, 145));
        RECT flr = {shaftAreaX - 58, y, shaftAreaX - 5, y + afH};
        DrawText(hdc, fl, -1, &flr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        if (f + 1 <= numFloors && floorWaits[f + 1] > 0)
        {
            SetTextColor(hdc, RGB(255, 180, 40));
            WCHAR wlbl[16]; swprintf(wlbl, 16, L"[%d人]", floorWaits[f + 1]);
            RECT wlr = {shaftStartX + totalSW + 12, y, shaftStartX + totalSW + 75, y + afH};
            DrawText(hdc, wlbl, -1, &wlr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        HPEN gp = CreatePen(PS_SOLID, 1, RGB(45, 48, 55));
        SelectObject(hdc, gp);
        MoveToEx(hdc, shaftAreaX - 10, y + afH, nullptr);
        LineTo(hdc, shaftStartX + totalSW + 30, y + afH);
        DeleteObject(gp);
    }

    HPEN lw = CreatePen(PS_SOLID, 2, RGB(70, 75, 85));
    SelectObject(hdc, lw);
    MoveToEx(hdc, shaftAreaX - 10, aFY, nullptr);
    LineTo(hdc, shaftAreaX - 10, aFY + aFH);
    DeleteObject(lw);

    for (int e = 0; e < numElevators; e++)
    {
        int sx = shaftStartX + e * (shaftW + shaftGap);
        HPEN sp = CreatePen(PS_SOLID, 2, RGB(55, 58, 68));
        HBRUSH sb = CreateSolidBrush(RGB(28, 31, 40));
        RECT sr = {sx, aFY, sx + shaftW, aFY + aFH};
        FillRect(hdc, &sr, sb); Rectangle(hdc, sx, aFY, sx + shaftW, aFY + aFH);
        DeleteObject(sp); DeleteObject(sb);

        SetTextColor(hdc, m_elevColors[e % 8]);
        RECT enr = {sx, aFY + aFH + 2, sx + shaftW, aFY + aFH + 22};
        WCHAR en[32]; swprintf(en, 32, L"E%d", e + 1);
        DrawText(hdc, en, -1, &enr, DT_CENTER | DT_VCENTER);

        if (e >= (int)m_sim->elevators.size()) continue;
        const Elevator& elev = m_sim->elevators[e];
        int ef = elev.currentFloor, ey = aFY + (numFloors - ef) * afH, eh = afH, margin = 5;

        COLORREF ec = m_elevColors[e % 8];
        HBRUSH eb = CreateSolidBrush(ec);
        HPEN ep = CreatePen(PS_SOLID, 2, RGB(GetRValue(ec)/2, GetGValue(ec)/2, GetBValue(ec)/2));
        SelectObject(hdc, ep); SelectObject(hdc, eb);
        RoundRect(hdc, sx + margin, ey + margin, sx + shaftW - margin, ey + eh - margin, 6, 6);
        DeleteObject(eb); DeleteObject(ep);

        SetTextColor(hdc, RGB(255, 255, 255));
        int nPax = (int)elev.passengers.size();
        WCHAR paxLabel[32];
        swprintf(paxLabel, 32, L"%d人", nPax);
        RECT plr = {sx + margin, ey + 2, sx + shaftW - margin, ey + eh - margin};
        DrawText(hdc, paxLabel, -1, &plr, DT_CENTER | DT_VCENTER);

        int px = sx + shaftW / 2, py = ey + eh / 2;
        if (elev.direction != 0)
        {
            HPEN dp = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            SelectObject(hdc, dp);
            MoveToEx(hdc, px, py - 7, nullptr); LineTo(hdc, px, py + 7);
            if (elev.direction > 0)
            {
                MoveToEx(hdc, px - 3, py + 4, nullptr); LineTo(hdc, px, py + 7); LineTo(hdc, px + 3, py + 4);
            }
            else
            {
                MoveToEx(hdc, px - 3, py - 4, nullptr); LineTo(hdc, px, py - 7); LineTo(hdc, px + 3, py - 4);
            }
            DeleteObject(dp);
        }
    }

    // (不再显示底部交通模式提示)

    DeleteObject(simTitle); DeleteObject(simNormal);
    DeleteObject(simSmall); DeleteObject(simTiny);
}

// ============================================================
// 仿真结果报告 — 字体与初始界面一致，修复%S，重建指标体系
// ============================================================

void ElevatorGUI::drawResults(HDC hdc, RECT& client)
{
    HBRUSH dbg = CreateSolidBrush(RGB(20, 24, 33));
    FillRect(hdc, &client, dbg); DeleteObject(dbg);
    SetBkMode(hdc, TRANSPARENT);

    // 结果页字体(与初始界面一致: title=30, normal=21, small=18, tiny=15)
    SelectObject(hdc, m_hFontTitle);
    SetTextColor(hdc, RGB(100, 200, 255));
    RECT tr = {30, 22, client.right - 30, 65};
    DrawText(hdc, L"仿真结果报告", -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 算法与场景 — 使用wstring避免%S宽窄字符问题
    std::wstring algoW = toWide(m_result.algorithmName);
    std::wstring sceneW = toWide(m_result.scenarioName);
    SelectObject(hdc, m_hFontNormal);
    SetTextColor(hdc, RGB(200, 210, 225));
    WCHAR algoLbl[256];
    swprintf(algoLbl, 256, L"算法: %s  |  场景: %s", algoW.c_str(), sceneW.c_str());
    RECT alr = {30, 72, client.right - 30, 102};
    DrawText(hdc, algoLbl, -1, &alr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // === 重构的综合评价指标卡片 (2行 x 5列) ===
    int cardW = 175, cardH = 104, cardGap = 16;
    int cardsPerRow = 5;
    int totalWCards = cardsPerRow * cardW + (cardsPerRow - 1) * cardGap;
    int cardsX = (client.right - totalWCards) / 2;
    int cardsY = 115;

    struct Card { const wchar_t* title; WCHAR val[32]; const wchar_t* unit; COLORREF c; };

    Card resultCards[10];
    // 按照用户要求组织指标
    // Row1: 平均候梯时间 | 成功运送人数 | 平均乘梯时间 | 电梯启停次数 | 电梯内现有人数
    // Row2: 电梯空转时间 | 拥挤度>50%时间 | 综合得分 | 总乘客数 | 服务完成率

    auto fillCard = [](Card& card, const wchar_t* title, double val, const wchar_t* unit, COLORREF c)
    {
        card.title = title; card.unit = unit; card.c = c;
        swprintf(card.val, 32, L"%.1f", val);
    };

    // 计算各电梯当前载客总数
    int totalCurrentPax = 0;
    for (const auto& ed : m_result.elevatorDetails) totalCurrentPax += ed.currentPax;

    resultCards[0] = {L"平均候梯时间", L"", L"秒",     RGB(219, 68, 55)};
    resultCards[1] = {L"成功运送人数", L"", L"人",     RGB(66, 133, 244)};
    resultCards[2] = {L"平均乘梯时间", L"", L"秒",     RGB(15, 157, 88)};
    resultCards[3] = {L"电梯启停次数", L"", L"次",     RGB(244, 160, 0)};
    resultCards[4] = {L"电梯内总人数", L"", L"人",     RGB(156, 39, 176)};
    resultCards[5] = {L"电梯空转时间", L"", L"秒",     RGB(0, 188, 212)};
    resultCards[6] = {L"拥挤>50%时间", L"", L"%",      RGB(255, 87, 34)};
    resultCards[7] = {L"综合评价得分", L"", L"分",     RGB(255, 210, 50)};
    resultCards[8] = {L"总乘客数",     L"", L"人",     RGB(139, 195, 74)};
    resultCards[9] = {L"服务完成率",   L"", L"%",      RGB(200, 140, 80)};

    swprintf(resultCards[0].val, 32, L"%.1f", m_result.avgWaitingTime);
    swprintf(resultCards[1].val, 32, L"%d",  m_result.servedPassengers);
    swprintf(resultCards[2].val, 32, L"%.1f", m_result.avgTravelTime);
    swprintf(resultCards[3].val, 32, L"%d",  m_result.totalStops);
    swprintf(resultCards[4].val, 32, L"%d",  totalCurrentPax);
    swprintf(resultCards[5].val, 32, L"%.0f", m_result.emptyRunningTime);
    swprintf(resultCards[6].val, 32, L"%.1f", m_result.crowdingRate);
    swprintf(resultCards[7].val, 32, L"%.1f", m_result.comprehensiveScore);
    swprintf(resultCards[8].val, 32, L"%d",  m_result.totalPassengers);
    swprintf(resultCards[9].val, 32, L"%.1f", m_result.serviceRate);

    for (int i = 0; i < 10; i++)
    {
        int col = i % cardsPerRow, row = i / cardsPerRow;
        int cx = cardsX + col * (cardW + cardGap);
        int cy = cardsY + row * (cardH + cardGap);

        HBRUSH cb = CreateSolidBrush(RGB(35, 40, 50));
        HPEN cp = CreatePen(PS_SOLID, 2, resultCards[i].c);
        SelectObject(hdc, cp); SelectObject(hdc, cb);
        RoundRect(hdc, cx, cy, cx + cardW, cy + cardH, 10, 10);
        DeleteObject(cb); DeleteObject(cp);

        // 数值 (21号)
        SelectObject(hdc, m_hFontNormal);
        SetTextColor(hdc, resultCards[i].c);
        RECT vr = {cx + 8, cy + 6, cx + cardW - 8, cy + 52};
        DrawText(hdc, resultCards[i].val, -1, &vr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 单位 (15号)
        SelectObject(hdc, m_hFontTiny);
        SetTextColor(hdc, RGB(135, 145, 160));
        RECT ur = {cx + 8, cy + 48, cx + cardW - 8, cy + 68};
        DrawText(hdc, resultCards[i].unit, -1, &ur, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 名称 (15号)
        RECT nr = {cx + 6, cy + 70, cx + cardW - 6, cy + cardH - 5};
        DrawText(hdc, resultCards[i].title, -1, &nr, DT_CENTER | DT_WORDBREAK);
    }

    // === 各电梯详情表 ===
    if (!m_result.elevatorDetails.empty())
    {
        int detY = cardsY + 2 * (cardH + cardGap) + 12;
        SelectObject(hdc, m_hFontSmall);
        SetTextColor(hdc, RGB(200, 210, 220));
        RECT detR = {30, detY, client.right - 30, detY + 28};
        DrawText(hdc, L"各电梯运行详情", -1, &detR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        int tblX = (client.right - 720) / 2, tblY = detY + 32;
        int colWids[] = {70, 80, 85, 85, 80, 80, 80, 80, 80};
        const wchar_t* colNames[] = {
            L"电梯", L"总距离", L"空驶距离", L"载客距离", L"空转(秒)",
            L"停靠次", L"登梯数", L"拥挤(秒)", L"利用率"
        };

        HBRUSH hdrB = CreateSolidBrush(RGB(42, 47, 57));
        RECT hdrR = {tblX, tblY, tblX + 720, tblY + 28};
        FillRect(hdc, &hdrR, hdrB); DeleteObject(hdrB);

        SelectObject(hdc, m_hFontTiny);
        int cx2 = tblX;
        for (int i = 0; i < 9; i++)
        {
            SetTextColor(hdc, RGB(180, 190, 205));
            RECT cr2 = {cx2, tblY, cx2 + colWids[i], tblY + 28};
            DrawText(hdc, colNames[i], -1, &cr2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            cx2 += colWids[i];
        }

        for (size_t i = 0; i < m_result.elevatorDetails.size(); i++)
        {
            const auto& ed = m_result.elevatorDetails[i];
            int ry2 = tblY + 30 + (int)i * 26;
            if (i % 2 == 0)
            {
                HBRUSH rbg2 = CreateSolidBrush(RGB(30, 34, 42));
                RECT rr2 = {tblX, ry2, tblX + 720, ry2 + 26};
                FillRect(hdc, &rr2, rbg2); DeleteObject(rbg2);
            }
            WCHAR rowData[9][32];
            swprintf(rowData[0], 32, L"E%d", ed.elevatorId + 1);
            swprintf(rowData[1], 32, L"%.1f", ed.totalDistance);
            swprintf(rowData[2], 32, L"%.1f", ed.emptyDistance);
            swprintf(rowData[3], 32, L"%.1f", ed.loadedDistance);
            swprintf(rowData[4], 32, L"%.0f", ed.emptyRunningTime);
            swprintf(rowData[5], 32, L"%d",   ed.totalStops);
            swprintf(rowData[6], 32, L"%d",   ed.totalBoarded);
            swprintf(rowData[7], 32, L"%.0f", ed.crowdedTime);
            swprintf(rowData[8], 32, L"%.1f%%", ed.utilizationRate);

            int cx3 = tblX;
            for (int j = 0; j < 9; j++)
            {
                SetTextColor(hdc, RGB(160, 170, 185));
                RECT cr3 = {cx3, ry2, cx3 + colWids[j], ry2 + 26};
                DrawText(hdc, rowData[j], -1, &cr3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                cx3 += colWids[j];
            }
        }
    }

    // 底部
    int botY = client.bottom - 50;
    SelectObject(hdc, m_hFontSmall);
    SetTextColor(hdc, RGB(120, 130, 145));
    RECT botR = {30, botY, client.right - 30, botY + 30};
    DrawText(hdc, L"点击「重新设置」修改参数再次仿真  |  可切换算法对比 GWO vs 最近电梯",
             -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// 入口
// ============================================================

int runGUI(HINSTANCE hInstance, int nCmdShow)
{
    ElevatorGUI app(hInstance);
    if (!app.init(nCmdShow)) return 1;
    return app.run();
}
