#pragma once
#include <windows.h>

namespace Config
{
    constexpr wchar_t APP_NAME[] = L"Desktop Rain";
    constexpr int TRAY_ICON_ID = 1;
    constexpr UINT WM_APP_TRAY_ICON = WM_APP + 2;
    constexpr UINT WM_APP_CREATE_RAINDROP = WM_APP + 1;

    constexpr float MAX_WIND_SPEED = 300.0f;
    constexpr float GRAVITY = 400.0f;
    constexpr size_t MAX_RAINDROPS = 1000;
    constexpr size_t MAX_PARTICLES = 5000;
}
