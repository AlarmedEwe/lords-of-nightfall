#include "LevelEditor.h"
#include "rlImGui.h"
#include "imgui.h"
#include "raymath.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

struct PropInfo {
    const char* displayName;
    const char* modelFile;
    bool isTree;
};

static PropInfo g_AvailableProps[] = {
    { "Arvore Comum 1", "Nature_CommonTree_1.lonmodel", true },
    { "Arvore Comum 2", "Nature_CommonTree_2.lonmodel", true },
    { "Arvore Comum 3", "Nature_CommonTree_3.lonmodel", true },
    { "Arvore Seca 1", "Nature_DeadTree_1.lonmodel", true },
    { "Arvore Seca 2", "Nature_DeadTree_2.lonmodel", true },
    { "Pinheiro 1", "Nature_Pine_1.lonmodel", true },
    { "Pinheiro 2", "Nature_Pine_2.lonmodel", true },
    { "Arvore Retorcida 1", "Nature_TwistedTree_1.lonmodel", true },
    { "Arbusto Comum", "Nature_Bush_Common.lonmodel", false },
    { "Arbusto com Flores", "Nature_Bush_Common_Flowers.lonmodel", false },
    { "Samambaia", "Nature_Fern_1.lonmodel", false },
    { "Planta Selvagem", "Nature_Plant_1.lonmodel", false },
    { "Flores 1", "Nature_Flower_3_Group.lonmodel", false },
    { "Flores 2", "Nature_Flower_4_Group.lonmodel", false },
    { "Grama Alta", "Nature_Grass_Common_Tall.lonmodel", false },
    { "Cogumelo", "Nature_Mushroom_Common.lonmodel", false },
    { "Rocha Media 1", "Nature_Rock_Medium_1.lonmodel", false },
    { "Rocha Media 2", "Nature_Rock_Medium_2.lonmodel", false },
    { "Pedra Redonda", "Nature_Pebble_Round_1.lonmodel", false }
};
static int g_NumProps = sizeof(g_AvailableProps) / sizeof(g_AvailableProps[0]);

struct MonsterInfo {
    const char* name;
    const char* category;
    const char* modelFile;
};

