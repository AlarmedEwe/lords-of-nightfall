#include "UISystem.h"
#include "UI/UIWidgets.h"
#include "UI/HUDView.h"
#include "UI/InventoryView.h"
#include "UI/EnemyUI.h"

void UISystem::Init()
{
    panelTexture = UIWidgets::TryLoadTexture("UI/Panel/panel-000.png");
    borderTexture = UIWidgets::TryLoadTexture("UI/Border/panel-border-000.png");
    buttonTexture = UIWidgets::TryLoadTexture("UI/Panel/panel-014.png");
    dividerTexture = UIWidgets::TryLoadTexture("UI/Divider/divider-000.png");

    isLoaded = (panelTexture.id > 0 || borderTexture.id > 0);
}

void UISystem::Shutdown()
{
    if (panelTexture.id > 0) UnloadTexture(panelTexture);
    if (borderTexture.id > 0) UnloadTexture(borderTexture);
    if (buttonTexture.id > 0) UnloadTexture(buttonTexture);
    if (dividerTexture.id > 0) UnloadTexture(dividerTexture);

    for (auto& pair : iconCache)
    {
        if (pair.second.id > 0) UnloadTexture(pair.second);
    }
    iconCache.clear();

    isLoaded = false;
}

Texture2D UISystem::GetIcon(const std::string& path)
{
    if (path.empty()) return (Texture2D){ 0 };
    auto it = iconCache.find(path);
    if (it != iconCache.end()) return it->second;

    Texture2D tex = UIWidgets::TryLoadTexture(path.c_str());
    if (tex.id > 0)
    {
        iconCache[path] = tex;
        return tex;
    }
    return (Texture2D){ 0 };
}

void UISystem::DrawPanel(Rectangle rect, Color tint)
{
    UIWidgets::DrawPanel(*this, rect, tint);
}

void UISystem::DrawBorder(Rectangle rect, Color color, float thickness)
{
    UIWidgets::DrawBorder(*this, rect, color, thickness);
}

bool UISystem::DrawButton(Rectangle rect, const char* text, bool active)
{
    return UIWidgets::DrawButton(*this, rect, text, active);
}

bool UISystem::DrawTabButton(Rectangle rect, const char* text, bool selected)
{
    return UIWidgets::DrawTabButton(rect, text, selected);
}

bool UISystem::DrawSlider(Rectangle rect, const char* label, float& value, float minVal, float maxVal)
{
    return UIWidgets::DrawSlider(rect, label, value, minVal, maxVal);
}

bool UISystem::DrawToggle(Rectangle rect, const char* label, bool& value)
{
    return UIWidgets::DrawToggle(rect, label, value);
}

bool UISystem::DrawOptionSelector(Rectangle rect, const char* label, const char* currentOption)
{
    return UIWidgets::DrawOptionSelector(*this, rect, label, currentOption);
}

void UISystem::DrawDivider(float posX, float posY, float width)
{
    UIWidgets::DrawDivider(*this, posX, posY, width);
}

void UISystem::DrawHeader(Rectangle rect, const char* title)
{
    UIWidgets::DrawHeader(*this, rect, title);
}

void UISystem::DrawHUD(const PlayerObject& player, const char* promptText)
{
    HUDView::Draw(player, promptText, *this);
}

void UISystem::DrawInventory(PlayerObject& player, bool& isOpen, int& outDroppedSlot)
{
    InventoryView::Draw(player, isOpen, outDroppedSlot, *this);
}

void UISystem::DrawEnemyOverheadUI(const EnemyObject& enemy, Camera3D camera, Vector3 playerPos)
{
    EnemyUI::DrawOverhead(enemy, camera, playerPos, *this);
}
