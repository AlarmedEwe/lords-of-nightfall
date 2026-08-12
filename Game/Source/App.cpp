#include "App.h"
#include "raylib.h"

void App::Init()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(screenWidth, screenHeight, "Lords of Nightfall");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Impede que a tecla ESC feche a aplicação automaticamente

    settings.Init();
    ui.Init();

    mainMenuScene.Init();

    // Inicializa a cena de gameplay
    scene.objects.push_back(&terrain);
    scene.objects.push_back(&player);
    scene.Init();

#if defined(LON_DEVELOPMENT) || defined(LON_DEBUG)
    // No modo Development e Debug, inicia direto na cena de Gameplay
    state = GameState::Gameplay;
    DisableCursor();
#else
    // No modo Release, inicia no Menu Principal com cursor habilitado
    state = GameState::MainMenu;
    EnableCursor();
#endif
}

void App::StartGameplay()
{
    state = GameState::Gameplay;
    isPaused = false;
    settingsMenu.isOpen = false;
    DisableCursor();
}

void App::ReturnToMainMenu()
{
    state = GameState::MainMenu;
    isPaused = false;
    settingsMenu.isOpen = false;
    EnableCursor();
}

void App::Run()
{
    while (!WindowShouldClose() && running)
    {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f; // Clamp para evitar instabilidade da física

        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        if (state == GameState::MainMenu)
        {
            if (IsKeyPressed(KEY_ESCAPE) && settingsMenu.isOpen)
            {
                settingsMenu.isOpen = false;
            }

            mainMenuScene.Update(dt);

            MainMenuAction action = MainMenuAction::None;

            BeginDrawing();
            ClearBackground((Color){15, 12, 22, 255});

            mainMenuScene.Draw(ui, settingsMenu, settings, screenWidth, screenHeight, action);

            EndDrawing();

            if (action == MainMenuAction::Play)
            {
                StartGameplay();
            }
            else if (action == MainMenuAction::Quit)
            {
                running = false;
            }
        }
        else if (state == GameState::Gameplay)
        {
            // Trata tecla ESC para pausar/despausar ou fechar menu de configurações
            if (IsKeyPressed(KEY_ESCAPE))
            {
                if (settingsMenu.isOpen)
                {
                    settingsMenu.isOpen = false;
                }
                else
                {
                    isPaused = !isPaused;
                    if (isPaused)
                        EnableCursor();
                    else
                        DisableCursor();
                }
            }

            // Atualiza gameplay apenas quando NÃO estiver pausado
            if (!isPaused)
            {
                player.cameraYaw = scene.cameraYaw;
                scene.Update(dt);
                scene.UpdateCamera(player.position, dt, settings.mouseSensitivity);
            }

            // Renderização
            BeginDrawing();
            ClearBackground((Color){20, 20, 30, 255});

            scene.Draw();

            // Overlay de Pausa
            if (isPaused)
            {
                // Fundo escuro cobrindo o jogo
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 170});

                if (settingsMenu.isOpen)
                {
                    settingsMenu.Draw(ui, settings, screenWidth, screenHeight);
                }
                else
                {
                    float panelW = 340.0f;
                    float panelH = 380.0f;
                    float panelX = (screenWidth - panelW) / 2.0f;
                    float panelY = (screenHeight - panelH) / 2.0f;

                    Rectangle pauseRect = { panelX, panelY, panelW, panelH };
                    ui.DrawPanel(pauseRect, (Color){22, 18, 30, 245});

                    Rectangle headerRect = { panelX + 20, panelY + 25, panelW - 40, 48 };
                    ui.DrawHeader(headerRect, "JOGO PAUSADO");

                    ui.DrawDivider(panelX + 20, panelY + 88, panelW - 40);

                    float btnY = panelY + 110.0f;
                    float btnW = panelW - 60.0f;
                    float btnH = 44.0f;
                    float btnX = panelX + 30.0f;

                    // 1. Continuar
                    Rectangle btnResume = { btnX, btnY, btnW, btnH };
                    if (ui.DrawButton(btnResume, "CONTINUAR"))
                    {
                        isPaused = false;
                        DisableCursor();
                    }

                    // 2. Configurações
                    Rectangle btnSettings = { btnX, btnY + 60.0f, btnW, btnH };
                    if (ui.DrawButton(btnSettings, "CONFIGURAÇÕES"))
                    {
                        settingsMenu.isOpen = true;
                    }

                    // 3. Voltar ao Menu
                    Rectangle btnMenu = { btnX, btnY + 120.0f, btnW, btnH };
                    if (ui.DrawButton(btnMenu, "VOLTAR AO MENU"))
                    {
                        ReturnToMainMenu();
                    }

                    // 4. Sair
                    Rectangle btnQuit = { btnX, btnY + 180.0f, btnW, btnH };
                    if (ui.DrawButton(btnQuit, "SAIR"))
                    {
                        running = false;
                    }
                }
            }
            else
            {
                // HUD Mínimo durante o jogo
                DrawFPS(10, 10);
                DrawText("WASD: Mover | Mouse: Câmera | Espaço: Pular | ESC: Pausa",
                         10, screenHeight - 30, 16, RAYWHITE);
            }

            EndDrawing();
        }
    }
}

void App::Shutdown()
{
    mainMenuScene.Shutdown();
    scene.Shutdown();
    ui.Shutdown();
    CloseWindow();
}
