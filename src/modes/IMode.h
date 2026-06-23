#pragma once
#include <windows.h>
#include <d2d1.h>
#include <array>
#include <span>

class IMode
{
public:
    IMode(int screenWidth, int screenHeight);
    virtual ~IMode();

    virtual void Update(float dt) = 0;
    virtual void Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush) = 0;
    virtual void Clear() = 0;
    virtual bool HasActiveElements() const = 0;
    virtual void AddElement() = 0;

    // Color management
    virtual COLORREF GetColor() const = 0;
    virtual void SetColor(COLORREF color) = 0;

    // Auto mode management
    bool IsAutoModeEnabled() const { return m_autoMode; }
    void SetAutoMode(bool enabled) { m_autoMode = enabled; }

    virtual void Configure(void *configData) {}

protected:
    void UpdateAutoSpawn(float dt, float interval);

    int m_screenWidth;
    int m_screenHeight;
    bool m_autoMode = false;

private:
    float m_autoSpawnTimer = 0.0f;
};
