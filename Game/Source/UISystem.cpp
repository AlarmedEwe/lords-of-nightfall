#include "UISystem.h"
#include "GameObjects/EnemyObject.h"
#include "raymath.h"
#include <cstdio>

static Texture2D TryLoadTexture(const char* relPath)
{
    const char* candidatePaths[] = {
        relPath,
        TextFormat("Assets/%s", relPath),
        TextFormat("Build/Development/Assets/%s", relPath),
        TextFormat("Game/Assets/kenney_fantasy-ui-borders/PNG/Double/%s", relPath),
        TextFormat("Game/Assets/Ultimate RPG Items Pack/%s", relPath)
    };

    for (const char* path : candidatePaths)
    {
        if (FileExists(path))
        {
            Texture2D tex = LoadTexture(path);
            if (tex.id > 0)
                return tex;
        }
    }
    return (Texture2D){ 0 };
}

void UISystem::Init()
{
    panelTexture = TryLoadTexture("UI/Panel/panel-000.png");
    borderTexture = TryLoadTexture("UI/Border/panel-border-000.png");
    buttonTexture = TryLoadTexture("UI/Panel/panel-014.png");
    dividerTexture = TryLoadTexture("UI/Divider/divider-000.png");

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

    Texture2D tex = TryLoadTexture(path.c_str());
    if (tex.id > 0)
    {
        iconCache[path] = tex;
        return tex;
    }
    return (Texture2D){ 0 };
}

void UISystem::DrawPanel(Rectangle rect, Color tint)
{
    if (panelTexture.id > 0)
    {
        NPatchInfo nPatch = {
            { 0.0f, 0.0f, (float)panelTexture.width, (float)panelTexture.height },
            16, 16, 16, 16,
            NPATCH_NINE_PATCH
        };
        DrawTextureNPatch(panelTexture, nPatch, rect, (Vector2){0, 0}, 0.0f, tint);
    }
    else
    {
        DrawRectangleRounded(rect, 0.05f, 4, tint);
    }
    DrawBorder(rect, (Color){185, 145, 60, 255}, 2.0f);
}

void UISystem::DrawBorder(Rectangle rect, Color color, float thickness)
{
    if (borderTexture.id > 0)
    {
        NPatchInfo nPatch = {
            { 0.0f, 0.0f, (float)borderTexture.width, (float)borderTexture.height },
            16, 16, 16, 16,
            NPATCH_NINE_PATCH
        };
        DrawTextureNPatch(borderTexture, nPatch, rect, (Vector2){0, 0}, 0.0f, color);
    }
    else
    {
        DrawRectangleLinesEx(rect, thickness, color);
        float cornerLen = 12.0f;
        DrawRectangle(rect.x - 2, rect.y - 2, cornerLen, 4, color);
        DrawRectangle(rect.x - 2, rect.y - 2, 4, cornerLen, color);
        DrawRectangle(rect.x + rect.width - cornerLen + 2, rect.y - 2, cornerLen, 4, color);
        DrawRectangle(rect.x + rect.width - 2, rect.y - 2, 4, cornerLen, color);
        DrawRectangle(rect.x - 2, rect.y + rect.height - 2, cornerLen, 4, color);
        DrawRectangle(rect.x - 2, rect.y + rect.height - cornerLen + 2, 4, cornerLen, color);
        DrawRectangle(rect.x + rect.width - cornerLen + 2, rect.y + rect.height - 2, cornerLen, 4, color);
        DrawRectangle(rect.x + rect.width - 2, rect.y + rect.height - cornerLen + 2, 4, cornerLen, color);
    }
}

bool UISystem::DrawButton(Rectangle rect, const char* text, bool active)
{
    Vector2 mousePos = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color bgColor = active ? (Color){80, 50, 20, 240} :
                    (hovered ? (Color){60, 42, 25, 230} : (Color){35, 26, 40, 220});
    Color borderColor = hovered ? (Color){245, 200, 80, 255} : (Color){160, 120, 50, 220};
    Color textColor = hovered ? (Color){255, 230, 140, 255} : (Color){220, 210, 190, 255};

    if (buttonTexture.id > 0)
    {
        NPatchInfo nPatch = {
            { 0.0f, 0.0f, (float)buttonTexture.width, (float)buttonTexture.height },
            16, 16, 16, 16,
            NPATCH_NINE_PATCH
        };
        DrawTextureNPatch(buttonTexture, nPatch, rect, (Vector2){0, 0}, 0.0f, bgColor);
    }
    else
    {
        DrawRectangleRounded(rect, 0.15f, 4, bgColor);
    }

    DrawBorder(rect, borderColor, hovered ? 2.5f : 1.5f);

    int fontSize = 18;
    int textWidth = MeasureText(text, fontSize);
    int posX = rect.x + (rect.width - textWidth) / 2;
    int posY = rect.y + (rect.height - fontSize) / 2;

    DrawText(text, posX, posY, fontSize, textColor);

    return clicked;
}