static MonsterInfo g_AvailableMonsters[] = {
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
static int g_NumMonsters = sizeof(g_AvailableMonsters) / sizeof(g_AvailableMonsters[0]);

struct ItemInfo {
    const char* name;
    ItemType type;
};

static ItemInfo g_AvailableItems[] = {
    { "Espada de Aco", ItemType::Weapon },
    { "Machado de Guerra", ItemType::Weapon },
    { "Pocao de Vida", ItemType::Potion },
    { "Armadura de Ranger", ItemType::Armor },
    { "Roupa de Campones", ItemType::Armor }
};
static int g_NumItems = sizeof(g_AvailableItems) / sizeof(g_AvailableItems[0]);

void LevelEditor::Init()
{
    editorCamera.position = (Vector3){ 0.0f, 15.0f, -20.0f };
    editorCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    editorCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    editorCamera.fovy = 60.0f;
    editorCamera.projection = CAMERA_PERSPECTIVE;

    cameraYaw = -90.0f;
    cameraPitch = -20.0f;
    isOpen = false;
    isFreeCamActive = false;
}

static Vector3 GetGroundRaycastHit(Ray ray, TerrainObject& terrain)
{
    // Intersecção simples com o plano Y=GetHeightAt
    Vector3 hitPos = (Vector3){ 0.0f, 0.0f, 0.0f };
    float step = 0.5f;
    float maxDist = 300.0f;

    for (float d = 0.0f; d < maxDist; d += step)
    {
        Vector3 p = Vector3Add(ray.position, Vector3Scale(ray.direction, d));
        float groundY = terrain.GetHeightAt(p.x, p.z);
        if (p.y <= groundY)
        {
            hitPos = (Vector3){ p.x, groundY, p.z };
            break;
        }
    }
    return hitPos;
}

void LevelEditor::Update(Scene& scene, TerrainObject& terrain, Physics& physics, float deltaTime)
{
    if (!isOpen) return;

    // 1. Mouse Look (Girar olhar da câmera ao segurar Botão Direito do Mouse)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        if (!isFreeCamActive)
        {
            isFreeCamActive = true;
            DisableCursor();
        }

        Vector2 mouseDelta = GetMouseDelta();
        cameraYaw += mouseDelta.x * 0.15f;
        cameraPitch -= mouseDelta.y * 0.15f;
        cameraPitch = std::clamp(cameraPitch, -89.0f, 89.0f);
    }
    else
    {
        if (isFreeCamActive)
        {
            isFreeCamActive = false;
            EnableCursor();
        }
    }

    // 2. Movimentação WASD Incondicional da Câmera no Level Editor
    Vector2 mousePos = GetMousePosition();
    float sidebarWidth = 350.0f + 16.0f;
    bool isMouseOverSidebar = (mousePos.x >= 0.0f && mousePos.x <= sidebarWidth && mousePos.y >= 0.0f && mousePos.y <= (float)GetScreenHeight());

    float yawRad = cameraYaw * DEG2RAD;
    float pitchRad = cameraPitch * DEG2RAD;

    Vector3 forward = {
        cosf(pitchRad) * cosf(yawRad),
        sinf(pitchRad),
        cosf(pitchRad) * sinf(yawRad)
    };
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3CrossProduct(forward, (Vector3){ 0.0f, 1.0f, 0.0f });
    if (Vector3Length(right) < 0.001f) right = (Vector3){ 1.0f, 0.0f, 0.0f };
    else right = Vector3Normalize(right);

    Vector3 forwardXZ = (Vector3){ forward.x, 0.0f, forward.z };
    if (Vector3Length(forwardXZ) > 0.001f) forwardXZ = Vector3Normalize(forwardXZ);
    else forwardXZ = forward;

    // Rodinha do mouse ajusta a velocidade de voo, o raio do brush ou a rolagem de listas
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f)
    {
        if (isMouseOverSidebar)
        {
            if (currentTool == EditorTool::Props)
                propsScroll = std::max(0.0f, propsScroll - wheel * 30.0f);
            else if (currentTool == EditorTool::Enemies)
                enemiesScroll = std::max(0.0f, enemiesScroll - wheel * 30.0f);
            else if (currentTool == EditorTool::Pickups)
                pickupsScroll = std::max(0.0f, pickupsScroll - wheel * 30.0f);
            else if (currentTool == EditorTool::Terraform)
                brushRadius = std::clamp(brushRadius + wheel * 1.0f, 1.0f, 30.0f);
        }
        else
        {
            if (isFreeCamActive)
                cameraMoveSpeed = std::clamp(cameraMoveSpeed + wheel * 5.0f, 2.0f, 120.0f);
            else if (currentTool == EditorTool::Terraform)
                brushRadius = std::clamp(brushRadius + wheel * 1.0f, 1.0f, 30.0f);
        }
    }

    float moveSpeed = cameraMoveSpeed;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) moveSpeed *= 3.0f;

    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
    if (IsKeyDown(KEY_W)) moveDir = Vector3Add(moveDir, forwardXZ);
    if (IsKeyDown(KEY_S)) moveDir = Vector3Subtract(moveDir, forwardXZ);
    if (IsKeyDown(KEY_D)) moveDir = Vector3Add(moveDir, right);
    if (IsKeyDown(KEY_A)) moveDir = Vector3Subtract(moveDir, right);
    if (IsKeyDown(KEY_E) || IsKeyDown(KEY_SPACE)) moveDir.y += 1.0f;
    if (IsKeyDown(KEY_Q)) moveDir.y -= 1.0f;

    if (Vector3Length(moveDir) > 0.0f)
    {
        moveDir = Vector3Normalize(moveDir);
        editorCamera.position = Vector3Add(editorCamera.position, Vector3Scale(moveDir, moveSpeed * deltaTime));
    }

    editorCamera.target = Vector3Add(editorCamera.position, forward);

    // 3. Interações no Mundo 3D com Ferramentas
    if (!isMouseOverSidebar && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Ray mouseRay = GetScreenToWorldRay(mousePos, editorCamera);
        Vector3 hitPoint = GetGroundRaycastHit(mouseRay, terrain);

        bool isCtrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        if (currentTool == EditorTool::Terraform)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                float deltaH = isCtrlDown ? -brushStrength * deltaTime : brushStrength * deltaTime;
                terrain.ModifyHeightAt(physics, hitPoint.x, hitPoint.z, deltaH, brushRadius);
            }
        }
        else if (currentTool == EditorTool::Props)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                if (isCtrlDown)
                {
                    for (auto it = scene.objects.begin(); it != scene.objects.end(); ++it)
                    {
                        if (auto* prop = dynamic_cast<NatureObject*>(*it))
                        {
                            if (Vector3Distance(prop->position, hitPoint) <= 3.0f)
                            {
                                prop->Shutdown(physics);
                                delete prop;
                                scene.objects.erase(it);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    const auto& pInfo = g_AvailableProps[selectedPropIdx];
                    float yaw = (float)(rand() % 360);
                    auto* prop = new NatureObject(pInfo.modelFile, hitPoint, yaw, 1.0f, pInfo.isTree);
                    prop->Init(physics);
                    if (prop->natureModel && prop->natureModel->isLoaded)
                    {
                        scene.lightingSystem.ApplyToModel(prop->natureModel->rayModel);
                    }
                    scene.objects.push_back(prop);
                }
            }
        }
        else if (currentTool == EditorTool::Enemies)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                if (isCtrlDown)
                {
                    for (auto it = scene.objects.begin(); it != scene.objects.end(); ++it)
                    {
                        if (auto* enemy = dynamic_cast<EnemyObject*>(*it))
                        {
                            if (Vector3Distance(enemy->position, hitPoint) <= 4.0f)
                            {
                                enemy->Shutdown(physics);
                                delete enemy;
                                scene.objects.erase(it);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    const auto& mInfo = g_AvailableMonsters[selectedEnemyIdx];
                    auto* enemy = new EnemyObject(mInfo.name, mInfo.category, mInfo.modelFile, selectedEnemyLevel, hitPoint);
                    enemy->Init(physics);
                    scene.objects.push_back(enemy);
                }
            }
        }
        else if (currentTool == EditorTool::Pickups)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                if (isCtrlDown)
                {
                    for (auto it = scene.objects.begin(); it != scene.objects.end(); ++it)
                    {
                        if (auto* pickup = dynamic_cast<PickupObject*>(*it))
                        {
                            if (Vector3Distance(pickup->position, hitPoint) <= 3.0f)
                            {
                                pickup->Shutdown(physics);
                                delete pickup;
                                scene.objects.erase(it);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    const auto& iInfo = g_AvailableItems[selectedPickupIdx];
                    Item item;
                    if (selectedPickupIdx == 0) item = Items::CreateSword();
                    else if (selectedPickupIdx == 1) item = Items::CreateAxe();
                    else if (selectedPickupIdx == 2) item = Items::CreateHealthPotion();
                    else if (selectedPickupIdx == 3) item = Items::CreateRangerArmor();
                    else item = Items::CreatePeasantOutfit();

                    auto* pickup = new PickupObject(item, hitPoint);
                    pickup->Init(physics);
                    scene.objects.push_back(pickup);
                }
            }
        }
    }
}

void LevelEditor::DrawUI(UISystem& ui, Scene& scene, TerrainObject& terrain, Physics& physics)
{
    if (!isOpen) return;

    float screenH = (float)GetScreenHeight();

    float panelX = 16.0f;
    float panelY = 16.0f;
    float panelW = 350.0f;
    float panelH = screenH - 32.0f;

    Rectangle sidebarRect = { panelX, panelY, panelW, panelH };

    // 1. Fundo do painel principal (estilo escuro do Menu de Pausa com borda Kenney)
    ui.DrawPanel(sidebarRect, (Color){22, 18, 30, 245});

    // 2. Header Superior
    Rectangle headerRect = { panelX + 16.0f, panelY + 16.0f, panelW - 32.0f, 44.0f };
    ui.DrawHeader(headerRect, "LEVEL EDITOR (F1)");

    const char* subText = "Segure RMB + WASD: Câmera Livre";
    int subFontSize = 14;
    int subTextW = MeasureText(subText, subFontSize);
    DrawText(subText, panelX + (panelW - subTextW) / 2.0f, panelY + 68.0f, subFontSize, (Color){180, 170, 160, 255});

    ui.DrawDivider(panelX + 16.0f, panelY + 88.0f, panelW - 32.0f);

    // 3. Botões de Seleção de Ferramentas (2 linhas de 2 abas)
    DrawText("FERRAMENTAS:", panelX + 18.0f, panelY + 100.0f, 16, (Color){255, 225, 120, 255});

    float tabW = (panelW - 40.0f) * 0.5f;
    float tabH = 34.0f;
    float tabX1 = panelX + 16.0f;
    float tabX2 = panelX + 16.0f + tabW + 8.0f;
    float tabY1 = panelY + 124.0f;
    float tabY2 = panelY + 164.0f;

    Rectangle tabTerraform = { tabX1, tabY1, tabW, tabH };
    if (ui.DrawTabButton(tabTerraform, "1. TERRAFORM", currentTool == EditorTool::Terraform))
    {
        currentTool = EditorTool::Terraform;
    }

    Rectangle tabProps = { tabX2, tabY1, tabW, tabH };
    if (ui.DrawTabButton(tabProps, "2. PROPS", currentTool == EditorTool::Props))
    {
        currentTool = EditorTool::Props;
    }

    Rectangle tabEnemies = { tabX1, tabY2, tabW, tabH };
    if (ui.DrawTabButton(tabEnemies, "3. INIMIGOS", currentTool == EditorTool::Enemies))
    {
        currentTool = EditorTool::Enemies;
    }

    Rectangle tabPickups = { tabX2, tabY2, tabW, tabH };
    if (ui.DrawTabButton(tabPickups, "4. COLETÁVEIS", currentTool == EditorTool::Pickups))
    {
        currentTool = EditorTool::Pickups;
    }

    ui.DrawDivider(panelX + 16.0f, panelY + 208.0f, panelW - 32.0f);

    // 4. Conteúdo da Ferramenta Selecionada
    float contentY = panelY + 220.0f;
    float footerHeight = 100.0f;
    float maxContentH = (panelY + panelH - footerHeight) - contentY;

    if (currentTool == EditorTool::Terraform)
    {
        DrawText("[ TERRAFORMAGEM ]", panelX + 18.0f, contentY, 18, (Color){255, 225, 120, 255});

        Rectangle sliderR1 = { panelX + 16.0f, contentY + 30.0f, panelW - 32.0f, 36.0f };
        ui.DrawSlider(sliderR1, "Raio Brush", brushRadius, 1.0f, 30.0f);

        Rectangle sliderR2 = { panelX + 16.0f, contentY + 74.0f, panelW - 32.0f, 36.0f };
        ui.DrawSlider(sliderR2, "Força", brushStrength, 1.0f, 25.0f);

        // Caixa de Instruções
        Rectangle instrRect = { panelX + 16.0f, contentY + 126.0f, panelW - 32.0f, 100.0f };
        DrawRectangleRounded(instrRect, 0.08f, 4, (Color){15, 12, 20, 230});
        ui.DrawBorder(instrRect, (Color){140, 110, 50, 200}, 1.5f);

        DrawText("INSTRUÇÕES:", instrRect.x + 12.0f, instrRect.y + 10.0f, 15, (Color){255, 220, 100, 255});
        DrawText("• Clique Esq: Subir terreno", instrRect.x + 12.0f, instrRect.y + 32.0f, 14, (Color){220, 210, 190, 255});
        DrawText("• Ctrl + Clique: Abaixar terreno", instrRect.x + 12.0f, instrRect.y + 52.0f, 14, (Color){220, 210, 190, 255});
        DrawText("• Scroll do Mouse: Ajustar raio", instrRect.x + 12.0f, instrRect.y + 72.0f, 14, (Color){220, 210, 190, 255});
    }
    else if (currentTool == EditorTool::Props)
    {
        DrawText("[ PROPS DO CENÁRIO ]", panelX + 18.0f, contentY, 18, (Color){255, 225, 120, 255});

        // Lista com rolagem
        float listH = std::min(220.0f, maxContentH - 100.0f);
        Rectangle listRect = { panelX + 16.0f, contentY + 28.0f, panelW - 32.0f, listH };
        DrawRectangleRounded(listRect, 0.05f, 4, (Color){15, 12, 20, 240});
        ui.DrawBorder(listRect, (Color){140, 110, 50, 200}, 1.5f);

        BeginScissorMode((int)listRect.x + 2, (int)listRect.y + 2, (int)listRect.width - 4, (int)listRect.height - 4);

        float itemH = 28.0f;
        float totalItemsH = g_NumProps * itemH;
        float maxScroll = std::max(0.0f, totalItemsH - listRect.height + 6.0f);
        propsScroll = std::clamp(propsScroll, 0.0f, maxScroll);

        Vector2 mousePos = GetMousePosition();

        for (int i = 0; i < g_NumProps; i++)
        {
            float itemY = listRect.y + 4.0f + (i * itemH) - propsScroll;
            if (itemY + itemH < listRect.y || itemY > listRect.y + listRect.height) continue;

            Rectangle itemRect = { listRect.x + 4.0f, itemY, listRect.width - 8.0f, itemH - 2.0f };
            bool isHovered = CheckCollisionPointRec(mousePos, itemRect) && CheckCollisionPointRec(mousePos, listRect);
            bool isSelected = (selectedPropIdx == i);

            if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                selectedPropIdx = i;
            }

            Color itemBg = isSelected ? (Color){80, 50, 20, 240} :
                           (isHovered ? (Color){50, 38, 48, 220} : (Color){25, 20, 32, 180});
            Color itemBorder = isSelected ? (Color){245, 200, 80, 255} :
                               (isHovered ? (Color){180, 140, 60, 220} : (Color){70, 55, 40, 150});
            Color itemText = isSelected ? (Color){255, 230, 130, 255} :
                             (isHovered ? (Color){245, 235, 200, 255} : (Color){200, 190, 180, 255});

            DrawRectangleRounded(itemRect, 0.15f, 4, itemBg);
            DrawRectangleLinesEx(itemRect, isSelected ? 1.5f : 1.0f, itemBorder);
            DrawText(g_AvailableProps[i].displayName, itemRect.x + 10.0f, itemRect.y + (itemH - 16.0f) / 2.0f, 15, itemText);
        }

        EndScissorMode();

        // Instruções
        float instrY = listRect.y + listRect.height + 10.0f;
        Rectangle instrRect = { panelX + 16.0f, instrY, panelW - 32.0f, 74.0f };
        DrawRectangleRounded(instrRect, 0.08f, 4, (Color){15, 12, 20, 230});
        ui.DrawBorder(instrRect, (Color){140, 110, 50, 200}, 1.5f);

        DrawText("INSTRUÇÕES:", instrRect.x + 12.0f, instrRect.y + 8.0f, 15, (Color){255, 220, 100, 255});
        DrawText("• Clique Esq: Posicionar prop", instrRect.x + 12.0f, instrRect.y + 28.0f, 14, (Color){220, 210, 190, 255});
        DrawText("• Ctrl + Clique: Deletar prop", instrRect.x + 12.0f, instrRect.y + 48.0f, 14, (Color){220, 210, 190, 255});
    }
    else if (currentTool == EditorTool::Enemies)
    {
        DrawText("[ INIMIGOS ]", panelX + 18.0f, contentY, 18, (Color){255, 225, 120, 255});

        // Lista com rolagem
        float listH = std::min(180.0f, maxContentH - 150.0f);
        Rectangle listRect = { panelX + 16.0f, contentY + 28.0f, panelW - 32.0f, listH };
        DrawRectangleRounded(listRect, 0.05f, 4, (Color){15, 12, 20, 240});
        ui.DrawBorder(listRect, (Color){140, 110, 50, 200}, 1.5f);

        BeginScissorMode((int)listRect.x + 2, (int)listRect.y + 2, (int)listRect.width - 4, (int)listRect.height - 4);

        float itemH = 28.0f;
        float totalItemsH = g_NumMonsters * itemH;
        float maxScroll = std::max(0.0f, totalItemsH - listRect.height + 6.0f);
        enemiesScroll = std::clamp(enemiesScroll, 0.0f, maxScroll);

        Vector2 mousePos = GetMousePosition();

        for (int i = 0; i < g_NumMonsters; i++)
        {
            float itemY = listRect.y + 4.0f + (i * itemH) - enemiesScroll;
            if (itemY + itemH < listRect.y || itemY > listRect.y + listRect.height) continue;

            Rectangle itemRect = { listRect.x + 4.0f, itemY, listRect.width - 8.0f, itemH - 2.0f };
            bool isHovered = CheckCollisionPointRec(mousePos, itemRect) && CheckCollisionPointRec(mousePos, listRect);
            bool isSelected = (selectedEnemyIdx == i);

            if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                selectedEnemyIdx = i;
            }

            Color itemBg = isSelected ? (Color){80, 50, 20, 240} :
                           (isHovered ? (Color){50, 38, 48, 220} : (Color){25, 20, 32, 180});
            Color itemBorder = isSelected ? (Color){245, 200, 80, 255} :
                               (isHovered ? (Color){180, 140, 60, 220} : (Color){70, 55, 40, 150});
            Color itemText = isSelected ? (Color){255, 230, 130, 255} :
                             (isHovered ? (Color){245, 235, 200, 255} : (Color){200, 190, 180, 255});

            DrawRectangleRounded(itemRect, 0.15f, 4, itemBg);
            DrawRectangleLinesEx(itemRect, isSelected ? 1.5f : 1.0f, itemBorder);

            std::string label = std::string(g_AvailableMonsters[i].name) + " [" + g_AvailableMonsters[i].category + "]";
            DrawText(label.c_str(), itemRect.x + 10.0f, itemRect.y + (itemH - 16.0f) / 2.0f, 15, itemText);
        }

        EndScissorMode();

        // Slider Nível
        float lvlY = listRect.y + listRect.height + 8.0f;
        Rectangle sliderLvl = { panelX + 16.0f, lvlY, panelW - 32.0f, 32.0f };
        float lvlF = (float)selectedEnemyLevel;
        if (ui.DrawSlider(sliderLvl, "Nível", lvlF, 1.0f, 50.0f))
        {
            selectedEnemyLevel = (int)lvlF;
        }

        // Botão Resetar Spawns
        Rectangle resetBtn = { panelX + 16.0f, lvlY + 38.0f, panelW - 32.0f, 30.0f };
        if (ui.DrawButton(resetBtn, "RESETAR POSIÇÕES"))
        {
            ResetEnemies(scene);
        }

        // Instruções
        Rectangle instrRect = { panelX + 16.0f, lvlY + 74.0f, panelW - 32.0f, 54.0f };
        DrawRectangleRounded(instrRect, 0.08f, 4, (Color){15, 12, 20, 230});
        ui.DrawBorder(instrRect, (Color){140, 110, 50, 200}, 1.5f);

        DrawText("• Clique Esq: Spawnar inimigo", instrRect.x + 12.0f, instrRect.y + 10.0f, 14, (Color){220, 210, 190, 255});
        DrawText("• Ctrl + Clique: Remover inimigo", instrRect.x + 12.0f, instrRect.y + 30.0f, 14, (Color){220, 210, 190, 255});
    }
    else if (currentTool == EditorTool::Pickups)
    {
        DrawText("[ ITENS COLETÁVEIS ]", panelX + 18.0f, contentY, 18, (Color){255, 225, 120, 255});

        // Lista com rolagem
        float listH = std::min(220.0f, maxContentH - 100.0f);
        Rectangle listRect = { panelX + 16.0f, contentY + 28.0f, panelW - 32.0f, listH };
        DrawRectangleRounded(listRect, 0.05f, 4, (Color){15, 12, 20, 240});
        ui.DrawBorder(listRect, (Color){140, 110, 50, 200}, 1.5f);

        BeginScissorMode((int)listRect.x + 2, (int)listRect.y + 2, (int)listRect.width - 4, (int)listRect.height - 4);

        float itemH = 28.0f;
        float totalItemsH = g_NumItems * itemH;
        float maxScroll = std::max(0.0f, totalItemsH - listRect.height + 6.0f);
        pickupsScroll = std::clamp(pickupsScroll, 0.0f, maxScroll);

        Vector2 mousePos = GetMousePosition();

        for (int i = 0; i < g_NumItems; i++)
        {
            float itemY = listRect.y + 4.0f + (i * itemH) - pickupsScroll;
            if (itemY + itemH < listRect.y || itemY > listRect.y + listRect.height) continue;

            Rectangle itemRect = { listRect.x + 4.0f, itemY, listRect.width - 8.0f, itemH - 2.0f };
            bool isHovered = CheckCollisionPointRec(mousePos, itemRect) && CheckCollisionPointRec(mousePos, listRect);
            bool isSelected = (selectedPickupIdx == i);

            if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                selectedPickupIdx = i;
            }

            Color itemBg = isSelected ? (Color){80, 50, 20, 240} :
                           (isHovered ? (Color){50, 38, 48, 220} : (Color){25, 20, 32, 180});
            Color itemBorder = isSelected ? (Color){245, 200, 80, 255} :
                               (isHovered ? (Color){180, 140, 60, 220} : (Color){70, 55, 40, 150});
            Color itemText = isSelected ? (Color){255, 230, 130, 255} :
                             (isHovered ? (Color){245, 235, 200, 255} : (Color){200, 190, 180, 255});

            DrawRectangleRounded(itemRect, 0.15f, 4, itemBg);
            DrawRectangleLinesEx(itemRect, isSelected ? 1.5f : 1.0f, itemBorder);
            DrawText(g_AvailableItems[i].name, itemRect.x + 10.0f, itemRect.y + (itemH - 16.0f) / 2.0f, 15, itemText);
        }

        EndScissorMode();

        // Instruções
        float instrY = listRect.y + listRect.height + 10.0f;
        Rectangle instrRect = { panelX + 16.0f, instrY, panelW - 32.0f, 74.0f };
        DrawRectangleRounded(instrRect, 0.08f, 4, (Color){15, 12, 20, 230});
        ui.DrawBorder(instrRect, (Color){140, 110, 50, 200}, 1.5f);

        DrawText("INSTRUÇÕES:", instrRect.x + 12.0f, instrRect.y + 8.0f, 15, (Color){255, 220, 100, 255});
        DrawText("• Clique Esq: Posicionar item", instrRect.x + 12.0f, instrRect.y + 28.0f, 14, (Color){220, 210, 190, 255});
        DrawText("• Ctrl + Clique: Deletar item", instrRect.x + 12.0f, instrRect.y + 48.0f, 14, (Color){220, 210, 190, 255});
    }

    // 5. Rodapé Fixo (Salvar / Recarregar Level)
    float footerY = panelY + panelH - footerHeight;
    ui.DrawDivider(panelX + 16.0f, footerY, panelW - 32.0f);

    Rectangle saveBtn = { panelX + 16.0f, footerY + 12.0f, panelW - 32.0f, 36.0f };
    if (ui.DrawButton(saveBtn, "SALVAR LEVEL"))
    {
        SaveLevel(scene, terrain, "Build/Development/Saved");
    }

    Rectangle loadBtn = { panelX + 16.0f, footerY + 54.0f, panelW - 32.0f, 32.0f };
    if (ui.DrawButton(loadBtn, "RECARREGAR SALVO"))
    {
        LoadLevel(scene, terrain, physics, "Build/Development/Saved");
    }
}

