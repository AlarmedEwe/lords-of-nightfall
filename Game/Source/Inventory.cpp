#include "Inventory.h"

bool Inventory::AddItem(const Item& item)
{
    for (int i = 0; i < CAPACITY; i++)
    {
        if (!slots[i].IsValid())
        {
            slots[i] = item;
            return true;
        }
    }
    return false; // Inventário cheio
}

bool Inventory::RemoveItem(int index)
{
    if (index >= 0 && index < CAPACITY && slots[index].IsValid())
    {
        slots[index] = Item();
        return true;
    }
    return false;
}

bool Inventory::EquipItem(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= CAPACITY || !slots[slotIndex].IsValid())
        return false;

    Item item = slots[slotIndex];
    if (item.type == ItemType::Weapon)
    {
        Item oldWeapon = equippedWeapon;
        equippedWeapon = item;
        slots[slotIndex] = oldWeapon;
        return true;
    }
    else if (item.type == ItemType::Armor)
    {
        Item oldArmor = equippedArmor;
        equippedArmor = item;
        slots[slotIndex] = oldArmor;
        return true;
    }

    return false;
}

bool Inventory::UnequipWeapon()
{
    if (equippedWeapon.IsValid() && AddItem(equippedWeapon))
    {
        equippedWeapon = Item();
        return true;
    }
    return false;
}

bool Inventory::UnequipArmor()
{
    if (equippedArmor.IsValid() && AddItem(equippedArmor))
    {
        equippedArmor = Item();
        return true;
    }
    return false;
}

bool Inventory::UseItem(int slotIndex, float& outHeal)
{
    outHeal = 0.0f;
    if (slotIndex < 0 || slotIndex >= CAPACITY || !slots[slotIndex].IsValid())
        return false;

    Item item = slots[slotIndex];
    if (item.type == ItemType::Potion)
    {
        outHeal = item.healAmount;
        slots[slotIndex] = Item();
        return true;
    }

    return false;
}

void Inventory::SwapSlots(int slotA, int slotB)
{
    if (slotA >= 0 && slotA < CAPACITY && slotB >= 0 && slotB < CAPACITY)
    {
        Item temp = slots[slotA];
        slots[slotA] = slots[slotB];
        slots[slotB] = temp;
    }
}

float Inventory::GetTotalAttackBonus() const
{
    return equippedWeapon.IsValid() ? equippedWeapon.attackBonus : 0.0f;
}

float Inventory::GetTotalDefenseBonus() const
{
    return equippedArmor.IsValid() ? equippedArmor.defenseBonus : 0.0f;
}