bool UISystem::DrawTabButton(Rectangle rect, const char* text, bool selected)
{
    Vector2 mousePos = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color bgColor = selected ? (Color){70, 50, 30, 250} :
                    (hovered ? (Color){45, 35, 50, 220} : (Color){25, 20, 32, 200});
    Color borderColor = selected ? (Color){230, 180, 60, 255} : (Color){100, 80, 50, 180};
    Color textColor = selected ? (Color){255, 220, 100, 255} : (Color){170, 160, 150, 255};

    DrawRectangleRounded(rect, 0.2f, 4, bgColor);
    DrawBorder(rect, borderColor, selected ? 2.0f : 1.0f);

    int fontSize = 18;
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, rect.x + (rect.width - textWidth) / 2, rect.y + (rect.height - fontSize) / 2, fontSize, textColor);

    return clicked;
}

bool UISystem::DrawSlider(Rectangle rect, const char* label, float& value, float minVal, float maxVal)
{
    Vector2 mousePos = GetMousePosition();
    Rectangle sliderBar = { rect.x + 180, rect.y + rect.height / 2 - 4, rect.width - 240, 8 };
    Rectangle handle = {
        sliderBar.x + ((value - minVal) / (maxVal - minVal)) * sliderBar.width - 8,
        sliderBar.y - 6,
        16,
        20
    };

    bool hovered = CheckCollisionPointRec(mousePos, sliderBar) || CheckCollisionPointRec(mousePos, handle);
    bool changed = false;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (hovered || CheckCollisionPointRec(mousePos, rect)))
    {
        float norm = (mousePos.x - sliderBar.x) / sliderBar.width;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        float newValue = minVal + norm * (maxVal - minVal);
        if (newValue != value)
        {
            value = newValue;
            changed = true;
        }
    }

    DrawText(label, rect.x + 10, rect.y + (rect.height - 18) / 2, 18, (Color){220, 210, 190, 255});
    DrawRectangleRounded(sliderBar, 0.5f, 4, (Color){15, 12, 20, 255});
    DrawRectangleLinesEx(sliderBar, 1.0f, (Color){140, 110, 50, 255});

    Color handleColor = hovered ? (Color){245, 200, 70, 255} : (Color){190, 145, 50, 255};
    DrawRectangleRounded(handle, 0.2f, 4, handleColor);
    DrawRectangleLinesEx(handle, 1.0f, (Color){255, 240, 180, 255});

    char valText[32];
    snprintf(valText, sizeof(valText), "%.2f", value);
    DrawText(valText, rect.x + rect.width - 50, rect.y + (rect.height - 16) / 2, 16, (Color){240, 220, 160, 255});

    return changed;
}

bool UISystem::DrawToggle(Rectangle rect, const char* label, bool& value)
{
    Vector2 mousePos = GetMousePosition();
    Rectangle toggleBox = { rect.x + rect.width - 50, rect.y + (rect.height - 24) / 2, 40, 24 };
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked)
    {
        value = !value;
    }

    DrawText(label, rect.x + 10, rect.y + (rect.height - 18) / 2, 18, (Color){220, 210, 190, 255});

    Color boxBg = value ? (Color){70, 130, 50, 240} : (Color){40, 30, 40, 240};
    DrawRectangleRounded(toggleBox, 0.4f, 4, boxBg);
    DrawRectangleLinesEx(toggleBox, 1.5f, hovered ? (Color){240, 200, 80, 255} : (Color){140, 110, 50, 255});

    Rectangle knob = value ? Rectangle{ toggleBox.x + 20, toggleBox.y + 2, 18, 20 }
                           : Rectangle{ toggleBox.x + 2, toggleBox.y + 2, 18, 20 };
    DrawRectangleRounded(knob, 0.3f, 4, (Color){230, 220, 200, 255});

    return clicked;
}

