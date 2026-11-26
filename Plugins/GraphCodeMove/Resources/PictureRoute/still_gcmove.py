# Key poses from still pictures into a .gcmove (MOVE-CAP1 E20, Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md): a folder of pictures of the character,
# each one key pose (an image model's, a drawing's, a render's), and a stills.json the author writes: their order, the paws down in each,
# and how they are timed -> the joint reader's peaks (dino_reader.py stills, in its own torch environment) -> the rig fitted to each picture
# alone, its camera unknown (fit_still.fit_free) -> each pose stood on its floor, facing the rig's forward, in place -> one clip on the
# character's skeleton, a frame a still -> a .gcmove keying each frame (`pose(source=<clip>, at=frame(n), from=reference)`) with its paws
# planted, timed by a travel at the author's speed (a walk, a run: take_gcmove.py's shape) or at the author's seconds (anything else:
# POSES.md's per-still windows). The floor comes with each pose (fit_still.fit_free): a picture's horizon is level (no roll), the body's
# root stands upright on the floor and leans by its pelvis, and the paws the author says are down are held at one height.
# stills.json, a travel:  {"stills": [{"image": "k1.png", "down": ["toe_F_R", "toe_F_L", "toe_B_L"]}, ...], "loop": true, "velocity": 0.4}
#   ("image" may list several pictures of the same pose from different sides: ["k1_side.png", "k1_front.png"]; they are fitted as one
#   pose, each with its own camera, which settles the depth one picture leaves open)
#              seconds:   {"stills": [{"image": "k1.png", "down": [...], "at": 0.0}, ...], "seconds": 1.2}   (the clip's length; default the
#                         last still's moment)
# "collision": a seconds-timed move keeps the body clear of itself between keys (collision(self), on unless false); a travel, if true.
# "turns" (several pictures of one still): SAY HOW FAR ROUND THE BODY EACH PICTURE IS TAKEN FROM THE FIRST, AND WHICH SIDE: "turns": [0, "+45"]
#   (a string with its sign: positive = the camera went counterclockwise round the body seen from above); a plain number, [0, 45], is the
#   size only. The pair fit starts and stays near it (fit_still.fit_views_free turns=). On our man's 17 held-out pairs: 6.63 cm mean with
#   no turns, 4.68 with the size, 2.88 with the side (lead 25 Nov); S1's left arm 190-256 px -> 5-26 px with the size alone.
# "hands" (26 Nov, lead 02:59Z): A PICTURE DOES NOT TELL A CLOSED HAND (the reader reads two knuckles a hand, not the fingers' closure, so
#   the fit leaves every finger at rest): SAY IT. "hands": "fist" closes both, "fist_left" / "fist_right" one; the key is then written
#   with the hand's body word over it ($Hand(curl=1), HumanBody's: a closed hand is a fist with the thumb over the index and middle) and
#   the presets line that word needs. Absent, the hands are as the fit left them (today's file).
# "down" lists the paws planted in that still, about to lift first and the one staying down longest last (the travel carries the body by
# the paw written last; take_gcmove.py). The paw names and their legs are the cat's (take_gcmove.LEGS).
# A move that keeps the body clear of itself has each key that lies inside the body pushed out first: the key held one frame, its down
# paws planted, collision(self) (`<out>Key<n>.gcmove` beside the move, building A_<out>Key<n>), and the move keys that. A picture fit lays a resting
# limb up to 2.8 cm into the body (two coats each side of a contact); fitted into each frame of the move, the collision fit dragged the
# sit's folded hind leg off its key and the hock snapped 67.5 degrees in a frame; pushed out first, 18.4 (2026-10-29).
# Usage: python still_gcmove.py <stills_dir> <clip /Game/.../A_Name> <out.gcmove> [--name A_...] [--no-import] [--raw-keys]
#   (--no-import keys a clip imported before; --raw-keys keys it as it was fitted, unpushed)
import argparse, json, os, subprocess, sys
import numpy as np
import capture, characters, fit_still, gltf_rig, take_gcmove, twist_helpers, write_gltf

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULTS = dict(fit_still.DEFAULTS, model=characters.ACTIVE.reader, python=characters.route_paths.python())
FPS = 30.0


