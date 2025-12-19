#include "RainApplication.h"
#include <shellapi.h>
#include <commdlg.h>
#include <cmath>
#include <numbers>
#include <chrono>
#include <algorithm>

static RainApplication *g_pAppInstance = nullptr;

RainApplication::RainApplication(HINSTANCE hInstance) : m_hInstance(hInstance)
{
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
    m_raindropSpeedY = static_cast<float>(m_screenHeight) * 2.0f;

    std::random_device rd;
    m_gen = std::mt19937(rd());

    m_raindrops.reserve(Config::MAX_RAINDROPS);
    m_particles.reserve(Config::MAX_PARTICLES);

    g_pAppInstance = this;
}

RainApplication::~RainApplication()
{
    if (m_hook)
        UnhookWindowsHookEx(m_hook);
    CleanupTrayIcon();
    g_pAppInstance = nullptr;
}

int RainApplication::Run()
{
    if (!InitializeWindow())
        return 0;
    if (!InitializeDirect2D())
        return 0;

    SetupTrayIcon();
    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, m_hInstance, 0);

    MSG msg = {};
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_running)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                m_running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            auto currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> diff = currentTime - lastTime;
            float dt = diff.count();
            lastTime = currentTime;

            Update(dt);
            Render();

            if (m_raindrops.empty() && m_particles.empty())
            {
                WaitMessage();
            }
            else
            {
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
    }
    return 0;
}

void RainApplication::AddRaindrop()
{
    std::uniform_int_distribution<> distX(0, m_screenWidth);
    std::uniform_real_distribution<> distDepth(0.4f, 1.0f);

    float depth = distDepth(m_gen);
    m_raindrops.emplace_back(Raindrop{
        .x = static_cast<float>(distX(m_gen)),
        .y = -50.0f,
        .length = 5 + static_cast<int>(25 * depth),
        .depth = depth});
}

void RainApplication::Update(float dt)
{
    float windFactor = 0.0f;
    if (!m_raindrops.empty())
    {
        POINT mousePos;
        GetCursorPos(&mousePos);
        windFactor = (static_cast<float>(mousePos.x) - (m_screenWidth / 2.0f)) / (m_screenWidth / 2.0f);
    }

    if (m_autoRain)
    {
        static float autoRainTimer = 0.0f;
        autoRainTimer += dt;
        if (autoRainTimer >= 0.05f)
        {
            AddRaindrop();
            autoRainTimer = 0.0f;
        }
    }

    for (auto &drop : m_raindrops)
    {
        drop.x += (Config::MAX_WIND_SPEED * windFactor * drop.depth) * dt;
        drop.y += (m_raindropSpeedY * drop.depth) * dt;

        if (drop.y > m_screenHeight)
        {
            SpawnSplash(drop.x);
        }
    }

    std::erase_if(m_raindrops, [this](const Raindrop &r)
                  { return r.y > m_screenHeight || r.x < -50.0f || r.x > m_screenWidth + 50.0f; });

    if (m_raindrops.size() > Config::MAX_RAINDROPS)
    {
        m_raindrops.erase(m_raindrops.begin(), m_raindrops.begin() + (m_raindrops.size() - Config::MAX_RAINDROPS));
    }

    for (auto &p : m_particles)
    {
        p.vy += Config::GRAVITY * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.lifetime -= dt;
    }

    std::erase_if(m_particles, [](const Particle &p)
                  { return p.lifetime <= 0.0f; });

    if (m_particles.size() > Config::MAX_PARTICLES)
    {
        m_particles.erase(m_particles.begin(), m_particles.begin() + (m_particles.size() - Config::MAX_PARTICLES));
    }
}

void RainApplication::SpawnSplash(float x)
{
    std::uniform_int_distribution<> distCount(3, 5);
    std::uniform_real_distribution<float> distAngle(0.0f, std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> distSpeed(50.0f, 150.0f);
    std::uniform_real_distribution<float> distLife(0.2f, 0.5f);

    int count = distCount(m_gen);
    for (int i = 0; i < count; ++i)
    {
        float angle = distAngle(m_gen);
        float speed = distSpeed(m_gen);

        m_particles.emplace_back(Particle{
            .x = x,
            .y = static_cast<float>(m_screenHeight - 1),
            .vx = std::cos(angle) * speed,
            .vy = -std::abs(std::sin(angle) * speed),
            .lifetime = distLife(m_gen),
            .maxLifetime = distLife(m_gen)});
        m_particles.back().maxLifetime = m_particles.back().lifetime;
    }
}

void RainApplication::Render()
{
    if (!m_renderTarget)
    {
        CreateDeviceResources();
        if (!m_renderTarget)
            return;
    }

    m_renderTarget->BeginDraw();
    m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    m_renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    float baseR = GetRValue(m_rainColor) / 255.0f;
    float baseG = GetGValue(m_rainColor) / 255.0f;
    float baseB = GetBValue(m_rainColor) / 255.0f;

    for (const auto &drop : m_raindrops)
    {
        float intensity = (drop.depth <= 0.4f) ? 0.2f : (drop.depth <= 0.7f ? 0.45f : 0.6f);

        m_brush->SetColor(D2D1::ColorF(baseR * intensity, baseG * intensity, baseB * intensity));

        D2D1_POINT_2F start = {drop.x, drop.y};
        D2D1_POINT_2F end = {drop.x, drop.y + drop.length};
        m_renderTarget->DrawLine(start, end, m_brush.get(), 1.0f);
    }

    for (const auto &p : m_particles)
    {
        float progress = p.lifetime / p.maxLifetime;
        float brightness = 0.2f + (0.6f) * progress;

        m_brush->SetColor(D2D1::ColorF(baseR * brightness, baseG * brightness, baseB * brightness));

        D2D1_POINT_2F start = {p.x, p.y};
        D2D1_POINT_2F end = {p.x, p.y + 2.0f};
        m_renderTarget->DrawLine(start, end, m_brush.get(), 1.0f);
    }

    HRESULT hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
    }
}

