#include "HUDView.h"
#include "UIWidgets.h"
#include "UISystem.h"
#include "raymath.h"
#include <cstdio>

namespace HUDView
{

void Draw(const PlayerObject& player, const char* promptText, UISystem& ui)
{
    // Painel Principal do HUD (Canto Superior Esquerdo)
    Rectangle hudPanel = { 20.0f, 20.0f, 340.0f, 130.0f };
    UIWidgets::DrawPanel(ui, hudPanel, (Color){20, 16, 28, 230});

    // Badge do Nível do Jogador
    Rectangle lvlBadge = { hudPanel.x + 15, hudPanel.y + 15, 80, 32 };
    DrawRectangleRounded(lvlBadge, 0.3f, 4, (Color){60, 42, 18, 250});
    UIWidgets::DrawBorder(ui, lvlBadge, (Color){245, 200, 80, 255}, 1.5f);

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

        UIWidgets::DrawPanel(ui, promptBox, (Color){25, 20, 15, 240});
        UIWidgets::DrawBorder(ui, promptBox, (Color){255, 215, 80, 255}, 2.0f);

        int textW = MeasureText(promptText, 18);
        DrawText(promptText, promptBox.x + (promptBox.width - textW) / 2.0f, promptBox.y + 13.0f, 18, (Color){255, 235, 130, 255});
    }
}

} // namespace HUDView
