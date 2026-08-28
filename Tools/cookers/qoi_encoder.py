import os
import ctypes
import subprocess

_lib_path = os.path.join(os.path.dirname(__file__), "libqoi_encoder.so")
_c_src_path = os.path.join(os.path.dirname(__file__), "qoi_encoder.c")
_lib = None

def _ensure_lib():
    global _lib
    if _lib is not None:
        return _lib

    if not os.path.exists(_lib_path) and os.path.exists(_c_src_path):
        try:
            subprocess.run([
                "gcc", "-O3", "-shared", "-fPIC",
                _c_src_path, "-o", _lib_path, "-lm"
            ], check=True, capture_output=True)
        except Exception as e:
            print(f"[QOIEncoder] Falha ao compilar libqoi_encoder.so: {e}")
            return None

    if os.path.exists(_lib_path):
        try:
            _lib = ctypes.CDLL(_lib_path)
            _lib.encode_image_to_lontex.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
            _lib.encode_image_to_lontex.restype = ctypes.c_int
        except Exception as e:
            print(f"[QOIEncoder] Falha ao carregar libqoi_encoder.so: {e}")
            _lib = None

    return _lib

def encode_image_to_lontex(input_image_path, output_lontex_path):
    """
    Converte qualquer arquivo de imagem (.png, .jpg, .bmp) para o formato binário .lontex (QOI).
    Retorna True se a conversão foi bem-sucedida, False caso contrário.
    """
    lib = _ensure_lib()
    if lib is None:
        return False

    os.makedirs(os.path.dirname(output_lontex_path), exist_ok=True)
    res = lib.encode_image_to_lontex(
        input_image_path.encode('utf-8'),
        output_lontex_path.encode('utf-8')
    )
    return res == 1
