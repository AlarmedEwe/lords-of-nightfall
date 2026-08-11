#!/usr/bin/env python3
"""
AssetCooker - Converte assets brutos (FBX, PNG, WAV) em formatos binários/otimizados do motor.
Implementa verificação de timestamp (incremental build) para copiar/processar
apenas os arquivos modificados.
"""

import sys
import os
import shutil

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

    ui_double_dir = os.path.join(source_dir, "kenney_fantasy-ui-borders", "PNG", "Double")
    ui_out_dir = os.path.join(output_dir, "UI")

    processed_count = 0
    skipped_count = 0

    if os.path.exists(ui_double_dir):
        for root, dirs, files in os.walk(ui_double_dir):
            for file in files:
                if not file.lower().endswith(('.png', '.jpg', '.jpeg')):
                    continue

                src_file = os.path.join(root, file)
                rel_path = os.path.relpath(src_file, ui_double_dir)
                dst_file = os.path.join(ui_out_dir, rel_path)

                os.makedirs(os.path.dirname(dst_file), exist_ok=True)

                # Incremental build check
                if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                    shutil.copy2(src_file, dst_file)
                    processed_count += 1
                else:
                    skipped_count += 1

    print(f"[AssetCooker] Assets processados: {processed_count}, Up-to-date: {skipped_count}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
