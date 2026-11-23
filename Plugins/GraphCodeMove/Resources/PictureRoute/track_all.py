# Joint balls from a video with EVERY bone drawn at once, as an AI video of the rigged character shows them (E10). No render without
# the overlay to subtract: the overlay is what is neither the grey body nor the black ground (measured on the cat: the same pixels as
# subtracting a plain render). Each bone's ring colour and radius are read once, off the first frame, whose pose is known (the video
# model starts from a frame we rendered); after that each ball is looked for where it is expected, in its own colour, as a ring (an
# annulus filter), and its centre fitted as a circle OF THAT RADIUS to the arc that shows (other bones' rays are drawn over the rings;
# a free circle followed the visible arc, 1-5 px off). With the rig in the loop (track_with_rig) "expected" is where the rig, fitted to
# both cameras' balls, puts it. Writes obs.json as capture.py does. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E10).
# Usage: python track_all.py render <asset> <frames> <out_dir>        renders both cameras with all bones drawn (frames saved as PNG)
#        python track_all.py first <asset> <at 0-1> <out_dir> [camera ...]  the frame to hand a video model, and its joints (first.json)
#        python track_all.py firstplain <asset> <at 0-1> <out_dir> [camera ...]  the same with no bones drawn, lit (dino_take.py reads it)
#        python track_all.py frames <video.mp4> <camera> <out_dir>    a video's frames into the folder (one camera per video)
#        python track_all.py track <out_dir> [<first-pose.gltf>] [rig <rig.gltf> <ikrig.gc> <calibration.gltf>]
#            the first frame's pose from first.json, or a glTF whose frame 0 it is; with rig, the rig is fitted in the loop
import json, os, sys
import numpy as np
import cv2
from PIL import Image
import camera, capture, characters, render, triangulate


def __getattr__(name):     # ORIGIN, the preview's placement of the character (render manifest, bubbles[].origin): characters.origin(); the cat's [-2.937, 1.016, 1.000]
    if name == 'ORIGIN': return characters.origin()
    raise AttributeError(name)

NEAR = 45.0                               # how far (RGB) a pixel may be from a ring's colour and still be its


def overlay(I, chroma=22, bright=150):
    """Pixels of the drawn bones: coloured, or brighter than the body (unlit, the cat is grey ~115 and the ground black)."""
    return ((I.max(2) - I.min(2)) > chroma) | (I.max(2) > bright)


def ring_kernel(r):
    k = int(np.ceil(r)) + 2
    yy, xx = np.mgrid[-k:k + 1, -k:k + 1]
    d = np.hypot(xx, yy)
    w = ((d >= r - 0.7) & (d <= r + 0.7)).astype(np.float32)
    return w / w.sum()


def colour_of(I, mask, at, r):
    """A ball's colour: the median of the most saturated bright overlay pixels on its ring. A bone's ray has a pale side and a dark
    side of the ring's own hue, and a pale side mostly hid a ring once: its colour was learned, and the ray followed (E10, shoulder_R)."""
    ys, xs = np.mgrid[0:I.shape[0], 0:I.shape[1]]
    d = np.hypot(xs - at[0], ys - at[1])
    sel = mask & (d >= r - 1.5) & (d <= r + 1.5) & (I.max(2) > 150)
    if sel.sum() < 5: return None
    px = I[sel]; chroma = px.max(1) - px.min(1)
    return np.median(px[chroma >= np.percentile(chroma, 60)], 0)


def radius_of(mask, at):
    """A ring's radius around its known centre: the most common distance of its colour's pixels (0.25 px bins, 2-9 px)."""
    ys, xs = np.where(mask)
    d = np.hypot(xs - at[0], ys - at[1]); d = d[(d > 2) & (d < 9)]
    if len(d) < 6: return None
    h, e = np.histogram(d, bins=np.arange(2, 9.01, 0.25))
    return e[np.argmax(h)] + 0.125


