# Two animations rendered from the same cameras at the same moments, and how many pixels differ. Usage: python compare_renders.py <asset_a> <asset_b> [frames]
import sys
import numpy as np
import render
CAMS = {'left': {'location': [-140, 5, 14], 'look_at': [0, 5, 13], 'fov': 38}, 'front_right_high': {'location': [95, 105, 55], 'look_at': [2, 5, 12], 'fov': 40}}
frames = [int(x) for x in sys.argv[3].split(',')] if len(sys.argv) > 3 else [0, 4, 8, 12, 16, 20, 24, 28]
times = [f / 30 for f in frames]
for name, cam in CAMS.items():
    a = render.frames(sys.argv[1], cam, times, 960, 540)
    b = render.frames(sys.argv[2], cam, times, 960, 540)
    rows = []
    for f, x, y in zip(frames, a, b):
        X = np.asarray(x, float); Y = np.asarray(y, float)
        d = np.abs(X - Y).sum(2)
        cat = (X.sum(2) > 30) | (Y.sum(2) > 30)
        rows.append('f%d %.2f%%' % (f, 100.0 * (d > 30).sum() / max(cat.sum(), 1)))
    print('%-17s differing pixels, as a share of the cat:  %s' % (name, '  '.join(rows)))
