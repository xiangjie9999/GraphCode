# The joint reader for video with NO bones drawn: DINOv2 (Meta, Apache-2.0; frozen) describes each 14 px patch of a picture the
# same way across frames, lighting and small changes of look, and a small head trained on the character's OWN renders (dino_data.py:
# every joint's place known) turns those descriptions into one heat map per joint. A video model may restyle the character a little
# and cannot keep drawn bones straight; this reads the character itself. Its points go to the rig fit as track_all's balls do
# (dino_track.py). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E14-).
# Runs in its own environment (torch with CUDA, kept off the system Python): D:/VideoCaptureML/venv. Needs only characters.py (stdlib) beside it.
# Every mode takes --character <name> (default cat; a character's reader is models/<name>.pt).
# Usage: python dino_reader.py train <data_dir> [<data_dir> ...] --out <model.pt> [--test <data_dir>] [--epochs N] [--backbone vitb14] [--limit N]
#        (--limit: at most N of the pictures, an even spread over the sets in a fixed order: the Train window's Pictures, ruling 106 (i))
#        python dino_reader.py test <model.pt> <data_dir> [--damage]          px error per joint on a labelled set (labels.json)
#        python dino_reader.py read <model.pt> <frames_dir> <camera> <out.json>   <camera>_NNN.png -> each joint's peaks per frame
#        python dino_reader.py stills <model.pt> <stills_dir> <out.json>   every picture on its own (E20) -> each joint's peaks per still
#        python dino_reader.py zeroshot <data_dir> [--backbone vitb14]           first frame's joint features matched in the others
import argparse, io, json, math, os, sys, time
import numpy as np
import torch, torch.nn as nn, torch.nn.functional as F
from PIL import Image, ImageFilter

# The joints a character's reader reads come from characters.py (derived from the character's .gcrig; `--character <name>`, default cat):
# the reader's output order IS that list, so a reader taught for one character reads only that one (a checkpoint keeps its list).
import characters
CHARACTER = characters.NAME
BONES = characters.ACTIVE.bones
SIZE = 448                  # crop side, 32 DINOv2 patches
OUT = 112                   # heat map side: one cell is 4 px of the crop
SIGMA = 1.5                 # target spread, cells
MEAN, STD = np.array([0.485, 0.456, 0.406]), np.array([0.229, 0.224, 0.225])
import route_paths
HUB = route_paths.hub()     # TORCH_HOME, else the per-user cache `picture_route.py setup` fills (route_paths.py)


# What the backbone needs in the torch hub cache: route_paths.backbone_missing (NO SILENT DOWNLOAD, slice 5 review, 19 Nov).
WEIGHT_MB = route_paths.WEIGHT_MB


def backbone_missing(name='vitb14'):
    return route_paths.backbone_missing(name, HUB)


def backbone(name='vitb14'):
    missing = backbone_missing(name)
    if missing: raise SystemExit('the DINOv2 backbone is not on this machine, and nothing is downloaded from here: %s (Meta, Apache-2.0; '
                                 'fetch it yourself, or point TORCH_HOME at a cache that has it)' % missing)
    os.environ['TORCH_HOME'] = HUB
    m = torch.hub.load('facebookresearch/dinov2', 'dinov2_%s_reg' % name, trust_repo=True)
    for p in m.parameters(): p.requires_grad_(False)
    return m.eval()


