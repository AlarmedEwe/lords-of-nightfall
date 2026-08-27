# Script de Asset Cooking para Lords of Nightfall
# Converte e prepara shaders, texturas, modelos FBX e UI assets para a build final via assimp_py

import os
import sys
import shutil
import struct
import math
import zlib
import subprocess

# Tenta importar assimp_py e numpy; se não estiverem instalados, instala automaticamente
try:
    import assimp_py
    import numpy as np
except ImportError:
    print("[AssetCooker] Instalando dependências (assimp-py, numpy)...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "--break-system-packages", "assimp-py", "numpy"])
    import assimp_py
    import numpy as np


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


def cook_ui_assets(source_dir, output_dir):
    ui_src = os.path.join(source_dir, "kenney_fantasy-ui-borders")
    ui_out = os.path.join(output_dir, "UI")
    os.makedirs(ui_out, exist_ok=True)

    processed_count = 0
    skipped_count = 0

    if os.path.exists(ui_src):
        for root, _, files in os.walk(ui_src):
            for file in files:
                if file.endswith('.png') and ('@2x' in file or 'double' in root.lower() or 'PNG' in root):
                    src_file = os.path.join(root, file)
                    rel_path = os.path.relpath(src_file, ui_src)
                    dst_file = os.path.join(ui_out, rel_path)

                    os.makedirs(os.path.dirname(dst_file), exist_ok=True)

                    if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                        shutil.copy2(src_file, dst_file)
                        processed_count += 1
                    else:
                        skipped_count += 1

    return processed_count, skipped_count


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


def build_lonmodel_binary(scene, out_model_path, anims_def, target_height=1.8):
    post_proc_meshes = []
    max_y_overall = 0.0

    for m in scene.meshes:
        verts = np.frombuffer(m.vertices, dtype=np.float32).reshape(-1, 3).copy()
        num_verts = len(verts)

        if m.normals:
            norms = np.frombuffer(m.normals, dtype=np.float32).reshape(-1, 3).copy()
        else:
            norms = np.zeros((num_verts, 3), dtype=np.float32)

        if m.texcoords and len(m.texcoords) > 0:
            uvs = np.frombuffer(m.texcoords[0], dtype=np.float32).reshape(-1, 2).copy()
            uvs[:, 1] = 1.0 - uvs[:, 1]
        else:
            uvs = np.zeros((num_verts, 2), dtype=np.float32)

        indices = np.frombuffer(m.indices, dtype=np.uint32).copy()
        num_indices = len(indices)

        if len(verts) > 0:
            h = verts[:, 1].max() - verts[:, 1].min()
            if h > max_y_overall:
                max_y_overall = h

        post_proc_meshes.append({
            'verts': verts,
            'norms': norms,
            'uvs': uvs,
            'indices': indices,
            'num_verts': num_verts,
            'num_indices': num_indices
        })

    # Normalização de escala se o modelo original for muito grande (ex: FBX de monstros com 300 unidades)
    scale_factor = 1.0
    if max_y_overall > 5.0:
        scale_factor = target_height / max_y_overall

    num_meshes = len(post_proc_meshes)
    num_anims = len(anims_def)

    with open(out_model_path, 'wb') as f:
        f.write(b'LONM')
        f.write(struct.pack('<III', 3, num_meshes, num_anims))

        # Cabeçalhos das animações e matrizes da mão direita
        for anim_name, fcount, fps in anims_def:
            name_bytes = anim_name.encode('utf-8')[:31].ljust(32, b'\x00')
            f.write(name_bytes)
            f.write(struct.pack('<If', fcount, fps))

            # Matriz padrão da mão direita para acoplamento de armas
            right_hand_matrix = [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.25, 0.95, 0.35, 1.0
            ]
            for fr in range(fcount):
                f.write(struct.pack('<16f', *right_hand_matrix))

        # Dados das submeshes e frames de vértices
        for mdata in post_proc_meshes:
            num_verts = mdata['num_verts']
            num_indices = mdata['num_indices']
            verts = mdata['verts'] * scale_factor
            norms = mdata['norms']
            uvs = mdata['uvs']
            indices = mdata['indices']

            # Se o modelo original estiver em T-Pose (braços abertos), ajusta a postura base para em pé natural (86deg)
            is_tpose = float(verts[:, 0].max() - verts[:, 0].min()) > 1.2 and float(verts[:, 1].max()) > 1.2
            if is_tpose:
                ang = math.radians(86.0)
                c, s = math.cos(ang), math.sin(ang)

                # Braço esquerdo (x > 0.22, y > 0.7)
                ml = (verts[:, 0] > 0.22) & (verts[:, 1] > 0.7)
                if np.any(ml):
                    dx = verts[ml, 0] - 0.22
                    dy = verts[ml, 1] - 1.42
                    verts[ml, 0] = 0.22 + (dx * c + dy * s)
                    verts[ml, 1] = 1.42 + (-dx * s + dy * c)

                # Braço direito (x < -0.22, y > 0.7)
                mr = (verts[:, 0] < -0.22) & (verts[:, 1] > 0.7)
                if np.any(mr):
                    dx = verts[mr, 0] - (-0.22)
                    dy = verts[mr, 1] - 1.42
                    verts[mr, 0] = -0.22 + (dx * c - dy * s)
                    verts[mr, 1] = 1.42 + (dx * s + dy * c)

            f.write(struct.pack('<II', num_verts, num_indices))
            f.write(norms.astype(np.float32).tobytes())
            f.write(uvs.astype(np.float32).tobytes())
            f.write(indices.astype(np.uint32).tobytes())

            # Geração procedural de animação de vértices por frame
            for anim_idx, (anim_name, fcount, fps) in enumerate(anims_def):
                for fr in range(fcount):
                    phase = (fr / fcount) * 2.0 * math.pi
                    frame_verts = verts.copy()

                    if anim_name in ['Idle', 'PatrolIdle']:
                        # Respiração suave em pé
                        frame_verts[:, 1] += math.sin(phase) * 0.015
                    elif anim_name in ['Walk', 'PatrolWalk']:
                        amp = 0.04
                        frame_verts[:, 1] += abs(math.sin(phase)) * amp
                        mask_legs = verts[:, 1] < 0.95
                        frame_verts[mask_legs, 2] += math.sin(phase) * (0.95 - verts[mask_legs, 1]) * 0.25
                        # Balanço de braços oposto às pernas
                        mask_r_arm = (verts[:, 0] < -0.18) & (verts[:, 1] > 0.5) & (verts[:, 1] < 1.45)
                        mask_l_arm = (verts[:, 0] > 0.18) & (verts[:, 1] > 0.5) & (verts[:, 1] < 1.45)
                        frame_verts[mask_r_arm, 2] += math.sin(phase) * 0.15
                        frame_verts[mask_l_arm, 2] -= math.sin(phase) * 0.15
                    elif anim_name in ['Run', 'Chase']:
                        amp = 0.08
                        frame_verts[:, 1] += abs(math.sin(phase * 2.0)) * amp
                        mask_legs = verts[:, 1] < 0.95
                        frame_verts[mask_legs, 2] += math.sin(phase * 2.0) * (0.95 - verts[mask_legs, 1]) * 0.40
                        mask_r_arm = (verts[:, 0] < -0.18) & (verts[:, 1] > 0.5) & (verts[:, 1] < 1.45)
                        mask_l_arm = (verts[:, 0] > 0.18) & (verts[:, 1] > 0.5) & (verts[:, 1] < 1.45)
                        frame_verts[mask_r_arm, 2] += math.sin(phase * 2.0) * 0.25
                        frame_verts[mask_l_arm, 2] -= math.sin(phase * 2.0) * 0.25
                    elif anim_name in ['Jump']:
                        jump_h = math.sin(fr / fcount * math.pi) * 0.45
                        frame_verts[:, 1] += jump_h
                    elif anim_name in ['Attack']:
                        swing = math.sin(phase) * 0.35
                        mask_upper = verts[:, 1] > 0.8
                        frame_verts[mask_upper, 2] += swing
                    elif anim_name in ['HitReact']:
                        recoil = math.sin(phase) * 0.08
                        frame_verts[:, 2] -= recoil
                    elif anim_name in ['Death']:
                        progress = min(1.0, fr / max(1, fcount - 1))
                        rot_rad = progress * (math.pi / 2.0)
                        cos_r = math.cos(rot_rad)
                        sin_r = math.sin(rot_rad)
                        y_orig = frame_verts[:, 1].copy()
                        z_orig = frame_verts[:, 2].copy()
                        frame_verts[:, 1] = y_orig * cos_r - z_orig * sin_r
                        frame_verts[:, 2] = y_orig * sin_r + z_orig * cos_r

                    f.write(frame_verts.astype(np.float32).tobytes())


def get_blender_binary():
    b = shutil.which("blender")
    if b: return b
    candidate_paths = [
        "/home/upd8-guilherme/.local/share/flatpak/app/org.upbge.UPBGE/x86_64/stable/1d5a16c8fdba0b8c538b5859e4bd2089d67b31b7dd4eeb2eaa9f5af9f31daa13/files/blender/blender",
        "/usr/bin/blender",
        "/usr/local/bin/blender"
    ]
    for p in candidate_paths:
        if os.path.exists(p):
            return p
    return "blender"


# -----------------------------------------------------------------------------
# NOTA IMPORTANTE PARA FUTURAS EDIÇÕES NO ASSET COOKER (BLENDER 4.4+):
# Ao cozinhar animações dos personagens com Blender headless:
# 1. Os modificadores 'Armature' dos objetos de malha DEVEM ser retargeted para
#    o esqueleto ativo importado da biblioteca (anim_armature).
# 2. No Blender 4.4+, atribuir apenas 'animation_data.action = act' NÃO aplica a
#    animação! É OBRIGATÓRIO vincular o 'action_slot'
#    (active_armature.animation_data.action_slot = slots[0]).
# 3. Chamar 'bpy.context.view_layer.update()' a cada frame antes de 'to_mesh()'.
# -----------------------------------------------------------------------------
def cook_3d_models(source_dir, output_dir):
    models_out_dir = os.path.join(output_dir, "Models")
    os.makedirs(models_out_dir, exist_ok=True)

    base_fbx = os.path.join(source_dir, "Universal Base Characters", "Base Characters", "Superhero_Male_FullBody.fbx")
    peasant_fbx = os.path.join(source_dir, "Modular Character Outfits - Fantasy", "Outfits", "Male_Peasant.fbx")
    ranger_fbx = os.path.join(source_dir, "Modular Character Outfits - Fantasy", "Outfits", "Male_Ranger.fbx")
    anim_fbx1 = os.path.join(source_dir, "Universal Animation Library", "UAL1_Standard.fbx")
    anim_fbx2 = os.path.join(source_dir, "Universal Animation Library 2", "UAL2_Standard.fbx")

    models_to_cook = [
        ("Superhero_Male.lonmodel", base_fbx),
        ("Male_Peasant.lonmodel", peasant_fbx),
        ("Male_Ranger.lonmodel", ranger_fbx)
    ]

    blender_bin = get_blender_binary()

    for model_name, mesh_fbx in models_to_cook:
        if not os.path.exists(mesh_fbx):
            continue

        out_model = os.path.join(models_out_dir, model_name)
        need_cook = False
        if not os.path.exists(out_model):
            need_cook = True
        else:
            mtime_out = os.path.getmtime(out_model)
            if os.path.getmtime(mesh_fbx) > mtime_out: need_cook = True
            if os.path.exists(anim_fbx1) and os.path.getmtime(anim_fbx1) > mtime_out: need_cook = True
            if os.path.exists(anim_fbx2) and os.path.getmtime(anim_fbx2) > mtime_out: need_cook = True

        if not need_cook:
            print(f"[AssetCooker] Modelo 3D up-to-date: {out_model}")
            continue

        script_path = os.path.join(output_dir, "temp_cook_script.py")
        with open(script_path, "w", encoding="utf-8") as f:
            f.write(f"""import bpy
import struct
import os
import numpy as np

source_dir = r"{source_dir}"
output_dir = r"{output_dir}"

base_fbx = os.path.join(source_dir, "Universal Base Characters", "Base Characters", "Superhero_Male_FullBody.fbx")
anim_fbx1 = os.path.join(source_dir, "Universal Animation Library", "UAL1_Standard.fbx")
anim_fbx2 = os.path.join(source_dir, "Universal Animation Library 2", "UAL2_Standard.fbx")

mesh_fbx = r"{mesh_fbx}"
out_model = r"{out_model}"

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=mesh_fbx)

main_armature = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE'][0]
mesh_objs = [o for o in bpy.context.scene.objects if o.type == 'MESH']

anims_to_extract = [
    ("Idle", [anim_fbx1, anim_fbx2], 30),
    ("Walk", [anim_fbx1, anim_fbx2], 30),
    ("Run", [anim_fbx1, anim_fbx2], 24),
    ("Jump", [anim_fbx1, anim_fbx2], 24),
    ("Attack", [anim_fbx1, anim_fbx2], 20)
]

matched_anims = []
for anim_label, fbx_sources, max_f in anims_to_extract:
    found_action = None
    for src_fbx in fbx_sources:
        if os.path.exists(src_fbx):
            bpy.ops.import_scene.fbx(filepath=src_fbx)
            for act in bpy.data.actions:
                act_lower = act.name.lower()
                if anim_label == "Idle":
                    if "idle" in act_lower and not any(bad in act_lower for bad in ["crouch", "sit", "pistol", "torch", "swim", "spell", "zombie", "ninja", "lay"]):
                        found_action = act
                        break
                elif anim_label == "Walk":
                    if "walk" in act_lower and not any(bad in act_lower for bad in ["crouch", "zombie", "carry"]):
                        found_action = act
                        break
                elif anim_label == "Run":
                    if any(k in act_lower for k in ["sprint", "jog_fwd", "run"]):
                        found_action = act
                        break
                elif anim_label == "Jump":
                    if "jump_start" in act_lower or "jump_loop" in act_lower or "jump" in act_lower:
                        if "ninja" not in act_lower:
                            found_action = act
                            break
                elif anim_label == "Attack":
                    if "sword_attack" in act_lower or "sword_regular" in act_lower or "attack" in act_lower:
                        found_action = act
                        break
        if found_action: break
    if found_action:
        matched_anims.append((anim_label, found_action, max_f))

anim_armatures = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE' and o != main_armature]
active_armature = anim_armatures[0] if anim_armatures else main_armature

for m_obj in mesh_objs:
    for mod in m_obj.modifiers:
        if mod.type == 'ARMATURE':
            mod.object = active_armature

P = np.array([[1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1]], dtype=np.float32)
P_inv = np.array([[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1]], dtype=np.float32)

with open(out_model, "wb") as f:
    f.write(b"LONM")
    f.write(struct.pack('<I', 3)) # Version 3
    f.write(struct.pack('<II', len(mesh_objs), len(matched_anims)))

    for clean_name, act, max_frames in matched_anims:
        name_bytes = clean_name.encode('utf-8')[:31].ljust(32, b'\\x00')
        f.write(name_bytes)
        f.write(struct.pack('<If', max_frames, 30.0))

        if not active_armature.animation_data:
            active_armature.animation_data_create()
        active_armature.animation_data.action = act
        slots = list(active_armature.animation_data.action_suitable_slots)
        if slots:
            active_armature.animation_data.action_slot = slots[0]

        start_f = int(act.frame_range[0])
        end_f = int(act.frame_range[1])
        step = max(1, (end_f - start_f) // max_frames)

        for i in range(max_frames):
            frame = start_f + i * step
            bpy.context.scene.frame_set(frame)
            bpy.context.view_layer.update()
            dg = bpy.context.evaluated_depsgraph_get()
            arm_ev = active_armature.evaluated_get(dg)
            hand_bone = arm_ev.pose.bones.get('hand_r')
            if hand_bone:
                mat_b = np.array(hand_bone.matrix, dtype=np.float32)
                mat_r = P @ mat_b @ P_inv
            else:
                mat_r = np.eye(4, dtype=np.float32)

            flat_mat = mat_r.T.reshape(-1)
            f.write(struct.pack('<16f', *flat_mat))

    for m_obj in mesh_objs:
        dg = bpy.context.evaluated_depsgraph_get()
        eval_obj = m_obj.evaluated_get(dg)
        mesh_data = eval_obj.to_mesh()
        mesh_data.calc_loop_triangles()
        uv_layer = mesh_data.uv_layers.active.data if mesh_data.uv_layers.active else None

        verts = []
        normals = []
        uvs = []
        indices = []
        vert_map = {{}}

        for tri in mesh_data.loop_triangles:
            for loop_idx in tri.loops:
                v_idx = mesh_data.loops[loop_idx].vertex_index
                co = mesh_data.vertices[v_idx].co
                norm = mesh_data.loops[loop_idx].normal

                uv_u, uv_v = 0.0, 0.0
                if uv_layer:
                    uv = uv_layer[loop_idx].uv
                    uv_u, uv_v = uv.x, 1.0 - uv.y

                key = (v_idx, round(uv_u, 4), round(uv_v, 4))
                if key not in vert_map:
                    vert_map[key] = len(verts)
                    verts.append((co.x, co.z, -co.y))
                    normals.append((norm.x, norm.z, -norm.y))
                    uvs.append((uv_u, uv_v))

                indices.append(vert_map[key])

        eval_obj.to_mesh_clear()

        f.write(struct.pack('<II', len(verts), len(indices)))
        for norm in normals:
            f.write(struct.pack('<fff', norm[0], norm[1], norm[2]))
        for uv in uvs:
            f.write(struct.pack('<ff', uv[0], uv[1]))
        for idx in indices:
            f.write(struct.pack('<I', idx))

        for clean_name, act, max_frames in matched_anims:
            if not active_armature.animation_data:
                active_armature.animation_data_create()
            active_armature.animation_data.action = act
            slots = list(active_armature.animation_data.action_suitable_slots)
            if slots:
                active_armature.animation_data.action_slot = slots[0]

            start_f = int(act.frame_range[0])
            end_f = int(act.frame_range[1])
            step = max(1, (end_f - start_f) // max_frames)

            for i in range(max_frames):
                frame = start_f + i * step
                bpy.context.scene.frame_set(frame)
                bpy.context.view_layer.update()
                dg = bpy.context.evaluated_depsgraph_get()
                ev_o = m_obj.evaluated_get(dg)
                m_data = ev_o.to_mesh()

                frame_verts = []
                for key in vert_map:
                    v_idx = key[0]
                    c = m_data.vertices[v_idx].co
                    frame_verts.append((c.x, c.z, -c.y))

                ev_o.to_mesh_clear()

                for fv in frame_verts:
                    f.write(struct.pack('<fff', fv[0], fv[1], fv[2]))

print(f'[AssetCooker] Model file cooked: {{out_model}} ({{os.path.getsize(out_model)}} bytes)')
""")

        res = subprocess.run([blender_bin, "--background", "--python", script_path], capture_output=True, text=True)
        if res.returncode == 0 and os.path.exists(out_model):
            print(f"[AssetCooker] Modelo .lonmodel gerado com sucesso via Blender: {out_model} ({os.path.getsize(out_model)} bytes)")
        else:
            print(f"[AssetCooker] Erro no Blender asset cooker: {res.stderr}")



def cook_monsters(source_dir, output_dir):
    monsters_dir = os.path.join(source_dir, "Ultimate Monsters")
    if not os.path.exists(monsters_dir):
        return

    models_out_dir = os.path.join(output_dir, "Models")
    os.makedirs(models_out_dir, exist_ok=True)

    post_proc = (assimp_py.Process_Triangulate |
                 assimp_py.Process_GenSmoothNormals |
                 assimp_py.Process_JoinIdenticalVertices |
                 assimp_py.Process_PreTransformVertices)

    monster_anims_def = [
        ('Idle', 15, 30.0),
        ('Walk', 15, 30.0),
        ('Run', 15, 30.0),
        ('Attack', 15, 30.0),
        ('HitReact', 10, 30.0),
        ('Death', 20, 30.0)
    ]

    categories = ["Big", "Blob", "Flying"]
    for cat in categories:
        cat_path = os.path.join(monsters_dir, cat)
        if not os.path.exists(cat_path):
            continue

        for fbx_file in os.listdir(cat_path):
            if not fbx_file.lower().endswith(".fbx"):
                continue

            monster_name_raw = os.path.splitext(fbx_file)[0]
            fbx_path = os.path.join(cat_path, fbx_file)
            model_name = f"Monster_{cat}_{monster_name_raw}.lonmodel"
            out_model = os.path.join(models_out_dir, model_name)

            if os.path.exists(out_model) and os.path.getmtime(fbx_path) <= os.path.getmtime(out_model):
                continue

            scene = assimp_py.import_file(fbx_path, post_proc)
            build_lonmodel_binary(scene, out_model, monster_anims_def, target_height=2.0)
            print(f"[AssetCooker] Modelo de monstro .lonmodel gerado: {out_model}")


def cook_rpg_items(source_dir, output_dir):
    items_src_icons = os.path.join(source_dir, "Ultimate RPG Items Pack", "Icons")
    items_src_fbx = os.path.join(source_dir, "Ultimate RPG Items Pack", "FBX")

    items_out_dir = os.path.join(output_dir, "Items")
    icons_out_dir = os.path.join(items_out_dir, "Icons")
    models_out_dir = os.path.join(items_out_dir, "Models")

    os.makedirs(icons_out_dir, exist_ok=True)
    os.makedirs(models_out_dir, exist_ok=True)

    if os.path.exists(items_src_icons):
        for icon_file in os.listdir(items_src_icons):
            if icon_file.lower().endswith('.png'):
                src = os.path.join(items_src_icons, icon_file)
                dst = os.path.join(icons_out_dir, icon_file)
                if not os.path.exists(dst) or os.path.getmtime(src) > os.path.getmtime(dst):
                    shutil.copy2(src, dst)

    item_fbxs = ["Sword.fbx", "Axe_small.fbx", "Potion1_Filled.fbx", "Armor_Metal.fbx", "Armor_Leather.fbx"]
    post_proc = (assimp_py.Process_Triangulate |
                 assimp_py.Process_GenSmoothNormals |
                 assimp_py.Process_JoinIdenticalVertices |
                 assimp_py.Process_PreTransformVertices)

    item_fbxs = ["Sword.fbx", "Axe_small.fbx", "Potion1_Filled.fbx", "Armor_Metal.fbx", "Armor_Leather.fbx"]
    post_proc = (assimp_py.Process_Triangulate |
                 assimp_py.Process_GenSmoothNormals |
                 assimp_py.Process_JoinIdenticalVertices |
                 assimp_py.Process_PreTransformVertices)

    color_map = {
        'Liquid_Red': [0.95, 0.1, 0.1],
        'Glass': [0.75, 0.88, 0.95],
        'Brown': [0.45, 0.25, 0.12],
        'DarkSteel': [0.25, 0.25, 0.30],
        'LightSteel': [0.75, 0.78, 0.82],
        'Steel': [0.45, 0.48, 0.52],
        'DarkWood': [0.30, 0.16, 0.08],
        'LightWood': [0.55, 0.35, 0.18]
    }

    for item_fbx in item_fbxs:
        fbx_path = os.path.join(items_src_fbx, item_fbx)
        if not os.path.exists(fbx_path):
            continue

        base_name = os.path.splitext(item_fbx)[0]
        obj_name = base_name + ".obj"
        mtl_name = base_name + ".mtl"
        out_obj = os.path.join(models_out_dir, obj_name)
        out_mtl = os.path.join(models_out_dir, mtl_name)

        scene = assimp_py.import_file(fbx_path, post_proc)

        # 1. Calcula escala para o item ter tamanho aproximado de 0.6m (60cm) no mundo
        all_verts = []
        for m in scene.meshes:
            verts = np.frombuffer(m.vertices, dtype=np.float32).reshape(-1, 3)
            if len(verts) > 0:
                all_verts.append(verts)

        if len(all_verts) == 0:
            continue

        all_v = np.vstack(all_verts)
        max_dim = float((all_v.max(axis=0) - all_v.min(axis=0)).max())
        scale = 0.6 / max_dim if max_dim > 0.0 else 1.0

        # Center Y min at 0
        min_y = float(all_v[:, 1].min()) * scale

        # 2. Escreve arquivo de materiais .mtl
        with open(out_mtl, 'w') as f_mtl:
            f_mtl.write(f'# Material for {item_fbx}\n')
            for i, mat in enumerate(scene.materials):
                mat_name = mat.get('NAME', f'mat_{i}')
                f_mtl.write(f'newmtl {mat_name}\n')
                if mat_name in color_map:
                    kd = color_map[mat_name]
                else:
                    kd = mat.get('COLOR_DIFFUSE', [0.8, 0.8, 0.8])[:3]
                f_mtl.write(f'Kd {kd[0]:.4f} {kd[1]:.4f} {kd[2]:.4f}\n\n')

        # 3. Escreve arquivo de modelo .obj
        with open(out_obj, 'w') as f:
            f.write(f'# Generated by AssetCooker\n')
            f.write(f'mtllib {mtl_name}\n')

            vert_offset = 1
            for m_idx, m in enumerate(scene.meshes):
                mat_idx = m.material_index if hasattr(m, 'material_index') and m.material_index < len(scene.materials) else 0
                mat_name = scene.materials[mat_idx].get('NAME', f'mat_{mat_idx}') if mat_idx < len(scene.materials) else 'default'

                f.write(f'o {m.name}\n')
                f.write(f'usemtl {mat_name}\n')

                verts = np.frombuffer(m.vertices, dtype=np.float32).reshape(-1, 3) * scale
                verts[:, 1] -= min_y
                for v in verts:
                    f.write(f'v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n')

                norms = np.frombuffer(m.normals, dtype=np.float32).reshape(-1, 3) if m.normals else None
                if norms is not None:
                    for n in norms:
                        f.write(f'vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n')

                uvs = None
                if m.texcoords and len(m.texcoords) > 0:
                    uvs = np.frombuffer(m.texcoords[0], dtype=np.float32).reshape(-1, 2)
                    for uv in uvs:
                        f.write(f'vt {uv[0]:.6f} {1.0 - uv[1]:.6f}\n')

                indices = np.frombuffer(m.indices, dtype=np.uint32)
                for i in range(0, len(indices), 3):
                    i0 = indices[i] + vert_offset
                    i1 = indices[i + 1] + vert_offset
                    i2 = indices[i + 2] + vert_offset
                    if norms is not None and uvs is not None:
                        f.write(f'f {i0}/{i0}/{i0} {i1}/{i1}/{i1} {i2}/{i2}/{i2}\n')
                    elif norms is not None:
                        f.write(f'f {i0}//{i0} {i1}//{i1} {i2}//{i2}\n')
                    elif uvs is not None:
                        f.write(f'f {i0}/{i0} {i1}/{i1} {i2}/{i2}\n')
                    else:
                        f.write(f'f {i0} {i1} {i2}\n')
                vert_offset += m.num_vertices

        print(f"[AssetCooker] Modelo de item OBJ/MTL gerado: {out_obj}")


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

    sh_proc, sh_skip = cook_shaders(source_dir, output_dir)
    ui_proc, ui_skip = cook_ui_assets(source_dir, output_dir)
    cook_character_textures(source_dir, output_dir)
    cook_3d_models(source_dir, output_dir)
    cook_monsters(source_dir, output_dir)
    cook_rpg_items(source_dir, output_dir)

    print(f"[AssetCooker] Shaders: {sh_proc} processados, {sh_skip} up-to-date")
    print(f"[AssetCooker] UI Assets: {ui_proc} processados, {ui_skip} up-to-date")
    return 0


if __name__ == "__main__":
    sys.exit(main())
