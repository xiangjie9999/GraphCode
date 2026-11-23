# Joints from video with no bones drawn: the joint reader's peaks (dino_reader.py read: per frame and joint up to three [x, y, score])
# chosen with the rig in the loop, as track_all.track_with_rig chooses rings. Each frame: the rig carried on from the last two frames
# says where each joint should be; each joint takes its peak nearest that, a stronger peak a little favoured (the reader mixes up a
# left and a right paw where they look alike, and the rig knows which is which); the rig is fitted to those, robustly, each joint
# weighted by its peak's score; the peaks are chosen again round the fitted rig and fitted again; a point still more than `gate` px
# from the rig is dropped. Forward from the known first frame, then backward from where that ended, merged. Writes obs.json (balls,
# and conf: the peak scores) for fit_views / fuse_takes. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E15-).
# Usage: python dino_track.py <take_dir> <rig.gltf> <ikrig.gc> <calibration.gltf> <peaks.json> [<peaks.json> ...] [--rings]
#        --rings: the hybrid, for a video made from a first frame with the bones drawn (track_all.py first): rings and reader together
#        take_dir holds cameras.json and first.json (track_all.py first / firstplain), frames.json; one peaks.json per camera
import json, os, sys
import numpy as np
import camera, capture, characters, fit_rig, gltf_rig


def choose(peaks, guess, window, favour=4.0):
    """The peak nearest `guess` within `window` px, a stronger peak counting up to `favour` px nearer: (xy, score) or None."""
    best = None
    for x, y, s in peaks:
        d = np.hypot(x - guess[0], y - guess[1])
        if d > window: continue
        cost = d - favour * s
        if best is None or cost < best[0]: best = (cost, np.array([x, y]), s)
    return None if best is None else (best[1], best[2])


def with_rings(peaks, rings, agree=8.0):
    """The hybrid: a drawn ring (track_all's finder, sub-pixel where the video kept it) where the reader also puts the joint takes that
    peak's place and counts 0.9 at least; a ring the reader does not back joins as a weak candidate (0.3); a video model that garbles a
    bone's drawing leaves the reader and the rig to outvote it."""
    out = [list(p) for p in peaks]
    for x in rings:
        near = [k for k, p in enumerate(out) if np.hypot(p[0] - x[0], p[1] - x[1]) <= agree]
        if near:
            k = min(near, key=lambda k: np.hypot(out[k][0] - x[0], out[k][1] - x[1])); out[k] = [x[0], x[1], max(0.9, out[k][2])]
        else: out.append([x[0], x[1], 0.3])
    return out


