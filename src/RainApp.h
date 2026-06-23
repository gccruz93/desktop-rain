#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <memory>
#include "Utils.h"
#include "ModeManager.h"

class RainApp
{
public:
    RainApp(HINSTANCE hInstance);
    ~RainApp();

    int Run();

private:
    // System
    HINSTANCE m_hInstance;
    HWND m_hwnd = nullptr;
    HHOOK m_hook = nullptr;
    bool m_running = true;
    int m_screenWidth;
    int m_screenHeight;

    // Rendering
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    ComPtr<ID2D1SolidColorBrush> m_brush;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<IDWriteTextFormat> m_matrixTextFormat;

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

    void SetupTrayIcon();
    void CleanupTrayIcon();

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};
