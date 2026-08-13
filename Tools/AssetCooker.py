# Script de Asset Cooking para Lords of Nightfall
# Converte e prepara shaders, texturas, modelos FBX e UI assets para a build final

import os
import sys
import shutil
import subprocess

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
                
                # Timestamp check incremental
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
                    
                    # Preserva a estrutura de subpastas (Vector, PNG, etc)
                    rel_path = os.path.relpath(src_file, ui_src)
                    dst_file = os.path.join(ui_out, rel_path)
                    
                    os.makedirs(os.path.dirname(dst_file), exist_ok=True)

                    if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                        shutil.copy2(src_file, dst_file)
                        processed_count += 1
                    else:
                        skipped_count += 1

    return processed_count, skipped_count

def cook_character_textures(source_dir, output_dir):
    tex_src_dir = os.path.join(source_dir, "Universal Base Characters", "textures")
    tex_out_dir = os.path.join(output_dir, "Textures")
    os.makedirs(tex_out_dir, exist_ok=True)

    if os.path.exists(tex_src_dir):
        for f in os.listdir(tex_src_dir):
            if f.endswith('.png'):
                src = os.path.join(tex_src_dir, f)
                dst = os.path.join(tex_out_dir, f)
                if not os.path.exists(dst) or os.path.getmtime(src) > os.path.getmtime(dst):
                    shutil.copy2(src, dst)

def cook_3d_models(source_dir, output_dir):
    models_out_dir = os.path.join(output_dir, "Models")
    os.makedirs(models_out_dir, exist_ok=True)

    # 1. Modelo Base (Superhero Male)
    base_fbx = os.path.join(source_dir, "Universal Base Characters", "Base Characters", "Superhero_Male_FullBody.fbx")
    anim_fbx1 = os.path.join(source_dir, "Universal Animation Library", "UAL1_Standard.fbx")
    anim_fbx2 = os.path.join(source_dir, "Universal Animation Library 2", "UAL2_Standard.fbx")

    models_to_cook = [
        ("Superhero_Male.lonmodel", base_fbx),
        ("Male_Peasant.lonmodel", os.path.join(source_dir, "Modular Character Outfits - Fantasy", "Outfits", "Male_Peasant.fbx")),
        ("Male_Ranger.lonmodel", os.path.join(source_dir, "Modular Character Outfits - Fantasy", "Outfits", "Male_Ranger.fbx"))
    ]

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

        if main_armature and not main_armature.animation_data:
            main_armature.animation_data_create()
        if main_armature:
            main_armature.animation_data.action = act

        start_f = int(act.frame_range[0])
        end_f = int(act.frame_range[1])
        step = max(1, (end_f - start_f) // max_frames)

        for i in range(max_frames):
            frame = start_f + i * step
            bpy.context.scene.frame_set(frame)
            dg = bpy.context.evaluated_depsgraph_get()
            arm_ev = main_armature.evaluated_get(dg)
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
            main_armature.animation_data.action = act
            start_f = int(act.frame_range[0])
            end_f = int(act.frame_range[1])
            step = max(1, (end_f - start_f) // max_frames)

            for i in range(max_frames):
                frame = start_f + i * step
                bpy.context.scene.frame_set(frame)
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

        res = subprocess.run(["blender", "--background", "--python", script_path], capture_output=True, text=True)
        if res.returncode == 0 and os.path.exists(out_model):
            print(f"[AssetCooker] Modelo .lonmodel gerado com sucesso: {out_model} ({os.path.getsize(out_model)} bytes)")
        else:
            print(f"[AssetCooker] Erro no Blender asset cooker: {res.stderr}")

def cook_rpg_items(source_dir, output_dir):
    items_src_icons = os.path.join(source_dir, "Ultimate RPG Items Pack", "Icons")
    items_src_fbx = os.path.join(source_dir, "Ultimate RPG Items Pack", "FBX")

    items_out_dir = os.path.join(output_dir, "Items")
    icons_out_dir = os.path.join(items_out_dir, "Icons")
    models_out_dir = os.path.join(items_out_dir, "Models")

    os.makedirs(icons_out_dir, exist_ok=True)
    os.makedirs(models_out_dir, exist_ok=True)

    # Copiar Ícones dos Itens
    if os.path.exists(items_src_icons):
        for icon_file in os.listdir(items_src_icons):
            if icon_file.lower().endswith('.png'):
                src = os.path.join(items_src_icons, icon_file)
                dst = os.path.join(icons_out_dir, icon_file)
                if not os.path.exists(dst) or os.path.getmtime(src) > os.path.getmtime(dst):
                    shutil.copy2(src, dst)

    # Converter modelos 3D de itens selecionados para .obj triangulado via Blender
    item_fbxs = ["Sword.fbx", "Axe_small.fbx", "Potion1_Filled.fbx", "Armor_Metal.fbx", "Armor_Leather.fbx"]
    for item_fbx in item_fbxs:
        fbx_path = os.path.join(items_src_fbx, item_fbx)
        if not os.path.exists(fbx_path):
            continue
        obj_name = os.path.splitext(item_fbx)[0] + ".obj"
        out_obj = os.path.join(models_out_dir, obj_name)

        if not os.path.exists(out_obj) or os.path.getmtime(fbx_path) > os.path.getmtime(out_obj):
            script = f"""import bpy
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=r"{fbx_path}")
bpy.ops.wm.obj_export(filepath=r"{out_obj}", export_triangulated_mesh=True)
"""
            res = subprocess.run(["blender", "--background", "--python-expr", script], capture_output=True, text=True)
            if res.returncode == 0 and os.path.exists(out_obj):
                print(f"[AssetCooker] Modelo de item OBJ gerado: {out_obj}")
            else:
                print(f"[AssetCooker] Erro ao exportar OBJ: {res.stderr}")

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
    cook_rpg_items(source_dir, output_dir)

    print(f"[AssetCooker] Shaders: {sh_proc} processados, {sh_skip} up-to-date")
    print(f"[AssetCooker] UI Assets: {ui_proc} processados, {ui_skip} up-to-date")
    return 0

if __name__ == "__main__":
    sys.exit(main())