class Reader(nn.Module):
    """Frozen DINOv2 (its last four blocks' patch tokens) -> 256 channels at 32 x 32, raised to 112 x 112 and joined with a small
    stem on the picture itself (the patches are 14 px; the stem keeps edges), two convolutions -> one heat map per joint."""

    def __init__(self, dino, joints=len(BONES), tune=0):
        super().__init__()
        self.dino = dino
        self.tune = tune                    # how many of DINOv2's last blocks learn too (0: all frozen)
        for p in (dino.blocks[-tune:].parameters() if tune else []): p.requires_grad_(True)
        if tune:
            for p in dino.norm.parameters(): p.requires_grad_(True)
        d = dino.embed_dim
        self.reduce = nn.Sequential(nn.Conv2d(4 * d, 256, 1), nn.GELU(), nn.Conv2d(256, 256, 3, padding=1), nn.GELU())
        self.stem = nn.Sequential(nn.Conv2d(3, 32, 3, 2, 1), nn.GELU(), nn.Conv2d(32, 64, 3, 2, 1), nn.GELU())
        self.head = nn.Sequential(nn.Conv2d(256 + 64, 192, 3, padding=1), nn.GELU(), nn.Conv2d(192, 128, 3, padding=1), nn.GELU(),
                                  nn.Conv2d(128, joints, 1))

    def features(self, x):
        with torch.set_grad_enabled(self.training and self.tune > 0), torch.autocast('cuda', dtype=torch.bfloat16):
            layers = self.dino.get_intermediate_layers(x, n=4, reshape=True)
        return torch.cat(layers, 1).float()

    def forward(self, x):
        f = self.reduce(self.features(x))
        f = F.interpolate(f, size=(OUT, OUT), mode='bilinear', align_corners=False)
        return self.head(torch.cat([f, self.stem(x)], 1))


# ---- pictures: crops, damage, targets ----

def box_of(points, w, h, margin=0.18, jitter=0.0, rng=None):
    """A square round the points (pixels), `margin` of its side added, moved and resized by up to `jitter`."""
    P = np.array(points)
    lo, hi = P.min(0), P.max(0)
    c, side = (lo + hi) / 2, (hi - lo).max() * (1 + 2 * margin)
    if jitter and rng is not None:
        side *= rng.uniform(1 - jitter, 1 + jitter)
        c = c + rng.uniform(-jitter, jitter, 2) * side
    return [float(c[0] - side / 2), float(c[1] - side / 2), float(side)]


def crop(img, box):
    x, y, s = box
    return img.transform((SIZE, SIZE), Image.AFFINE, (s / SIZE, 0, x, 0, s / SIZE, y), resample=Image.BILINEAR)


def to_tensor(img):
    a = (np.asarray(img, np.float32) / 255.0 - MEAN) / STD
    return torch.from_numpy(a.transpose(2, 0, 1).astype(np.float32))


