#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <memory>
#include <vector>
#include "Utils.h"
#include "ModeManager.h"
#include "Config.h"

struct MonitorInfo
{
    HMONITOR handle;
    RECT     screenRect; // coordenadas do virtual screen (Win32)
    RECT     d2dRect;    // coordenadas D2D (relativas ao canto superior esquerdo da janela)
};

class RainApp
{
public:
    RainApp(HINSTANCE hInstance);
    ~RainApp();

    int Run();

private:
    // System
    HINSTANCE m_hInstance;
    HWND      m_hwnd    = nullptr;
    HHOOK     m_hook    = nullptr;
    bool      m_running = true;

    // Virtual desktop dimensions
    int m_virtualX = 0;
    int m_virtualY = 0;
    int m_virtualW = 0;
    int m_virtualH = 0;

    // Monitor list and target
    std::vector<MonitorInfo> m_monitors;
    MonitorTarget            m_monitorTarget        = MonitorTarget::Active;
    int                      m_selectedMonitorIndex = 0;

    // Rendering
    ComPtr<ID2D1Factory>          m_d2dFactory;
    ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    ComPtr<ID2D1SolidColorBrush>  m_brush;
    ComPtr<IDWriteFactory>        m_dwriteFactory;
    ComPtr<IDWriteTextFormat>     m_matrixTextFormat;

    // Mode Management
    std::unique_ptr<ModeManager> m_modeManager;

    // Methods
    void Update(float dt);
    void Render();
    void Clear();

    bool InitializeWindow();
    bool InitializeDirect2D();
    void CreateDeviceResources();
    void DiscardDeviceResources();

    void EnumerateMonitors();
    RECT GetAutoSpawnRect() const;
    void AddElementForTarget();

    void SetupTrayIcon();
    void CleanupTrayIcon();

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC hdc, LPRECT lpRect, LPARAM lParam);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};
