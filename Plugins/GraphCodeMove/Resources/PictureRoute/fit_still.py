# Key poses from still pictures (MOVE-CAP1 E20, Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md): each still's joints as the reader found them
# (dino_reader.py stills: up to three peaks a joint) and the rig fitted to them with no frame before it. A still has no previous frame to
# start from and no motion to smooth by, so the fit starts from the rest pose, takes each joint's strongest peak, then the peak nearest
# where the fitted rig puts it, and tries the left and right legs both ways (the reader confuses a left and a right paw that look alike;
# a video's tracker settles it from the frame before); the fit with the smaller reprojection is kept.
# Usage: python fit_still.py test <stills_dir> <peaks.json> [--views 1|2] [--every N]   against the truth a dino_data.py set keeps (cameras known)
#        python fit_still.py test <stills_dir> <peaks.json> --unknown [--every N] [--down take_report.json]   each still alone, its camera
#        withheld and solved (fit_free); --down: the paws down at each frame, the floor fitted with the pose
import argparse, json, os, sys
import numpy as np
import camera, capture, characters, fit_rig, gltf_rig

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = characters.ROOT
PROFILE = characters.ACTIVE     # which character this run is for (characters.py): `--character <name>`
DEFAULTS = {   # the character as its skeleton is now (rig_now.py), as dino_take.py reads it
    'rig': PROFILE.rig, 'calib': PROFILE.calib, 'ikrig': PROFILE.ikrig,
    'skin': PROFILE.skin,        # skin_now.py: the built mesh's skin, for the ears' level (the cat's)
    'gcrig': PROFILE.gcrig,      # its feet's contacts (paw_pads), with its parents' chains
}


def model_of(a):
    """The rig to fit, each joint held to the IK rig's own one-sided range in the fit's angles (fit_rig.ik_rig_limits): with a two-sided
    bound a seated still folded its hock 44 degrees past the stop, GraphCode clamped it, and the lower leg stood 4 cm in the pelvis."""
    rig = gltf_rig.Rig(a.rig); axes = fit_rig.ik_rig_axes(a.ikrig)
    orders = fit_rig.rig_orders(gltf_rig.Rig(a.calib), axes, rig.names)
    model = fit_rig.Model(rig, axes, orders, fit_rig.ik_rig_limits(a.ikrig, ranges=True))
    model.skin = a.skin
    model.pads = paw_pads(a.gcrig, rig) if getattr(a, 'gcrig', None) else {}
    return model


def paw_pads(path, rig):
    """Each paw's contacts as the .gcrig writes them (`contacts { Sole_B_L = contact(chain=leg_B_L,at=$sole_B_L,bone=hock_L) }`), what
    its plants roll on: {the chain's last bone (the toe): [(the bone carrying the contact, its place at rest, Unreal cm)]}; a contact with
    no bone= rides the toe. A .gcrig's chains, points and contacts come with its parents' (`parent:`, characters.gcrig_text): the MetaHuman's
    leg_l / leg_r are the grandparent's, its Heel / Ball / Toe contacts the child's."""
    import re
    text = characters.gcrig_text(path)
    points = {m.group(1): np.array([float(v) for v in m.group(2).split(',')])
              for m in re.finditer(r'^\s*(\w+)\s*=\s*\(\s*(-?[\d.]+\s*,\s*-?[\d.]+\s*,\s*-?[\d.]+)\s*\)', text, re.M)}
    chains = {m.group(1): [b.strip() for b in m.group(2).split('->')] for m in re.finditer(r'^\s*(\w+)\s*=\s*(\w+(?:\s*->\s*\w+)+)', text, re.M)}
    contacts = {}                                                       # by name: a child's contact of a name replaces its parent's
    for m in re.finditer(r'^\s*(\w+)\s*=\s*contact\(chain=(\w+),\s*at=\$(\w+)(?:,\s*bone=(\w+))?\)', text, re.M):
        contacts[m.group(1)] = m.groups()[1:]
    pads = {}
    for chain, at, bone in contacts.values():
        toe = chains[chain][-1]
        pads.setdefault(toe, []).append((rig.index(bone or toe), points[at]))
    return pads


def mirrored(bone):
    return PROFILE.mirrored(bone)


def strongest(peaks, bones, swap=False, least=0.2):
    """Each joint's strongest peak (its mirror's, swapped), with its score as the weight."""
    seen, weight = {}, {}
    for j, b in enumerate(bones):
        q = peaks[j]
        if q and q[0][2] > least:
            name = mirrored(b) if swap else b
            seen[name] = np.array(q[0][:2]); weight[name] = max(0.05, q[0][2])
    return seen, weight


def nearest(peaks, bones, projected, gate=40.0):
    """Each joint's peak nearest where the fitted rig puts it, within `gate` px: either side's peaks may be its, or only its own side's
    where the profile says the sides are distinct (a person: the reader is right, and the other side's peaks let the fit settle mirrored)."""
    by = {b: peaks[j] for j, b in enumerate(bones)}
    seen, weight = {}, {}
    either = not PROFILE.sides_distinct
    for b, at in projected.items():
        best = None
        for q in (by.get(b) or []) + (by.get(mirrored(b)) or [] if either and mirrored(b) != b else []):
            d = np.linalg.norm(np.array(q[:2]) - at)
            if d < gate and (best is None or d < best[0]): best = (d, q)
        if best: seen[b] = np.array(best[1][:2]); weight[b] = max(0.05, best[1][2])
    return seen, weight


