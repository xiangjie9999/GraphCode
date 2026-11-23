"""Static mesh bundles: Blender owns evaluation/FBX, Integrate owns material translation."""
import hashlib
import json
from pathlib import Path
import re

import bpy
from mathutils import Matrix

from .blender_export import export_material
from .compiler import identifier


def export_meshes(objects, directory, destination, mode="hybrid", resolution=2048,
                  surface_policy="strict"):
    directory = Path(directory).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    manifest = directory / "integrate.json"
    manifest.unlink(missing_ok=True)
    if not re.fullmatch(r"/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+", destination):
        raise ValueError("Choose a /Game destination folder using letters, digits and underscores")
    objects = sorted(set(objects), key=lambda obj: obj.name)
    if not objects or any(obj.type != "MESH" for obj in objects):
        raise ValueError("Select one or more mesh objects")
    if any(obj.mode != "OBJECT" for obj in objects):
        raise ValueError("Switch selected meshes to Object Mode before exporting")
    names = [identifier(obj.name) for obj in objects]
    if len(set(name.casefold() for name in names)) != len(names):
        raise ValueError("Selected object names become the same Unreal name; rename one before export")
    if abs(bpy.context.scene.unit_settings.scale_length - 1.0) > 1e-8:
        raise ValueError("Mesh and material delivery currently requires scene Unit Scale 1 (one Blender unit = one metre)")
    for obj in objects:
        if obj.find_armature() or any(mod.type == "ARMATURE" for mod in obj.modifiers):
            raise ValueError(f"{obj.name}: skeletal mesh delivery is not implemented; export a static mesh object")
        if abs(obj.matrix_world.to_3x3().determinant()) < 1e-10:
            raise ValueError(f"{obj.name}: zero scale cannot produce a mesh")
    depsgraph = bpy.context.evaluated_depsgraph_get()
    instance_parents = {instance.parent.original for instance in depsgraph.object_instances
                        if instance.is_instance and instance.parent}
    for obj in objects:
        if obj in instance_parents:
            raise ValueError(f"{obj.name}: realize generated instances on an export copy first (Geometry Nodes: Realize Instances); mesh-only export would omit them")
    bundle = {"format": "graphcode-integrate", "version": 1, "integration": "blender-mesh",
              "destination": destination, "meshes": [], "materials": [],
              "source": {"blend_file": bpy.data.filepath, "blender_version": bpy.app.version_string,
                         "frame": bpy.context.scene.frame_current}, "report": "report.json"}
    report = {"status": "exported", "meshes": [], "materials": [], "limitations": [
        "Static geometry at the current frame; no skeleton, animation or scene placement.",
        "Object origins become asset pivots; rotation and scale are baked, translation is recorded only.",
        "Materials use UV0; other UV channels are preserved by FBX but not translated into shaders.",
        "Geometry-dependent textures freeze the current source scene and require nonoverlapping UV0."]}
    try:
        for obj, name in zip(objects, names):
            asset_folder = destination + "/" + name
            mesh_asset = asset_folder + "/SM_" + name
            data = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph),
                                                  preserve_all_data_layers=True, depsgraph=depsgraph)
            scene = bpy.data.scenes.new("GCIntegrate_MeshExport")
            copy = bpy.data.objects.new("SM_" + name, data)
            placeholders = []
            try:
                scene.collection.objects.link(copy)
                scene.unit_settings.scale_length = 1
                if not data.polygons:
                    raise ValueError(f"{obj.name}: evaluated mesh has no faces")
                # Use evaluated material slots (Geometry Nodes may add slots); object overrides
                # are already represented by new_from_object's evaluated mesh.
                originals = list(data.materials)
                if len(obj.material_slots) == len(originals):
                    originals = [slot.material for slot in obj.material_slots]
                used = sorted({polygon.material_index for polygon in data.polygons})
                if originals and any(i >= len(originals) or originals[i] is None for i in used):
                    raise ValueError(f"{obj.name}: assign a material to every used material slot")
                slots = []
                translated = {}
                for index in used if originals else []:
                    material = originals[index]
                    if material not in translated:
                        mat_name = "M_" + identifier(material.name) + "_" + str(index)
                        mat_asset = asset_folder + "/" + mat_name
                        mat_folder = directory / name / mat_name
                        export_material(material, mat_folder, mat_asset, mode, resolution, obj, surface_policy)
                        translated[material] = mat_asset
                        bundle["materials"].append({"asset": mat_asset,
                            "bundle": (mat_folder / "integrate.json").relative_to(directory).as_posix()})
                        report["materials"].append({"asset": mat_asset,
                            "report": (mat_folder / "report.json").relative_to(directory).as_posix(),
                            "conversion": json.loads((mat_folder / "report.json").read_text(encoding="utf-8"))})
                    slots.append({"name": "GCSlot_" + str(index), "material": translated[material]})
                # Drop unused slots deliberately, retaining polygon identity by an explicit map.
                polygon_slots = [polygon.material_index for polygon in data.polygons]
                data.materials.clear()
                for slot in slots:
                    candidate = slot["name"]
                    suffix = 0
                    while bpy.data.materials.get(candidate):
                        suffix += 1
                        candidate = slot["name"] + "_" + str(suffix)
                    temporary = bpy.data.materials.new(candidate)
                    placeholders.append(temporary)
                    data.materials.append(temporary)
                    slot["name"] = temporary.name  # Blender may suffix a pre-existing user name.
                remap = {old: new for new, old in enumerate(used)}
                for polygon, original_slot in zip(data.polygons, polygon_slots):
                    polygon.material_index = remap[original_slot] if slots else 0
                transform = obj.matrix_world.copy()
                transform.translation = (0, 0, 0)
                data.transform(transform)
                if transform.determinant() < 0:
                    data.flip_normals()
                data.update()
                copy.matrix_world = Matrix.Identity(4)
                # Deterministic triangulation is shared with the FBX's tangent basis.
                triangulate = copy.modifiers.new("Integrate triangulation", "TRIANGULATE")
                triangulate.quad_method = "FIXED"
                scene.view_layers[0].objects.active = copy
                copy.select_set(True, view_layer=scene.view_layers[0])
                output = directory / name / ("SM_" + name + ".fbx")
                output.parent.mkdir(parents=True, exist_ok=True)
                with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0],
                                               active_object=copy, object=copy,
                                               selected_objects=[copy], selected_editable_objects=[copy]):
                    result = bpy.ops.export_scene.fbx(filepath=str(output), use_selection=True,
                        object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
                        use_tspace=True, bake_anim=False, add_leaf_bones=False,
                        axis_forward="-Y", axis_up="Z", apply_unit_scale=True,
                        apply_scale_options="FBX_SCALE_ALL", path_mode="STRIP", embed_textures=False)
                if "FINISHED" not in result or not output.is_file():
                    raise ValueError(f"{obj.name}: Blender did not write the FBX")
                vertices = [vertex.co for vertex in data.vertices]
                dimensions = [100 * (max(v[i] for v in vertices) - min(v[i] for v in vertices)) for i in range(3)]
                item = {"asset": mesh_asset, "file": output.relative_to(directory).as_posix(),
                        "sha256": hashlib.sha256(output.read_bytes()).hexdigest(), "slots": slots,
                        "source_object": obj.name, "source_matrix": [list(row) for row in obj.matrix_world],
                        "dimensions_cm": dimensions, "uv_channels": len(data.uv_layers),
                        "vertices": len(data.vertices), "polygons": len(data.polygons)}
                bundle["meshes"].append(item)
                report["meshes"].append(item)
            finally:
                bpy.data.objects.remove(copy, do_unlink=True)
                bpy.data.meshes.remove(data)
                for material in placeholders:
                    bpy.data.materials.remove(material)
                bpy.data.scenes.remove(scene)
        manifest.write_text(json.dumps(bundle, indent=2), encoding="utf-8")
    except Exception as error:
        report.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    return bundle
