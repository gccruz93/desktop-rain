#include "RainApp.h"
#include <shellapi.h>
#include <commdlg.h>
#include <cmath>
#include <numbers>
#include <chrono>
#include <algorithm>
#include <format>
#include <ranges>

static RainApp *g_pAppInstance = nullptr;

RainApp::RainApp(HINSTANCE hInstance) : m_hInstance(hInstance)
{
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
    m_raindropSpeedY = static_cast<float>(m_screenHeight) * 2.0f;

    std::random_device rd;
    m_gen = std::mt19937(rd());

    m_raindrops.reserve(Config::MAX_RAINDROPS);
    m_particles.reserve(Config::MAX_PARTICLES);
    m_snowflakes.reserve(Config::MAX_SNOWFLAKES);
    m_matrixColumns.reserve(Config::MAX_MATRIX_COLUMNS);

    g_pAppInstance = this;
}

RainApp::~RainApp()
{
    if (m_hook)
        UnhookWindowsHookEx(m_hook);
    CleanupTrayIcon();
    g_pAppInstance = nullptr;
}

int RainApp::Run()
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

            bool hasActiveElements = !m_raindrops.empty() || !m_particles.empty() ||
                                     !m_snowflakes.empty() || !m_matrixColumns.empty();
            if (!hasActiveElements)
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

void RainApp::AddRaindrop()
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

void RainApp::AddSnowflake()
{
    std::uniform_int_distribution<> distX(0, m_screenWidth);
    std::uniform_real_distribution<float> distDepth(0.3f, 1.0f);
    std::uniform_real_distribution<float> distSize(Config::SNOW_MIN_SIZE, Config::SNOW_MAX_SIZE);
    std::uniform_real_distribution<float> distLifetime(Config::SNOW_LIFETIME_MIN, Config::SNOW_LIFETIME_MAX);
    std::uniform_real_distribution<float> distSway(0.0f, 2.0f * std::numbers::pi_v<float>);

    float depth = distDepth(m_gen);
    float size = distSize(m_gen) * depth;
    float lifetime = distLifetime(m_gen);

    m_snowflakes.emplace_back(Snowflake{
        .x = static_cast<float>(distX(m_gen)),
        .y = -20.0f,
        .size = size,
        .baseSize = size,
        .speed = Config::SNOW_BASE_SPEED * depth,
        .swayOffset = distSway(m_gen),
        .lifetime = lifetime,
        .maxLifetime = lifetime,
        .depth = depth,
        .onGround = false,
        .groundTimer = 0.0f});
}

void RainApp::AddMatrixColumn()
{
    int maxColumns = m_screenWidth / Config::MATRIX_CHAR_SIZE;
    std::uniform_int_distribution<> distX(0, maxColumns - 1);
    std::uniform_real_distribution<float> distInterval(Config::MATRIX_JUMP_INTERVAL_MIN, Config::MATRIX_JUMP_INTERVAL_MAX);

    int gridX = distX(m_gen);

    // Check if column already exists at this grid position
    for (const auto &col : m_matrixColumns)
    {
        if (col.gridX == gridX && col.active)
            return;
    }

    MatrixColumn newCol{};
    newCol.gridX = gridX;
    newCol.gridY = -1; // Start above screen
    newCol.jumpInterval = distInterval(m_gen);
    newCol.jumpTimer = 0.0f;
    newCol.active = true;

    // Initialize all chars with random characters
    for (int i = 0; i < Config::MATRIX_TRAIL_LENGTH; ++i)
    {
        newCol.chars[i] = GetRandomMatrixChar();
    }

    m_matrixColumns.push_back(newCol);
}

wchar_t RainApp::GetRandomMatrixChar()
{
    // Mix of Katakana, Latin characters, and numbers for Matrix effect
    static const wchar_t matrixChars[] =
        L"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン"
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%&*";
    static const int charCount = sizeof(matrixChars) / sizeof(wchar_t) - 1;

    std::uniform_int_distribution<> dist(0, charCount - 1);
    return matrixChars[dist(m_gen)];
}

void RainApp::Update(float dt)
{
    switch (m_currentMode)
    {
    case Config::AppMode::Rain:
        UpdateRain(dt);
        break;
    case Config::AppMode::Snow:
        UpdateSnow(dt);
        break;
    case Config::AppMode::Matrix:
        UpdateMatrix(dt);
        break;
    }
}

void RainApp::UpdateRain(float dt)
{
    float windFactor = 0.0f;
    if (!m_raindrops.empty())
    {
        POINT mousePos;
        GetCursorPos(&mousePos);
        windFactor = (static_cast<float>(mousePos.x) - (m_screenWidth / 2.0f)) / (m_screenWidth / 2.0f);
    }

    if (m_autoMode)
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
            SpawnRainSplash(drop.x);
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

    std::erase_if(m_particles, [](const RainSplash &p)
                  { return p.lifetime <= 0.0f; });

    if (m_particles.size() > Config::MAX_PARTICLES)
    {
        m_particles.erase(m_particles.begin(), m_particles.begin() + (m_particles.size() - Config::MAX_PARTICLES));
    }
}

