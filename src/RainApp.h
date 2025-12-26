#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <random>
#include <memory>
#include "Config.h"
#include "Entities.h"

template <typename T>
struct ComDeleter
{
    void operator()(T *ptr) const
    {
        if (ptr)
            ptr->Release();
    }
};
template <typename T>
using ComPtr = std::unique_ptr<T, ComDeleter<T>>;

class RainApp
{
public:
    RainApp(HINSTANCE hInstance);
    ~RainApp();

    int Run();
    void AddRaindrop();
    void AddSnowflake();
    void AddMatrixColumn();

    // Customization
    bool m_autoMode = false;
    Config::AppMode m_currentMode = Config::AppMode::Rain;
    COLORREF m_rainColor = RGB(255, 255, 255);
    COLORREF m_snowColor = RGB(255, 255, 255);
    COLORREF m_matrixColor = RGB(0, 255, 70);
    COLORREF m_customColors[16] = {0};

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

    // Rain
    std::vector<Raindrop> m_raindrops;
    std::vector<RainSplash> m_particles;
    float m_raindropSpeedY;

    // Snow
    std::vector<Snowflake> m_snowflakes;

    // Matrix
    std::vector<MatrixColumn> m_matrixColumns;

    // Utils
    std::mt19937 m_gen;

    // Methods
    void Update(float dt);
    void UpdateRain(float dt);
    void UpdateSnow(float dt);
    void UpdateMatrix(float dt);

    void Render();
    void RenderRain();
    void RenderSnow();
    void RenderMatrix();

    void Clear();

    void SpawnRainSplash(float x);
    wchar_t GetRandomMatrixChar();

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
