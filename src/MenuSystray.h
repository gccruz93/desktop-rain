#pragma once

enum class MenuSystray : UINT_PTR
{
    Exit,
    ToggleAutoMode,
    ShowColorSelector,
    SetModeRain,
    SetModeSnow,
    SetModeMatrix,
    SetMonitorActive,
    SetMonitorAll,
    // Specific monitor slots: SetMonitorSpecific + index (up to 16 monitors)
    SetMonitorSpecific = 100,
};
