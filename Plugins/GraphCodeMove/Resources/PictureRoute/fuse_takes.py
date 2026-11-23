# Takes that are not one motion, fused into one: aligned by phase (average_takes), then at each phase ONE pose fitted to every take's
# joint balls at once, each take a view from its own camera (fit_rig.solve_views), started from and gently held to the takes' average.
# Two AI generations from two cameras are two motions, so they never triangulate frame for frame; lined up by the gait they do, and
# one take's camera pins what another's leaves in depth (one camera turns 2 px of jitter into ~2 cm of depth). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E9).
# Usage: python fuse_takes.py <rig.gltf> <ikrig.gc> <calibration.gltf> <out fit.json> <obs_a.json> <fit_a.json> [<obs_b> <fit_b> ...]
#        [--part i/k]   fit only phases i, i+k, ... into <out>.part<i>; then --join k writes <out> from the parts (a phase is ~10 s a take)
#        [--against <earlier fused fit.json>]   line the takes up by their own balls on that fusion (a second round)
#        [--rolls]   also match the drawn bones' rolls each take read (sharp frames only: E12)
import json, os, sys
import numpy as np
import average_takes as A, camera, fit_rig, gltf_rig


def ball_at(balls, x):
    """A joint ball at fractional frame x of the take (its obs holds the cycle's frames and the first again)."""
    i = int(np.floor(x)); a = x - i
    p, q = balls[i], balls[min(i + 1, len(balls) - 1)]
    if p is None or q is None: return None
    return (1 - a) * np.asarray(p) + a * np.asarray(q)


def value_at(values, x):
    """A per-frame reading (or None) at fractional frame x, as ball_at."""
    if not values: return None
    i = int(np.floor(x)); a = x - i
    p, q = values[i], values[min(i + 1, len(values) - 1)]
    return None if p is None or q is None else (1 - a) * p + a * q


def seen_against(rig, ref_R, ref_T, obs, cap=20.0):
    """How far each pose of the reference, seen from a take's camera, is from the balls the take saw in each of its frames: the median
    pixel distance over the balls seen (each capped, so a wrong ball weighs no more than a missing limb), reference x take frames."""
    J = gltf_rig.to_unreal(A.joints(rig, ref_R, ref_T))
    n = len(ref_R); D = np.zeros((n, n))
    for c in obs['cameras'].values():
        cam = camera.Camera.from_render(c['camera'], obs['width'], obs['height'])
        names = list(c['balls'])
        P = np.array([cam.project(J[i][[rig.index(b) for b in names]] + np.asarray(c['origin'] or [0, 0, 0])) for i in range(n)])
        for j in range(n):
            seen = [k for k, b in enumerate(names) if c['balls'][b][j] is not None]
            x = np.array([c['balls'][names[k]][j] for k in seen])
            D[:, j] += np.median(np.minimum(np.linalg.norm(P[:, seen] - x[None], axis=2), cap), axis=1)
    return D


def bone_offsets(rig, ref_R, ref_T, obs, where):
    """{bone: median pixel distance of a take's balls from the reference seen from its camera, at the take's matched frames}."""
    J = gltf_rig.to_unreal(A.joints(rig, ref_R, ref_T)); out = {}
    for c in obs['cameras'].values():
        cam = camera.Camera.from_render(c['camera'], obs['width'], obs['height'])
        for b, balls in c['balls'].items():
            d = [np.linalg.norm(cam.project(J[i][rig.index(b)] + np.asarray(c['origin'] or [0, 0, 0]))[0] - v)
                 for i, v in ((i, ball_at(balls, x)) for i, x in enumerate(where)) if v is not None]
            if d: out[b] = float(np.median(d))
    return out


def fuse(rig, model, takes, obs_list, phases=None, loss='soft_l1', scale=3.0, hold=0.05, reference=None, use_rolls=False):
    """Per phase of the takes' average: the fused pose (local rotations, translations) and its mean pixel error. Given a reference (an
    earlier fusion, one cycle), each take is lined up by how its own balls sit on the reference seen from its camera, not by its own
    one-camera fit, whose depth is a guess (E11: four takes fused by their fits, 1.18 cm; two, 0.75)."""
    weights = [1.0] * len(takes)
    if reference is None:
        ref_R, ref_T = A.average(rig, takes)
        ref_f = A.features(rig, ref_R, ref_T)
        maps = [A.matching(ref_f, A.features(rig, *A.cycle(t['rotations'], t['translations']))) for t in takes]
    else:
        ref_R, ref_T = reference
        costs = [seen_against(rig, ref_R, ref_T, obs) for obs in obs_list]
        maps = [A.matching_costs(D) for D in costs]
        # How far each take's balls sit from the reference along its match: a take that agrees less counts less (E11: one take's
        # wrong rings, consistent over frames, pulled four takes fused to 1.17 cm against two's 0.73).
        # Per take and bone: the median distance of its balls from the reference seen from its camera, along its match; a bone
        # counts less in a take where it sits further off than that bone does across the takes (a take's forepaw on a wrong ring).
        off = [bone_offsets(rig, ref_R, ref_T, obs, m) for obs, m in zip(obs_list, maps)]
        named = sorted({b for o in off for b in o})
        weights = []
        for o in off:
            w = {}
            for b in named:
                if b not in o: continue
                typical = np.median([x[b] for x in off if b in x])
                w[b] = float(min(1.0, max(typical, 0.5) / max(o[b], 1e-6)))
            weights.append(w)
        print('bones weighted below 0.5: %s' % ['%d:%s %.2f' % (k, b, v) for k, w in enumerate(weights) for b, v in w.items() if v < 0.5])
    J = A.joints(rig, ref_R, ref_T)
    named = [b for b in rig.names if b not in ('#0', 'root') and 'twist' not in b]
    rest = np.zeros(model.size); rest[:3] = rig.rest_t[model.pelvis]
    out = {}
    for i in (phases if phases is not None else range(len(ref_R))):
        start, _ = model.solve({b: J[i, rig.index(b)] for b in named}, rest)       # the average, as the rig's parameters
        views, count = [], {}
        for obs, where, w in zip(obs_list, maps, weights):
            for c in obs['cameras'].values():
                seen = {b: v for b, v in ((b, ball_at(balls, where[i])) for b, balls in c['balls'].items()) if v is not None}
                # The drawn bones' rolls it read (E12), on bones long enough on screen; only when asked (--rolls): read off soft, JPEG
                # frames they were 1.4-2.7 px off (sharp: 0.1-1) and turned four fused takes' forelimbs 5-9 degrees worse (E11/E12).
                rolls = {}
                for b, r in (c.get('rolls', {}) if use_rolls else {}).items():
                    if b in seen and r['child'] in seen and np.linalg.norm(seen[r['child']] - seen[b]) >= 50:
                        q, sp = (value_at(r.get(k), where[i]) for k in ('q', 's'))
                        if q is not None or sp is not None: rolls[b] = (r['child'], q, sp)
                views.append((camera.Camera.from_render(c['camera'], obs['width'], obs['height']), c['origin'] or [0, 0, 0], seen, w, rolls))
                for b in seen: count[b] = count.get(b, 0) + 1
        p, errs = fit_rig.solve_views(model, views, start, start, hold, loss, scale)
        Rl, t = model.locals(p)
        out[i] = ([gltf_rig.matrix_to_quat(r).tolist() for r in Rl], np.asarray(t).tolist(), float(np.concatenate(errs).mean()), count)
    return out, len(ref_R)