def basis(up, forward):
    z = up / np.linalg.norm(up); y = forward - (forward @ z) * z; y /= np.linalg.norm(y)
    return np.array([np.cross(y, z), y, z])


def stand(model, p, toes, soles=False):
    """The fitted pose stood on the floor (its up is the body's own: fit_free stands the root upright): the body's forward (pelvis to
    chest, on the floor) the rest pose's, the pelvis over the rest pelvis, the down paws at the rest paws' height (none down: the pelvis
    at its rest height), or with `soles` (a stance, as fit_still fits one) each down paw's lowest contact on the floor. Returns each
    node's local rotation (matrices) and translation, glTF, and the body's places in Unreal axes."""
    rig = model.rig
    O = gltf_rig.to_unreal(np.eye(3)).T / 100.0                          # Unreal cm = 100 O (glTF metres)
    W, P = model.frames(p); B = 100.0 * P @ O.T
    rest = model.params_of(rig.rest_r, rig.rest_t)
    B0 = 100.0 * model.positions(rest) @ O.T
    pel = rig.index('pelvis')
    # The body's facing from the line across its hips and shoulders, which stays level whatever the spine does: the pelvis-to-chest
    # line pointed steeply down in Codex's play bow, its run along the floor was short, and every paw stood 10-20 cm to one side.
    up = np.array([0.0, 0.0, 1.0])
    across = lambda X: characters.ACTIVE.across(X, rig.index)          # right minus left over the mid-plane pairs (characters.py `mids`)
    Rg = basis(up, np.cross(up, across(B0))).T @ basis(up, np.cross(up, across(B)))
    B1 = B @ Rg.T
    ti = [rig.index(t) for t in toes]
    lift = (B0[ti, 2].mean() - B1[ti, 2].mean()) if ti else B0[pel, 2] - B1[pel, 2]
    if soles and ti and all(t in model.pads for t in toes):
        M = gltf_rig.to_unreal(np.eye(3)).T                              # glTF metres -> Unreal cm
        lows = [min(B[b, 2] + (M @ model.carried(p, b, np.linalg.solve(M, at - B0[b]))[0])[2] for b, at in model.pads[t]) for t in toes]
        lift = -float(np.mean(lows))
    shift = np.array([B0[pel, 0] - B1[pel, 0], B0[pel, 1] - B1[pel, 1], lift])
    Rg_g = O.T @ Rg @ O; t_g = O.T @ shift / 100.0
    R, t = model.locals(p)
    q = rig.parent[pel]
    R[pel] = W[q].T @ (Rg_g @ W[pel]); t[pel] = W[q].T @ (Rg_g @ P[pel] + t_g - P[q])
    return R, t, B1 + shift, Rg, shift


def camera_of(model, turn, place, factor, size, Rg, shift):
    """The picture's own camera where the stood pose is, as gc_render takes one (preview space: the component plus the preview's
    placement, track_all.ORIGIN): render the built clip from it at that still's moment and it lies over the picture."""
    centre = gltf_rig.to_unreal(model.positions(model.params_of(model.rig.rest_r, model.rig.rest_t))).mean(0)
    Rt = fit_still.rotation(turn)
    eye = (centre - place @ Rt) @ Rg.T + shift + np.asarray(characters.origin())
    ahead = Rt[0] @ Rg.T
    lens = fit_still.Lens(size[0], size[1])
    fov = 2 * np.degrees(np.arctan((size[0] / 2.0) / (lens.focal * factor)))
    return {'location': [round(float(v), 2) for v in eye], 'look_at': [round(float(v), 2) for v in eye + 100.0 * ahead], 'fov': round(float(fov), 2)}


