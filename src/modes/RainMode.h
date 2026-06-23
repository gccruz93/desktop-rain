#pragma once
#include "IMode.h"
#include <vector>
#include <random>

struct Raindrop
{
    float x, y;
    int length;
    float depth;
};

struct RainSplash
{
    float x, y;
    float vx, vy;
    float lifetime;
    float maxLifetime;
};

class RainMode : public IMode
{
public:
    RainMode(int screenWidth, int screenHeight);
    ~RainMode();

    void Update(float dt) override;
    void Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush) override;
    void Clear() override;
    bool HasActiveElements() const override;
    void AddElement() override;

    COLORREF GetColor() const override;
    void SetColor(COLORREF color) override;

private:
    float m_MAX_WIND_SPEED = 300.0f;
    float m_GRAVITY = 400.0f;
    size_t m_MAX_RAINDROPS = 1000;
    size_t m_MAX_PARTICLES = 5000;

    std::vector<Raindrop> m_raindrops;
    std::vector<RainSplash> m_particles;
    float m_raindropSpeedY;

    COLORREF m_rainColor = RGB(255, 255, 255);

    std::mt19937 m_gen;

    void UpdateRain(float dt);
    void SpawnSplash(float x);
};