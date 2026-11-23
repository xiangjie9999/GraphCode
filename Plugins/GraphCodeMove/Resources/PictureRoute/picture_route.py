# Pictures to animation, one entry point for ANY character (Plugins/GraphCode/Docs/Guides/MOTION-FROM-VIDEO.md, "Pictures to animation in
# four commands"). The character is described once by its own GraphCode files (characters.py derives its joints, legs, feet, pairs and
# chest from the .gcrig and IK rig); no constant is edited. Every step prints the NEXT command. Steps that need the editor (its bridge,
# http://localhost:3000) or the graphics card only run with --run; without it they print what they would run.
#   init <name> --gcrig <.gcrig or /Game path> --ikrig <IK rig> --mesh </Game/... skeletal mesh>
#        [--character-bp </Game/... stage Blueprint> --part Body --worn SkeletalMesh --controls <rig> --physics-asset </Game/...>]
#        [--from-gcmove <an existing .gcmove of the character>: its header is copied] [--skin <file>] [--skeleton </Game/..>] [--reader <model.pt>]
#   setup [--venv <dir>] [--hub <dir>]     the reader's own Python made on this PC, run by any Python 3.9+ (the editor's own): a venv
#                                           (default %LOCALAPPDATA%/GraphCode/PictureRoute/venv), PyTorch with CUDA (download.pytorch.org,
#                                           about 4.5 GB), NumPy, SciPy, OpenCV, Pillow (PyPI), then the DINOv2 backbone into the torch hub
#                                           cache (346 MB). Downloads: only when run (the Train window's Set up... asks first)
#   --project <folder of the .uproject> on any step: the project the route works in (route_paths.py; the editor passes it)
#   list                                    every character with a config: one JSON line each (its reader and whether it is taught, the
#                                           rendered teach sets and their pictures, a held-out set, the framing): the Train window's list
#   cuda                                    whether the reader's Python has torch with a CUDA card (one line: "cuda yes <card>" or "cuda no: why")
#   check <name> [--bridge]                 what is missing: files, environment, rig template, framing, reader (and the editor with --bridge)
#   template <name> <clip /Game/...> [--run]  the rig template glTF from the editor, no older export needed (rig_now.py --scratch)
#   references <name> <clip /Game/...> [--out dir] [--run]   the character's size and the two reference pictures to hand an image model
#   teach <name> <clip /Game/...>... [--count 24] [--times 20] [--epochs 30] [--run]   random poses + the clips given -> renders -> a reader
#        [--steps render|reader] only the renders (the editor) or only the reader (the graphics card; no clips needed), [--out <model.pt>]
#        where the reader is written (default the character's), [--data <dir>,<dir>] the rendered sets it is taught on (default the teach
#        folder's data), [--pictures N] at most N of them, [--test <dir>] a held-out labelled set read every 5 passes and at the end,
#        [--init <model.pt>] taught on from that reader's weights (dino_reader.py --init), [--tag T] with --steps render: a render that
#        ADDS pictures: random poses with a seed of its own imported into the fresh /Game/__PictureRoute/<name>/r<T>, the clips kept in
#        teach/<name>/r<T>, the pictures in a fresh teach/<name>/data_<T>; nothing earlier is overwritten (an import over a clip stops the
#        editor on a modal question)
#   overlay <name> <stills_dir> [--out dir]  the reader's points and the fitted rig drawn over each picture (still_overlay.py)
#        (ruling 106 (i), the Train window: Saved/Research/MOVE-PARAM1/CODER-NOTES-13.md)
#   heldout <name> <labelled dir> [--reader <model.pt>] [--fits 2] [--pictures 3] [--out dir]   the reader on held-out pictures: px and the
#        worst joints, cm of the pose fitted from two pictures of a moment (camera withheld), and a few pictures with its points drawn
#   references ... [--keep-framing]: the saved framing used and left as it is (the Pose tab's Reference photo)
#   read <name> <stills_dir>                every picture's joints (peaks.json), by the character's reader
#   fit <name> <stills_dir> <clip /Game/.../A_New>   fit the rig to each picture (stills.gltf, stills_report.json), no editor
#   build <name> <stills_dir> <clip /Game/.../A_New> <out.gcmove> [--run]   import the glTF as a NEW clip and write the .gcmove
import json, os, subprocess, sys
import route_paths                              # takes --project out of sys.argv: the project every step works in

