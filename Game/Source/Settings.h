#ifndef SETTINGS_H
#define SETTINGS_H

#include "raylib.h"

enum class ShadowQuality
{
    Off = 0,
    Low = 1,
    Medium = 2,
    High = 3
};

struct Settings
{
    bool isFullscreen = false;
    ShadowQuality shadowQuality = ShadowQuality::High;
    int resolutionIndex = 0; // 0: 1280x720, 1: 1600x900, 2: 1920x1080
    float mouseSensitivity = 1.0f; // 0.1f a 2.0f
    float masterVolume = 0.8f;     // 0.0f a 1.0f

    void Init();
    void Apply();
};

#endif // SETTINGS_H
