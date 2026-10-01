#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <string>
#include <atomic>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

static HINSTANCE g_hInst = nullptr;
static HWND g_hwnd = nullptr;
static std::atomic<bool> g_enabled{false};
static bool g_captured = false;
static bool g_injected = false;
static float g_sensX = 2.5f;
static float g_sensY = 2.5f;
static bool g_invertY = false;
static POINT g_savedCursor{};

static const UINT WMAPP_TRAY = WM_APP + 1;
static const UINT ID_TRAY = 1001;
static const UINT ID_TOGGLE = 1002;
static const UINT ID_QUIT = 1003;

static void CenterCursorInClient(HWND target) {
    RECT r{};
    GetClientRect(target, &r);
    POINT p{(r.left+r.right)/2, (r.top+r.bottom)/2};
    ClientToScreen(target, &p);
    SetCursorPos(p.x, p.y);
}

static HWND FindPPSSPP() {
    HWND h = FindWindowW(L"PPSSPPWnd", nullptr);
    if (!h) h = FindWindowW(L"PPSSPPWndClass", nullptr);
    if (!h) h = FindWindowW(nullptr, L"PPSSPP");
    return h;
}

static void CaptureMouse() {
    if (g_captured) return;
    GetCursorPos(&g_savedCursor);
    g_captured = true;
    ShowCursor(FALSE);
    CenterCursorInClient(FindPPSSPP());
}

static void ReleaseMouse() {
    if (!g_captured) return;
    ShowCursor(TRUE);
    SetCursorPos(g_savedCursor.x, g_savedCursor.y);
    g_captured = false;
}

static void SetEnabled(bool on) {
    g_enabled = on;
    if (on) {
        if (!FindPPSSPP()) {
            g_enabled = false;
            MessageBoxW(g_hwnd, L"找不到正在运行的 PPSSPP。请先启动 PPSSPP。", L"PPSSPP Mouse Camera", MB_ICONWARNING);
            return;
        }
        CaptureMouse();
    } else {
        ReleaseMouse();
    }
}

static void NudgeAnalog(HWND ppsspp, LONG dx, LONG dy) {
    // V2 uses PPSSPP's configurable right-analog directional bindings.
    // Raw mouse deltas are converted to short directional pulses.
    if (!ppsspp || !IsWindow(ppsspp)) return;

    int sx = (int)std::lround(std::abs((double)dx) * g_sensX);
    int sy = (int)std::lround(std::abs((double)dy) * g_sensY);
    sx = std::clamp(sx, 1, 40);
    sy = std::clamp(sy, 1, 40);

    auto tap = [](WORD vk, int repeats) {
        for (int i=0; i<repeats; ++i) {
            keybd_event((BYTE)vk, 0, 0, 0);
            keybd_event((BYTE)vk, 0, KEYEVENTF_KEYUP, 0);
        }
    };

    if (dx > 0) tap(VK_RIGHT, sx);
    if (dx < 0) tap(VK_LEFT, sx);
    if (dy > 0) tap(g_invertY ? VK_UP : VK_DOWN, sy);
    if (dy < 0) tap(g_invertY ? VK_DOWN : VK_UP, sy);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01; // Generic Desktop
        rid.usUsage = 0x02;     // Mouse
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = hwnd;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));

        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = ID_TRAY;
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage = WMAPP_TRAY;
        nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        lstrcpyW(nid.szTip, L"PPSSPP Mouse Camera V2");
        Shell_NotifyIconW(NIM_ADD, &nid);
        return 0;
    }

    case WM_INPUT: {
        if (!g_enabled || g_injected) return 0;
        RAWINPUT ri{};
        UINT size = sizeof(ri);
        if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
            return 0;
        if (ri.header.dwType == RIM_TYPEMOUSE) {
            LONG dx = ri.data.mouse.lLastX;
            LONG dy = ri.data.mouse.lLastY;
            if ((dx || dy) && GetForegroundWindow() == FindPPSSPP()) {
                g_injected = true;
                NudgeAnalog(FindPPSSPP(), dx, dy);
                g_injected = false;
                CenterCursorInClient(FindPPSSPP());
            }
        }
        return 0;
    }

    case WM_HOTKEY:
        if (wp == 1) SetEnabled(!g_enabled);
        if (wp == 2) PostMessageW(hwnd, WM_CLOSE, 0, 0);
        return 0;

    case WMAPP_TRAY:
        if (lp == WM_LBUTTONDBLCLK) SetEnabled(!g_enabled);
        if (lp == WM_RBUTTONUP) {
            POINT pt{}; GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, ID_TOGGLE, g_enabled ? L"关闭鼠标镜头" : L"开启鼠标镜头");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, ID_QUIT, L"退出");
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            if (cmd == ID_TOGGLE) SetEnabled(!g_enabled);
            if (cmd == ID_QUIT) PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
        return 0;

    case WM_DESTROY: {
        ReleaseMouse();
        UnregisterHotKey(hwnd, 1);
        UnregisterHotKey(hwnd, 2);
        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = ID_TRAY;
        Shell_NotifyIconW(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    g_hInst = hInst;

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PPSSPPMouseCameraV2";
    RegisterClassW(&wc);

    g_hwnd = CreateWindowExW(0, wc.lpszClassName, L"PPSSPP Mouse Camera V2",
                             0, 0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) return 1;

    RegisterHotKey(g_hwnd, 1, 0, VK_F8);
    RegisterHotKey(g_hwnd, 2, 0, VK_F9);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
