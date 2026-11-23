# Where the picture route keeps its things, the same on every machine (stdlib only: the editor's own Python imports it).
#   the project   `--project <folder of the .uproject>` on the command line (the editor passes it; taken out of sys.argv here), else
#                 GRAPHCODE_PROJECT, else the folder two up from here when it holds a .uproject (a project's own Tools/VideoCapture), else
#                 the first folder up from here that holds one (the plugin's Resources/PictureRoute inside a project). Set into
#                 GRAPHCODE_PROJECT, so every script this one starts sees the same project.
#   per user      %LOCALAPPDATA%/GraphCode/PictureRoute: the reader's Python (venv, made by `picture_route.py setup`) and the DINOv2 cache
#                 (torchhub): several gigabytes, shared by every project on the PC.
#   per project   <project>/Saved/GraphCode/PictureRoute/models: each character's taught reader (GRAPHCODE_PICTURE_MODELS overrides).
# The machine the route was first measured on keeps its own environment under D:/VideoCaptureML; it is used only where it exists and the
# per-user one does not (nobody else has it, and nothing here tells anyone to make it).
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))


def _take(flag):
    for k, a in enumerate(sys.argv):
        if a == flag and k + 1 < len(sys.argv):
            value = sys.argv[k + 1]; del sys.argv[k:k + 2]; return value
        if a.startswith(flag + '='):
            del sys.argv[k]; return a[len(flag) + 1:]
    return None


def _has_project(folder):
    try: return any(f.endswith('.uproject') for f in os.listdir(folder))
    except OSError: return False


def _project():
    given = _take('--project') or os.environ.get('GRAPHCODE_PROJECT')
    if given: return os.path.normpath(os.path.abspath(given))
    up = os.path.normpath(os.path.join(HERE, '..', '..'))
    if _has_project(up): return up
    folder = HERE
    while os.path.dirname(folder) != folder:
        folder = os.path.dirname(folder)
        if _has_project(folder): return folder
    raise SystemExit('the picture route does not know which project it is for: pass --project <the folder that holds the .uproject> '
                     '(the editor passes it)')


PROJECT = _project()
os.environ['GRAPHCODE_PROJECT'] = PROJECT
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'      # the scripts this one starts write no __pycache__ beside the shipped scripts either
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'      # the scripts this one starts write no __pycache__ beside the shipped scripts either

LOCAL = os.path.join(os.environ.get('LOCALAPPDATA') or os.path.expanduser('~'), 'GraphCode', 'PictureRoute')
VENV = os.path.join(LOCAL, 'venv')
LOCAL_HUB = os.path.join(LOCAL, 'torchhub')
_DEV = 'D:/VideoCaptureML'


def venv_python(venv=VENV):
    return os.path.join(venv, 'Scripts', 'python.exe') if os.name == 'nt' else os.path.join(venv, 'bin', 'python')


def python():
    """The reader's Python: GRAPHCODE_PICTURE_PYTHON, else the per-user environment `setup` makes (its path even before it is made)."""
    given = os.environ.get('GRAPHCODE_PICTURE_PYTHON')
    if given: return given
    mine = venv_python()
    dev = _DEV + '/venv/Scripts/python.exe'
    return dev if not os.path.exists(mine) and os.path.exists(dev) else mine


def hub():
    """The torch hub cache the DINOv2 backbone is read from: TORCH_HOME, else the per-user one `setup` fills."""
    given = os.environ.get('TORCH_HOME')
    if given: return given
    dev = _DEV + '/torchhub'
    return dev if not os.path.isdir(LOCAL_HUB) and os.path.isdir(dev) else LOCAL_HUB


# What the DINOv2 backbone needs in the torch hub cache, and its size where measured. NO SILENT DOWNLOAD (slice 5 review, 19 Nov): torch.hub
# would fetch both from GitHub and dl.fbaipublicfiles.com when they are missing; the route stops and says what is missing instead. Here, not in
# dino_reader.py, so `picture_route.py cuda` asks without loading a character (a customer's project has none until one is added).
WEIGHT_MB = {'vitb14': 346, 'vits14': 88}


def backbone_missing(name='vitb14', cache=None):
    """Empty when the DINOv2 code and this backbone's weights are in the hub cache; else what is missing, its size and where it goes."""
    cache = cache or hub()
    repo = os.path.join(cache, 'hub', 'facebookresearch_dinov2_main')
    weights = os.path.join(cache, 'hub', 'checkpoints', 'dinov2_%s_reg4_pretrain.pth' % name)
    gone = []
    if not os.path.isdir(repo): gone.append('the DINOv2 code (github.com/facebookresearch/dinov2, a few MB) in %s' % repo)
    if not os.path.exists(weights):
        gone.append('the weights dinov2_%s_reg4_pretrain.pth (%s) in %s' % (name, '%d MB' % WEIGHT_MB[name] if name in WEIGHT_MB else 'size not measured', os.path.dirname(weights)))
    return '; '.join(gone)


# What the DINOv2 backbone needs in the torch hub cache, and its size where measured. NO SILENT DOWNLOAD (slice 5 review, 19 Nov): torch.hub
# would fetch both from GitHub and dl.fbaipublicfiles.com when they are missing; the route stops and says what is missing instead. Here, not in
# dino_reader.py, so `picture_route.py cuda` asks without loading a character (a customer's project has none until one is added).
WEIGHT_MB = {'vitb14': 346, 'vits14': 88}


def backbone_missing(name='vitb14', cache=None):
    """Empty when the DINOv2 code and this backbone's weights are in the hub cache; else what is missing, its size and where it goes."""
    cache = cache or hub()
    repo = os.path.join(cache, 'hub', 'facebookresearch_dinov2_main')
    weights = os.path.join(cache, 'hub', 'checkpoints', 'dinov2_%s_reg4_pretrain.pth' % name)
    gone = []
    if not os.path.isdir(repo): gone.append('the DINOv2 code (github.com/facebookresearch/dinov2, a few MB) in %s' % repo)
    if not os.path.exists(weights):
        gone.append('the weights dinov2_%s_reg4_pretrain.pth (%s) in %s' % (name, '%d MB' % WEIGHT_MB[name] if name in WEIGHT_MB else 'size not measured', os.path.dirname(weights)))
    return '; '.join(gone)


def models():
    """The folder each character's taught reader is written to (<name>.pt)."""
    return (os.environ.get('GRAPHCODE_PICTURE_MODELS') or os.path.join(PROJECT, 'Saved', 'GraphCode', 'PictureRoute', 'models')).replace('\\', '/')


def reader(name):
    """A character's reader when its config names none: <models>/<name>.pt (one taught before under D:/VideoCaptureML/models is kept where
    it is, when only that one exists)."""
    mine = '%s/%s.pt' % (models(), name)
    dev = '%s/models/%s.pt' % (_DEV, name)
    return dev if not os.path.exists(mine) and os.path.exists(dev) else mine