def circle_of_radius(P, c, r, tol=0.8, steps=10):
    """The centre of a circle of known radius through the points near it (Gauss-Newton from c)."""
    for _ in range(steps):
        d = np.hypot(P[:, 0] - c[0], P[:, 1] - c[1]); inl = (np.abs(d - r) < tol) & (d > 1e-6)
        if inl.sum() < 5: return None
        u = (P[inl] - c) / d[inl][:, None]
        c = c + np.linalg.lstsq(u, d[inl] - r, rcond=None)[0]
    return c


def hue_of(colour):
    return float(cv2.cvtColor(np.uint8([[np.clip(colour, 0, 255)]]), cv2.COLOR_RGB2HSV)[0, 0, 0])


def ray_support(hsv, mask, a, b, hue, clear=6.0, samples=9, tol=10):
    """How much of the segment a -> b (its ends' rings left out) is drawn in the given hue (0-180, OpenCV's): a bone's ray, pale or
    dark, keeps its ring's hue. The fraction of samples with such a pixel within 1 px."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    L = np.linalg.norm(b - a)
    if L < 2 * clear + 4: return None
    h, w = mask.shape; hits = 0
    for t in np.linspace(clear / L, 1 - clear / L, samples):
        x, y = a + t * (b - a)
        x0, x1, y0, y1 = int(max(0, x - 1)), int(min(w, x + 2)), int(max(0, y - 1)), int(min(h, y + 2))
        H = hsv[y0:y1, x0:x1].astype(int)
        d = np.abs(H[..., 0] - hue); d = np.minimum(d, 180 - d)
        hits += bool(np.any(mask[y0:y1, x0:x1] & (H[..., 1] > 35) & (d < tol)))
    return hits / samples


def roll_at(hsv, hue, a, c, half, tol=12):
    """A drawn bone's roll as its pyramid shows it (fit_rig.roll_mark models the same): across the drawn bone from joint a to its child
    c (pixel centres), at a dozen points along it, the mean offset of the pale pixels of its hue minus the dark ones', each divided by
    2(1 - t) (the pyramid narrows to its tip): the median, in pixels at the base, signed along (-u_y, u_x) for u = c - a; and how far
    apart its two pale edges (and its two dark ones) stand, halved, the same way: the other side of the roll, which moves first where
    the pale side faces across the picture and the offset sits at its most (E12: a right shoulder 20 degrees off moved the offset 0.35
    px). `half`: how far across to look. (q, s), either None where too few points read (E12, against the truth: q 0.5-0.85 px median
    off, correlation 0.98-0.99; s 0.33-0.40 px, 0.72-0.96)."""
    a, c = np.asarray(a, float) - 0.5, np.asarray(c, float) - 0.5
    Lp = np.linalg.norm(c - a)
    if Lp < 25: return None, None
    u = (c - a) / Lp; n = np.array([-u[1], u[0]])
    h, w = hsv.shape[:2]; est, apart = [], []
    for t in np.linspace(max(0.12, 9 / Lp), 0.6, 12):
        p = a + t * (c - a); pale, dark = [], []
        for s in np.arange(-half, half + 0.01, 0.5):
            xi, yi = int(round(p[0] + s * n[0])), int(round(p[1] + s * n[1]))
            if not (0 <= xi < w and 0 <= yi < h): continue
            H, S, V = hsv[yi, xi]; d = abs(int(H) - hue); d = min(d, 180 - d)
            if V > 200 and 8 < S < 90 and d < tol + 2: pale.append(s)
            if V < 150 and S > 60 and d < tol: dark.append(s)
        if pale and dark: est.append((np.mean(pale) - np.mean(dark)) / (2 * (1 - t)))
        sp = [max(0.0, (max(g) - min(g)) - 1.5) / 2 for g in (pale, dark) if len(g) >= 3]     # a line is ~1.5 px wide
        if sp: apart.append(np.mean(sp) / (1 - t))
    return (float(np.median(est)) if len(est) >= 4 else None, float(np.median(apart)) if len(apart) >= 4 else None)


def ring_at(I, mask, colour, r, guess, window=16, least=0.35, pull=0.015, support=None, weight=0.5):
    """The ring of the given colour and radius within `window` px of `guess` (the annulus filter's best, a little in favour of the
    nearer), its centre refined as a circle of that radius: pixel index coordinates, or None. `support(xy)`, when given, scores a
    candidate by the drawing round it (the rays that should meet it, 0-1); the best of the window's few strongest rings is kept (E11:
    37 bones share ~8 colours, and a one-camera rig bends to reach a near ring of a twin colour)."""
    h, w = mask.shape
    pad = window + int(np.ceil(r)) + 3                       # the crop holds a whole ring centred anywhere in the window
    x0, y0 = int(max(0, guess[0] - pad)), int(max(0, guess[1] - pad))
    x1, y1 = int(min(w, guess[0] + pad + 1)), int(min(h, guess[1] + pad + 1))
    if x1 - x0 < 8 or y1 - y0 < 8: return None
    m = (mask[y0:y1, x0:x1] & (np.linalg.norm(I[y0:y1, x0:x1] - colour, axis=2) < NEAR)).astype(np.float32)
    S = cv2.filter2D(m, -1, ring_kernel(r), borderType=cv2.BORDER_CONSTANT)
    yy, xx = np.mgrid[y0:y1, x0:x1]
    away = np.hypot(xx - guess[0], yy - guess[1])
    score = np.where(away <= window, S - pull * away, -1.0).astype(np.float32)
    if support is None:
        j = np.unravel_index(np.argmax(score), S.shape)
    else:
        top = (score == cv2.dilate(score, np.ones((5, 5), np.uint8))) & (S >= least) & (away <= window)
        cands = sorted(zip(*np.where(top)), key=lambda q: -score[q])[:4]
        if not cands: return None
        def total(q):
            s = support(np.array([q[1] + x0, q[0] + y0], float))
            return score[q] + (weight * s if s is not None else 0.0)
        j = max(cands, key=total)
    if S[j] < least: return None
    peak = np.array([j[1] + x0, j[0] + y0], float)
    ys, xs = np.where(m > 0)
    P = np.stack([xs + x0, ys + y0], 1).astype(float)
    P = P[np.hypot(P[:, 0] - peak[0], P[:, 1] - peak[1]) < r + 2]
    c = circle_of_radius(P, peak, r) if len(P) >= 5 else None
    return c if c is not None and np.linalg.norm(c - peak) <= 2.0 else peak


def rings_from(image, first):
    """Each bone's ring colour and radius, off the first frame at its known places: {bone: (colour, radius)}."""
    I0 = np.asarray(image, float); m0 = overlay(I0); out = {}
    for b, p in first.items():
        for r in (4, 5, 3, 6):
            col = colour_of(I0, m0, p, r)
            if col is not None: break
        if col is None: continue
        rad = radius_of(m0 & (np.linalg.norm(I0 - col, axis=2) < NEAR), p)
        if rad is not None: out[b] = (col, rad)
    return out


