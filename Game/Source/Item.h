#ifndef ITEM_H
#define ITEM_H

#include <string>

enum class ItemType
{
    None,
    Weapon,
    Armor,
    Potion
};

struct Item
{
    int id = 0;
    std::string name;
    std::string description;
    ItemType type = ItemType::None;
    float attackBonus = 0.0f;
    float defenseBonus = 0.0f;
    float healAmount = 0.0f;
    std::string iconFileName;
    std::string modelFileName;
    std::string outfitModelFile; // Se for armadura, modelo .lonmodel a equipar
    std::string outfitTextureFile;

    bool IsValid() const { return type != ItemType::None; }
};

namespace Items
{
    inline Item CreateSword()
    {
        Item item;
        item.id = 1;
        item.name = "Espada de Aço";
        item.description = "+20 Dano de Ataque";
        item.type = ItemType::Weapon;
        item.attackBonus = 20.0f;
        item.iconFileName = "Items/Icons/Sword.png";
        item.modelFileName = "Items/Models/Sword.obj";
        return item;
    }

    inline Item CreateAxe()
    {
        Item item;
        item.id = 2;
        item.name = "Machado de Combate";
        item.description = "+35 Dano de Ataque";
        item.type = ItemType::Weapon;
        item.attackBonus = 35.0f;
        item.iconFileName = "Items/Icons/Axe_Small.png";
        item.modelFileName = "Items/Models/Axe_small.obj";
        return item;
    }

    inline Item CreateHealthPotion()
    {
        Item item;
        item.id = 3;
        item.name = "Poção de Vida";
        item.description = "Recupera +40 de Vida (HP)";
        item.type = ItemType::Potion;
        item.healAmount = 40.0f;
        item.iconFileName = "Items/Icons/Potion1_Filled_Red.png";
        item.modelFileName = "Items/Models/Potion1_Filled.obj";
        return item;
    }

    inline Item CreateRangerArmor()
    {
        Item item;
        item.id = 4;
        item.name = "Armadura do Patrulheiro";
        item.description = "+25 Defesa. Altera visual para Ranger.";
        item.type = ItemType::Armor;
        item.defenseBonus = 25.0f;
        item.iconFileName = "Items/Icons/Armor_Leather.png";
        item.modelFileName = "Items/Models/Armor_Leather.obj";
        item.outfitModelFile = "Male_Ranger.lonmodel";
        item.outfitTextureFile = "T_Superhero_Male_Ligh.png";
        return item;
    }

    inline Item CreatePeasantOutfit()
    {
        Item item;
        item.id = 5;
        item.name = "Traje do Camponês";
        item.description = "+10 Defesa. Altera visual para Camponês.";
        item.type = ItemType::Armor;
        item.defenseBonus = 10.0f;
        item.iconFileName = "Items/Icons/Armor_Metal.png";
        item.modelFileName = "Items/Models/Armor_Metal.obj";
        item.outfitModelFile = "Male_Peasant.lonmodel";
        item.outfitTextureFile = "T_Superhero_Male_Ligh.png";
        return item;
    }
}

#endif // ITEM_H