def fit(model, shots, start, rounds=2):
    """shots: [(Camera, origin, peaks)] of one moment. Both ways round the legs; returns (params, mean px, how)."""
    U = lambda p: gltf_rig.to_unreal(model.positions(p))
    best = None
    for swap in (False, True):
        views = [(cam, origin) + strongest(pk, capture.BONES, swap) for cam, origin, pk in shots]
        p, errs = fit_rig.solve_views(model, views, start, loss='soft_l1', scale=3.0)
        for _ in range(rounds):
            P = U(p)
            views = [(cam, origin) + nearest(pk, capture.BONES, {b: cam.project(P[model.rig.index(b)] + np.asarray(origin))[0]
                                                                  for b in capture.BONES}) for cam, origin, pk in shots]
            p, errs = fit_rig.solve_views(model, views, p, loss='soft_l1', scale=3.0)
        px = float(np.mean(np.concatenate(errs))) if errs else np.inf
        if best is None or px < best[1]: best = (p, px, 'swapped' if swap else 'as read')
    return best


def rotation(v):
    """A turn by its axis times its angle (radians)."""
    a = np.linalg.norm(v)
    if a < 1e-12: return np.eye(3)
    k = v / a; K = np.array([[0, -k[2], k[1]], [k[2], 0, -k[0]], [-k[1], k[0], 0]])
    return np.eye(3) + np.sin(a) * K + (1 - np.cos(a)) * K @ K


class Lens:
    """A camera at the origin looking along +X (camera.Camera's axes: +Y image right, +Z up) whose focal length is the one unknown
    left: the body is placed in front of it, so its place and aim are the body's, solved with the pose (a still from an image model
    has no camera to read)."""
    def __init__(self, width, height, fov=40.0):
        self.width, self.height = width, height
        self.focal = (width / 2.0) / np.tan(np.radians(fov) / 2.0)

    def project(self, P, scale=1.0):
        P = np.atleast_2d(P)
        return np.stack([self.width / 2.0 + self.focal * scale * P[:, 1] / P[:, 0], self.height / 2.0 - self.focal * scale * P[:, 2] / P[:, 0]], 1)


LIMBS = PROFILE.limbs   # each leg's joints below its root, its partner, and the pair whose middle is the body's mid-plane there (characters.py:
# the cat's four legs, the MetaHuman's two; its arms are not planted limbs)
STAND = 100.0   # px a radian a stance's down paw tips sideways (fit_views_free `stand`), the floor's weight over a paw's few cm
STANDS = PROFILE.stands   # a down paw (its toe) -> the bone
# that carries its pads (the leg chain's 4th, GcMoveStoodPaw's paw); only hinges lie below it, so its side-to-side line is the paw's


def upright(yaw, pitch):
    """The body's turn before a level lens (no roll, as a picture's horizon has none) tipped `pitch` down, the body turned `yaw` round its
    own up: lens = upright @ body. The body's up is the world's, so its root stands on a level floor."""
    return rotation(np.array([0.0, -pitch, 0.0])) @ rotation(np.array([0.0, 0.0, yaw]))


def fit_free(model, peaks, size, **kw):
    """One still, its camera unknown (fit_views_free with one picture). Returns (pose params, turn as axis-angle, place, focal factor,
    mean px, how, pitch down in radians), or None when too few joints were read."""
    got = fit_views_free(model, [(peaks, size)], **kw)
    if got is None: return None
    return (got[0],) + tuple(got[1][0])


