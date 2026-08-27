#include "rlImGui.h"
#include "rlgl.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

static const char* GetClipboardTextCallback(void* userData)
{
    (void)userData;
    return GetClipboardText();
}

static void SetClipboardTextCallback(void* userData, const char* text)
{
    (void)userData;
    SetClipboardText(text);
}

namespace rlImGui
{

static Texture2D g_FontTexture = { 0 };

void Setup(bool loadDarkTheme)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = "rlImGui";
    io.BackendRendererName = "rlImGui_rlgl";

    io.GetClipboardTextFn = GetClipboardTextCallback;
    io.SetClipboardTextFn = SetClipboardTextCallback;
    io.ClipboardUserData = nullptr;

    // Carrega textura de fonte do ImGui
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    Image fontImage = {
        .data = pixels,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    g_FontTexture = LoadTextureFromImage(fontImage);
    io.Fonts->SetTexID((ImTextureID)&g_FontTexture);

    if (loadDarkTheme)
    {
        ImGui::StyleColorsDark();
    }
}

static void UpdateKeyEvents(ImGuiIO& io)
{
    struct KeyPair { int rayKey; ImGuiKey imguiKey; };
    static const KeyPair keyMap[] = {
        { KEY_TAB, ImGuiKey_Tab },
        { KEY_LEFT, ImGuiKey_LeftArrow },
        { KEY_RIGHT, ImGuiKey_RightArrow },
        { KEY_UP, ImGuiKey_UpArrow },
        { KEY_DOWN, ImGuiKey_DownArrow },
        { KEY_PAGE_UP, ImGuiKey_PageUp },
        { KEY_PAGE_DOWN, ImGuiKey_PageDown },
        { KEY_HOME, ImGuiKey_Home },
        { KEY_END, ImGuiKey_End },
        { KEY_INSERT, ImGuiKey_Insert },
        { KEY_DELETE, ImGuiKey_Delete },
        { KEY_BACKSPACE, ImGuiKey_Backspace },
        { KEY_SPACE, ImGuiKey_Space },
        { KEY_ENTER, ImGuiKey_Enter },
        { KEY_ESCAPE, ImGuiKey_Escape },
        { KEY_KP_ENTER, ImGuiKey_KeypadEnter },
        { KEY_A, ImGuiKey_A }, { KEY_B, ImGuiKey_B }, { KEY_C, ImGuiKey_C },
        { KEY_D, ImGuiKey_D }, { KEY_E, ImGuiKey_E }, { KEY_F, ImGuiKey_F },
        { KEY_G, ImGuiKey_G }, { KEY_H, ImGuiKey_H }, { KEY_I, ImGuiKey_I },
        { KEY_J, ImGuiKey_J }, { KEY_K, ImGuiKey_K }, { KEY_L, ImGuiKey_L },
        { KEY_M, ImGuiKey_M }, { KEY_N, ImGuiKey_N }, { KEY_O, ImGuiKey_O },
        { KEY_P, ImGuiKey_P }, { KEY_Q, ImGuiKey_Q }, { KEY_R, ImGuiKey_R },
        { KEY_S, ImGuiKey_S }, { KEY_T, ImGuiKey_T }, { KEY_U, ImGuiKey_U },
        { KEY_V, ImGuiKey_V }, { KEY_W, ImGuiKey_W }, { KEY_X, ImGuiKey_X },
        { KEY_Y, ImGuiKey_Y }, { KEY_Z, ImGuiKey_Z }
    };

    for (const auto& pair : keyMap)
    {
        io.AddKeyEvent(pair.imguiKey, IsKeyDown(pair.rayKey));
    }
}

void Begin()
{
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = (ImVec2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
    io.DeltaTime = GetFrameTime();

    // Mouse position & buttons
    Vector2 mousePos = GetMousePosition();
    io.AddMousePosEvent(mousePos.x, mousePos.y);

    io.AddMouseButtonEvent(0, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    io.AddMouseButtonEvent(1, IsMouseButtonDown(MOUSE_BUTTON_RIGHT));
    io.AddMouseButtonEvent(2, IsMouseButtonDown(MOUSE_BUTTON_MIDDLE));

    float wheel = GetMouseWheelMove();
    io.AddMouseWheelEvent(0.0f, wheel);

    // Modifiers & Keys
    io.AddKeyEvent(ImGuiMod_Ctrl, IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));
    io.AddKeyEvent(ImGuiMod_Shift, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
    io.AddKeyEvent(ImGuiMod_Super, IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER));

    UpdateKeyEvents(io);

    // Character input
    int c = GetCharPressed();
    while (c > 0)
    {
        io.AddInputCharacter(c);
        c = GetCharPressed();
    }

    ImGui::NewFrame();
}

static void RenderTriangles(int count, const ImDrawVert* vertices, const ImDrawIdx* indices, Texture2D* texture)
{
    if (count <= 0) return;

    rlSetTexture(texture ? texture->id : 0);
    rlBegin(RL_TRIANGLES);

    for (int i = 0; i < count; i++)
    {
        ImDrawIdx index = indices[i];
        const ImDrawVert& v = vertices[index];

        Color color = GetColor(v.col);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(v.uv.x, v.uv.y);
        rlVertex2f(v.pos.x, v.pos.y);
    }

    rlEnd();
    rlSetTexture(0);
}

void End()
{
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->CmdListsCount == 0) return;

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlDisableDepthTest();
    rlEnableScissorTest();

    // Configura Matriz de Projeção Ortopédica 2D para as coordenadas de tela do ImGui
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    rlOrtho(0.0, (double)drawData->DisplaySize.x, (double)drawData->DisplaySize.y, 0.0, -1.0, 1.0);
    rlMatrixMode(RL_MODELVIEW);
    rlPushMatrix();
    rlLoadIdentity();

    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        const ImDrawVert* vertBase = cmdList->VtxBuffer.Data;
        const ImDrawIdx* idxBase = cmdList->IdxBuffer.Data;

        for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
        {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdIdx];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmdList, pcmd);
            }
            else
            {
                ImVec2 pos = drawData->DisplayPos;
                int scissorX = (int)(pcmd->ClipRect.x - pos.x);
                int scissorY = (int)(pcmd->ClipRect.y - pos.y);
                int scissorW = (int)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                int scissorH = (int)(pcmd->ClipRect.w - pcmd->ClipRect.y);

                rlScissor(scissorX, GetScreenHeight() - (scissorY + scissorH), scissorW, scissorH);

                Texture2D* tex = (Texture2D*)pcmd->GetTexID();
                RenderTriangles(pcmd->ElemCount, vertBase, idxBase + pcmd->IdxOffset, tex);
            }
        }
    }

    rlDrawRenderBatchActive();

    // Restaura matrizes de Projeção e Modelview
    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW);
    rlPopMatrix();

    rlDisableScissorTest();
    rlEnableDepthTest();
    rlEnableBackfaceCulling();
}

void Shutdown()
{
    if (g_FontTexture.id > 0)
    {
        UnloadTexture(g_FontTexture);
        g_FontTexture = (Texture2D){ 0 };
    }
    ImGui::DestroyContext();
}

void ImageRenderTexture(const RenderTexture2D* image)
{
    if (image)
        ImGui::Image((ImTextureID)&image->texture, (ImVec2){ (float)image->texture.width, (float)image->texture.height }, (ImVec2){ 0, 1 }, (ImVec2){ 1, 0 });
}

void ImageTexture(const Texture2D* image)
{
    if (image)
        ImGui::Image((ImTextureID)image, (ImVec2){ (float)image->width, (float)image->height });
}

} // namespace rlImGui