bool UISystem::DrawOptionSelector(Rectangle rect, const char* label, const char* currentOption)
{
    Vector2 mousePos = GetMousePosition();
    Rectangle btnRect = { rect.x + 180, rect.y + (rect.height - 30) / 2, rect.width - 190, 30 };
    bool hovered = CheckCollisionPointRec(mousePos, btnRect);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    DrawText(label, rect.x + 10, rect.y + (rect.height - 18) / 2, 18, (Color){220, 210, 190, 255});

    Color btnBg = hovered ? (Color){60, 45, 30, 230} : (Color){35, 25, 40, 220};
    DrawRectangleRounded(btnRect, 0.2f, 4, btnBg);
    DrawBorder(btnRect, hovered ? (Color){240, 200, 80, 255} : (Color){140, 110, 50, 220}, 1.5f);

    int textW = MeasureText(currentOption, 16);
    DrawText(currentOption, btnRect.x + (btnRect.width - textW) / 2, btnRect.y + (btnRect.height - 16) / 2, 16, (Color){255, 230, 150, 255});

    return clicked;
}

void UISystem::DrawDivider(float posX, float posY, float width)
{
    if (dividerTexture.id > 0)
    {
        DrawTexturePro(dividerTexture,
                       (Rectangle){ 0, 0, (float)dividerTexture.width, (float)dividerTexture.height },
                       (Rectangle){ posX, posY, width, 8.0f },
                       (Vector2){ 0, 0 }, 0.0f, (Color){200, 160, 80, 220});
    }
    else
    {
        DrawLineEx((Vector2){ posX, posY }, (Vector2){ posX + width, posY }, 2.0f, (Color){140, 105, 45, 200});
        DrawCircleV((Vector2){ posX + width / 2, posY }, 4.0f, (Color){220, 180, 70, 255});
    }
}

void UISystem::DrawHeader(Rectangle rect, const char* title)
{
    DrawPanel(rect, (Color){30, 22, 38, 250});
    DrawBorder(rect, (Color){220, 175, 70, 255}, 2.5f);

    int fontSize = 24;
    int titleW = MeasureText(title, fontSize);
    DrawText(title, rect.x + (rect.width - titleW) / 2, rect.y + (rect.height - fontSize) / 2, fontSize, (Color){255, 225, 120, 255});
}

