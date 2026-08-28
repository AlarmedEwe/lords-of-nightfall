import os
import shutil
import subprocess


def get_blender_binary():
    b = shutil.which("blender")
    if b:
        return b
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
            if os.path.getmtime(mesh_fbx) > mtime_out:
                need_cook = True
            if os.path.exists(anim_fbx1) and os.path.getmtime(anim_fbx1) > mtime_out:
                need_cook = True
            if os.path.exists(anim_fbx2) and os.path.getmtime(anim_fbx2) > mtime_out:
                need_cook = True

        if not need_cook:
            print(f"[AssetCooker] Modelo 3D up-to-date: {out_model}")
            continue

        # Fallback: se o modelo já existe em outra pasta de build (ex: Development), copia para a pasta atual
        project_root = os.path.abspath(os.path.join(output_dir, "..", "..", ".."))
        cached_candidates = [
            os.path.join(project_root, "Build", "Development", "Assets", "Models", model_name),
            os.path.join(project_root, "Build", "Debug", "Assets", "Models", model_name),
            os.path.join(project_root, "Build", "Release", "Assets", "Models", model_name),
            os.path.join(project_root, "Build", "Debug", "assets", "Models", model_name),
            os.path.join(source_dir, "Models", model_name)
        ]
        copied_from_cache = False
        for cand in cached_candidates:
            if os.path.exists(cand) and os.path.abspath(cand) != os.path.abspath(out_model):
                shutil.copy2(cand, out_model)
                print(f"[AssetCooker] Modelo 3D copiado do cache ({cand} -> {out_model})")
                copied_from_cache = True
                break

        if copied_from_cache:
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

        try:
            res = subprocess.run([blender_bin, "--background", "--python", script_path], capture_output=True, text=True)
            if res.returncode == 0 and os.path.exists(out_model):
                print(f"[AssetCooker] Modelo .lonmodel gerado com sucesso via Blender: {out_model} ({os.path.getsize(out_model)} bytes)")
            else:
                print(f"[AssetCooker] Erro no Blender asset cooker: {res.stderr}")
        except FileNotFoundError:
            print(f"[AssetCooker] Blender não encontrado no sistema ('{blender_bin}'). Pulando bake de animação...")
        except Exception as e:
            print(f"[AssetCooker] Exceção ao executar Blender: {e}")