def fits(a, spec):
    """Each still's reader peaks (read once, kept in the folder as peaks.json) and the rig fitted to it, its camera unknown."""
    peaks_file = os.path.join(a.stills, 'peaks.json')
    if not os.path.exists(peaks_file):
        subprocess.check_call([a.python, os.path.join(HERE, 'dino_reader.py'), 'stills', a.model, a.stills, peaks_file])
    read = json.load(open(peaks_file))
    if read.get('bones') and len(read['bones']) != len(capture.BONES):
        raise SystemExit('%s was read for %d joints; the %s profile has %d (--character?)' % (peaks_file, len(read['bones']), characters.NAME, len(capture.BONES)))
    by = {n: (pk, size) for n, pk, size in zip(read['images'], read['peaks'], read['sizes'])}
    model = fit_still.model_of(a)
    stills = spec['stills']
    def fit(s):
        missing = [n for n in pictures(s) if n not in by]
        if missing: raise SystemExit('%s was not read: is it in %s?' % (missing, a.stills))
        # A stance's down paws lie pads to the floor; a travel's tip as they lift (fit_still.fit_views_free, `stand`).
        got = fit_still.fit_views_free(model, [by[n] for n in pictures(s)], down=s.get('down', []), turns=s.get('turns'),
                                       stand=0.0 if 'velocity' in spec else fit_still.STAND)
        if got is None: raise SystemExit('%s: the reader found fewer than six joints; is the whole character in the picture?' % label(s))
        print('%-24s fitted: %s' % (label(s), '; '.join('%.1f px, focal x%.2f, %s' % (c[3], c[2], c[4]) for c in got[1])), flush=True)
        return got
    return model, [fit(s) for s in stills]


def pictures(s):
    """A still's pictures: one, or several of the same pose from different sides (stills.json "image": a name or a list)."""
    return list(s['image']) if isinstance(s['image'], list) else [s['image']]


def label(s):
    return ' + '.join(pictures(s))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('stills'); ap.add_argument('clip'); ap.add_argument('out'); ap.add_argument('--name')
    ap.add_argument('--no-import', action='store_true')              # write the glTF and the .gcmove, import nothing
    ap.add_argument('--raw-keys', action='store_true')               # key the fitted poses as they are, not pushed out of the body
    ap.add_argument('--fit-only', action='store_true')               # stop after the fits: stills.gltf and stills_report.json (no import, no .gcmove)
    for k, v in DEFAULTS.items(): ap.add_argument('--' + k, default=v)
    a = ap.parse_args()
    if not a.clip.startswith('/Game/'):   # Git Bash rewrites a leading /Game/ into its own install folder: run from PowerShell or cmd
        raise SystemExit('the clip must be a /Game/... path, got %s' % a.clip)
    spec = json.load(open(os.path.join(a.stills, 'stills.json')))
    stills = spec['stills']
    timed = 'velocity' in spec
    if not timed and any('at' not in s for s in stills):
        raise SystemExit('stills.json says neither a "velocity" (a travel) nor every still\'s "at" (seconds)')
    for s in stills:
        bad = [p for p in s.get('down', []) if p not in take_gcmove.LEGS]
        if bad: raise SystemExit('%s: %s are not paws (%s)' % (label(s), bad, ', '.join(take_gcmove.LEGS)))
        hand_lines(s)                                                  # an unknown "hands" refused before the fit, not after it
    if timed and any(not s.get('down') for s in stills):
        raise SystemExit('a travel carries the body by a planted paw: every still needs one down (or time them with "at")')
    model, got = fits(a, spec)
    read = json.load(open(os.path.join(a.stills, 'peaks.json'))); sizes = dict(zip(read['images'], read['sizes']))
    rig = model.rig
    R, T, report, prints = [], [], [], []
    for s, (p, cams) in zip(stills, got):
        turn, place, factor, px, how, pitch = cams[0]
        Rl, t, B, Rg, shift = stand(model, p, s.get('down', []), soles=not timed)
        R.append([gltf_rig.matrix_to_quat(r) for r in Rl]); T.append(t)
        prints.append({paw: B[rig.index(paw), :2] for paw in s.get('down', [])})
        report.append({'image': label(s), 'pelvis_cm': [round(float(v), 2) for v in B[rig.index('pelvis')]], 'pictures': [
            {'image': name, 'reprojection_px': round(c[3], 2), 'focal_factor': round(c[2], 3), 'start': c[4],
             'lens_pitch_down_deg': round(float(np.degrees(c[5])), 1), 'size': sizes[name],
             'camera': camera_of(model, c[0], c[1], c[2], sizes[name], Rg, shift)} for name, c in zip(pictures(s), cams)]})
        print('%-24s stood: the lens %s deg down, pelvis %.1f cm up' % (label(s), ' / '.join('%.1f' % q['lens_pitch_down_deg'] for q in
              report[-1]['pictures']), B[rig.index('pelvis'), 2]))
    R = twist_helpers.apply(rig, R, twist_helpers.calibrate(gltf_rig.Rig(a.calib)))
    gltf = os.path.join(a.stills, 'stills.gltf')
    write_gltf.write(rig, R, np.array(T), np.repeat(rig.rest_t[rig.index('root')][None], len(R), 0), gltf, True, FPS)
    json.dump(report, open(os.path.join(a.stills, 'stills_report.json'), 'w'), indent=1)
    if a.fit_only: return
    if not a.no_import:
        import run_take
        run_take.import_clip(os.path.abspath(gltf), a.clip, characters.ACTIVE.mesh, FPS)
        print('imported', a.clip)
    keys = [(a.clip, k) for k in range(len(stills))]
    if collides(spec) and not a.raw_keys: keys = clean_keys(spec, a.clip, a.out)
    moments = None
    if not timed and len(stills) > 2:                                   # a seconds-timed move: when its joined travel reaches each still
        total = float(spec.get('seconds', stills[-1]['at']))
        moments = joined_moments(spec, keys + ([keys[0]] if spec.get('loop', False) else []), a.out, int(round(total * FPS)))
    elif not timed and len(stills) == 2 and not spec.get('loop', False):
        # Two stills: the travel fills the clip, so it reaches the second at its end, not at the still's own `at` (the sit's two ends at
        # 0 and 1.0 of 1.3 s planted its paws again at 0.77 of the clip, the legs still moving, 2026-10-29).
        moments = [0.0, float(spec.get('seconds', stills[-1]['at']))]
    write_move(spec, a.clip, a.out, a.name, prints, keys=keys, moments=moments)


