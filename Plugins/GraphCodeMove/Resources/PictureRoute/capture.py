# Synthetic capture: renders an animation from each camera, frame by frame, once plain and once per bone with only that bone drawn,
# and finds each bone's joint ball in its picture. Writes obs.json: per camera its render camera, size, the preview's placement
# (manifest 'origin'), and per bone the ball's pixel centre per frame (pixel centres, so +0.5 on the index). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
# Usage: python capture.py <asset> <frames> <out_dir> [bone ...]
import json, os, re, sys, time
import numpy as np
from PIL import Image
import characters, render, detect

BONES = characters.ACTIVE.bones     # characters.py: derived per character


def __getattr__(name):     # CAMERAS: the character's two cameras (framing: read when asked for, so a script that needs none runs before the character is rendered)
    if name == 'CAMERAS': return characters.ACTIVE.cameras
    raise AttributeError(name)


WIDTH, HEIGHT = 960, 540
MESH = characters.ACTIVE.mesh     # the character the clips play on (rig_now reads its poses)


def shots(asset, camera, times, bones=None):
    """Images and the preview origin for the given times (8 views a call)."""
    images, origin = [], None
    for start in range(0, len(times), 8):
        views = [{'name': 'f%03d' % (start + k), 'width': WIDTH, 'height': HEIGHT, 'camera': camera, 'at': t, 'to': t, 'count': 1}
                 for k, t in enumerate(times[start:start + 8])]
        body = {'asset': asset, 'views': views, 'view_mode': 'unlit'}
        if bones: body['highlights'] = [{'type': 'bones', 'of': bones, 'draw': ['ray']}]
        r = render.call(body)
        data = r['data']
        images += [Image.open(v['path']).convert('RGB') for v in data['views']]
        if bones and origin is None:
            m = json.load(open(data['manifest']))
            for v in m['views']:
                for b in v.get('bubbles', []):
                    g = re.findall(r'[-0-9.]+', b.get('origin', ''))
                    if len(g) == 3: origin = [float(x) for x in g]
    return images, origin


def main():
    asset, frames, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    bones = sys.argv[4:] or BONES
    os.makedirs(out, exist_ok=True)
    times = [f / (frames - 1) for f in range(frames)]
    result = {'asset': asset, 'frames': frames, 'times': times, 'width': WIDTH, 'height': HEIGHT, 'cameras': {}}
    t0 = time.time()
    for name, cam in characters.ACTIVE.cameras.items():
        plain, _ = shots(asset, cam, times)
        entry = {'camera': cam, 'origin': None, 'balls': {}, 'pixels': {}}
        for bone in bones:
            drawn, origin = shots(asset, cam, times, [bone])
            if origin and entry['origin'] is None: entry['origin'] = origin
            centres, counts = [], []
            for p, d in zip(plain, drawn):
                c, r, n = detect.ball_circle(p, d)
                centres.append(None if c is None else [float(c[0]) + 0.5, float(c[1]) + 0.5]); counts.append(int(n))
            entry['balls'][bone] = centres; entry['pixels'][bone] = counts
            print('%s %-16s %d/%d frames found  (%.0f s)' % (name, bone, sum(c is not None for c in centres), frames, time.time() - t0), flush=True)
        result['cameras'][name] = entry
        json.dump(result, open(os.path.join(out, 'obs.json'), 'w'), indent=1)
    print('done in %.0f s' % (time.time() - t0))


if __name__ == '__main__':
    main()