def fit_views_free(model, views, rounds=2, yaws=tuple(range(0, 360, 45)), keep=2, fov=40.0, down=(), floor=30.0, tol=1e-6, mirror=10.0,
                   support=1.0, lateral=0.3, level=None, stand=0.0, natural=10.0):
    """Pictures of ONE pose, each its own camera unknown (one picture, or several from different sides): the pose, how the body is turned and placed before a Lens, and the focal length (a factor on the
    lens's, kept near it). The lens is level and tipped down by a pitch solved here, and the body's root stands upright on the floor
    (`upright`), so the body's lean is its pelvis's, inside the rig's limits; a lean toward or away from the lens, which one picture
    cannot see, stays near the rest pose's. 2026-10-26: with the turn free (three angles) and the floor fitted after it, the floor's
    up came out 8.1 degrees off the truth on average (18.6 worst) on the test walk.
    The rest pose alone is turned to face the lens from each of `yaws` (degrees round the body's up) a little from above, the legs both
    ways round; the whole fit runs from each side's `keep` best, and the least reprojection wins. `down`: the joints on the floor in
    this still (its down paws' toes, as the author says them), each at its own rest height (its pad) on one floor (`floor` px a cm),
    which also settles the depth one picture leaves open for those paws. A down paw is a fact the picture shows, so the floor outweighs
    the pictures: at 1 px a cm a seated still stood one hind toe 2.8 cm under the floor and the other 2.8 above, and GraphCode's plants
    wrenched each back down, folding the hocks 32-49 degrees past their stops (2026-10-27). The down paws' middle stands under the body's (hips and shoulders) sideways (`support` px a
    cm): one picture cannot see depth, and Codex's side-view stretch had all four paws 18 cm to one side of the body (2026-10-26).
    Each leg joint (knee or elbow down to the toe) keeps, softly (`lateral` px a cm), its rest offset sideways from its hip or shoulder:
    from the side, a leg's swing toward or away from the lens does not show, and a picture whose cat is not quite our sculpt (an image
    model's) makes the fit tilt a leg in depth to match its drawn length; Codex's standing cat came out with its paws crossed and an
    elbow 1.3 cm inside the chest (refused by the build's self-collision check).
    A leg the reader did not see (at most one of its four lower joints read at least half sure in any picture), whose partner it saw
    (three or more), copies the partner mirrored across the body's middle in a stance (`stand`, all four paws down: standing, sitting,
    stretching), after the whole fit and by its own joints alone (copied()): the partner stays as its pictures put it. A sitting cat's
    far hind leg, hidden behind the near one, otherwise stays as rest leaves it and stood out in front of the front paws (Codex's sit,
    2026-10-26); read off the near paw twice, it stood on its pads, heel 6 cm up, where the near one sits on its heel (user 2026-10-29:
    "can we make it such that the other leg mirrors the correct one?"). ~~Pulled to the partner's mirror inside the whole fit
    (`mirror` px a cm, at most one joint read at all)~~: the pull moved both legs, and with the half-sure count the near hind leg
    left its heel (0.03 -> 2.2 cm), 2026-10-29. Not in a stride: with only the pair down, a test-walk still whose far foreleg was
    hidden went from 2.0 to 5.8 cm.
    `tol`: the whole fit's xtol and ftol; 1e-6 against 1e-9 on the 21 test stills, 2026-10-26:
    pose 1.86 vs 1.96 cm mean (worst 16.7 both), the floor 2.9 vs 3.2 degrees, 2.5 times fewer evaluations; 1e-4 stopped short (2.46,
    one 29 cm).
    The head holds its eyes level (`level` px a cm the ear tips stand off level, as their skin poses them: posed_ear_tips()): the reader reads no point on the face, only
    two on the back of the skull 5 cm apart, so at its ~10 px the head's tilt was the reader's noise, and Codex's sit came out with the
    head rolled 30 degrees where both pictures have its eyes level (2026-10-27). Its turn and nod are the pictures'. Levelled after
    the whole fit, by the neck and head alone (levelled()). 30 px a cm, the floor's weight: level eyes are a fact the picture shows, as
    a down paw is; at 3, with the ears measured as drawn, the sit key's stood 24 degrees off level (0.3 at 30, 0.3 px more; 2026-10-27).
    In a stance (`stand` > 0: still_gcmove passes STAND for stills timed by seconds) a down paw's pads face the floor (`stand` px a
    radian): its side-to-side line (STANDS' bone, square to its toes and the up at rest) stays level, the one turn of a paw the read
    points cannot show; how steep it lies (wrist, paw and toe are read) is the pictures'. Not in a stride (0, the default): a walking
    paw tips as it lifts, and on the 21 test stills levelling it cost 1.73 -> 1.92 cm mean.
    And no contact of a down paw stands under the floor (what its plants roll on, paw_pads: heel, pad and toes; `floor` px a cm under
    the floor the toes stand on, the rest pose's z 0; nothing above it): fitted by its toes alone, Codex's seated left hind paw stood its
    pad 2.9 cm into the floor, toes up, the knee at its stop, and GraphCode's plant then curled the paw up, pads to the camera
    (2026-10-29). ~~Each pad at or above its rest height~~ lifted that seat 3.1 cm and left a straight foreleg short of the floor, and
    ~~each pad AT its rest height~~ stood the seated hind leg upright on its pads: a cat sits on the back of its metatarsus, its heel.
    Left to the fit, Codex's bow put a forepaw on its side, pads to the camera (2026-10-27). ~~The whole bind turn but for the floor's
    up (what `plant(leg, on=$Floor)` stands)~~: it forced a standing cat's steep paw on the bow, whose forearms then came down steep,
    paws bunched; on the 21 test stills it cost 1.73 -> 1.95 cm mean (4.78 -> 5.29 at 95 %). A twist (a turn
    along a bone, which no joint's place shows) keeps to rest at `natural` px a radian (user 2026-10-27: no data, the most natural
    pose; the right shoulder's had sat at its -20 stop); every other turn at 0.01, as before. On the test walk 1.68 -> 1.73 cm mean.
    The pose is the pictures'; a limb they put inside the body is GraphCode's to push out (`collision(self)`, user 2026-10-27: "use the
    pose first, then push out by contact"), not this fit's.
    Several pictures share the pose and each keeps its own lens, so one picture's unseen depth is another's width: Codex drew each pose
    from the side and from three-quarters, and from the side alone a foreleg crossed into the chest (2026-10-26). The pictures need not
    be one moment seen twice (an image model draws them apart); the robust loss lets their small differences go.
    Returns (pose params, [(turn as axis-angle, place, focal factor, mean px, how, pitch down in radians) a picture]), or None when too
    few joints were read.
    The character's own rules are its profile's (characters.py): how many feet down fit a floor (the cat two, the MetaHuman one: with ONE
    foot down its own lowest contacts are held at their rest height and its middle at its rest offset from the body's, which only fixes
    the gauge the body's free place leaves; the floor's tilt is the level lens and the upright root's), the pairs whose middle is the
    mid-plane, whether a hidden leg copies its partner (four paws), and `level` (None: the profile's; 0 leaves the head as read)."""
    import itertools
    from scipy.optimize import least_squares
    prof = PROFILE
    if level is None: level = prof.level
    V = len(views)
    lenses = [Lens(size[0], size[1], fov) for _, size in views]
    U = lambda p: gltf_rig.to_unreal(model.positions(p))
    rest = model.params_of(model.rig.rest_r, model.rig.rest_t)
    P0 = U(rest); centre = P0.mean(0); extent = np.ptp(P0, 0).max()
    names = capture.BONES
    idx = [model.rig.index(b) for b in names]
    n = model.size
    off = lambda v: n + 6 * v                                            # a picture's [yaw, pitch down, place x3, log focal factor]
    lo = np.concatenate([model.lower] + [[-np.inf, np.radians(-30.0), -np.inf, -np.inf, -np.inf, np.log(0.5)]] * V)
    hi = np.concatenate([model.upper] + [[np.inf, np.radians(85.0), np.inf, np.inf, np.inf, np.log(2.0)]] * V)
    toes = [model.rig.index(b) for b in down] if len(down) >= prof.floor_feet else []
    nf = len(toes); C = (np.eye(nf) - 1.0 / nf if nf > 1 else np.eye(nf)) if nf else None   # each toe's height less the toes' mean (one foot: its own)
    mids = [model.rig.index(b) for b in prof.mids]
    ns = 1 if nf else 0                                                  # the support row: the toes' middle less the body's, sideways
    roots = [(model.rig.index(j), model.rig.index(mid[0] if PROFILE.is_left(leg) else mid[1])) for leg, (js, _, mid) in LIMBS.items() for j in js]
    wide = np.array([P0[t, 0] - P0[r, 0] for t, r in roots])            # each leg joint's rest offset sideways from its hip or shoulder
    up = np.array([0.0, 1.0, 0.0])                                      # the floor's up in the rig's own (glTF) frame
    G0 = model.positions(rest)
    stands = []                                                         # (the pads' bone, its side-to-side line at rest)
    for t in (down if stand else []):
        if STANDS.get(t) not in model.rig.names: continue
        s = model.rig.index(STANDS[t]); side = np.cross(up, G0[model.rig.index(t)] - G0[s])
        stands.append((s, side / np.linalg.norm(side)))
    nt = len(stands)
    M = gltf_rig.to_unreal(np.eye(3)).T                                 # glTF metres -> Unreal cm, one fixed linear map
    pads = getattr(model, 'pads', {})
    soles = [[(b, np.linalg.solve(M, at - P0[b])) for b, at in pads[t]] for t in down] if stand and nf and all(t in pads for t in down) else []
    def lowest(x, P, soles):
        """Each down paw's lowest contact (a sole's: [(its bone, its offset at rest, glTF)]): its height, its bone, how its offset turns."""
        out = []
        for sole in soles:
            got = [(P[b, 2] + (M @ v)[2], b, D) for b, d in sole for v, D in [model.carried(x[:n], b, d)]]
            out.append(min(got, key=lambda g: g[0]))
        return out
    pull = np.full(n - 3, 1e-2)                                         # each turn's pull to rest; a twist's is `natural`
    k = 0
    for i, order in model.joints:                                       # the twist: the IK rig's X, along the bone (a .gcrig's twists=)
        for axis in order:
            if axis == fit_rig.UE_TO_GLTF_AXIS['X']: pull[k] = natural
            k += 1
    turn = lambda x, v: upright(x[off(v)], x[off(v) + 1])

    def placed(x, v):
        return (U(x[:n]) - centre) @ turn(x, v).T + x[off(v) + 2:off(v) + 5]

    def robust(r, f=3.0):
        """The picture's reads, robust (soft_l1 at f px: a misread joint pulls little), as residuals whose squares are that loss, and
        each one's derivative by its raw residual. Only the reads: the floor, the stance and the priors stay quadratic, as hard as they
        are weighted. Robust too (least_squares' loss on every row, to 2026-10-29), a fact gave way wherever it was far off: the sit's
        seated far hind paw stood 2.5 cm under the floor while the others stood 0.4-1.3 cm over it, its toes bent up past 90 degrees."""
        s = np.sqrt(1.0 + (r / f) ** 2); g = np.sqrt(2.0 / (s + 1.0))   # rho = 2 f^2 (s - 1) = (r g)^2, with no cancellation near 0
        return r * g, 1.0 / (s * g)

    s0 = float(P0[toes, 0].mean() - P0[mids, 0].mean()) if nf == 1 else 0.0   # one foot: its sideways offset from the body's middle at rest

    def solve(x0, seens, soles=soles, hard=True):
        base = np.array([z for z, _, _ in lowest(rest, P0, soles)]) if nf == 1 and soles else 0.0   # one foot: its lowest contact's rest height
        bs = [[idx[names.index(k)] for k in seen] for seen, _ in seens]
        Ts = [np.array([seen[k] for k in seen]) for seen, _ in seens]
        Ws = [np.array([weight[k] for k in seen])[:, None] for seen, weight in seens]
        ms = [len(b) for b in bs]
        def pixels(x, P):
            return np.concatenate([(Ws[v] * (lenses[v].project((P[bs[v]] - centre) @ turn(x, v).T + x[off(v) + 2:off(v) + 5],
                                                                np.exp(x[off(v) + 5])) - Ts[v])).ravel() for v in range(V)])
        def resid(x):
            P = U(x[:n])
            return np.concatenate([robust(pixels(x, P))[0] if hard else pixels(x, P), pull * x[3:n], 2.0 * x[[off(v) + 5 for v in range(V)]],
                                         floor * C @ ((np.array([z for z, _, _ in lowest(x, P, soles)]) - base) if soles else P[toes, 2] - P0[toes, 2]) if nf else [], [support * (P[toes, 0].mean() - P[mids, 0].mean() - s0)] if ns else [],
                                         lateral * (np.array([P[t, 0] - P[r, 0] for t, r in roots]) - wide)]
                                  + [[stand * (model.carried(x[:n], s, side)[0] @ up) for s, side in stands]])
        def jac(x):
            # Exact, from the rig's own derivative (fit_rig.Model.jacobian): the pose's columns through each picture's turn, place and
            # lens; the yaws' and pitches' by differences of the turn alone, which costs no forward pass.
            G, J = model.jacobian(x[:n])
            B = np.einsum('ij,mjk->mik', M, J)                             # every node's cm by each pose parameter
            out = np.zeros((2 * sum(ms) + n - 3 + V + nf + ns + len(roots) + nt, len(x)))
            r0 = 0
            for v in range(V):
                o, m, W = off(v), ms[v], Ws[v]
                Rt = turn(x, v); A = gltf_rig.to_unreal(G[bs[v]]) - centre; Q = A @ Rt.T + x[o + 2:o + 5]
                f = lenses[v].focal * np.exp(x[o + 5]); X = Q[:, 0]
                D = np.zeros((m, 2, 3))                                   # pixel by the point before the lens
                D[:, 0, 0] = -f * Q[:, 1] / X ** 2; D[:, 0, 1] = f / X; D[:, 1, 0] = f * Q[:, 2] / X ** 2; D[:, 1, 2] = -f / X
                rows = slice(r0, r0 + 2 * m)
                out[rows, :n] = (W[:, :, None] * np.einsum('mij,mjk->mik', D, np.einsum('ij,mjk->mik', Rt @ M, J[bs[v]]))).reshape(2 * m, n)
                for c in (0, 1):
                    e = np.zeros(2); e[c] = 1e-7
                    dR = (upright(x[o] + e[0], x[o + 1] + e[1]) - upright(x[o] - e[0], x[o + 1] - e[1])) / 2e-7
                    out[rows, o + c] = (W * np.einsum('mij,mj->mi', D, A @ dR.T)).ravel()
                out[rows, o + 2:o + 5] = (W[:, :, None] * D).reshape(2 * m, 3)
                out[rows, o + 5] = (W * np.stack([f * Q[:, 1] / X, -f * Q[:, 2] / X], 1)).ravel()
                r0 += 2 * m
            out[r0 + np.arange(n - 3), 3 + np.arange(n - 3)] = pull; r0 += n - 3
            for v in range(V): out[r0 + v, off(v) + 5] = 2.0
            r0 += V
            if nf:                                                         # a toe's height is its body Z; a sole's, its bone's and its offset's
                out[r0:r0 + nf, :n] = floor * C @ (np.array([B[b, 2] + (M @ D)[2] for _, b, D in lowest(x, gltf_rig.to_unreal(G), soles)]) if soles else B[toes, 2])
                r0 += nf
            if ns: out[r0, :n] = support * (B[toes, 0].mean(0) - B[mids, 0].mean(0)); r0 += 1
            for k, (t, r) in enumerate(roots): out[r0 + k, :n] = lateral * (B[t, 0] - B[r, 0])
            r0 += len(roots)
            for s, side in stands: out[r0, :n] = stand * (up @ model.carried(x[:n], s, side)[1]); r0 += 1
            if hard: out[:2 * sum(ms)] *= robust(pixels(x, gltf_rig.to_unreal(G)))[1][:, None]
            return out
        r = least_squares(resid, np.clip(x0, lo, hi), jac=jac, bounds=(lo, hi), max_nfev=400, xtol=tol, ftol=tol,
                          **({} if hard else dict(loss='soft_l1', f_scale=3.0)))
        pxs = [float(np.mean(np.linalg.norm(lenses[v].project(placed(r.x, v)[bs[v]], np.exp(r.x[off(v) + 5])) - Ts[v], axis=1)))
               for v in range(V)]
        return r.x, pxs, float(r.cost)

    def rigid(y, b, T, W, lens):
        # The rest pose turned, placed and seen: [yaw, pitch, place, log focal] alone, which says where a whole fit is worth starting.
        Q = (P0[b] - centre) @ upright(y[0], y[1]).T + y[2:5]
        return np.concatenate([(W * (lens.project(Q, np.exp(y[5])) - T)).ravel(), [2.0 * y[5]]])

    # Each picture's starts: the rest pose turned to face its lens from each of `yaws`, the legs both ways round; its `keep` best a
    # way round (one each when there are several pictures, whose ways round then combine).
    choices = []
    for v, (peaks, size) in enumerate(views):
        lens, mine = lenses[v], []
        for swap in ((False,) if PROFILE.sides_distinct else (False, True)):     # distinct sides: no label-swapped start
            seen, weight = strongest(peaks, names, swap)
            if len(seen) < 6: continue
            b = [idx[names.index(k)] for k in seen]
            T = np.array(list(seen.values())); W = np.array([weight[k] for k in seen])[:, None]
            depth = lens.focal * extent / max(np.ptp(T, 0).max(), 1.0)   # how far the body stands for its picture's size
            aim = T.mean(0)
            ray = np.array([1.0, (aim[0] - size[0] / 2.0) / lens.focal, -(aim[1] - size[1] / 2.0) / lens.focal])
            tried = []
            for yaw in yaws:
                # Facing the lens from `yaw`, seen a little from above; placed on the ray through the picture's middle of the joints.
                r = least_squares(rigid, np.concatenate([np.radians([yaw, 15.0]), ray * depth, [0.0]]), args=(b, T, W, lens),
                                  bounds=(lo[n:n + 6], hi[n:n + 6]), loss='soft_l1', f_scale=3.0)
                tried.append((r.cost, yaw, r.x))
            mine += [(swap, yaw, y, (seen, weight)) for _, yaw, y in sorted(tried, key=lambda t: t[0])[:keep if V == 1 else 1]]
        if not mine: return None
        choices.append(mine)
    best = None
    for combo in itertools.product(*choices):                             # the whole fit from each combination of starts
        # Kept by the whole cost, the priors with the pixels: from the side a leg crossed in depth and one where it belongs match the
        # picture alike, and choosing by pixels alone kept crossed forelegs (an elbow inside the chest, Codex's sit, 2026-10-26). The floor
        # under the pads does not vote: it says where the pose may go, not how the picture reads; with it, the seated still's reading
        # that its rig could match only with a pad 2.9 cm under the floor cost more than its mirror (its lens put 12 deg UP), 2026-10-29.
        # And every row is soft here, as the reading was always chosen (hard=False): with the floor hard in the choice too, the bow's
        # picture read its legs the other way round (13.3 / 11.1 px against 10.2 / 10.2). The refinement below holds the floor hard.
        x, pxs, cost = solve(np.concatenate([rest] + [y for _, _, y, _ in combo]), [sw for _, _, _, sw in combo], soles=[], hard=False)
        if best is None or cost < best[2]:
            best = (x, pxs, cost, [('swapped' if swap else 'as read') + ' from %d deg' % yaw for swap, yaw, _, _ in combo])
    x, pxs, _, hows = best
    def apart(read, size):
        """A stance's reads with every left/right pair at one spot taken out (within 1% of the picture's width): the reader could not
        tell the two apart, and either taken as both put the hidden one where the near one is. In the seated three-quarter picture both
        hind paws read within 10-11 px, the near paw twice; the far leg was fitted to it, upright under the hip with its knee at its fold
        stop, its pads on their side, and its hip jerked 34 degrees in a frame as its paw slid to the seat (2026-10-29)."""
        seen, weight = read
        if not stand: return seen, weight
        both = [b for b in seen if b.endswith(prof.left) and mirrored(b) in seen and np.linalg.norm(seen[b] - seen[mirrored(b)]) < 0.01 * size[0]]
        drop = set(both) | {mirrored(b) for b in both}
        return {b: p for b, p in seen.items() if b not in drop}, {b: w for b, w in weight.items() if b not in drop}
    for _ in range(rounds):
        seens = []
        for v, (peaks, size) in enumerate(views):
            Q = placed(x, v)
            seens.append(apart(nearest(peaks, names, {j: lenses[v].project(Q[model.rig.index(j)], np.exp(x[off(v) + 5]))[0] for j in names}), size))
        if all(len(seen) >= 6 for seen, _ in seens): x, pxs, _ = solve(x, seens)

    def copied(x, seens, sure=0.5):
        """The fitted pose with each unseen leg copying its seen partner mirrored, the rest held: only the unseen leg's joints turn (its
        root, the hip or shoulder, down to its paw), its joints toward the partner's mirrored across the upright plane square to the
        pair of roots through their middle (`mirror` px a cm), its paw's lowest contact on the floor. Its reads take no part: they were
        the partner's, read twice. Upright, so the copy stands at its partner's heights, on the floor: the seated still's hips lie 7
        degrees off level, and mirrored across their own middle the copied paw stood 1.2 cm into the floor; lifted out by the floor, the
        copy twisted its hip to the 30-degree stop and set its knee 2.8 cm wide (2026-10-29). ~~And its pads level (`stand`)~~: the
        partner's lie 10 degrees off, and below the hip a leg has only hinges, so levelling the copy's paw twisted its hip to the stop
        again (knee 2.1 cm wide); the copy takes the partner's tilt, mirrored."""
        best = {}
        for seen, weight in seens:
            for j in seen: best[j] = max(best.get(j, 0.0), weight[j])
        count = lambda leg: sum(best.get(j, 0.0) >= sure for j in LIMBS[leg][0])
        for leg, (js, partner, mid) in LIMBS.items():
            if count(leg) > 1 or count(partner) < 3: continue
            P = U(x[:n]); a, c = model.rig.index(mid[0]), model.rig.index(mid[1])
            across = (P[a] - P[c]) * [1.0, 1.0, 0.0]; across /= np.linalg.norm(across); middle = (P[a] + P[c]) / 2.0
            them = [model.rig.index(j) for j in js]
            T = np.array([P[q] - 2.0 * ((P[q] - middle) @ across) * across for q in (model.rig.index(j) for j in LIMBS[partner][0])])
            root = a if PROFILE.is_left(leg) else c
            chain, i = [root], them[-1]
            while i != root: chain.append(i); i = model.rig.parent[i]
            cols, k = [], 3
            for i, order in model.joints:
                if i in chain: cols += range(k, k + len(order))
                k += len(order)
            cols = np.array(cols)
            def resid(y):
                z = x.copy(); z[cols] = y; P = U(z[:n])
                heights = np.array([h for h, _, _ in lowest(z, P, soles)]) if soles else P[toes, 2] - P0[toes, 2]
                return np.concatenate([mirror * (P[them] - T).ravel(), pull[cols - 3] * y, floor * C @ heights])
            r = least_squares(resid, np.clip(x[cols], lo[cols], hi[cols]), bounds=(lo[cols], hi[cols]), xtol=tol, ftol=tol)
            x = x.copy(); x[cols] = r.x
            off_by = np.linalg.norm(U(x[:n])[them] - T, axis=1)
            print('    leg %s unseen (%d of its joints read half sure; %s %d): copied from %s mirrored, its joints %.2f cm off it (worst %.2f)'
                  % (leg, count(leg), partner, count(partner), partner, off_by.mean(), off_by.max()), flush=True)
        return x
    if stand and prof.four_paw_mirror and nf == len(LIMBS) and all(l[0][-1] in down for l in LIMBS.values()) and rounds: x = copied(x, seens)
    # ~~A leg whose shape the pictures do not show, started again from its partner's mirrored~~ (tried 2026-10-29 on the sit's seated far
    # hind leg, knee and hock unread): kept by cost, the leg let go back to the same shape, on its pads with the heel 7.8 cm up. Both seated
    # pictures are of one side; the reader puts both hind paws within 11 px in the three-quarter one (the near paw read twice), so no
    # picture says where the far leg is: a picture from its side does.
    if level and 'head' in model.rig.names: x, pxs = levelled(model, x, views, lenses, placed, off, lo, hi, level)
    return x[:n], [(rotation_vector(turn(x, v)), x[off(v) + 2:off(v) + 5], float(np.exp(x[off(v) + 5])), pxs[v], hows[v], float(x[off(v) + 1]))
                   for v in range(V)]


