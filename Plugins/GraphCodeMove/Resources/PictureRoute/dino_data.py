# Pictures for the joint reader (dino_reader.py), each with where every joint is in it. A clip's moment is rendered once, small, with
# every bone drawn: the render's manifest names each drawn joint's place (the preview's placement included, as track_all.first_frame
# reads it). The same moment is rendered plain from cameras round the character, lit or unlit, a few with the bones drawn (a video
# made from a drawn first frame may keep some of them), and each joint projected into each picture. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E14).
# Usage: python dino_data.py train <out_dir> <times per clip> <asset> [<asset> ...]     random cameras round the character
#        (an asset argument @list.txt stands for the /Game/... paths in that file, one a line: dino_poses.py --authored writes one)
#        every script takes --character cat|metahuman (characters.py); the MetaHuman's origin comes from --origin-manifest
#        train [--seed N]: the cameras' and moments' draws start from N (default 14) plus the pictures already in <out_dir>, so a
#        fresh folder rendered with another seed draws new cameras (picture_route.py teach --tag, the Train window's render)
#        python dino_data.py clip <out_dir> <asset> <frames> [lit|unlit ...]          capture.CAMERAS, every frame of a 1 s clip (a test)
#        python dino_data.py take <clip_dir> <look> <take_dir>     one look of a 'clip' set as a take folder (<camera>_NNN.png, cameras.json,
#                                                                  first.json with frame 0's joints, frames.json), as track_all lays them out
import json, os, re, sys, time
import numpy as np
from PIL import Image
import camera, capture, characters, render

def __getattr__(name):     # CENTRE: the character's middle in the preview (cm), what random cameras look at; LENGTH: its longest extent, how far a camera stands (characters.py framing)
    if name == 'CENTRE': return np.array(characters.ACTIVE.centre)
    if name == 'LENGTH': return characters.ACTIVE.length
    raise AttributeError(name)


# THE STAGE (2026-11-16): a character whose animation preview is not how it looks (a MetaHuman: the body mesh alone, no head, no
# clothes, no hair) is rendered on its stage Blueprint playing the clip (gc_render asset=<stage>, animation=<clip>), the look an image
# model is shown. GC_STAGE=<the Blueprint's /Game path> turns it on; unset, every render is the clip's own preview as before. The
# stage stands the character elsewhere in its picture's space, so the cameras are moved by where its pelvis stands against the
# preview's (OFFSET, measured once on the first clip).
STAGE = os.environ.get('GC_STAGE') or None
OFFSET = np.zeros(3)


def subject(asset):
    return {'asset': STAGE, 'kind': 'scene', 'animation': asset} if STAGE else {'asset': asset}


def measure_offset(asset):
    global OFFSET
    if not STAGE: return
    probe = [{'name': 'o', 'width': 64, 'height': 64, 'at': 0, 'highlights': [{'type': 'bones', 'of': ['pelvis'], 'draw': ['ray']}]}]
    a = places(json.load(open(render.call({'asset': asset, 'views': probe})['data']['manifest']))['views'][0])['pelvis']
    b = places(json.load(open(render.call(dict(subject(asset), views=probe))['data']['manifest']))['views'][0])['pelvis']
    OFFSET = np.array(b) - np.array(a)
    print('stage %s: the character stands %s cm from its preview' % (STAGE, OFFSET.round(1).tolist()), flush=True)


def places(view):
    """{bone: [x, y, z]} from a drawn view's manifest entry."""
    return {b['bone']: [float(x) for x in re.findall(r'[-0-9.]+', b['centre'])] for b in view.get('bubbles', [])}


def random_camera(rng, width, height):
    az, el = np.radians(rng.uniform(0, 360)), np.radians(rng.uniform(-5, 50))
    fov = rng.uniform(30, 60)
    fill = rng.uniform(*characters.ACTIVE.fill)                    # the character's share of the picture's width (the cat's 0.35-0.8)
    d = (characters.ACTIVE.length / fill) / (2 * np.tan(np.radians(fov) / 2))
    at = np.array(characters.ACTIVE.centre) + OFFSET + rng.normal(0, characters.ACTIVE.look_jitter, 3)
    loc = at + d * np.array([np.cos(el) * np.cos(az), np.cos(el) * np.sin(az), np.sin(el)])
    return {'location': loc.round(2).tolist(), 'look_at': at.round(2).tolist(), 'fov': round(float(fov), 2)}


def length_of(asset, probe=(0, 0.5, 1, 1.5, 2, 3, 4, 6)):
    """The clip's length, near enough: renders hold the last pose past the end (GraphCodeRenderService clamps), so the first probe time
    from which every later pose is the same."""
    views = [{'name': 'p%d' % k, 'width': 64, 'height': 64, 'at': t, 'highlights': [{'type': 'bones', 'of': capture.BONES, 'draw': ['ray']}]}
             for k, t in enumerate(probe)]
    m = json.load(open(render.call({'asset': asset, 'views': views})['data']['manifest']))
    P = [np.array([v for _, v in sorted(places(view).items())]) for view in m['views']]
    for k in range(len(P)):
        if all(np.abs(P[j] - P[k]).max() < 1e-3 for j in range(k, len(P))): return probe[k]
    return probe[-1]


