#include "MatrixMode.h"
#include <algorithm>
#include <cmath>
#include <random>

MatrixMode::MatrixMode(int screenWidth, int screenHeight)
    : IMode(screenWidth, screenHeight)
{
    m_matrixColumns.reserve(m_MAX_COLUMNS);
    std::random_device rd;
    m_gen = std::mt19937(rd());
}

MatrixMode::~MatrixMode()
{
    Clear();
}

void MatrixMode::Update(float dt)
{
    UpdateMatrix(dt);
}

void MatrixMode::UpdateMatrix(float dt)
{
    UpdateAutoSpawn(dt, 0.05f);

    int maxRows = m_screenHeight / m_CHAR_SIZE;

    for (auto &col : m_matrixColumns)
    {
        if (!col.active)
            continue;

        col.jumpTimer += dt;
        if (col.jumpTimer >= col.jumpInterval)
        {
            col.jumpTimer = 0.0f;

            // Cascade: shift characters down
            for (int i = m_TRAIL_LENGTH - 1; i > 0; --i)
            {
                col.chars[i] = col.chars[i - 1];
            }
            col.chars[0] = GetRandomMatrixChar();

            col.gridY++;

            int trailTop = col.gridY - (m_TRAIL_LENGTH - 1);
            if (trailTop > maxRows)
            {
                col.active = false;
            }
        }
    }

    std::erase_if(m_matrixColumns, [](const MatrixColumn &c)
                  { return !c.active; });

    if (m_matrixColumns.size() > m_MAX_COLUMNS)
    {
        m_matrixColumns.erase(m_matrixColumns.begin(), m_matrixColumns.begin() + (m_matrixColumns.size() - m_MAX_COLUMNS));
    }
}

void MatrixMode::Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush)
{
    if (!m_textFormat)
        return;

    float baseR = GetRValue(m_matrixColor) / 255.0f;
    float baseG = GetGValue(m_matrixColor) / 255.0f;
    float baseB = GetBValue(m_matrixColor) / 255.0f;

    for (const auto &col : m_matrixColumns)
    {
        if (!col.active)
            continue;

        float pixelX = static_cast<float>(col.gridX * m_CHAR_SIZE);

        for (int i = 0; i < m_TRAIL_LENGTH; ++i)
        {
            int gridRow = col.gridY - i;
            float pixelY = static_cast<float>(gridRow * m_CHAR_SIZE);

            if (pixelY < -m_CHAR_SIZE || pixelY > m_screenHeight)
                continue;

            float fade;
            if (i == 0)
            {
                fade = 1.0f;
            }
            else
            {
                float linearFade = 1.0f - (static_cast<float>(i) / (m_TRAIL_LENGTH));
                fade = std::sqrt(linearFade);
                fade = std::max(0.15f, fade);
            }

            if (fade <= 0.0f)
                continue;

            D2D1_RECT_F rect = {
                pixelX,
                pixelY,
                pixelX + m_CHAR_SIZE,
                pixelY + m_CHAR_SIZE};

            // Draw minimal dark background
            float bgAlpha = (i == 0) ? 0.7f : fade * 0.5f;
            brush->SetColor(D2D1::ColorF(0.0f, 0.02f, 0.0f, bgAlpha));
            renderTarget->FillRectangle(rect, brush);

            // Set color for character
            if (i == 0)
            {
                brush->SetColor(D2D1::ColorF(
                    std::min(1.0f, baseR + 0.8f),
                    std::min(1.0f, baseG + 0.8f),
                    std::min(1.0f, baseB + 0.8f),
                    1.0f));
            }
            else
            {
                brush->SetColor(D2D1::ColorF(baseR * fade, baseG * fade, baseB * fade, fade));
            }

            wchar_t ch[2] = {col.chars[i], L'\0'};
            renderTarget->DrawText(
                ch,
                1,
                m_textFormat,
                rect,
                brush);
        }
    }
}

void MatrixMode::Clear()
{
    m_matrixColumns.clear();
}

bool MatrixMode::HasActiveElements() const
{
    return !m_matrixColumns.empty();
}

void MatrixMode::AddElement()
{
    int maxColumns = m_screenWidth / m_CHAR_SIZE;
    std::uniform_int_distribution<> distX(0, maxColumns - 1);
    std::uniform_real_distribution<float> distInterval(m_JUMP_INTERVAL_MIN, m_JUMP_INTERVAL_MAX);

    int gridX = distX(m_gen);

    for (const auto &col : m_matrixColumns)
    {
        if (col.gridX == gridX && col.active)
            return;
    }

    MatrixColumn newCol{};
    newCol.gridX = gridX;
    newCol.gridY = -1;
    newCol.jumpInterval = distInterval(m_gen);
    newCol.jumpTimer = 0.0f;
    newCol.active = true;

    for (int i = 0; i < m_TRAIL_LENGTH; ++i)
    {
        newCol.chars[i] = GetRandomMatrixChar();
    }

    m_matrixColumns.push_back(newCol);
}

wchar_t MatrixMode::GetRandomMatrixChar()
{
    static const wchar_t matrixChars[] =
        L"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン"
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%&*";
    static const int charCount = sizeof(matrixChars) / sizeof(wchar_t) - 1;

    std::uniform_int_distribution<> dist(0, charCount - 1);
    return matrixChars[dist(m_gen)];
}

COLORREF MatrixMode::GetColor() const
{
    return m_matrixColor;
}

void MatrixMode::SetColor(COLORREF color)
{
    m_matrixColor = color;
}

void MatrixMode::Configure(void *configData)
{
    if (configData)
    {
        m_textFormat = static_cast<IDWriteTextFormat *>(configData);
    }
}