# The character the still-picture route runs on, DERIVED from the character's own GraphCode files, so a user with any character runs it
# without editing a constant (Docs/Research/metahuman-picture-route.md; Plugins/GraphCode/Docs/Guides/MOTION-FROM-VIDEO.md).
#   character config   characters/<name>.json (shipped) or Saved/Research/VideoCapture/characters/<name>.json (picture_route.py init):
#                      the files that say who the character is: its .gcrig, its IK rig, its skeletal mesh, and for a .gcmove header its stage
#                      Blueprint / part / controls / physics asset (or read from an existing .gcmove of it).
#                      `sides_distinct`: true where the reader tells left from right (a person); false or absent: the cat's either-side rule.
#   derived from the .gcrig (parent files followed, `parent:`): the joints the reader reads, the planted legs (every chain with `contacts`)
#                      and their roll contacts, each leg's limb joints and mid-plane pair, the left/right suffix, the chest, the stands,
#                      whether a hidden leg copies its partner (four legs), head levelling (only where a skin file is given).
#   framing            cameras, centre, length, the preview's origin: from a render manifest of the character (`--origin-manifest`, or the
#                      one `picture_route.py references` saved under Saved/Research/VideoCapture/characters/<name>.frame.json).
#   OPTIONAL override  characters/<name>.override.json (or the Saved folder): any derived value the rig cannot give. Read only when present.
#                      The shipped cat's holds what its taught reader and its tuning fix (see characters/cat.override.json).
# Chosen once at start-up: `--character <name>` on any script's command line (taken out of sys.argv here), else GC_CHARACTER, else cat.
import json, os, re, sys
import route_paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = route_paths.PROJECT                 # the project's folder (route_paths.py: --project, or found from here)
SAVED = os.path.join(ROOT, 'Saved', 'Research', 'VideoCapture')
NOW = os.path.join(SAVED, 'now')
FOLDERS = [os.path.join(HERE, 'characters'), os.path.join(SAVED, 'characters')]
MODELS = route_paths.models()             # each character's taught reader (route_paths.py)
SKIP = ('twist', 'corrective', 'metacarpal')        # helper bones the fit never turns (fit_rig skips "twist"); the glTF's own root


def game_path(p):
    """A /Game/... path from a Content-relative file, absolute file or /Game path (extension dropped)."""
    p = p.replace('\\', '/')
    if p.startswith('/Game/'): return os.path.splitext(p)[0] if p.rsplit('/', 1)[-1].count('.') else p
    rel = os.path.relpath(os.path.abspath(p), os.path.join(ROOT, 'Content')).replace('\\', '/')
    return '/Game/' + os.path.splitext(rel)[0]


def content_file(p, ext):
    """The Content file of a /Game path (or the file itself when given as one)."""
    p = p.replace('\\', '/')
    if p.startswith('/Game/'): return os.path.join(ROOT, 'Content', p[len('/Game/'):] + ext)
    return p if os.path.isabs(p) else os.path.join(ROOT, p)


def gcrig_text(path, seen=None):
    """A .gcrig's text with its parents' before it (`gcrig 1 parent:/Game/...`: /Game/X -> Content/X.gcrig), so a child's chains, points
    and contacts come after, and win over, what it inherits."""
    seen = seen or set()
    path = os.path.abspath(path)
    if path in seen: return ''
    seen.add(path)
    text = open(path, encoding='utf-8').read()
    m = re.search(r'\bparent:(/Game/\S+)', text.lstrip().split('\n\n')[0])         # the header block: up to the first blank line
    if m:
        parent = content_file(m.group(1), '.gcrig')
        if os.path.exists(parent): return gcrig_text(parent, seen) + '\n' + text
    return text


