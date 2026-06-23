#include "IMode.h"

IMode::IMode(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight)
{
}

IMode::~IMode()
{
}

void IMode::UpdateAutoSpawn(float dt, float interval)
{
    if (!m_autoMode)
    {
        m_autoSpawnTimer = 0.0f;
        return;
    }

    m_autoSpawnTimer += dt;
    if (m_autoSpawnTimer >= interval)
    {
        AddElement();
        m_autoSpawnTimer = 0.0f;
    }
}