def collides(spec):
    """Whether the move keeps the body clear of itself (write_move's collision(self)): a seconds-timed move unless "collision": false,
    a travel only if true."""
    return spec.get('collision', 'velocity' not in spec)


def content_path(path):
    """A file under the project's Content folder as its /Game/... path, its extension dropped."""
    rel = os.path.relpath(os.path.abspath(path), os.path.abspath(os.path.join(characters.ROOT, 'Content')))
    if rel.startswith('..'): raise SystemExit("%s is not under the project's Content folder: its keys are cleaned by building .gcmoves there" % path)
    return '/Game/' + os.path.splitext(rel)[0].replace('\\', '/')


def clean_keys(spec, clip, out):
    """Each still's key pushed out of the body (the usage note above): one .gcmove beside `out` a still, built at once. Returns each
    key's (clip, frame)."""
    import run_take
    stem = os.path.splitext(os.path.basename(out))[0]
    into = clip.rsplit('/', 1)[0]
    keys = []
    for k, s in enumerate(spec['stills']):
        name = '%sKey%d' % (stem, k)
        path = os.path.join(os.path.dirname(os.path.abspath(out)), name + '.gcmove')
        # Built first without collision(self), the final check says whether the planted key is inside the body at all; only one that is
        # is pushed out (the user, 2026-10-27: "use the pose first, then push out by contact"). Pushed out though clear, the bow's keys
        # changed and its right elbow turned 35 degrees in one frame between them (2026-10-29).
        for push in (False, True):
            lines = [take_gcmove.HEADER + '         into:%s fps:%d' % (into, FPS), 'build {', '  A_%s = $Move' % name, '}',
                     "# Written by Tools/VideoCapture/still_gcmove.py: %s's key (%s, frame %d) held one frame, its down paws planted%s" % (
                         label(s), clip, k, ', pushed' if push else ''),
                     ('# out of the body by collision(self) before %s.gcmove blends it with the others.' % stem) if push else
                     '# (the check that it is clear of the body; %s.gcmove keys the fitted pose itself when it is).' % stem]
            if s.get('down'): lines.append('Floor = ground(z=0)')
            lines += ['K = pose(source=%s, at=frame(%d), from=reference) { }' % (clip, k), 'Move = clip(frames=1) {', '  $K']
            lines += ['  plant(%s, on=$Floor, roll=(%s))' % take_gcmove.LEGS[p] for p in s.get('down', [])]
            lines += (['  collision(self)'] if push else []) + ['}', '']
            open(path, 'w', newline='\n').write('\n'.join(lines))
            r = run_take.tool('gc_apply', {'asset_path': content_path(path), 'create': True})
            if r.get('success') or push or 'self-collision in' not in json.dumps(r): break
        if not r.get('success'): raise SystemExit("cleaning %s's key failed: %s" % (label(s), json.dumps(r)[:800]))
        if push:
            print('%-24s key pushed out of the body: %s/A_%s' % (label(s), into, name), flush=True)
            keys.append(('%s/A_%s' % (into, name), 0))
        else:
            print('%-24s key clear of the body: keyed as fitted' % label(s), flush=True)
            keys.append((clip, k))
    return keys


