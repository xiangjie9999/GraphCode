# The built move beside the pictures it was keyed from (MOVE-CAP1 E20): each picture, the move rendered at that still's moment from the
# picture's own solved camera (still_gcmove.py keeps it in stills_report.json), and the two laid over each other, one row a picture.
# Usage: python still_compare.py <stills_dir> <built clip /Game/...> <out.png> [--times t1 t2 ...]   (default: stills.json's "at"; a
#        travel-timed move names its keys' times, the build's note "pose 1 at 0.000 s, pose 2 at ...")
import argparse, json, os
import numpy as np
from PIL import Image, ImageDraw
import render

ROW = 480                                                                # each picture's width on the sheet


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('stills'); ap.add_argument('clip'); ap.add_argument('out'); ap.add_argument('--times', type=float, nargs='*')
    a = ap.parse_args()
    spec = json.load(open(os.path.join(a.stills, 'stills.json')))
    report = json.load(open(os.path.join(a.stills, 'stills_report.json')))
    times = a.times or [float(s['at']) for s in spec['stills']]
    rows = []
    for t, still in zip(times, report):
        for pic in still.get('pictures') or [still]:                     # a report from before several pictures a still: one, at its top
            w, h = pic['size']; rh = int(round(ROW * h / w))
            view = {'name': 'v', 'width': ROW, 'height': rh, 'camera': pic['camera'], 'at': t}
            r = render.call({'asset': a.clip, 'views': [view]})
            built = Image.open(r['data']['views'][0]['path']).convert('RGB')
            picture = Image.open(os.path.join(a.stills, pic['image'])).convert('RGB').resize((ROW, rh))
            both = Image.blend(picture, built, 0.5)
            row = Image.new('RGB', (3 * ROW, rh + 22), (24, 24, 24))
            for k, im in enumerate((picture, built, both)): row.paste(im, (k * ROW, 22))
            ImageDraw.Draw(row).text((6, 4), '%s at %.2f s: the picture | the built move from its camera | both' % (pic['image'], t),
                                     fill=(230, 230, 230))
            rows.append(row)
    sheet = Image.new('RGB', (3 * ROW, sum(r.height for r in rows)), (24, 24, 24))
    y = 0
    for r in rows: sheet.paste(r, (0, y)); y += r.height
    sheet.save(a.out)
    print('wrote', a.out, '(%d pictures)' % len(rows))


if __name__ == '__main__':
    main()
