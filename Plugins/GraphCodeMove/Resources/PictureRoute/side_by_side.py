# A video of two animations side by side with their overlay, per capture camera, one row each: original | capture | overlay, and first,
# given the folder of the video the capture read (track_all.py render: <camera>_<frame>.png), that video.
# Usage: python side_by_side.py <asset_a> <asset_b> <out.mp4> [label_a] [label_b] [input_frames_dir]
import os, sys
import cv2
import numpy as np
from PIL import Image, ImageDraw
import render
CAMS = {'left': {'location': [-140, 5, 14], 'look_at': [0, 5, 13], 'fov': 38}, 'front_right_high': {'location': [95, 105, 55], 'look_at': [2, 5, 12], 'fov': 40}}
la = sys.argv[4] if len(sys.argv) > 4 else 'original'; lb = sys.argv[5] if len(sys.argv) > 5 else 'from video'
times = [f / 30 for f in range(30)] * 2
shots = {n: (render.frames(sys.argv[1], c, times[:30], 640, 360, view_mode='lit'), render.frames(sys.argv[2], c, times[:30], 640, 360, view_mode='lit')) for n, c in CAMS.items()}
source = sys.argv[6] if len(sys.argv) > 6 else None
writer = None
for k in range(60):
    rows = []
    for n in CAMS:
        a, b = shots[n][0][k % 30], shots[n][1][k % 30]
        ov = Image.merge('RGB', (a.convert('L'), b.convert('L'), b.convert('L')))
        tiles = []
        shown = [(a, la), (b, lb), (ov, 'overlay: red only %s, cyan only %s' % (la, lb))]
        if source: shown.insert(0, (Image.open(os.path.join(source, '%s_%03d.png' % (n, k % 30))).convert('RGB').resize((640, 360)), 'the video it read'))
        for img, text in shown:
            t = img.copy(); ImageDraw.Draw(t).text((8, 6), '%s  f%d' % (text, k % 30), fill=(255, 255, 0)); tiles.append(np.asarray(t))
        rows.append(np.hstack(tiles))
    frame = np.vstack(rows)[:, :, ::-1]
    if writer is None: writer = cv2.VideoWriter(sys.argv[3], cv2.VideoWriter_fourcc(*'mp4v'), 30, (frame.shape[1], frame.shape[0]))
    writer.write(np.ascontiguousarray(frame))
writer.release(); print(sys.argv[3])
