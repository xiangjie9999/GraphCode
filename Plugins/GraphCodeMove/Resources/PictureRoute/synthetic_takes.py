# E8: takes like an AI video's. The same walk, each generation timed a little differently (a phase offset and an uneven pace), swinging
# a little more or less, seen from ONE camera (two AI generations are two motions, so they never triangulate together), its joint balls
# a few pixels off and now and then badly wrong. Each take is fitted alone (fit_views), then the takes are averaged by phase; this
# measures how many takes the average needs and which alignment holds. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E8).
# Usage: python synthetic_takes.py make <out_dir> <first> <count> [noise_px] [outlier_rate] [loss] [scale_px] [loop]   (a take's fit ~1 min;
#        run takes in parallel)
#        python synthetic_takes.py average <out_dir>
#        python synthetic_takes.py score <fit.json> [label]   (a fused or averaged take against the walk as authored)
import json, os, sys
import numpy as np
import camera, fit_rig, fit_views, gltf_rig
from average_takes import cycle, at_phase, mean_quats, joints, features, align, average
from capture import CAMERAS, BONES, WIDTH, HEIGHT

HERE = os.path.dirname(os.path.abspath(__file__))
TRUTH = os.path.join(HERE, '..', '..', 'Saved', 'Research', 'VideoCapture', 'walk-level', 'truth', 'A_CatWalkAccel.gltf')
CALIB = os.path.join(HERE, '..', '..', 'Saved', 'Research', 'VideoCapture', 'truth', 'A_CatWalkAccel.gltf')
IKRIG = os.path.join(HERE, '..', '..', 'Content', '__MCPTests', 'CatMove', 'IKR_BritishShorthairCat.gc')
ORIGIN = [-2.937, 1.016, 1.000]           # the preview's placement of the cat (render manifest, bubbles[].origin)


def qmul(a, b):
    ax, ay, az, aw = a; bx, by, bz, bw = b
    return np.array([aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                     aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz])


def qinv(q): return np.array([-q[0], -q[1], -q[2], q[3]])


def qpow(q, g):
    q = q if q[3] >= 0 else -q
    angle = 2 * np.arccos(np.clip(q[3], -1, 1))
    if angle < 1e-9: return np.array([0, 0, 0, 1.0])
    axis = q[:3] / np.sin(angle / 2)
    return np.concatenate([axis * np.sin(angle * g / 2), [np.cos(angle * g / 2)]])


def make(out, first, count, noise, outliers, loss='linear', scale=1.0, loop=False):
    os.makedirs(out, exist_ok=True)
    truth = gltf_rig.Rig(TRUTH)
    R0, T0 = cycle(truth.anim_r, truth.anim_t)
    n = len(R0)
    mean_r = np.array([mean_quats(R0[:, b]) for b in range(R0.shape[1])]); mean_t = T0.mean(0)
    axes = fit_rig.ik_rig_axes(IKRIG); orders = fit_rig.rig_orders(gltf_rig.Rig(CALIB), axes, truth.names)
    limits = fit_rig.ik_rig_limits(IKRIG)
    pelvis = truth.index('pelvis'); root = truth.index('root')
    for k in range(first, first + count):
        rng = np.random.default_rng(1000 + k)
        offset, pace, lean, gain = rng.uniform(0, 1), rng.uniform(0.1, 0.3), rng.uniform(0, 2 * np.pi), rng.uniform(0.9, 1.1)
        cam_name = list(CAMERAS)[k % len(CAMERAS)]
        cam = camera.Camera.from_render(CAMERAS[cam_name], WIDTH, HEIGHT)
        balls = {b: [] for b in BONES}
        shown = []
        for f in range(n + 1):
            u = f / n
            phase = offset + u + pace * np.sin(2 * np.pi * u + lean) / (2 * np.pi) - pace * np.sin(lean) / (2 * np.pi)
            r, t = at_phase(R0, T0, phase)
            r = np.array([qmul(mean_r[b], qpow(qmul(qinv(mean_r[b]), r[b]), gain)) for b in range(len(r))])
            t = t.copy(); t[pelvis] = mean_t[pelvis] + gain * (t[pelvis] - mean_t[pelvis]); t[root] = truth.rest_t[root]
            shown.append(phase % 1.0)
            P = gltf_rig.to_unreal(truth.globals(t, r)[1])
            for b in BONES:
                x = cam.project(P[truth.index(b)] + ORIGIN)[0] + rng.normal(0, noise, 2)
                if rng.uniform() < outliers:
                    a = rng.uniform(0, 2 * np.pi); x = x + rng.uniform(15, 40) * np.array([np.cos(a), np.sin(a)])
                balls[b].append(x.tolist())
        obs = {'frames': n + 1, 'width': WIDTH, 'height': HEIGHT,
               'cameras': {cam_name: {'camera': CAMERAS[cam_name], 'origin': ORIGIN, 'balls': balls}}}
        json.dump(obs, open(os.path.join(out, 'obs_%02d.json' % k), 'w'))
        R, T, res, _ = fit_views.run(obs, [cam_name], truth, axes, orders, limits, loss=loss, scale=scale, loop=loop)
        json.dump({'names': truth.names, 'rotations': R.tolist(), 'translations': T.tolist(), 'camera': cam_name,
                   'offset': offset, 'pace': pace, 'gain': gain, 'shown_phase': shown, 'reprojection_px': res},
                  open(os.path.join(out, 'take_%02d.json' % k), 'w'))
        print('take %d (%s, pace %.2f, gain %.2f): reprojection %.2f px' % (k, cam_name, pace, gain, np.mean(res)), flush=True)


