#include "RainMode.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <windows.h>

RainMode::RainMode(int screenWidth, int screenHeight)
    : IMode(screenWidth, screenHeight)
{
    m_raindrops.reserve(m_MAX_RAINDROPS);
    m_particles.reserve(m_MAX_PARTICLES);
    m_raindropSpeedY = static_cast<float>(screenHeight) * 2.0f;

    std::random_device rd;
    m_gen = std::mt19937(rd());
}

RainMode::~RainMode()
{
    Clear();
}

void RainMode::Update(float dt)
{
    UpdateRain(dt);
}

void RainMode::UpdateRain(float dt)
{
    float windFactor = 0.0f;
    if (!m_raindrops.empty())
    {
        POINT mousePos;
        GetCursorPos(&mousePos);
        windFactor = (static_cast<float>(mousePos.x) - (m_screenWidth / 2.0f)) / (m_screenWidth / 2.0f);
    }

    UpdateAutoSpawn(dt, 0.05f);

    for (auto &drop : m_raindrops)
    {
        drop.x += (m_MAX_WIND_SPEED * windFactor * drop.depth) * dt;
        drop.y += (m_raindropSpeedY * drop.depth) * dt;

        if (drop.y > drop.groundY)
        {
            SpawnSplash(drop.x, drop.groundY);
        }
    }

    std::erase_if(m_raindrops, [this](const Raindrop &r)
                  { return r.y > r.groundY || r.x < -50.0f || r.x > m_screenWidth + 50.0f; });

    if (m_raindrops.size() > m_MAX_RAINDROPS)
    {
        m_raindrops.erase(m_raindrops.begin(), m_raindrops.begin() + (m_raindrops.size() - m_MAX_RAINDROPS));
    }

    for (auto &p : m_particles)
    {
        p.vy += m_GRAVITY * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.lifetime -= dt;
    }

    std::erase_if(m_particles, [](const RainSplash &p)
                  { return p.lifetime <= 0.0f; });

    if (m_particles.size() > m_MAX_PARTICLES)
    {
        m_particles.erase(m_particles.begin(), m_particles.begin() + (m_particles.size() - m_MAX_PARTICLES));
    }
}

void RainMode::Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush)
{
    float baseR = GetRValue(m_rainColor) / 255.0f;
    float baseG = GetGValue(m_rainColor) / 255.0f;
    float baseB = GetBValue(m_rainColor) / 255.0f;

    for (const auto &drop : m_raindrops)
    {
        float intensity = (drop.depth <= 0.4f) ? 0.2f : (drop.depth <= 0.7f ? 0.45f : 0.6f);

        brush->SetColor(D2D1::ColorF(baseR * intensity, baseG * intensity, baseB * intensity));

        D2D1_POINT_2F start = {drop.x, drop.y};
        D2D1_POINT_2F end = {drop.x, drop.y + drop.length};
        renderTarget->DrawLine(start, end, brush, 1.0f);
    }

    for (const auto &p : m_particles)
    {
        float progress = p.lifetime / p.maxLifetime;
        float brightness = 0.2f + (0.6f) * progress;

        brush->SetColor(D2D1::ColorF(baseR * brightness, baseG * brightness, baseB * brightness));

        D2D1_POINT_2F start = {p.x, p.y};
        D2D1_POINT_2F end = {p.x, p.y + 2.0f};
        renderTarget->DrawLine(start, end, brush, 1.0f);
    }
}

void RainMode::Clear()
{
    m_raindrops.clear();
    m_particles.clear();
}

bool RainMode::HasActiveElements() const
{
    return !m_raindrops.empty() || !m_particles.empty();
}

void RainMode::AddElement()
{
    std::uniform_int_distribution<> distX(m_spawnRegion.left, m_spawnRegion.right);
    std::uniform_real_distribution<> distDepth(0.4f, 1.0f);

    float depth = distDepth(m_gen);
    m_raindrops.emplace_back(Raindrop{
        .x = static_cast<float>(distX(m_gen)),
        .y = static_cast<float>(m_spawnRegion.top) - 50.0f,
        .groundY = static_cast<float>(m_spawnRegion.bottom),
        .length = 5 + static_cast<int>(25 * depth),
        .depth = depth});
}

void RainMode::SpawnSplash(float x, float groundY)
{
    std::uniform_int_distribution<> distCount(3, 5);
    std::uniform_real_distribution<float> distAngle(0.0f, 3.14159f);
    std::uniform_real_distribution<float> distSpeed(50.0f, 150.0f);
    std::uniform_real_distribution<float> distLife(0.2f, 0.5f);

    int count = distCount(m_gen);
    for (int i = 0; i < count; ++i)
    {
        float angle = distAngle(m_gen);
        float speed = distSpeed(m_gen);
        float life = distLife(m_gen);

        m_particles.emplace_back(RainSplash{
            .x = x,
            .y = groundY - 1.0f,
            .vx = std::cos(angle) * speed,
            .vy = -std::abs(std::sin(angle) * speed),
            .lifetime = life,
            .maxLifetime = life});
    }
}

COLORREF RainMode::GetColor() const
{
    return m_rainColor;
}

void RainMode::SetColor(COLORREF color)
{
    m_rainColor = color;
}
