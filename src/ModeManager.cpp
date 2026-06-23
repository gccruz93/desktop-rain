#include "ModeManager.h"

ModeManager::ModeManager(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight)
{
    SetMode(ModeType::Rain);
}

void ModeManager::SetMode(ModeType m)
{
    if (m == m_currentMode && m_activeMode)
    {
        return;
    }

    m_currentMode = m;
    m_activeMode = ModeFactory::CreateByType(m, m_screenWidth, m_screenHeight);

    // Sync states
    if (m_activeMode)
    {
        m_activeMode->SetAutoMode(GetAutoMode());

        // Configure MatrixMode if needed
        if (m == ModeType::Matrix && m_matrixTextFormat)
        {
            m_activeMode->Configure(m_matrixTextFormat);
        }
    }
}

void ModeManager::Update(float dt)
{
    if (m_activeMode)
        m_activeMode->Update(dt);
}

void ModeManager::Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush)
{
    if (m_activeMode)
        m_activeMode->Render(renderTarget, brush);
}

void ModeManager::Clear()
{
    if (m_activeMode)
        m_activeMode->Clear();
}

bool ModeManager::HasActiveElements() const
{
    return m_activeMode ? m_activeMode->HasActiveElements() : false;
}

void ModeManager::AddElement()
{
    if (m_activeMode)
        m_activeMode->AddElement();
}

COLORREF ModeManager::GetColor() const
{
    return m_activeMode ? m_activeMode->GetColor() : RGB(255, 255, 255);
}

void ModeManager::SetColor(COLORREF color)
{
    if (m_activeMode)
        m_activeMode->SetColor(color);
}

void ModeManager::SetAutoMode(bool autoMode)
{
    if (m_activeMode)
    {
        m_activeMode->SetAutoMode(autoMode);
    }
}

bool ModeManager::GetAutoMode() const
{
    return m_activeMode ? m_activeMode->IsAutoModeEnabled() : false;
}

void ModeManager::SetMatrixTextFormat(IDWriteTextFormat *tf)
{
    m_matrixTextFormat = tf;

    // If current mode is Matrix, configure it immediately
    if (m_activeMode && m_currentMode == ModeType::Matrix)
    {
        m_activeMode->Configure(tf);
    }
}