def shoot(asset, t, cams, out, start, size):
    """One moment: a tiny drawn view for the places, then each camera's picture. cams: [(camera, look)], look 'lit', 'unlit' or
    'drawn' (lit, every bone drawn). Returns the label rows."""
    w, h = size
    views = [{'name': 'places', 'width': 64, 'height': 64, 'at': t, 'highlights': [{'type': 'bones', 'of': capture.BONES, 'draw': ['ray']}]}]
    for k, (cam, look) in enumerate(cams):       # 'rings': unlit with every bone drawn, as the rings route (track_all) reads it
        v = {'name': 'v%d' % k, 'width': w, 'height': h, 'camera': cam, 'at': t, 'view_mode': 'unlit' if look in ('unlit', 'rings') else 'lit'}
        if look in ('drawn', 'rings'): v['highlights'] = [{'type': 'bones', 'of': capture.BONES, 'draw': ['ray']}]
        views.append(v)
    r = render.call(dict(subject(asset), views=views))['data']
    m = json.load(open(r['manifest']))
    P = places(m['views'][0])
    rows = []
    for k, (cam, look) in enumerate(cams):
        name = '%06d.png' % (start + k)
        Image.open(r['views'][k + 1]['path']).convert('RGB').save(os.path.join(out, name))
        pin = camera.Camera.from_render(cam, w, h)
        rows.append({'image': name, 'asset': asset, 'at': t, 'look': look, 'camera': cam, 'width': w, 'height': h,
                     'joints': {b: pin.project(np.array(p))[0].round(3).tolist() for b, p in P.items()},
                     'places': P})
    return rows


def expand(args):
    """Asset arguments, an `@file` replaced by the lines of that file (blank lines and #-comments out)."""
    out = []
    for a in args:
        if a.startswith('@'):
            out += [l.strip() for l in open(a[1:], encoding='utf-8') if l.strip() and not l.lstrip().startswith('#')]
        else: out.append(a)
    return out


def take(clip_dir, look, out):
    rows = [r for r in json.load(open(os.path.join(clip_dir, 'labels.json'))) if r['look'] == look]
    os.makedirs(out, exist_ok=True)
    cams, first, n = {}, {}, 0
    for r in rows:
        Image.open(os.path.join(clip_dir, r['image'])).save(os.path.join(out, '%s_%03d.png' % (r['view'], r['frame'])))
        cams[r['view']] = r['camera']; n = max(n, r['frame'] + 1)
        if r['frame'] == 0: first[r['view']] = r['places']
    json.dump(cams, open(os.path.join(out, 'cameras.json'), 'w'), indent=1)
    json.dump({'asset': rows[0]['asset'], 'at': 0.0, 'places': first}, open(os.path.join(out, 'first.json'), 'w'), indent=1)
    json.dump({'asset': rows[0]['asset'], 'frames': n, 'times': [f / (n - 1) for f in range(n)]}, open(os.path.join(out, 'frames.json'), 'w'))


def main():
    mode, out = sys.argv[1], sys.argv[2]
    if mode == 'take':
        take(sys.argv[2], sys.argv[3], sys.argv[4]); return
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, 'labels.json')
    rows = json.load(open(path)) if os.path.exists(path) else []
    t0 = time.time()
    if mode == 'train':
        seed = int(characters._take(sys.argv, '--seed') or 14)
        per, assets = int(sys.argv[3]), expand(sys.argv[4:])
        measure_offset(assets[0])
        rng = np.random.default_rng(len(rows) + seed)
        size = (768, 576)
        for asset in assets:
            L = length_of(asset)
            times = [0.0] if L == 0 else sorted(rng.uniform(0, L, per))
            for t in times:
                looks = rng.choice(['lit', 'unlit', 'drawn'], 7, p=[0.6, 0.25, 0.15])
                cams = [(random_camera(rng, *size), str(look)) for look in looks]
                rows += shoot(asset, float(t), cams, out, len(rows), size)
            json.dump(rows, open(path, 'w'))
            print('%-60s length %.2f s, %d moments; %d pictures (%.0f s)' % (asset, L, len(times), len(rows), time.time() - t0), flush=True)
    elif mode == 'clip':
        asset, frames = sys.argv[3], int(sys.argv[4])
        looks = sys.argv[5:] or ['lit', 'unlit']
        for f in range(frames):
            t = f / (frames - 1)
            cams = [(cfg, look) for look in looks for cfg in capture.CAMERAS.values()]
            new = shoot(asset, t, cams, out, len(rows), (capture.WIDTH, capture.HEIGHT))
            for row, (name, look) in zip(new, [(n, l) for l in looks for n in capture.CAMERAS]):
                row['view'] = name; row['frame'] = f
            rows += new
        json.dump(rows, open(path, 'w'))
        print('%d pictures (%.0f s)' % (len(rows), time.time() - t0))


if __name__ == '__main__':
    main()