def parse_rig(text):
    """(chains {name: [bones]}, goals {chain names with a goal}, contacts {name: (chain, point, bone or None)}, points {name: [x, y, z]})
    in declaration order."""
    chains, goals, contacts, points = {}, set(), {}, {}
    for m in re.finditer(r'^\s*(\w+)\s*=\s*(\w+(?:\s*->\s*\w+)+)(.*)$', text, re.M):
        chains[m.group(1)] = [b.strip() for b in m.group(2).split('->')]
        if '[goal=' in m.group(3): goals.add(m.group(1))
    for m in re.finditer(r'^\s*(\w+)\s*=\s*contact\(chain=(\w+),\s*at=\$(\w+)(?:,\s*bone=(\w+))?\)', text, re.M):
        contacts[m.group(1)] = (m.group(2), m.group(3), m.group(4))
    for m in re.finditer(r'^\s*(\w+)\s*=\s*\(\s*(-?[\d.]+\s*,\s*-?[\d.]+\s*,\s*-?[\d.]+)\s*\)', text, re.M):
        points[m.group(1)] = [float(v) for v in m.group(2).split(',')]
    return chains, goals, contacts, points


def sides(bones):
    """The left/right suffix pair this rig names its sides by: whichever of ('_l', '_r') / ('_L', '_R') pairs more bones."""
    best = None
    for l, r in (('_l', '_r'), ('_L', '_R')):
        n = sum(1 for b in bones if b.endswith(l) and b[:-len(l)] + r in bones)
        if n and (best is None or n > best[0]): best = (n, l, r)
    return (best[1], best[2]) if best else ('_L', '_R')


def derive_joints(chains, goals, contacts):
    """The joints the reader reads: every chain's bones (no twist / corrective / metacarpal helpers, no `root`), in declaration order,
    leaving out the end-effector sub-chains (fingers, toes: chains that start on an arm or a leg and are neither), of which each hand
    contributes its first and last finger's base (not the thumb): what lets the fit see which way a palm faces."""
    legs = {c for c, _, _ in contacts.values()}
    arms = {c for c in goals if c not in legs}
    owner = {}
    for c in chains:
        for b in chains[c]: owner.setdefault(b, c)
    sub = [c for c in chains if c not in legs and c not in arms and owner[chains[c][0]] in (legs | arms) and len(chains[c]) > 2]
    out = []
    keep = lambda b: b != 'root' and not any(s in b for s in SKIP)
    for c in chains:
        if c in sub: continue
        out += [b for b in chains[c] if keep(b) and b not in out]
    groups = {}
    for c in sub:
        if 'thumb' in c or owner[chains[c][0]] not in arms: continue
        groups.setdefault(chains[c][0], []).append(c)
    for start, cs in groups.items():
        for c in ([cs[0], cs[-1]] if len(cs) > 1 else cs):
            base = [b for b in chains[c][1:] if keep(b)]
            if base and base[0] not in out: out.append(base[0])
    return out


class Profile:
    """One character's constants, derived (derive()), the optional override over them, the framing read when asked for."""
    FRAMING = ('cameras', 'centre', 'length', 'fill', 'look_jitter', 'origin')

    def __init__(self, **kw):
        self.__dict__.update(kw)

    def __getattr__(self, name):
        if name in Profile.FRAMING:
            frame = self.__dict__.get('_frame_file')
            if frame and os.path.exists(frame):
                data = json.load(open(frame))
                if name in data: return data[name]
            if name in ('fill', 'look_jitter'): return {'fill': (0.35, 0.8), 'look_jitter': 3.0}[name]
            raise SystemExit('the %s profile has no %s (the character\'s size and the preview\'s placement of it): render it once with '
                             '`python picture_route.py references %s <clip>` (saves %s), or give --origin-manifest <a gc_render manifest.json '
                             'with a bones highlight>; or set "%s" in an override file' % (self.name, name, self.name, frame, name))
        raise AttributeError(name)

    def is_left(self, name):
        """Whether a leg key or a bone name is the left side's ('B_L', 'l', 'thigh_l')."""
        return name[-1] in 'Ll'

    def mirrored(self, bone):
        """The bone on the other side (a bone with no side is its own mirror)."""
        l, r = self.left, self.right
        if bone.endswith(l): return bone[:-len(l)] + r
        if bone.endswith(r): return bone[:-len(r)] + l
        return bone

    def across(self, X, index):
        """The body's right-minus-left across its mid-plane pairs (hips, shoulders), summed left to right; `index` names a bone's row in X."""
        m = self.mids
        t = X[index(m[1])] - X[index(m[0])]
        for k in range(2, len(m), 2): t = t + X[index(m[k + 1])] - X[index(m[k])]
        return t