def ear_tips(model):
    """The two ear tips at rest (glTF m), each with the bones its skin follows and how much: the highest skin either side of the head,
    among the vertices the head and the bones under it move most. The body's left-right is not it: the sculpt's head is turned 44
    degrees to its side at rest, and levelling that line left Codex's sit key with its ear tips 25 degrees off level (2026-10-27).
    The skin is the built mesh's (skin_now.py, `skin`), not the rig template's, which keeps an older export's: that gave each tip
    0.18-0.24 to the neck where the built cat gives 0.33, and put one tip 2.8 cm off (2026-10-27)."""
    if getattr(model, '_ear', None) is not None: return model._ear
    rig = model.rig
    skin = json.load(open(model.skin))
    U = np.array(skin['positions']); W = skin['weights']                  # the mesh's own space (Unreal cm)
    head = rig.index('head')
    under = {rig.names[i] for i in {head} | {i for i in range(len(rig.names)) if any(a == head for a in ancestors(rig, i))}}
    mine = np.array([v for v, w in enumerate(W) if w and max(w, key=w.get) in under])
    rest = model.params_of(rig.rest_r, rig.rest_t)
    P = gltf_rig.to_unreal(model.positions(rest))
    ahead = P[head] - P[rig.index('neck')]; ahead[2] = 0.0
    across = np.cross(ahead, [0.0, 0.0, 1.0])                             # the head's own sideways at rest, by where it points
    side = (U[mine] - P[head]) @ across > 0
    tips = [mine[side][np.argmax(U[mine][side][:, 2])], mine[~side][np.argmax(U[mine][~side][:, 2])]]
    model._ear = []
    for k in tips:
        missing = [b for b in W[k] if b not in rig.names]
        if missing: raise SystemExit('the skin (%s) names bones the rig template has not: %s' % (model.skin, missing))
        total = sum(W[k].values())
        model._ear.append((np.array([U[k][0], U[k][2], U[k][1]]) / 100.0, np.array([rig.index(b) for b in W[k]]),
                           np.array([x / total for x in W[k].values()])))
    model._ear_rest = model.frames(rest)
    return model._ear


