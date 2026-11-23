"""Read-only assembly inspection in Unreal's Python console.

Shared source-geometry operations for gc_check_assembly. Uses source triangles in
world space, not physics collision or bounding boxes as the verdict.
"""
# @Description: Inspect selected assemblies for mesh penetration without changing assets.

import math

import unreal
import gc_inspection_memory as memory

Q = unreal.GeometryScript_MeshQueries
IDENTITY = unreal.Transform()
LAST_REPORT = None
_boolean_scratch = None


def xyz(v):
    return [float(v.x), float(v.y), float(v.z)]


def bounds(mesh):
    box = Q.get_mesh_bounding_box(mesh)
    return xyz(box.min), xyz(box.max)


def debug_messages(debug):
    return [str(m.message) for m in debug.get_editor_property("messages")]


def mesh_copy(mesh):
    memory.checkpoint()
    out = unreal.DynamicMesh()
    unreal.GeometryScript_MeshEdits.append_mesh(out, mesh, IDENTITY)
    return out


def inspect_pair(a, b, volume_epsilon_cm3=0.1):
    """Measure common solid volume, including full containment. Inputs stay unchanged."""
    alo, ahi = bounds(a)
    blo, bhi = bounds(b)
    lo = [max(x, y) for x, y in zip(alo, blo)]
    hi = [min(x, y) for x, y in zip(ahi, bhi)]
    if any(h < l - 1e-6 for l, h in zip(lo, hi)):
        return {"status": "separated"}
    if not Q.get_is_closed_mesh(a) or not Q.get_is_closed_mesh(b):
        return {"status": "inconclusive", "reason": "Open mesh: solid volume is undefined."}
    # Broad phase touching is not proof of a surface contact. Keep it qualified.
    if any(h - l <= 1e-6 for l, h in zip(lo, hi)):
        return {"status": "bounds_touch_only", "region_min_cm": lo, "region_max_cm": hi}
    global _boolean_scratch
    memory.checkpoint()
    if _boolean_scratch is None:
        _boolean_scratch = unreal.DynamicMesh()
    common = _boolean_scratch
    common.reset()
    try:
        unreal.GeometryScript_MeshEdits.append_mesh(common, a, IDENTITY)
        return _measure_intersection(common, b, volume_epsilon_cm3)
    finally:
        # Keep one UObject but release its triangle buffers on every return/error.
        common.reset()


def _measure_intersection(common, b, volume_epsilon_cm3):
    debug = unreal.GeometryScriptDebug()
    options = unreal.GeometryScriptMeshBooleanOptions(
        fill_holes=False, simplify_output=False, allow_empty_result=True,
        output_transform_space=unreal.GeometryScriptBooleanOutputSpace.SHARED_TRANSFORM_SPACE)
    unreal.GeometryScript_MeshBooleans.apply_mesh_boolean(
        common, IDENTITY, b, IDENTITY,
        unreal.GeometryScriptBooleanOperation.INTERSECTION, options, debug)
    messages = debug_messages(debug)
    if messages:
        return {"status": "inconclusive", "reason": "; ".join(messages)}
    if common.get_triangle_count() == 0:
        return {"status": "no_solid_overlap"}
    if not Q.get_is_closed_mesh(common):
        return {"status": "inconclusive", "reason": "Boolean result is not a closed solid."}
    area, volume = Q.get_mesh_volume_area(common)
    volume = abs(float(volume))
    if not math.isfinite(volume):
        return {"status": "inconclusive", "reason": "Non-finite intersection volume."}
    if volume <= volume_epsilon_cm3:
        return {"status": "below_volume_tolerance", "volume_cm3": volume}
    low, high = bounds(common)
    return {
        "status": "penetration", "volume_cm3": volume,
        "region_min_cm": low, "region_max_cm": high,
        "region_center_cm": [(x + y) * 0.5 for x, y in zip(low, high)],
    }


def descendants(actors):
    result = {}
    for actor in actors:
        for item in [actor, *actor.get_all_child_actors(include_descendants=True)]:
            result[item.get_path_name()] = item
    return list(result.values())


def collect(actors, selected_paths, max_parts=128, max_triangles=250000, checkpoint=lambda: None):
    parts, skipped = [], []
    local_meshes = {}
    total_triangles = 0
    for actor in descendants(actors):
        selected = actor.get_path_name() in selected_paths
        for comp in actor.get_components_by_class(unreal.MeshComponent):
            name = comp.get_path_name()
            if not isinstance(comp, unreal.StaticMeshComponent) or isinstance(comp, unreal.SplineMeshComponent):
                skipped.append({"part": name, "reason": "Deformed or non-static mesh is not supported."})
                continue
            asset = comp.get_editor_property("static_mesh")
            if not asset:
                skipped.append({"part": name, "reason": "No static mesh."})
                continue
            key = asset.get_path_name()
            try:
                checkpoint()
                memory.checkpoint()
                if key not in local_meshes:
                    mesh = unreal.DynamicMesh()
                    debug = unreal.GeometryScriptDebug()
                    _, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
                        asset, mesh, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                        unreal.GeometryScriptMeshReadLOD(), debug)
                    if outcome != unreal.GeometryScriptOutcomePins.SUCCESS or debug_messages(debug):
                        raise RuntimeError("Mesh copy failed: " + "; ".join(debug_messages(debug)))
                    if mesh.get_triangle_count() == 0:
                        raise RuntimeError("Empty mesh source.")
                    local_meshes[key] = mesh
                if isinstance(comp, unreal.InstancedStaticMeshComponent):
                    transforms = [(f"{name}[{i}]", comp.get_instance_transform(i, world_space=True))
                                  for i in range(comp.get_instance_count())]
                else:
                    transforms = [(name, comp.get_world_transform())]
                for part_name, transform in transforms:
                    checkpoint()
                    count = local_meshes[key].get_triangle_count()
                    if len(parts) >= max_parts or total_triangles + count > max_triangles:
                        skipped.append({'part': 'remaining geometry', 'reason': 'Part/triangle budget reached.'})
                        return parts, skipped
                    mesh = mesh_copy(local_meshes[key])
                    unreal.GeometryScript_MeshTransforms.transform_mesh(mesh, transform, True)
                    parts.append({"id": part_name, "actor": actor.get_path_name(),
                                  "label": actor.get_actor_label(), "asset": key,
                                  "mesh": mesh, "selected": selected})
                    total_triangles += count
            except memory.MemoryBudgetExceeded as exc:
                skipped.append({"part": "remaining geometry", "reason": str(exc)})
                return parts, skipped
            except Exception as exc:
                skipped.append({"part": name, "reason": str(exc)})
    return parts, skipped


