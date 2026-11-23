# Red/cyan overlays of two animations from the capture cameras: red = only the first (the original), cyan = only the second (the
# capture), grey = both. Usage: python overlay.py <asset_a> <asset_b> <out.png> [frames]
import sys
import numpy as np
from PIL import Image, ImageDraw
import render
CAMS = {'left': {'location': [-140, 5, 14], 'look_at': [0, 5, 13], 'fov': 38}, 'front_right_high': {'location': [95, 105, 55], 'look_at': [2, 5, 12], 'fov': 40}}
frames = [int(x) for x in sys.argv[4].split(',')] if len(sys.argv) > 4 else [0, 8, 16, 24]
rows = []
for name, cam in CAMS.items():
    a = render.frames(sys.argv[1], cam, [f / 30 for f in frames], 960, 540)
    b = render.frames(sys.argv[2], cam, [f / 30 for f in frames], 960, 540)
    tiles = []
    for f, x, y in zip(frames, a, b):
        X = x.convert('L'); Y = y.convert('L')
        t = Image.merge('RGB', (X, Y, Y)).resize((480, 270))
        ImageDraw.Draw(t).text((6, 4), '%s f%d' % (name, f), fill=(255, 255, 0))
        tiles.append(t)
    rows.append(tiles)
sheet = Image.new('RGB', (480 * len(frames), 270 * len(rows)))
for r, tiles in enumerate(rows):
    for c, t in enumerate(tiles): sheet.paste(t, (c * 480, r * 270))
sheet.save(sys.argv[3]); print(sys.argv[3])