def track(frames, first):
    """One camera, no rig: each ball where its last two places carry it. frames: images in order; first: {bone: pixel at frame 0}.
    Returns {bone: [pixel centre (+0.5) or None per frame]}. Loses a ball for good once it misses (E10: 57-76 % within 2 px)."""
    rings = rings_from(frames[0], first)
    places = {b: [np.asarray(p, float)] for b, p in first.items()}
    out = {b: [] for b in first}
    for f, im in enumerate(frames):
        I = np.asarray(im, float); m = overlay(I)
        for b in first:
            seen = [p for p in places[b] if p is not None]
            if b not in rings or not seen: out[b].append(None); continue
            guess = seen[-1] if len(seen) < 2 else seen[-1] + (seen[-1] - seen[-2])
            c = ring_at(I, m, *rings[b], guess if f else first[b], window=8 if f == 0 else 24)
            if f: places[b].append(c)
            else: places[b] = [c if c is not None else np.asarray(first[b], float)]
            out[b].append(None if c is None else [float(c[0]) + 0.5, float(c[1]) + 0.5])
    return out


class Drawing:
    """One camera's drawn bones as its first frame shows them, where their pose is known: each ring's colour, radius and hue, and which
    rays meet which ball (a ball sits where its parent's ray ends, when the parent has no other drawn child, and starts its own toward
    its one child)."""

    def __init__(self, image0, first, rig):
        self.rings = rings_from(image0, first)
        self.hues = {b: hue_of(v[0]) for b, v in self.rings.items()}
        drawn = set(first)
        kids = {b: [k for k in drawn if rig.parent[rig.index(k)] == rig.index(b)] for b in drawn}
        parent = {b: rig.names[rig.parent[rig.index(b)]] for b in drawn if rig.parent[rig.index(b)] >= 0}
        self.up = {b: q for b, q in parent.items() if q in drawn and len(kids[q]) == 1}
        self.down = {b: kids[b][0] for b in drawn if len(kids[b]) == 1}
        self.order = [b for b in rig.names if b in drawn]                     # parents first

    def look(self, I, guesses, window, least=0.35, rays=True):
        """{bone: ring centre (pixel index coordinates)} for each bone found within `window` px of its guess."""
        m = overlay(I); found = {}
        hsv = cv2.cvtColor(np.clip(I, 0, 255).astype(np.uint8), cv2.COLOR_RGB2HSV) if rays else None
        for b in self.order:
            if b not in guesses or b not in self.rings: continue
            support = None
            if rays:
                def support(xy, b=b):
                    s = []
                    if b in self.up and self.up[b] in self.hues:
                        s.append(ray_support(hsv, m, found.get(self.up[b], guesses.get(self.up[b], xy)), xy, self.hues[self.up[b]]))
                    if b in self.down and self.down[b] in guesses:
                        s.append(ray_support(hsv, m, xy, guesses[self.down[b]], self.hues[b]))
                    s = [v for v in s if v is not None]
                    return np.mean(s) if s else None
            x = ring_at(I, m, *self.rings[b], guesses[b], window=window, least=least, support=support)
            if x is not None: found[b] = x
        return found