void RainApp::UpdateSnow(float dt)
{
    static float elapsedTime = 0.0f;
    elapsedTime += dt;

    if (m_autoMode)
    {
        static float autoSnowTimer = 0.0f;
        autoSnowTimer += dt;
        if (autoSnowTimer >= 0.1f)
        {
            AddSnowflake();
            autoSnowTimer = 0.0f;
        }
    }

    for (auto &flake : m_snowflakes)
    {
        if (flake.onGround)
        {
            flake.groundTimer += dt;
            flake.size = flake.baseSize * (1.0f - (flake.groundTimer / Config::SNOW_GROUND_DURATION));
        }
        else
        {
            // Gentle swaying motion
            float sway = Config::SNOW_SWAY_AMPLITUDE * std::sin(elapsedTime * Config::SNOW_SWAY_FREQUENCY + flake.swayOffset);
            flake.x += sway * dt * flake.depth;
            flake.y += flake.speed * dt;

            // Decrease lifetime and size during fall
            flake.lifetime -= dt;
            float lifetimeRatio = flake.lifetime / flake.maxLifetime;
            flake.size = flake.baseSize * (0.3f + 0.7f * lifetimeRatio);

            // Check if hit ground or lifetime expired
            if (flake.y >= m_screenHeight - flake.size)
            {
                if (flake.lifetime > 0.5f)
                {
                    // Hit ground with remaining lifetime - stay on ground
                    flake.onGround = true;
                    flake.y = m_screenHeight - flake.size;
                    flake.groundTimer = 0.0f;
                }
            }
        }
    }

    std::erase_if(m_snowflakes, [](const Snowflake &s)
                  { 
                      if (s.onGround)
                          return s.groundTimer >= Config::SNOW_GROUND_DURATION;
                      return s.lifetime <= 0.0f || s.size < 0.5f; });

    if (m_snowflakes.size() > Config::MAX_SNOWFLAKES)
    {
        m_snowflakes.erase(m_snowflakes.begin(), m_snowflakes.begin() + (m_snowflakes.size() - Config::MAX_SNOWFLAKES));
    }
}

void RainApp::UpdateMatrix(float dt)
{
    if (m_autoMode)
    {
        static float autoMatrixTimer = 0.0f;
        autoMatrixTimer += dt;
        if (autoMatrixTimer >= 0.05f)
        {
            AddMatrixColumn();
            autoMatrixTimer = 0.0f;
        }
    }

    int maxRows = m_screenHeight / Config::MATRIX_CHAR_SIZE;

    for (auto &col : m_matrixColumns)
    {
        if (!col.active)
            continue;

        col.jumpTimer += dt;
        if (col.jumpTimer >= col.jumpInterval)
        {
            col.jumpTimer = 0.0f;

            // Cascade: shift characters down (last takes previous, etc.)
            for (int i = Config::MATRIX_TRAIL_LENGTH - 1; i > 0; --i)
            {
                col.chars[i] = col.chars[i - 1];
            }
            // Head gets a new random character
            col.chars[0] = GetRandomMatrixChar();

            // Move head down one grid cell
            col.gridY++;

            // Check if entire trail is off screen
            int trailTop = col.gridY - (Config::MATRIX_TRAIL_LENGTH - 1);
            if (trailTop > maxRows)
            {
                col.active = false;
            }
        }
    }

    std::erase_if(m_matrixColumns, [](const MatrixColumn &c)
                  { return !c.active; });

    if (m_matrixColumns.size() > Config::MAX_MATRIX_COLUMNS)
    {
        m_matrixColumns.erase(m_matrixColumns.begin(), m_matrixColumns.begin() + (m_matrixColumns.size() - Config::MAX_MATRIX_COLUMNS));
    }
}

void RainApp::SpawnRainSplash(float x)
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

        m_particles.emplace_back(RainSplash{
            .x = x,
            .y = static_cast<float>(m_screenHeight - 1),
            .vx = std::cos(angle) * speed,
            .vy = -std::abs(std::sin(angle) * speed),
            .lifetime = distLife(m_gen),
            .maxLifetime = distLife(m_gen)});
        m_particles.back().maxLifetime = m_particles.back().lifetime;
    }
}

