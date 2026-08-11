#include "Settings.h"

void Settings::Init()
{
    SetMasterVolume(masterVolume);
}

void Settings::Apply()
{
    SetMasterVolume(masterVolume);

    if (IsWindowReady())
    {
        if (isFullscreen != IsWindowFullscreen())
        {
            ToggleFullscreen();
        }

        if (!isFullscreen)
        {
            int targetW = 1280;
            int targetH = 720;
            if (resolutionIndex == 1) { targetW = 1600; targetH = 900; }
            else if (resolutionIndex == 2) { targetW = 1920; targetH = 1080; }

            if (GetScreenWidth() != targetW || GetScreenHeight() != targetH)
            {
                SetWindowSize(targetW, targetH);
            }
        }
    }
}