def track_with_rig(frames, cams, first, model, smooth=0.2, scale=2.0, gate=3.0, rays=True):
    """All cameras at once, the rig in the loop. frames: {camera: [images]}; cams: {camera: (Camera, origin)}; first: {camera: {bone:
    pixel at frame 0}}. Each frame: look for every ball near where the rig, carried on from the last two frames, puts it; fit the rig to
    what was found (robust); look again within a few pixels of where the fitted rig puts each ball; fit again; a ball still more than
    `gate` px from the fitted rig is dropped as another bone's ring. Run forward, then backward from where it ended, and merged.
    Returns {camera: {bone: [pixel centre (+0.5) or None]}} and each drawn bone's roll as its pyramid shows it, {camera: {bone:
    {'child': its child, 'q': [pixels or None], 's': [pixels or None]}}} (roll_at)."""
    import fit_rig, gltf_rig
    n = len(next(iter(frames.values())))
    rig = model.rig
    drawing = {c: Drawing(frames[c][0], first[c], rig) for c in cams}

    def project(p):
        P = gltf_rig.to_unreal(model.positions(p))
        return {c: {b: cam.project(P[model.rig.index(b)] + np.asarray(origin))[0] - 0.5 for b in first[c]} for c, (cam, origin) in cams.items()}

    def look(images, guesses, window, least=0.35):
        return {c: drawing[c].look(images[c], guesses[c], window, least, rays) for c in cams}

    def fit(found, start, previous):
        views = [(cams[c][0], cams[c][1], {b: v + 0.5 for b, v in found[c].items()}) for c in cams if found[c]]
        return fit_rig.solve_views(model, views, start, previous, smooth, 'soft_l1', scale)[0]

    def one_way(order, start, guesses0):
        """Frames in the given order from known parameters `start` (and where they put the balls first, guesses0): per camera and
        bone, {frame: pixel centre (+0.5) or None}, and the last frame's parameters."""
        out = {c: {b: {} for b in first[c]} for c in cams}
        rolls = {c: {b: {} for b in drawing[c].down} for c in cams}
        params = [start]
        for k, f in enumerate(order):
            images = {c: np.asarray(frames[c][f], float) for c in cams}
            pred = params[-1] if len(params) < 2 else params[-1] + (params[-1] - params[-2])
            found = look(images, project(pred) if k else guesses0, 10 if k else 6)
            p = fit(found, pred, params[-1])
            again = look(images, project(p), 4)
            p = fit(again, p, params[-1])
            # A lost ball leaves its limb to the rig and the other view, and the rig drifts out of a tight window and never finds it
            # again (E10: a hind leg 3-6 cm off for 20 frames): look for it wider round the fitted rig, then fit and look tight again.
            lost = {c: [b for b in first[c] if b not in again[c]] for c in cams}
            if k and any(lost.values()):
                at = project(p)
                wide = look(images, {c: {b: at[c][b] for b in lost[c]} for c in cams}, 40, least=0.5)    # a clear ring only
                if any(wide.values()):
                    p = fit({c: {**again[c], **wide[c]} for c in cams}, p, params[-1])
                    again = look(images, project(p), 4)
                    p = fit(again, p, params[-1])
            final = project(p)
            params = [p] if k == 0 else params + [p]
            for c in cams:
                for b in first[c]:
                    x = again[c].get(b)
                    keep = x is not None and np.linalg.norm(x - final[c][b]) <= gate
                    out[c][b][f] = [float(x[0]) + 0.5, float(x[1]) + 0.5] if keep else None
            # Each drawn bone's roll off its pyramid, between its own ball and its child's as kept (E12).
            W, G = model.frames(p); U = gltf_rig.to_unreal(G)
            for c, (cam, origin) in cams.items():
                hsv = cv2.cvtColor(np.clip(images[c], 0, 255).astype(np.uint8), cv2.COLOR_RGB2HSV)
                for b, k2 in drawing[c].down.items():
                    x, y = out[c][b][f], out[c][k2][f]
                    if x is None or y is None or b not in drawing[c].hues: rolls[c][b][f] = (None, None); continue
                    a3, c3 = U[rig.index(b)] + np.asarray(origin), U[rig.index(k2)] + np.asarray(origin)
                    hcm = np.clip(np.linalg.norm(c3 - a3) * 0.06, 0.4, 1.5)
                    half = 2 * hcm * cam.focal / np.linalg.norm(a3 - cam.location) + 4
                    rolls[c][b][f] = roll_at(hsv, drawing[c].hues[b], x, y, half)
        return out, rolls, params[-1]

    rest = np.zeros(model.size); rest[:3] = model.rig.rest_t[model.pelvis]
    known = fit({c: dict(first[c]) for c in cams}, rest, None)                  # the known first pose, as the rig's parameters
    ahead, roll_a, last = one_way(range(n), known, first)
    # Backward from where the forward pass ended: a limb the forward pass lost in a fast swing is met from the other side. A ball
    # both passes found in the same place, or only one found, is kept; where they disagree, neither (E10).
    behind, roll_b, _ = one_way(range(n - 1, -1, -1), last, project(last))
    out = {c: {b: [] for b in first[c]} for c in cams}
    for c in cams:
        for b in first[c]:
            for f in range(n):
                x, y = ahead[c][b][f], behind[c][b][f]
                out[c][b].append(x if y is None else y if x is None else x if np.hypot(x[0] - y[0], x[1] - y[1]) <= 1.5 else None)
    print('found: %s' % ', '.join('%s %d ahead, %d behind, %d kept' % (c, sum(v is not None for b in ahead[c].values() for v in b.values()),
          sum(v is not None for b in behind[c].values() for v in b.values()), sum(v is not None for b in out[c].values() for v in b)) for c in cams))
    both = lambda c, b, f, k: [v[k] for v in (roll_a[c][b][f], roll_b[c][b][f]) if v[k] is not None]
    mean = lambda vs: float(np.mean(vs)) if vs else None
    rolls = {c: {b: {'child': k2, 'q': [mean(both(c, b, f, 0)) for f in range(n)], 's': [mean(both(c, b, f, 1)) for f in range(n)]}
                 for b, k2 in drawing[c].down.items()} for c in cams}
    return out, rolls


