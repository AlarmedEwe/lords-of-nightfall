import os
from .qoi_encoder import encode_image_to_lontex

def cook_terrain_assets(source_dir, output_dir):
    """
    Converte as texturas de terreno brutas (.png) para o formato binário proprietário (.lontex / QOI)
    com verificação incremental de timestamp.
    """
    src_terrain_dir = os.path.join(source_dir, "Terrain")
    out_terrain_dir = os.path.join(output_dir, "Terrain")
    os.makedirs(out_terrain_dir, exist_ok=True)

    if not os.path.exists(src_terrain_dir):
        return 0, 0

    processed = 0
    skipped = 0

    for filename in os.listdir(src_terrain_dir):
        if filename.lower().endswith((".png", ".jpg", ".jpeg", ".bmp")):
            src_file = os.path.join(src_terrain_dir, filename)
            base_name = os.path.splitext(filename)[0]
            dst_file = os.path.join(out_terrain_dir, base_name + ".lontex")

            if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                if encode_image_to_lontex(src_file, dst_file):
                    print(f"[AssetCooker] Textura de terreno compilada: {dst_file}")
                    processed += 1
                else:
                    print(f"[AssetCooker] ERRO ao compilar textura de terreno: {src_file}")
            else:
                skipped += 1

    return processed, skipped
