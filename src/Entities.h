#pragma once
#include <string>

// Rain
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

// Snow
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

// Matrix
struct MatrixColumn
{
    int gridX;
    int gridY;
    float jumpInterval;
    float jumpTimer;
    wchar_t chars[Config::MATRIX_TRAIL_LENGTH];
    bool active;
};
