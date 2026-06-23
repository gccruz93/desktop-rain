#pragma once

namespace Config
{
    constexpr char APP_NAME[] = "Desktop Rain";
    constexpr char APP_VERSION[] = "1.3";
    constexpr int TRAY_ICON_ID = 1;
    constexpr UINT WM_APP_TRAY_ICON = WM_APP + 1;
}

enum class MonitorTarget
{
    Active,   // monitor onde está a janela em foco
    All,      // todos os monitores simultaneamente
    Specific, // monitor selecionado pelo usuário
};