def first_frame(asset, at, cameras, out, look='drawn', size=(capture.WIDTH, capture.HEIGHT)):
    """The frame to hand a video model, from each camera, and where each joint is in it (the render's manifest names each drawn joint's
    place as drawn, the preview's placement included: measured 2026-10-18 against the clip's own export, equal after that placement):
    writes <camera>_first.png, cameras.json and first.json ({camera: {bone: [x, y, z]}}). look 'drawn': every bone drawn, unlit (the
    rings route); 'lit' or 'unlit': the character alone (the joint reader's route, dino_reader.py), its places read off a second, tiny
    view with the bones drawn."""
    import re
    os.makedirs(out, exist_ok=True)
    places = {}
    bones = [{'type': 'bones', 'of': capture.BONES, 'draw': ['ray']}]
    for name, cfg in cameras.items():
        view = {'name': name, 'width': size[0], 'height': size[1], 'camera': cfg, 'at': at, 'to': at, 'count': 1,
                'view_mode': 'unlit' if look in ('drawn', 'unlit') else 'lit'}
        if look == 'drawn': view['highlights'] = bones
        r = render.call({'asset': asset, 'views': [view] + ([] if look == 'drawn' else
                         [{'name': 'places', 'width': 64, 'height': 64, 'at': at, 'highlights': bones}])})
        view = r['data']['views'][0]
        Image.open(view['path']).convert('RGB').save(os.path.join(out, '%s_first.png' % name))
        manifest = json.load(open(r['data']['manifest']))
        drawn = manifest['views'][0 if look == 'drawn' else 1]
        places[name] = {b['bone']: [float(x) for x in re.findall(r'[-0-9.]+', b['centre'])] for b in drawn['bubbles']}
    json.dump(cameras, open(os.path.join(out, 'cameras.json'), 'w'), indent=1)
    # The whole pose there, read off the clip itself (rig_now: the editor's pose reader), as glTF locals: a fit started from it knows
    # the twists a joint's place cannot show (fit_views.first_pose; 2026-10-26, E18).
    import rig_now
    dump = rig_now.read_clip(asset, capture.MESH, 1, os.path.join(out, 'first.pose.json'), times=[at])
    local = dump['local'][0]
    pose = {'rotations': {n: rig_now.r_to_gltf(v['r']) for n, v in local.items()},
            'translations': {n: rig_now.t_to_gltf(v['t']) for n, v in local.items()}}
    json.dump({'asset': asset, 'at': at, 'places': places, 'pose': pose}, open(os.path.join(out, 'first.json'), 'w'), indent=1)