def track(peaks, cams, first, model, smooth=0.2, scale=3.0, gate=6.0, window=30.0, tight=8.0, drawn=None, known=None):
    """peaks: {camera: [frame][joint] -> [[x, y, s], ...]} (dino_reader's order of joints, capture.BONES); cams: {camera: (Camera,
    origin)}; first: {camera: {bone: pixel}} at frame 0. drawn (the hybrid, a video with the bones drawn): {camera: (frames, Drawing)},
    each frame's rings looked for round the same guesses and merged into the peaks (with_rings). Returns {camera: {bone: [pixel or
    None]}}, {camera: {bone: [score or None]}}."""
    n = len(next(iter(peaks.values())))
    bones = capture.BONES

    def project(p):
        P = gltf_rig.to_unreal(model.positions(p))
        return {c: {b: cam.project(P[model.rig.index(b)] + np.asarray(o))[0] for b in bones} for c, (cam, o) in cams.items()}

    def look(f, guesses, win):
        out = {}
        for c in cams:
            out[c] = {}
            rings = {}
            if drawn and c in drawn:                               # ring centres come back as pixel indices: +0.5 to the pinhole's
                frames, drawing = drawn[c]
                rings = {b: v + 0.5 for b, v in drawing.look(np.asarray(frames[f], float), {b: g - 0.5 for b, g in guesses[c].items()},
                                                             min(win, 12.0)).items()}
            for j, b in enumerate(bones):
                cand = with_rings(peaks[c][f][j], [rings[b]] if b in rings else [])
                x = choose(cand, guesses[c][b], win)
                if x is not None: out[c][b] = x
        return out

    def fit(found, start, previous):
        views = [(cams[c][0], cams[c][1], {b: v[0] for b, v in found[c].items()}, {b: max(0.05, v[1]) for b, v in found[c].items()})
                 for c in cams if found[c]]
        return fit_rig.solve_views(model, views, start, previous, smooth, 'soft_l1', scale)[0]

    def one_way(order, start, guesses0):
        out = {c: {b: {} for b in bones} for c in cams}
        params = [start]
        for k, f in enumerate(order):
            pred = params[-1] if len(params) < 2 else params[-1] + (params[-1] - params[-2])
            found = look(f, project(pred) if k else guesses0, window if k else tight)
            p = fit(found, pred, params[-1])
            again = look(f, project(p), tight)
            p = fit(again, p, params[-1])
            final = project(p)
            params = [p] if k == 0 else params + [p]
            for c in cams:
                for b in bones:
                    x = again[c].get(b)
                    out[c][b][f] = (x[0], x[1]) if x is not None and np.linalg.norm(x[0] - final[c][b]) <= gate else None
        return out, params[-1]

    # The first frame's pose: known whole when the take says it (the clip we rendered it from, twists included), else fitted to its
    # joints' places (a twist the places cannot show is then a guess).
    if known is None:
        rest = np.zeros(model.size); rest[:3] = model.rig.rest_t[model.pelvis]
        known = fit_rig.solve_views(model, [(cams[c][0], cams[c][1], first[c]) for c in cams], rest, None, 0.05, 'linear', 1.0)[0]
    ahead, last = one_way(range(n), known, first)
    behind, _ = one_way(range(n - 1, -1, -1), last, project(last))
    balls = {c: {b: [] for b in bones} for c in cams}; conf = {c: {b: [] for b in bones} for c in cams}
    for c in cams:
        for b in bones:
            for f in range(n):
                x, y = ahead[c][b][f], behind[c][b][f]
                keep = y if x is None else x if y is None else x if np.linalg.norm(x[0] - y[0]) <= 2.0 else None
                balls[c][b].append(None if keep is None else [float(keep[0][0]), float(keep[0][1])])
                conf[c][b].append(None if keep is None else float(keep[1]))
    return balls, conf


def main():
    args = [a for a in sys.argv[1:] if a != '--rings']
    hybrid = '--rings' in sys.argv
    take, rig_path, ikrig, calib = args[:4]
    peak_files = args[4:]
    ORIGIN = characters.origin()
    cameras = json.load(open(os.path.join(take, 'cameras.json')))
    first3 = json.load(open(os.path.join(take, 'first.json')))['places']
    peaks, cams, first, size = {}, {}, {}, None
    for path in peak_files:
        r = json.load(open(path)); c = r['camera']
        peaks[c] = r['peaks']; size = (r['width'], r['height'])
        cam = camera.Camera.from_render(cameras[c], *size)
        cams[c] = (cam, ORIGIN)
        first[c] = {b: cam.project(np.array(p))[0] for b, p in first3[c].items() if b in capture.BONES}
    rig = gltf_rig.Rig(rig_path); axes = fit_rig.ik_rig_axes(ikrig)
    model = fit_rig.Model(rig, axes, fit_rig.rig_orders(gltf_rig.Rig(calib), axes, rig.names), fit_rig.ik_rig_limits(ikrig))
    drawn = None
    if hybrid:
        from PIL import Image
        import track_all
        drawn = {}
        for c in cams:
            frames = [Image.open(os.path.join(take, '%s_%03d.png' % (c, f))).convert('RGB') for f in range(len(peaks[c]))]
            drawn[c] = (frames, track_all.Drawing(frames[0], {b: v - 0.5 for b, v in first[c].items()}, rig))
    import fit_views
    pose = fit_views.first_pose(os.path.join(take, 'first.json'), rig)
    known = model.params_of(pose['rotations'], pose['translations']) if pose else None
    balls, conf = track(peaks, cams, first, model, drawn=drawn, known=known)
    n = len(next(iter(peaks.values())))
    meta = json.load(open(os.path.join(take, 'frames.json'))) if os.path.exists(os.path.join(take, 'frames.json')) else {}
    out = {'asset': meta.get('asset'), 'frames': n, 'times': meta.get('times'), 'width': size[0], 'height': size[1], 'cameras': {}}
    for c in cams:
        out['cameras'][c] = {'camera': cameras[c], 'origin': ORIGIN, 'balls': balls[c], 'conf': conf[c]}
        print('%s: %d of %d joints kept' % (c, sum(v is not None for b in balls[c].values() for v in b), n * len(capture.BONES)))
    json.dump(out, open(os.path.join(take, 'obs.json'), 'w'), indent=1)


if __name__ == '__main__':
    main()
