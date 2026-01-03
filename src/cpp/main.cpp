// !!! ЭТИ ДВЕ СТРОКИ ВАЖНЫ ДЛЯ UNICODE !!!
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

// Глобальные переменные
std::atomic<bool> running(false);
const int VK_F8_KEY = 0x77;

// Базовое разрешение (исходные координаты)
const double BASE_W = 1920.0;
const double BASE_H = 1080.0;

struct PatternStep {
    int x;
    int y;
    float delay;
};

// Исправленная инициализация вектора (без лишних фигурных скобок)
const std::vector<PatternStep> CLICK_SEQUENCE = {
    {288, 201, 1.0f},   // Select Slot
    {1710, 985, 2.0f},  // Auto Select
    {522, 760, 0.8f},   // Confirm
    {439, 991, 2.0f},   // Upgrade
    {439, 991, 0.5f},   // Skip/Confirm
    {1814, 61, 1.0f}    // Close
};

// ================= Логика масштабирования и кликов =================

void ClickScaled(int base_x, int base_y) {
    // Получаем текущее разрешение экрана
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);

    // Считаем масштаб (double для точности)
    double scale_x = (double)screen_w / BASE_W;
    double scale_y = (double)screen_h / BASE_H;

    // Масштабируем и нормализуем для SendInput (0..65535)
    int abs_x = (int)((base_x * scale_x) * (65535.0 / screen_w));
    int abs_y = (int)((base_y * scale_y) * (65535.0 / screen_h));

    // Движение
    INPUT move = { 0 };
    move.type = INPUT_MOUSE;
    move.mi.dx = abs_x;
    move.mi.dy = abs_y;
    move.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &move, sizeof(INPUT));
    
    Sleep(20);

    // Нажатие
    INPUT down = { 0 };
    down.type = INPUT_MOUSE;
    down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &down, sizeof(INPUT));

    Sleep(10);

    // Отпускание
    INPUT up = { 0 };
    up.type = INPUT_MOUSE;
    up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &up, sizeof(INPUT));
}

// ================= Потоки =================

void AutomationLoop() {
    Beep(1200, 150);

    // 5 секунд отсчет
    for (int i = 5; i > 0; --i) {
        if (!running) return;
        Beep(800, 100);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Beep(1500, 200);
    Beep(1500, 200);

    while (running) {
        for (const auto& step : CLICK_SEQUENCE) {
            if (!running) return;
            
            ClickScaled(step.x, step.y);
            
            int sleep_ms = static_cast<int>(step.delay * 1000);
            // Дробим ожидание на части по 50мс для быстрой реакции на стоп
            for (int t = 0; t < sleep_ms; t += 50) {
                if (!running) return;
                Sleep(50);
            }
        }
    }
}

void HotkeyListener() {
    while (true) {
        if (GetAsyncKeyState(VK_F8_KEY) & 1) {
            if (running) {
                running = false;
                Beep(400, 300);
            }
        }
        Sleep(50);
    }
}

// ================= GUI (WinAPI) =================

void StartAutomation() {
    if (running) return;
    running = true;
    std::thread(AutomationLoop).detach();
}

void StopAutomation() {
    if (running) {
        running = false;
        Beep(400, 300);
    }
}

void ShowDisclaimer(HWND hWnd) {
    const wchar_t* msg = 
        L"1. Open Echo Upgrade screen.\n"
        L"2. Set game to FULLSCREEN.\n"
        L"3. Works on 1080p, 2K, 4K (16:9 ratio).\n\n"
        L"Press F8 to STOP immediately.\n\n"
        L"Use at your own risk.";
    MessageBox(hWnd, msg, L"Instructions", MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT hBold = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        HWND hLabel = CreateWindow(L"STATIC", L"EchoFlux Automation", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 10, 380, 30, hwnd, NULL, NULL, NULL);
        SendMessage(hLabel, WM_SETFONT, (WPARAM)hBold, TRUE);

        HWND hDesc = CreateWindow(L"STATIC", L"Supports: 1080p, 2K, 4K (Fullscreen)\nAspect Ratio 16:9 recommended", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 50, 380, 40, hwnd, NULL, NULL, NULL);
        SendMessage(hDesc, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hBtnDesc = CreateWindow(L"BUTTON", L"Instructions", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 100, 100, 200, 30, hwnd, (HMENU)1, NULL, NULL);
        SendMessage(hBtnDesc, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hBtnStart = CreateWindow(L"BUTTON", L"START", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 100, 140, 200, 30, hwnd, (HMENU)2, NULL, NULL);
        SendMessage(hBtnStart, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hBtnStop = CreateWindow(L"BUTTON", L"STOP (F8)", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 100, 180, 200, 30, hwnd, (HMENU)3, NULL, NULL);
        SendMessage(hBtnStop, WM_SETFONT, (WPARAM)hFont, TRUE);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: ShowDisclaimer(hwnd); break;
        case 2: StartAutomation(); break;
        case 3: StopAutomation(); break;
        }
        break;
    }
    case WM_CLOSE: running = false; DestroyWindow(hwnd); break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    std::thread(HotkeyListener).detach();
    const wchar_t CLASS_NAME[] = L"EchoFluxClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"EchoFlux", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 260, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}