#ifndef INVENTORY_VIEW_H
#define INVENTORY_VIEW_H

#include "GameObjects/PlayerObject.h"

struct UISystem;

namespace InventoryView
{
    void Draw(PlayerObject& player, bool& isOpen, int& outDroppedSlot, UISystem& ui);
}

#endif // INVENTORY_VIEW_H
