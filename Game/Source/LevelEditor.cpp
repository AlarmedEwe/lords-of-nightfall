#include "LevelEditor.h"
#include "TerrainAligner.h"
#include "rlImGui.h"
#include "imgui.h"
#include "raymath.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string Base64Encode(const uint8_t* data, size_t len)
{
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3)
    {
        uint32_t b = (data[i] << 16);
        if (i + 1 < len) b |= (data[i + 1] << 8);
        if (i + 2 < len) b |= data[i + 2];

        out.push_back(b64_table[(b >> 18) & 0x3F]);
        out.push_back(b64_table[(b >> 12) & 0x3F]);
        out.push_back((i + 1 < len) ? b64_table[(b >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < len) ? b64_table[b & 0x3F] : '=');
    }
    return out;
}

static std::vector<uint8_t> Base64Decode(const std::string& str)
{
    std::vector<uint8_t> out;
    out.reserve((str.length() / 4) * 3);
    uint32_t val = 0;
    int valb = -8;
    for (char c : str)
    {
        if (c == '=' || c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
        const char* p = strchr(b64_table, c);
        if (!p) continue;
        val = (val << 6) | (p - b64_table);
        valb += 6;
        if (valb >= 0)
        {
            out.push_back((uint8_t)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

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

    // 1. Mouse Look
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

    // 2. Movimentação WASD
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
            if (terraformMode == TerraformMode::Elevation)
            {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                {
                    float deltaH = isCtrlDown ? -brushStrength * deltaTime : brushStrength * deltaTime;
                    terrain.ModifyHeightAt(physics, hitPoint.x, hitPoint.z, deltaH, brushRadius);
                    TerrainAligner::AlignAllInRadius(scene, terrain, physics, hitPoint.x, hitPoint.z, brushRadius);
                }
            }
            else // TexturePaint
            {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                {
                    terrain.PaintTextureAt(hitPoint.x, hitPoint.z, selectedTextureIdx, paintStrength * deltaTime, brushRadius);
                }
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
                    const auto& itInfo = g_AvailableItems[selectedPickupIdx];
                    Item item;
                    if (itInfo.type == ItemType::Weapon)
                    {
                        if (strstr(itInfo.name, "Machado")) item = Items::CreateAxe();
                        else item = Items::CreateSword();
                    }
                    else if (itInfo.type == ItemType::Armor)
                    {
                        if (strstr(itInfo.name, "Ranger")) item = Items::CreateRangerArmor();
                        else item = Items::CreatePeasantOutfit();
                    }
                    else
                    {
                        item = Items::CreateHealthPotion();
                    }

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

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // 1. Sidebar Panel
    float panelX = 12.0f;
    float panelY = 12.0f;
    float panelW = 350.0f;
    float panelH = screenH - 24.0f;

    Rectangle panelRect = { panelX, panelY, panelW, panelH };
    ui.DrawPanel(panelRect, (Color){20, 16, 26, 245});
    ui.DrawHeader((Rectangle){ panelX, panelY, panelW, 40.0f }, "[ LEVEL EDITOR ]");

    // 2. Header
    float headerY = panelY + 48.0f;
    DrawText("Ferramentas de Construção", panelX + 16.0f, headerY, 14, (Color){220, 200, 150, 255});

    ui.DrawDivider(panelX + 16.0f, headerY + 22.0f, panelW - 32.0f);

    // 3. Tab Buttons
    float tabW = (panelW - 38.0f) / 2.0f;
    float tabH = 36.0f;
    float tabX1 = panelX + 16.0f;
    float tabX2 = tabX1 + tabW + 6.0f;
    float tabY1 = headerY + 32.0f;
    float tabY2 = tabY1 + tabH + 6.0f;

    Rectangle tabTerraform = { tabX1, tabY1, tabW, tabH };
    if (ui.DrawTabButton(tabTerraform, "1. TERRENO", currentTool == EditorTool::Terraform))
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

    ui.DrawDivider(panelX + 16.0f, panelY + 162.0f, panelW - 32.0f);

    // 4. Conteúdo da Ferramenta Selecionada
    float contentY = panelY + 172.0f;
    float footerHeight = 100.0f;
    float maxContentH = (panelY + panelH - footerHeight) - contentY;

    if (currentTool == EditorTool::Terraform)
    {
        DrawText("[ TERRAFORMAGEM & PINTURA ]", panelX + 18.0f, contentY, 17, (Color){255, 225, 120, 255});

        // Sub-tabs de modo: [ ELEVAÇÃO ] e [ PINTURA ]
        float subTabW = (panelW - 38.0f) / 2.0f;
        float subTabH = 26.0f;
        Rectangle tabElev = { panelX + 16.0f, contentY + 26.0f, subTabW, subTabH };
        Rectangle tabPaint = { panelX + 16.0f + subTabW + 6.0f, contentY + 26.0f, subTabW, subTabH };

        if (ui.DrawTabButton(tabElev, "ELEVAÇÃO", terraformMode == TerraformMode::Elevation))
        {
            terraformMode = TerraformMode::Elevation;
        }
        if (ui.DrawTabButton(tabPaint, "PINTURA TEXTURA", terraformMode == TerraformMode::TexturePaint))
        {
            terraformMode = TerraformMode::TexturePaint;
        }

        if (terraformMode == TerraformMode::Elevation)
        {
            Rectangle sliderR1 = { panelX + 16.0f, contentY + 62.0f, panelW - 32.0f, 34.0f };
            ui.DrawSlider(sliderR1, "Raio Brush", brushRadius, 1.0f, 30.0f);

            Rectangle sliderR2 = { panelX + 16.0f, contentY + 104.0f, panelW - 32.0f, 34.0f };
            ui.DrawSlider(sliderR2, "Força", brushStrength, 1.0f, 25.0f);

            Rectangle instrRect = { panelX + 16.0f, contentY + 154.0f, panelW - 32.0f, 90.0f };
            DrawRectangleRounded(instrRect, 0.08f, 4, (Color){15, 12, 20, 230});
            ui.DrawBorder(instrRect, (Color){140, 110, 50, 200}, 1.5f);

            DrawText("INSTRUÇÕES:", instrRect.x + 12.0f, instrRect.y + 8.0f, 14, (Color){255, 220, 100, 255});
            DrawText("• Clique Esq: Subir terreno", instrRect.x + 12.0f, instrRect.y + 28.0f, 13, (Color){220, 210, 190, 255});
            DrawText("• Ctrl + Clique: Abaixar terreno", instrRect.x + 12.0f, instrRect.y + 46.0f, 13, (Color){220, 210, 190, 255});
            DrawText("• Scroll do Mouse: Ajustar raio", instrRect.x + 12.0f, instrRect.y + 64.0f, 13, (Color){220, 210, 190, 255});
        }
        else // TerraformMode::TexturePaint
        {
            // Seleção de textura ativa para pintura
            float btnW = (panelW - 42.0f) / 2.0f;
            float btnH = 26.0f;
            const char* texNames[4] = { "1. Grama", "2. Terra", "3. Areia", "4. Pavimento" };
            for (int t = 0; t < 4; t++)
            {
                float bx = panelX + 16.0f + (t % 2) * (btnW + 10.0f);
                float by = contentY + 58.0f + (t / 2) * (btnH + 6.0f);
                Rectangle bRect = { bx, by, btnW, btnH };
                if (ui.DrawTabButton(bRect, texNames[t], selectedTextureIdx == t))
                {
                    selectedTextureIdx = t;
                }
            }

            // Sliders para Escalas UV de cada textura
            float uvY = contentY + 124.0f;
            Rectangle sG = { panelX + 16.0f, uvY + 0.0f, panelW - 32.0f, 30.0f };
            ui.DrawSlider(sG, "Escala Grama", terrain.uvScaleGrass, 1.0f, 80.0f);

            Rectangle sD = { panelX + 16.0f, uvY + 34.0f, panelW - 32.0f, 30.0f };
            ui.DrawSlider(sD, "Escala Terra", terrain.uvScaleDirt, 1.0f, 80.0f);

            Rectangle sS = { panelX + 16.0f, uvY + 68.0f, panelW - 32.0f, 30.0f };
            ui.DrawSlider(sS, "Escala Areia", terrain.uvScaleSand, 1.0f, 80.0f);

            Rectangle sP = { panelX + 16.0f, uvY + 102.0f, panelW - 32.0f, 30.0f };
            ui.DrawSlider(sP, "Escala Pavimento", terrain.uvScalePavement, 1.0f, 80.0f);

            // Raio e Força de Pintura
            float brushY = uvY + 138.0f;
            Rectangle sRadius = { panelX + 16.0f, brushY + 0.0f, panelW - 32.0f, 30.0f };
            ui.DrawSlider(sRadius, "Raio Brush", brushRadius, 1.0f, 30.0f);

            Rectangle sStrength = { panelX + 16.0f, brushY + 34.0f, panelW - 32.0f, 30.0f };
            ui.DrawSlider(sStrength, "Opacidade / Força", paintStrength, 0.2f, 10.0f);
        }
    }
    else if (currentTool == EditorTool::Props)
    {
        DrawText("[ PROPS DO CENÁRIO ]", panelX + 18.0f, contentY, 18, (Color){255, 225, 120, 255});

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

        float lvlSliderY = listRect.y + listRect.height + 8.0f;
        Rectangle lvlSliderRect = { panelX + 16.0f, lvlSliderY, panelW - 32.0f, 36.0f };
        float lvlFloat = (float)selectedEnemyLevel;
        ui.DrawSlider(lvlSliderRect, "Nível Inimigo", lvlFloat, 1.0f, 50.0f);
        selectedEnemyLevel = (int)lvlFloat;

        float resetBtnY = lvlSliderY + 42.0f;
        Rectangle resetBtn = { panelX + 16.0f, resetBtnY, panelW - 32.0f, 32.0f };
        if (ui.DrawButton(resetBtn, "RESETAR INIMIGOS"))
        {
            ResetEnemies(scene);
        }

        float instrY = resetBtnY + 38.0f;
        Rectangle instrRect = { panelX + 16.0f, instrY, panelW - 32.0f, 74.0f };
        DrawRectangleRounded(instrRect, 0.08f, 4, (Color){15, 12, 20, 230});
        ui.DrawBorder(instrRect, (Color){140, 110, 50, 200}, 1.5f);

        DrawText("INSTRUÇÕES:", instrRect.x + 12.0f, instrRect.y + 8.0f, 15, (Color){255, 220, 100, 255});
        DrawText("• Clique Esq: Posicionar inimigo", instrRect.x + 12.0f, instrRect.y + 28.0f, 14, (Color){220, 210, 190, 255});
        DrawText("• Ctrl + Clique: Deletar inimigo", instrRect.x + 12.0f, instrRect.y + 48.0f, 14, (Color){220, 210, 190, 255});
    }
    else if (currentTool == EditorTool::Pickups)
    {
        DrawText("[ COLETÁVEIS ]", panelX + 18.0f, contentY, 18, (Color){255, 225, 120, 255});

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

        float instrY = listRect.y + listRect.height + 10.0f;
        Rectangle instrRect = { panelX + 16.0f, instrY, panelW - 32.0f, 74.0f };
        DrawRectangleRounded(instrRect, 0.08f, 4, (Color){15, 12, 20, 230});
        ui.DrawBorder(instrRect, (Color){140, 110, 50, 200}, 1.5f);

        DrawText("INSTRUÇÕES:", instrRect.x + 12.0f, instrRect.y + 8.0f, 15, (Color){255, 220, 100, 255});
        DrawText("• Clique Esq: Posicionar item", instrRect.x + 12.0f, instrRect.y + 28.0f, 14, (Color){220, 210, 190, 255});
        DrawText("• Ctrl + Clique: Deletar item", instrRect.x + 12.0f, instrRect.y + 48.0f, 14, (Color){220, 210, 190, 255});
    }

    // 5. Footer com Botões Salvar e Carregar Level
    float footerY = panelY + panelH - 96.0f;
    ui.DrawDivider(panelX + 16.0f, footerY - 8.0f, panelW - 32.0f);

    Rectangle saveBtn = { panelX + 16.0f, footerY, panelW - 32.0f, 32.0f };
    if (ui.DrawButton(saveBtn, "SALVAR LEVEL (.lvl)"))
    {
        SaveLevel(scene, terrain, "Game/Assets/Levels/Default.lvl");
    }

    Rectangle loadBtn = { panelX + 16.0f, footerY + 40.0f, panelW - 32.0f, 32.0f };
    if (ui.DrawButton(loadBtn, "RECARREGAR SALVO (.lvl)"))
    {
        LoadLevel(scene, terrain, physics, "Game/Assets/Levels/Default.lvl");
    }
}

void LevelEditor::Draw3DGizmos(const Scene& scene, const TerrainObject& terrain)
{
    if (!isOpen) return;

    Vector2 mousePos = GetMousePosition();
    bool isOverSidebar = (mousePos.x <= 370.0f);

    // Renderiza Brush de Terraformagem ou Preview de Spawn
    if (!isOverSidebar && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Ray mouseRay = GetScreenToWorldRay(mousePos, editorCamera);
        Vector3 hitPoint = GetGroundRaycastHit(mouseRay, (TerrainObject&)terrain);

        if (currentTool == EditorTool::Terraform)
        {
            if (terraformMode == TerraformMode::Elevation)
            {
                DrawCircle3D((Vector3){ hitPoint.x, hitPoint.y + 0.1f, hitPoint.z }, brushRadius, (Vector3){ 1.0f, 0.0f, 0.0f }, 90.0f, (Color){ 255, 100, 100, 180 });
            }
            else // TexturePaint
            {
                Color brushColor = (selectedTextureIdx == 0) ? (Color){ 60, 220, 60, 200 } :
                                   (selectedTextureIdx == 1) ? (Color){ 160, 110, 50, 200 } :
                                   (selectedTextureIdx == 2) ? (Color){ 240, 220, 90, 200 } :
                                                               (Color){ 180, 180, 200, 200 };
                DrawCircle3D((Vector3){ hitPoint.x, hitPoint.y + 0.1f, hitPoint.z }, brushRadius, (Vector3){ 1.0f, 0.0f, 0.0f }, 90.0f, brushColor);
            }
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
    // Garante que as pastas existam
    mkdir("Game", 0755);
    mkdir("Game/Assets", 0755);
    mkdir("Game/Assets/Levels", 0755);
    mkdir("Assets", 0755);
    mkdir("Assets/Levels", 0755);
    mkdir("Build", 0755);
    mkdir("Build/Development", 0755);
    mkdir("Build/Development/Assets", 0755);
    mkdir("Build/Development/Assets/Levels", 0755);
    mkdir("Saved", 0755);

    std::vector<std::string> targetPaths = {
        "Game/Assets/Levels/Default.lvl",
        "Assets/Levels/Default.lvl",
        "Build/Development/Assets/Levels/Default.lvl",
        "Saved/level_data.json"
    };

    if (!savedDir.empty())
    {
        if (savedDir.find(".lvl") != std::string::npos || savedDir.find(".json") != std::string::npos)
            targetPaths.push_back(savedDir);
        else
            targetPaths.push_back(savedDir + "/Default.lvl");
    }

    std::stringstream f;
    f << "{\n";
    f << "  \"heights\": [";
    for (size_t i = 0; i < terrain.heightData.size(); i++)
    {
        f << terrain.heightData[i];
        if (i + 1 < terrain.heightData.size()) f << ", ";
    }
    f << "],\n";

    // Escalas de UV do Terreno
    f << "  \"uvScales\": [ "
      << terrain.uvScaleGrass << ", "
      << terrain.uvScaleDirt << ", "
      << terrain.uvScaleSand << ", "
      << terrain.uvScalePavement << " ],\n";

    // Stencil / Splatmap em Base64
    if (!terrain.splatmapData.empty())
    {
        f << "  \"splatmap\": \"" << Base64Encode(terrain.splatmapData.data(), terrain.splatmapData.size()) << "\",\n";
    }

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
    f << "\n  ],\n";

    f << "  \"pickups\": [\n";
    bool firstPickup = true;
    for (auto* obj : scene.objects)
    {
        if (auto* pickup = dynamic_cast<PickupObject*>(obj))
        {
            if (!firstPickup) f << ",\n";
            firstPickup = false;
            f << "    { \"name\": \"" << pickup->item.name << "\", \"type\": " << (int)pickup->item.type
              << ", \"x\": " << pickup->position.x << ", \"y\": " << pickup->position.y << ", \"z\": " << pickup->position.z << " }";
        }
    }
    f << "\n  ]\n";
    f << "}\n";

    std::string jsonStr = f.str();
    bool anySaved = false;

    for (const auto& path : targetPaths)
    {
        std::ofstream out(path);
        if (out.is_open())
        {
            out << jsonStr;
            out.close();
            anySaved = true;
            TraceLog(LOG_INFO, "[LevelEditor] Level salvo com sucesso em %s", path.c_str());
        }
    }

    return anySaved;
}

bool LevelEditor::LoadLevel(Scene& scene, TerrainObject& terrain, Physics& physics, const std::string& savedDir)
{
    const char* candidatePaths[] = {
        savedDir.c_str(),
        "Game/Assets/Levels/Default.lvl",
        "Assets/Levels/Default.lvl",
        "Build/Development/Assets/Levels/Default.lvl",
        "Saved/Default.lvl",
        "Saved/level_data.json",
        "Build/Development/Saved/level_data.json"
    };

    std::string filePath = "";
    for (const char* path : candidatePaths)
    {
        if (path && path[0] != '\0' && FileExists(path))
        {
            filePath = path;
            break;
        }
    }

    if (filePath.empty()) return false;

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

    // 2. Carrega UV Scales do terreno
    size_t uvPos = content.find("\"uvScales\": [");
    if (uvPos != std::string::npos)
    {
        size_t start = uvPos + 13;
        size_t end = content.find("]", start);
        if (end != std::string::npos)
        {
            std::string uvStr = content.substr(start, end - start);
            std::stringstream ss(uvStr);
            float g = 25.0f, d = 25.0f, s = 25.0f, p = 25.0f;
            char comma;
            if (ss >> g >> comma >> d >> comma >> s >> comma >> p)
            {
                terrain.uvScaleGrass = g;
                terrain.uvScaleDirt = d;
                terrain.uvScaleSand = s;
                terrain.uvScalePavement = p;
            }
        }
    }

    // 3. Carrega Stencil / Splatmap
    size_t splatPos = content.find("\"splatmap\": \"");
    if (splatPos != std::string::npos)
    {
        size_t start = splatPos + 13;
        size_t end = content.find("\"", start);
        if (end != std::string::npos)
        {
            std::string b64Str = content.substr(start, end - start);
            std::vector<uint8_t> decodedSplat = Base64Decode(b64Str);
            if (decodedSplat.size() == (size_t)(TerrainObject::SPLATMAP_RES * TerrainObject::SPLATMAP_RES * 4))
            {
                terrain.SetSplatmapData(decodedSplat);
            }
        }
    }

    // 4. Remove objetos dinâmicos existentes (props, inimigos e coletáveis)
    for (auto it = scene.objects.begin(); it != scene.objects.end(); )
    {
        GameObject* obj = *it;
        if (dynamic_cast<NatureObject*>(obj) || dynamic_cast<EnemyObject*>(obj) || dynamic_cast<PickupObject*>(obj))
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

    // 5. Re-instancia Props
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

    // 6. Re-instancia Inimigos
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

    // 7. Re-instancia Coletáveis
    size_t pickupsPos = content.find("\"pickups\": [");
    if (pickupsPos != std::string::npos)
    {
        size_t endPickups = content.find("]", pickupsPos);
        std::string pickupsBlock = content.substr(pickupsPos, endPickups - pickupsPos);

        size_t p = 0;
        while ((p = pickupsBlock.find("{", p)) != std::string::npos)
        {
            size_t pEnd = pickupsBlock.find("}", p);
            if (pEnd == std::string::npos) break;
            std::string itemStr = pickupsBlock.substr(p, pEnd - p);

            char nameBuf[64] = {0};
            int typeVal = 0;
            float x = 0, y = 0, z = 0;

            if (sscanf(itemStr.c_str(), " { \"name\": \"%63[^\"]\", \"type\": %d, \"x\": %f, \"y\": %f, \"z\": %f", nameBuf, &typeVal, &x, &y, &z) >= 5)
            {
                Item item;
                if (strstr(nameBuf, "Machado")) item = Items::CreateAxe();
                else if (strstr(nameBuf, "Pocao") || strstr(nameBuf, "Poção")) item = Items::CreateHealthPotion();
                else if (strstr(nameBuf, "Ranger")) item = Items::CreateRangerArmor();
                else if (strstr(nameBuf, "Campones") || strstr(nameBuf, "Camponês")) item = Items::CreatePeasantOutfit();
                else item = Items::CreateSword();

                auto* pickup = new PickupObject(item, (Vector3){ x, y, z });
                pickup->Init(physics);
                scene.objects.push_back(pickup);
            }
            p = pEnd + 1;
        }
    }

    TerrainAligner::AlignAll(scene, terrain, physics);
    physics.system.OptimizeBroadPhase();

    TraceLog(LOG_INFO, "[LevelEditor] Level carregado com sucesso a partir de %s", filePath.c_str());
    return true;
}
