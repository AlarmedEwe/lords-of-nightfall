#include "MainMenuScene.h"
#include "raymath.h"
#include <cmath>

void MainMenuScene::Init()
{
    physics.Init();

    camera.position = {0.0f, cameraHeight, cameraDistance};
    camera.target = centerTarget;
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    skybox.Init();
    lightingSystem.Init(1024);

    terrain.Init(physics);
    terrain.model.materials[0].shader = lightingSystem.shadowShader;
    physics.system.OptimizeBroadPhase();

    // Carrega o modelo 3D lonmodel v2 do personagem no Menu Principal
    characterModel.Load("Superhero_Male.lonmodel", "T_Superhero_Male_Ligh.png");
    characterModel.SetAnimation(0); // Idle
    if (characterModel.isLoaded)
    {
        lightingSystem.ApplyToModel(characterModel.rayModel);
    }
}

void MainMenuScene::Update(float deltaTime)
{
    physics.Update(deltaTime);

    // Câmera gira continuamente em torno do centro da cena
    cameraAngle += deltaTime * 0.25f;

    float rad = cameraAngle;
    camera.position.x = centerTarget.x + sinf(rad) * cameraDistance;
    camera.position.z = centerTarget.z + cosf(rad) * cameraDistance;
    camera.position.y = centerTarget.y + cameraHeight + sinf(rad * 0.5f) * 1.0f;
    camera.target = centerTarget;

    // Atualiza a animação Idle do modelo 3D
    characterModel.Update(deltaTime);
}

void MainMenuScene::Draw(UISystem& ui, SettingsMenu& settingsMenu, Settings& settings, int screenWidth, int screenHeight, MainMenuAction& outAction)
{
    outAction = MainMenuAction::None;

    Vector3 charPos = centerTarget;
    float terrainH = terrain.GetHeightAt(charPos.x, charPos.z);
    charPos.y = terrainH;
    centerTarget.y = charPos.y + 1.0f;

    // Pass 1: Renderiza o mapa de sombras a partir da luz do sol
    lightingSystem.BeginShadowMapPass(charPos);
        terrain.Draw();
        if (characterModel.isLoaded)
        {
            characterModel.Draw(charPos, cameraAngle * RAD2DEG, 1.0f, WHITE);
        }
    lightingSystem.EndShadowMapPass();

    // Pass 2: Renderização da Cena 3D com Skybox e Sombras
    skybox.Draw(camera, lightingSystem.lightDir);

    lightingSystem.BindLightingUniforms(camera.position);

    BeginMode3D(camera);

    terrain.Draw();

    if (characterModel.isLoaded)
    {
        characterModel.Draw(charPos, cameraAngle * RAD2DEG, 1.0f, WHITE);
    }
    else
    {
        Vector3 cubePos = {charPos.x, charPos.y + 1.0f, charPos.z};
        DrawCube(cubePos, 2.0f, 2.0f, 2.0f, (Color){160, 50, 60, 255});
        DrawCubeWires(cubePos, 2.05f, 2.05f, 2.05f, (Color){240, 200, 80, 255});
    }

    EndMode3D();

    // Se o menu de configurações estiver aberto, desenha apenas ele
    if (settingsMenu.isOpen)
    {
        settingsMenu.Draw(ui, settings, screenWidth, screenHeight);
        return;
    }

    // Painel do Menu Principal
    float panelW = 340.0f;
    float panelH = 380.0f;
    float panelX = 60.0f;
    float panelY = (screenHeight - panelH) / 2.0f;

    Rectangle mainPanelRect = { panelX, panelY, panelW, panelH };
    ui.DrawPanel(mainPanelRect, (Color){20, 16, 28, 235});

    // Título do Jogo
    Rectangle titleRect = { panelX + 20, panelY + 25, panelW - 40, 50 };
    ui.DrawHeader(titleRect, "LORDS OF NIGHTFALL");

    ui.DrawDivider(panelX + 20, panelY + 90, panelW - 40);

    // Botões de Ação
    float btnY = panelY + 115.0f;
    float btnW = panelW - 60.0f;
    float btnH = 46.0f;
    float btnX = panelX + 30.0f;

    Rectangle btnPlay = { btnX, btnY, btnW, btnH };
    if (ui.DrawButton(btnPlay, "JOGAR"))
    {
        outAction = MainMenuAction::Play;
    }

    Rectangle btnSettings = { btnX, btnY + 65.0f, btnW, btnH };
    if (ui.DrawButton(btnSettings, "CONFIGURAÇÕES"))
    {
        settingsMenu.isOpen = true;
    }

    Rectangle btnQuit = { btnX, btnY + 130.0f, btnW, btnH };
    if (ui.DrawButton(btnQuit, "SAIR"))
    {
        outAction = MainMenuAction::Quit;
    }

    ui.DrawDivider(panelX + 20, panelY + panelH - 45, panelW - 40);

    // Créditos no Canto Inferior Esquerdo
#if defined(LON_DEBUG)
    const char* buildStr = "Stormhowl Studios | Build v0.4.0 (Debug)";
#elif defined(LON_DEVELOPMENT)
    const char* buildStr = "Stormhowl Studios | Build v0.4.0 (Development)";
#else
    const char* buildStr = "Stormhowl Studios | Build v0.4.0 (Release)";
#endif

    DrawRectangle(10, screenHeight - 34, MeasureText(buildStr, 14) + 16, 24, (Color){15, 12, 22, 200});
    DrawRectangleLines(10, screenHeight - 34, MeasureText(buildStr, 14) + 16, 24, (Color){140, 105, 45, 180});
    DrawText(buildStr, 18, screenHeight - 29, 14, (Color){220, 200, 160, 255});
}

void MainMenuScene::Shutdown()
{
    skybox.Shutdown();
    lightingSystem.Shutdown();
    characterModel.Unload();
    terrain.Shutdown(physics);
    physics.Shutdown();
}
