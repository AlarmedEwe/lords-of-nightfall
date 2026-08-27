#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "raylib.h"
#include <string>

struct UISystem;

namespace UIWidgets
{
    Texture2D TryLoadTexture(const char* relPath);

    void DrawPanel(UISystem& ui, Rectangle rect, Color tint = (Color){25, 22, 35, 240});
    void DrawBorder(UISystem& ui, Rectangle rect, Color color = (Color){195, 155, 70, 255}, float thickness = 3.0f);
    bool DrawButton(UISystem& ui, Rectangle rect, const char* text, bool active = false);
    bool DrawTabButton(Rectangle rect, const char* text, bool selected);
    bool DrawSlider(Rectangle rect, const char* label, float& value, float minVal, float maxVal);
    bool DrawToggle(Rectangle rect, const char* label, bool& value);
    bool DrawOptionSelector(UISystem& ui, Rectangle rect, const char* label, const char* currentOption);
    void DrawDivider(UISystem& ui, float posX, float posY, float width);
    void DrawHeader(UISystem& ui, Rectangle rect, const char* title);
}

#endif // UI_WIDGETS_H
