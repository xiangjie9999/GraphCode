# Bone rotations from 3D joint places, on the character's own skeleton (its glTF export's rest pose). Top-down: a bone with two or
# more tracked children takes the rotation that best carries its rest child vectors onto the seen ones (Kabsch); a bone with one
# tracked child points at it and takes its roll from the plane that child bends in toward its own child (the child's hinge axis is
# fixed in the bone's frame); a bone with none keeps its rest turn relative to its parent. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E4).
# Usage: python fit.py <points.json> <rig.gltf> <out.json> [truth.gltf]
import json, sys
import numpy as np
import gltf_rig

HELPERS = ('twist',)


def to_gltf(u):
    """Unreal component cm -> glTF metres (inverse of gltf_rig.to_unreal)."""
    u = np.asarray(u, float)
    return np.array([u[0], u[2], u[1]]) / 100.0


def kabsch(V, W, weights=None):
    V = np.asarray(V, float); W = np.asarray(W, float)
    w = np.ones(len(V)) if weights is None else np.asarray(weights, float)
    H = (V * w[:, None]).T @ W
    U, _, Vt = np.linalg.svd(H)
    d = np.sign(np.linalg.det(Vt.T @ U.T))
    return Vt.T @ np.diag([1, 1, d]) @ U.T


def unit(v):
    n = np.linalg.norm(v)
    return v / n if n > 1e-9 else v


def fit_frame(rig, seen, previous=None):
    """seen: name -> glTF world position for the tracked joints. Returns local rotations (quaternions) and the pelvis translation."""
    n = len(rig.names)
    W = np.zeros((n, 3, 3)); P = np.zeros((n, 3))
    local_r = rig.rest_r.copy(); local_t = rig.rest_t.copy()
    children = {i: [c for c in range(n) if rig.parent[c] == i and not any(h in rig.names[c] for h in HELPERS)] for i in range(n)}
    rest_R = [gltf_rig.quat_to_matrix(q) for q in rig.rest_r]
    for i in rig.order:
        p = rig.parent[i]
        name = rig.names[i]
        if p < 0:
            W[i] = rest_R[i]; P[i] = rig.rest_t[i]; continue
        if name in seen and name == 'pelvis':                      # the body's place: the pelvis is shifted, not only turned
            local_t[i] = W[p].T @ (seen[name] - P[p])
        P[i] = P[p] + W[p] @ local_t[i]
        here = seen.get(name, P[i])
        tracked = [c for c in children[i] if rig.names[c] in seen]
        if any(h in name for h in HELPERS) or name == 'root':
            W[i] = W[p] @ rest_R[i]
        elif len(tracked) >= 2:
            V = [rig.rest_t[c] for c in tracked]
            Wv = [seen[rig.names[c]] - here for c in tracked]
            W[i] = kabsch(V, Wv)
        elif len(tracked) == 1:
            c = tracked[0]
            v1 = rig.rest_t[c]; w1 = seen[rig.names[c]] - here
            grand = [g for g in children[c] if rig.names[g] in seen]
            R = None
            if grand:
                g = grand[0]
                v2 = np.cross(v1, rest_R[c] @ rig.rest_t[g])
                w2 = np.cross(w1, seen[rig.names[g]] - seen[rig.names[c]])
                if np.linalg.norm(v2) > 1e-6 * np.linalg.norm(v1) ** 2 and np.linalg.norm(w2) > 0.02 * np.linalg.norm(w1) * np.linalg.norm(seen[rig.names[g]] - seen[rig.names[c]]):
                    R = kabsch([unit(v1), unit(v2)], [unit(w1), unit(w2)], [1.0, 0.5])
            if R is None:                                              # straight or no grandchild: the least turn from the rest
                base = W[p] @ rest_R[i] if previous is None else W[p] @ gltf_rig.quat_to_matrix(previous[i])
                a = unit(base @ v1); b = unit(w1)
                axis = np.cross(a, b); s = np.linalg.norm(axis); c_ = np.dot(a, b)
                if s < 1e-9: R = base
                else:
                    k = axis / s; K = np.array([[0, -k[2], k[1]], [k[2], 0, -k[0]], [-k[1], k[0], 0]])
                    R = (np.eye(3) + s * K + (1 - c_) * K @ K) @ base
            W[i] = R
        else:
            W[i] = W[p] @ rest_R[i]
        local_r[i] = gltf_rig.matrix_to_quat(W[p].T @ W[i])
    return local_r, local_t


def fit(points, rig):
    bones = points['bones']; frames = points['frames']
    out_r, out_t = [], []
    previous = None
    for f in range(frames):
        seen = {b: to_gltf(points['points'][b][f]) for b in bones if points['points'][b][f] is not None}
        r, t = fit_frame(rig, seen, previous)
        out_r.append(r); out_t.append(t); previous = r
    return np.array(out_r), np.array(out_t)


def angle(q1, q2):
    d = abs(float(np.dot(q1, q2)))
    return np.degrees(2 * np.arccos(min(1.0, d)))


if __name__ == '__main__':
    points = json.load(open(sys.argv[1]))
    rig = gltf_rig.Rig(sys.argv[2])
    R, T = fit(points, rig)
    json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': T.tolist()}, open(sys.argv[3], 'w'))
    if len(sys.argv) > 4:
        truth = gltf_rig.Rig(sys.argv[4])
        rows = []
        for i, name in enumerate(rig.names):
            if name in ('#0', 'root') or any(h in name for h in HELPERS): continue
            e = [angle(R[f, i], truth.anim_r[f, i]) for f in range(len(R))]
            # direction of the bone toward its first child, world space, captured against truth
            rows.append((max(e), name, np.mean(e)))
        for worst, name, mean in sorted(rows, reverse=True):
            print('%-17s local turn error mean %6.2f deg, max %6.2f deg' % (name, mean, worst))
