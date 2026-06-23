#pragma once
#include "IMode.h"
#include <vector>
#include <random>
#include <dwrite.h>

static constexpr int MATRIX_TRAIL_LENGTH = 10;

struct MatrixColumn
{
    int gridX;
    int gridY;
    float jumpInterval;
    float jumpTimer;
    wchar_t chars[MATRIX_TRAIL_LENGTH];
    bool active;
};

class MatrixMode : public IMode
{
public:
    MatrixMode(int screenWidth, int screenHeight);
    ~MatrixMode();

    void Update(float dt) override;
    void Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush) override;
    void Clear() override;
    bool HasActiveElements() const override;
    void AddElement() override;

    COLORREF GetColor() const override;
    void SetColor(COLORREF color) override;

    void Configure(void *configData) override;

private:
    std::vector<MatrixColumn> m_matrixColumns;
    COLORREF m_matrixColor = RGB(0, 255, 70);

    float m_JUMP_INTERVAL_MIN = 0.02f;
    float m_JUMP_INTERVAL_MAX = 0.05f;
    int m_CHAR_SIZE = 14;
    static constexpr int m_TRAIL_LENGTH = MATRIX_TRAIL_LENGTH;
    size_t m_MAX_COLUMNS = 270; // 3840 / 14 = 270. To support 4k monitors

    std::mt19937 m_gen;
    IDWriteTextFormat *m_textFormat = nullptr;

    wchar_t GetRandomMatrixChar();
    void UpdateMatrix(float dt);
};
