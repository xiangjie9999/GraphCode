# Bone rotations from 3D joint places, turning each joint only about the axes its IK rig leaves free (FullBodyIK AllBoneSettings:
# X/Y/Z Locked, Limited or Free), one least-squares solve per frame over every tracked joint, started from the frame before.
# Unreal's local X, Y, Z are the glTF export's x, z, y (measured 2026-10-18: a knee free about Unreal Y turns about glTF z).
# A joint with two free axes composes them in one order, a fact of the rig, measured once on another clip of the same rig
# (rig_orders). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E4).
import itertools, json, re, sys
import numpy as np
from scipy.optimize import least_squares
from scipy.spatial.transform import Rotation
import gltf_rig

UE_TO_GLTF_AXIS = {'X': 0, 'Y': 2, 'Z': 1}


def ik_rig_axes(path):
    """Bone -> the glTF local axes the IK rig leaves free (Limited or Free). Each bone's settings run from its '(X=' to its 'Bone=name'
    (PreferredAngles=(...) nests inside, so a plain parenthesis match cuts the entry short)."""
    text = open(path, encoding='utf-8').read()
    out = {}
    starts = [m.start() for m in re.finditer(r'(?<!PreferredAngles=)\(X=', text)]
    for m in re.finditer(r',Bone=(\w+)\)', text):   # the comma: 'Settings.RootBone=pelvis)' is not a bone's entry
        begin = max([s for s in starts if s < m.start()], default=None)
        if begin is None: continue
        body = text[begin:m.start()]
        body = re.sub(r'PreferredAngles=\([^)]*\)', '', body)
        out[m.group(1)] = [UE_TO_GLTF_AXIS[a] for a in 'XYZ' if re.search(r'(?<![A-Za-z])%s=(Limited|Free)' % a, body)]
    return out


def ik_rig_limits(path, ranges=False):
    """Bone -> {glTF axis: the larger of |Min| and |Max| in radians} for each Limited axis (Free axes are absent). Symmetric on purpose:
    the IK rig's signs differ from the fit's (a Y limit is stored about the negative axis, and the glTF basis is mirrored), and a bound
    this loose still forbids what the rig cannot do (a right shoulder twisted 134 degrees against its 20, 2026-10-18). `ranges`: the
    IK rig's own (Min, Max) instead, turned into the fit's angles: the glTF basis is mirrored, which turns an X or Z range round, and a
    Y range is stored about the negative axis, which turns it back. Checked on the cat, 2026-10-27, on all 19 axes whose way round was
    measured (17 by a probe clip turning each +3 degrees, the chest and spine by GraphCode's own clamp notes)."""
    text = open(path, encoding='utf-8').read()
    out = {}
    starts = [m.start() for m in re.finditer(r'(?<!PreferredAngles=)\(X=', text)]
    for m in re.finditer(r',Bone=(\w+)\)', text):
        begin = max([s for s in starts if s < m.start()], default=None)
        if begin is None: continue
        body = re.sub(r'PreferredAngles=\([^)]*\)', '', text[begin:m.start()])
        limits = {}
        for a in 'XYZ':
            if re.search(r'(?<![A-Za-z])%s=Limited' % a, body):
                lo = re.search(r'Min%s=(-?[0-9.]+)' % a, body); hi = re.search(r'Max%s=(-?[0-9.]+)' % a, body)
                lo, hi = float(lo.group(1)) if lo else 0.0, float(hi.group(1)) if hi else 0.0
                own = (lo, hi) if a == 'Y' else (-hi, -lo)
                limits[UE_TO_GLTF_AXIS[a]] = tuple(np.radians(own)) if ranges else np.radians(max(abs(lo), abs(hi)))
        out[m.group(1)] = limits
    return out


def compose(order, angles):
    R = np.eye(3)
    for axis, t in zip(order, angles):
        v = np.zeros(3); v[axis] = t
        R = R @ Rotation.from_rotvec(v).as_matrix()
    return R


def rig_orders(rig, axes, bones):
    """For each joint with 2 free axes, the order its turns compose in, read off a clip of the rig (its own export)."""
    orders = {}
    for b in bones:
        free = axes.get(b, [])
        if len(free) != 2: continue
        i = rig.index(b); R0 = gltf_rig.quat_to_matrix(rig.rest_r[i])
        best = None
        for order in itertools.permutations(free):
            worst = 0.0
            for f in range(0, len(rig.anim_r), 2):
                D = R0.T @ gltf_rig.quat_to_matrix(rig.anim_r[f, i])
                r = least_squares(lambda p: (compose(order, p) - D).ravel(), np.zeros(2))
                worst = max(worst, np.abs(r.fun).max())
            if best is None or worst < best[0]: best = (worst, list(order))
        orders[b] = best[1]
    return orders


