# A take fitted straight from 2D joint balls, one camera or several, frame by frame (fit_rig.solve_views), started at the rig's rest
# for frame 0 and from the frame before after that; a loop also backward, each frame keeping the pass that fits the path (run). Writes the same fit.json fit_rig does. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E6).
# Usage: python fit_views.py <obs.json> <camera,camera|all> <rig.gltf> <ikrig.gc> <calibration.gltf> <out fit.json> [truth.gltf]
#        python fit_views.py report <fit.json> <truth.gltf>
import json, sys
import numpy as np
import camera, fit_rig, gltf_rig


def views_at(obs, cams, f, rolls=True, long_px=50.0):
    """Each camera's balls at frame f, and (rolls) the drawn bones' rolls it read (track_all.roll_at), as solve_views takes them: only
    on a bone at least `long_px` long in the picture (E12: shoulders, hips, elbows read 0.1-1 px off; wrists, paws, tail pieces 3-5 px,
    their own size, and a wrist's bad roll turned its elbow 11 degrees wrong)."""
    out = []
    for cam, origin, n in cams:
        c = obs['cameras'][n]
        seen = {b: np.array(v[f]) for b, v in c['balls'].items() if v[f] is not None}
        r = {b: (v['child'], v['q'][f], v.get('s', [None] * (f + 1))[f]) for b, v in c.get('rolls', {}).items()
             if rolls and (v['q'][f] is not None or v.get('s', [None] * (f + 1))[f] is not None) and b in seen and v['child'] in seen
             and np.linalg.norm(seen[v['child']] - seen[b]) >= long_px}
        # A learned reader's points carry its confidence (dino_track: the peak's score); drawn balls count 1.
        conf = c.get('conf')
        weight = {b: max(0.05, conf[b][f]) for b in seen if conf and conf.get(b) and conf[b][f] is not None} if conf else 1.0
        out.append((cam, origin, seen, weight, r))
    return out


def first_pose(first_json, rig):
    """first.json's 'pose' ({bone: glTF local rotation / translation}, written by track_all.first_frame from the clip itself) in the
    rig's node order, rest where it names nothing; None when the take has no known first pose."""
    pose = json.load(open(first_json)).get('pose') if isinstance(first_json, str) else first_json
    if not pose: return None
    R, T = rig.rest_r.copy(), rig.rest_t.copy()
    for i, n in enumerate(rig.names):
        if n in pose['rotations']: R[i] = pose['rotations'][n]
        if n in pose['translations']: T[i] = pose['translations'][n]
    return {'rotations': R, 'translations': T}


def one_pass(model, obs, cams, frames, start, smooth, loss, scale, rolls=True, previous=None):
    """Frame by frame in the given order, each from the frame before (the first from `previous` when the pose before it is known);
    the parameters and the pixel errors per frame."""
    p, out = start, {}
    for f in frames:
        p, errs = fit_rig.solve_views(model, views_at(obs, cams, f, rolls), p, previous, smooth, loss, scale)
        previous = p; out[f] = (p, np.concatenate(errs))
    return out