void LevelEditor::Draw3DGizmos(const Scene& scene, const TerrainObject& terrain)
{
    if (!isOpen) return;

    // Renderiza Brush de Terraformagem ou Preview de Spawn
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Ray mouseRay = GetScreenToWorldRay(GetMousePosition(), editorCamera);
        Vector3 hitPoint = GetGroundRaycastHit(mouseRay, (TerrainObject&)terrain);

        if (currentTool == EditorTool::Terraform)
        {
            DrawCircle3D((Vector3){ hitPoint.x, hitPoint.y + 0.1f, hitPoint.z }, brushRadius, (Vector3){ 1.0f, 0.0f, 0.0f }, 90.0f, (Color){ 255, 100, 100, 180 });
        }
        else if (currentTool == EditorTool::Props || currentTool == EditorTool::Enemies || currentTool == EditorTool::Pickups)
        {
            DrawSphere((Vector3){ hitPoint.x, hitPoint.y + 0.5f, hitPoint.z }, 0.6f, (Color){ 100, 255, 100, 180 });
        }
    }

    // Desenha marcas nos pontos de spawn dos inimigos no level
    for (auto* obj : scene.objects)
    {
        if (auto* enemy = dynamic_cast<EnemyObject*>(obj))
        {
            DrawCylinder(enemy->spawnPos, 0.4f, 0.4f, 0.8f, 8, (Color){ 255, 200, 0, 140 });
        }
    }
}