def find(name, suffix):
    for folder in FOLDERS:
        path = os.path.join(folder, name + suffix)
        if os.path.exists(path): return path
    return None


def read_header(path):
    """The .gcmove header an existing move of the character carries (everything before its `into:`), as lines ending in newlines."""
    text = open(path, encoding='utf-8').read()
    head = text.split('into:')[0].rstrip()
    head = re.sub(r'[ \t]+$', '', head, flags=re.M)
    return head + '\n'


def derive(name, cfg, override=None):
    """The profile for a character config (a dict) and its optional override."""
    override = override or {}
    gcrig = content_file(cfg['gcrig'], '.gcrig'); ikrig = content_file(cfg['ikrig'], '.gc')
    chains, goals, contacts, points = parse_rig(gcrig_text(gcrig))
    bones = [b for b in derive_joints(chains, goals, contacts) if b not in override.get('skip_joints', ())]
    left, right = sides({b for c in chains.values() for b in c})
    prof = Profile(name=name, left=left, right=right, gcrig=gcrig, ikrig=ikrig, mesh=cfg['mesh'], skin=content_file(cfg['skin'], '') if cfg.get('skin') else None)
    prof.chains, prof.contacts, prof.points = chains, contacts, points
    # planted legs: every chain the rig declares contacts for, in declaration order, rolling on its contacts in the file's order
    legs, limbs, stands, raw = {}, {}, {}, {}
    for c in chains:
        mine = [n for n, (chain, _, _) in contacts.items() if chain == c]
        if not mine: continue
        chain = chains[c]; toe = chain[-1]
        legs[toe] = (c, ', '.join(mine))
        sided = [b for b in chain if (b.endswith(left) or b.endswith(right)) and not re.search(r'clavicle|scapula', b)]
        root = override.get('limb_roots', {}).get(c) or (sided[0] if sided else chain[0])
        key = c[4:] if c.startswith('leg_') else c
        raw[key] = (tuple(chain[chain.index(root) + 1:]), root)
        stands[toe] = chain[-2]
    for key in override.get('limb_order', raw):
        js, root = raw[key]                                      # a leg's partner: the leg whose root is this one's mirror
        mate = [k for k, v in raw.items() if v[1] == prof.mirrored(root) and k != key]
        pair = (root, prof.mirrored(root)) if prof.is_left(root) else (prof.mirrored(root), root)
        limbs[key] = (js, mate[0] if mate else key, pair)
    prof.legs, prof.limbs, prof.stands = legs, limbs, stands
    # the mid-plane pairs: each limb root (legs) then each arm's (the chains with a goal that are not legs: its first sided bone that is not a clavicle / scapula)
    pairs = []
    for key in limbs:
        p = limbs[key][2]
        if p not in pairs: pairs.append(p)
    for c in chains:
        if c in goals and c not in {v[0] for v in legs.values()}:
            sided = [b for b in chains[c] if b.endswith((left, right)) and not re.search(r'clavicle|scapula', b)]
            if sided and sided[0].endswith(left):
                p = (sided[0], prof.mirrored(sided[0]))
                if p not in pairs: pairs.append(p)
    prof.mids = tuple(b for p in pairs for b in p)
    # the chest: the top of a `chest` chain, else a bone named chest, else the bone below the neck on the chain from the pelvis
    if 'chest' in chains: prof.chest = chains['chest'][-1]
    elif 'chest' in {b for c in chains.values() for b in c}: prof.chest = 'chest'
    else:
        spine = max((c for c in chains.values() if any(b.startswith('neck') for b in c)), key=len, default=[])
        below = [b for b in spine if b.startswith('neck')]
        prof.chest = spine[spine.index(below[0]) - 1] if below and spine.index(below[0]) else (spine[-1] if spine else None)
    prof.bones = bones
    prof.floor_feet = 1                      # one foot down fits a floor
    prof.four_paw_mirror = len(legs) >= 4    # a hidden leg copies its partner where the rig has four legs
    # whether the reader's left and right can be trusted (a person: 2-7 % swapped in front views), so a joint takes only its own side's
    # peaks and the fit tries no label-swapped starts; false (the default, the cat's look-alike paws): either side's peak may be a joint's
    prof.sides_distinct = bool(cfg.get('sides_distinct', False))
    prof.level = 30.0 if prof.skin else 0.0  # head levelling only where a skin file says what to level by (ear tips); off otherwise
    prof.reader = cfg.get('reader') or route_paths.reader(name)
    safe = re.sub(r'\W', '', name).capitalize()
    prof.random_prefix = 'A_%sRandomPoses' % safe
    biped = len(legs) == 2                   # a person's pose caps: pelvis and spine stay upright-ish (random poses, dino_poses.py)
    spine = set(chains['chest'] if 'chest' in chains else chains.get('spine', [])) - {'pelvis'}
    prof.pose_cap_deg = 45.0
    prof.pose_caps = dict({b: 12.0 for b in spine}, pelvis=10.0) if biped else {}
    prof.pose_pelvis_cm = 2.0
    here = lambda p: content_file(p, '') if p else p
    prof.rig = content_file(cfg['rig'], '') if cfg.get('rig') else os.path.join(NOW, 'A_%sNow.gltf' % safe)
    prof.calib = content_file(cfg['calib'], '') if cfg.get('calib') else prof.rig
    # the skeleton gc_import puts a clip on, from the mesh's sidecar (`Skeleton = obj'/pkg.obj'`)
    skeleton = cfg.get('skeleton')
    sidecar = content_file(cfg['mesh'], '.gc')
    if not skeleton and os.path.exists(sidecar):
        m = re.search(r"^\s*Skeleton = obj'([^']+)'", open(sidecar, encoding='utf-8').read(), re.M)
        skeleton = m.group(1) if m else None
    prof.skeleton = skeleton
    # the .gcmove header: a character on a stage Blueprint, or a bare skeleton, then the IK rig, controls and physics asset
    if cfg.get('header_from'): header = read_header(content_file(cfg['header_from'], ''))
    elif cfg.get('character'):
        header = 'gcmove 1 character:%s part:%s worn:%s\n' % (cfg['character'], cfg.get('part', 'Body'), cfg.get('worn', 'SkeletalMesh'))
        header += '         ik_rig:%s\n         controls:%s\n' % (game_path(cfg['ikrig']), game_path(cfg.get('controls', cfg['gcrig'])))
        if cfg.get('physics_asset'): header += '         physics_asset:%s\n' % game_path(cfg['physics_asset'])
    else:
        header = 'gcmove 1 skeleton:%s\n         ik_rig:%s\n' % ((skeleton or '').split('.')[0], game_path(cfg['ikrig']))
    prof.header = header
    prof._frame_file = os.path.join(SAVED, 'characters', name + '.frame.json')
    for k, v in override.items():
        if k in ('limb_roots', 'limb_order', 'skip_joints') or k.startswith('_'): continue
        if k == 'legs': v = {t: tuple(x) for t, x in v.items()}
        elif k in ('mids', 'fill'): v = tuple(v)
        setattr(prof, k, v)
    return prof


