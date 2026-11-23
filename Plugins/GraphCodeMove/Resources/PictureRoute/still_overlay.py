# A still with the reader's peaks and the fitted rig drawn over it (MOVE-CAP1 E20): each joint's strongest peak as a ring, the rig
# fitted with its camera unknown (fit_still.fit_free) as dots joined bone to bone, so a reader slip or a fit gone wrong shows at a glance.
# Usage: python still_overlay.py <stills_dir> <peaks.json> <out_dir> [image ...]   (stills.json's down paws used where it lists the image)
import json, os, sys
import numpy as np
from PIL import Image, ImageDraw
import capture, characters, fit_still, gltf_rig


def draw(model, path, peaks, got, out):
    p, turn, place, factor, px, how, pitch = got
    im = Image.open(path).convert('RGB'); w, h = im.size
    d = ImageDraw.Draw(im)
    lens = fit_still.Lens(w, h)
    U = gltf_rig.to_unreal(model.positions(p))
    rest = model.params_of(model.rig.rest_r, model.rig.rest_t)
    centre = gltf_rig.to_unreal(model.positions(rest)).mean(0)
    Q = (U - centre) @ fit_still.rotation(turn).T + place
    at = {b: lens.project(Q[model.rig.index(b)], factor)[0] for b in capture.BONES}
    r = max(3, w // 300)
    for j, b in enumerate(capture.BONES):
        q = peaks[j]
        if q and q[0][2] > 0.2:
            x, y = q[0][:2]; d.ellipse([x - 2 * r, y - 2 * r, x + 2 * r, y + 2 * r], outline=(255, 200, 0), width=2)
    for b in capture.BONES:
        i = model.rig.index(b); k = model.rig.parent[i]
        while k >= 0 and model.rig.names[k] not in at: k = model.rig.parent[k]
        if k >= 0: d.line([tuple(at[model.rig.names[k]]), tuple(at[b])], fill=(0, 220, 255), width=2)
    for b, (x, y) in at.items(): d.ellipse([x - r, y - r, x + r, y + r], fill=(255, 60, 60))
    d.text((10, 10), '%s  %.1f px  lens %.0f deg down  %s' % (os.path.basename(path), px, np.degrees(pitch), how), fill=(255, 255, 255))
    im.save(out)


def main():
    stills, peaks_file, out = sys.argv[1:4]
    os.makedirs(out, exist_ok=True)
    read = json.load(open(peaks_file))
    by = {n: (pk, size) for n, pk, size in zip(read['images'], read['peaks'], read['sizes'])}
    spec = os.path.join(stills, 'stills.json')
    # A still may be several pictures of one pose (stills.json "image" a list): they are fitted together, as still_gcmove fits them, and the
    # one pose is drawn over each with its own camera.
    groups = []
    for s in (json.load(open(spec))['stills'] if os.path.exists(spec) else []):
        names = s['image'] if isinstance(s['image'], list) else [s['image']]
        groups.append((names, s.get('down', [])))
    wanted = set(sys.argv[4:])
    import types
    model = fit_still.model_of(types.SimpleNamespace(**fit_still.DEFAULTS))
    for names, down in groups or [([n], list(characters.ACTIVE.legs)) for n in read['images']]:
        if wanted and not wanted & set(names): continue
        p, cams = fit_still.fit_views_free(model, [by[n] for n in names], down=down)
        for name, cam in zip(names, cams):
            draw(model, os.path.join(stills, name), by[name][0], (p,) + tuple(cam), os.path.join(out, os.path.splitext(name)[0] + '_fit.png'))
            print(name, '%.1f px' % cam[3], cam[4], flush=True)


if __name__ == '__main__':
    main()