void UISystem::DrawHUD(const PlayerObject& player, const char* promptText)
{
    // Painel Principal do HUD (Canto Superior Esquerdo)
    Rectangle hudPanel = { 20.0f, 20.0f, 340.0f, 130.0f };
    DrawPanel(hudPanel, (Color){20, 16, 28, 230});

    // Badge do Nível do Jogador
    Rectangle lvlBadge = { hudPanel.x + 15, hudPanel.y + 15, 80, 32 };
    DrawRectangleRounded(lvlBadge, 0.3f, 4, (Color){60, 42, 18, 250});
    DrawBorder(lvlBadge, (Color){245, 200, 80, 255}, 1.5f);

    char lvlStr[32];
    snprintf(lvlStr, sizeof(lvlStr), "NIVEL %d", player.level);
    int lvlW = MeasureText(lvlStr, 14);
    DrawText(lvlStr, lvlBadge.x + (lvlBadge.width - lvlW) / 2, lvlBadge.y + 9, 14, (Color){255, 230, 120, 255});

    // Barra de Vida (HP)
    Rectangle hpBarBg = { hudPanel.x + 105, hudPanel.y + 15, 220, 20 };
    float hpPercent = Clamp(player.health / player.maxHealth, 0.0f, 1.0f);
    Rectangle hpBarFill = { hpBarBg.x + 2, hpBarBg.y + 2, (hpBarBg.width - 4) * hpPercent, hpBarBg.height - 4 };

    DrawRectangleRounded(hpBarBg, 0.3f, 4, (Color){40, 12, 12, 240});
    DrawRectangleRounded(hpBarFill, 0.3f, 4, (Color){210, 40, 40, 255});
    DrawRectangleLinesEx(hpBarBg, 1.5f, (Color){180, 120, 50, 240});

    char hpStr[32];
    snprintf(hpStr, sizeof(hpStr), "HP: %.0f / %.0f", player.health, player.maxHealth);
    int hpW = MeasureText(hpStr, 12);
    DrawText(hpStr, hpBarBg.x + (hpBarBg.width - hpW) / 2, hpBarBg.y + 4, 12, WHITE);

    // Barra de Experiência (XP)
    Rectangle xpBarBg = { hudPanel.x + 105, hudPanel.y + 42, 220, 16 };
    float xpPercent = Clamp(player.xp / player.maxXp, 0.0f, 1.0f);
    Rectangle xpBarFill = { xpBarBg.x + 2, xpBarBg.y + 2, (xpBarBg.width - 4) * xpPercent, xpBarBg.height - 4 };

    DrawRectangleRounded(xpBarBg, 0.3f, 4, (Color){20, 15, 35, 240});
    DrawRectangleRounded(xpBarFill, 0.3f, 4, (Color){160, 60, 220, 255});
    DrawRectangleLinesEx(xpBarBg, 1.5f, (Color){140, 100, 180, 240});

    char xpStr[32];
    snprintf(xpStr, sizeof(xpStr), "XP: %.0f / %.0f", player.xp, player.maxXp);
    int xpW = MeasureText(xpStr, 11);
    DrawText(xpStr, xpBarBg.x + (xpBarBg.width - xpW) / 2, xpBarBg.y + 2, 11, (Color){240, 220, 255, 255});

    // Exibição de Stats de Combate (Ataque / Defesa)
    char statsStr[64];
    snprintf(statsStr, sizeof(statsStr), "Ataque: %.0f   Defesa: %.0f", player.GetTotalAttack(), player.GetTotalDefense());
    DrawText(statsStr, hudPanel.x + 15, hudPanel.y + 70, 14, (Color){220, 210, 180, 255});

    // Dica de Controles Rápida
    DrawText("[I] Inventario | [Mouse 1] Atacar | [WASD] Mover", hudPanel.x + 15, hudPanel.y + 100, 12, (Color){160, 150, 140, 220});

    // Prompt de Interação no Centro da Tela quando próximo de Pickups
    if (promptText && promptText[0] != '\0')
    {
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        Rectangle promptBox = { sw / 2.0f - 210.0f, sh - 100.0f, 420.0f, 44.0f };

        DrawPanel(promptBox, (Color){25, 20, 15, 240});
        DrawBorder(promptBox, (Color){255, 215, 80, 255}, 2.0f);

        int textW = MeasureText(promptText, 18);
        DrawText(promptText, promptBox.x + (promptBox.width - textW) / 2.0f, promptBox.y + 13.0f, 18, (Color){255, 235, 130, 255});
    }
}