HAND_PRESETS = '/GraphCodeMove/Presets/Human/HumanBody'     # the hand's body word ($Hand) the "hands" statement writes
HANDS = {'fist': ['  $Hand(curl=1)'], 'fist_left': ['  $Hand(curl=1, right=false)'], 'fist_right': ['  $Hand(curl=1, left=false)']}


def hand_lines(s):
    """The key's hand lines from the author's "hands" statement in stills.json (none when absent); an unknown value is refused by name."""
    h = s.get('hands')
    if not h: return []
    if h not in HANDS: raise SystemExit('%s: "hands": %r is not one of %s' % (label(s), h, ', '.join(sorted(HANDS))))
    return list(HANDS[h])


def profile(points):
    """profile(...) through (fraction, value) points, 0 and 1 included, one value a fraction (the later wins)."""
    keyed = {}
    for f, v in points: keyed[round(min(max(f, 0.0), 1.0), 4)] = v
    keyed.setdefault(0.0, points[0][1]); keyed.setdefault(1.0, points[-1][1])
    return 'profile(%s)' % ', '.join('%.4g=%.4g' % (f, keyed[f]) for f in sorted(keyed))


def joined_moments(spec, keys, out, frames):
    """When the joined travel (write_move's `$Steps [pace=$Pace]`) reaches each still, in seconds: a timing build of the keys alone
    (`<out>Timing.gcmove`, no plants, no collision), read off its note ("pose 2 at source 0.544, reached at 0.231 s"). GraphCode places
    the in-between poses by their share of the turn, so the plants are timed to where it puts them, not to the stills' own moments."""
    import re, run_take
    stem = os.path.splitext(os.path.basename(out))[0]
    into = keys[0][0].rsplit('/', 1)[0]
    name = stem + 'Timing'
    lines = [take_gcmove.HEADER + '         into:%s fps:%d' % (into, FPS), 'build {', '  A_%s = $Move' % name, '}',
             "# Written by Tools/VideoCapture/still_gcmove.py: %s.gcmove's keys joined alone, to read when the travel reaches each one." % stem]
    lines += ['K%d = pose(source=%s, at=frame(%d), from=reference) { }' % ((k,) + key) for k, key in enumerate(keys, 1)]
    lines += ['Steps = pose(steps) { %s }' % ', '.join('$K%d' % k for k in range(1, len(keys) + 1)), 'Pace = profile(0=1, 1=1)',
              'Move = clip(frames=%d) {' % frames, '  $Steps [pace=$Pace]', '}', '']
    path = os.path.join(os.path.dirname(os.path.abspath(out)), name + '.gcmove')
    open(path, 'w', newline='\n').write('\n'.join(lines))
    r = run_take.tool('gc_apply', {'asset_path': content_path(path), 'create': True})
    if not r.get('success'): raise SystemExit('the timing build of %s failed: %s' % (stem, json.dumps(r)[:800]))
    seconds = frames / FPS
    got = {int(k): float(t) for k, t in re.findall(r'pose (\d+) at source [\d.]+, reached at ([\d.]+) s', json.dumps(r))}
    if len(keys) > 2 and len(got) != len(keys) - 2: raise SystemExit('the timing build of %s says where %d of its %d in-between poses are' % (stem, len(got), len(keys) - 2))
    moments = [0.0] + [got[k] for k in range(2, len(keys))] + [seconds]
    print('the joined travel reaches the stills at %s s' % ', '.join('%.3f' % m for m in moments), flush=True)
    return moments


