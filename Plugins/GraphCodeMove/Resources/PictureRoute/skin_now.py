# The character's skin as it is NOW: every vertex's place (the mesh's own space, cm) and the bones its skin follows, read off the built
# skeletal mesh in the editor (SkinWeightModifier, and Geometry Script for the places; read only, nothing committed back), because the
# rig template rig_now.py writes keeps an older export's skin (EXPORT-CRASH1). On the cat that older skin gave each ear tip 0.18-0.24 to
# the neck where the built mesh gives 0.33, and put one tip 2.8 cm off (2026-10-27). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E20).
# Usage: python skin_now.py <skeletal mesh asset> <out.json>
import json, os, sys, time
import rig_now

EDITOR_SCRIPT = r'''
# @Description: Read a skeletal mesh's skin weights and vertex places for Tools/VideoCapture (read only)
import unreal, json, traceback
def read():
    mesh = unreal.load_asset(MESH)
    m = unreal.SkinWeightModifier()
    if not m.set_skeletal_mesh(mesh): return {'error': 'SkinWeightModifier could not read ' + MESH}
    n = m.get_num_vertices()
    weights = [{str(k): float(x) for k, x in m.get_vertex_weights(v).items() if float(x) > 0.0} for v in range(n)]
    dm = unreal.DynamicMesh()
    unreal.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(mesh, dm, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                                                  unreal.GeometryScriptMeshReadLOD())
    if unreal.GeometryScript_MeshQueries.get_vertex_count(dm) != n:
        return {'error': 'the copied mesh and the skin count different vertices'}
    places = []
    for v in range(n):
        q = unreal.GeometryScript_MeshQueries.get_vertex_position(dm, v)
        p = [x for x in (q if isinstance(q, tuple) else (q,)) if type(x).__name__ == 'Vector'][0]
        places.append([p.x, p.y, p.z])
    return {'mesh': MESH, 'positions': places, 'weights': weights}
try:
    found = read()
except Exception:
    found = {'error': traceback.format_exc()}
open(OUT, 'w').write(json.dumps(found))
'''


def read_skin(mesh, out):
    """Runs EDITOR_SCRIPT in the editor (bridge execute_script, a task) and returns what it wrote."""
    out = os.path.abspath(out).replace('\\', '/')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    if os.path.exists(out): os.remove(out)
    head = 'MESH = %r\nOUT = %r\n' % (mesh, out)
    task = rig_now.tool('execute_script', {'script_type': 'python', 'script_content': head + EDITOR_SCRIPT})['data']['task_id']
    for _ in range(1200):
        state = rig_now.tool('task_status', {'task_id': task})['data']['status']
        if state == 'completed': break
        if state in ('failed', 'cancelled', 'timed_out'):
            raise RuntimeError('skin read %s: %s' % (state, json.dumps(rig_now.tool('task_result', {'task_id': task}))[:600]))
        time.sleep(0.5)
    if not os.path.exists(out): raise RuntimeError('skin read wrote nothing (task %s)' % task)
    skin = json.load(open(out))
    if 'error' in skin: raise RuntimeError('skin read: ' + skin['error'])
    return skin


if __name__ == '__main__':
    if not sys.argv[1].startswith('/Game/'):   # Git Bash rewrites a leading /Game/ into its own install folder: MSYS_NO_PATHCONV=1, or PowerShell
        raise SystemExit('the mesh must be a /Game/... path, got %s' % sys.argv[1])
    skin = read_skin(sys.argv[1], sys.argv[2])
    print('%s: %d vertices -> %s' % (sys.argv[1], len(skin['weights']), sys.argv[2]))
