#include "RainApp.h"
#include "Config.h"
#include "MenuSystray.h"
#include "modes/MatrixMode.h"
#include <shellapi.h>
#include <commdlg.h>
#include <chrono>
#include <format>

static RainApp *g_pAppInstance = nullptr;

RainApp::RainApp(HINSTANCE hInstance) : m_hInstance(hInstance)
{
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);

    m_modeManager = std::make_unique<ModeManager>(m_screenWidth, m_screenHeight);

    g_pAppInstance = this;
}

RainApp::~RainApp()
{
    m_modeManager->Clear();
    m_modeManager.release();
    m_modeManager = nullptr;

    if (m_hook)
    {
        UnhookWindowsHookEx(m_hook);
    }

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
            {
                m_running = false;
            }
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

            if (m_modeManager->HasActiveElements())
            {
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
            else
            {
                WaitMessage();
            }
        }
    }
    return 0;
}

void RainApp::Update(float dt)
{
    m_modeManager->Update(dt);
}

void RainApp::Render()
{
    if (!m_renderTarget)
    {
        CreateDeviceResources();
        if (!m_renderTarget)
        {
            return;
        }
    }

    m_renderTarget->BeginDraw();
    m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    m_renderTarget->Clear(D2D1::ColorF(1 / 255.0f, 0.0f, 1 / 255.0f, 1.0f));

    m_modeManager->Render(m_renderTarget.get(), m_brush.get());

    HRESULT hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
    }
}

void RainApp::Clear()
{
    m_modeManager->Clear();
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
    {
        return false;
    }

    m_d2dFactory.reset(rawFactory);

    IDWriteFactory *rawDWriteFactory = nullptr;
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown **>(&rawDWriteFactory));

    if (FAILED(hr))
    {
        return false;
    }

    m_dwriteFactory.reset(rawDWriteFactory);

    IDWriteTextFormat *rawTextFormat = nullptr;
    hr = m_dwriteFactory->CreateTextFormat(
        L"Consolas",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"en-us",
        &rawTextFormat);

    if (SUCCEEDED(hr))
    {
        m_matrixTextFormat.reset(rawTextFormat);
        m_matrixTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_matrixTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_modeManager->SetMatrixTextFormat(m_matrixTextFormat.get());
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
    {
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
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

            // Mode submenu
            ModeType currentMode = m_modeManager->GetCurrentMode();
            AppendMenuA(hModeMenu, (currentMode == ModeType::Rain) ? MF_CHECKED : MF_STRING, (UINT_PTR)MenuSystray::SetModeRain, "Rain");
            AppendMenuA(hModeMenu, (currentMode == ModeType::Snow) ? MF_CHECKED : MF_STRING, (UINT_PTR)MenuSystray::SetModeSnow, "Snow");
            AppendMenuA(hModeMenu, (currentMode == ModeType::Matrix) ? MF_CHECKED : MF_STRING, (UINT_PTR)MenuSystray::SetModeMatrix, "Matrix");
            AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hModeMenu, "Mode");

            AppendMenuA(hMenu, m_modeManager->GetAutoMode() ? MF_CHECKED : MF_STRING, (UINT_PTR)MenuSystray::ToggleAutoMode, "Auto Mode");
            AppendMenuA(hMenu, MF_STRING, (UINT_PTR)MenuSystray::ShowColorSelector, "Choose Color");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuA(hMenu, MF_STRING, (UINT_PTR)MenuSystray::Exit, "Exit");

            SetForegroundWindow(m_hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, m_hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case (UINT_PTR)MenuSystray::Exit:
            PostMessage(m_hwnd, WM_QUIT, 0, 0);
            break;
        case (UINT_PTR)MenuSystray::ToggleAutoMode:
            m_modeManager->SetAutoMode(!m_modeManager->GetAutoMode());
            break;
        case (UINT_PTR)MenuSystray::ShowColorSelector:
        {
            CHOOSECOLOR cc = {0};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = m_hwnd;
            cc.lpCustColors = m_modeManager->m_customColors.data();
            cc.rgbResult = m_modeManager->GetColor();
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            if (ChooseColor(&cc))
            {
                m_modeManager->SetColor(cc.rgbResult);
            }
            break;
        }
        case (UINT_PTR)MenuSystray::SetModeRain:
            m_modeManager->SetMode(ModeType::Rain);
            break;
        case (UINT_PTR)MenuSystray::SetModeSnow:
            m_modeManager->SetMode(ModeType::Snow);
            break;
        case (UINT_PTR)MenuSystray::SetModeMatrix:
            m_modeManager->SetMode(ModeType::Matrix);
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
            if (g_pAppInstance && !g_pAppInstance->m_modeManager->GetAutoMode())
            {
                g_pAppInstance->m_modeManager->AddElement();
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
