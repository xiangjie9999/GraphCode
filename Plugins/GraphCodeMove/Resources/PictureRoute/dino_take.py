# One take from video with no bones drawn, end to end: a folder made by `track_all.py firstplain` (the first frame handed to the video
# model, its joints, its cameras) with each camera's video beside it as <camera>.mp4 (or its frames as <camera>_NNN.png) ->
# frames -> the joint reader's peaks (dino_reader.py, in its own torch environment) -> joints chosen with the rig in the loop
# (dino_track.py) -> the rig fitted to every camera at once (fit_views) -> a glTF on the character's skeleton (write_gltf, travel from
# the planted paws, twist helpers from their drivers) -> imported as a clip, root motion and loop as asked. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
# Usage: python dino_take.py <take_dir> [--clip /Game/.../A_Name] [--fps N (default the video's)] [--loop] [--rings] [--truth truth.gltf]
#        [--model D:/VideoCaptureML/models/cat.pt] [--rig rig.gltf] [--ikrig IKR.gc] [--calib calibration.gltf]
import argparse, glob, json, os, subprocess, sys
import numpy as np
import characters, fit_rig, fit_views, gltf_rig, track_all, twist_helpers, travel, write_gltf

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
NOW = os.path.join(ROOT, 'Saved', 'Research', 'VideoCapture', 'now')
DEFAULTS = {   # the cat as its skeleton is now (rig_now.py; 2026-10-26), and the reader trained on its renders
    'model': 'D:/VideoCaptureML/models/cat.pt',
    'python': 'D:/VideoCaptureML/venv/Scripts/python.exe',
    'rig': os.path.join(NOW, 'A_CatWalkAccelNow.gltf'),
    'calib': os.path.join(NOW, 'A_CatWalkAccelNow.gltf'),
    'ikrig': os.path.join(ROOT, 'Content', '__MCPTests', 'CatMove', 'IKR_BritishShorthairCat.gc'),
}


