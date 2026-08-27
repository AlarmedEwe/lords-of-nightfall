#ifndef INVENTORY_H
#define INVENTORY_H

#include "Item.h"
#include <vector>

struct Inventory
{
    static const int CAPACITY = 16;
    Item slots[CAPACITY];

    Item equippedWeapon;
    Item equippedArmor;
    int gold = 0;

    bool AddItem(const Item& item);
    bool RemoveItem(int index);
    bool EquipItem(int slotIndex);
    bool UnequipWeapon();
    bool UnequipArmor();
    bool UseItem(int slotIndex, float& outHeal);
    void SwapSlots(int slotA, int slotB);

    float GetTotalAttackBonus() const;
    float GetTotalDefenseBonus() const;
};

#endif // INVENTORY_H