void LevelEditor::ResetEnemies(Scene& scene)
{
    for (auto* obj : scene.objects)
    {
        if (auto* enemy = dynamic_cast<EnemyObject*>(obj))
        {
            enemy->position = enemy->spawnPos;
            enemy->health = enemy->maxHealth;
            enemy->isDead = false;
        }
    }
}

bool LevelEditor::SaveLevel(const Scene& scene, const TerrainObject& terrain, const std::string& savedDir)
{
    mkdir("Build", 0755);
    mkdir("Build/Development", 0755);
    mkdir(savedDir.c_str(), 0755);

    std::string filePath = savedDir + "/level_data.json";
    std::ofstream f(filePath);
    if (!f.is_open()) return false;

    f << "{\n";
    f << "  \"heights\": [";
    for (size_t i = 0; i < terrain.heightData.size(); i++)
    {
        f << terrain.heightData[i];
        if (i + 1 < terrain.heightData.size()) f << ", ";
    }
    f << "],\n";

    f << "  \"props\": [\n";
    bool firstProp = true;
    for (auto* obj : scene.objects)
    {
        if (auto* prop = dynamic_cast<NatureObject*>(obj))
        {
            if (!firstProp) f << ",\n";
            firstProp = false;
            f << "    { \"modelFile\": \"" << prop->modelFile << "\", \"x\": " << prop->position.x
              << ", \"y\": " << prop->position.y << ", \"z\": " << prop->position.z
              << ", \"yaw\": " << prop->yaw << ", \"scale\": " << prop->scaleVal
              << ", \"isTree\": " << (prop->isTree ? "true" : "false") << " }";
        }
    }
    f << "\n  ],\n";

    f << "  \"enemies\": [\n";
    bool firstEnemy = true;
    for (auto* obj : scene.objects)
    {
        if (auto* enemy = dynamic_cast<EnemyObject*>(obj))
        {
            if (!firstEnemy) f << ",\n";
            firstEnemy = false;
            f << "    { \"name\": \"" << enemy->name << "\", \"category\": \"" << enemy->category
              << "\", \"modelFile\": \"" << enemy->modelFile << "\", \"level\": " << enemy->level
              << ", \"x\": " << enemy->spawnPos.x << ", \"y\": " << enemy->spawnPos.y << ", \"z\": " << enemy->spawnPos.z << " }";
        }
    }
    f << "\n  ]\n";
    f << "}\n";

    f.close();
    TraceLog(LOG_INFO, "[LevelEditor] Level salvo com sucesso em %s", filePath.c_str());
    return true;
}

