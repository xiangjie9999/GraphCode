# The body's travel from a take recorded in place: a planted paw is still on the ground, so in an in-place picture it slides back at the
# body's speed. A paw counts as planted in a frame step where its toe is within `lift` of its lowest over the cycle AND it moves back
# (a curled toe in swing can sit as low as a planted one, but it moves forward); each step's travel is the median backward step of its
# planted paws (along the walk, Unreal +Y = glTF +z), summed into the root. Measured 2026-10-18 on the cat walk (truth 40.00 cm a cycle):
# toe 1.5 cm -> 39.20 from the capture, 39.23 from the true points; height alone (6 mm) gave 1.96; the paw bone 35.88.
# Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E7).
import numpy as np
import characters, gltf_rig

PAWS = list(characters.ACTIVE.legs)     # the planted toes (the cat's four paws; the MetaHuman's ball_l, ball_r)


def root_travel(rig, R, T, lift=0.015, frames_per_second=30.0, axis=2):
    """Root translations (glTF metres) per frame, the loop's last frame its first plus one cycle's travel; the speed per step (m/s);
    the planted mask (steps x paws)."""
    root = rig.index('root'); idx = [rig.index(p) for p in PAWS]
    n = len(R)
    P = []
    for f in range(n):
        t = np.array(T[f]); t[root] = rig.rest_t[root]
        _, G = rig.globals(t, np.array(R[f]))
        P.append(G[idx])
    P = np.array(P)                                        # frames x paws x 3
    step = np.diff(P[:, :, axis], axis=0)
    planted = (P[:-1, :, 1] <= P[:, :, 1].min(0) + lift) & (step < 0)
    speed = np.array([np.median(-step[f][planted[f]]) if planted[f].any() else np.nan for f in range(n - 1)])
    good = ~np.isnan(speed)
    speed[~good] = np.median(speed[good]) if good.any() else 0.0
    out = np.repeat(rig.rest_t[root][None], n, 0).astype(float)
    out[1:, axis] += np.cumsum(speed)
    return out, speed * frames_per_second, planted


def stance(rig, R, T, near=0.03, axis=2, least=3, loop=False):
    """Steps x paws: the paw in stance, moving back with the body (in a picture played in place a planted paw slides back at the body's
    speed) within `near` of its lowest, in runs of at least `least` steps, a gap of one or two steps closed. Not root_travel's 1.5 cm test:
    the cat's forepaw toe rides about 2 cm up early in stance while the paw rolls onto it, so that test read the left fore landing at
    frame 19 of 31 in the true walk and at frame 8 in its capture, where both move back from frame 6 (MOVE-CAP1 E19, 2026-10-26)."""
    root = rig.index('root'); idx = [rig.index(p) for p in PAWS]
    P = []
    for f in range(len(R)):
        t = np.array(T[f]); t[root] = rig.rest_t[root]
        _, G = rig.globals(t, np.array(R[f]))
        P.append(G[idx])
    P = np.array(P)
    step = np.diff(P[:, :, axis], axis=0)
    back = step < -0.3 * np.median(np.abs(step), axis=0)
    down = back & (P[:-1, :, 1] <= P[:, :, 1].min(0) + near)
    n = len(down); out = down.copy()
    for k in range(down.shape[1]):
        col = np.concatenate([down[:, k]] * 3) if loop else down[:, k]
        for gap in (1, 2):                                     # close short gaps inside a stance
            for f in range(1, len(col) - gap):
                if col[f - 1] and not col[f:f + gap].any() and col[f + gap]: col[f:f + gap] = True
        runs, f = [], 0
        while f < len(col):
            if col[f]:
                g = f
                while g < len(col) and col[g]: g += 1
                runs.append((f, g)); f = g
            else: f += 1
        keep = np.zeros(len(col), bool)
        for a, b in runs:
            if b - a >= least: keep[a:b] = True
        out[:, k] = keep[n:2 * n] if loop else keep
    return out


def key_speed(rig, R, T, keys, frames_per_second=30.0, loop=False, axis=2):
    """The travel speed that keeps the take's own timing when its keys are linked by `travel(root, velocity=)`: GraphCode moves the body
    over each stretch by the drawn step of the paw planted at both ends, the one written LAST in the later key (Intent/CatWalkTravel.gcmove,
    W30-W32), so the speed is those steps summed over the time from the first key to the last (round to the first again on a loop).
    keys: landings(). Returns (speed m/s, distance m, seconds)."""
    root = rig.index('root')
    def place(f, paw):
        t = np.array(T[f]); t[root] = rig.rest_t[root]
        _, G = rig.globals(t, np.array(R[f]))
        return G[rig.index(paw)][axis]
    frames = [int(round(s * frames_per_second)) for s, _, _ in keys]
    pairs = list(zip(range(len(keys)), range(1, len(keys)))) + ([(len(keys) - 1, 0)] if loop else [])
    distance = 0.0
    for i, j in pairs:
        both = [p for p in keys[j][2] if p in keys[i][2]]
        if not both: continue
        paw = both[-1]
        distance += place(frames[i], paw) - place(frames[j], paw)
    seconds = ((len(R) - 1) if loop else (frames[-1] - frames[0])) / frames_per_second
    return distance / seconds, distance, seconds


def landings(down, frames_per_second=30.0, loop=False):
    """When each paw lands (the first stance step after one in the air) and which paws are down then: the moments a take is keyed at
    (`pose(source=<take>, at=<seconds>s)`, MOVE-CAP1 E19) and the plants written inside each key. down: steps x paws (stance). A loop's
    first step counts as a landing when its last step is in the air. Returns [(seconds, paw, [paws down])] in time order."""
    # The paws down are listed as Intent/CatWalkTravel.gcmove writes a key's plants: the paw down longest (behind, about to lift), the paw
    # landing, then the one under the body LAST (it landed in the key before and carries the body: the stretch's travel is its drawn step).
    n = len(down)
    def since(j, f):
        g = 0
        while g < n and down[(f - g - 1) % n if loop else max(0, f - g - 1), j] and (loop or f - g - 1 >= 0): g += 1
        return g
    out = []
    for k, paw in enumerate(PAWS):
        for f in range(n):
            before = down[f - 1, k] if f > 0 else (down[-1, k] if loop else True)
            if down[f, k] and not before:
                others = sorted([j for j in range(len(PAWS)) if down[f, j] and j != k], key=lambda j: -since(j, f))
                order = ([PAWS[others[0]]] if others else []) + [paw] + [PAWS[j] for j in others[1:]]
                out.append((f / frames_per_second, paw, order))
    return sorted(out)