def score(rig, R, T):
    """Against the walk as authored, after the same alignment (timing set aside): joint places (cm) and bone turns (degrees)."""
    tR, tT = cycle(rig.anim_r, rig.anim_t)
    tf = features(rig, tR, tT)
    aR, aT = align(tf, R, T, features(rig, R, T), True)
    Jt, Ja = joints(rig, tR, tT), joints(rig, aR, aT)
    keep = [i for i, n in enumerate(rig.names) if n not in ('#0', 'root') and 'twist' not in n]
    e = np.linalg.norm(Jt[:, keep] - Ja[:, keep], axis=2).ravel() * 100
    worst = [max(np.degrees(2 * np.arccos(min(1.0, abs(float(np.dot(aR[f, i], tR[f, i])))))) for f in range(len(tR))) for i in keep]
    return np.mean(e), np.percentile(e, 95), e.max(), np.median(worst)


def shown(rig, take):
    """The motion the take's video showed, frame by frame (its phases and gain, no noise)."""
    R0, T0 = cycle(rig.anim_r, rig.anim_t)
    mean_r = np.array([mean_quats(R0[:, b]) for b in range(R0.shape[1])]); mean_t = T0.mean(0); pelvis = rig.index('pelvis')
    Rs, Ts = [], []
    for phase in take['shown_phase']:
        r, t = at_phase(R0, T0, phase)
        Rs.append([qmul(mean_r[b], qpow(qmul(qinv(mean_r[b]), r[b]), take['gain'])) for b in range(len(r))])
        t = t.copy(); t[pelvis] = mean_t[pelvis] + take['gain'] * (t[pelvis] - mean_t[pelvis]); Ts.append(t)
    return np.array(Rs), np.array(Ts)


def fit_error(rig, take):
    """The fit against what its own video showed, frame for frame (cm): mean, 95 %, max, and the worst joint."""
    keep = [i for i, n in enumerate(rig.names) if n not in ('#0', 'root') and 'twist' not in n]
    e = np.linalg.norm(joints(rig, *shown(rig, take))[:, keep] - joints(rig, np.array(take['rotations']), np.array(take['translations']))[:, keep], axis=2) * 100
    return e.mean(), np.percentile(e, 95), e.max(), rig.names[keep[int(np.argmax(e.mean(0)))]]


def main():
    if sys.argv[1] == 'make':
        out, first, count = sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
        noise = float(sys.argv[5]) if len(sys.argv) > 5 else 2.0
        outliers = float(sys.argv[6]) if len(sys.argv) > 6 else 0.03
        loss = sys.argv[7] if len(sys.argv) > 7 else 'linear'
        scale = float(sys.argv[8]) if len(sys.argv) > 8 else 1.0
        loop = len(sys.argv) > 9 and sys.argv[9] == 'loop'
        make(out, first, count, noise, outliers, loss, scale, loop)
        return
    rig = gltf_rig.Rig(TRUTH)
    if sys.argv[1] == 'score':
        t = json.load(open(sys.argv[2]))
        print('%-24s mean %.2f cm, 95%% %.2f, max %.2f; bone worst median %.2f deg' % ((sys.argv[3] if len(sys.argv) > 3 else 'take') + ':',
              *score(rig, *cycle(t['rotations'], t['translations']))))
        return
    out = sys.argv[2]
    takes = [json.load(open(os.path.join(out, p))) for p in sorted(os.listdir(out)) if p.startswith('take_') and p.endswith('.json')]
    print('%d takes; each alone:' % len(takes))
    for k, t in enumerate(takes):
        print('  take %d %-16s pace %.2f gain %.2f: mean %.2f cm, 95%% %.2f, max %.2f; bone worst median %.2f deg'
              % (k, t['camera'], t['pace'], t['gain'], *score(rig, *cycle(t['rotations'], t['translations']))))
        print('      against what its video showed: mean %.2f cm, 95%% %.2f, max %.2f; worst joint %s' % fit_error(rig, t))
    for count in sorted({2, 3, 5, len(takes)}):
        if count > len(takes): continue
        for label, warp, robust in (('shift + mean', False, False), ('warp + mean', True, False), ('warp + median', True, True)):
            R, T = average(rig, takes[:count], warp, robust)
            print('%d takes, %-14s: mean %.2f cm, 95%% %.2f, max %.2f; bone worst median %.2f deg' % (count, label, *score(rig, R, T)), flush=True)


if __name__ == '__main__':
    main()
