#pragma once
#include "IMode.h"
#include "RainMode.h"
#include "SnowMode.h"
#include "MatrixMode.h"
#include <memory>
#include <concepts>

enum class ModeType
{
    Rain,
    Snow,
    Matrix
};

// Concept to ensure mode types derive from IMode
template <typename T>
concept DerivedFromIMode = std::derived_from<T, IMode>;

class ModeFactory
{
public:
    // Factory method using concepts
    template <DerivedFromIMode ModeT, typename... Args>
    static std::unique_ptr<IMode> Create(Args &&...args)
    {
        return std::make_unique<ModeT>(std::forward<Args>(args)...);
    }

    // Convenience method for creating by enum
    static std::unique_ptr<IMode> CreateByType(ModeType type, int screenWidth, int screenHeight)
    {
        switch (type)
        {
        case ModeType::Rain:
            return Create<RainMode>(screenWidth, screenHeight);
        case ModeType::Snow:
            return Create<SnowMode>(screenWidth, screenHeight);
        case ModeType::Matrix:
            return Create<MatrixMode>(screenWidth, screenHeight);
        default:
            return Create<RainMode>(screenWidth, screenHeight);
        }
    }
};