def load(name):
    config = find(name, '.json')
    if not config: raise SystemExit('no character "%s": run `python picture_route.py init %s --gcrig ... --ikrig ... --mesh ...` (or put %s.json in %s)'
                                    % (name, name, name, FOLDERS[0]))
    cfg = json.load(open(config))
    override = find(name, '.override.json')
    return derive(name, cfg, json.load(open(override)) if override else None)


def _take(argv, flag):
    """Removes `--flag value` or `--flag=value` from argv (in place); its value, or None."""
    for k, a in enumerate(argv):
        if a == flag and k + 1 < len(argv):
            value = argv[k + 1]; del argv[k:k + 2]; return value
        if a.startswith(flag + '='):
            del argv[k]; return a[len(flag) + 1:]
    return None


def choose(argv=None, environ=None):
    """The character named on argv (`--character`) or in the environment (GC_CHARACTER); the cat when neither says."""
    argv = sys.argv if argv is None else argv
    environ = os.environ if environ is None else environ
    return _take(argv, '--character') or environ.get('GC_CHARACTER') or 'cat'


_manifest = _take(sys.argv, '--origin-manifest') or os.environ.get('GC_ORIGIN_MANIFEST')
_named = _take(sys.argv, '--character') or os.environ.get('GC_CHARACTER')
NAME = _named or 'cat'
if _manifest: os.environ['GC_ORIGIN_MANIFEST'] = os.path.abspath(_manifest)
# A character named on the command line or in GC_CHARACTER must load (its error is the answer). The cat is only the default where its files
# are: in a project without them (every customer's), `list`, `init` and `setup` run with no character loaded (ACTIVE None).
if _named: ACTIVE = load(NAME)
else:
    try: ACTIVE = load(NAME) if find(NAME, '.json') else None
    except (Exception, SystemExit): ACTIVE = None
