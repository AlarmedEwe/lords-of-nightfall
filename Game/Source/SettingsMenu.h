#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include "UISystem.h"
#include "Settings.h"

enum class SettingsTab
{
    Graphics,
    Gameplay,
    Audio
};

struct SettingsMenu
{
    bool isOpen = false;
    SettingsTab currentTab = SettingsTab::Graphics;

    void Draw(UISystem& ui, Settings& settings, int screenWidth, int screenHeight);
};

#endif // SETTINGS_MENU_H