HERE = os.path.dirname(os.path.abspath(__file__))
# The reader's Python (torch with CUDA): `--python <python.exe>` on this command line (the editor passes its setting to each process it starts),
# else GRAPHCODE_PICTURE_PYTHON, else the per-user one `setup` makes (route_paths.py). The scripts this one starts use the same (they get it as
# the variable).
def _python():
    for k, a in enumerate(sys.argv):
        if a == '--python' and k + 1 < len(sys.argv):
            value = sys.argv[k + 1]; del sys.argv[k:k + 2]; return value
    return route_paths.python()
PY = _python()
os.environ['GRAPHCODE_PICTURE_PYTHON'] = PY
STEPS = ('setup', 'list', 'cuda', 'init', 'check', 'template', 'references', 'teach', 'overlay', 'heldout', 'read', 'fit', 'build')
NO_CHARACTER = ('setup', 'list', 'cuda')         # steps that name no character


def opts(argv, flags=(), values=()):
    """(positionals, {option: value or True}) ; `values` take an argument, `flags` do not; --authored-style lists are not needed."""
    pos, got, k = [], {}, 0
    while k < len(argv):
        a = argv[k]
        if a.startswith('--') and a[2:] in values: got[a[2:]] = argv[k + 1]; k += 2
        elif a.startswith('--') and a[2:] in flags: got[a[2:]] = True; k += 1
        elif a.startswith('--'): raise SystemExit('unknown option %s' % a)
        else: pos.append(a); k += 1
    return pos, got


def show(cmd): return ' '.join('"%s"' % c if ' ' in c else c for c in cmd)


def run(cmd, go, why):
    """Runs a command (`go`) or prints it as the next thing to run; returns its exit code (0 when only printed)."""
    print('%s: %s' % ('running' if go else 'next', why)); print('  ' + show(cmd), flush=True)
    return subprocess.call(cmd, cwd=HERE) if go else 0


def script(name, *args): return [PY, os.path.join(HERE, name + '.py')] + list(args)


def step_init(argv):
    pos, o = opts(argv, values=('gcrig', 'ikrig', 'mesh', 'character-bp', 'part', 'worn', 'controls', 'physics-asset', 'from-gcmove', 'skin', 'skeleton', 'reader'))
    if len(pos) != 1 or not all(k in o for k in ('gcrig', 'ikrig', 'mesh')): raise SystemExit(__doc__.split('#   check')[0])
    cfg = {'gcrig': o['gcrig'], 'ikrig': o['ikrig'], 'mesh': o['mesh']}
    for src, dst in (('character-bp', 'character'), ('part', 'part'), ('worn', 'worn'), ('controls', 'controls'), ('physics-asset', 'physics_asset'),
                     ('from-gcmove', 'header_from'), ('skin', 'skin'), ('skeleton', 'skeleton'), ('reader', 'reader')):
        if src in o: cfg[dst] = o[src]
    folder = os.path.join(route_paths.PROJECT, 'Saved', 'Research', 'VideoCapture', 'characters')   # the project's, never beside this script
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, pos[0] + '.json')
    with open(path, 'w') as f: json.dump(cfg, f, indent=2)
    os.environ['GC_CHARACTER'] = pos[0]; sys.argv = ['picture_route']
    try:
        import characters                          # derives the character from its files: a config that does not derive is not kept
    except BaseException:
        os.remove(path); raise
    print('wrote %s' % path)
    summary(characters.ACTIVE)
    print('next: python picture_route.py check %s' % pos[0])


