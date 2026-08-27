# Package cookers - Ferramentas de conversão e preparação de assets para Lords of Nightfall

import sys
import subprocess

# Garante que dependências básicas de python estejam instaladas
try:
    import assimp_py
    import numpy as np
except ImportError:
    print("[AssetCooker] Instalando dependências (assimp-py, numpy)...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "--break-system-packages", "assimp-py", "numpy"])
