#ifndef PLAYER_EQUIPMENT_H
#define PLAYER_EQUIPMENT_H

#include "../LONModel.h"
#include "../Inventory.h"
#include <string>

struct PlayerObject;

namespace PlayerEquipment
{
    void SetOutfit(PlayerObject& player, const std::string& modelFile, const std::string& textureFile);
    void Sync(PlayerObject& player);
    void Draw(PlayerObject& player, Vector3 drawPos);
    void Unload(PlayerObject& player);
}

#endif // PLAYER_EQUIPMENT_H