def write_move(spec, clip, out, name=None, prints=None, moves=3.0, keys=None, moments=None):
    stills = spec['stills']; loop = spec.get('loop', False)
    keys = keys or [(clip, k) for k in range(len(stills))]                # each still's (clip, frame): clean_keys's, or the fit's own
    name = name or 'A_' + os.path.splitext(os.path.basename(out))[0]
    travel = 'velocity' in spec
    hands = any(s.get('hands') for s in stills)
    lines = [take_gcmove.HEADER + ('         presets:(%s)\n' % HAND_PRESETS if hands else '') + '         into:%s fps:%d' % (clip.rsplit('/', 1)[0], FPS), 'build {',
             '  %s = $Move [%sreadings%s]' % (name, 'loop, ' if loop else '', ', root_motion' if travel else ''), '}',
             '# Written by Tools/VideoCapture/still_gcmove.py (MOVE-CAP1 E20): %d key poses read off still pictures, a frame of %s each, the' % (
                 len(stills), clip),
             '# paws down in each planted%s. Edit freely: it is an ordinary .gcmove.' % (
                 ' (about to lift first, the one staying down longest last)' if travel else '')]
    if any(s.get('down') for s in stills): lines.append('Floor = ground(z=0)')
    for k, s in enumerate(stills, 1):
        down = s.get('down', []) if travel else []                      # timed by seconds, the clip plants them (below)
        inner = hand_lines(s) + ['  plant(%s, on=$Floor, roll=(%s))' % take_gcmove.LEGS[p] for p in down]
        lines.append('K%d = pose(source=%s, at=frame(%d), from=reference) {%s   # %s' % ((k,) + keys[k - 1] + ('' if inner else ' }', label(s))))
        if inner: lines += inner + ['}']
    if travel:
        steps = ', '.join('$K%d' % k for k in range(1, len(stills) + 1)) + (', $K1' if loop else '')
        lines += ['Steps = pose(steps) { %s }' % steps, 'Move = clip() {', '  $Steps [clock=root]',
                  '  travel(root, velocity=%.3fm/s)' % spec['velocity']] + (['  collision(self)'] if spec.get('collision', False) else []) + ['}', '']
    else:
        # Each still after the first takes over from the one before over the time between their moments (POSES.md, "three stills or
        # more"): from=reference, so at full weight it replaces the one before on every bone. A loop comes back to the first still as a
        # last key of its own. A paw down in every still is planted throughout; one down in some, weighted by the stills it is down in.
        # A paw whose print moves more than `moves` cm between two stills (Codex's sit: the hind paws come forward 6-10 cm) lets go of
        # the floor halfway between them and takes hold again AT the next: held on one print, the legs folded past their stops (a hip
        # asked 106 degrees of side) and a thigh went into the spine (refused, 2026-10-26). A plant takes its print where the paw is when
        # its weight starts to rise (GraphCodeMoveProgramSolve, RollAcquire on the rising edge), so it rises over the frame before the
        # still, not from halfway: ramped from halfway, the print was the half-blended pose's paw, the next still's own paw lay 4 cm from
        # it, and the plant pulled each hock 32-49 degrees past its stop into the haunch (refused, 2026-10-27).
        at = [float(s['at']) for s in stills]
        total = float(spec.get('seconds', at[-1]))
        order = list(range(1, len(stills) + 1))
        downs = [set(s.get('down', [])) for s in stills]
        prints = list(prints) if prints else [{} for _ in stills]
        if loop:
            k = len(stills) + 1
            lines.append('K%d = pose(source=%s, at=frame(%d), from=reference) { }   # %s again: the loop closes' % ((k,) + keys[0] + (label(stills[0]),)))
            order.append(k); at.append(total); downs.append(downs[0]); prints.append(prints[0])
        # frames= is the clip's length in frames (ANIMATION.md: "frames=48 at fps:24 is two seconds"), so each still's moment lands on a sample. Written one more
        # (2026-10-27), every key fell between two samples and no sample held a paw fully at a key (the bow's forepaws peaked at 0.995).
        frames = int(round(total * FPS))
        # ONE TRAVEL THROUGH THE STILLS (user 2026-10-29: "many actions supposed to be a continuous one, but it's like it split into two
        # parts"): a pose(steps) of them, paced to fill the clip, is joined per bone through every still (POSES.md, "Through three or more
        # poses"), from rest to rest; a joint that keeps its direction passes an in-between still at speed. ~~Each still after the first
        # its own reference, flat at 0 until its window opens and at 1 after~~: every window eased in and out, so the sit all but
        # stopped at its middle picture (pelvis 1 cm/s at 0.5 s, 32 at its fastest) and halted at the seat in one frame. GraphCode places
        # the in-between stills by their share of the turn (`moments`, joined_moments), and the plants are timed to them.
        if moments: at = list(moments)
        if len(order) == 1:
            # ONE STILL (25 Nov 2026, the jab session's S1 test): a pose(steps) of one pose has nothing to time and the build refuses it
            # ("no joint the pose names turns between its first and last source frame ... nothing to time"); one still is a HELD pose.
            lines.append('Move = clip(frames=1) {')
            lines.append('  $K%d' % order[0])
        else:
            lines.append('Steps = pose(steps) { %s }' % ', '.join('$K%d' % k for k in order))
            lines.append('Pace = profile(0=1, 1=1)')
            lines.append('Move = clip(frames=%d) {' % frames)
            lines.append('  $Steps [pace=$Pace]')
        for paw in [p for p in take_gcmove.LEGS if any(p in d for d in downs)]:
            points = [(t / total, 1.0 if paw in d else 0.0) for t, d in zip(at, downs)]
            for i in range(len(at) - 1):
                a_, b_ = prints[i].get(paw), prints[i + 1].get(paw)
                if a_ is not None and b_ is not None and np.linalg.norm(np.asarray(b_) - np.asarray(a_)) > moves:
                    points += [((at[i] + at[i + 1]) / 2.0 / total, 0.0), ((at[i + 1] - 1.0 / FPS) / total, 0.0)]
            points.sort()
            # The plant keeps the drawn turn: the fit lays the paw's pads to the floor (fit_still, `stand`) and the pictures say how steep
            # it lies. ~~plant(leg, on=$Floor)~~ (tried 2026-10-27) stands a paw at its bind turn, a standing cat's steep paw: the bow's
            # forearms came down steep, paws bunched.
            if all(v == 1.0 for _, v in points): lines.append('  plant(%s, on=$Floor, roll=(%s))' % take_gcmove.LEGS[paw])
            else: lines.append('  plant(%s, on=$Floor, roll=(%s), weight=%s)' % (take_gcmove.LEGS[paw] + (profile(points),)))
        # The body kept clear of itself between the keys (each key is clear; the blend between two can pass a hip through the spine or
        # an elbow into the chest: Codex's sit and stretch, refused by the final check, 2026-10-26). "collision": false leaves it out.
        # ~~and off the floor, collision(self, $Floor)~~ (2026-10-27): a paw resting on the floor is allowed no give there (its overlap at
        # rest, 0), so the sit was refused for paws and wrists 0.1-1.4 cm into it and the fit lifted a hind leg 7.7 cm off its print;
        # the plants keep the paws on the floor.
        if spec.get('collision', True): lines.append('  collision(self)')
        lines += ['}', '']
    open(out, 'w', newline='\n').write('\n'.join(lines))
    print('wrote %s: %d keys, %s' % (out, len(stills), 'travel(root, velocity=%.3fm/s)' % spec['velocity'] if travel
                                       else 'each at its moment over %.3f s' % float(spec.get('seconds', stills[-1]['at']))))


if __name__ == '__main__':
    main()