def fill_unseen(rig, R, seen):
    """Where no take saw any ball a bone moves (its own children and theirs) at a phase, that bone's turn there is a guess the fit
    held near the average; it is laid instead between the nearest phases either side, round the cycle, where one was seen (E11: a
    swinging forepaw unseen for 6 phases in every take, 15-21 cm off). R: phases x bones x 4 (one cycle); seen: per phase {bone: views}."""
    R = np.array(R, float); n = len(R)
    kids = {i: [] for i in range(len(rig.names))}
    for i, p in enumerate(rig.parent):
        if p >= 0: kids[p].append(i)
    def below(i):
        out = []
        for k in kids[i]: out += [rig.names[k]] + below(k)
        return out
    filled = 0
    for b, name in enumerate(rig.names):
        if name in ('#0', 'root') or 'twist' in name: continue
        moved = [d for d in below(b) if 'twist' not in d]
        if not moved: continue
        known = [i for i in range(n) if any(seen[i].get(d, 0) for d in moved)]
        if len(known) < 2 or len(known) == n: continue
        for i in range(n):
            if i in known: continue
            a = max([k for k in known if k < i], default=max(known) - n)
            c = min([k for k in known if k > i], default=min(known) + n)
            R[i, b] = A.nlerp(R[a % n, b], R[c % n, b], (i - a) / (c - a)); filled += 1
    return R, filled


def main():
    args = sys.argv[1:]
    part = join = None
    if '--part' in args:
        k = args.index('--part'); part = [int(x) for x in args[k + 1].split('/')]; del args[k:k + 2]
    if '--join' in args:
        k = args.index('--join'); join = int(args[k + 1]); del args[k:k + 2]
    reference = None
    use_rolls = '--rolls' in args
    if use_rolls: args.remove('--rolls')
    if '--against' in args:                       # an earlier fusion's fit.json: line the takes up by their balls on it
        k = args.index('--against'); ref = json.load(open(args[k + 1])); del args[k:k + 2]
        reference = A.cycle(ref['rotations'], ref['translations'])
    rig_path, ikrig, calib, out = args[:4]
    rig = gltf_rig.Rig(rig_path)
    if join:
        parts = {}
        for i in range(join): parts.update({int(f): v for f, v in json.load(open('%s.part%d' % (out, i))).items()})
        frames = [parts[f] for f in sorted(parts)]
        R, filled = fill_unseen(rig, [f[0] for f in frames], [f[3] for f in frames])
        R = np.concatenate([R, R[:1]]); frames.append(frames[0])                                    # close the loop again
        json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': [f[1] for f in frames],
                   'reprojection_px': [f[2] for f in frames]}, open(out, 'w'))
        print('fused %d phases: mean reprojection %.2f px; %d unseen bone-phases laid between seen ones' % (len(parts),
              np.mean([f[2] for f in frames]), filled))
        return
    pairs = args[4:]
    obs_list = [json.load(open(p)) for p in pairs[0::2]]
    takes = [json.load(open(p)) for p in pairs[1::2]]
    axes = fit_rig.ik_rig_axes(ikrig)
    model = fit_rig.Model(rig, axes, fit_rig.rig_orders(gltf_rig.Rig(calib), axes, rig.names), fit_rig.ik_rig_limits(ikrig))
    n = len(takes[0]['rotations']) - 1
    phases = range(part[0], n, part[1]) if part else None
    result, _ = fuse(rig, model, takes, obs_list, phases, reference=reference, use_rolls=use_rolls)
    json.dump(result, open('%s.part%d' % (out, part[0]) if part else out + '.part0', 'w'))
    if not part: main_join = [rig_path, ikrig, calib, out, '--join', '1']; sys.argv = [sys.argv[0]] + main_join; main()


if __name__ == '__main__':
    main()