bool LevelEditor::LoadLevel(Scene& scene, TerrainObject& terrain, Physics& physics, const std::string& savedDir)
{
    std::string filePath = savedDir + "/level_data.json";
    std::ifstream f(filePath);
    if (!f.is_open()) return false;

    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string content = buffer.str();
    f.close();

    // 1. Carrega heightmap do terreno
    size_t heightsPos = content.find("\"heights\": [");
    if (heightsPos != std::string::npos)
    {
        size_t start = heightsPos + 12;
        size_t end = content.find("]", start);
        if (end != std::string::npos)
        {
            std::string heightsStr = content.substr(start, end - start);
            std::stringstream ss(heightsStr);
            float val;
            char comma;
            size_t idx = 0;
            while (ss >> val)
            {
                if (idx < terrain.heightData.size())
                {
                    terrain.heightData[idx] = val;
                }
                ss >> comma;
                idx++;
            }
            if (terrain.meshGenerated && terrain.mesh.vertices)
            {
                for (size_t i = 0; i < terrain.heightData.size(); i++)
                {
                    terrain.mesh.vertices[i * 3 + 1] = terrain.heightData[i];
                }
                UpdateMeshBuffer(terrain.mesh, 0, terrain.mesh.vertices, terrain.GRID_SIZE * terrain.GRID_SIZE * 3 * sizeof(float), 0);
                terrain.RebuildPhysicsShape(physics);
            }
        }
    }

    // 2. Remove objetos dinâmicos existentes (props e inimigos)
    for (auto it = scene.objects.begin(); it != scene.objects.end(); )
    {
        GameObject* obj = *it;
        if (dynamic_cast<NatureObject*>(obj) || dynamic_cast<EnemyObject*>(obj))
        {
            obj->Shutdown(physics);
            delete obj;
            it = scene.objects.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 3. Re-instancia Props
    size_t propsPos = content.find("\"props\": [");
    if (propsPos != std::string::npos)
    {
        size_t endProps = content.find("]", propsPos);
        std::string propsBlock = content.substr(propsPos, endProps - propsPos);

        size_t p = 0;
        while ((p = propsBlock.find("{", p)) != std::string::npos)
        {
            size_t pEnd = propsBlock.find("}", p);
            if (pEnd == std::string::npos) break;
            std::string itemStr = propsBlock.substr(p, pEnd - p);

            char modelBuf[128] = {0};
            float x = 0, y = 0, z = 0, yaw = 0, scale = 1.0f;

            if (sscanf(itemStr.c_str(), " { \"modelFile\": \"%127[^\"]\", \"x\": %f, \"y\": %f, \"z\": %f, \"yaw\": %f, \"scale\": %f", modelBuf, &x, &y, &z, &yaw, &scale) >= 4)
            {
                bool isTree = (itemStr.find("\"isTree\": true") != std::string::npos);
                auto* prop = new NatureObject(modelBuf, (Vector3){ x, y, z }, yaw, scale, isTree);
                prop->Init(physics);
                if (prop->natureModel && prop->natureModel->isLoaded)
                {
                    scene.lightingSystem.ApplyToModel(prop->natureModel->rayModel);
                }
                scene.objects.push_back(prop);
            }
            p = pEnd + 1;
        }
    }

    // 4. Re-instancia Inimigos
    size_t enemiesPos = content.find("\"enemies\": [");
    if (enemiesPos != std::string::npos)
    {
        size_t endEnemies = content.find("]", enemiesPos);
        std::string enemiesBlock = content.substr(enemiesPos, endEnemies - enemiesPos);

        size_t p = 0;
        while ((p = enemiesBlock.find("{", p)) != std::string::npos)
        {
            size_t pEnd = enemiesBlock.find("}", p);
            if (pEnd == std::string::npos) break;
            std::string itemStr = enemiesBlock.substr(p, pEnd - p);

            char nameBuf[64] = {0}, catBuf[32] = {0}, modelBuf[128] = {0};
            int level = 5;
            float x = 0, y = 0, z = 0;

            if (sscanf(itemStr.c_str(), " { \"name\": \"%63[^\"]\", \"category\": \"%31[^\"]\", \"modelFile\": \"%127[^\"]\", \"level\": %d, \"x\": %f, \"y\": %f, \"z\": %f", nameBuf, catBuf, modelBuf, &level, &x, &y, &z) >= 7)
            {
                auto* enemy = new EnemyObject(nameBuf, catBuf, modelBuf, level, (Vector3){ x, y, z });
                enemy->Init(physics);
                scene.objects.push_back(enemy);
            }
            p = pEnd + 1;
        }
    }

    TraceLog(LOG_INFO, "[LevelEditor] Level carregado com sucesso a partir de %s", filePath.c_str());
    return true;
}
