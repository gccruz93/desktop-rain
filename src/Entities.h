#pragma once

struct Raindrop
{
    float x, y;
    int length;
    float depth; // 0.0 to 1.0
};

struct Particle
{
    float x, y;
    float vx, vy;
    float lifetime;
    float maxLifetime;
};
