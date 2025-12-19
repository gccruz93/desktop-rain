#pragma once
#include <windows.h>
#include <d2d1.h>
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

class RainApplication
{
public:
    RainApplication(HINSTANCE hInstance);
    ~RainApplication();

    int Run();
    void AddRaindrop();

    // Customization
    bool m_autoRain = false;
    COLORREF m_rainColor = RGB(255, 255, 255);
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

    // Simulation
    std::vector<Raindrop> m_raindrops;
    std::vector<Particle> m_particles;
    std::mt19937 m_gen;
    float m_raindropSpeedY;

    // Methods
    void Update(float dt);
    void Render();
    void SpawnSplash(float x);

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
