import os
import shutil


def cook_shaders(source_dir, output_dir):
    shaders_src = os.path.join(source_dir, "Shaders")
    shaders_out = os.path.join(output_dir, "Shaders")
    os.makedirs(shaders_out, exist_ok=True)

    processed_count = 0
    skipped_count = 0

    if os.path.exists(shaders_src):
        for f in os.listdir(shaders_src):
            if f.endswith('.vs') or f.endswith('.fs'):
                src_file = os.path.join(shaders_src, f)
                dst_file = os.path.join(shaders_out, f)

                if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                    shutil.copy2(src_file, dst_file)
                    processed_count += 1
                else:
                    skipped_count += 1

    return processed_count, skipped_count
