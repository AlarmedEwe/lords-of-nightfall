#ifndef LON_TEXTURE_LOADER_H
#define LON_TEXTURE_LOADER_H

#include "raylib.h"
#include <string>

namespace TextureLoader
{
    // Carrega uma textura no formato proprietário .lontex (QOI) com fallback para imagem bruta
    Texture2D Load(const std::string& relPath, bool generateMipmaps = true, bool wrapRepeat = true);

    // Descarrega uma textura da VRAM com segurança
    void Unload(Texture2D& texture);
}

#endif // LON_TEXTURE_LOADER_H