void RainApp::Render()
{
    if (!m_renderTarget)
    {
        CreateDeviceResources();
        if (!m_renderTarget)
            return;
    }

    m_renderTarget->BeginDraw();
    m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    // Use transparent background for all modes
    m_renderTarget->Clear(D2D1::ColorF(1 / 255.0f, 0.0f, 1 / 255.0f, 1.0f));

    switch (m_currentMode)
    {
    case Config::AppMode::Rain:
        RenderRain();
        break;
    case Config::AppMode::Snow:
        RenderSnow();
        break;
    case Config::AppMode::Matrix:
        RenderMatrix();
        break;
    }

    HRESULT hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
    }
}

void RainApp::RenderRain()
{
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
}

void RainApp::RenderSnow()
{
    float baseR = GetRValue(m_snowColor) / 255.0f;
    float baseG = GetGValue(m_snowColor) / 255.0f;
    float baseB = GetBValue(m_snowColor) / 255.0f;

    m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    for (const auto &flake : m_snowflakes)
    {
        float alpha = flake.depth;
        if (flake.onGround)
        {
            alpha *= (1.0f - (flake.groundTimer / Config::SNOW_GROUND_DURATION));
        }

        m_brush->SetColor(D2D1::ColorF(baseR, baseG, baseB, alpha));

        D2D1_ELLIPSE ellipse = {
            {flake.x, flake.y},
            flake.size,
            flake.size};
        m_renderTarget->FillEllipse(ellipse, m_brush.get());
    }
}

void RainApp::RenderMatrix()
{
    if (!m_matrixTextFormat)
        return;

    float baseR = GetRValue(m_matrixColor) / 255.0f;
    float baseG = GetGValue(m_matrixColor) / 255.0f;
    float baseB = GetBValue(m_matrixColor) / 255.0f;

    for (const auto &col : m_matrixColumns)
    {
        if (!col.active)
            continue;

        float pixelX = static_cast<float>(col.gridX * Config::MATRIX_CHAR_SIZE);

        for (int i = 0; i < Config::MATRIX_TRAIL_LENGTH; ++i)
        {
            int gridRow = col.gridY - i;
            float pixelY = static_cast<float>(gridRow * Config::MATRIX_CHAR_SIZE);

            // Skip if off screen
            if (pixelY < -Config::MATRIX_CHAR_SIZE || pixelY > m_screenHeight)
                continue;

            // Calculate fade based on position in trail (0 = head, 5 = tail)
            float fade;
            if (i == 0)
            {
                fade = 1.0f;
            }
            else
            {
                // Fade from 1.0 to 0.0 over the trail length
                fade = 1.0f - (static_cast<float>(i) / (Config::MATRIX_TRAIL_LENGTH + Config::MATRIX_TRAIL_LENGTH / 1.5));
                fade = std::max(0.15f, fade);
            }

            // Skip rendering if fully faded
            if (fade <= 0.0f)
                continue;

            D2D1_RECT_F rect = {
                pixelX,
                pixelY,
                pixelX + Config::MATRIX_CHAR_SIZE,
                pixelY + Config::MATRIX_CHAR_SIZE};

            // Set color for the character
            if (i == 0)
            {
                // Head - bright white/green
                m_brush->SetColor(D2D1::ColorF(
                    std::min(1.0f, baseR + 0.8f),
                    std::min(1.0f, baseG + 0.8f),
                    std::min(1.0f, baseB + 0.8f),
                    1.0f));
            }
            else
            {
                m_brush->SetColor(D2D1::ColorF(baseR * fade, baseG * fade, baseB * fade, fade));
            }

            wchar_t ch[2] = {col.chars[i], L'\0'};
            m_renderTarget->DrawText(
                ch,
                1,
                m_matrixTextFormat.get(),
                rect,
                m_brush.get());
        }
    }
}

void RainApp::Clear()
{
    m_raindrops.clear();
    m_snowflakes.clear();
    m_matrixColumns.clear();
    m_particles.clear();
}

bool RainApp::InitializeWindow()
{
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = Config::APP_NAME;
    wc.hbrBackground = nullptr;

    RegisterClassA(&wc);

    m_hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        Config::APP_NAME, Config::APP_NAME,
        WS_POPUP, 0, 0, m_screenWidth, m_screenHeight,
        nullptr, nullptr, m_hInstance, this);

    if (!m_hwnd)
        return false;

    SetLayeredWindowAttributes(m_hwnd, RGB(1, 0, 1), 0, LWA_COLORKEY);
    ShowWindow(m_hwnd, SW_SHOW);
    return true;
}

bool RainApp::InitializeDirect2D()
{
    ID2D1Factory *rawFactory = nullptr;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &rawFactory);
    if (FAILED(hr))
        return false;

    m_d2dFactory.reset(rawFactory);

    // Create DirectWrite factory for Matrix mode text
    IDWriteFactory *rawDWriteFactory = nullptr;
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown **>(&rawDWriteFactory));

    if (FAILED(hr))
        return false;

    m_dwriteFactory.reset(rawDWriteFactory);

    // Create text format for Matrix characters
    IDWriteTextFormat *rawTextFormat = nullptr;
    hr = m_dwriteFactory->CreateTextFormat(
        L"Consolas",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        static_cast<float>(Config::MATRIX_CHAR_SIZE),
        L"en-us",
        &rawTextFormat);

    if (SUCCEEDED(hr))
    {
        m_matrixTextFormat.reset(rawTextFormat);
        m_matrixTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_matrixTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    return true;
}

