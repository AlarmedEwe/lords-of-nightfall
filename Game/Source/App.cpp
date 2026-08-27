#include "App.h"
#include "GameObjects/EnemyObject.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(LON_DEVELOPMENT) || defined(LON_DEBUG)
static void CaptureScreenshotToDiskAndClipboard()
{
    const char* homeDir = getenv("HOME");
    std::string picturesDir = homeDir ? (std::string(homeDir) + "/Pictures") : "/home/upd8-guilherme/Pictures";
    std::string screenshotDir = picturesDir + "/Screenshots";

    mkdir(picturesDir.c_str(), 0755);
    mkdir(screenshotDir.c_str(), 0755);

    time_t rawtime;
    struct tm* timeinfo;
    char timeBuffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "LON_%Y-%m-%d_%H-%M-%S.png", timeinfo);

    std::string fullPath = screenshotDir + "/" + timeBuffer;
    TakeScreenshot(fullPath.c_str());

    std::string copyCmd = "python3 -c \"import gi; gi.require_version('Gdk', '3.0'); gi.require_version('Gtk', '3.0'); from gi.repository import Gtk, Gdk, GdkPixbuf; pixbuf = GdkPixbuf.Pixbuf.new_from_file('" + fullPath + "'); clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD); clipboard.set_image(pixbuf); clipboard.store()\"";
    (void)system(copyCmd.c_str());
}
#endif

void App::Init()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(screenWidth, screenHeight, "Lords of Nightfall");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    settings.Init();
    ui.Init();

    mainMenuScene.Init();

    // Inicializa a cena de gameplay com o jogador e o terreno
    scene.objects.push_back(&terrain);
    scene.objects.push_back(&player);

    // Spawna Pickups de teste espalhados pelo terreno
    scene.objects.push_back(new PickupObject(Items::CreateSword(), (Vector3){ 4.0f, 0.0f, 4.0f }));
    scene.objects.push_back(new PickupObject(Items::CreateAxe(), (Vector3){ -4.0f, 0.0f, 3.0f }));
    scene.objects.push_back(new PickupObject(Items::CreateHealthPotion(), (Vector3){ 2.0f, 0.0f, -5.0f }));
    scene.objects.push_back(new PickupObject(Items::CreateRangerArmor(), (Vector3){ -3.0f, 0.0f, -4.0f }));
    scene.objects.push_back(new PickupObject(Items::CreatePeasantOutfit(), (Vector3){ 5.0f, 0.0f, -3.0f }));

    // Spawna centenas de inimigos aleatórios pelo terreno
    struct MonsterArchetype {
        const char* name;
        const char* category;
        const char* modelFile;
    };

    MonsterArchetype archetypes[] = {
        { "Demonio Infernal", "Big", "Monster_Big_Demon.lonmodel" },
        { "Ogro Gigante", "Big", "Monster_Big_Orc.lonmodel" },
        { "Yeti da Montanha", "Big", "Monster_Big_Yeti.lonmodel" },
        { "Demonio Azul", "Big", "Monster_Big_BlueDemon.lonmodel" },
        { "Cacto Selvagem", "Big", "Monster_Big_Cactoro.lonmodel" },
        { "Guerreiro Tribal", "Big", "Monster_Big_Tribal.lonmodel" },

        { "Blob Gororoba", "Blob", "Monster_Blob_GreenBlob.lonmodel" },
        { "Blob Espinhoso", "Blob", "Monster_Blob_GreenSpikyBlob.lonmodel" },
        { "Blob Rosa", "Blob", "Monster_Blob_PinkBlob.lonmodel" },
        { "Cogumelo Selvagem", "Blob", "Monster_Blob_Mushnub.lonmodel" },
        { "Mago da Colina", "Blob", "Monster_Blob_Wizard.lonmodel" },
        { "Gato Mutante", "Blob", "Monster_Blob_Cat.lonmodel" },

        { "Dragao Alado", "Flying", "Monster_Flying_Dragon.lonmodel" },
        { "Fantasma Sombrio", "Flying", "Monster_Flying_Ghost.lonmodel" },
        { "Gargula de Pedra", "Flying", "Monster_Flying_Goleling.lonmodel" },
        { "Abelha Gigante", "Flying", "Monster_Flying_Armabee.lonmodel" },
        { "Alpaca Voadora", "Flying", "Monster_Flying_Alpaking.lonmodel" }
    };
    int numArchetypes = sizeof(archetypes) / sizeof(archetypes[0]);

    srand((unsigned int)time(NULL));
    for (int i = 0; i < 120; i++)
    {
        int archIdx = rand() % numArchetypes;
        const auto& arch = archetypes[archIdx];

        float angle = ((float)(rand() % 360)) * DEG2RAD;
        float dist = 30.0f + (float)(rand() % 650) / 10.0f; // Área segura: inimigos aparecem a no mínimo 30m do jogador
        float spawnX = cosf(angle) * dist;
        float spawnZ = sinf(angle) * dist;
        Vector3 spawnPos = { spawnX, 0.0f, spawnZ };

        int enemyLevel = 1 + (rand() % 15);

        auto* enemy = new EnemyObject(arch.name, arch.category, arch.modelFile, enemyLevel, spawnPos);
        scene.objects.push_back(enemy);
    }

    scene.Init();

#if defined(LON_DEVELOPMENT) || defined(LON_DEBUG)
    state = GameState::Gameplay;
    DisableCursor();
#else
    state = GameState::MainMenu;
    EnableCursor();
#endif
}

