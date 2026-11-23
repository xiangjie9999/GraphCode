# The character's skeleton and a clip as they are NOW, without the glTF exporter (EXPORT-CRASH1: two editor crashes exporting the
# cat; no glTF export of it in the shared editor). The editor's own pose reader (AnimPoseExtensions, as Tools/Research/Gait/
# measure_walk.py uses it) gives every bone's reference and per-frame local and component transform; the hierarchy is read off those
# (parents_of), and they replace the hierarchy, rest and keys of an older export of the same rig, whose skin's inverse binds are made
# again from the new rest and which gains any bone added since. The result reads as a fresh export would: the rig template and, for
# a clip, its truth. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E14).
# Usage: python rig_now.py <old export.gltf> <clip asset> <skeletal mesh asset> <frames> <out.gltf>
#        python rig_now.py --scratch <clip asset> <skeletal mesh asset> <frames> <out.gltf>     no older export needed (a new character's)
#        python rig_now.py --from-json <pose.json> <out.gltf>                                    the same from a saved pose read, no editor
import json, os, struct, sys, time, urllib.request
import numpy as np
import gltf_rig, render

EDITOR_SCRIPT = r'''
# @Description: Read a clip's bone transforms (reference and per frame, local and component space) for Tools/VideoCapture
import unreal, json
clip = unreal.load_asset(CLIP); mesh = unreal.load_asset(MESH)
options = unreal.AnimPoseEvaluationOptions(optional_skeletal_mesh=mesh, evaluate_curves=False)
length = clip.get_editor_property('sequence_length')
def tr(x): return {'t': [x.translation.x, x.translation.y, x.translation.z], 'r': [x.rotation.x, x.rotation.y, x.rotation.z, x.rotation.w],
                   's': [x.scale3d.x, x.scale3d.y, x.scale3d.z]}
pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(clip, 0.0, options)
names = [str(n) for n in unreal.AnimPoseExtensions.get_bone_names(pose)]
L, S = unreal.AnimPoseSpaces.LOCAL, unreal.AnimPoseSpaces.WORLD
out = {'clip': CLIP, 'mesh': MESH, 'length': length, 'names': names,
       'reference': {n: tr(unreal.AnimPoseExtensions.get_ref_bone_pose(pose, n, L)) for n in names}, 'times': [], 'local': [], 'component': []}
for t in (TIMES or [length * f / max(1, FRAMES - 1) for f in range(FRAMES)]):
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(clip, t, options)
    out['times'].append(t)
    out['local'].append({n: tr(unreal.AnimPoseExtensions.get_bone_pose(pose, n, L)) for n in names})
    out['component'].append({n: tr(unreal.AnimPoseExtensions.get_bone_pose(pose, n, S)) for n in names})
open(OUT, 'w').write(json.dumps(out))
'''


def tool(name, body, timeout=120):
    req = urllib.request.Request('http://localhost:3000/mcp/tool/' + name, data=json.dumps(body).encode(), headers=render.HEADERS)
    return json.load(urllib.request.urlopen(req, timeout=timeout))


def read_clip(clip, mesh, frames, out, times=None):
    """Runs EDITOR_SCRIPT in the editor (bridge execute_script, a task) and returns what it wrote: `frames` evenly over the clip, or
    the given `times` (seconds)."""
    out = os.path.abspath(out).replace('\\', '/')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    if os.path.exists(out): os.remove(out)
    head = 'CLIP = %r\nMESH = %r\nFRAMES = %d\nTIMES = %r\nOUT = %r\n' % (clip, mesh, frames, list(times) if times else None, out)
    task = tool('execute_script', {'script_type': 'python', 'script_content': head + EDITOR_SCRIPT,
                                   'description': 'Read a clip\'s bone poses to a JSON file (Tools/VideoCapture/rig_now.py)'})['data']['task_id']
    for _ in range(600):
        state = tool('task_status', {'task_id': task})['data']['status']
        if state == 'completed': break
        if state in ('failed', 'cancelled', 'timed_out'): raise RuntimeError('pose read %s: %s' % (state, json.dumps(tool('task_result', {'task_id': task}))[:600]))
        time.sleep(0.5)
    result = tool('task_result', {'task_id': task})
    if not os.path.exists(out): raise RuntimeError('pose read wrote nothing: ' + json.dumps(result)[:600])
    return json.load(open(out))


