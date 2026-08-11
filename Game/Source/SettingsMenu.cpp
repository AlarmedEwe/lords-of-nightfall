#include "SettingsMenu.h"

void SettingsMenu::Draw(UISystem& ui, Settings& settings, int screenWidth, int screenHeight)
{
    if (!isOpen) return;

    // Fundo escurecido semi-transparente
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 160});

    // Janela modal centralizada
    float modalW = 560.0f;
    float modalH = 460.0f;
    float modalX = (screenWidth - modalW) / 2.0f;
    float modalY = (screenHeight - modalH) / 2.0f;

    Rectangle modalRect = { modalX, modalY, modalW, modalH };
    ui.DrawPanel(modalRect, (Color){20, 16, 28, 245});

    // Cabeçalho
    Rectangle headerRect = { modalX + 20, modalY + 20, modalW - 40, 44 };
    ui.DrawHeader(headerRect, "CONFIGURAÇÕES");

    // Abas: Gráficos | Gameplay | Áudio
    float tabW = (modalW - 50.0f) / 3.0f;
    Rectangle tabGraphics = { modalX + 20, modalY + 76, tabW, 36 };
    Rectangle tabGameplay = { modalX + 25 + tabW, modalY + 76, tabW, 36 };
    Rectangle tabAudio    = { modalX + 30 + tabW * 2, modalY + 76, tabW, 36 };

    if (ui.DrawTabButton(tabGraphics, "Gráficos", currentTab == SettingsTab::Graphics))
        currentTab = SettingsTab::Graphics;
    if (ui.DrawTabButton(tabGameplay, "Gameplay", currentTab == SettingsTab::Gameplay))
        currentTab = SettingsTab::Gameplay;
    if (ui.DrawTabButton(tabAudio, "Áudio", currentTab == SettingsTab::Audio))
        currentTab = SettingsTab::Audio;

    ui.DrawDivider(modalX + 20, modalY + 122, modalW - 40);

    // Conteúdo da Aba
    float contentY = modalY + 135.0f;
    float itemH = 42.0f;

    if (currentTab == SettingsTab::Graphics)
    {
        // 1. Tela Cheia
        Rectangle itemFull = { modalX + 30, contentY, modalW - 60, itemH };
        if (ui.DrawToggle(itemFull, "Tela Cheia", settings.isFullscreen))
        {
            settings.Apply();
        }

        // 2. Qualidade das Sombras
        Rectangle itemShadow = { modalX + 30, contentY + 50, modalW - 60, itemH };
        const char* shadowNames[] = { "Desativada", "Baixa", "Média", "Alta" };
        int currentShadowIdx = (int)settings.shadowQuality;
        if (ui.DrawOptionSelector(itemShadow, "Qualidade Sombras", shadowNames[currentShadowIdx]))
        {
            currentShadowIdx = (currentShadowIdx + 1) % 4;
            settings.shadowQuality = (ShadowQuality)currentShadowIdx;
            settings.Apply();
        }

        // 3. Resolução
        Rectangle itemRes = { modalX + 30, contentY + 100, modalW - 60, itemH };
        const char* resNames[] = { "1280x720 (HD)", "1600x900 (HD+)", "1920x1080 (FHD)" };
        if (ui.DrawOptionSelector(itemRes, "Resolução", resNames[settings.resolutionIndex]))
        {
            settings.resolutionIndex = (settings.resolutionIndex + 1) % 3;
            settings.Apply();
        }
    }
    else if (currentTab == SettingsTab::Gameplay)
    {
        // Sensibilidade do Mouse
        Rectangle itemSens = { modalX + 30, contentY + 10, modalW - 60, itemH };
        if (ui.DrawSlider(itemSens, "Sensibilidade Mouse", settings.mouseSensitivity, 0.1f, 3.0f))
        {
            settings.Apply();
        }
    }
    else if (currentTab == SettingsTab::Audio)
    {
        // Volume Principal
        Rectangle itemVol = { modalX + 30, contentY + 10, modalW - 60, itemH };
        if (ui.DrawSlider(itemVol, "Volume Principal", settings.masterVolume, 0.0f, 1.0f))
        {
            settings.Apply();
        }
    }

    ui.DrawDivider(modalX + 20, modalY + modalH - 70, modalW - 40);

    // Botão Voltar
    Rectangle backBtnRect = { modalX + (modalW - 160) / 2, modalY + modalH - 55, 160, 38 };
    if (ui.DrawButton(backBtnRect, "Voltar"))
    {
        isOpen = false;
    }
}
