# Separate takes tracked again with their fusion as the guide (E13). Alone, a one-camera take's rig bends in depth to reach a wrong
# ring of a twin colour and keeps it for frames (E11); the fused walk, which the other takes' cameras pin, says where each ball should
# be in each of the take's own frames (the take lined up on it by its balls, fuse_takes.seen_against), so each ball is looked for
# again only a few pixels round that, with the rays that should meet it (track_all.Drawing); a ball is kept where that look and the
# take's own tracking agree. Then fused again (fuse_takes --against the first fusion).
# Usage: python refine_takes.py <rig.gltf> <fused.json> <out_dir> <frames_dir_a> <obs_a> [<frames_dir_b> <obs_b> ...] [--window px]
#   writes <out_dir>/guided_<k>.json (the guided look) and obs_<k>.json (the balls both looks agree on; fuse these), k in the order given
import json, os, sys
import numpy as np
from PIL import Image
import average_takes as A, camera, fuse_takes, gltf_rig, track_all


def take_phases(where, n):
    """The reference phase (in frames, fractional) at each of a take's n + 1 frames: the inverse of `where` (reference phase -> the
    take's matched frame, cyclic), unwrapped and held monotone (a warp path can stand still)."""
    w = np.array(where, float)
    for i in range(1, n):
        while w[i] < w[i - 1] - n / 2: w[i] += n
        while w[i] > w[i - 1] + n / 2: w[i] -= n
    w = np.maximum.accumulate(np.append(w, w[0] + n))
    out = []
    for j in range(n + 1):
        x = float(j)
        while x < w[0]: x += n
        while x >= w[0] + n: x -= n
        out.append(float(np.interp(x, w, np.arange(n + 1))) % n)
    return out


def refine(rig, ref_R, ref_T, obs, frames, window=8):
    name, c = next(iter(obs['cameras'].items()))
    cam = camera.Camera.from_render(c['camera'], obs['width'], obs['height']); origin = np.asarray(c['origin'] or [0, 0, 0])
    n = len(ref_R)
    phases = take_phases(A.matching_costs(fuse_takes.seen_against(rig, ref_R, ref_T, obs)), n)
    first = {b: np.array(v[0]) - 0.5 for b, v in c['balls'].items() if v[0] is not None}      # its first frame, known
    drawing = track_all.Drawing(frames[0], first, rig)
    root = rig.index('root')
    balls = {b: [] for b in c['balls']}
    for j, im in enumerate(frames):
        r, t = A.at_phase(ref_R, ref_T, phases[j] / n)
        t = t.copy(); t[root] = rig.rest_t[root]
        P = gltf_rig.to_unreal(rig.globals(t, r)[1])
        guess = {b: cam.project(P[rig.index(b)] + origin)[0] - 0.5 for b in c['balls']}
        found = drawing.look(np.asarray(im, float), guess, window)
        for b in balls: balls[b].append(None if b not in found else [float(found[b][0]) + 0.5, float(found[b][1]) + 0.5])
    return {'frames': obs['frames'], 'width': obs['width'], 'height': obs['height'],
            'cameras': {name: {'camera': c['camera'], 'origin': c['origin'], 'balls': balls}}}


if __name__ == '__main__':
    args = sys.argv[1:]
    window = 8
    if '--window' in args:
        k = args.index('--window'); window = float(args[k + 1]); del args[k:k + 2]
    rig = gltf_rig.Rig(args[0])
    fused = json.load(open(args[1])); ref_R, ref_T = A.cycle(fused['rotations'], fused['translations'])
    out = args[2]; os.makedirs(out, exist_ok=True)
    for k, (folder, obs_path) in enumerate(zip(args[3::2], args[4::2])):
        obs = json.load(open(obs_path))
        name = next(iter(obs['cameras']))
        frames = [Image.open(os.path.join(folder, '%s_%03d.png' % (name, f))).convert('RGB') for f in range(obs['frames'])]
        new = refine(rig, ref_R, ref_T, obs, frames, window)
        json.dump(new, open(os.path.join(out, 'guided_%02d.json' % k), 'w'))
        # Kept where the take's own tracking and the guided look agree within 2 px (E13: wrong balls 22-95 a take -> 4-12; alone,
        # either pass keeps errors the other does not make).
        agreed = json.loads(json.dumps(obs)); c = agreed['cameras'][name]; c.pop('rolls', None)
        for b, v in c['balls'].items():
            c['balls'][b] = [x if x is not None and y is not None and np.hypot(x[0] - y[0], x[1] - y[1]) <= 2 else None
                             for x, y in zip(v, new['cameras'][name]['balls'][b])]
        json.dump(agreed, open(os.path.join(out, 'obs_%02d.json' % k), 'w'))
        count = lambda o: sum(v is not None for v in sum(o['cameras'][name]['balls'].values(), []))
        print('take %d (%s): %d balls before, %d found again, %d agreed' % (k, name, count(obs), count(new), count(agreed)), flush=True)
