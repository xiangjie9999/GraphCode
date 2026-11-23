# Takes that are not one motion, averaged by phase (user, 2026-10-18: "Even when different motion, they will be similar, we shall find
# an average"). Each take is a fitted loop (fit.json); its phase is found from its own paws, never from a clock: the four toes' place
# relative to the pelvis (height and along the walk) is laid take against take at the best circular shift, then time-warped inside it
# (two AI generations are never timed alike). The average starts as the take closest to all others and is re-made from every take
# aligned to it, a few rounds, each bone's turn the per-phase median (quaternions, signs aligned), so one take's bad frame does not show.
# Measured (E6, E8): Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
# Usage: python average_takes.py <rig.gltf> <out fit.json> <fit_a.json> <fit_b.json> [...]
import json, sys
import numpy as np
import gltf_rig

PAWS = ['toe_F_L', 'toe_F_R', 'toe_B_L', 'toe_B_R']


def cycle(R, T):
    """A loop's distinct frames: its last sample is its first again (31 samples over a 30-frame cycle), fitted separately, so close but
    never exactly equal; it is dropped by construction, not by comparing."""
    return np.array(R[:-1]), np.array(T[:-1])


def nlerp(a, b, t):
    b = b if np.dot(a, b) >= 0 else -b
    q = (1 - t) * a + t * b
    return q / np.linalg.norm(q)


def at_phase(R, T, u):
    """The pose at phase u (cycles) of one cycle (frames x bones arrays, wrapping)."""
    n = len(R); x = (u % 1.0) * n; i = int(np.floor(x)) % n; j = (i + 1) % n; a = x - np.floor(x)
    return np.array([nlerp(R[i, b], R[j, b], a) for b in range(R.shape[1])]), (1 - a) * T[i] + a * T[j]


def mean_quats(qs):
    qs = np.array([q if np.dot(q, qs[0]) >= 0 else -q for q in qs])
    m = qs.mean(0); return m / np.linalg.norm(m)


def median_quats(qs):
    qs = np.array([q if np.dot(q, qs[0]) >= 0 else -q for q in qs])
    m = np.median(qs, 0); return m / np.linalg.norm(m)


def joints(rig, R, T):
    """In-place joint places (glTF metres), frames x nodes x 3."""
    root = rig.index('root'); out = []
    for f in range(len(R)):
        t = np.array(T[f]); t[root] = rig.rest_t[root]
        out.append(rig.globals(t, R[f])[1])
    return np.array(out)


def features(rig, R, T):
    """Per frame, the four toes' place relative to the pelvis (height and along the walk): the gait, wherever the take began."""
    J = joints(rig, R, T); pel = rig.index('pelvis'); idx = [rig.index(p) for p in PAWS]
    return np.concatenate([J[:, idx, 1] - J[:, pel, 1][:, None], J[:, idx, 2] - J[:, pel, 2][:, None]], 1)


def dtw(a, b):
    """Cost and path of dynamic time warping between a (n x d) and b (m x d)."""
    return dtw_costs(np.linalg.norm(a[:, None] - b[None], axis=2))


def dtw_costs(D):
    """Cost and path of dynamic time warping over a cost matrix (reference frames x take frames)."""
    n, m = D.shape
    C = np.full((n + 1, m + 1), np.inf); C[0, 0] = 0
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            C[i, j] = D[i - 1, j - 1] + min(C[i - 1, j], C[i, j - 1], C[i - 1, j - 1])
    i, j, path = n, m, []
    while i > 0 and j > 0:
        path.append((i - 1, j - 1))
        k = np.argmin([C[i - 1, j - 1], C[i - 1, j], C[i, j - 1]])
        i, j = (i - 1, j - 1) if k == 0 else (i - 1, j) if k == 1 else (i, j - 1)
    return C[n, m], path[::-1]


def matching(ref_f, f, warp=True):
    """For each reference frame, the take's frame (fractional, within its cycle) at the same moment of the gait: the best circular
    shift, then (warp) the time warping inside it, a reference frame taking the mean of the take frames matched to it."""
    n = len(ref_f)
    if not warp:
        best = int(np.argmin([np.sum((np.roll(f, -k, 0) - ref_f) ** 2) for k in range(n)]))
        return (np.arange(n, dtype=float) + best) % n
    (_, path), best = min(((dtw(ref_f, np.roll(f, -k, 0)), k) for k in range(n)), key=lambda t: t[0][0])
    where = np.zeros(n); hits = np.zeros(n)
    for i, j in path: where[i] += j; hits[i] += 1
    return (where / hits + best) % n


def matching_costs(D):
    """matching() from a cost matrix (reference frame i against take frame j, one cycle each): the best circular shift and warp."""
    n = D.shape[0]
    (_, path), best = min(((dtw_costs(np.roll(D, -k, 1)), k) for k in range(n)), key=lambda t: t[0][0])
    where = np.zeros(n); hits = np.zeros(n)
    for i, j in path: where[i] += j; hits[i] += 1
    return (where / hits + best) % n


def align(ref_f, R, T, f, warp=True):
    """The take (one cycle, R, T, its features f) resampled onto the reference's phases."""
    n = len(ref_f)
    out = [at_phase(R, T, x / n) for x in matching(ref_f, f, warp)]
    return np.array([r for r, _ in out]), np.array([t for _, t in out])


def average(rig, takes, warp=True, robust=True, rounds=3):
    """Takes averaged by phase (one cycle, R and T): the reference starts as the take closest to all others, then (rounds) every take is
    aligned to the current average and averaged again, so the average's timing is the takes' own, not one take's."""
    cyc = [cycle(t['rotations'], t['translations']) for t in takes]
    feats = [features(rig, R, T) for R, T in cyc]
    if len(cyc) == 1: return cyc[0]
    cost = [sum(np.sum((np.roll(fb, -int(np.argmin([np.sum((np.roll(fb, -k, 0) - fa) ** 2) for k in range(len(fa))])), 0) - fa) ** 2)
                for fb in feats) for fa in feats]
    ref_R, ref_T = cyc[int(np.argmin(cost))]
    for _ in range(rounds):
        ref_f = features(rig, ref_R, ref_T)
        al = [align(ref_f, R, T, f, warp) for (R, T), f in zip(cyc, feats)]
        pick = median_quats if robust else mean_quats
        ref_R = np.array([[pick([a[0][i, b] for a in al]) for b in range(ref_R.shape[1])] for i in range(len(ref_R))])
        ref_T = np.median([a[1] for a in al], 0) if robust else np.mean([a[1] for a in al], 0)
    return ref_R, ref_T


if __name__ == '__main__':
    rig = gltf_rig.Rig(sys.argv[1])
    takes = [json.load(open(p)) for p in sys.argv[3:]]
    R, T = average(rig, takes)
    R = np.concatenate([R, R[:1]]); T = np.concatenate([T, T[:1]])         # close the loop again
    json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': T.tolist()}, open(sys.argv[2], 'w'))
    print('%d takes averaged by phase' % len(takes))
