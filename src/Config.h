#pragma once

namespace Config
{
    // App
    constexpr char APP_NAME[] = "Desktop Rain";
    constexpr char APP_VERSION[] = "1.1";
    constexpr int TRAY_ICON_ID = 1;
    constexpr UINT WM_APP_TRAY_ICON = WM_APP + 1;
    constexpr UINT WM_APP_CREATE_RAINDROP = WM_APP + 2;

    // Modes
    enum class AppMode
    {
        Rain,
        Snow,
        Matrix
    };

    // Rain
    constexpr float MAX_WIND_SPEED = 300.0f;
    constexpr float GRAVITY = 400.0f;
    constexpr size_t MAX_RAINDROPS = 100;
    constexpr size_t MAX_PARTICLES = 500;

    // Snow
    constexpr float SNOW_BASE_SPEED = 80.0f;
    constexpr float SNOW_SWAY_AMPLITUDE = 30.0f;
    constexpr float SNOW_SWAY_FREQUENCY = 2.0f;
    constexpr float SNOW_MIN_SIZE = 2.0f;
    constexpr float SNOW_MAX_SIZE = 7.0f;
    constexpr float SNOW_LIFETIME_MIN = 3.0f;
    constexpr float SNOW_LIFETIME_MAX = 10.0f;
    constexpr float SNOW_GROUND_DURATION = 2.0f;
    constexpr size_t MAX_SNOWFLAKES = 100;

    // Matrix
    constexpr float MATRIX_JUMP_INTERVAL_MIN = 0.02f;
    constexpr float MATRIX_JUMP_INTERVAL_MAX = 0.05f;
    constexpr int MATRIX_CHAR_SIZE = 14;
    constexpr int MATRIX_TRAIL_LENGTH = 10;
    constexpr size_t MAX_MATRIX_COLUMNS = 270; // 3840 / 14 = 270. To support 4k monitors
}