def run(obs, names, rig, axes, orders, limits=None, smooth=0.05, loss='linear', scale=1.0, loop=False, rolls=True, first=None):
    """`loop` (the take's last frame is its first again): a second pass runs backward from the last frame, started at the first
    frame's fit, and each frame keeps the forward or backward fit along the path that moves least and closes on itself. Where a far
    limb's two depth branches meet in one view, a pass can leave on the wrong one and keep it (2026-10-18, E8: a hind paw 30 cm off
    for the second half of the cycle); the pass from the other end arrives on the right one. `first`: the first frame's pose when it
    is known (a video made from a frame we rendered: the clip's own pose there, `first.json` 'pose'), {'rotations', 'translations'}
    per node; the forward pass starts from it and the first frame is tied to it."""
    model = fit_rig.Model(rig, axes, orders, limits)
    cams = [(camera.Camera.from_render(obs['cameras'][n]['camera'], obs['width'], obs['height']), obs['cameras'][n]['origin'] or [0, 0, 0], n) for n in names]
    n = obs['frames']
    p0 = np.zeros(model.size); p0[:3] = rig.rest_t[model.pelvis]
    known = model.params_of(np.array(first['rotations']), np.array(first['translations'])) if first else None
    passes = [one_pass(model, obs, cams, range(n), p0 if known is None else known, smooth, loss, scale, rolls, known)]
    if loop: passes.append(one_pass(model, obs, cams, range(n - 1, -1, -1), passes[0][0][0], smooth, loss, scale, rolls))
    rho = (lambda e: np.sum(2 * (np.sqrt(1 + (e / scale) ** 2) - 1) * scale ** 2)) if loss == 'soft_l1' else (lambda e: np.sum(e ** 2))
    cost = np.array([[rho(c[f][1]) for c in passes] for f in range(n)])
    J = np.array([[model.positions(c[f][0]) * 100.0 for c in passes] for f in range(n)])       # cm
    move = lambda a, b: np.sum((a - b) ** 2)
    best = None
    for first in range(len(passes)):
        D = np.full(len(passes), np.inf); D[first] = cost[0, first]; back = []
        for f in range(1, n):
            step = np.array([[D[i] + move(J[f - 1, i], J[f, j]) for i in range(len(passes))] for j in range(len(passes))])
            back.append(step.argmin(1)); D = step.min(1) + cost[f]
        if loop: D = D + 10.0 * np.array([move(J[n - 1, j], J[0, first]) for j in range(len(passes))])
        k = int(D.argmin())
        if best is None or D[k] < best[0]:
            path = [k]
            for b in reversed(back): path.append(int(b[path[-1]]))
            best = (D[k], path[::-1])
    R, T, res = [], [], []
    for f, k in enumerate(best[1]):
        Rl, t = model.locals(passes[k][f][0])
        R.append([gltf_rig.matrix_to_quat(r) for r in Rl]); T.append(t); res.append(float(passes[k][f][1].mean()))
    return np.array(R), np.array(T), res, model


def report(R, T, truth):
    """A fit against the clip it was captured from, frame for frame, in place: joint places (cm) and bone turns (degrees)."""
    root = truth.index('root'); errs = []
    for f in range(len(R)):
        t_true = truth.anim_t[f].copy(); t_true[root] = truth.rest_t[root]
        _, Pt = truth.globals(t_true, truth.anim_r[f])
        _, Pc = truth.globals(T[f], R[f])
        for i, n in enumerate(truth.names):
            if n in ('#0', 'root') or 'twist' in n: continue
            errs.append(np.linalg.norm(Pt[i] - Pc[i]) * 100.0)
    print('joint places vs truth: mean %.2f cm, 95%% %.2f cm, max %.2f cm' % (np.mean(errs), np.percentile(errs, 95), max(errs)))
    rows = []
    for i, name in enumerate(truth.names):
        if name in ('#0', 'root') or 'twist' in name: continue
        e = [np.degrees(2 * np.arccos(min(1.0, abs(float(np.dot(R[f, i], truth.anim_r[f, i])))))) for f in range(len(R))]
        rows.append((max(e), name, np.mean(e)))
    rows.sort(reverse=True)
    print('bone turns: median of each bone\'s worst %.2f deg; worst: %s' % (np.median([r[0] for r in rows]), ', '.join('%s %.1f/%.1f' % (n, m, w) for w, n, m in rows[:6])))


if __name__ == '__main__':
    if sys.argv[1] == 'report':                      # python fit_views.py report <fit.json> <truth.gltf>
        fit = json.load(open(sys.argv[2])); report(np.array(fit['rotations']), np.array(fit['translations']), gltf_rig.Rig(sys.argv[3]))
        sys.exit(0)
    obs = json.load(open(sys.argv[1]))
    names = list(obs['cameras']) if sys.argv[2] == 'all' else sys.argv[2].split(',')
    rig = gltf_rig.Rig(sys.argv[3]); axes = fit_rig.ik_rig_axes(sys.argv[4])
    orders = fit_rig.rig_orders(gltf_rig.Rig(sys.argv[5]), axes, rig.names)
    R, T, res, model = run(obs, names, rig, axes, orders, fit_rig.ik_rig_limits(sys.argv[4]))
    json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': T.tolist(), 'reprojection_px': res}, open(sys.argv[6], 'w'))
    print('views %s: mean reprojection %.2f px' % (names, np.mean(res)))
    if len(sys.argv) > 7: report(R, T, gltf_rig.Rig(sys.argv[7]))