def summary(p):
    print('character %s: %d joints read, %d planted leg(s) %s, sides %s/%s, chest %s, mid-plane %s, floor from %d foot/feet, head level %s'
          % (p.name, len(p.bones), len(p.legs), list(p.legs), p.left, p.right, p.chest, list(p.mids), p.floor_feet, 'on' if p.level else 'off'))


def step_setup(argv):
    """The reader's own Python, made on this PC (the Train window's Set up...): a virtual environment made by the Python running this one (the
    editor's own, stdlib only), PyTorch with CUDA and the rest from PyPI, then the DINOv2 backbone (Meta, Apache-2.0) into the torch hub cache.
    Downloads only when run; safe to run again (pip keeps what is installed, the hub keeps what it has). A line per stage, 'setup: ...'."""
    pos, o = opts(argv, values=('venv', 'hub', 'index'))
    venv = os.path.abspath(o.get('venv') or route_paths.VENV)
    hub = os.path.abspath(o.get('hub') or route_paths.LOCAL_HUB)
    py = route_paths.venv_python(venv)
    def call(cmd, why, env=None):
        print('setup: ' + why, flush=True); print('  ' + show(cmd), flush=True)
        code = subprocess.call(cmd, env=env)
        if code: raise SystemExit('setup stopped: %s failed (exit %d)' % (why, code))
    # One that no longer runs (its base Python moved with an engine update) is made again in place; pip then reinstalls into it.
    works = os.path.exists(py) and subprocess.call([py, '-c', 'pass'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0
    if works: print('setup: the environment is there already (%s)' % venv, flush=True)
    else: call([sys.executable, '-m', 'venv', '--clear', venv] if os.path.exists(venv) else [sys.executable, '-m', 'venv', venv], 'making the environment in %s' % venv)
    pip = [py, '-m', 'pip', 'install', '--disable-pip-version-check', '--progress-bar', 'off']
    call(pip + ['torch', 'torchvision', '--index-url', o.get('index', 'https://download.pytorch.org/whl/cu128')],
         'PyTorch with CUDA (download.pytorch.org, about 4.5 GB; minutes, depending on the connection)')
    call(pip + ['numpy', 'scipy', 'opencv-python', 'pillow'], 'NumPy, SciPy, OpenCV and Pillow (PyPI)')
    os.makedirs(hub, exist_ok=True)
    env = dict(os.environ, TORCH_HOME=hub)
    # dino_reader.backbone(): the same hub entry, so what is fetched here is what the reader loads (dinov2_vitb14_reg4_pretrain.pth).
    call([py, '-c', "import torch; torch.hub.load('facebookresearch/dinov2', 'dinov2_vitb14_reg', trust_repo=True)"],
         'the DINOv2 backbone into %s (346 MB)' % hub, env)
    print('setup done: %s' % py, flush=True)


def step_list(argv):
    """One JSON line per character config (shipped characters/<name>.json, or Saved/Research/VideoCapture/characters/), never an override or
    a framing file: its reader and whether it exists (taught), the rendered teach sets under Saved/Research/VideoCapture/teach/<name>/ (a
    folder named data or data_* with a labels.json) and their pictures, a held-out set there (heldout/labels.json) and the framing file.
    Read-only, stdlib only (fast: the window lists at once)."""
    import characters
    names = []
    for folder in characters.FOLDERS:
        if not os.path.isdir(folder): continue
        for f in sorted(os.listdir(folder)):
            if f.endswith('.json') and not f.endswith(('.override.json', '.frame.json')) and f[:-5] not in names: names.append(f[:-5])
    for name in names:
        row = {'name': name}
        try:
            prof = characters.load(name)
        except (Exception, SystemExit) as e:
            row['error'] = str(e); print(json.dumps(row), flush=True); continue
        teach = os.path.join(characters.SAVED, 'teach', name)
        sets = []
        if os.path.isdir(teach):
            for f in sorted(os.listdir(teach)):
                labels = os.path.join(teach, f, 'labels.json')
                if (f == 'data' or f.startswith('data_')) and os.path.exists(labels): sets.append((os.path.join(teach, f), len(json.load(open(labels)))))
        held = os.path.join(teach, 'heldout', 'labels.json')
        authored = []                            # the clips a render of this character listed beside its random poses (assets.txt)
        for folder in [teach] + ([os.path.join(teach, f) for f in sorted(os.listdir(teach)) if f.startswith('r') and f[1:].isdigit()] if os.path.isdir(teach) else []):
            listed = os.path.join(folder, 'assets.txt')
            if os.path.exists(listed):
                for line in open(listed).read().split():
                    if not line.startswith('/Game/__PictureRoute/') and line not in authored: authored.append(line)
        row.update(reader=prof.reader, taught=os.path.exists(prof.reader), reader_time=os.path.getmtime(prof.reader) if os.path.exists(prof.reader) else 0,
                   sets=[{'dir': d, 'pictures': n} for d, n in sets], heldout=os.path.dirname(held) if os.path.exists(held) else '',
                   heldout_pictures=len(json.load(open(held))) if os.path.exists(held) else 0, framing=os.path.exists(prof._frame_file) or 'cameras' in vars(prof),
                   mesh=prof.mesh, skeleton=prof.skeleton or '', legs=len(prof.legs), sides_distinct=prof.sides_distinct, bones=list(prof.bones),
                   authored=authored, rig_template=os.path.exists(prof.rig))
        print(json.dumps(row), flush=True)


def step_cuda(argv):
    """One line for the Train window: the reader teaches only on a CUDA card (dino_reader.py train: device cuda)."""
    try:
        import torch
    except Exception as e:
        print('cuda no: torch is not in %s (%s)' % (sys.executable, e), flush=True); return
    if torch.cuda.is_available(): print('cuda yes %s, torch %s' % (torch.cuda.get_device_name(0), torch.__version__), flush=True)
    else: print('cuda no: torch %s sees no CUDA card' % torch.__version__, flush=True)
    missing = route_paths.backbone_missing()     # never `import dino_reader`: it loads a character, and a new project has none yet
    print('backbone ' + ('cached' if not missing else 'missing: ' + missing), flush=True)


def step_check(argv):
    pos, o = opts(argv, flags=('bridge',))
    import characters, gltf_rig
    p = characters.ACTIVE; bad = []
    def line(ok, what, fix=None):
        print('  [%s] %s' % ('ok' if ok else 'MISSING', what))
        if not ok and fix: bad.append(fix)
    summary(p)
    line(os.path.exists(p.gcrig), 'rig file %s' % p.gcrig, 'give the right --gcrig to `init`')
    line(os.path.exists(p.ikrig), 'IK rig file %s' % p.ikrig, 'give the right --ikrig to `init`')
    line(bool(p.legs), 'a planted leg (a chain with `contacts` in the .gcrig, parents followed)', 'declare contacts for the feet: GraphCode rig guide, "contacts"')
    line(len(p.bones) >= 6, '%d joints to read (needs six or more)' % len(p.bones))
    line(bool(p.skeleton), 'skeleton for the clip import (%s)' % p.skeleton, 'give --skeleton </Game/...> to `init`')
    py_ok = os.path.exists(PY) and subprocess.call([PY, '-c', 'import torch, numpy, scipy, PIL'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0
    line(py_ok, 'reader environment (%s with torch, numpy, scipy, pillow)' % PY, 'set up the reader\'s Python (a separate download, see the guide)')
    have = os.path.exists(p.rig)
    line(have, 'rig template glTF %s' % p.rig, 'python picture_route.py template %s <a clip of the character /Game/...> --run' % p.name)
    if have:
        names = set(gltf_rig.Rig(p.rig).names); gone = [b for b in p.bones if b not in names]
        line(not gone, 'every joint is in the template' + (' (not: %s)' % gone if gone else ''), 'build the template again from a clip of THIS character')
    try: p.origin; p.cameras; line(True, 'framing (cameras, centre, length, origin)')
    except SystemExit: line(False, 'framing: no render of the character yet', 'python picture_route.py references %s <clip /Game/...> --run' % p.name)
    line(os.path.exists(p.reader), 'reader weights %s' % p.reader, 'python picture_route.py teach %s <clip /Game/...>... --run' % p.name)
    if o.get('bridge'):
        import urllib.request
        try: urllib.request.urlopen('http://localhost:3000/', timeout=3); up = True
        except Exception as e: up = getattr(e, 'code', 0) != 0 and hasattr(e, 'code')
        line(up, 'the editor\'s bridge answers on localhost:3000', 'start the editor (one editor per checkout)')
    print('\nnext: ' + ('; '.join(bad) if bad else 'python picture_route.py read %s <stills_dir>' % p.name))


def step_template(argv):
    pos, o = opts(argv, flags=('run',))
    import characters
    p = characters.ACTIVE
    if run(script('rig_now', '--scratch', pos[1], p.mesh, '30', p.rig, '--character', p.name), o.get('run'), 'read the rig and the clip from the editor, write the template'):
        raise SystemExit('stopped: the rig template was not made from %s' % pos[1])
    print('next: python picture_route.py check %s' % p.name)


def high_three_quarters(frame):
    """The two views the Reference photo is (ruling 106, DINO ON A HUMAN, 18 Nov: two HIGH FRONT THREE-QUARTER views, about +-45 degrees,
    the camera above the chest, NOT front and side): the framing's front_right_high and its mirror across the character's middle (x), the
    front_left_high of the measured V pair (Saved/Research/MOVE-PARAM1/DINO-HUMAN-PROOF.md section 17)."""
    right = frame['cameras']['front_right_high']; c = frame['centre']
    left = dict(right, location=[round(2 * c[0] - right['location'][0], 2)] + list(right['location'][1:]),
                look_at=[round(2 * c[0] - right['look_at'][0], 2)] + list(right['look_at'][1:]))
    return {'front_right_high': right, 'front_left_high': left}


def step_references(argv):
    pos, o = opts(argv, flags=('run', 'keep-framing'), values=('out', 'stage'))
    import characters
    p = characters.ACTIVE
    # Which two views: the framing's own cameras (left and front_right_high, as before), or, where the character's config says
    # "reference_views": "high_three_quarters" (a person: the MetaHuman), the two high front three-quarter views (ruling 106).
    config = characters.find(p.name, '.json')
    high = config and json.load(open(config)).get('reference_views') == 'high_three_quarters'
    out = o.get('out') or os.path.join(characters.SAVED, 'references', p.name)
    if not o.get('run'):
        print('next: render the character once (its bones for its size, then two lit pictures):')
        print('  python picture_route.py references %s %s --run' % (p.name, pos[1])); return
    import render
    os.makedirs(out, exist_ok=True)
    # --keep-framing (the Pose tab's Reference photo): the character's saved framing is used and left as it is, so a posed clip never moves
    # the centre and length the reader was taught on and the fit reads; with no saved framing it is made as before.
    if o.get('keep-framing') and os.path.exists(p._frame_file):
        frame = json.load(open(p._frame_file))
        print('the character\'s saved framing: %s (length %.0f cm, centre %s)' % (p._frame_file, frame['length'], frame['centre']), flush=True)
    else:
        r = render.call({'asset': pos[1], 'views': [{'name': 'places', 'width': 64, 'height': 64, 'at': 0,
                                                     'highlights': [{'type': 'bones', 'of': p.bones, 'draw': ['ray']}]}]})['data']
        frame = characters.frame_of_manifest(r['manifest'])
        os.makedirs(os.path.dirname(p._frame_file), exist_ok=True); json.dump(frame, open(p._frame_file, 'w'), indent=1)
        print('saved the character\'s framing: %s (length %.0f cm, centre %s)' % (p._frame_file, frame['length'], frame['centre']), flush=True)
    cameras = high_three_quarters(frame) if high else frame['cameras']
    subject = {'asset': pos[1]}
    # --stage <Blueprint> (the Pose tab, UX round 2): the clip drawn on the file's own character as the tab shows it (dressed, its head and hair), not on its
    # bare preview body; the stage stands the body elsewhere, so the cameras move by where its pelvis stands against the preview's (dino_data.py THE STAGE).
    if o.get('stage'):
        # Review UXR2 N4: a stage picture never goes over the preview-drawn references the reader was taught on (references/<name>).
        if not o.get('out'): raise SystemExit('references --stage needs --out <folder>: the default folder holds the pictures the reader was taught on')
        import numpy as np, re as _re
        probe = [{'name': 'o', 'width': 64, 'height': 64, 'at': 0, 'highlights': [{'type': 'bones', 'of': ['pelvis'], 'draw': ['ray']}]}]
        def pelvis(sub):
            view = json.load(open(render.call(dict(sub, views=probe))['data']['manifest']))['views'][0]
            for b in view.get('bubbles', []):
                if b['bone'] == 'pelvis': return np.array([float(x) for x in _re.findall(r'[-0-9.]+', b['centre'])])
            # Review UXR2 S10: never a zero offset (both cameras would point at the stage's origin, far from the body).
            raise SystemExit('references --stage: the picture of %s names no bone "pelvis" to place the cameras by, so the reference pictures were not made '
                             '(draw them without --stage, on the preview body)' % sub['asset'])
        subject = {'asset': o['stage'], 'kind': 'scene', 'animation': pos[1]}
        offset = pelvis(subject) - pelvis({'asset': pos[1]})
        print('drawn on the stage %s (the character stands %s cm from its preview)' % (o['stage'], offset.round(1).tolist()), flush=True)
        def moved(cam):
            cam = dict(cam)
            for key in ('location', 'look_at'):
                if key in cam: cam[key] = (np.array(cam[key], dtype=float) + offset).round(2).tolist()
            return cam
        cameras = {k: moved(cam) for k, cam in cameras.items()}
    views = [{'name': k, 'width': 960, 'height': 540, 'camera': cam, 'at': 0, 'view_mode': 'lit'} for k, cam in cameras.items()]
    got = render.call(dict(subject, views=views))['data']
    from PIL import Image
    for v, k in zip(got['views'], cameras):
        Image.open(v['path']).convert('RGB').save(os.path.join(out, 'ref_%s.png' % k))
        print('saved %s' % os.path.join(out, 'ref_%s.png' % k), flush=True)
    print('reference pictures for the image model: %s' % out)
    print('next: give them to the image model with one pose a picture, the whole figure in frame, 16:9, two views of each pose; then\n'
          '  python picture_route.py teach %s <clip /Game/...>... --run   (once per character; skip it if %s exists)' % (p.name, p.reader))


def step_teach(argv):
    pos, o = opts(argv, flags=('run',), values=('count', 'times', 'epochs', 'steps', 'out', 'data', 'pictures', 'test', 'init', 'tag'))
    import characters
    p = characters.ACTIVE
    clips = pos[1:]
    only = o.get('steps', 'all')
    if only not in ('all', 'render', 'reader'): raise SystemExit('--steps takes render or reader (or leave it out: both)')
    if not clips and only != 'reader': raise SystemExit('teach <name> <clip /Game/...>...: the authored clips of the character to teach on (poses it really takes)')
    out = os.path.join(characters.SAVED, 'teach', p.name)
    folder = '/Game/__PictureRoute/%s' % p.name
    go = o.get('run')
    data = o['data'].split(',') if o.get('data') else [os.path.join(out, 'data')]
    poses_out, seed, rendered = out, '14', os.path.join(out, 'data')
    if o.get('tag'):
        tag = o['tag']
        if not tag.isdigit(): raise SystemExit('--tag takes digits (the time: 20261119031500)')
        poses_out, folder, rendered = os.path.join(out, 'r' + tag), folder + '/r' + tag, os.path.join(out, 'data_' + tag)
        seed = str(int(tag) % 2147483647)
        if not o.get('data'): data = [os.path.join(out, 'data'), rendered]
        for there in (poses_out, rendered):
            if os.path.exists(there): raise SystemExit('%s exists: a tagged render goes into fresh folders only' % there)
        # The look new pictures are rendered in follows what the character's READER was taught on (lead, 19 Nov): its config's "render_look",
        # "preview" (the clip's own preview body: the MetaHuman's reader, run 4, was taught on teach/metahuman/data and data_v, the bare preview
        # body) or "stage" (its stage Blueprint, GC_STAGE, dino_data.py "THE STAGE", as data_stage was). Absent: the preview, as the route always did.
        config = characters.find(p.name, '.json')
        cfg = json.load(open(config)) if config else {}
        look = cfg.get('render_look', 'preview')
        if look not in ('preview', 'stage'): raise SystemExit('%s: render_look is "preview" or "stage", not %r' % (config, look))
        if look == 'stage' and not cfg.get('character'): raise SystemExit('%s: render_look "stage" needs the character\'s stage Blueprint ("character")' % config)
        if look == 'stage': os.environ['GC_STAGE'] = cfg['character']
        else: os.environ.pop('GC_STAGE', None)
        print('rendering on the %s (render_look %s in %s: the look its reader was taught on)' % ('stage ' + cfg['character'] if look == 'stage' else 'preview body',
              look, os.path.basename(config) if config else 'no config'), flush=True)
    reader = (script('dino_reader', 'train', *data, '--out', o.get('out', p.reader), '--epochs', o.get('epochs', '30'), '--character', p.name)
              + (['--limit', o['pictures']] if o.get('pictures') else []) + (['--test', o['test']] if o.get('test') else []) + (['--init', o['init']] if o.get('init') else []))
    steps = [(script('dino_poses', p.rig, p.ikrig, p.calib, poses_out, folder, o.get('count', '24'), seed, '--authored', *clips, '--character', p.name),
              'random poses on the rig, imported as clips (the editor), the authored ones listed with them'),
             (script('dino_data', 'train', rendered, o.get('times', '20'), '@' + os.path.join(poses_out, 'assets.txt'), '--character', p.name)
              + (['--seed', seed] if o.get('tag') else []),
              'render every clip from random cameras with each joint\'s place (the editor)'),
             (reader, 'teach the reader (the graphics card; about an hour)')]
    steps = steps[:2] if only == 'render' else steps[2:] if only == 'reader' else steps
    if go and only != 'render': os.makedirs(os.path.dirname(os.path.abspath(o.get('out', p.reader))), exist_ok=True)
    for cmd, why in steps:
        if run(cmd, go, why): raise SystemExit('stopped: %s failed' % why)
    print('next: python picture_route.py read %s <stills_dir>' % p.name if go else 'next: the same with --run, or run the three commands above in order')


def step_overlay(argv):
    """The Pose tab's Upload pictures, beside its trial: each picture with the reader's points and the rig fitted to it drawn over it
    (still_overlay.py, the pictures of one still fitted together as stills.json groups them)."""
    pos, o = opts(argv, values=('out',))
    import characters
    p = characters.ACTIVE
    out = o.get('out') or os.path.join(pos[1], 'drawn')
    if run(script('still_overlay', pos[1], os.path.join(pos[1], 'peaks.json'), out, '--character', p.name), True,
           'draw the reader\'s points and the fitted rig over each picture'): raise SystemExit('stopped: the drawing failed')
    print('pictures drawn: %s' % out, flush=True)


def step_heldout(argv):
    """The reader on held-out pictures (a labelled set, dino_data.py's labels.json, never taught): px and the worst joints (dino_reader test),
    the pose fitted from two pictures of a moment with the camera withheld, cm after the best turn and shift (fit_still test --unknown
    --views 2, every Nth picture so that about `fits` fits run), and `pictures` of them with the reader's points and the fitted rig drawn
    (still_overlay.py). Ruling 106 (i)/(j): what the Train window shows when teaching ends."""
    pos, o = opts(argv, values=('reader', 'fits', 'pictures', 'out'))
    import characters
    p = characters.ACTIVE
    held = pos[1]; reader = o.get('reader', p.reader)
    out = o.get('out') or os.path.join(held, 'result')
    os.makedirs(out, exist_ok=True)
    rows = json.load(open(os.path.join(held, 'labels.json')))
    peaks = os.path.join(out, 'peaks.json')
    for cmd, why in ((script('dino_reader', 'test', reader, held, '--character', p.name), 'the reader against the held-out joints (px)'),
                     (script('dino_reader', 'stills', reader, held, peaks, '--character', p.name), 'read every held-out picture')):
        if run(cmd, True, why): raise SystemExit('stopped: %s failed' % why)
    plain = [r for r in rows if r['look'] in ('lit', 'unlit')]
    every = max(1, len(plain) // max(1, int(o.get('fits', '2'))))
    if run(script('fit_still', 'test', held, peaks, '--unknown', '--views', '2', '--every', str(every), '--character', p.name), True,
           'fit the pose from two pictures of each moment, the camera withheld (cm)'): raise SystemExit('stopped: the held-out fit failed')
    count = int(o.get('pictures', '3'))
    shown = [r['image'] for r in plain[::max(1, len(plain) // max(1, count))]][:count]
    if shown and run(script('still_overlay', held, peaks, os.path.join(out, 'drawn'), *shown, '--character', p.name), True,
                     'draw the reader\'s points and the fitted rig over a few held-out pictures'): raise SystemExit('stopped: the drawing failed')
    print('held-out pictures drawn: %s' % os.path.join(out, 'drawn'), flush=True)


def step_read(argv):
    pos, _ = opts(argv)
    import characters
    p = characters.ACTIVE
    out = os.path.join(pos[1], 'peaks.json')
    if run(script('dino_reader', 'stills', p.reader, pos[1], out, '--character', p.name), True, 'read every picture\'s joints'):
        raise SystemExit('stopped: reading the pictures failed')
    print('next: write %s/stills.json (the order, the feet down, the seconds: see the guide), then\n  python picture_route.py fit %s %s <clip /Game/.../A_New>' % (pos[1], p.name, pos[1]))


def step_fit(argv):
    pos, _ = opts(argv)
    import characters
    p = characters.ACTIVE
    code = run(script('still_gcmove', pos[1], pos[2], os.path.join(pos[1], 'unused.gcmove'), '--fit-only', '--character', p.name), True, 'fit the rig to each picture')
    print('next: python picture_route.py build %s %s %s <out.gcmove> --run   (needs the editor; the clip name must be NEW)' % (p.name, pos[1], pos[2]))
    raise SystemExit(code)


def step_build(argv):
    pos, o = opts(argv, flags=('run',))
    import characters
    p = characters.ACTIVE
    run(script('still_gcmove', pos[1], pos[2], pos[3], '--character', p.name), o.get('run'), 'import the glTF as a new clip, write the .gcmove and build its keys')
    print('next: build %s with gc_apply (create:true), then look at the clip (still_overlay.py lays a key on its picture)' % pos[3])


if __name__ == '__main__':
    if len(sys.argv) >= 2 and sys.argv[1] in NO_CHARACTER: globals()['step_' + sys.argv[1]](sys.argv[2:]); raise SystemExit(0)
    if len(sys.argv) < 3 or sys.argv[1] not in STEPS: raise SystemExit(__doc__)
    step, argv = sys.argv[1], sys.argv[2:]
    if step != 'init': os.environ['GC_CHARACTER'] = argv[0]; sys.argv = ['picture_route']
    globals()['step_' + step](argv if step == 'init' else argv)