def stand_on_planted(rig, R, T, gcrig, loop):
    """The take lowered (or raised) so its paws in stance stand their lowest contact on the floor: each paw's median lowest contact over
    its stance frames (the .gcrig's contacts, fit_still.paw_pads), and the lowest of those taken off the pelvis, so no paw is on the
    whole pressed under (the median of them all put a hind paw 1.26 cm under at its stops). Prints the shift."""
    import fit_still
    pads = fit_still.paw_pads(gcrig, rig)
    S = travel.stance(rig, R, T, loop=loop)
    rest_r, rest_p = rig.globals(rig.rest_t, rig.rest_r)
    to_gltf = lambda u: np.array([u[k] / (100.0 * s) for k, (axis, s) in sorted(enumerate(gltf_rig.UNREAL), key=lambda e: e[1][0])])
    root, pel = rig.index('root'), rig.index('pelvis')
    heights = {}
    for f in range(len(R)):
        t = np.array(T[f]); t[root] = rig.rest_t[root]
        W, P = rig.globals(t, np.array(R[f]))
        for k, toe in enumerate(travel.PAWS):
            if not S[min(f, len(S) - 1), k] or toe not in pads: continue
            heights.setdefault(toe, []).append(min((P[b] + W[b] @ rest_r[b].T @ (to_gltf(at) - rest_p[b]))[1] for b, at in pads[toe]))
    if not heights: return T
    drop = min(float(np.median(h)) for h in heights.values())
    T = np.array(T, float)
    for f in range(len(R)):
        W, _ = rig.globals(T[f], np.array(R[f]))
        T[f][pel] = T[f][pel] - W[rig.parent[pel]].T @ np.array([0.0, drop, 0.0])
    print('stood on its planted paws: the body %s %.2f cm (the median lowest contact of each paw in stance: %s)' % ('down' if drop > 0 else 'up', abs(drop) * 100.0,
          ', '.join('%s %.2f' % (toe, 100.0 * float(np.median(h))) for toe, h in heights.items())))
    return T


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('take'); ap.add_argument('--clip'); ap.add_argument('--fps', type=float)       # default: the video's, else 30
    ap.add_argument('--loop', action='store_true'); ap.add_argument('--truth')
    ap.add_argument('--rings', action='store_true')        # the hybrid: the video was made from a first frame with the bones drawn
    for k, v in DEFAULTS.items(): ap.add_argument('--' + k, default=v)
    a = ap.parse_args()
    cameras = json.load(open(os.path.join(a.take, 'cameras.json')))
    # 1. Frames: each camera's video into <camera>_NNN.png at the size its first frame was rendered.
    for c in cameras:
        video = os.path.join(a.take, c + '.mp4')
        if os.path.exists(video):
            import cv2
            v = cv2.VideoCapture(video)
            w, h = v.get(cv2.CAP_PROP_FRAME_WIDTH), v.get(cv2.CAP_PROP_FRAME_HEIGHT)
            # The camera is the first frame's (16:9); a video of another shape was cropped or padded by its generator, and squeezing
            # it into 960 x 540 would move every joint. Refuse rather than measure a different camera.
            if h and abs(w / h - track_all.capture.WIDTH / track_all.capture.HEIGHT) > 0.01:
                raise SystemExit('%s is %dx%d; the first frame was 16:9. Ask the generator for 16:9 (e.g. 1920x1080), not cropped.' % (video, w, h))
            if a.fps is None: a.fps = v.get(cv2.CAP_PROP_FPS) or None       # the video's own rate (generators make 24 or 30)
            if not os.path.exists(os.path.join(a.take, '%s_000.png' % c)):
                print('%s: %d frames' % (c, track_all.frames_from_video(video, c, a.take)))
    a.fps = a.fps or 30.0
    seen = [c for c in cameras if os.path.exists(os.path.join(a.take, '%s_000.png' % c))]
    if not seen: raise SystemExit('no frames: put <camera>.mp4 or <camera>_NNN.png in %s' % a.take)
    counts = {c: len(glob.glob(os.path.join(a.take, '%s_[0-9][0-9][0-9].png' % c))) for c in seen}
    n = min(counts.values())
    if not os.path.exists(os.path.join(a.take, 'frames.json')):
        json.dump({'asset': a.take, 'frames': n, 'times': [f / a.fps for f in range(n)]}, open(os.path.join(a.take, 'frames.json'), 'w'))
    # 2. The joint reader on each camera's frames (its own environment: torch with CUDA on D:).
    peaks = []
    for c in seen:
        out = os.path.join(a.take, 'peaks_%s.json' % c); peaks.append(out)
        if not os.path.exists(out):
            subprocess.check_call([a.python, os.path.join(HERE, 'dino_reader.py'), 'read', a.model, a.take, c, out])
    # 3. Joints with the rig in the loop -> obs.json.
    subprocess.check_call([sys.executable, os.path.join(HERE, 'dino_track.py'), a.take, a.rig, a.ikrig, a.calib] + peaks
                          + (['--rings'] if a.rings else []))
    obs = json.load(open(os.path.join(a.take, 'obs.json')))
    # 4. The rig fitted to every camera at once, robust to a few wrong points.
    rig = gltf_rig.Rig(a.rig); axes = fit_rig.ik_rig_axes(a.ikrig)
    orders = fit_rig.rig_orders(gltf_rig.Rig(a.calib), axes, rig.names)
    R, T, res, model = fit_views.run(obs, seen, rig, axes, orders, fit_rig.ik_rig_limits(a.ikrig), loss='soft_l1', scale=3.0, loop=a.loop,
                                     first=fit_views.first_pose(os.path.join(a.take, 'first.json'), rig))
    fitted = os.path.join(a.take, 'fit.json')
    json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': T.tolist(), 'reprojection_px': res}, open(fitted, 'w'))
    print('fitted %d frames from %s: reprojection %.2f px mean' % (len(R), seen, np.mean(res)))
    if a.truth: fit_views.report(R, T, gltf_rig.Rig(a.truth))
    # 4b. The take stood on its planted paws (2026-10-30, Docs/Research/cat-animation-accuracy-20261028.md): a walk whose paws are on the
    # floor came back 0.8-1.0 cm high at every paw in stance (its video fitted from the capture cameras), and GraphCode's plants, which hold
    # a paw ON the floor, then pressed each leg that far into its drawing; it sprang back as the paw rolled (the left wrist 25 degrees in
    # one sample). The body goes down by the median lowest contact of the paws in stance, one number for the take.
    T = stand_on_planted(rig, R, T, os.path.join(ROOT, 'Content', '__MCPTests', 'CatMove', 'BritishShorthairCat.gcrig'), a.loop)
    # 5. A glTF on the character's skeleton: twist helpers from their drivers, travel from the planted paws.
    R = twist_helpers.apply(rig, R, twist_helpers.calibrate(gltf_rig.Rig(a.calib)))
    root, speed, planted = travel.root_travel(rig, R, T, frames_per_second=a.fps)
    out = os.path.join(a.take, 'capture.gltf')
    write_gltf.write(rig, R, T, root, out, True, a.fps)
    # The take's report for keying it in a .gcmove (MOVE-CAP1 E19): when each paw lands and which paws are down then, and the speed.
    # The speed is the whole take's travel over its time (the median step read 36.6 cm/s on a walk that goes 40 cm in its 1 s cycle).
    seconds = (len(R) - 1) / a.fps
    stance = travel.stance(rig, R, T, loop=a.loop)
    keys = travel.landings(stance, a.fps, a.loop)
    report = {'fps': a.fps, 'seconds': seconds, 'frames': len(R), 'loop': a.loop,
              'speed_cm_s': float(abs(root[-1][2] - root[0][2]) * 100.0 / seconds),
              'landings': [{'at': round(s, 4), 'frame': int(round(s * a.fps)), 'lands': paw, 'down': down} for s, paw, down in keys],
              # each paw's stance, the steps (frame f to f+1) it is down: what take_gcmove.py plants in a key at any frame
              'stance': {paw: [int(f) for f in np.nonzero(stance[:, k])[0]] for k, paw in enumerate(travel.PAWS)}}
    if len(keys) > 1:
        # travel(root, velocity=) that keeps the take's timing when keys at these landings are linked (the drawn steps over the time).
        report['travel_velocity_m_s'] = round(travel.key_speed(rig, R, T, keys, a.fps, a.loop)[0], 4)
    json.dump(report, open(os.path.join(a.take, 'take_report.json'), 'w'), indent=1)
    print('the take: %.3f s; each paw landing (down: about to lift, landing, under the body):' % seconds)
    for l in report['landings']: print('  at=frame(%d) (%.4f s)  %-8s lands; down: %s' % (l['frame'], l['at'], l['lands'], ', '.join(l['down'])))
    if 'travel_velocity_m_s' in report:
        print('keys at these landings keep the take\'s timing with travel(root, velocity=%.3fm/s)' % report['travel_velocity_m_s'])
    # The same take in place (the root at rest): the source a .gcmove keys it from (`pose(source=<clip>_InPlace, at=...)`), where the
    # body's travel is the travel line's (travel(root, velocity=<the report's speed>)), not a root shift inside each key (MOVE-CAP1 E19).
    in_place = os.path.join(a.take, 'capture_inplace.gltf')
    write_gltf.write(rig, R, T, np.repeat(rig.rest_t[rig.index('root')][None], len(R), 0), in_place, True, a.fps)
    # 6. Imported as clips (new names: an import over an existing clip stops on a modal question): the take walking, and in place.
    if a.clip:
        import run_take
        run_take.import_clip(os.path.abspath(out), a.clip, characters.ACTIVE.mesh, a.fps)
        if a.loop: run_take.play_like_the_source(a.clip, os.path.join(ROOT, 'Content'))
        run_take.import_clip(os.path.abspath(in_place), a.clip + '_InPlace', characters.ACTIVE.mesh, a.fps)
        print('imported', a.clip, 'and', a.clip + '_InPlace')


if __name__ == '__main__':
    main()