bool RainApplication::InitializeWindow()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = Config::APP_NAME;
    wc.hbrBackground = nullptr;

    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        Config::APP_NAME, Config::APP_NAME,
        WS_POPUP, 0, 0, m_screenWidth, m_screenHeight,
        nullptr, nullptr, m_hInstance, this);

    if (!m_hwnd)
        return false;

    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(m_hwnd, SW_SHOW);
    return true;
}

bool RainApplication::InitializeDirect2D()
{
    ID2D1Factory *rawFactory = nullptr;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &rawFactory);
    if (SUCCEEDED(hr))
    {
        m_d2dFactory.reset(rawFactory);
        return true;
    }
    return false;
}

void RainApplication::CreateDeviceResources()
{
    if (!m_renderTarget && m_hwnd)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        ID2D1HwndRenderTarget *rawRT = nullptr;
        HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &rawRT);

        if (SUCCEEDED(hr))
        {
            m_renderTarget.reset(rawRT);
            ID2D1SolidColorBrush *rawBrush = nullptr;
            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &rawBrush);
            m_brush.reset(rawBrush);
        }
    }
}

void RainApplication::DiscardDeviceResources()
{
    m_renderTarget.reset();
    m_brush.reset();
}

void RainApplication::SetupTrayIcon()
{
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hwnd;
    nid.uID = Config::TRAY_ICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = Config::WM_APP_TRAY_ICON;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    if (!nid.hIcon)
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, Config::APP_NAME);
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RainApplication::CleanupTrayIcon()
{
    if (m_hwnd)
    {
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = m_hwnd;
        nid.uID = Config::TRAY_ICON_ID;
        Shell_NotifyIcon(NIM_DELETE, &nid);
    }
}

LRESULT CALLBACK RainApplication::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RainApplication *pApp = nullptr;

    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
        pApp = reinterpret_cast<RainApplication *>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
    }
    else
    {
        pApp = reinterpret_cast<RainApplication *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pApp)
    {
        return pApp->HandleMessage(uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT RainApplication::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case Config::WM_APP_TRAY_ICON:
        if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();

            AppendMenu(hMenu, m_autoRain ? MF_CHECKED : MF_STRING, 2, L"Auto Rain");
            AppendMenu(hMenu, MF_STRING, 3, L"Choose Color");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, 1, L"Exit");

            SetForegroundWindow(m_hwnd);

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, m_hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) // Toggle Auto Rain
        {
            m_autoRain = !m_autoRain;
        }
        else if (LOWORD(wParam) == 3) // Choose Color
        {
            CHOOSECOLOR cc = {0};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = m_hwnd;
            cc.lpCustColors = m_customColors;
            cc.rgbResult = m_rainColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            if (ChooseColor(&cc))
            {
                m_rainColor = cc.rgbResult;
            }
        }
        else if (LOWORD(wParam) == 1) // Exit
        {
            PostMessage(m_hwnd, WM_QUIT, 0, 0);
        }
        return 0;
    case Config::WM_APP_CREATE_RAINDROP:
        AddRaindrop();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK RainApplication::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
    {
        KBDLLHOOKSTRUCT *pKey = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        bool isModifier = (pKey->vkCode == VK_SHIFT || pKey->vkCode == VK_CONTROL ||
                           pKey->vkCode == VK_MENU || pKey->vkCode == VK_LSHIFT ||
                           pKey->vkCode == VK_RSHIFT || pKey->vkCode == VK_LCONTROL ||
                           pKey->vkCode == VK_RCONTROL || pKey->vkCode == VK_LMENU ||
                           pKey->vkCode == VK_RMENU);

        if (!isModifier && g_pAppInstance && !g_pAppInstance->m_autoRain)
        {
            g_pAppInstance->AddRaindrop();
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