def posed_ear_tips(model, params):
    """Where the skin puts the two ear tips in a pose (glTF m): each follows its bones by its weights (linear blend, as the engine draws
    it). The built cat's tips are a third the neck's (its head band covers the whole skull, BritishShorthairCat.gcrig and
    GC-MOVE-CAT-PLAN.md W42 (d)), so a line the head bone alone carried said 2.8 degrees off level at Codex's sit key where the drawn
    ears stood 11.6 off on the template's older skin (2026-10-27)."""
    tips = ear_tips(model); Wr, Pr = model._ear_rest
    Wp, Pp = model.frames(params)
    return [sum(w * (Wp[j] @ Wr[j].T @ (v - Pr[j]) + Pp[j]) for j, w in zip(js, ws)) for v, js, ws in tips]


def ancestors(rig, i):
    while rig.parent[i] >= 0:
        i = rig.parent[i]; yield i


def levelled(model, x, views, lenses, placed, off, lo, hi, level, joints=('neck', 'head')):
    """The fitted pose with its head turned level, the rest held: only the neck and head turn, to keep the eyes level (`level` px a cm
    the ear tips stand off level, posed_ear_tips()) and the head's own read points (neck, head, skull_support) where the pictures put them.
    Levelled after the whole fit, not in it: inside it the term moved the body too (Codex's sit key: the left forearm's twist 14 -> 45
    degrees, the right hock 5 more), and the build then slid the right hip 55 degrees short (2026-10-27)."""
    from scipy.optimize import least_squares
    n = model.size
    cols, k = [], 3
    for i, order in model.joints:
        if model.rig.names[i] in joints: cols += range(k, k + len(order))
        k += len(order)
    M = gltf_rig.to_unreal(np.eye(3)).T
    ear_tips(model)
    names = capture.BONES
    seens = []
    for v, (peaks, _) in enumerate(views):
        Q = placed(x, v)
        seen, weight = nearest(peaks, names, {j: lenses[v].project(Q[model.rig.index(j)], np.exp(x[off(v) + 5]))[0] for j in names})
        mine = [j for j in seen if j in ('neck', 'head', 'skull_support')]
        seens.append(([model.rig.index(j) for j in mine], np.array([seen[j] for j in mine]), np.array([weight[j] for j in mine])[:, None]))
    def resid(y):
        z = x.copy(); z[cols] = y
        rows = [(W * (lenses[v].project(placed(z, v)[b], np.exp(z[off(v) + 5])) - T)).ravel() for v, (b, T, W) in enumerate(seens) if b]
        A, B = posed_ear_tips(model, z[:n])
        return np.concatenate(rows + [[level * (M @ (A - B))[2]]])
    r = least_squares(resid, x[cols], bounds=(lo[cols], hi[cols]), loss='soft_l1', f_scale=3.0)
    x = x.copy(); x[cols] = r.x
    pxs = []
    for v, (peaks, _) in enumerate(views):
        Q = placed(x, v)
        seen, _ = nearest(peaks, names, {j: lenses[v].project(Q[model.rig.index(j)], np.exp(x[off(v) + 5]))[0] for j in names})
        pxs.append(float(np.mean(np.linalg.norm(lenses[v].project(Q[[model.rig.index(j) for j in seen]], np.exp(x[off(v) + 5]))
                                                - np.array(list(seen.values())), axis=1))))
    return x, pxs


