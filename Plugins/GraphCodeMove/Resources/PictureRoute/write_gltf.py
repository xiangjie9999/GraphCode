# Writes a captured take as a glTF animation on the character's own skeleton: a copy of its glTF export (same nodes, rest, mesh and
# skin) with the animation's keys replaced: every bone's rotation, the pelvis's translation, the root's travel recovered from the planted
# paws (travel.py: a video played in place carries none of its own), twist helpers from their drivers by the rig's rule (twist_helpers.py) when a calibration clip is given. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E5).
# Usage: python write_gltf.py <template.gltf> <fit.json> <out.gltf>
import json, os, shutil, struct, sys
import numpy as np
import gltf_rig


def main():
    template, fitted, out = sys.argv[1], sys.argv[2], sys.argv[3]
    calibration_clip = sys.argv[4] if len(sys.argv) > 4 else None   # another clip of the rig: the twist helpers' drivers and shares
    rig = gltf_rig.Rig(template)
    fit = json.load(open(fitted))
    R = np.array(fit['rotations']); T = np.array(fit['translations'])
    if calibration_clip:
        import twist_helpers
        R = twist_helpers.apply(rig, R, twist_helpers.calibrate(gltf_rig.Rig(calibration_clip)))
        helpers_from_drivers = True
    else:
        helpers_from_drivers = False
    import travel
    root_track, _, _ = travel.root_travel(rig, R, T)
    write(rig, R, T, root_track, out, helpers_from_drivers)


def resize(g, data, n, fps):
    """The template's animation made `n` keys long at `fps`: new key times and a new output for every channel, appended to the buffer
    (a video take is as long as the video, the template as long as its clip)."""
    def append(rows, kind):
        while len(data) % 4: data.append(0)
        g['bufferViews'].append({'buffer': 0, 'byteOffset': len(data), 'byteLength': 4 * len(rows) * len(rows[0])})
        for row in rows: data.extend(struct.pack('<%df' % len(row), *row))
        g['accessors'].append({'bufferView': len(g['bufferViews']) - 1, 'componentType': 5126, 'count': len(rows), 'type': kind,
                               'min': np.min(rows, 0).tolist(), 'max': np.max(rows, 0).tolist()})
        return len(g['accessors']) - 1
    anim = g['animations'][0]
    times = append([[f / fps] for f in range(n)], 'SCALAR')
    for s in anim['samplers']:
        kind = g['accessors'][s['output']]['type']
        s['input'] = times; s['output'] = append([[0.0] * (4 if kind == 'VEC4' else 3)] * n, kind)
    g['buffers'][0]['byteLength'] = len(data)


def write(rig, R, T, root_track, out, helpers_from_drivers=True, fps=30.0):
    """The template's glTF with its keys replaced: R every node's rotation per frame, T the pelvis's translation, root_track the root's;
    the template's own keys when the take has as many frames, otherwise len(R) keys at `fps`."""
    g = json.loads(json.dumps(rig.gltf))
    data = bytearray(rig.buffers[0])
    anim = g['animations'][0]
    if g['accessors'][anim['samplers'][0]['output']]['count'] != len(R): resize(g, data, len(R), fps)
    frames = None
    for ch in anim['channels']:
        node, what = ch['target']['node'], ch['target']['path']
        acc = g['accessors'][anim['samplers'][ch['sampler']]['output']]
        view = g['bufferViews'][acc['bufferView']]
        count = acc['count']
        frames = count if frames is None else frames
        start = view.get('byteOffset', 0) + acc.get('byteOffset', 0)
        name = rig.names[node]
        if what == 'scale':
            values = np.repeat(rig.rest_s[node][None], count, 0)
            stride = view.get('byteStride', 12)
            for k in range(count): struct.pack_into('<3f', data, start + k * stride, *values[k])
            acc['min'] = values.min(0).tolist(); acc['max'] = values.max(0).tolist()
        if what == 'rotation':
            values = np.repeat(rig.rest_r[node][None], count, 0) if ('twist' in name and not helpers_from_drivers) else R[:count, node]
            stride = view.get('byteStride', 16)
            for k in range(count): struct.pack_into('<4f', data, start + k * stride, *values[k])
            acc['min'] = values.min(0).tolist(); acc['max'] = values.max(0).tolist()
        elif what == 'translation':
            values = T[:count, node] if name == 'pelvis' else root_track[:count] if name == 'root' else np.repeat(rig.rest_t[node][None], count, 0)
            stride = view.get('byteStride', 12)
            for k in range(count): struct.pack_into('<3f', data, start + k * stride, *values[k])
            acc['min'] = values.min(0).tolist(); acc['max'] = values.max(0).tolist()
    folder = os.path.dirname(os.path.abspath(out)); os.makedirs(folder, exist_ok=True)
    base = os.path.splitext(os.path.basename(out))[0]
    g['buffers'][0]['uri'] = base + '.bin'
    open(os.path.join(folder, base + '.bin'), 'wb').write(bytes(data))
    # A take is an animation: no materials or textures, so an import onto the character's skeleton has nothing else to create (2026-10-18:
    # Interchange wrote the template's material beside the clip despite MaterialImport=DoNotImport, and the second import stopped on a
    # modal "override asset?" for it, holding the editor's bridge until answered).
    for key in ('materials', 'images', 'textures', 'samplers'): g.pop(key, None)
    for mesh in g.get('meshes', []):
        for prim in mesh.get('primitives', []): prim.pop('material', None)
    json.dump(g, open(out, 'w'), indent=1)
    print('wrote %s (%d frames, travel %.2f cm)' % (out, frames, 100 * (root_track[-1][2] - root_track[0][2])))


if __name__ == '__main__':
    main()
