import os
import struct
import math
import numpy as np


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