void UISystem::DrawInventory(PlayerObject& player, bool& isOpen, int& outDroppedSlot)
{
    outDroppedSlot = -1;
    if (!isOpen) return;

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    Rectangle invRect = { sw / 2.0f - 320.0f, sh / 2.0f - 240.0f, 640.0f, 480.0f };

    // Fundo escurecido atrás do inventário
    DrawRectangle(0, 0, (int)sw, (int)sh, (Color){ 0, 0, 0, 160 });

    // Janela Principal
    DrawPanel(invRect, (Color){22, 18, 28, 250});
    DrawHeader((Rectangle){ invRect.x, invRect.y, invRect.width, 45.0f }, "INVENTARIO DE EQUIPAMENTOS");

    // Botão Fechar [X]
    Rectangle closeBtn = { invRect.x + invRect.width - 40, invRect.y + 8, 30, 30 };
    if (DrawButton(closeBtn, "X"))
    {
        isOpen = false;
    }

    // Painel Esquerdo: Equipamentos Equipados
    float leftX = invRect.x + 25.0f;
    float topY = invRect.y + 65.0f;

    DrawText("EQUIPADOS", leftX, topY, 16, (Color){245, 200, 80, 255});

    // Slot Arma
    Rectangle weaponSlot = { leftX, topY + 25.0f, 70.0f, 70.0f };
    DrawPanel(weaponSlot, (Color){35, 25, 40, 240});
    DrawBorder(weaponSlot, player.inventory.equippedWeapon.IsValid() ? (Color){245, 180, 50, 255} : (Color){100, 90, 80, 200}, 2.0f);
    DrawText("Arma", weaponSlot.x + 18, weaponSlot.y + 75, 12, (Color){180, 170, 160, 255});

    if (player.inventory.equippedWeapon.IsValid())
    {
        Texture2D icon = GetIcon(player.inventory.equippedWeapon.iconFileName);
        if (icon.id > 0)
        {
            DrawTexturePro(icon, (Rectangle){0, 0, (float)icon.width, (float)icon.height}, (Rectangle){weaponSlot.x + 5, weaponSlot.y + 5, 60, 60}, (Vector2){0,0}, 0.0f, WHITE);
        }
        else
        {
            DrawText(player.inventory.equippedWeapon.name.c_str(), weaponSlot.x + 5, weaponSlot.y + 25, 10, GOLD);
        }

        if (CheckCollisionPointRec(GetMousePosition(), weaponSlot) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            player.inventory.UnequipWeapon();
        }
    }

    // Slot Armadura
    Rectangle armorSlot = { leftX + 90.0f, topY + 25.0f, 70.0f, 70.0f };
    DrawPanel(armorSlot, (Color){35, 25, 40, 240});
    DrawBorder(armorSlot, player.inventory.equippedArmor.IsValid() ? (Color){245, 180, 50, 255} : (Color){100, 90, 80, 200}, 2.0f);
    DrawText("Armadura", armorSlot.x + 10, armorSlot.y + 75, 12, (Color){180, 170, 160, 255});

    if (player.inventory.equippedArmor.IsValid())
    {
        Texture2D icon = GetIcon(player.inventory.equippedArmor.iconFileName);
        if (icon.id > 0)
        {
            DrawTexturePro(icon, (Rectangle){0, 0, (float)icon.width, (float)icon.height}, (Rectangle){armorSlot.x + 5, armorSlot.y + 5, 60, 60}, (Vector2){0,0}, 0.0f, WHITE);
        }
        else
        {
            DrawText(player.inventory.equippedArmor.name.c_str(), armorSlot.x + 5, armorSlot.y + 25, 10, SKYBLUE);
        }

        if (CheckCollisionPointRec(GetMousePosition(), armorSlot) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            player.inventory.UnequipArmor();
        }
    }

    // Status Totais
    DrawText(TextFormat("Ataque Total: %.0f", player.GetTotalAttack()), leftX, topY + 125.0f, 15, (Color){230, 210, 160, 255});
    DrawText(TextFormat("Defesa Total: %.0f", player.GetTotalDefense()), leftX, topY + 145.0f, 15, (Color){230, 210, 160, 255});

    DrawDivider(leftX, topY + 175.0f, 180.0f);

    // Painel Direito: Grade de Inventário (4x4 = 16 Slots)
    float gridX = invRect.x + 230.0f;
    float gridY = invRect.y + 65.0f;
    float slotSize = 64.0f;
    float gap = 12.0f;

    DrawText("MOCHILA (16 SLOTS)", gridX, gridY, 16, (Color){245, 200, 80, 255});

    Vector2 mousePos = GetMousePosition();

    for (int i = 0; i < Inventory::CAPACITY; i++)
    {
        int col = i % 4;
        int row = i / 4;
        Rectangle slotRec = { gridX + col * (slotSize + gap), gridY + 25.0f + row * (slotSize + gap), slotSize, slotSize };

        bool hovered = CheckCollisionPointRec(mousePos, slotRec);
        bool isSelected = (selectedSlot == i);

        Color slotBg = isSelected ? (Color){80, 60, 30, 250} : (hovered ? (Color){45, 35, 55, 240} : (Color){28, 22, 34, 230});
        Color slotBorderColor = isSelected ? (Color){255, 220, 90, 255} : (hovered ? (Color){220, 170, 70, 255} : (Color){100, 85, 70, 200});

        DrawPanel(slotRec, slotBg);
        DrawBorder(slotRec, slotBorderColor, isSelected ? 2.5f : 1.5f);

        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            selectedSlot = i;
        }

        const Item& slotItem = player.inventory.slots[i];
        if (slotItem.IsValid())
        {
            Texture2D icon = GetIcon(slotItem.iconFileName);
            if (icon.id > 0)
            {
                DrawTexturePro(icon, (Rectangle){0, 0, (float)icon.width, (float)icon.height}, (Rectangle){slotRec.x + 4, slotRec.y + 4, 56, 56}, (Vector2){0,0}, 0.0f, WHITE);
            }
            else
            {
                DrawText(slotItem.name.c_str(), slotRec.x + 4, slotRec.y + 22, 10, WHITE);
            }
        }
    }

    // Painel Inferior de Detalhes e Ações do Item Selecionado
    float detailsY = invRect.y + 365.0f;
    Rectangle detailsBox = { invRect.x + 20.0f, detailsY, invRect.width - 40.0f, 95.0f };
    DrawPanel(detailsBox, (Color){18, 14, 24, 240});

    if (selectedSlot >= 0 && selectedSlot < Inventory::CAPACITY && player.inventory.slots[selectedSlot].IsValid())
    {
        Item& item = player.inventory.slots[selectedSlot];

        DrawText(item.name.c_str(), detailsBox.x + 15, detailsBox.y + 12, 18, (Color){255, 220, 100, 255});
        DrawText(item.description.c_str(), detailsBox.x + 15, detailsBox.y + 36, 14, (Color){210, 200, 190, 255});

        // Botões de Ação
        if (item.type == ItemType::Weapon || item.type == ItemType::Armor)
        {
            Rectangle equipBtn = { detailsBox.x + detailsBox.width - 240, detailsBox.y + 18, 105, 34 };
            if (DrawButton(equipBtn, "Equipar"))
            {
                player.inventory.EquipItem(selectedSlot);
            }
        }
        else if (item.type == ItemType::Potion)
        {
            Rectangle useBtn = { detailsBox.x + detailsBox.width - 240, detailsBox.y + 18, 105, 34 };
            if (DrawButton(useBtn, "Usar"))
            {
                float heal = 0.0f;
                if (player.inventory.UseItem(selectedSlot, heal))
                {
                    player.Heal(heal);
                }
            }
        }

        Rectangle dropBtn = { detailsBox.x + detailsBox.width - 120, detailsBox.y + 18, 105, 34 };
        if (DrawButton(dropBtn, "Dropar"))
        {
            outDroppedSlot = selectedSlot;
        }
    }
    else
    {
        DrawText("Selecione um item da mochila para ver detalhes e acoes.", detailsBox.x + 15, detailsBox.y + 35, 14, (Color){160, 150, 140, 220});
    }
}

