# E11, the production path rehearsed end to end on renders: separate "generations" of the walk, ONE camera each, every bone drawn,
# each starting at its own moment of the cycle and running at its own uneven pace (rendered at those times), blurred and saved as
# JPEG like a video model's frames; each tracked alone (track_all.track_with_rig, the rig in the loop, its first frame's pose known as
# a frame we render), fitted alone (fit_views, robust), then fused with the others (fuse_takes). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E11).
# Usage: python rehearse_takes.py make <out_dir> <take> <camera> <offset> <pace> <lean>     (~2-3 min a take; run takes in parallel;
#        a take already tracked, obs_<k>.json, is only fitted)
#        then: fuse_takes.py ... <out_dir>/obs_<k>.json <out_dir>/take_<k>.json ...; synthetic_takes.py score <fused.json>
import io, json, os, sys
import numpy as np
from PIL import Image, ImageFilter
import average_takes as A, camera, capture, fit_rig, fit_views, gltf_rig, render, track_all
from synthetic_takes import TRUTH, CALIB, IKRIG, ORIGIN

ASSET = '/Game/__MCPTests/CatMove/A_CatWalkAccel'


def like_a_video(image, blur=0.7, quality=60):
    """A rendered frame as a video model's: softened and JPEG-compressed."""
    buf = io.BytesIO()
    image.filter(ImageFilter.GaussianBlur(blur)).save(buf, 'JPEG', quality=quality)
    return Image.open(io.BytesIO(buf.getvalue())).convert('RGB')


def make(out, k, cam_name, offset, pace, lean):
    os.makedirs(out, exist_ok=True)
    rig = gltf_rig.Rig(TRUTH)
    R0, T0 = A.cycle(rig.anim_r, rig.anim_t); n = len(R0)
    phases = [(offset + f / n + pace * (np.sin(2 * np.pi * f / n + lean) - np.sin(lean)) / (2 * np.pi)) % 1.0 for f in range(n + 1)]
    cfg = capture.CAMERAS[cam_name]
    cam = camera.Camera.from_render(cfg, capture.WIDTH, capture.HEIGHT)
    axes = fit_rig.ik_rig_axes(IKRIG); orders = fit_rig.rig_orders(gltf_rig.Rig(CALIB), axes, rig.names)
    saved = os.path.join(out, 'obs_%02d.json' % k)
    if os.path.exists(saved): obs = json.load(open(saved))           # tracked already: fit only
    else: obs = track(out, k, rig, R0, T0, phases, cam_name, cfg, cam, axes, orders)
    R, T, res, _ = fit_views.run(obs, [cam_name], rig, axes, orders, fit_rig.ik_rig_limits(IKRIG), loss='soft_l1', scale=2.0)
    json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': T.tolist(), 'camera': cam_name, 'offset': offset,
               'pace': pace, 'gain': 1.0, 'shown_phase': phases, 'reprojection_px': res}, open(os.path.join(out, 'take_%02d.json' % k), 'w'))
    balls = obs['cameras'][cam_name]['balls']
    found = sum(v is not None for b in balls.values() for v in b)
    print('take %d (%s, offset %.2f, pace %.2f): %d of %d balls, reprojection %.2f px' % (k, cam_name, offset, pace, found,
          len(balls) * (n + 1), np.mean(res)), flush=True)


def track(out, k, rig, R0, T0, phases, cam_name, cfg, cam, axes, orders):
    """Render the take, as a video, and track its balls (saved as obs_<k>.json)."""
    root = rig.index('root'); n = len(R0)
    frames = [like_a_video(im) for im in render.frames(ASSET, cfg, phases, capture.WIDTH, capture.HEIGHT, bones=capture.BONES)]
    frames[0].save(os.path.join(out, 'take_%02d_first.png' % k))
    r, t = A.at_phase(R0, T0, phases[0]); t = t.copy(); t[root] = rig.rest_t[root]
    P = gltf_rig.to_unreal(rig.globals(t, r)[1])
    first = {b: cam.project(P[rig.index(b)] + ORIGIN)[0] - 0.5 for b in capture.BONES}       # the known first frame
    model = fit_rig.Model(rig, axes, orders, fit_rig.ik_rig_limits(IKRIG))
    balls, rolls = track_all.track_with_rig({cam_name: frames}, {cam_name: (cam, ORIGIN)}, {cam_name: first}, model)
    obs = {'frames': n + 1, 'width': capture.WIDTH, 'height': capture.HEIGHT,
           'cameras': {cam_name: {'camera': cfg, 'origin': ORIGIN, 'balls': balls[cam_name], 'rolls': rolls[cam_name]}}}
    json.dump(obs, open(os.path.join(out, 'obs_%02d.json' % k), 'w'))
    return obs


def save_frames(out, k, cam_name, offset, pace, lean):
    """The take's frames, as the video model would hand them back, into <out>/frames_<k>/<camera>_<NNN>.png (for refine_takes)."""
    n = len(A.cycle(gltf_rig.Rig(TRUTH).anim_r, gltf_rig.Rig(TRUTH).anim_t)[0])
    phases = [(offset + f / n + pace * (np.sin(2 * np.pi * f / n + lean) - np.sin(lean)) / (2 * np.pi)) % 1.0 for f in range(n + 1)]
    folder = os.path.join(out, 'frames_%02d' % k); os.makedirs(folder, exist_ok=True)
    for f, im in enumerate(render.frames(ASSET, capture.CAMERAS[cam_name], phases, capture.WIDTH, capture.HEIGHT, bones=capture.BONES)):
        like_a_video(im).save(os.path.join(folder, '%s_%03d.png' % (cam_name, f)))


if __name__ == '__main__':
    if sys.argv[1] == 'make':
        make(sys.argv[2], int(sys.argv[3]), sys.argv[4], float(sys.argv[5]), float(sys.argv[6]), float(sys.argv[7]))
    if sys.argv[1] == 'frames':
        save_frames(sys.argv[2], int(sys.argv[3]), sys.argv[4], float(sys.argv[5]), float(sys.argv[6]), float(sys.argv[7]))
