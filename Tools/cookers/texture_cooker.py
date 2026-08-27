import os
import shutil
import struct
import zlib


def create_monster_atlas(filepath):
    width = 256
    height = 256

    palette_grid = [
        [(220, 50, 40), (240, 90, 30), (200, 30, 30), (180, 20, 20)],
        [(170, 25, 45), (150, 15, 35), (190, 40, 60), (210, 60, 70)],
        [(130, 40, 180), (100, 20, 150), (160, 60, 210), (90, 30, 120)],
        [(40, 80, 200), (30, 60, 170), (50, 100, 220), (20, 40, 130)],
        [(50, 180, 220), (30, 150, 190), (70, 210, 240), (40, 130, 160)],
        [(40, 140, 50), (30, 110, 40), (60, 160, 70), (25, 90, 30)],
        [(80, 210, 60), (110, 230, 70), (60, 180, 50), (130, 240, 90)],
        [(160, 210, 50), (140, 190, 40), (180, 230, 70), (120, 170, 30)],
        [(240, 200, 40), (220, 180, 30), (255, 220, 60), (200, 160, 20)],
        [(230, 130, 30), (210, 110, 20), (250, 150, 50), (190, 90, 10)],
        [(130, 80, 40), (100, 60, 30), (160, 100, 50), (80, 50, 20)],
        [(60, 65, 75), (40, 45, 50), (80, 85, 95), (30, 30, 35)],
        [(180, 30, 40), (40, 30, 35), (210, 40, 50), (25, 25, 30)],
        [(210, 190, 170), (180, 160, 140), (230, 210, 190), (160, 140, 120)],
        [(190, 195, 205), (160, 165, 175), (220, 225, 235), (130, 135, 145)],
        [(245, 245, 250), (255, 215, 0), (230, 230, 240), (255, 255, 255)]
    ]

    raw_data = bytearray()
    for y in range(height):
        raw_data.append(0)
        row_idx = (height - 1 - y) * 16 // height
        row_colors = palette_grid[min(15, row_idx)]
        for x in range(width):
            col_idx = (x * len(row_colors)) // width
            r, g, b = row_colors[min(len(row_colors) - 1, col_idx)]
            raw_data.extend([r, g, b])

    compressed = zlib.compress(raw_data)

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)

    png_bytes = b'\x89PNG\r\n\x1a\n' + \
                chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) + \
                chunk(b'IDAT', compressed) + \
                chunk(b'IEND', b'')

    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'wb') as f:
        f.write(png_bytes)


def cook_character_textures(source_dir, output_dir):
    tex_out_dir = os.path.join(output_dir, "Textures")
    os.makedirs(tex_out_dir, exist_ok=True)

    # 1. Texturas do Personagem Base
    base_tex_dir = os.path.join(source_dir, "Universal Base Characters", "Base Characters", "Textures")
    if os.path.exists(base_tex_dir):
        for root, _, files in os.walk(base_tex_dir):
            for f in files:
                if f.lower().endswith('.png'):
                    src = os.path.join(root, f)
                    dst = os.path.join(tex_out_dir, f)
                    if not os.path.exists(dst) or os.path.getmtime(src) > os.path.getmtime(dst):
                        shutil.copy2(src, dst)

    # 2. Texturas das Roupas Modular Outfits
    outfits_tex_dir = os.path.join(source_dir, "Modular Character Outfits - Fantasy", "Textures")
    if os.path.exists(outfits_tex_dir):
        for root, _, files in os.walk(outfits_tex_dir):
            for f in files:
                if f.lower().endswith('.png'):
                    src = os.path.join(root, f)
                    dst = os.path.join(tex_out_dir, f)
                    if not os.path.exists(dst) or os.path.getmtime(src) > os.path.getmtime(dst):
                        shutil.copy2(src, dst)

    # 3. Atlas de cores para os monstros
    atlas_path = os.path.join(tex_out_dir, "Atlas_Monsters.png")
    if not os.path.exists(atlas_path):
        create_monster_atlas(atlas_path)
