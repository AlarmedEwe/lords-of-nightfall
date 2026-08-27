import os
import shutil


def cook_terrain_assets(source_dir, output_dir):
    """
    Copia as texturas do terreno (grass, dirt, sand, pavement)
    para o diretório de assets da build.
    """
    src_terrain_dir = os.path.join(source_dir, "Terrain")
    out_terrain_dir = os.path.join(output_dir, "Terrain")
    os.makedirs(out_terrain_dir, exist_ok=True)

    if not os.path.exists(src_terrain_dir):
        return 0, 0

    processed = 0
    skipped = 0

    for filename in os.listdir(src_terrain_dir):
        if filename.lower().endswith(".png") or filename.lower().endswith(".jpg"):
            src_file = os.path.join(src_terrain_dir, filename)
            dst_file = os.path.join(out_terrain_dir, filename)

            if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                shutil.copy2(src_file, dst_file)
                processed += 1
            else:
                skipped += 1

    return processed, skipped
