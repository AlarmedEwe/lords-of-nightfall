#ifndef RLIMGUI_H
#define RLIMGUI_H

#include "raylib.h"
#include "imgui.h"

namespace rlImGui
{
    void Setup(bool loadDarkTheme = true);
    void Begin();
    void End();
    void Shutdown();

    void ImageRenderTexture(const RenderTexture2D* image);
    void ImageTexture(const Texture2D* image);
}

#endif // RLIMGUI_H