# Unreal (X, Y, Z) = 100 * (glTF x, z, y) (gltf_rig.UNREAL): the bases differ by a mirror swapping the last two axes, so a turn about
# an axis keeps its axis (swapped) and reverses its sense.
def t_to_gltf(t): return [t[0] / 100.0, t[2] / 100.0, t[1] / 100.0]
def r_to_gltf(q): return [-q[0], -q[2], -q[1], q[3]]
def s_to_gltf(s): return [s[0], s[2], s[1]]


def qrot(q, v):
    u, w = np.array(q[:3], float), q[3]; v = np.asarray(v, float)
    return v + 2 * w * np.cross(u, v) + 2 * np.cross(u, np.cross(u, v))


def parents_of(dump):
    """Each bone's parent, read off the transforms (AnimPose names bones, not their hierarchy): the one bone whose component transform
    carries this bone's local to its component place, over three frames; None for the top bone. A tie goes to a real parent over None (a top
    bone at the identity, the MetaHuman's `root`, explains its child's place as well as no parent does)."""
    frames = [0, len(dump['local']) // 3, 2 * len(dump['local']) // 3]
    out = {}
    for b in dump['names']:
        best = (1e9, None)
        for p in [None] + [n for n in dump['names'] if n != b]:
            e = 0.0
            for f in frames:
                loc, cb = dump['local'][f][b], dump['component'][f][b]
                if p is None: e += np.linalg.norm(np.array(loc['t']) - cb['t'])
                else:
                    cp = dump['component'][f][p]
                    e += np.linalg.norm(np.array(cp['t']) + qrot(cp['r'], np.array(cp['s']) * loc['t']) - cb['t'])
            if p is None: e += 1e-7                                # cm: breaks the tie toward a real parent
            if e < best[0]: best = (e, p)
        out[b] = best[1]
    return out


def template_now(old, dump, out):
    """The old export with the current hierarchy and rest (nodes, inverse binds) and the dumped clip as its animation (its frame count
    must match). A bone the old export lacks is added (node, skin joint, inverse bind, keys); one the editor lacks refuses. Returns the
    bones whose parent changed and the bones added."""
    rig = gltf_rig.Rig(old)
    g = json.loads(json.dumps(rig.gltf)); data = bytearray(rig.buffers[0])
    missing = [n for n in rig.names if n not in dump['reference'] and not n.startswith('#')]
    if missing: raise RuntimeError('bones the editor does not have: %s' % missing)
    parent = parents_of(dump)
    names = list(rig.names)
    for n in dump['names']:
        if n not in names: names.append(n); g['nodes'].append({'name': n})
    holder = [i for i, n in enumerate(rig.names) if rig.parent[i] < 0 and n not in parent]      # the export's own top node ('#0')
    for node in g['nodes']: node.pop('children', None)
    for i, n in enumerate(names):
        if n in parent and parent[n] is not None: g['nodes'][names.index(parent[n])].setdefault('children', []).append(i)
    for h in holder: g['nodes'][h]['children'] = [i for i, n in enumerate(names) if n in parent and parent[n] is None]
    for i, n in enumerate(names):
        if n not in dump['reference']: continue
        ref = dump['reference'][n]
        g['nodes'][i].update({'translation': t_to_gltf(ref['t']), 'rotation': r_to_gltf(ref['r']), 'scale': s_to_gltf(ref['s'])})

    def put(acc_index, rows, width):
        acc = g['accessors'][acc_index]; view = g['bufferViews'][acc['bufferView']]
        start = view.get('byteOffset', 0) + acc.get('byteOffset', 0); stride = view.get('byteStride', 4 * width)
        for k, row in enumerate(rows): struct.pack_into('<%df' % width, data, start + k * stride, *row)
        acc['min'] = np.min(rows, 0).tolist(); acc['max'] = np.max(rows, 0).tolist()

    def append(rows, kind):
        while len(data) % 4: data.append(0)
        g['bufferViews'].append({'buffer': 0, 'byteOffset': len(data), 'byteLength': 4 * len(rows) * len(rows[0])})
        for row in rows: data.extend(struct.pack('<%df' % len(row), *row))
        g['accessors'].append({'bufferView': len(g['bufferViews']) - 1, 'componentType': 5126, 'count': len(rows), 'type': kind,
                               'min': np.min(rows, 0).tolist(), 'max': np.max(rows, 0).tolist()})
        return len(g['accessors']) - 1

    added = list(range(len(rig.names), len(names)))
    # Inverse binds from the new rest, in the skin's joint order (the added bones last).
    Wr, Pr = {}, {}
    def world(i):
        if i in Wr: return Wr[i], Pr[i]
        node = g['nodes'][i]
        R = gltf_rig.quat_to_matrix(node.get('rotation', [0, 0, 0, 1])) * np.array(node.get('scale', [1, 1, 1]))
        t = np.array(node.get('translation', [0, 0, 0]), float)
        up = [k for k, m in enumerate(g['nodes']) if i in m.get('children', [])]
        if up: Wp, Pp = world(up[0]); Wr[i], Pr[i] = Wp @ R, Pp + Wp @ t
        else: Wr[i], Pr[i] = R, t
        return Wr[i], Pr[i]
    for skin in g.get('skins', []):
        if 'inverseBindMatrices' not in skin: continue
        skin['joints'] = skin['joints'] + added
        rows = []
        for j in skin['joints']:
            W, P = world(j); M = np.eye(4); M[:3, :3] = W; M[:3, 3] = P
            rows.append(np.linalg.inv(M).T.ravel().tolist())               # glTF matrices are column-major
        skin['inverseBindMatrices'] = append(rows, 'MAT4')
    anim = g['animations'][0]
    times = anim['samplers'][anim['channels'][0]['sampler']]['input']
    for ch in anim['channels']:
        n = names[ch['target']['node']]; what = ch['target']['path']
        acc = anim['samplers'][ch['sampler']]['output']
        if g['accessors'][acc]['count'] != len(dump['local']): raise RuntimeError('frame count differs from the template')
        if n not in dump['reference']: continue
        if what == 'rotation': put(acc, [r_to_gltf(f[n]['r']) for f in dump['local']], 4)
        elif what == 'translation': put(acc, [t_to_gltf(f[n]['t']) for f in dump['local']], 3)
        elif what == 'scale': put(acc, [s_to_gltf(f[n]['s']) for f in dump['local']], 3)
    for i in added:
        n = names[i]
        for what, rows, kind in (('translation', [t_to_gltf(f[n]['t']) for f in dump['local']], 'VEC3'),
                                 ('rotation', [r_to_gltf(f[n]['r']) for f in dump['local']], 'VEC4'),
                                 ('scale', [s_to_gltf(f[n]['s']) for f in dump['local']], 'VEC3')):
            anim['samplers'].append({'input': times, 'output': append(rows, kind), 'interpolation': 'LINEAR'})
            anim['channels'].append({'sampler': len(anim['samplers']) - 1, 'target': {'node': i, 'path': what}})
    g['buffers'][0]['byteLength'] = len(data)
    folder = os.path.dirname(os.path.abspath(out)); os.makedirs(folder, exist_ok=True)
    base = os.path.splitext(os.path.basename(out))[0]
    g['buffers'][0]['uri'] = base + '.bin'
    for key in ('materials', 'images', 'textures', 'samplers'): g.pop(key, None)        # as write_gltf: a rig, not a look
    for mesh in g.get('meshes', []):
        for prim in mesh.get('primitives', []): prim.pop('material', None)
    open(os.path.join(folder, base + '.bin'), 'wb').write(bytes(data))
    json.dump(g, open(out, 'w'), indent=1)
    moved = {n: parent[n] for n in rig.names if n in parent and rig.parent[rig.index(n)] >= 0
             and parent[n] != rig.names[rig.parent[rig.index(n)]] and parent[n] is not None}
    return moved, [names[i] for i in added]


def template_scratch(dump, out, parents=None):
    """The rig template built from nothing but what EDITOR_SCRIPT reads (names, reference locals, parents, the clip's locals), the glTF a
    fresh export would give for hierarchy, rest and keys: an unnamed top node carrying the skin and the smallest skinned mesh (one triangle
    bound to the first top bone), every bone a node, one skin with inverse binds from the rest, one animation of every bone's translation,
    rotation and scale. Loads in glTF readers and gltf_rig.Rig; write_gltf.py and template_now() take it as a template."""
    names = list(dump['names'])
    parent = parents or dump.get('parents') or parents_of(dump)
    index = {n: i + 1 for i, n in enumerate(names)}
    nodes = [{'skin': 0, 'mesh': 0}]
    for n in names:
        ref = dump['reference'][n]
        nodes.append({'name': n, 'translation': t_to_gltf(ref['t']), 'rotation': r_to_gltf(ref['r']), 'scale': s_to_gltf(ref['s'])})
    for n in names: nodes[index[parent[n]] if parent.get(n) else 0].setdefault('children', []).append(index[n])
    data, views, accessors = bytearray(), [], []

    def add(array, ctype, kind, target=None, limits=True):
        while len(data) % 4: data.append(0)
        blob = np.ascontiguousarray(array).tobytes()
        views.append({'buffer': 0, 'byteOffset': len(data), 'byteLength': len(blob)})
        if target: views[-1]['target'] = target
        data.extend(blob)
        acc = {'bufferView': len(views) - 1, 'componentType': ctype, 'count': len(array), 'type': kind}
        if limits: acc['min'] = np.min(array, 0).reshape(-1).tolist(); acc['max'] = np.max(array, 0).reshape(-1).tolist()
        accessors.append(acc); return len(accessors) - 1

    W, P = {}, {}                                      # inverse binds from the rest, in the skin's joint order (column-major)
    def world(i):
        if i not in W:
            node = nodes[i]; R = gltf_rig.quat_to_matrix(node['rotation']) * np.array(node['scale']); t = np.array(node['translation'], float)
            up = parent.get(names[i - 1])
            if up: Wp, Pp = world(index[up]); W[i], P[i] = Wp @ R, Pp + Wp @ t
            else: W[i], P[i] = R, t
        return W[i], P[i]
    ibm = []
    for n in names:
        M = np.eye(4); M[:3, :3], M[:3, 3] = world(index[n]); ibm.append(np.linalg.inv(M).T.ravel())
    skin = {'inverseBindMatrices': add(np.array(ibm, '<f4').reshape(-1, 16), 5126, 'MAT4', limits=False), 'skeleton': 0, 'joints': list(range(1, len(names) + 1))}
    top = next(n for n in names if not parent.get(n))
    base = world(index[top])[1]
    tri = np.array([base, base + [0.01, 0, 0], base + [0, 0.01, 0]], '<f4')
    prim = {'attributes': {'POSITION': add(tri, 5126, 'VEC3', 34962),
                           'JOINTS_0': add(np.tile(np.array([names.index(top), 0, 0, 0], '<u2'), (3, 1)), 5123, 'VEC4', 34962, False),
                           'WEIGHTS_0': add(np.tile(np.array([1, 0, 0, 0], '<f4'), (3, 1)), 5126, 'VEC4', 34962, False)},
            'indices': add(np.array([0, 1, 2], '<u2'), 5123, 'SCALAR', 34963, False)}
    frames = len(dump['local'])
    times = dump.get('times') or [dump['length'] * f / max(1, frames - 1) for f in range(frames)]
    t_in = add(np.array(times, '<f4').reshape(-1, 1), 5126, 'SCALAR')
    samplers, channels = [], []
    for n in names:
        for what, rows, kind in (('translation', [t_to_gltf(f[n]['t']) for f in dump['local']], 'VEC3'),
                                 ('rotation', [r_to_gltf(f[n]['r']) for f in dump['local']], 'VEC4'),
                                 ('scale', [s_to_gltf(f[n]['s']) for f in dump['local']], 'VEC3')):
            samplers.append({'input': t_in, 'output': add(np.array(rows, '<f4'), 5126, kind), 'interpolation': 'LINEAR'})
            channels.append({'sampler': len(samplers) - 1, 'target': {'node': index[n], 'path': what}})
    folder = os.path.dirname(os.path.abspath(out)); os.makedirs(folder, exist_ok=True)
    stem = os.path.splitext(os.path.basename(out))[0]
    g = {'asset': {'version': '2.0', 'generator': 'Tools/VideoCapture/rig_now.py (from scratch)'}, 'scene': 0, 'scenes': [{'nodes': [0]}], 'nodes': nodes,
         'skins': [skin], 'meshes': [{'name': 'Template', 'primitives': [prim]}], 'accessors': accessors, 'bufferViews': views,
         'buffers': [{'uri': stem + '.bin', 'byteLength': len(data)}], 'animations': [{'name': 'Rest', 'samplers': samplers, 'channels': channels}]}
    open(os.path.join(folder, stem + '.bin'), 'wb').write(bytes(data))
    json.dump(g, open(out, 'w'), indent=1)
    return g


def check(path, dump):
    """The new glTF's forward kinematics against the editor's own component-space places (cm): the axes and composition are right."""
    rig = gltf_rig.Rig(path); err = []
    for f in range(len(dump['local'])):
        _, P = rig.frame(f); U = gltf_rig.to_unreal(P)
        err += [np.linalg.norm(U[i] - dump['component'][f][n]['t']) for i, n in enumerate(rig.names) if n in dump['component'][f]]
    return float(np.mean(err)), float(np.max(err))


if __name__ == '__main__':
    if sys.argv[1] == '--from-json':
        dump = json.load(open(sys.argv[2])); out = sys.argv[3]
        template_scratch(dump, out)
        print('%s: %d bones from scratch; forward kinematics vs the editor: mean %.4f cm, max %.4f cm' % ((out, len(dump['names'])) + check(out, dump)))
        sys.exit(0)
    if sys.argv[1] == '--scratch':
        clip, mesh, frames, out = sys.argv[2:6]
        dump = read_clip(clip, mesh, int(frames), os.path.splitext(out)[0] + '.pose.json')
        template_scratch(dump, out)
        print('%s: %d bones from scratch, length %.3f s; forward kinematics vs the editor: mean %.4f cm, max %.4f cm' % ((out, len(dump['names']), dump['length']) + check(out, dump)))
        sys.exit(0)
    old, clip, mesh, frames, out = sys.argv[1:6]
    dump = read_clip(clip, mesh, int(frames), os.path.splitext(out)[0] + '.pose.json')
    moved, added = template_now(old, dump, out)
    print('parents changed since the old export: %s; bones added: %s' % (moved, added))
    print('%s: %d bones, length %.3f s; forward kinematics vs the editor: mean %.4f cm, max %.4f cm' % ((out, len(dump['names']), dump['length']) + check(out, dump)))
