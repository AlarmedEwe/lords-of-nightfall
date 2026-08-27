# Script de Asset Cooking para Lords of Nightfall
# Orquestrador CLI de conversão e preparação de assets para a build final

import os
import sys
import site

# Garante inclusão de site-packages do usuário (ex: /var/data/python/lib/python3.13/site-packages)
user_site = site.getusersitepackages()
if user_site and user_site not in sys.path and os.path.exists(user_site):
    sys.path.append(user_site)
for extra in ["/var/data/python/lib/python3.13/site-packages", "/var/data/python/lib/python3.12/site-packages"]:
    if os.path.exists(extra) and extra not in sys.path:
        sys.path.append(extra)

from cookers.shaders_cooker import cook_shaders
from cookers.ui_cooker import cook_ui_assets
from cookers.texture_cooker import cook_character_textures
from cookers.character_cooker import cook_3d_models
from cookers.monster_cooker import cook_monsters
from cookers.item_cooker import cook_rpg_items


def main():
    source_dir = os.path.join(os.path.dirname(__file__), "..", "Game", "Assets")
    output_dir = sys.argv[1] if len(sys.argv) > 1 else None

    if output_dir is None:
        print("[AssetCooker] Nenhum diretório de saída especificado. Nada a fazer.")
        return 0

    source_dir = os.path.abspath(source_dir)
    output_dir = os.path.abspath(output_dir)
    os.makedirs(output_dir, exist_ok=True)

    print(f"[AssetCooker] Source: {source_dir}")
    print(f"[AssetCooker] Output: {output_dir}")

    sh_proc, sh_skip = cook_shaders(source_dir, output_dir)
    ui_proc, ui_skip = cook_ui_assets(source_dir, output_dir)
    cook_character_textures(source_dir, output_dir)
    cook_3d_models(source_dir, output_dir)
    cook_monsters(source_dir, output_dir)
    cook_rpg_items(source_dir, output_dir)

    print(f"[AssetCooker] Shaders: {sh_proc} processados, {sh_skip} up-to-date")
    print(f"[AssetCooker] UI Assets: {ui_proc} processados, {ui_skip} up-to-date")
    return 0


if __name__ == "__main__":
    sys.exit(main())