def frames_from_video(video, name, out, size=(capture.WIDTH, capture.HEIGHT)):
    """A video's frames as <camera>_<NNN>.png (the size the camera was set for), and frames.json."""
    os.makedirs(out, exist_ok=True)
    v = cv2.VideoCapture(video); n = 0
    while True:
        ok, frame = v.read()
        if not ok: break
        Image.fromarray(cv2.resize(frame, size)[:, :, ::-1]).save(os.path.join(out, '%s_%03d.png' % (name, n))); n += 1
    meta_path = os.path.join(out, 'frames.json')
    meta = json.load(open(meta_path)) if os.path.exists(meta_path) else {'asset': video}
    meta['frames'] = n if 'frames' not in meta else min(meta['frames'], n)
    meta['times'] = [f / max(1, meta['frames'] - 1) for f in range(meta['frames'])]
    json.dump(meta, open(meta_path, 'w'))
    return n


def main():
    if sys.argv[1] == 'render':
        asset, n, out = sys.argv[2], int(sys.argv[3]), sys.argv[4]
        os.makedirs(out, exist_ok=True)
        times = [f / (n - 1) for f in range(n)]
        for name, cam in capture.CAMERAS.items():
            for f, im in enumerate(render.frames(asset, cam, times, capture.WIDTH, capture.HEIGHT, bones=capture.BONES)):
                im.save(os.path.join(out, '%s_%03d.png' % (name, f)))
        json.dump({'asset': asset, 'frames': n, 'times': times}, open(os.path.join(out, 'frames.json'), 'w'))
        json.dump(capture.CAMERAS, open(os.path.join(out, 'cameras.json'), 'w'), indent=1)
        print('rendered %d frames x %d cameras' % (n, len(capture.CAMERAS)))
        return
    if sys.argv[1] in ('first', 'firstplain'):           # firstplain: the character alone, lit (the joint reader's route)
        # The plain frame at 1920 x 1080 (a sharper start for the video model; the same camera, so the same picture at any size: the
        # video's frames are read back at the camera's 960 x 540).
        first_frame(sys.argv[2], float(sys.argv[3]), {k: capture.CAMERAS[k] for k in (sys.argv[5:] or capture.CAMERAS)}, sys.argv[4],
                    'drawn' if sys.argv[1] == 'first' else 'lit', (capture.WIDTH, capture.HEIGHT) if sys.argv[1] == 'first' else (1920, 1080))
        print('first frame and its joints written to', sys.argv[4])
        return
    if sys.argv[1] == 'frames':
        print('%d frames' % frames_from_video(sys.argv[2], sys.argv[3], sys.argv[4]))
        return
    out = sys.argv[2]
    args = sys.argv[3:]
    pose = args.pop(0) if args and args[0] != 'rig' else None          # a glTF whose frame 0 is the first frame's pose, or first.json
    meta = json.load(open(os.path.join(out, 'frames.json')))
    n = meta['frames']
    cameras = json.load(open(os.path.join(out, 'cameras.json'))) if os.path.exists(os.path.join(out, 'cameras.json')) else capture.CAMERAS
    cameras = {k: v for k, v in cameras.items() if os.path.exists(os.path.join(out, '%s_000.png' % k))}
    ORIGIN = characters.origin()
    if pose: T0 = {k: {b: np.array(v[0]) + ORIGIN for b, v in triangulate.truth_points(pose, capture.BONES, 1).items()} for k in cameras}
    else: T0 = {k: {b: np.array(v) for b, v in json.load(open(os.path.join(out, 'first.json')))['places'][k].items()} for k in cameras}
    result = {'asset': meta.get('asset'), 'frames': n, 'times': meta.get('times'), 'width': capture.WIDTH, 'height': capture.HEIGHT,
              'cameras': {}}
    cams, first, frames = {}, {}, {}
    for name, cfg in cameras.items():
        cam = camera.Camera.from_render(cfg, capture.WIDTH, capture.HEIGHT)
        cams[name] = (cam, ORIGIN)
        first[name] = {b: cam.project(T0[name][b])[0] - 0.5 for b in capture.BONES if b in T0[name]}
        frames[name] = [Image.open(os.path.join(out, '%s_%03d.png' % (name, f))).convert('RGB') for f in range(n)]
    if args and args[0] == 'rig':
        import fit_rig, gltf_rig
        rig_path, ikrig, calib = args[1:4]
        rig = gltf_rig.Rig(rig_path); axes = fit_rig.ik_rig_axes(ikrig)
        model = fit_rig.Model(rig, axes, fit_rig.rig_orders(gltf_rig.Rig(calib), axes, rig.names), fit_rig.ik_rig_limits(ikrig))
        balls, rolls = track_with_rig(frames, cams, first, model)
    else:
        balls, rolls = {name: track(frames[name], first[name]) for name in cams}, {}
    for name, cfg in cameras.items():
        result['cameras'][name] = {'camera': cfg, 'origin': ORIGIN, 'balls': balls[name]}
        if name in rolls: result['cameras'][name]['rolls'] = rolls[name]
        print('%s: %d of %d balls found' % (name, sum(v is not None for b in balls[name].values() for v in b), n * len(balls[name])))
    json.dump(result, open(os.path.join(out, 'obs.json'), 'w'), indent=1)


if __name__ == '__main__':
    main()