class Model:
    def __init__(self, rig, axes, orders, limits=None):
        self.rig = rig
        self.joints = []          # (node index, order of free glTF axes)
        for i, name in enumerate(rig.names):
            free = axes.get(name, [])
            if name in ('#0', 'root') or 'twist' in name or not free: continue
            self.joints.append((i, orders.get(name, free)))
        self.pelvis = rig.index('pelvis')
        self.size = 3 + sum(len(o) for _, o in self.joints)
        self.rest_R = [gltf_rig.quat_to_matrix(q) for q in rig.rest_r]
        # Bounds: the pelvis's place is free; each limited axis within the rig's (symmetric) range.
        self.lower = np.full(self.size, -np.inf); self.upper = np.full(self.size, np.inf)
        k = 3
        for i, order in self.joints:
            for axis in order:
                half = (limits or {}).get(rig.names[i], {}).get(axis)     # a half-width, or (lo, hi) the right way round
                if isinstance(half, tuple): self.lower[k], self.upper[k] = half[0] - 1e-6, half[1] + 1e-6
                elif half is not None: self.lower[k], self.upper[k] = -half - 1e-6, half + 1e-6
                k += 1

    def params_of(self, rotations, translations):
        """The parameters of a known pose: each joint's turns about its free axes, solved from its local rotation (glTF quaternion
        per node), the pelvis's place from its translation. A pose known from the first frame we rendered carries the twists a
        joint's place cannot show (2026-10-26: started from places alone, a fit kept a forearm twisted 127 degrees the wrong way)."""
        p = np.zeros(self.size); p[:3] = translations[self.pelvis]
        k = 3
        for i, order in self.joints:
            D = self.rest_R[i].T @ gltf_rig.quat_to_matrix(rotations[i])
            lo, hi = self.lower[k:k + len(order)], self.upper[k:k + len(order)]
            r = least_squares(lambda a: (compose(order, a) - D).ravel(), np.clip(np.zeros(len(order)), lo, hi), bounds=(lo, hi))
            p[k:k + len(order)] = r.x; k += len(order)
        return p

    def locals(self, params):
        R = [r.copy() for r in self.rest_R]
        t = self.rig.rest_t.copy()
        t[self.pelvis] = params[:3]
        k = 3
        for i, order in self.joints:
            R[i] = self.rest_R[i] @ compose(order, params[k:k + len(order)]); k += len(order)
        return R, t

    def positions(self, params):
        return self.frames(params)[1]

    def frames(self, params):
        """Every node's world rotation (glTF) and place."""
        R, t = self.locals(params)
        rig = self.rig
        W = [None] * len(rig.names); P = np.zeros((len(rig.names), 3))
        for i in rig.order:
            p = rig.parent[i]
            if p < 0: W[i] = R[i]; P[i] = t[i]
            else: W[i] = W[p] @ R[i]; P[i] = P[p] + W[p] @ t[i]
        return W, P

    def jacobian(self, params):
        """Every node's place (glTF) and how it moves with each parameter, (nodes, 3, size): turning a joint about its k-th free axis
        swings every node below it about that axis through the joint (axis x arm); the pelvis's place carries it and all below. One
        pass instead of a forward pass per parameter (a still fitted by differences took ~7 minutes, 2026-10-26)."""
        R, t = self.locals(params)
        rig = self.rig; n = len(rig.names)
        if not hasattr(self, '_below'):
            below = np.eye(n, dtype=bool)  # below[i, j]: j is i or under it
            for i in rig.order:
                if rig.parent[i] >= 0: below[:, i] |= below[:, rig.parent[i]]
            self._below = below
        W = [None] * n; P = np.zeros((n, 3))
        for i in rig.order:
            p = rig.parent[i]
            if p < 0: W[i] = R[i]; P[i] = t[i]
            else: W[i] = W[p] @ R[i]; P[i] = P[p] + W[p] @ t[i]
        J = np.zeros((n, 3, self.size))
        p = rig.parent[self.pelvis]
        J[self._below[self.pelvis], :, 0:3] = W[p] if p >= 0 else np.eye(3)
        k = 3
        for i, order in self.joints:
            p = rig.parent[i]
            B = (W[p] if p >= 0 else np.eye(3)) @ self.rest_R[i]
            under = self._below[i].copy(); under[i] = False
            arms = P[under] - P[i]
            for axis, a in zip(order, params[k:k + len(order)]):
                J[under, :, k] = np.cross(B[:, axis], arms)
                v = np.zeros(3); v[axis] = a
                B = B @ Rotation.from_rotvec(v).as_matrix(); k += 1
        return P, J

    def carried(self, params, node, d):
        """A direction fixed to `node` (glTF, `d` as it lies at rest) as the pose turns it, and how it turns with each parameter, (3, size):
        a joint at or above the node turning about its k-th axis turns it about that axis (axis x direction), as jacobian() swings a place;
        the pelvis's place moves no direction."""
        if not hasattr(self, '_below'): self.jacobian(params)            # which nodes lie under which
        rig = self.rig; R, _ = self.locals(params)
        W = [None] * len(rig.names); W0 = [None] * len(rig.names)
        for i in rig.order:
            p = rig.parent[i]
            W[i] = R[i] if p < 0 else W[p] @ R[i]; W0[i] = self.rest_R[i] if p < 0 else W0[p] @ self.rest_R[i]
        v = W[node] @ W0[node].T @ d
        D = np.zeros((3, self.size))
        k = 3
        for i, order in self.joints:
            p = rig.parent[i]
            B = (W[p] if p >= 0 else np.eye(3)) @ self.rest_R[i]
            for axis, a in zip(order, params[k:k + len(order)]):
                if self._below[i, node]: D[:, k] = np.cross(B[:, axis], v)
                r = np.zeros(3); r[axis] = a
                B = B @ Rotation.from_rotvec(r).as_matrix(); k += 1
        return v, D

    def solve(self, seen, start, previous=None, smooth=0.2):
        """`smooth`: cm of joint error a radian of change from the frame before costs, so a turn the joints cannot see (a twist two
        straight bones share) stays where it was instead of wandering; 0.2 lets a seen 30-degree step through for ~0.1 cm."""
        idx = [self.rig.index(b) for b in seen]
        target = np.array([seen[b] for b in seen])
        def resid(p):
            prior = smooth * (p[3:] - previous[3:]) if previous is not None else 1e-3 * p[3:]
            return np.concatenate([((self.positions(p)[idx] - target) * 100.0).ravel(), prior])
        x0 = np.clip(start, self.lower, self.upper)
        r = least_squares(resid, x0, bounds=(self.lower, self.upper), method='trf', xtol=1e-12, ftol=1e-12)
        err = np.linalg.norm((self.positions(r.x)[idx] - target) * 100.0, axis=1)
        return r.x, err