void App::StartGameplay()
{
    state = GameState::Gameplay;
    isPaused = false;
    isInventoryOpen = false;
    settingsMenu.isOpen = false;
    DisableCursor();
}

void App::ReturnToMainMenu()
{
    state = GameState::MainMenu;
    isPaused = false;
    isInventoryOpen = false;
    settingsMenu.isOpen = false;
    EnableCursor();
}

void App::Run()
{
    while (running && !WindowShouldClose())
    {
        float dt = GetFrameTime();
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

#if defined(LON_DEVELOPMENT) || defined(LON_DEBUG)
        if (IsKeyPressed(KEY_P))
        {
            CaptureScreenshotToDiskAndClipboard();
        }
#endif

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
            // Toggle Inventário (I ou TAB)
            if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_TAB))
            {
                if (!isPaused)
                {
                    isInventoryOpen = !isInventoryOpen;
                    if (isInventoryOpen) EnableCursor();
                    else DisableCursor();
                }
            }

            // Tecla ESC (Pausa ou Fecha Inventário/Configurações)
            if (IsKeyPressed(KEY_ESCAPE))
            {
                if (settingsMenu.isOpen)
                {
                    settingsMenu.isOpen = false;
                }
                else if (isInventoryOpen)
                {
                    isInventoryOpen = false;
                    DisableCursor();
                }
                else
                {
                    isPaused = !isPaused;
                    if (isPaused) EnableCursor();
                    else DisableCursor();
                }
            }

            // Atualiza gameplay quando não pausado nem no inventário
            if (!isPaused && !isInventoryOpen)
            {
                player.cameraYaw = scene.cameraYaw;
                player.PerformAttackSweep(scene.objects, &scene.damageFeedback);

                for (size_t i = 0; i < scene.objects.size(); )
                {
                    auto* obj = scene.objects[i];
                    if (auto* enemy = dynamic_cast<EnemyObject*>(obj))
                    {
                        if (enemy->isMarkedForCleanup)
                        {
                            enemy->Shutdown(scene.physics);
                            delete enemy;
                            scene.objects.erase(scene.objects.begin() + i);
                            continue;
                        }
                        else
                        {
                            enemy->UpdateAI(&player, dt, &scene.damageFeedback);
                        }
                    }
                    i++;
                }

                scene.Update(dt);
                scene.UpdateCamera(player.position, dt, settings.mouseSensitivity);
            }

            // Interação com Pickups no mundo
            activePromptText = "";
            PickupObject* closestPickup = nullptr;
            float minDistance = 2.5f;

            for (auto* obj : scene.objects)
            {
                if (auto* pickup = dynamic_cast<PickupObject*>(obj))
                {
                    if (!pickup->isCollected)
                    {
                        float dist = Vector3Distance(player.position, pickup->position);
                        if (dist < minDistance)
                        {
                            minDistance = dist;
                            closestPickup = pickup;
                        }
                    }
                }
            }

            if (closestPickup)
            {
                activePromptText = TextFormat("Pressione [E] para coletar: %s", closestPickup->item.name.c_str());
                if (!isPaused && !isInventoryOpen && IsKeyPressed(KEY_E))
                {
                    if (player.inventory.AddItem(closestPickup->item))
                    {
                        closestPickup->isCollected = true;
                    }
                }
            }

            // Renderização da Cena
            BeginDrawing();
            ClearBackground((Color){20, 20, 30, 255});

            scene.Draw();

            // Renderiza Números de Dano Flutuantes na UI
            scene.damageFeedback.DrawUI(scene.camera);

            // Desenha Barras de Vida e Nome em cima da cabeça dos inimigos visíveis
            for (auto* obj : scene.objects)
            {
                if (auto* enemy = dynamic_cast<EnemyObject*>(obj))
                {
                    ui.DrawEnemyOverheadUI(*enemy, scene.camera, player.position);
                }
            }

            // Desenha HUD do RPG
            ui.DrawHUD(player, activePromptText.c_str());

            // Desenha Tela de Inventário
            int droppedSlot = -1;
            ui.DrawInventory(player, isInventoryOpen, droppedSlot);

            if (droppedSlot >= 0)
            {
                Item item = player.inventory.slots[droppedSlot];
                if (item.IsValid() && player.inventory.RemoveItem(droppedSlot))
                {
                    Vector3 dropPos = { player.position.x + sinf(player.meshYaw * DEG2RAD) * 1.5f, player.position.y, player.position.z + cosf(player.meshYaw * DEG2RAD) * 1.5f };
                    auto* newPickup = new PickupObject(item, dropPos);
                    newPickup->Init(scene.physics);
                    scene.objects.push_back(newPickup);
                }
            }

            // Overlay de Pausa
            if (isPaused)
            {
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

                    Rectangle btnResume = { btnX, btnY, btnW, btnH };
                    if (ui.DrawButton(btnResume, "CONTINUAR"))
                    {
                        isPaused = false;
                        DisableCursor();
                    }

                    Rectangle btnSettings = { btnX, btnY + 60.0f, btnW, btnH };
                    if (ui.DrawButton(btnSettings, "CONFIGURAÇÕES"))
                    {
                        settingsMenu.isOpen = true;
                    }

                    Rectangle btnMenu = { btnX, btnY + 120.0f, btnW, btnH };
                    if (ui.DrawButton(btnMenu, "VOLTAR AO MENU"))
                    {
                        ReturnToMainMenu();
                    }

                    Rectangle btnQuit = { btnX, btnY + 180.0f, btnW, btnH };
                    if (ui.DrawButton(btnQuit, "SAIR"))
                    {
                        running = false;
                    }
                }
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
