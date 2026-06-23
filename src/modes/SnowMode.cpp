#include "SnowMode.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

SnowMode::SnowMode(int screenWidth, int screenHeight)
    : IMode(screenWidth, screenHeight)
{
    m_snowflakes.reserve(m_MAX_SNOWFLAKES);
    std::random_device rd;
    m_gen = std::mt19937(rd());
}

SnowMode::~SnowMode()
{
    Clear();
}

void SnowMode::Update(float dt)
{
    UpdateSnow(dt);
}

void SnowMode::UpdateSnow(float dt)
{
    m_elapsedTime += dt;

    UpdateAutoSpawn(dt, 0.1f);

    for (auto &flake : m_snowflakes)
    {
        if (flake.onGround)
        {
            flake.groundTimer += dt;
            flake.size = flake.baseSize * (1.0f - (flake.groundTimer / m_SNOW_GROUND_DURATION));
        }
        else
        {
            // Gentle swaying motion
            float sway = m_SNOW_SWAY_AMPLITUDE * std::sin(m_elapsedTime * m_SNOW_SWAY_FREQUENCY + flake.swayOffset);
            flake.x += sway * dt * flake.depth;
            flake.y += flake.speed * dt;

            // Decrease lifetime and size during fall
            flake.lifetime -= dt;
            float lifetimeRatio = flake.lifetime / flake.maxLifetime;
            flake.size = flake.baseSize * (0.3f + 0.7f * lifetimeRatio);

            // Check if hit ground or lifetime expired
            if (flake.y >= m_screenHeight - flake.size)
            {
                if (flake.lifetime > 0.5f)
                {
                    // Hit ground with remaining lifetime - stay on ground
                    flake.onGround = true;
                    flake.y = m_screenHeight - flake.size;
                    flake.groundTimer = 0.0f;
                }
            }
        }
    }

    std::erase_if(m_snowflakes, [](const Snowflake &s)
                  {
                      if (s.onGround)
                          return s.groundTimer >= 2.0f;
                      return s.lifetime <= 0.0f || s.size < 0.5f; });

    if (m_snowflakes.size() > m_MAX_SNOWFLAKES)
    {
        m_snowflakes.erase(m_snowflakes.begin(), m_snowflakes.begin() + (m_snowflakes.size() - m_MAX_SNOWFLAKES));
    }
}

void SnowMode::Render(ID2D1HwndRenderTarget *renderTarget, ID2D1SolidColorBrush *brush)
{
    float baseR = GetRValue(m_snowColor) / 255.0f;
    float baseG = GetGValue(m_snowColor) / 255.0f;
    float baseB = GetBValue(m_snowColor) / 255.0f;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    for (const auto &flake : m_snowflakes)
    {
        float alpha = flake.depth;
        if (flake.onGround)
        {
            alpha *= (1.0f - (flake.groundTimer / m_SNOW_GROUND_DURATION));
        }

        brush->SetColor(D2D1::ColorF(baseR, baseG, baseB, alpha));

        D2D1_ELLIPSE ellipse = {
            {flake.x, flake.y},
            flake.size,
            flake.size};
        renderTarget->FillEllipse(ellipse, brush);
    }
}

void SnowMode::Clear()
{
    m_snowflakes.clear();
}

bool SnowMode::HasActiveElements() const
{
    return !m_snowflakes.empty();
}

void SnowMode::AddElement()
{
    std::uniform_int_distribution<> distX(0, m_screenWidth);
    std::uniform_real_distribution<float> distDepth(0.3f, 1.0f);
    std::uniform_real_distribution<float> distSize(m_SNOW_MIN_SIZE, m_SNOW_MAX_SIZE);
    std::uniform_real_distribution<float> distLifetime(m_SNOW_LIFETIME_MIN, m_SNOW_LIFETIME_MAX);
    std::uniform_real_distribution<float> distSway(0.0f, 2.0f * std::numbers::pi_v<float>);

    float depth = distDepth(m_gen);
    float size = distSize(m_gen) * depth;
    float lifetime = distLifetime(m_gen);

    m_snowflakes.emplace_back(Snowflake{
        .x = static_cast<float>(distX(m_gen)),
        .y = -20.0f,
        .size = size,
        .baseSize = size,
        .speed = m_SNOW_BASE_SPEED * depth,
        .swayOffset = distSway(m_gen),
        .lifetime = lifetime,
        .maxLifetime = lifetime,
        .depth = depth,
        .onGround = false,
        .groundTimer = 0.0f});
}

COLORREF SnowMode::GetColor() const
{
    return m_snowColor;
}

void SnowMode::SetColor(COLORREF color)
{
    m_snowColor = color;
}
