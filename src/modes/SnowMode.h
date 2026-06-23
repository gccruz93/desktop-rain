#pragma once
#include "IMode.h"
#include <vector>
#include <random>

struct Snowflake
{
    float x, y;
    float size;
    float baseSize;
    float speed;
    float swayOffset;
    float lifetime;
    float maxLifetime;
    float depth;
    bool onGround;
    float groundTimer;
};

class SnowMode : public IMode
{
public:
    SnowMode(int screenWidth, int screenHeight);
    ~SnowMode();

    void Update(float dt) override;
    void Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush) override;
    void Clear() override;
    bool HasActiveElements() const override;
    void AddElement() override;

    COLORREF GetColor() const override;
    void SetColor(COLORREF color) override;

private:
    float m_SNOW_BASE_SPEED = 80.0f;
    float m_SNOW_SWAY_AMPLITUDE = 30.0f;
    float m_SNOW_SWAY_FREQUENCY = 2.0f;
    float m_SNOW_MIN_SIZE = 2.0f;
    float m_SNOW_MAX_SIZE = 6.0f;
    float m_SNOW_LIFETIME_MIN = 3.0f;
    float m_SNOW_LIFETIME_MAX = 8.0f;
    float m_SNOW_GROUND_DURATION = 2.0f;
    size_t m_MAX_SNOWFLAKES = 500;

    std::vector<Snowflake> m_snowflakes;
    COLORREF m_snowColor = RGB(255, 255, 255);

    float m_elapsedTime = 0.0f;
    std::mt19937 m_gen;

    void UpdateSnow(float dt);
};
