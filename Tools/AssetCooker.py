#!/usr/bin/env python3
"""
AssetCooker - Converte assets brutos (FBX, PNG, WAV) em formatos binários/otimizados do motor.
Implementa verificação de timestamp (incremental build) para copiar/processar apenas arquivos modificados.
"""

import sys
import os
import shutil
import subprocess

def cook_ui_assets(source_dir, output_dir):
    ui_double_dir = os.path.join(source_dir, "kenney_fantasy-ui-borders", "PNG", "Double")
    ui_out_dir = os.path.join(output_dir, "UI")

    processed = 0
    skipped = 0

    if os.path.exists(ui_double_dir):
        for root, dirs, files in os.walk(ui_double_dir):
            for file in files:
                if not file.lower().endswith(('.png', '.jpg', '.jpeg')):
                    continue

                src_file = os.path.join(root, file)
                rel_path = os.path.relpath(src_file, ui_double_dir)
                dst_file = os.path.join(ui_out_dir, rel_path)

                os.makedirs(os.path.dirname(dst_file), exist_ok=True)

                if not os.path.exists(dst_file) or os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                    shutil.copy2(src_file, dst_file)
                    processed += 1
                else:
                    skipped += 1

    return processed, skipped

def cook_character_textures(source_dir, output_dir):
    tex_src = os.path.join(source_dir, "Universal Base Characters", "Base Characters", "Textures", "T_Superhero_Male_Ligh.png")
    tex_out_dir = os.path.join(output_dir, "Textures")
    os.makedirs(tex_out_dir, exist_ok=True)

    dst_file = os.path.join(tex_out_dir, "T_Superhero_Male_Ligh.png")
    if os.path.exists(tex_src):
        if not os.path.exists(dst_file) or os.path.getmtime(tex_src) > os.path.getmtime(dst_file):
            shutil.copy2(tex_src, dst_file)
            print(f"[AssetCooker] Textura do personagem copiada: {dst_file}")

def cook_3d_models(source_dir, output_dir):
    fbx_mesh = os.path.join(source_dir, "Universal Base Characters", "Base Characters", "Superhero_Male_FullBody.fbx")
    fbx_anim = os.path.join(source_dir, "Universal Animation Library", "UAL1_Standard.fbx")

    models_out_dir = os.path.join(output_dir, "Models")
    os.makedirs(models_out_dir, exist_ok=True)
    out_model = os.path.join(models_out_dir, "Superhero_Male.lonmodel")

    if not os.path.exists(fbx_mesh):
        return

    # Verificação incremental
    if os.path.exists(out_model) and os.path.getmtime(fbx_mesh) <= os.path.getmtime(out_model):
        if not os.path.exists(fbx_anim) or os.path.getmtime(fbx_anim) <= os.path.getmtime(out_model):
            print(f"[AssetCooker] Modelo 3D up-to-date: {out_model}")
            return

    print(f"[AssetCooker] Gerando .lonmodel v2 com animações de vértices via Blender: {out_model}")

    script_path = os.path.join(models_out_dir, "bake_lonmodel.py")
    with open(script_path, "w") as sf:
        sf.write(f"""import bpy, os, struct

bpy.ops.wm.read_factory_settings(use_empty=True)

fbx_mesh = r"{fbx_mesh}"
fbx_anim = r"{fbx_anim}"
out_model = r"{out_model}"

bpy.ops.import_scene.fbx(filepath=fbx_mesh)

char_armature = None
char_mesh_objs = []
for obj in bpy.data.objects:
    if obj.type == 'ARMATURE':
        char_armature = obj
    elif obj.type == 'MESH':
        char_mesh_objs.append(obj)

if os.path.exists(fbx_anim):
    bpy.ops.import_scene.fbx(filepath=fbx_anim)

    for obj in list(bpy.data.objects):
        if 'Mannequin' in obj.name or 'Armature.001' in obj.name:
            bpy.data.objects.remove(obj, do_unlink=True)

target_anims = [
    ('Idle_Loop', 'Idle', 30),
    ('Walk_Loop', 'Walk', 30),
    ('Sprint_Loop', 'Run', 20),
    ('Jump_Loop', 'Jump', 30)
]

if char_armature and not char_armature.animation_data:
    char_armature.animation_data_create()

matched_anims = []
for search_name, clean_name, max_frames in target_anims:
    act = None
    for a in bpy.data.actions:
        if a.name.endswith('|' + search_name) or a.name == search_name:
            act = a
            break
    if act:
        matched_anims.append((clean_name, act, max_frames))

print(f'Matched {{len(matched_anims)}} animation tracks.')

# Abre o arquivo binário .lonmodel v2
with open(out_model, 'wb') as f:
    f.write(b'LONM')
    f.write(struct.pack('<III', 2, len(char_mesh_objs), len(matched_anims)))

    for clean_name, act, max_frames in matched_anims:
        name_bytes = clean_name.encode('utf-8')[:31].ljust(32, b'\\x00')
        f.write(name_bytes)
        f.write(struct.pack('<If', max_frames, 30.0))

    for m_obj in char_mesh_objs:
        depsgraph = bpy.context.evaluated_depsgraph_get()
        eval_obj = m_obj.evaluated_get(depsgraph)
        mesh = eval_obj.to_mesh()
        mesh.calc_loop_triangles()

        verts = []
        normals = []
        uvs = []
        indices = []
        vert_map = {{}}

        uv_layer = mesh.uv_layers.active.data if mesh.uv_layers.active else None

        for tri in mesh.loop_triangles:
            for loop_idx in tri.loops:
                v_idx = mesh.loops[loop_idx].vertex_index
                co = mesh.vertices[v_idx].co
                norm = mesh.loops[loop_idx].normal

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

        num_verts = len(verts)
        num_indices = len(indices)
        f.write(struct.pack('<II', num_verts, num_indices))

        for norm in normals:
            f.write(struct.pack('<fff', norm[0], norm[1], norm[2]))
        for uv in uvs:
            f.write(struct.pack('<ff', uv[0], uv[1]))
        for idx in indices:
            f.write(struct.pack('<I', idx))

        for clean_name, act, max_frames in matched_anims:
            char_armature.animation_data.action = act
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
        print(f"[AssetCooker] Modelo .lonmodel v2 gerado com sucesso: {out_model} ({os.path.getsize(out_model)} bytes)")
    else:
        print(f"[AssetCooker] Erro no Blender asset cooker: {res.stderr}")

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

    ui_proc, ui_skip = cook_ui_assets(source_dir, output_dir)
    cook_character_textures(source_dir, output_dir)
    cook_3d_models(source_dir, output_dir)

    print(f"[AssetCooker] UI Assets: {ui_proc} processados, {ui_skip} up-to-date")
    return 0

if __name__ == "__main__":
    sys.exit(main())