void RainApp::CreateDeviceResources()
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

void RainApp::DiscardDeviceResources()
{
    m_renderTarget.reset();
    m_brush.reset();
}

void RainApp::SetupTrayIcon()
{
    NOTIFYICONDATAA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hwnd;
    nid.uID = Config::TRAY_ICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = Config::WM_APP_TRAY_ICON;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    if (!nid.hIcon)
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy_s(nid.szTip, Config::APP_NAME);
    Shell_NotifyIconA(NIM_ADD, &nid);
}

void RainApp::CleanupTrayIcon()
{
    if (m_hwnd)
    {
        NOTIFYICONDATAA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATAA);
        nid.hWnd = m_hwnd;
        nid.uID = Config::TRAY_ICON_ID;
        Shell_NotifyIconA(NIM_DELETE, &nid);
    }
}

LRESULT CALLBACK RainApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RainApp *pApp = nullptr;

    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
        pApp = reinterpret_cast<RainApp *>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
    }
    else
    {
        pApp = reinterpret_cast<RainApp *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pApp)
    {
        return pApp->HandleMessage(uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT RainApp::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case Config::WM_APP_TRAY_ICON:
        if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);

            auto menuText = std::format("{} v{}", Config::APP_NAME, Config::APP_VERSION);

            HMENU hMenu = CreatePopupMenu();
            HMENU hModeMenu = CreatePopupMenu();

            AppendMenuA(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED, 0, menuText.c_str());
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

            // Submenu Modes
            AppendMenuA(hModeMenu, (m_currentMode == Config::AppMode::Rain) ? MF_CHECKED : MF_STRING, 10, "Rain");
            AppendMenuA(hModeMenu, (m_currentMode == Config::AppMode::Snow) ? MF_CHECKED : MF_STRING, 11, "Snow");
            AppendMenuA(hModeMenu, (m_currentMode == Config::AppMode::Matrix) ? MF_CHECKED : MF_STRING, 12, "Matrix");
            AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hModeMenu, "Mode");

            AppendMenuA(hMenu, m_autoMode ? MF_CHECKED : MF_STRING, 2, "Auto Mode");
            AppendMenuA(hMenu, MF_STRING, 3, "Choose Color");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuA(hMenu, MF_STRING, 1, "Exit");

            SetForegroundWindow(m_hwnd);

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, m_hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: // Exit
            PostMessage(m_hwnd, WM_QUIT, 0, 0);
            break;
        case 2: // Toggle Auto Mode
            m_autoMode = !m_autoMode;
            break;
        case 3: // Choose Color
        {
            CHOOSECOLOR cc = {0};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = m_hwnd;
            cc.lpCustColors = m_customColors;

            // Set initial color based on current mode
            switch (m_currentMode)
            {
            case Config::AppMode::Rain:
                cc.rgbResult = m_rainColor;
                break;
            case Config::AppMode::Snow:
                cc.rgbResult = m_snowColor;
                break;
            case Config::AppMode::Matrix:
                cc.rgbResult = m_matrixColor;
                break;
            }
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            if (ChooseColor(&cc))
            {
                switch (m_currentMode)
                {
                case Config::AppMode::Rain:
                    m_rainColor = cc.rgbResult;
                    break;
                case Config::AppMode::Snow:
                    m_snowColor = cc.rgbResult;
                    break;
                case Config::AppMode::Matrix:
                    m_matrixColor = cc.rgbResult;
                    break;
                }
            }
            break;
        }
        case 10: // Rain mode
            m_currentMode = Config::AppMode::Rain;
            Clear();
            break;
        case 11: // Snow mode
            m_currentMode = Config::AppMode::Snow;
            Clear();
            break;
        case 12: // Matrix mode
            m_currentMode = Config::AppMode::Matrix;
            Clear();
            break;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK RainApp::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
    {
        KBDLLHOOKSTRUCT *pKey = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        switch (pKey->vkCode)
        {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_LMENU:
        case VK_RMENU:
            break;
        default:
            if (g_pAppInstance && !g_pAppInstance->m_autoMode)
            {
                switch (g_pAppInstance->m_currentMode)
                {
                case Config::AppMode::Rain:
                    g_pAppInstance->AddRaindrop();
                    break;
                case Config::AppMode::Snow:
                    g_pAppInstance->AddSnowflake();
                    break;
                case Config::AppMode::Matrix:
                    g_pAppInstance->AddMatrixColumn();
                    break;
                }
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