def to_gltf(u):
    u = np.asarray(u, float)
    return np.array([u[0], u[2], u[1]]) / 100.0


def run(points, rig, axes, orders, limits=None):
    model = Model(rig, axes, orders, limits)
    start = np.zeros(model.size); start[:3] = rig.rest_t[model.pelvis]
    rotations, translations, residuals = [], [], []
    previous = None
    for f in range(points['frames']):
        seen = {b: to_gltf(points['points'][b][f]) for b in points['bones'] if points['points'][b][f] is not None}
        p, err = model.solve(seen, start, previous)
        previous = p
        R, t = model.locals(p)
        rotations.append([gltf_rig.matrix_to_quat(r) for r in R]); translations.append(t); residuals.append(err.tolist())
        start = p
    return np.array(rotations), np.array(translations), residuals


if __name__ == '__main__':
    points_path, rig_path, ikrig_path, calib_path, out_path = sys.argv[1:6]
    truth_path = sys.argv[6] if len(sys.argv) > 6 else None
    rig = gltf_rig.Rig(rig_path)
    axes = ik_rig_axes(ikrig_path)
    orders = rig_orders(gltf_rig.Rig(calib_path), axes, [n for n in rig.names])
    points = json.load(open(points_path))
    R, T, res = run(points, rig, axes, orders, ik_rig_limits(ikrig_path))
    json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': T.tolist(), 'residual_cm': res, 'orders': orders}, open(out_path, 'w'))
    allres = [x for row in res for x in row]
    print('joint fit residual: mean %.3f cm, max %.3f cm' % (np.mean(allres), max(allres)))
    if truth_path:
        truth = gltf_rig.Rig(truth_path)
        rows = []
        for i, name in enumerate(rig.names):
            if name in ('#0', 'root') or 'twist' in name: continue
            e = [np.degrees(2 * np.arccos(min(1.0, abs(float(np.dot(R[f, i], truth.anim_r[f, i])))))) for f in range(len(R))]
            rows.append((max(e), name, np.mean(e)))
        for worst, name, mean in sorted(rows, reverse=True)[:12]:
            print('%-17s local turn error mean %6.2f deg, max %6.2f deg' % (name, mean, worst))
        print('all bones: median of max %.3f deg; bones over 1 deg: %d of %d' % (np.median([r[0] for r in rows]), sum(r[0] > 1 for r in rows), len(rows)))


MIRROR = np.array([[1.0, 0, 0], [0, 0, 1.0], [0, 1.0, 0]])            # glTF <-> Unreal axes (a bone's Unreal Z is its glTF y)