void UISystem::DrawEnemyOverheadUI(const EnemyObject& enemy, Camera3D camera, Vector3 playerPos)
{
    if (enemy.isMarkedForCleanup || enemy.health <= 0.0f) return;

    float distToPlayer = Vector3Distance(playerPos, enemy.position);
    bool isNear = (distToPlayer <= 12.0f);
    bool isDamaged = (enemy.health < enemy.maxHealth);

    if (!isNear && !isDamaged) return;

    float headOffset = (enemy.category == "Big") ? 2.3f : ((enemy.category == "Flying") ? 2.5f : 1.6f);
    Vector3 headWorldPos = { enemy.position.x, enemy.position.y + headOffset, enemy.position.z };

    Vector2 screenPos = GetWorldToScreen(headWorldPos, camera);

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    if (screenPos.x < 10.0f || screenPos.x > sw - 10.0f || screenPos.y < 10.0f || screenPos.y > sh - 10.0f)
        return;

    char labelText[64];
    snprintf(labelText, sizeof(labelText), "%s [Niv. %d]", enemy.name.c_str(), enemy.level);

    float boxW = 140.0f;
    float boxH = 34.0f;
    float boxX = screenPos.x - boxW / 2.0f;
    float boxY = screenPos.y - boxH / 2.0f;

    DrawPanel((Rectangle){ boxX, boxY, boxW, boxH }, (Color){ 20, 16, 28, 220 });
    DrawBorder((Rectangle){ boxX, boxY, boxW, boxH }, (Color){ 180, 140, 60, 255 }, 1.5f);

    int textW = MeasureText(labelText, 10);
    DrawText(labelText, (int)(screenPos.x - textW / 2.0f), (int)(boxY + 3.0f), 10, GOLD);

    float hpPct = Clamp(enemy.health / enemy.maxHealth, 0.0f, 1.0f);
    float barW = 120.0f;
    float barH = 10.0f;
    float barX = screenPos.x - barW / 2.0f;
    float barY = boxY + 18.0f;

    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, (Color){ 40, 20, 20, 255 });

    Color hpColor = (hpPct > 0.5f) ? (Color){ 60, 200, 80, 255 }
                  : ((hpPct > 0.25f) ? (Color){ 230, 180, 40, 255 } : (Color){ 220, 50, 50, 255 });

    DrawRectangle((int)barX, (int)barY, (int)(barW * hpPct), (int)barH, hpColor);
    DrawRectangleLines((int)barX, (int)barY, (int)barW, (int)barH, (Color){ 100, 100, 100, 180 });
}

