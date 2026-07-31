#include "App.h"
#include "raylib.h"

void App::Init()
{
    InitWindow(screenWidth, screenHeight, "Lords of Nightfall");
    SetTargetFPS(60);
    DisableCursor();

    scene.objects.push_back(&terrain);
    scene.objects.push_back(&player);
    scene.Init();
}

void App::Run()
{
    while (!WindowShouldClose() && running)
    {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f; // Clamp para evitar physics explosion

        // Toggle cursor com ESC alternativo (Tab)
        if (IsKeyPressed(KEY_TAB))
        {
            if (IsCursorHidden())
                EnableCursor();
            else
                DisableCursor();
        }

        // Atualiza a direção do player baseado na câmera
        player.facingYaw = scene.cameraYaw;

        scene.Update(dt);
        scene.UpdateCamera(player.position, dt);

        // Rendering
        BeginDrawing();
        ClearBackground((Color){20, 20, 30, 255});

        scene.Draw();

        // HUD mínimo
        DrawFPS(10, 10);
        DrawText("WASD: Mover | Mouse: Camera | Espaco: Pular | Tab: Cursor | Scroll: Zoom",
                 10, screenHeight - 30, 16, RAYWHITE);

        EndDrawing();
    }
}

void App::Shutdown()
{
    scene.Shutdown();
    CloseWindow();
}
