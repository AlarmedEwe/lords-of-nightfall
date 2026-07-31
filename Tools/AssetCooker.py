#!/usr/bin/env python3
"""
AssetCooker - Converte assets brutos (FBX, PNG, WAV) em formatos binários do motor.
Implementa verificação de timestamp para incremental build.

Por enquanto é um placeholder que não faz nada.
"""

import sys
import os


def main():
    source_dir = os.path.join(os.path.dirname(__file__), "..", "Game", "Assets")
    output_dir = sys.argv[1] if len(sys.argv) > 1 else None

    if output_dir is None:
        print("[AssetCooker] Nenhum diretório de saída especificado. Nada a fazer.")
        return 0

    os.makedirs(output_dir, exist_ok=True)
    print(f"[AssetCooker] Source: {os.path.abspath(source_dir)}")
    print(f"[AssetCooker] Output: {os.path.abspath(output_dir)}")
    print("[AssetCooker] Nenhum asset para processar (placeholder).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
