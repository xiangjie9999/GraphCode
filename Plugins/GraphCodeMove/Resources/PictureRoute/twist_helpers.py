# The rig's twist helpers from their drivers, as GraphCode bakes them (GraphCodeRigTwist.cpp): helper local =
# Frame * slerp(identity, the driver's twist about its own X, Share) * Frame^-1 * helper rest, Frame the driver's rest frame seen from
# the helper's parent. Which bone drives each helper and its share are a fact of the rig, read off a clip of it (calibrate), never off
# the take. The glTF basis is a mirror of Unreal's, which keeps X as X, so the rule holds there unchanged. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
import numpy as np
import gltf_rig


def qmul(a, b):
    ax, ay, az, aw = a; bx, by, bz, bw = b
    return np.array([aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                     aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz])


def qinv(q): return np.array([-q[0], -q[1], -q[2], q[3]])


def twist_x(q):
    t = np.array([q[0], 0.0, 0.0, q[3]]); n = np.linalg.norm(t)
    return t / n if n > 1e-12 else np.array([0, 0, 0, 1.0])


def slerp_identity(q, s):
    q = q if q[3] >= 0 else -q
    angle = 2 * np.arccos(np.clip(q[3], -1, 1))
    if angle < 1e-12: return np.array([0, 0, 0, 1.0])
    axis = q[:3] / np.sin(angle / 2)
    a = angle * s
    return np.concatenate([axis * np.sin(a / 2), [np.cos(a / 2)]])


def rest_globals(rig):
    R, _ = rig.frame(None)
    return [gltf_rig.matrix_to_quat(m) for m in R]


def frame_of(rig, driver, helper, G):
    """The driver's rest frame seen from the helper's parent (rest)."""
    p = rig.parent[helper]
    return qmul(qinv(G[p]), G[driver])


def helper_local(rig, driver, helper, share, driver_local, G):
    F = frame_of(rig, driver, helper, G)
    T = twist_x(qmul(qinv(rig.rest_r[driver]), driver_local))
    part = slerp_identity(T, share)
    return qmul(qmul(qmul(F, part), qinv(F)), rig.rest_r[helper])


def calibrate(rig):
    """helper -> (driver, share), from the clip's own baked helpers: the driver named by the helper's prefix, the share that best
    reproduces the helper over the clip."""
    G = rest_globals(rig)
    out = {}
    if len(rig.anim_r) < 3:
        # a still clip (a rest template) shows no twist, so every share fits it equally: leave the helpers at rest rather than pick
        # one at random (the MetaHuman's helpers are driven by its own post-process when it plays, 2026-11-16)
        print('twist helpers: the calibration clip has %d frame(s), no motion to read shares from; helpers stay at rest' % len(rig.anim_r))
        return out
    for h, name in enumerate(rig.names):
        if 'twist' not in name: continue
        prefix = name.split('_twist')[0]
        side = name.rsplit('_', 1)[-1]
        # the cat's helpers name their driver whole before '_twist'; the MetaHuman's keep the side at the end
        # (lowerarm_twist_01_l -> lowerarm_l, upperarm_twistCor_01_r -> upperarm_r)
        candidates = [prefix] + (['%s_%s' % (prefix, side)] if side in ('l', 'r', 'L', 'R') else [])
        found = [c for c in candidates if c in rig.names]
        if not found:
            print('twist helpers: %s names no driver (%s); left at rest' % (name, ' or '.join(candidates)))
            continue
        driver = rig.index(found[0])
        best = None
        for share in np.linspace(-1.5, 1.5, 301):
            err = 0.0
            for f in range(len(rig.anim_r)):
                q = helper_local(rig, driver, h, share, rig.anim_r[f, driver], G)
                err = max(err, 2 * np.degrees(np.arccos(min(1.0, abs(float(np.dot(q, rig.anim_r[f, h])))))))
            if best is None or err < best[0]: best = (err, share)
        out[name] = (rig.names[driver], float(best[1]), float(best[0]))
    return out


def apply(rig, rotations, calibration):
    """Fill each helper's local rotation, per frame, from its driver's fitted rotation."""
    G = rest_globals(rig)
    R = np.array(rotations, float)
    for name, (driver, share, _) in calibration.items():
        h, d = rig.index(name), rig.index(driver)
        for f in range(len(R)): R[f, h] = helper_local(rig, d, h, share, R[f, d], G)
    return R


if __name__ == '__main__':
    import sys
    calib = calibrate(gltf_rig.Rig(sys.argv[1]))
    for k, v in calib.items(): print('%-20s driver %-11s share %+.3f  (worst %.3f deg on the calibration clip)' % (k, *v))
    if len(sys.argv) > 2:
        truth = gltf_rig.Rig(sys.argv[2])
        R = apply(truth, truth.anim_r, calib)
        for name in calib:
            h = truth.index(name)
            e = max(2 * np.degrees(np.arccos(min(1.0, abs(float(np.dot(R[f, h], truth.anim_r[f, h])))))) for f in range(len(R)))
            print('  on the test clip, %-20s worst %.3f deg' % (name, e))
