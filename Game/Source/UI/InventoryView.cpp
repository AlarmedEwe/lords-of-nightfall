#include "InventoryView.h"
#include "UIWidgets.h"
#include "UISystem.h"
#include <cstdio>

namespace InventoryView
{

void Draw(PlayerObject& player, bool& isOpen, int& outDroppedSlot, UISystem& ui)
{
    outDroppedSlot = -1;
    if (!isOpen) return;

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    Rectangle invRect = { sw / 2.0f - 320.0f, sh / 2.0f - 240.0f, 640.0f, 480.0f };

    // Fundo escurecido atrás do inventário
    DrawRectangle(0, 0, (int)sw, (int)sh, (Color){ 0, 0, 0, 160 });

    // Janela Principal
    UIWidgets::DrawPanel(ui, invRect, (Color){22, 18, 28, 250});
    UIWidgets::DrawHeader(ui, (Rectangle){ invRect.x, invRect.y, invRect.width, 45.0f }, "INVENTARIO DE EQUIPAMENTOS");

    // Botão Fechar [X]
    Rectangle closeBtn = { invRect.x + invRect.width - 40, invRect.y + 8, 30, 30 };
    if (UIWidgets::DrawButton(ui, closeBtn, "X"))
    {
        isOpen = false;
    }

    // Painel Esquerdo: Equipamentos Equipados
    float leftX = invRect.x + 25.0f;
    float topY = invRect.y + 65.0f;

    DrawText("EQUIPADOS", leftX, topY, 16, (Color){245, 200, 80, 255});

    // Slot Arma
    Rectangle weaponSlot = { leftX, topY + 25.0f, 70.0f, 70.0f };
    UIWidgets::DrawPanel(ui, weaponSlot, (Color){35, 25, 40, 240});
    UIWidgets::DrawBorder(ui, weaponSlot, player.inventory.equippedWeapon.IsValid() ? (Color){245, 180, 50, 255} : (Color){100, 90, 80, 200}, 2.0f);
    DrawText("Arma", weaponSlot.x + 18, weaponSlot.y + 75, 12, (Color){180, 170, 160, 255});

    if (player.inventory.equippedWeapon.IsValid())
    {
        Texture2D icon = ui.GetIcon(player.inventory.equippedWeapon.iconFileName);
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
    UIWidgets::DrawPanel(ui, armorSlot, (Color){35, 25, 40, 240});
    UIWidgets::DrawBorder(ui, armorSlot, player.inventory.equippedArmor.IsValid() ? (Color){245, 180, 50, 255} : (Color){100, 90, 80, 200}, 2.0f);
    DrawText("Armadura", armorSlot.x + 10, armorSlot.y + 75, 12, (Color){180, 170, 160, 255});

    if (player.inventory.equippedArmor.IsValid())
    {
        Texture2D icon = ui.GetIcon(player.inventory.equippedArmor.iconFileName);
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

    UIWidgets::DrawDivider(ui, leftX, topY + 175.0f, 180.0f);

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
        bool isSelected = (ui.selectedSlot == i);

        Color slotBg = isSelected ? (Color){80, 60, 30, 250} : (hovered ? (Color){45, 35, 55, 240} : (Color){28, 22, 34, 230});
        Color slotBorderColor = isSelected ? (Color){255, 220, 90, 255} : (hovered ? (Color){220, 170, 70, 255} : (Color){100, 85, 70, 200});

        UIWidgets::DrawPanel(ui, slotRec, slotBg);
        UIWidgets::DrawBorder(ui, slotRec, slotBorderColor, isSelected ? 2.5f : 1.5f);

        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            ui.selectedSlot = i;
        }

        const Item& slotItem = player.inventory.slots[i];
        if (slotItem.IsValid())
        {
            Texture2D icon = ui.GetIcon(slotItem.iconFileName);
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
    UIWidgets::DrawPanel(ui, detailsBox, (Color){18, 14, 24, 240});

    if (ui.selectedSlot >= 0 && ui.selectedSlot < Inventory::CAPACITY && player.inventory.slots[ui.selectedSlot].IsValid())
    {
        Item& item = player.inventory.slots[ui.selectedSlot];

        DrawText(item.name.c_str(), detailsBox.x + 15, detailsBox.y + 12, 18, (Color){255, 220, 100, 255});
        DrawText(item.description.c_str(), detailsBox.x + 15, detailsBox.y + 36, 14, (Color){210, 200, 190, 255});

        // Botões de Ação
        if (item.type == ItemType::Weapon || item.type == ItemType::Armor)
        {
            Rectangle equipBtn = { detailsBox.x + detailsBox.width - 240, detailsBox.y + 18, 105, 34 };
            if (UIWidgets::DrawButton(ui, equipBtn, "Equipar"))
            {
                player.inventory.EquipItem(ui.selectedSlot);
            }
        }
        else if (item.type == ItemType::Potion)
        {
            Rectangle useBtn = { detailsBox.x + detailsBox.width - 240, detailsBox.y + 18, 105, 34 };
            if (UIWidgets::DrawButton(ui, useBtn, "Usar"))
            {
                float heal = 0.0f;
                if (player.inventory.UseItem(ui.selectedSlot, heal))
                {
                    player.Heal(heal);
                }
            }
        }

        Rectangle dropBtn = { detailsBox.x + detailsBox.width - 120, detailsBox.y + 18, 105, 34 };
        if (UIWidgets::DrawButton(ui, dropBtn, "Dropar"))
        {
            outDroppedSlot = ui.selectedSlot;
        }
    }
    else
    {
        DrawText("Selecione um item da mochila para ver detalhes e acoes.", detailsBox.x + 15, detailsBox.y + 35, 14, (Color){160, 150, 140, 220});
    }
}

} // namespace InventoryView
