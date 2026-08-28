#include "TextureLoader.h"
#include "qoi.h"

#include <cstdio>
#include <vector>

namespace TextureLoader
{

Texture2D Load(const std::string& relPath, bool generateMipmaps, bool wrapRepeat)
{
    if (relPath.empty()) return (Texture2D){ 0 };

    // Extrai o nome base sem extensão para procurar .lontex prioritariamente
    std::string baseWithoutExt = relPath;
    size_t lastDot = baseWithoutExt.find_last_of('.');
    if (lastDot != std::string::npos)
    {
        baseWithoutExt = baseWithoutExt.substr(0, lastDot);
    }

    std::string lontexPath = baseWithoutExt + ".lontex";

    std::vector<std::string> candidates = {
        lontexPath,
        "Assets/" + lontexPath,
        "Assets/Terrain/" + lontexPath,
        "Assets/Textures/" + lontexPath,
        "Assets/UI/" + lontexPath,
        "Build/Development/Assets/" + lontexPath,
        "Build/Development/Assets/Terrain/" + lontexPath,
        "Build/Development/Assets/Textures/" + lontexPath,
        "Build/Development/Assets/UI/" + lontexPath,
        // Fallbacks com caminho original
        relPath,
        "Assets/" + relPath,
        "Assets/Terrain/" + relPath,
        "Assets/Textures/" + relPath,
        "Assets/UI/" + relPath,
        "Build/Development/Assets/" + relPath,
        "Game/Assets/" + relPath,
        "Game/Assets/Terrain/" + relPath,
        "Game/Assets/kenney_fantasy-ui-borders/PNG/Double/" + relPath,
        "Game/Assets/Universal Base Characters/Base Characters/Textures/" + relPath,
        "Game/Assets/Modular Character Outfits - Fantasy/Textures/" + relPath
    };

    for (const auto& path : candidates)
    {
        if (FileExists(path.c_str()))
        {
            if (path.size() >= 7 && path.substr(path.size() - 7) == ".lontex")
            {
                qoi_desc desc;
                void* pixels = qoi_read(path.c_str(), &desc, 4);
                if (pixels)
                {
                    Image img = {
                        .data = pixels,
                        .width = (int)desc.width,
                        .height = (int)desc.height,
                        .mipmaps = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
                    };
                    Texture2D tex = LoadTextureFromImage(img);
                    RL_FREE(pixels);

                    if (tex.id > 0)
                    {
                        if (wrapRepeat) SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
                        else SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);
                        SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
                        if (generateMipmaps) GenTextureMipmaps(&tex);

                        TraceLog(LOG_INFO, "[TextureLoader] Textura .lontex (QOI) carregada: %s (ID: %u, %dx%d)", path.c_str(), tex.id, tex.width, tex.height);
                        return tex;
                    }
                }
            }
            else
            {
                Texture2D tex = LoadTexture(path.c_str());
                if (tex.id > 0)
                {
                    if (wrapRepeat) SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
                    else SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);
                    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
                    if (generateMipmaps) GenTextureMipmaps(&tex);

                    TraceLog(LOG_INFO, "[TextureLoader] Textura carregada (fallback): %s (ID: %u, %dx%d)", path.c_str(), tex.id, tex.width, tex.height);
                    return tex;
                }
            }
        }
    }

    TraceLog(LOG_WARNING, "[TextureLoader] Nao foi possivel carregar a textura: %s", relPath.c_str());
    return (Texture2D){ 0 };
}

void Unload(Texture2D& texture)
{
    if (texture.id > 0)
    {
        UnloadTexture(texture);
        texture = (Texture2D){ 0 };
    }
}

} // namespace TextureLoader
