#include "UISystem.h"
#include "raymath.h"
#include <cstdio>

static Texture2D TryLoadTexture(const char* relPath)
{
    const char* candidatePaths[] = {
        relPath,
        TextFormat("Assets/%s", relPath),
        TextFormat("Build/Development/Assets/%s", relPath),
        TextFormat("Game/Assets/kenney_fantasy-ui-borders/PNG/Double/%s", relPath)
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
    isLoaded = false;
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
        // Cantos fantasia decorativos
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

    int fontSize = 20;
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