def rotation_vector(R):
    """The axis-angle of a turn matrix."""
    a = np.arccos(np.clip((np.trace(R) - 1) / 2, -1, 1))
    if a < 1e-9: return np.zeros(3)
    v = np.array([R[2, 1] - R[1, 2], R[0, 2] - R[2, 0], R[1, 0] - R[0, 1]]) / (2 * np.sin(a))
    return v * a


def kabsch(A, T):
    """The turn that brings A's points (about their middle) nearest T's."""
    U_, _, Vt = np.linalg.svd((A - A.mean(0)).T @ (T - T.mean(0)))
    return Vt.T @ np.diag([1, 1, np.sign(np.linalg.det(Vt.T @ U_.T))]) @ U_.T


def aligned(A, T):
    """A's points moved by the turn and shift that bring them nearest T (Kabsch): the pose alone, wherever it stands."""
    return (A - A.mean(0)) @ kabsch(A, T).T + T.mean(0)


def errors(model, p, truth, origin):
    """Per joint, cm from the true place; and the same with both poses moved to put their pelvis at one place (the pose alone)."""
    P = gltf_rig.to_unreal(model.positions(p)) + np.asarray(origin)
    names = [b for b in capture.BONES if b in truth]
    A = np.array([P[model.rig.index(b)] for b in names]); T = np.array([truth[b] for b in names])
    k = names.index('pelvis')
    return np.linalg.norm(A - T, axis=1), np.linalg.norm((A - A[k]) - (T - T[k]), axis=1)


