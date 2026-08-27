import os
import struct
import math
import numpy as np


def relax_tpose_arms(verts, y_min, height, width):
    """
    Relaxa braços/membros esticados horizontalmente em T-Pose,
    posicionando-os de forma natural ao lado/frente do corpo.
    """
    torso_half_width = max(0.18, width * 0.16)
    shoulder_y = y_min + height * 0.62
    arm_base_y = y_min + height * 0.28

    # Braço Esquerdo
    ml = (verts[:, 0] > torso_half_width) & (verts[:, 1] > arm_base_y)
    if np.any(ml):
        dx = verts[ml, 0] - torso_half_width
        dy = verts[ml, 1] - shoulder_y
        ang = math.radians(56.0)
        c, s = math.cos(ang), math.sin(ang)
        verts[ml, 0] = torso_half_width + (dx * c + dy * s)
        verts[ml, 1] = shoulder_y + (-dx * s + dy * c)
        verts[ml, 2] += dx * 0.15

    # Braço Direito
    mr = (verts[:, 0] < -torso_half_width) & (verts[:, 1] > arm_base_y)
    if np.any(mr):
        dx = verts[mr, 0] - (-torso_half_width)
        dy = verts[mr, 1] - shoulder_y
        ang = math.radians(56.0)
        c, s = math.cos(ang), math.sin(ang)
        verts[mr, 0] = -torso_half_width + (dx * c - dy * s)
        verts[mr, 1] = shoulder_y + (dx * s + dy * c)
        verts[mr, 2] += -dx * 0.15


