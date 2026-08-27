import os
import shutil


def cook_levels(source_dir, output_dir):
    """
    Copia arquivos de fases (.lvl) da pasta Game/Assets/Levels para a pasta Assets/Levels da build.
    """
    src_levels_dir = os.path.join(source_dir, "Levels")
    out_levels_dir = os.path.join(output_dir, "Levels")
    os.makedirs(out_levels_dir, exist_ok=True)

    if not os.path.exists(src_levels_dir):
        os.makedirs(src_levels_dir, exist_ok=True)
        return 0, 0

    processed = 0
    skipped = 0

    for filename in os.listdir(src_levels_dir):
        if filename.endswith(".lvl") or filename.endswith(".json"):
            src_file = os.path.join(src_levels_dir, filename)
            dst_file = os.path.join(out_levels_dir, filename)

            if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                shutil.copy2(src_file, dst_file)
                processed += 1
            else:
                skipped += 1

    return processed, skipped
