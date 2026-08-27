import os
import shutil
import struct
import assimp_py
import numpy as np


def get_target_height(base_name):
    n = base_name.lower()
    if 'flower' in n: return 0.5
    if 'grass' in n: return 0.45
    if 'fern' in n: return 0.55
    if 'clover' in n: return 0.35
    if 'plant_1_big' in n: return 0.8
    if 'plant' in n: return 0.5
    if 'bush' in n: return 1.1
    if 'twistedtree' in n: return 8.5
    if 'deadtree' in n: return 8.5
    if 'commontree' in n: return 7.5
    if 'pine' in n: return 7.5
    if 'rock_medium' in n: return 1.2
    if 'mushroom' in n: return 0.4
    return None


def cook_nature_assets(source_dir, output_dir):
    nature_fbx_dir = os.path.join(source_dir, "Stylized Nature MegaKit", "FBX")
    nature_tex_dir = os.path.join(source_dir, "Stylized Nature MegaKit", "Textures")

    if not os.path.exists(nature_fbx_dir):
        print(f"[AssetCooker] Diretorio de FBX de natureza nao encontrado: {nature_fbx_dir}")
        return

    models_out_dir = os.path.join(output_dir, "Models")
    textures_out_dir = os.path.join(output_dir, "Textures")

    os.makedirs(models_out_dir, exist_ok=True)
    os.makedirs(textures_out_dir, exist_ok=True)

    # 1. Copia todas as texturas de natureza para a pasta de saída (com verificação de timestamp)
    if os.path.exists(nature_tex_dir):
        for tex_file in os.listdir(nature_tex_dir):
            if tex_file.lower().endswith(".png"):
                src_tex = os.path.join(nature_tex_dir, tex_file)
                dst_tex = os.path.join(textures_out_dir, tex_file)
                if not os.path.exists(dst_tex) or os.path.getmtime(src_tex) > os.path.getmtime(dst_tex):
                    shutil.copy2(src_tex, dst_tex)

    # 2. Configurações de pós-processamento do Assimp (SEM Process_PreTransformVertices para evitar 100x scale do Unity/FBX)
    post_proc = (assimp_py.Process_Triangulate |
                 assimp_py.Process_GenSmoothNormals |
                 assimp_py.Process_JoinIdenticalVertices)

    fbx_files = [f for f in os.listdir(nature_fbx_dir) if f.lower().endswith(".fbx")]
    cooked_count = 0
    skipped_count = 0

    for fbx_file in fbx_files:
        fbx_path = os.path.join(nature_fbx_dir, fbx_file)
        base_name = os.path.splitext(fbx_file)[0]
        out_model_name = f"Nature_{base_name}.lonmodel"
        out_model_path = os.path.join(models_out_dir, out_model_name)

        # Checagem incremental de build
        if os.path.exists(out_model_path) and os.path.getmtime(fbx_path) <= os.path.getmtime(out_model_path):
            skipped_count += 1
            continue

        try:
            scene = assimp_py.import_file(fbx_path, post_proc)
        except Exception as e:
            print(f"[AssetCooker] Erro ao importar {fbx_file}: {e}")
            continue

        if not scene.meshes:
            continue

        # Calcula bounding box geral para escala e alinhamento de base Y em 0.0
        all_verts_raw = []
        for m in scene.meshes:
            v_raw = np.frombuffer(m.vertices, dtype=np.float32).reshape(-1, 3).copy()
            if len(v_raw) > 0:
                v_gl = np.zeros_like(v_raw)
                v_gl[:, 0] = v_raw[:, 0]
                v_gl[:, 1] = v_raw[:, 2]
                v_gl[:, 2] = -v_raw[:, 1]
                all_verts_raw.append(v_gl)

        if not all_verts_raw:
            continue

        v_stacked = np.vstack(all_verts_raw)
        min_p = v_stacked.min(axis=0)
        max_p = v_stacked.max(axis=0)
        h_orig = max_p[1] - min_p[1]

        target_h = get_target_height(base_name)
        scale_factor = (target_h / h_orig) if (target_h and h_orig > 0) else 1.0
        min_y_scaled = min_p[1] * scale_factor

        post_proc_meshes = []
        for m in scene.meshes:
            v_raw = np.frombuffer(m.vertices, dtype=np.float32).reshape(-1, 3).copy()
            num_verts = len(v_raw)

            verts = np.zeros_like(v_raw)
            verts[:, 0] = v_raw[:, 0] * scale_factor
            verts[:, 1] = (v_raw[:, 2] * scale_factor) - min_y_scaled
            verts[:, 2] = -v_raw[:, 1] * scale_factor

            if m.normals:
                n_raw = np.frombuffer(m.normals, dtype=np.float32).reshape(-1, 3).copy()
                norms = np.zeros_like(n_raw)
                norms[:, 0] = n_raw[:, 0]
                norms[:, 1] = n_raw[:, 2]
                norms[:, 2] = -n_raw[:, 1]
            else:
                norms = np.zeros((num_verts, 3), dtype=np.float32)

            if m.texcoords and len(m.texcoords) > 0:
                uvs = np.frombuffer(m.texcoords[0], dtype=np.float32).reshape(-1, 2).copy()
                uvs[:, 1] = 1.0 - uvs[:, 1]
            else:
                uvs = np.zeros((num_verts, 2), dtype=np.float32)

            indices = np.frombuffer(m.indices, dtype=np.uint32).copy()
            num_indices = len(indices)

            # Extração de textura do material da submesh
            mat_idx = m.material_index if hasattr(m, 'material_index') and m.material_index < len(scene.materials) else 0
            mat = scene.materials[mat_idx] if mat_idx < len(scene.materials) else {}

            tex_name = ""
            t_dict = mat.get('TEXTURES', {})
            if 1 in t_dict and t_dict[1]:
                tex_name = t_dict[1][0]
            elif 'TEXTURE_BASE' in mat:
                tex_name = mat['TEXTURE_BASE']

            tex_name = os.path.basename(tex_name.replace('\\', '/'))

            # Se pegou mapa normal por engano, tenta direcionar para o mapa difuso
            if "_Normal.png" in tex_name:
                diffuse_cand = tex_name.replace("_Normal.png", ".png")
                if os.path.exists(os.path.join(nature_tex_dir, diffuse_cand)):
                    tex_name = diffuse_cand

            post_proc_meshes.append({
                'verts': verts,
                'norms': norms,
                'uvs': uvs,
                'indices': indices,
                'num_verts': num_verts,
                'num_indices': num_indices,
                'tex_name': tex_name
            })

        num_meshes = len(post_proc_meshes)
        num_anims = 1  # 1 animação estática padrão

        # Gravação do formato binário .lonmodel Versão 4
        with open(out_model_path, 'wb') as f:
            f.write(b'LONM')
            f.write(struct.pack('<III', 4, num_meshes, num_anims))

            # Header da animação "Idle" estática
            name_bytes = "Idle".encode('utf-8')[:31].ljust(32, b'\x00')
            f.write(name_bytes)
            f.write(struct.pack('<If', 1, 30.0))  # 1 frame, 30 fps

            # Matriz padrão da mão direita (identidade)
            hand_mat = [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0
            ]
            f.write(struct.pack('<16f', *hand_mat))

            # Submeshes
            for mdata in post_proc_meshes:
                tex_bytes = mdata['tex_name'].encode('utf-8')[:63].ljust(64, b'\x00')
                f.write(tex_bytes)

                num_verts = mdata['num_verts']
                num_indices = mdata['num_indices']
                verts = mdata['verts']
                norms = mdata['norms']
                uvs = mdata['uvs']
                indices = mdata['indices']

                f.write(struct.pack('<II', num_verts, num_indices))
                f.write(norms.astype(np.float32).tobytes())
                f.write(uvs.astype(np.float32).tobytes())
                f.write(indices.astype(np.uint32).tobytes())

                # Frame 0 de vértices
                f.write(verts.astype(np.float32).tobytes())

        cooked_count += 1

    print(f"[AssetCooker] Assets de natureza processados: {cooked_count} modelos (.lonmodel v4), {skipped_count} up-to-date")