def generate_clip_frames(verts, anim_name, fcount, fps, category):
    """
    Gera quadros de animação de vértices dinâmicos e fluidos por categoria.
    """
    frames = []
    y_min = float(verts[:, 1].min())
    y_max = float(verts[:, 1].max())
    h = max(0.1, y_max - y_min)
    w = float(verts[:, 0].max() - verts[:, 0].min())
    knee_y = y_min + h * 0.45
    torso_half_w = max(0.15, w * 0.18)

    for fr in range(fcount):
        phase = (fr / fcount) * 2.0 * math.pi
        fv = verts.copy()

        if category == "Blob":
            # -------------------------------------------------------------
            # BLOB / SLIME / MAGOS / GATOS
            # -------------------------------------------------------------
            if anim_name in ['Idle', 'PatrolIdle']:
                sq = 1.0 + 0.08 * math.sin(phase)
                fv[:, 1] = y_min + (verts[:, 1] - y_min) * sq + 0.04 * math.sin(phase)
                fv[:, 0] *= 1.0 / math.sqrt(max(0.2, sq))
                fv[:, 2] *= 1.0 / math.sqrt(max(0.2, sq))
                fv[:, 0] += math.sin(phase * 0.5) * 0.02
            elif anim_name in ['Walk', 'PatrolWalk']:
                hop = max(0.0, math.sin(phase))
                fv[:, 1] += hop * 0.25
                sq = 1.0 + (hop * 0.22 - (1.0 - hop) * 0.12)
                fv[:, 1] = y_min + (fv[:, 1] - y_min) * sq
                fv[:, 0] *= 1.0 / math.sqrt(max(0.2, sq))
                fv[:, 2] *= 1.0 / math.sqrt(max(0.2, sq))
                fv[:, 2] += (verts[:, 1] - y_min) * 0.15 * math.sin(phase)
            elif anim_name in ['Run', 'Chase']:
                hop = max(0.0, math.sin(phase * 2.0))
                fv[:, 1] += hop * 0.35
                sq = 1.0 + (hop * 0.30 - (1.0 - hop) * 0.18)
                fv[:, 1] = y_min + (fv[:, 1] - y_min) * sq
                fv[:, 0] *= 1.0 / math.sqrt(max(0.2, sq))
                fv[:, 2] *= 1.0 / math.sqrt(max(0.2, sq))
                fv[:, 2] += (verts[:, 1] - y_min) * 0.28
            elif anim_name in ['Attack']:
                if phase < math.pi * 0.8:
                    charge = math.sin(phase / 0.8 * math.pi)
                    fv[:, 2] -= charge * 0.25
                    fv[:, 1] = y_min + (verts[:, 1] - y_min) * (1.0 - charge * 0.2)
                else:
                    strike = math.sin((phase - math.pi * 0.8) / (math.pi * 1.2) * math.pi)
                    fv[:, 2] += strike * 0.55
                    fv[:, 1] = y_min + (verts[:, 1] - y_min) * (1.0 + strike * 0.15)
            elif anim_name in ['HitReact']:
                wobble = math.sin(phase * 3.0) * math.exp(-fr / fcount * 3.0)
                fv[:, 2] -= wobble * 0.25
                fv[:, 0] += math.cos(phase * 4.0) * wobble * 0.15
            elif anim_name in ['Death']:
                prog = min(1.0, fr / max(1, fcount - 1))
                flat_y = (1.0 - prog * 0.75)
                spread_xz = 1.0 + prog * 0.60
                fv[:, 1] = y_min + (verts[:, 1] - y_min) * flat_y
                fv[:, 0] *= spread_xz
                fv[:, 2] *= spread_xz

        elif category == "Flying":
            # -------------------------------------------------------------
            # FLYING (DRAGÕES, ABELHAS, FANTASMAS, ALPACAS)
            # -------------------------------------------------------------
            wing_l = verts[:, 0] > torso_half_w
            wing_r = verts[:, 0] < -torso_half_w

            if anim_name in ['Idle', 'PatrolIdle']:
                hover_y = math.sin(phase) * 0.14
                fv[:, 1] += hover_y
                wing_beat = math.sin(phase * 2.5)
                if np.any(wing_l):
                    dist_l = verts[wing_l, 0] - torso_half_w
                    fv[wing_l, 1] += wing_beat * dist_l * 0.65
                if np.any(wing_r):
                    dist_r = -torso_half_w - verts[wing_r, 0]
                    fv[wing_r, 1] += wing_beat * dist_r * 0.65
                fv[:, 2] += math.sin(phase) * 0.05
            elif anim_name in ['Walk', 'PatrolWalk']:
                hover_y = math.sin(phase * 1.5) * 0.16
                fv[:, 1] += hover_y
                wing_beat = math.sin(phase * 3.5)
                if np.any(wing_l):
                    dist_l = verts[wing_l, 0] - torso_half_w
                    fv[wing_l, 1] += wing_beat * dist_l * 0.85
                if np.any(wing_r):
                    dist_r = -torso_half_w - verts[wing_r, 0]
                    fv[wing_r, 1] += wing_beat * dist_r * 0.85
                fv[:, 2] += (verts[:, 1] - y_min) * 0.15
            elif anim_name in ['Run', 'Chase']:
                hover_y = math.sin(phase * 2.5) * 0.20
                fv[:, 1] += hover_y
                wing_beat = math.sin(phase * 5.5)
                if np.any(wing_l):
                    dist_l = verts[wing_l, 0] - torso_half_w
                    fv[wing_l, 1] += wing_beat * dist_l * 1.05
                if np.any(wing_r):
                    dist_r = -torso_half_w - verts[wing_r, 0]
                    fv[wing_r, 1] += wing_beat * dist_r * 1.05
                fv[:, 2] += (verts[:, 1] - y_min) * 0.35
            elif anim_name in ['Attack']:
                strike = math.sin(phase)
                fv[:, 2] += strike * 0.50
                fv[:, 1] -= abs(strike) * 0.25
                if np.any(wing_l): fv[wing_l, 1] += strike * 0.45
                if np.any(wing_r): fv[wing_r, 1] += strike * 0.45
            elif anim_name in ['HitReact']:
                recoil = math.sin(phase * 2.5) * math.exp(-fr / fcount * 3.0)
                fv[:, 2] -= recoil * 0.35
                fv[:, 1] += recoil * 0.20
            elif anim_name in ['Death']:
                prog = min(1.0, fr / max(1, fcount - 1))
                rot = prog * (math.pi / 2.0)
                c, s = math.cos(rot), math.sin(rot)
                fv[:, 1] -= prog * 1.5
                y_rel = fv[:, 1] - y_min
                z_rel = fv[:, 2]
                fv[:, 1] = y_min + (y_rel * c - z_rel * s)
                fv[:, 2] = y_rel * s + z_rel * c

        else:
            # -------------------------------------------------------------
            # BIG (YETI, DEMÔNIOS, OGROS, TRIBAL, DINOS)
            # -------------------------------------------------------------
            leg_l = (verts[:, 0] > 0.06) & (verts[:, 1] < knee_y)
            leg_r = (verts[:, 0] < -0.06) & (verts[:, 1] < knee_y)
            arm_l = (verts[:, 0] > torso_half_w) & (verts[:, 1] > y_min + h * 0.35)
            arm_r = (verts[:, 0] < -torso_half_w) & (verts[:, 1] > y_min + h * 0.35)
            torso = verts[:, 1] > knee_y

            if anim_name in ['Idle', 'PatrolIdle']:
                breathe = math.sin(phase) * 0.035
                fv[torso, 1] += breathe
                fv[:, 0] += math.sin(phase * 0.5) * 0.025
                if np.any(arm_l): fv[arm_l, 2] += math.sin(phase) * 0.05
                if np.any(arm_r): fv[arm_r, 2] -= math.sin(phase) * 0.05
            elif anim_name in ['Walk', 'PatrolWalk']:
                step_l = math.sin(phase)
                step_r = -math.sin(phase)
                lift_l = max(0.0, -math.cos(phase)) * 0.16
                lift_r = max(0.0, math.cos(phase)) * 0.16

                if np.any(leg_l):
                    fv[leg_l, 2] += step_l * 0.32
                    fv[leg_l, 1] += lift_l
                if np.any(leg_r):
                    fv[leg_r, 2] += step_r * 0.32
                    fv[leg_r, 1] += lift_r

                if np.any(arm_l): fv[arm_l, 2] -= step_l * 0.28
                if np.any(arm_r): fv[arm_r, 2] -= step_r * 0.28

                fv[torso, 1] += abs(math.sin(phase)) * 0.06
                fv[torso, 0] += math.sin(phase) * 0.035
            elif anim_name in ['Run', 'Chase']:
                step_l = math.sin(phase * 1.5)
                step_r = -math.sin(phase * 1.5)
                lift_l = max(0.0, -math.cos(phase * 1.5)) * 0.24
                lift_r = max(0.0, math.cos(phase * 1.5)) * 0.24

                if np.any(leg_l):
                    fv[leg_l, 2] += step_l * 0.48
                    fv[leg_l, 1] += lift_l
                if np.any(leg_r):
                    fv[leg_r, 2] += step_r * 0.48
                    fv[leg_r, 1] += lift_r

                if np.any(arm_l): fv[arm_l, 2] -= step_l * 0.42
                if np.any(arm_r): fv[arm_r, 2] -= step_r * 0.42

                fv[torso, 2] += (verts[torso, 1] - knee_y) * 0.25
                fv[torso, 1] += abs(math.sin(phase * 1.5)) * 0.08
            elif anim_name in ['Attack']:
                if phase < math.pi * 0.9:
                    windup = math.sin(phase / 0.9 * math.pi * 0.5)
                    if np.any(arm_l):
                        fv[arm_l, 1] += windup * 0.45
                        fv[arm_l, 2] -= windup * 0.25
                    if np.any(arm_r):
                        fv[arm_r, 1] += windup * 0.45
                        fv[arm_r, 2] -= windup * 0.25
                    fv[torso, 2] -= windup * 0.15
                else:
                    slam = math.sin((phase - math.pi * 0.9) / (math.pi * 1.1) * math.pi)
                    if np.any(arm_l):
                        fv[arm_l, 2] += slam * 0.65
                        fv[arm_l, 1] -= slam * 0.20
                    if np.any(arm_r):
                        fv[arm_r, 2] += slam * 0.65
                        fv[arm_r, 1] -= slam * 0.20
                    fv[torso, 2] += slam * 0.35
            elif anim_name in ['HitReact']:
                recoil = math.sin(phase * 2.5) * math.exp(-fr / fcount * 3.0)
                fv[torso, 2] -= recoil * 0.35
                if np.any(arm_l): fv[arm_l, 1] += recoil * 0.25
                if np.any(arm_r): fv[arm_r, 1] += recoil * 0.25
            elif anim_name in ['Death']:
                prog = min(1.0, fr / max(1, fcount - 1))
                rot = prog * (math.pi / 2.0)
                c, s = math.cos(rot), math.sin(rot)
                y_rel = fv[:, 1] - y_min
                z_rel = fv[:, 2]
                fv[:, 1] = y_min + (y_rel * c + z_rel * s) * (1.0 - prog * 0.4)
                fv[:, 2] = -y_rel * s + z_rel * c

        frames.append(fv)

    return frames


def build_lonmodel_binary(scene, out_model_path, anims_def, target_height=1.8, category="Big"):
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

            y_min = float(verts[:, 1].min())
            y_max = float(verts[:, 1].max())
            height = max(0.1, y_max - y_min)
            width = float(verts[:, 0].max() - verts[:, 0].min())

            # Se for biped/humanoid com braços abertos em T-Pose, relaxa para postura natural
            if category in ["Big", "Blob"] and width > 0.8 * height:
                relax_tpose_arms(verts, y_min, height, width)

            f.write(struct.pack('<II', num_verts, num_indices))
            f.write(norms.astype(np.float32).tobytes())
            f.write(uvs.astype(np.float32).tobytes())
            f.write(indices.astype(np.uint32).tobytes())

            for anim_idx, (anim_name, fcount, fps) in enumerate(anims_def):
                clip_frames = generate_clip_frames(verts, anim_name, fcount, fps, category)
                for fv in clip_frames:
                    f.write(fv.astype(np.float32).tobytes())
