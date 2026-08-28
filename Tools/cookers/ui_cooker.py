import os
from .qoi_encoder import encode_image_to_lontex

def cook_ui_assets(source_dir, output_dir):
    """
    Converte os assets brutos de UI (PNGs) para o formato binário proprietário (.lontex / QOI)
    com verificação incremental de timestamp.
    """
    ui_src = os.path.join(source_dir, "kenney_fantasy-ui-borders")
    ui_out = os.path.join(output_dir, "UI")
    os.makedirs(ui_out, exist_ok=True)

    processed_count = 0
    skipped_count = 0

    if os.path.exists(ui_src):
        for root, _, files in os.walk(ui_src):
            for file in files:
                if file.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp')) and ('@2x' in file or 'double' in root.lower() or 'PNG' in root):
                    src_file = os.path.join(root, file)
                    rel_path = os.path.relpath(src_file, ui_src)
                    base_rel, _ = os.path.splitext(rel_path)
                    dst_file = os.path.join(ui_out, base_rel + ".lontex")

                    os.makedirs(os.path.dirname(dst_file), exist_ok=True)

                    if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                        if encode_image_to_lontex(src_file, dst_file):
                            processed_count += 1
                        else:
                            print(f"[AssetCooker] ERRO ao compilar asset UI: {src_file}")
                    else:
                        skipped_count += 1

    return processed_count, skipped_count
