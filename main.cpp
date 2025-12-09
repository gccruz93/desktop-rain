#include <windows.h>
#include <random>
#include <vector>
#include <algorithm>
#include <cmath>
#include <shellapi.h>

#define IDI_MAINICON 101

const wchar_t APP_NAME[] = L"Desktop Rain";
HHOOK g_hook = NULL;
HWND g_hwnd = NULL;
const UINT WM_APP_CREATE_RAINDROP = WM_APP + 1;
const UINT WM_APP_TRAY_ICON = WM_APP + 2;
const UINT TRAY_ICON_ID = 1;

struct Raindrop
{
    float x, y;
    int length;
    float depth;
};

struct Particle
{
    float x, y;
    float vx, vy;
    float lifetime;
    float maxLifetime;
};
std::vector<Raindrop> g_raindrops;
std::vector<Particle> g_splashParticles;

const float MAX_WIND_SPEED = 300.0f;
float RAINDROP_SPEED_Y = 400.0f;
int screenWidth = 0;
int screenHeight = 0;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        if (g_hwnd != NULL)
        {
            PostMessage(g_hwnd, WM_APP_CREATE_RAINDROP, 0, 0);
        }
    }

    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static bool initialized = false;
    static HDC memDC = NULL;
    static HBITMAP memBitmap = NULL;

    const static int NUM_DEPTH_LAYERS = 3;
    static HPEN depthPens[NUM_DEPTH_LAYERS];

    const static int NUM_FADE_STAGES = 8;
    static HPEN fadePens[NUM_FADE_STAGES];

    static LARGE_INTEGER perf_freq;
    static LARGE_INTEGER last_counter;
    static bool timer_initialized = false;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distDropX;
    static std::uniform_int_distribution<> distDropY;
    static std::uniform_real_distribution<> distDropDepth(0.4f, 1.0f);

    static std::uniform_real_distribution<> distSplashAngle(0.0f, 3.14159f);
    static std::uniform_real_distribution<> distSplashSpeed(50.0f, 150.0f);
    static std::uniform_real_distribution<> distSplashLifetime(0.2f, 0.5f);
    static std::uniform_int_distribution<> distSplashParticleCount(3, 5);
    static const float GRAVITY = 400.0f;

    if (!initialized)
    {
        g_raindrops.reserve(100);
        g_splashParticles.reserve(500);

        if (!timer_initialized)
        {
            QueryPerformanceFrequency(&perf_freq);
            QueryPerformanceCounter(&last_counter);
            timer_initialized = true;
        }

        distDropX.param(std::uniform_int_distribution<>::param_type(0, screenWidth));
        distDropY.param(std::uniform_int_distribution<>::param_type(0, screenHeight));

        HDC hdc = GetDC(hwnd);
        memDC = CreateCompatibleDC(hdc);
        memBitmap = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
        SelectObject(memDC, memBitmap);
        ReleaseDC(hwnd, hdc);

        depthPens[0] = CreatePen(PS_SOLID, 1, RGB(25, 25, 25));
        depthPens[1] = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        depthPens[2] = CreatePen(PS_SOLID, 1, RGB(140, 140, 140));

        for (int i = 0; i < NUM_FADE_STAGES; ++i)
        {
            float progress = (float)(NUM_FADE_STAGES - 1 - i) / (NUM_FADE_STAGES - 1);
            int brightness = (int)(25 + (140 - 25) * progress);
            fadePens[i] = CreatePen(PS_SOLID, 1, RGB(brightness, brightness, brightness));
        }

        initialized = true;
    }

    switch (uMsg)
    {
    case WM_CREATE:
    {
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = TRAY_ICON_ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_APP_TRAY_ICON;
        nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON));
        if (!nid.hIcon)
        {
            nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        }
        wcscpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), APP_NAME);
        Shell_NotifyIcon(NIM_ADD, &nid);
        return 0;
    }

    case WM_APP_TRAY_ICON:
    {
        if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 2, L"Exit");

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == 2)
        {
            PostMessage(hwnd, WM_QUIT, 0, 0);
        }
        return 0;
    }

    case WM_DESTROY:
    {
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = TRAY_ICON_ID;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        for (int i = 0; i < NUM_DEPTH_LAYERS; ++i)
        {
            DeleteObject(depthPens[i]);
        }
        for (int i = 0; i < NUM_FADE_STAGES; ++i)
        {
            DeleteObject(fadePens[i]);
        }
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        PostQuitMessage(0);
        return 0;
    }

    case WM_PAINT:
    {
        float windFactor = 0.0f;
        if (!g_raindrops.empty())
        {
            POINT mousePos;
            GetCursorPos(&mousePos);
            windFactor = ((float)mousePos.x - (screenWidth / 2.0f)) / (screenWidth / 2.0f);
        }

        LARGE_INTEGER current_counter;
        QueryPerformanceCounter(&current_counter);
        float delta_time = (float)((double)(current_counter.QuadPart - last_counter.QuadPart) / (double)perf_freq.QuadPart);
        last_counter = current_counter;

        for (auto it = g_raindrops.begin(); it != g_raindrops.end();)
        {
            it->x += (MAX_WIND_SPEED * windFactor * it->depth) * delta_time;
            it->y += (RAINDROP_SPEED_Y * it->depth) * delta_time;

            if (it->y > screenHeight || it->x < -50.0f || it->x > screenWidth + 50.0f)
            {
                if (it->y > screenHeight)
                {
                    int numParticles = distSplashParticleCount(gen);
                    for (int i = 0; i < numParticles; ++i)
                    {
                        Particle p;
                        p.x = it->x;
                        p.y = (float)screenHeight - 1;

                        float angle = distSplashAngle(gen);
                        float speed = distSplashSpeed(gen);
                        p.vx = cos(angle) * speed;
                        p.vy = -abs(sin(angle) * speed);

                        p.lifetime = distSplashLifetime(gen);
                        p.maxLifetime = p.lifetime;
                        g_splashParticles.push_back(p);
                    }
                }

                it = g_raindrops.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (g_raindrops.size() > 10000)
        {
            g_raindrops.erase(g_raindrops.begin(), g_raindrops.begin() + 1000);
        }

        if (g_splashParticles.size() > 50000)
        {
            g_splashParticles.erase(g_splashParticles.begin(), g_splashParticles.begin() + 5000);
        }

        for (auto p_it = g_splashParticles.begin(); p_it != g_splashParticles.end();)
        {
            p_it->vy += GRAVITY * delta_time;
            p_it->x += p_it->vx * delta_time;
            p_it->y += p_it->vy * delta_time;
            p_it->lifetime -= delta_time;

            if (p_it->lifetime <= 0.0f)
            {
                p_it = g_splashParticles.erase(p_it);
            }
            else
            {
                ++p_it;
            }
        }

        // --- Rendering ---
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(memDC, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));

        for (const auto &drop : g_raindrops)
        {
            int penIdx = (drop.depth <= 0.4f) ? 0 : (drop.depth <= 0.7f ? 1 : 2);
            SelectObject(memDC, depthPens[penIdx]);

            int x = (int)drop.x;
            int y = (int)drop.y;
            MoveToEx(memDC, x, y, NULL);
            LineTo(memDC, x, y + drop.length);
        }

        for (const auto &p : g_splashParticles)
        {
            float lifeProgress = p.lifetime / p.maxLifetime;
            int fadeStage = (int)(lifeProgress * (NUM_FADE_STAGES - 1));
            fadeStage = (fadeStage < 0) ? 0 : (fadeStage >= NUM_FADE_STAGES ? NUM_FADE_STAGES - 1 : fadeStage);

            SelectObject(memDC, fadePens[fadeStage]);

            int x = (int)p.x;
            int y = (int)p.y;
            MoveToEx(memDC, x, y, NULL);
            LineTo(memDC, x, y + 2);
        }

        BitBlt(hdc, 0, 0, screenWidth, screenHeight, memDC, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_APP_CREATE_RAINDROP:
    {
        Raindrop newDrop;
        newDrop.x = (float)distDropX(gen);
        newDrop.y = -50;
        newDrop.depth = distDropDepth(gen);
        newDrop.length = 5 + (int)(25 * newDrop.depth);
        g_raindrops.push_back(newDrop);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_NAME;
    wc.hbrBackground = nullptr;

    RegisterClass(&wc);

    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RAINDROP_SPEED_Y = (float)screenHeight * 2;

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        APP_NAME,
        APP_NAME,
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr);

    g_hwnd = hwnd;

    if (hwnd == nullptr)
        return 0;

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    MSG msg = {};
    bool running = true;
    while (running)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            if (!g_raindrops.empty() || !g_splashParticles.empty())
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else
            {
                WaitMessage();
            }
        }
    }

    UnhookWindowsHookEx(g_hook);

    return 0;
}
