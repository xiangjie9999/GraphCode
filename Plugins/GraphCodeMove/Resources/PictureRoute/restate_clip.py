# A clip made on an older build of the rig, said again on the rig as it is now: the rig fitted to the clip's own joint places frame by
# frame (only the turns the IK rig allows), twist helpers from their drivers, the root's track kept, written and imported as a new clip.
# Found 2026-10-26: the cat's rest changed on 10-20, and the 10-18 walk's keys then twisted its spine 33 degrees about its length (the
# abdomen turned back the same, the sternum 162), turns today's rig locks; a test whose truth the rig cannot say measures the clip, not
# the capture. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E14).
# Usage: python restate_clip.py <clip now.gltf (rig_now.py)> <rig.gltf> <ikrig.gc> <calibration.gltf> <out.gltf> [/Game/.../A_Name]
import sys
import numpy as np
import capture, characters, fit_rig, gltf_rig, run_take, twist_helpers, write_gltf


def main():
    clip_path, rig_path, ikrig, calib, out = sys.argv[1:6]
    clip, rig = gltf_rig.Rig(clip_path), gltf_rig.Rig(rig_path)
    axes = fit_rig.ik_rig_axes(ikrig)
    model = fit_rig.Model(rig, axes, fit_rig.rig_orders(gltf_rig.Rig(calib), axes, rig.names), fit_rig.ik_rig_limits(ikrig))
    root = clip.index('root')
    p = np.zeros(model.size); p[:3] = rig.rest_t[model.pelvis]; previous = None
    R, T, worst = [], [], []
    for f in range(len(clip.anim_r)):
        t = clip.anim_t[f].copy(); t[root] = clip.rest_t[root]
        _, P = clip.globals(t, clip.anim_r[f])
        seen = {b: P[clip.index(b)] for b in capture.BONES}
        p, err = model.solve(seen, p, previous); previous = p
        Rl, tl = model.locals(p)
        R.append([gltf_rig.matrix_to_quat(r) for r in Rl]); T.append(tl); worst.append(max(err))
    R = twist_helpers.apply(rig, np.array(R), twist_helpers.calibrate(gltf_rig.Rig(calib)))
    write_gltf.write(rig, R, np.array(T), clip.anim_t[:, root], out, True)
    print('fitted to the clip\'s joints: worst joint per frame mean %.3f cm, max %.3f cm' % (np.mean(worst), max(worst)))
    if len(sys.argv) > 6:
        run_take.import_clip(__import__('os').path.abspath(out), sys.argv[6], characters.ACTIVE.mesh, run_take.gltf_rate(out))
        print('imported', sys.argv[6])


if __name__ == '__main__':
    main()
