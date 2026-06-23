#pragma once
#include "modes/IMode.h"
#include "modes/ModeFactory.h"
#include <memory>
#include <array>
#include <windows.h>
#include <d2d1.h>

class ModeManager
{
public:
    ModeManager(int screenWidth, int screenHeight);
    ~ModeManager() = default;

    void SetMode(ModeType m);
    void Update(float dt);
    void Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush);
    void Clear();

    bool HasActiveElements() const;
    void AddElement();

    ModeType GetCurrentMode() const { return m_currentMode; }
    void SetAutoMode(bool autoMode);
    bool GetAutoMode() const;

    COLORREF GetColor() const;
    void SetColor(COLORREF color);

    IMode *GetActiveMode() const { return m_activeMode.get(); }
    void SetMatrixTextFormat(IDWriteTextFormat *tf);

    std::array<COLORREF, 16> m_customColors{};

private:
    std::unique_ptr<IMode> m_activeMode;
    ModeType m_currentMode = ModeType::Rain;

    int m_screenWidth;
    int m_screenHeight;
    IDWriteTextFormat *m_matrixTextFormat = nullptr;
};
