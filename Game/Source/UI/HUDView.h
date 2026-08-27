#ifndef HUD_VIEW_H
#define HUD_VIEW_H

#include "GameObjects/PlayerObject.h"

struct UISystem;

namespace HUDView
{
    void Draw(const PlayerObject& player, const char* promptText, UISystem& ui);
}

#endif // HUD_VIEW_H