def test_unknown(a, model, rows, peaks):
    """Each still alone with its camera withheld: the pose, after the turn and shift that best match it to the truth (with no camera
    the place is the image model's to choose), and the field of view found against the one rendered. With `--down <take_report.json>`
    (the paws down at each frame), the floor is fitted with the pose and its up measured against the truth's (degrees)."""
    plain = [r for r in rows if r['look'] in ('lit', 'unlit') and r['image'] in peaks][::a.every]
    if a.views > 1:   # each still with the other views of its moment and look: one pose, each picture its own unknown camera
        group = {}
        for q in rows:
            if q['look'] in ('lit', 'unlit') and q['image'] in peaks: group.setdefault((q['frame'], q['look']), []).append(q)
    stance = json.load(open(a.down))['stance'] if a.down else {}
    E, fovs, ups, unfitted = [], [], [], 0
    for r in plain:
        down = [p for p, steps in stance.items() if r['frame'] in steps]
        if a.views > 1:
            others = [q for q in group[(r['frame'], r['look'])] if q['image'] != r['image']][:a.views - 1]
            got = fit_views_free(model, [(peaks[q['image']], (q['width'], q['height'])) for q in [r] + others], down=down, tol=a.tol)
            if got is None:   # the reader found fewer than six joints (a reader taught a pass or two): said, not a crash (slice 5, CODER-NOTES-13.md)
                print('%s  not fitted: the reader found fewer than six joints' % r['image'], flush=True); unfitted += 1; continue
            p, (turn, place, factor, px, how, pitch) = got[0], got[1][0]
        else:
            p, turn, place, factor, px, how, pitch = fit_free(model, peaks[r['image']], (r['width'], r['height']), down=down, tol=a.tol)
        Q = gltf_rig.to_unreal(model.positions(p))
        names = [b for b in capture.BONES if b in r['places']]
        A = np.array([Q[model.rig.index(b)] for b in names]); T = np.array([r['places'][b] for b in names])
        e = np.linalg.norm(aligned(A, T) - T, axis=1)
        lens = Lens(r['width'], r['height']); fov = 2 * np.degrees(np.arctan((r['width'] / 2.0) / (lens.focal * factor)))
        E.append(e); fovs.append((fov, r['camera']['fov']))
        up = ''
        if pitch is not None:   # the fitted floor's up in the body's frame, turned as the pose turns onto the truth, against the truth's +Z
            u = kabsch(A, T) @ (rotation(turn).T @ np.array([-np.sin(pitch), 0.0, np.cos(pitch)]))
            ups.append(np.degrees(np.arccos(np.clip(u[2], -1, 1)))); up = '  floor up %.1f deg off (%d paws)' % (ups[-1], len(down))
        print('%s  %.1f px  pose %.2f cm mean, %.2f worst  fov %.1f (rendered %.1f)  %s%s' % (r['image'], px, e.mean(), e.max(), fov,
              r['camera']['fov'], how, up), flush=True)
    if not E:
        print('\nno still fitted (%d not fitted): the reader found fewer than six joints in every picture' % unfitted); return
    E = np.concatenate(E)
    if unfitted: print('%d still(s) not fitted: the reader found fewer than six joints' % unfitted)
    print('\n%d stills (%d view%s each), camera unknown%s: the pose %.2f cm mean, %.2f at 95%%, %.2f worst; fov off by %.1f deg mean%s' % (len(plain), a.views, 's' if a.views > 1 else '',
          ', floor fitted' if a.down else '', E.mean(), np.percentile(E, 95), E.max(), np.mean([abs(f - g) for f, g in fovs]),
          '; floor up off by %.1f deg mean, %.1f worst' % (np.mean(ups), np.max(ups)) if ups else ''))