if ACTIVE is not None: os.environ['GC_CHARACTER'] = NAME   # a script this one starts makes the same choice
_index = None


def _bone_index(rig_names):
    return rig_names.index


def places_of_manifest(path):
    """(origin, {bone: [x, y, z]}) from a render manifest with a bones highlight (bubbles[].origin and .centre), as dino_data.places reads them."""
    origin, places = None, {}
    for view in json.load(open(path))['views']:
        for b in view.get('bubbles', []):
            g = re.findall(r'[-0-9.]+', b.get('origin', ''))
            if len(g) == 3: origin = [float(x) for x in g]
            if 'bone' in b and 'centre' in b: places[b['bone']] = [float(x) for x in re.findall(r'[-0-9.]+', b['centre'])]
    return origin, places


def origin_of_manifest(path):
    found = places_of_manifest(path)[0]
    if found is None: raise SystemExit('%s: no bubbles[].origin in the manifest (render with a bones highlight)' % path)
    return found


def frame_of_manifest(path, fov=50.0):
    """Cameras, centre, length, fill and origin from a manifest of the character's bones: its middle, its longest extent, two cameras that
    frame it (a side camera on its left and one from the front right and above) and the random cameras' share of the picture."""
    import numpy as np
    origin, places = places_of_manifest(path)
    P = np.array(list(places.values()))
    if len(P) < 4 or origin is None: raise SystemExit('%s: needs a bones highlight (>= 4 bones with places and an origin)' % path)
    lo, hi = P.min(0), P.max(0)
    centre = (lo + hi) / 2.0; extent = float((hi - lo).max())
    tall = (hi - lo)[2] >= max((hi - lo)[0], (hi - lo)[1])
    d = (extent / 2.0) / 0.65 / (np.tan(np.radians(fov) / 2.0) * 540.0 / 960.0)         # the longest extent fills 65 % of the picture's height
    c = [round(float(v), 2) for v in centre]
    return {'origin': origin, 'centre': c, 'length': round(extent, 2),
            'cameras': {'left': {'location': [round(float(centre[0] - d), 2), c[1], c[2]], 'look_at': c, 'fov': fov},
                        'front_right_high': {'location': [round(float(centre[0] + 0.7 * d), 2), round(float(centre[1] + 0.7 * d), 2), round(float(centre[2] + 0.25 * d), 2)],
                                             'look_at': c, 'fov': fov}},
            'fill': [0.24, 0.54] if tall else [0.35, 0.8], 'look_jitter': round(extent / 18.0, 2)}


def origin(profile=None):
    """The character's placement in the preview (cm): the manifest's when one was given, else the profile's own (override or saved framing)."""
    path = os.environ.get('GC_ORIGIN_MANIFEST')
    if path: return origin_of_manifest(path)
    return list((profile or ACTIVE).origin)
