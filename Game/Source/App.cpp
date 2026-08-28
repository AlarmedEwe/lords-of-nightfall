#include "App.h"
#include "GameObjects/EnemyObject.h"
#include "GameObjects/NatureObject.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(LON_DEVELOPMENT) || defined(LON_DEBUG)
#include "rlImGui.h"

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

bool App::Init()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(screenWidth, screenHeight, "Lords of Nightfall");
    if (!IsWindowReady())
    {
        TraceLog(LOG_ERROR, "GLFW/Window failed to initialize! Check display environment.");
        return false;
    }
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

    // Spawna elementos de natureza (Stylized Nature MegaKit) pelo mapa
    const char* treeModels[] = {
        "Nature_CommonTree_1.lonmodel", "Nature_CommonTree_2.lonmodel", "Nature_CommonTree_3.lonmodel",
        "Nature_CommonTree_4.lonmodel", "Nature_CommonTree_5.lonmodel",
        "Nature_DeadTree_1.lonmodel", "Nature_DeadTree_2.lonmodel", "Nature_DeadTree_3.lonmodel",
        "Nature_DeadTree_4.lonmodel", "Nature_DeadTree_5.lonmodel",
        "Nature_Pine_1.lonmodel", "Nature_Pine_2.lonmodel", "Nature_Pine_3.lonmodel",
        "Nature_Pine_4.lonmodel", "Nature_Pine_5.lonmodel",
        "Nature_TwistedTree_1.lonmodel", "Nature_TwistedTree_2.lonmodel", "Nature_TwistedTree_3.lonmodel",
        "Nature_TwistedTree_4.lonmodel", "Nature_TwistedTree_5.lonmodel"
    };
    int numTreeModels = sizeof(treeModels) / sizeof(treeModels[0]);

    const char* propModels[] = {
        "Nature_Bush_Common.lonmodel", "Nature_Bush_Common_Flowers.lonmodel",
        "Nature_Fern_1.lonmodel", "Nature_Plant_1.lonmodel", "Nature_Plant_7.lonmodel",
        "Nature_Flower_3_Group.lonmodel", "Nature_Flower_4_Group.lonmodel",
        "Nature_Grass_Common_Tall.lonmodel", "Nature_Grass_Wispy_Tall.lonmodel",
        "Nature_Mushroom_Common.lonmodel", "Nature_Mushroom_Laetiporus.lonmodel",
        "Nature_Rock_Medium_1.lonmodel", "Nature_Rock_Medium_2.lonmodel", "Nature_Rock_Medium_3.lonmodel",
        "Nature_Pebble_Round_1.lonmodel", "Nature_Pebble_Square_1.lonmodel"
    };
    int numPropModels = sizeof(propModels) / sizeof(propModels[0]);

    srand((unsigned int)time(NULL));

    // Spawna Árvores (Com Colisão Física Jolt)
    for (int i = 0; i < 160; i++)
    {
        const char* modelFile = treeModels[rand() % numTreeModels];
        float angle = ((float)(rand() % 360)) * DEG2RAD;
        float dist = 8.0f + (float)(rand() % 750) / 10.0f;
        float spawnX = cosf(angle) * dist;
        float spawnZ = sinf(angle) * dist;
        float yaw = (float)(rand() % 360);
        float scale = 0.8f + (float)(rand() % 60) / 100.0f;

        auto* tree = new NatureObject(modelFile, (Vector3){ spawnX, 0.0f, spawnZ }, yaw, scale, true);
        scene.objects.push_back(tree);
    }

    // Spawna Adereços de Natureza sem colisão (Arbustos, Plantas, Flores, Cogumelos, Pedras)
    for (int i = 0; i < 200; i++)
    {
        const char* modelFile = propModels[rand() % numPropModels];
        float angle = ((float)(rand() % 360)) * DEG2RAD;
        float dist = 5.0f + (float)(rand() % 780) / 10.0f;
        float spawnX = cosf(angle) * dist;
        float spawnZ = sinf(angle) * dist;
        float yaw = (float)(rand() % 360);
        float scale = 0.7f + (float)(rand() % 80) / 100.0f;

        auto* prop = new NatureObject(modelFile, (Vector3){ spawnX, 0.0f, spawnZ }, yaw, scale, false);
        scene.objects.push_back(prop);
    }

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
    levelEditor.Init();
    levelEditor.LoadLevel(scene, terrain, scene.physics, "Game/Assets/Levels/Default.lvl");
    state = GameState::Gameplay;
    DisableCursor();
#else
    state = GameState::MainMenu;
    EnableCursor();
#endif
    return true;
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
#if defined(LON_DEVELOPMENT) || defined(LON_DEBUG)
            if (IsKeyPressed(KEY_F1))
            {
                levelEditor.isOpen = !levelEditor.isOpen;
                if (levelEditor.isOpen)
                {
                    levelEditor.editorCamera.position = scene.camera.position;
                    levelEditor.editorCamera.target = scene.camera.target;
                    levelEditor.editorCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
                    levelEditor.editorCamera.fovy = 60.0f;

                    Vector3 dir = Vector3Normalize(Vector3Subtract(scene.camera.target, scene.camera.position));
                    if (Vector3Length(dir) > 0.001f)
                    {
                        levelEditor.cameraPitch = std::clamp(asin(dir.y) * RAD2DEG, -89.0f, 89.0f);
                        levelEditor.cameraYaw = atan2(dir.z, dir.x) * RAD2DEG;
                    }

                    EnableCursor();
                }
                else
                {
                    DisableCursor();
                }
            }

            if (levelEditor.isOpen)
            {
                levelEditor.Update(scene, terrain, scene.physics, dt);

                Camera3D origCam = scene.camera;
                scene.camera = levelEditor.editorCamera;

                BeginDrawing();
                ClearBackground((Color){20, 20, 30, 255});

                scene.Draw();

                BeginMode3D(levelEditor.editorCamera);
                levelEditor.Draw3DGizmos(scene, terrain);
                EndMode3D();

                levelEditor.DrawUI(ui, scene, terrain, scene.physics);

                EndDrawing();

                scene.camera = origCam;
                continue;
            }
#endif

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
#if defined(LON_DEVELOPMENT) || defined(LON_DEBUG)
    rlImGui::Shutdown();
#endif
    mainMenuScene.Shutdown();
    scene.Shutdown();
    ui.Shutdown();
    CloseWindow();
}