def test(a):
    rows = json.load(open(os.path.join(a.stills, 'labels.json')))
    read = json.load(open(a.peaks))
    peaks = dict(zip(read['images'], read['peaks']))
    model = model_of(a)
    if a.unknown: return test_unknown(a, model, rows, peaks)
    start = model.params_of(model.rig.rest_r, model.rig.rest_t)
    plain = [r for r in rows if r['look'] in ('lit', 'unlit') and r['image'] in peaks]
    moments = {}
    for r in plain: moments.setdefault((r['asset'], r['at']), []).append(r)
    whole, pose, px, swaps = [], [], [], 0
    for (asset, at), rs in sorted(moments.items())[::a.every]:
        groups = [rs[k:k + a.views] for k in range(0, len(rs) - a.views + 1, a.views)]
        for g in groups:
            shots = [(camera.Camera.from_render(r['camera'], r['width'], r['height']), characters.origin(), peaks[r['image']]) for r in g]
            p, mpx, how = fit(model, shots, start)
            e, ep = errors(model, p, g[0]['places'], characters.origin())
            whole.append(e); pose.append(ep); px.append(mpx); swaps += how == 'swapped'
            print('%-28s at %.3f  %s  %.1f px  joints %.2f cm mean, %.2f worst; pose %.2f / %.2f%s' % (asset.rsplit('/', 1)[1], at,
                  '+'.join(r['image'] for r in g), mpx, e.mean(), e.max(), ep.mean(), ep.max(), '  (legs swapped)' if how == 'swapped' else ''))
    E, EP = np.concatenate(whole), np.concatenate(pose)
    print('\n%d fits of %d view(s): joints %.2f cm mean, %.2f at 95%%, %.2f worst; the pose alone %.2f / %.2f / %.2f; %.1f px mean; %d fitted '
          'with the legs swapped' % (len(whole), a.views, E.mean(), np.percentile(E, 95), E.max(), EP.mean(), np.percentile(EP, 95), EP.max(),
          np.mean(px), swaps))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode'); ap.add_argument('rest', nargs='*'); ap.add_argument('--views', type=int, default=1)
    ap.add_argument('--every', type=int, default=1)   # every Nth moment of the set (a quicker look)
    ap.add_argument('--unknown', action='store_true')   # the camera withheld: solved with the pose (fit_free)
    ap.add_argument('--tol', type=float, default=1e-6)   # with --unknown: the whole fit's xtol/ftol
    ap.add_argument('--down')                           # with --unknown: a take_report.json whose stance says the paws down at each frame
    for k, v in DEFAULTS.items(): ap.add_argument('--' + k, default=v)
    a = ap.parse_args()
    if a.mode == 'test': a.stills, a.peaks = a.rest; test(a)
    else: raise SystemExit(__doc__ if __doc__ else 'modes: test')


if __name__ == '__main__':
    main()
