import os
import assimp_py
from .lonmodel_builder import build_lonmodel_binary


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
        ('Idle', 30, 30.0),
        ('Walk', 30, 30.0),
        ('Run', 24, 30.0),
        ('Attack', 25, 30.0),
        ('HitReact', 15, 30.0),
        ('Death', 30, 30.0)
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

            need_cook = False
            if not os.path.exists(out_model) or os.path.getmtime(fbx_path) > os.path.getmtime(out_model):
                need_cook = True

            # Força re-cook se os arquivos forem antigos (versão anterior sem category-specific anims)
            cooker_mtime = os.path.getmtime(__file__)
            if os.path.exists(out_model) and cooker_mtime > os.path.getmtime(out_model):
                need_cook = True

            if not need_cook:
                continue

            scene = assimp_py.import_file(fbx_path, post_proc)
            build_lonmodel_binary(scene, out_model, monster_anims_def, target_height=2.0, category=cat)
            print(f"[AssetCooker] Modelo de monstro .lonmodel ({cat}) gerado: {out_model}")