def silhouette(a):
    """The character's pixels in a render on black: everything but the near-black that reaches the picture's edge. A plain threshold
    (> 12) cut the far legs out of lit renders, where they stand in shadow (2026-10-26)."""
    import cv2
    m = cv2.morphologyEx(((a.max(2) > 3) * 255).astype(np.uint8), cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))
    back = (m == 0).astype(np.uint8); h, w = back.shape
    fill = np.zeros((h + 2, w + 2), np.uint8); outside = back.copy()
    for x, y in [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1), (w // 2, 0), (w // 2, h - 1), (0, h // 2), (w - 1, h // 2)]:
        if outside[y, x] == 1: cv2.floodFill(outside, fill, (x, y), 2)
    return outside != 2


def background(rng, h, w):
    k = rng.integers(4)
    if k == 0: return np.full((h, w, 3), rng.integers(0, 256, 3), np.uint8)
    if k == 1:
        t = np.linspace(0, 1, h)[:, None, None]
        return (rng.integers(0, 256, 3) * (1 - t) + rng.integers(0, 256, 3) * t).repeat(w, 1).astype(np.uint8)
    if k == 2:
        n = rng.integers(0, 256, (max(2, h // 64), max(2, w // 64), 3)).astype(np.uint8)
        return np.asarray(Image.fromarray(n).resize((w, h), Image.BICUBIC))
    n = rng.integers(0, 256, (h // 8 + 1, w // 8 + 1, 3)).astype(np.uint8)
    return np.asarray(Image.fromarray(n).resize((w, h), Image.BILINEAR).filter(ImageFilter.GaussianBlur(2)))


def damage(img, rng, strength=1.0):
    """What a generated video does to a frame we rendered: colour and light drift, softness, noise, compression, a new background."""
    a = np.asarray(img).copy()
    if rng.random() < 0.5 * strength:
        m = silhouette(a)
        a = np.where(m[..., None], a, background(rng, *a.shape[:2]))
    if rng.random() < 0.9 * strength:
        v = np.arange(256, dtype=np.float32)                                                   # exposure, white balance, gamma:
        gain, lift, gamma = rng.uniform(0.6, 1.4), rng.uniform(-30, 30), rng.uniform(0.7, 1.4)   # one lookup per channel
        lut = [np.clip(255.0 * np.clip((v * gain * w + lift) / 255.0, 0, 1) ** gamma, 0, 255).astype(np.uint8) for w in rng.uniform(0.8, 1.2, 3)]
        a = np.stack([lut[k][a[..., k]] for k in range(3)], 2).astype(np.float32)
        g = a.mean(2, keepdims=True); a = g + (a - g) * np.float32(rng.uniform(0.4, 1.6))      # saturation
    else: a = a.astype(np.float32)
    if rng.random() < 0.15 * strength: a = a.mean(2, keepdims=True).repeat(3, 2)
    img = Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))
    if rng.random() < 0.5 * strength: img = img.filter(ImageFilter.GaussianBlur(rng.uniform(0.3, 2.0)))
    if rng.random() < 0.4 * strength:
        a = np.asarray(img).astype(np.float32) + rng.standard_normal(np.asarray(img).shape, dtype=np.float32) * np.float32(rng.uniform(2, 10))
        img = Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))
    if rng.random() < 0.6 * strength:
        buf = io.BytesIO(); img.save(buf, 'JPEG', quality=int(rng.integers(25, 90))); img = Image.open(buf).convert('RGB')
    return img


class Pictures(torch.utils.data.Dataset):
    def __init__(self, dirs, train=True, damage_test=False, limit=0):
        self.rows = []
        for d in dirs:
            for r in json.load(open(os.path.join(d, 'labels.json'))):
                if all(b in r['joints'] for b in BONES): self.rows.append((d, r))
        if limit and limit < len(self.rows):                           # every k-th, so a few pictures still span the clips and looks
            step = len(self.rows) / float(limit)
            self.rows = [self.rows[int(k * step)] for k in range(limit)]
        self.train, self.damage_test = train, damage_test

    def __len__(self): return len(self.rows)

    def __getitem__(self, i):
        d, r = self.rows[i]
        rng = np.random.default_rng() if self.train else np.random.default_rng(i)
        img = Image.open(os.path.join(d, r['image'])).convert('RGB')
        pts = [r['joints'][b] for b in BONES]
        box = box_of(pts, *img.size, margin=rng.uniform(0.08, 0.3) if self.train else 0.18, jitter=0.12 if self.train else 0.0, rng=rng)
        img = crop(img, box)                                           # damage the crop: a third of the pixels (101 -> ~35 ms)
        if self.train or self.damage_test: img = damage(img, rng)
        # Pictures go to the trainer as bytes and the joints as places; the heat maps are made on the GPU (gpu_batch). Float pictures
        # and maps through eight workers ran Windows out of commit memory (error 1455, 2026-10-26).
        return torch.from_numpy(np.asarray(img).copy()).permute(2, 0, 1), torch.tensor(box), torch.tensor(pts, dtype=torch.float32)


def gpu_batch(x, box, pts, device='cuda'):
    """Byte pictures -> normalised floats; joint pixels -> heat maps and which joints the crop holds, on the GPU."""
    x = x.to(device).float() / 255.0
    x = (x - torch.tensor(MEAN, device=device, dtype=torch.float32)[:, None, None]) / torch.tensor(STD, device=device, dtype=torch.float32)[:, None, None]
    box, pts = box.to(device), pts.to(device)
    u = (pts[..., 0] - box[:, None, 0]) / box[:, None, 2] * OUT
    v = (pts[..., 1] - box[:, None, 1]) / box[:, None, 2] * OUT
    inside = ((u >= 0) & (u < OUT) & (v >= 0) & (v < OUT)).float()
    g = torch.arange(OUT, device=device, dtype=torch.float32) + 0.5
    H = torch.exp(-((g[None, None, None, :] - u[..., None, None]) ** 2 + (g[None, None, :, None] - v[..., None, None]) ** 2) / (2 * SIGMA ** 2))
    return x, H * inside[..., None, None], inside


# ---- decoding ----

def peaks(H, box, k=3, radius=2):
    """Per joint, up to k local maxima of its heat map, each refined by the soft centre of its (2r+1)^2 neighbourhood: [(x, y, score)]
    in picture pixels (the pinhole's convention: a pixel index's centre at +0.5)."""
    x0, y0, s = box
    out = []
    pool = F.max_pool2d(H[None], 2 * radius + 1, 1, radius)[0]
    for j in range(H.shape[0]):
        h = H[j]; cand = torch.nonzero((h == pool[j]) & (h > 0.05))
        sc = h[cand[:, 0], cand[:, 1]]
        order = torch.argsort(sc, descending=True)[:k]
        found = []
        for c in order:
            v, u = int(cand[c, 0]), int(cand[c, 1])
            ys, xs = slice(max(0, v - radius), min(h.shape[0], v + radius + 1)), slice(max(0, u - radius), min(h.shape[1], u + radius + 1))
            w = h[ys, xs].clamp(min=0); gy, gx = torch.meshgrid(torch.arange(ys.start, ys.stop), torch.arange(xs.start, xs.stop), indexing='ij')
            cu, cv = float((w * (gx + 0.5)).sum() / w.sum()), float((w * (gy + 0.5)).sum() / w.sum())
            found.append([x0 + cu / OUT * s, y0 + cv / OUT * s, float(sc[c])])
        out.append(found)
    return out


def run(model, imgs, boxes, device='cuda', batch=16):
    """Heat maps for picture crops, CPU tensors (n x joints x OUT x OUT)."""
    out = []
    model.eval()
    with torch.no_grad():
        for k in range(0, len(imgs), batch):
            x = torch.stack([to_tensor(crop(im, b)) for im, b in zip(imgs[k:k + batch], boxes[k:k + batch])]).to(device)
            out.append(model(x).float().cpu())
    return torch.cat(out)


def load(path, device='cuda'):
    state = torch.load(path, map_location='cpu')
    if len(state.get('bones', BONES)) != len(BONES):
        raise SystemExit('%s was taught %d joints; the %s profile reads %d (--character?)' % (path, len(state['bones']), CHARACTER, len(BONES)))
    model = Reader(backbone(state['backbone']), tune=state.get('tune', 0)).to(device)
    model.load_state_dict(state['weights'], strict=False)
    return model.eval()


# ---- modes ----

def evaluate(model, data, device='cuda'):
    """Each joint's best peak against its label, px in the picture: (mean, median, 95 %, share within 5 px, per joint mean)."""
    loader = torch.utils.data.DataLoader(data, batch_size=16, shuffle=False, num_workers=0)
    errs = []
    model.eval()
    with torch.no_grad():
        for x, box, pts in loader:
            H = model(gpu_batch(x, box, pts, device)[0]).float().cpu()
            for h, b, p in zip(H, box, pts):
                pk = peaks(h, b.tolist(), k=1)
                errs.append([math.hypot(q[0][0] - t[0], q[0][1] - t[1]) if q else 999.0 for q, t in zip(pk, p.tolist())])
    E = np.array(errs)
    return E.mean(), np.median(E), np.percentile(E, 95), (E < 5).mean(), E.mean(0)


def commit_free_gb():
    """The machine's free commit memory (Windows GlobalMemoryStatusEx: ullAvailPageFile), GB; None elsewhere."""
    try:
        import ctypes
        class Status(ctypes.Structure):
            _fields_ = [('dwLength', ctypes.c_ulong), ('dwMemoryLoad', ctypes.c_ulong)] + \
                       [(n, ctypes.c_ulonglong) for n in ('total', 'avail', 'pagetotal', 'pageavail', 'virttotal', 'virtavail', 'extended')]
        s = Status(); s.dwLength = ctypes.sizeof(Status)
        return s.pageavail / 1e9 if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(s)) else None
    except Exception:
        return None


def train(args):
    torch.backends.cuda.matmul.allow_tf32 = True
    device = 'cuda'
    # A shared machine (one editor, other agents' builds): my share of the card is capped so the driver never spills into system
    # memory (2026-10-26: a tuned run sat at 9 GB of host commit while the editor started, and the machine fell to 0.6 GB free).
    torch.cuda.set_per_process_memory_fraction(args.gpu_share)
    model = Reader(backbone(args.backbone), tune=args.tune).to(device)
    # --init: start from a taught reader's weights (2026-10-29: the cat's rig moved one paw joint; taught again from its 10-26 reader on
    # pictures of the fixed rig, in fewer passes, each run a shorter window for the shared machine's memory to stop it in).
    if args.init:
        state = torch.load(args.init, map_location='cpu')
        if state['backbone'] != args.backbone or state.get('tune', 0) != args.tune:
            raise SystemExit('%s is a %s reader with %d blocks taught, not %s with %d' % (args.init, state['backbone'], state.get('tune', 0), args.backbone, args.tune))
        model.load_state_dict(state['weights'], strict=False)
    data = Pictures(args.dirs, train=True, limit=args.limit)
    loader = torch.utils.data.DataLoader(data, batch_size=args.batch, shuffle=True, num_workers=args.workers, drop_last=True,
                                         persistent_workers=args.workers > 0)
    head = [p for n, p in model.named_parameters() if not n.startswith('dino.')]
    tuned = [p for n, p in model.named_parameters() if n.startswith('dino.') and p.requires_grad]
    params = head + tuned
    groups = [{'params': head, 'lr': args.lr}] + ([{'params': tuned, 'lr': args.lr * 0.1}] if tuned else [])
    opt = torch.optim.AdamW(groups, weight_decay=1e-4)
    steps = args.epochs * len(loader)
    sched = torch.optim.lr_scheduler.OneCycleLR(opt, max_lr=[g['lr'] for g in groups], total_steps=steps, pct_start=0.1)
    test = Pictures([args.test], train=False) if args.test else None
    print('%d pictures, %d steps a pass' % (len(data), len(loader)), flush=True)
    t0 = time.time()
    stop = False
    for epoch in range(args.epochs):
        model.train(); total = 0.0
        for step, (x, box, pts) in enumerate(loader):
            if step % 25 == 0:
                free = commit_free_gb()
                if free is not None and free < 1.5:
                    print('stopping: the machine has %.1f GB of commit memory free (under 1.5)' % free, flush=True); stop = True; break
            x, H, inside = gpu_batch(x, box, pts, device)
            P = model(x)
            loss = (((P - H) ** 2).mean((2, 3)) * inside).sum() / inside.sum().clamp(min=1) * 100.0
            opt.zero_grad(); loss.backward()
            torch.nn.utils.clip_grad_norm_(params, 1.0)            # at 1e-3 and no clip the loss went 0.040 -> 27.8 at the peak rate
            opt.step(); sched.step(); total += float(loss.detach())
        if stop: break
        line = 'pass %d: loss %.4f (%.0f s)' % (epoch + 1, total / len(loader), time.time() - t0)
        if test is not None and ((epoch + 1) % 5 == 0 or epoch + 1 == args.epochs):
            m, med, p95, within, _ = evaluate(model, test)
            line += '; test px mean %.2f, median %.2f, 95%% %.2f, within 5 px %.1f %%' % (m, med, p95, 100 * within)
        print(line, flush=True)
        torch.save({'backbone': args.backbone, 'tune': args.tune, 'bones': BONES, 'size': SIZE, 'out': OUT,
                    'weights': {k: v for k, v in model.state_dict().items() if not k.startswith('dino.') or
                                (args.tune and (k.startswith('dino.norm') or any(k.startswith('dino.blocks.%d.' % b)
                                 for b in range(len(model.dino.blocks) - args.tune, len(model.dino.blocks)))))}}, args.out)


def test(args):
    model = load(args.model)
    m, med, p95, within, per = evaluate(model, Pictures([args.data], train=False, damage_test=args.damage))
    print('px mean %.2f, median %.2f, 95%% %.2f, within 5 px %.1f %%' % (m, med, p95, 100 * within))
    print('worst joints: ' + ', '.join('%s %.1f' % (BONES[j], per[j]) for j in np.argsort(-per)[:8]))


def read(args):
    """Every frame of one camera: a first look at the whole picture for where the character is, then a look at a crop round what the
    first look found (the crop it was trained on). Writes {camera, frames, boxes, peaks[frame][joint] = [[x, y, score], ...]}."""
    model = load(args.model)
    names = sorted(f for f in os.listdir(args.frames) if f.startswith(args.camera + '_') and f[len(args.camera) + 1:-4].isdigit())
    imgs = [Image.open(os.path.join(args.frames, f)).convert('RGB') for f in names]
    w, h = imgs[0].size
    whole = [-0.0, (h - w) / 2.0, float(w)] if w >= h else [(w - h) / 2.0, 0.0, float(h)]
    rough = run(model, imgs, [whole] * len(imgs))
    boxes = []
    for H in rough:
        pk = peaks(H, whole, k=1)
        pts = [q[0][:2] for q in pk if q and q[0][2] > 0.2]
        boxes.append(box_of(pts, w, h) if len(pts) >= 6 else whole)
    fine = run(model, imgs, boxes)
    out = {'camera': args.camera, 'frames': names, 'width': w, 'height': h, 'bones': BONES, 'boxes': boxes,
           'peaks': [peaks(H, b, k=3) for H, b in zip(fine, boxes)]}
    json.dump(out, open(args.out, 'w'))
    print('%d frames read' % len(names))


def stills(args):
    """Still pictures, each on its own (MOVE-CAP1 E20: key poses an image model made): every picture in the folder (the order its labels.json
    lists them in, where it has one; else by name), each found first in the whole picture and then read in a crop round it, as read() does a
    frame. Writes {images, sizes, boxes, peaks[image][joint] = [[x, y, score], ...]} for fit_still.py."""
    model = load(args.model)
    labels = os.path.join(args.frames, 'labels.json')
    names = [r['image'] for r in json.load(open(labels))] if os.path.exists(labels) else \
        sorted(f for f in os.listdir(args.frames) if f.lower().endswith(('.png', '.jpg', '.jpeg', '.webp')))
    imgs = [Image.open(os.path.join(args.frames, f)).convert('RGB') for f in names]
    boxes = []
    for im in imgs:
        w, h = im.size
        whole = [-0.0, (h - w) / 2.0, float(w)] if w >= h else [(w - h) / 2.0, 0.0, float(h)]
        pk = peaks(run(model, [im], [whole])[0], whole, k=1)
        pts = [q[0][:2] for q in pk if q and q[0][2] > 0.2]
        boxes.append(box_of(pts, w, h) if len(pts) >= 6 else whole)
    fine = run(model, imgs, boxes)
    out = {'images': names, 'sizes': [list(im.size) for im in imgs], 'bones': BONES, 'boxes': boxes,
           'peaks': [peaks(H, b, k=3) for H, b in zip(fine, boxes)]}
    json.dump(out, open(args.out, 'w'))
    print('%d stills read' % len(names))


def zeroshot(args):
    """No training: each joint's DINOv2 description at its known place in the first picture of each camera, looked for in that
    camera's other pictures (the most alike patch, refined by the soft centre of its neighbours). The baseline E14 measures."""
    dino = backbone(args.backbone).cuda()
    rows = json.load(open(os.path.join(args.data, 'labels.json')))
    errs = []
    for view in sorted(set(r.get('view', '') + '/' + r['look'] for r in rows)):
        rs = sorted([r for r in rows if r.get('view', '') + '/' + r['look'] == view], key=lambda r: r.get('frame', 0))
        feats, boxes = [], []
        for r in rs:
            img = Image.open(os.path.join(args.data, r['image'])).convert('RGB')
            box = box_of([r['joints'][b] for b in BONES], *img.size)
            with torch.no_grad():
                f = dino.forward_features(to_tensor(crop(img, box))[None].cuda())['x_norm_patchtokens'][0]
            feats.append(F.normalize(f.reshape(32, 32, -1).permute(2, 0, 1)[None].float(), dim=1)); boxes.append(box)
        g0 = F.interpolate(feats[0], size=(OUT, OUT), mode='bilinear', align_corners=False)[0]
        x0, y0, s0 = boxes[0]
        refs = []
        for b in BONES:
            p = rs[0]['joints'][b]; u, v = int((p[0] - x0) / s0 * OUT), int((p[1] - y0) / s0 * OUT)
            refs.append(g0[:, min(OUT - 1, max(0, v)), min(OUT - 1, max(0, u))])
        R = torch.stack(refs)
        for r, f, box in zip(rs[1:], feats[1:], boxes[1:]):
            g = F.interpolate(f, size=(OUT, OUT), mode='bilinear', align_corners=False)[0]
            sim = torch.einsum('jc,chw->jhw', R, g)
            for j, b in enumerate(BONES):
                pk = peaks(torch.exp((sim[j:j + 1] - 1) * 20).float().cpu(), box, k=1)
                t = r['joints'][b]
                errs.append(math.hypot(pk[0][0][0] - t[0], pk[0][0][1] - t[1]) if pk[0] else 999.0)
    E = np.array(errs)
    print('zero-shot: px mean %.2f, median %.2f, 95%% %.2f, within 5 px %.1f %% (%d joint-frames)' % (E.mean(), np.median(E),
          np.percentile(E, 95), 100 * (E < 5).mean(), len(E)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode'); ap.add_argument('rest', nargs='*')
    ap.add_argument('--out'); ap.add_argument('--test'); ap.add_argument('--epochs', type=int, default=30)
    ap.add_argument('--backbone', default='vitb14'); ap.add_argument('--batch', type=int, default=16)
    ap.add_argument('--lr', type=float, default=3e-4); ap.add_argument('--workers', type=int, default=6)
    ap.add_argument('--damage', action='store_true'); ap.add_argument('--tune', type=int, default=0)   # DINOv2 blocks that learn
    ap.add_argument('--gpu-share', type=float, default=0.45)                                         # of the card, at most
    ap.add_argument('--init')                                                                        # train on from this reader's weights
    ap.add_argument('--limit', type=int, default=0)                                                  # train: at most N pictures (0: all)
    a = ap.parse_args()
    if a.mode == 'train': a.dirs = a.rest; train(a)
    elif a.mode == 'test': a.model, a.data = a.rest; test(a)
    elif a.mode == 'read': a.model, a.frames, a.camera, a.out = a.rest; read(a)
    elif a.mode == 'stills': a.model, a.frames, a.out = a.rest; stills(a)
    elif a.mode == 'zeroshot': a.data = a.rest[0]; zeroshot(a)


if __name__ == '__main__':
    main()
