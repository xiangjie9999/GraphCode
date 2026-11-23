# 3D joint places from two or more views: linear triangulation (DLT) refined by least squares on the reprojection, in the cat's own
# space (the preview's placement taken off). With a truth glTF, the error per joint per frame. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E2, E3).
# Usage: python triangulate.py <obs.json> <out.json> [truth.gltf]
import json, sys
import numpy as np
from scipy.optimize import least_squares
import camera, gltf_rig


def dlt(Ps, xs):
    A = []
    for P, (x, y) in zip(Ps, xs):
        A.append(x * P[2] - P[0]); A.append(y * P[2] - P[1])
    _, _, vt = np.linalg.svd(np.array(A))
    X = vt[-1]
    return X[:3] / X[3]


def refine(cams, xs, X0):
    r = least_squares(lambda X: np.concatenate([c.project(X)[0] - x for c, x in zip(cams, xs)]), X0)
    return r.x, np.sqrt((r.fun ** 2).reshape(-1, 2).sum(1)).max()


def triangulate(obs):
    names = list(obs['cameras'])
    cams = [camera.Camera.from_render(obs['cameras'][n]['camera'], obs['width'], obs['height']) for n in names]
    origins = [np.array(obs['cameras'][n]['origin'] or [0, 0, 0]) for n in names]
    bones = list(obs['cameras'][names[0]]['balls'])
    out = {'bones': bones, 'frames': obs['frames'], 'points': {}, 'reprojection': {}}
    for b in bones:
        pts, reps = [], []
        for f in range(obs['frames']):
            use = [(c, np.array(obs['cameras'][n]['balls'][b][f]), o) for c, n, o in zip(cams, names, origins) if obs['cameras'][n]['balls'][b][f]]
            if len(use) < 2: pts.append(None); reps.append(None); continue
            X0 = dlt([c.matrix() for c, _, _ in use], [x for _, x, _ in use])
            X, rep = refine([c for c, _, _ in use], [x for _, x, _ in use], X0)
            pts.append((X - use[0][2]).tolist()); reps.append(float(rep))
        out['points'][b] = pts; out['reprojection'][b] = reps
    return out


def truth_points(path, bones, frames):
    rig = gltf_rig.Rig(path)
    root = rig.index('root')
    T = {}
    for f in range(frames):
        t = rig.anim_t[f].copy(); t[root] = rig.rest_t[root]          # in place, as the preview plays it
        _, P = rig.globals(t, rig.anim_r[f])
        U = gltf_rig.to_unreal(P)
        for b in bones: T.setdefault(b, []).append(U[rig.index(b)])
    return T


if __name__ == '__main__':
    obs = json.load(open(sys.argv[1]))
    out = triangulate(obs)
    json.dump(out, open(sys.argv[2], 'w'), indent=1)
    if len(sys.argv) > 3:
        T = truth_points(sys.argv[3], out['bones'], out['frames'])
        worst = []
        for b in out['bones']:
            e = [np.linalg.norm(np.array(p) - t) for p, t in zip(out['points'][b], T[b]) if p is not None]
            r = [x for x in out['reprojection'][b] if x is not None]
            worst.append((max(e), b))
            print('%-17s error mean %.3f cm, max %.3f cm; reprojection max %.2f px; %d frames' % (b, np.mean(e), max(e), max(r), len(e)))
        allerr = [np.linalg.norm(np.array(p) - t) for b in out['bones'] for p, t in zip(out['points'][b], T[b]) if p is not None]
        print('ALL JOINTS: mean %.3f cm, 95%% %.3f cm, max %.3f cm (%s)' % (np.mean(allerr), np.percentile(allerr, 95), max(worst)[0], max(worst)[1]))
