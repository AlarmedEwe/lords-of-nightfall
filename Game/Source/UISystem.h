#ifndef UI_SYSTEM_H
#define UI_SYSTEM_H

#include "raylib.h"
#include "GameObjects/PlayerObject.h"
#include <unordered_map>
#include <string>

struct EnemyObject; // Forward declaration

struct UISystem
{
    Texture2D panelTexture = {};
    Texture2D borderTexture = {};
    Texture2D buttonTexture = {};
    Texture2D dividerTexture = {};
    bool isLoaded = false;

    // Cache de Texturas de Ícones dos Itens
    std::unordered_map<std::string, Texture2D> iconCache;
    int selectedSlot = -1; // Slot selecionado no Inventário

    void Init();
    void Shutdown();

    // Icon helper
    Texture2D GetIcon(const std::string& path);

    // Render Helpers
    void DrawPanel(Rectangle rect, Color tint = (Color){25, 22, 35, 240});
    void DrawBorder(Rectangle rect, Color color = (Color){195, 155, 70, 255}, float thickness = 3.0f);
    
    // UI Controls
    bool DrawButton(Rectangle rect, const char* text, bool active = false);
    bool DrawTabButton(Rectangle rect, const char* text, bool selected);
    bool DrawSlider(Rectangle rect, const char* label, float& value, float minVal, float maxVal);
    bool DrawToggle(Rectangle rect, const char* label, bool& value);
    bool DrawOptionSelector(Rectangle rect, const char* label, const char* currentOption);
    void DrawDivider(float posX, float posY, float width);
    void DrawHeader(Rectangle rect, const char* title);

    // RPG HUD & Inventory System
    void DrawHUD(const PlayerObject& player, const char* promptText);
    void DrawInventory(PlayerObject& player, bool& isOpen, int& outDroppedSlot);
    void DrawEnemyOverheadUI(const EnemyObject& enemy, Camera3D camera, Vector3 playerPos);
};

#endif // UI_SYSTEM_H
