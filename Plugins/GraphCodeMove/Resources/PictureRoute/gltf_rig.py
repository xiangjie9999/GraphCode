# A character's skeleton and animation read from Unreal's glTF export (gc_asset mode:"export" on an AnimSequence), with
# forward kinematics: each bone's world place per frame, in Unreal's own axes and centimetres. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
import json, os, struct
import numpy as np

COMPONENTS = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}
FORMATS = {5126: ('f', 4), 5125: ('I', 4), 5123: ('H', 2), 5121: ('B', 1)}


def quat_to_matrix(q):
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def matrix_to_quat(m):
    t = np.trace(m)
    if t > 0:
        s = np.sqrt(t + 1.0) * 2
        q = [(m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s, 0.25 * s]
    elif m[0, 0] > m[1, 1] and m[0, 0] > m[2, 2]:
        s = np.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2]) * 2
        q = [0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s, (m[2, 1] - m[1, 2]) / s]
    elif m[1, 1] > m[2, 2]:
        s = np.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2]) * 2
        q = [(m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s, (m[0, 2] - m[2, 0]) / s]
    else:
        s = np.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1]) * 2
        q = [(m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s, (m[1, 0] - m[0, 1]) / s]
    q = np.array(q)
    return q / np.linalg.norm(q)


class Rig:
    """The glTF's nodes as a skeleton: names, parents, rest local transforms, and the animation's per-frame locals."""

    def __init__(self, path):
        self.path = path
        self.gltf = json.load(open(path, encoding='utf-8'))
        folder = os.path.dirname(path)
        self.buffers = [open(os.path.join(folder, b['uri']), 'rb').read() for b in self.gltf['buffers']]
        nodes = self.gltf['nodes']
        self.names = [n.get('name', '#%d' % i) for i, n in enumerate(nodes)]
        self.parent = [-1] * len(nodes)
        for i, n in enumerate(nodes):
            for c in n.get('children', []):
                self.parent[c] = i
        self.rest_t = np.array([n.get('translation', [0, 0, 0]) for n in nodes], float)
        self.rest_r = np.array([n.get('rotation', [0, 0, 0, 1]) for n in nodes], float)
        self.rest_s = np.array([n.get('scale', [1, 1, 1]) for n in nodes], float)
        self.order = []
        seen = set()
        def visit(i):
            if i in seen: return
            if self.parent[i] >= 0: visit(self.parent[i])
            seen.add(i); self.order.append(i)
        for i in range(len(nodes)): visit(i)
        self.times, self.anim_t, self.anim_r = None, None, None
        if self.gltf.get('animations'):
            self._read_animation(self.gltf['animations'][0])

    def accessor(self, index):
        a = self.gltf['accessors'][index]
        view = self.gltf['bufferViews'][a['bufferView']]
        fmt, size = FORMATS[a['componentType']]
        n = COMPONENTS[a['type']]
        start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
        stride = view.get('byteStride', n * size)
        data = self.buffers[view['buffer']]
        out = np.empty((a['count'], n))
        for k in range(a['count']):
            out[k] = struct.unpack_from('<%d%s' % (n, fmt), data, start + k * stride)
        return out

    def _read_animation(self, anim):
        frames = None
        t = {}; r = {}
        for ch in anim['channels']:
            s = anim['samplers'][ch['sampler']]
            node, what = ch['target']['node'], ch['target']['path']
            times = self.accessor(s['input'])[:, 0]
            values = self.accessor(s['output'])
            if frames is None: frames = times
            if what == 'translation': t[node] = values
            elif what == 'rotation': r[node] = values
        self.times = frames
        count = len(frames)
        self.anim_t = np.repeat(self.rest_t[None], count, 0)
        self.anim_r = np.repeat(self.rest_r[None], count, 0)
        for node, v in t.items(): self.anim_t[:, node] = v
        for node, v in r.items(): self.anim_r[:, node] = v

    def index(self, name):
        return self.names.index(name)

    def globals(self, local_t, local_r):
        """World (glTF) rotation matrices and positions of every node, from locals (rows per node)."""
        n = len(self.names)
        R = np.zeros((n, 3, 3)); P = np.zeros((n, 3))
        for i in self.order:
            Rl = quat_to_matrix(local_r[i]) * self.rest_s[i]
            p = self.parent[i]
            if p < 0:
                R[i] = Rl; P[i] = local_t[i]
            else:
                R[i] = R[p] @ Rl; P[i] = P[p] + R[p] @ local_t[i]
        return R, P

    def frame(self, f):
        """World positions (glTF metres) of every node at animation frame f; f=None is the rest pose."""
        if f is None: return self.globals(self.rest_t, self.rest_r)
        return self.globals(self.anim_t[f], self.anim_r[f])


# glTF is Y up, right-handed, metres; Unreal is Z up, left-handed, centimetres. Measured 2026-10-18 on the cat export against the
# rig's own joint places (.gcrig points, 38 joints): Unreal (X, Y, Z) = 100 * (glTF x, z, y), no offset, worst joint 0.002 cm.
UNREAL = [(0, 1), (2, 1), (1, 1)]


def to_unreal(p, mapping=UNREAL):
    p = np.asarray(p, float)
    out = np.empty(p.shape)
    for k, (axis, sign) in enumerate(mapping):
        out[..., k] = sign * p[..., axis] * 100.0
    return out