def roll_mark(cam, origin, W, P, bone, child):
    """How a drawn bone's pyramid shows its roll in a picture (GraphCodeRenderHighlight.cpp Pyramid, GatherBones): its base is square
    round the joint, half as wide as 6 % of the bone (0.4-1.5 cm), turned so one side faces the bone's Z (its Y where it points along Z);
    that side's edges are pale, the other's dark. Returns the pale side's offset across the drawn bone, in pixels at the base, signed
    along (-u_y, u_x) for u the drawn bone (joint -> child) as track_all.roll_at reads it, and how far its two pale edges stand apart
    across it, halved (unsigned): (q, s), or None for a bone of no length."""
    import gltf_rig as _g
    a3 = _g.to_unreal(P[bone]) + origin; c3 = _g.to_unreal(P[child]) + origin
    L = np.linalg.norm(c3 - a3)
    if L < 1e-6: return None
    along = (c3 - a3) / L
    z, y = MIRROR @ W[bone][:, 1], MIRROR @ W[bone][:, 2]
    top = y if abs(np.dot(along, z)) > 0.9 else z
    top = top - along * np.dot(top, along); top /= np.linalg.norm(top)
    h = np.clip(L * 0.06, 0.4, 1.5)
    a2, c2 = cam.project(np.stack([a3, c3]))
    u = c2 - a2; Lp = np.linalg.norm(u)
    if Lp < 1e-6: return None
    n = np.array([-u[1], u[0]]) / Lp
    across = np.cross(top, along)
    pa, pb, pc, pd = cam.project(np.stack([a3 + h * top, a3 - h * top, a3 + h * across, a3 - h * across]))
    return float(np.dot(pa - pb, n) / 2), float(abs(np.dot(pc - pd, n)) / 2)


def solve_views(model, views, start, previous=None, smooth=0.05, loss='linear', scale=1.0, roll_weight=1.0):
    """Fit the rig straight to 2D joint balls seen by one or more cameras (no triangulation): each view is (camera, origin, {bone: pixel}
    [, weight]).
    Bone lengths and the rig's free axes carry the depth a single view lacks; `smooth` ties the turns to the frame before (radians,
    per pixel of reprojection) so a joint the picture cannot pin does not wander. `loss`/`scale` (pixels) as least_squares takes them:
    'soft_l1' at a few pixels lets a ball found far from its joint (another bone's ring, a glitch) pull far less. Returns the parameters
    and each view's pixel residuals."""
    import gltf_rig as _g
    names = [list(v[2].keys()) for v in views]
    idx = [[model.rig.index(b) for b in n] for n in names]
    targets = [np.array([v[2][b] for b in n]) for v, n in zip(views, names)]
    # A view's optional 4th part: how much it counts, one number or {bone: number}.
    weights = [np.array([(v[3].get(b, 1.0) if isinstance(v[3], dict) else v[3]) if len(v) > 3 else 1.0 for b in n])[:, None]
               for v, n in zip(views, names)]
    # A view's optional 5th part: the drawn bones' rolls it shows, {bone: (child, q, s)} (roll_mark; q or s None where not read): the
    # twist a joint's place cannot show (a forearm's, 17-19 degrees off from places alone, E10).
    rolls = [[(model.rig.index(b), model.rig.index(r[0]), r[1], r[2] if len(r) > 2 else None) for b, r in v[4].items()]
             if len(v) > 4 and v[4] else [] for v in views]
    def resid(p):
        if any(rolls): W, G = model.frames(p)
        else: G = model.positions(p)
        P = _g.to_unreal(G)
        out = [(w * (v[0].project(P[i] + np.asarray(v[1])) - t)).ravel() for v, i, t, w in zip(views, idx, targets, weights)]
        for v, rs in zip(views, rolls):
            for b, c, q, sp in rs:
                m = roll_mark(v[0], np.asarray(v[1]), W, G, b, c)
                if m is None: continue
                if q is not None: out.append(np.array([roll_weight * (m[0] - q)]))
                if sp is not None: out.append(np.array([roll_weight * (m[1] - sp)]))
        out.append(smooth * 100.0 * (p - previous) if previous is not None else 1e-2 * p[3:])    # first frame: the least turn breaks a tie
        return np.concatenate(out)
    # Within the IK rig's limits, as fit_rig.solve: unbounded, a forearm's twist the picture cannot see wandered to 43 degrees mean
    # (2026-10-18, capture2 from both views, shared between shoulder and elbow).
    x0 = np.clip(start, model.lower, model.upper)
    r = least_squares(resid, x0, bounds=(model.lower, model.upper), xtol=1e-10, ftol=1e-10, max_nfev=400, loss=loss, f_scale=scale)
    P = _g.to_unreal(model.positions(r.x))
    errs = [np.linalg.norm(v[0].project(P[i] + np.asarray(v[1])) - t, axis=1) for v, i, t in zip(views, idx, targets)]
    return r.x, errs
