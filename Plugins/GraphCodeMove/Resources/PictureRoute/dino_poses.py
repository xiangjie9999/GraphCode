# Clips of random poses on the character's own rig, to teach the joint reader (dino_reader.py) poses no authored clip holds: each free
# axis the IK rig leaves moves on a few slow sines round the rest pose, within the rig's limits and at most `cap` degrees, the pelvis a
# little; twist helpers from their drivers; the root at rest (played in place). Written as glTF on the character's export and
# imported onto its skeleton (run_take.import_clip). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E14).
# Usage: python dino_poses.py <rig.gltf> <ikrig.gc> <calibration.gltf> <out_dir> <destination folder /Game/...> <count> [seed] [name] [--authored <asset> ...]
#        --authored mixes real poses in: the named authored clips (/Game/... paths) join the random ones in <out_dir>/assets.txt, which
#        `dino_data.py train <out> <times> @<out_dir>/assets.txt` reads, so the reader is taught on poses the character really takes.
#        --character <name> (characters.py): the profile's name and caps; a person's pelvis and spine turn little in random poses.
#        (the rig and calibration as the skeleton is NOW: rig_now.py; an import over an existing clip stops on a modal question)
import os, sys
import numpy as np
import characters, fit_rig, gltf_rig, twist_helpers, write_gltf, run_take


def random_params(model, frames, rng, cap=np.radians(45), pelvis_cm=2.0, caps=None):
    """Per frame, the model's parameters: three to five sines per axis, random phases, amplitudes within the axis's limit. `caps`: a joint's
    own cap in degrees by its bone's name (characters.py `pose_caps`: a biped's pelvis and spine stay upright-ish; none for the cat)."""
    owner = [None] * 3 + [model.rig.names[i] for i, order in model.joints for _ in order]
    t = np.linspace(0, 1, frames)
    P = np.zeros((frames, model.size))
    P[:, :3] = model.rig.rest_t[model.pelvis]
    for k in range(model.size):
        if k < 3:
            amp = pelvis_cm / 100.0
        else:
            own = np.radians(caps[owner[k]]) if caps and owner[k] in caps else cap
            amp = min(own, model.upper[k] if np.isfinite(model.upper[k]) else own)
        waves = rng.integers(3, 6)
        x = sum(rng.uniform(-1, 1) * np.sin(2 * np.pi * rng.uniform(0.3, 2.0) * t + rng.uniform(0, 2 * np.pi)) for _ in range(waves))
        P[:, k] += amp * x / max(1.0, np.abs(x).max())
    return P


def main():
    authored = []
    if '--authored' in sys.argv:
        k = sys.argv.index('--authored'); end = next((j for j in range(k + 1, len(sys.argv)) if sys.argv[j].startswith('--')), len(sys.argv))
        authored = sys.argv[k + 1:end]; del sys.argv[k:end]
    rig_path, ikrig, calib, out, folder, count = sys.argv[1:7]
    rng = np.random.default_rng(int(sys.argv[7]) if len(sys.argv) > 7 else 14)
    rig = gltf_rig.Rig(rig_path); axes = fit_rig.ik_rig_axes(ikrig)
    model = fit_rig.Model(rig, axes, fit_rig.rig_orders(gltf_rig.Rig(calib), axes, rig.names), fit_rig.ik_rig_limits(ikrig))
    shares = twist_helpers.calibrate(gltf_rig.Rig(calib))
    frames = len(rig.anim_r); root = rig.index('root')
    os.makedirs(out, exist_ok=True)
    made = []
    for k in range(int(count)):
        P = random_params(model, frames, rng, caps=characters.ACTIVE.pose_caps)
        R = np.zeros((frames, len(rig.names), 4)); T = np.repeat(rig.rest_t[None], frames, 0)
        for f in range(frames):
            Rl, t = model.locals(P[f])
            R[f] = [gltf_rig.matrix_to_quat(r) for r in Rl]; T[f] = t
        R = twist_helpers.apply(rig, R, shares)
        name = '%s%02d' % (sys.argv[8] if len(sys.argv) > 8 else characters.ACTIVE.random_prefix, k)
        path = os.path.join(out, name + '.gltf')
        write_gltf.write(rig, R, T, np.repeat(rig.rest_t[root][None], frames, 0), path)
        run_take.import_clip(os.path.abspath(path), folder.rstrip('/') + '/' + name, characters.ACTIVE.mesh, run_take.gltf_rate(path))
        print('imported', folder.rstrip('/') + '/' + name, flush=True)
        made.append(folder.rstrip('/') + '/' + name)
    open(os.path.join(out, 'assets.txt'), 'w').write('\n'.join(made + authored) + '\n')
    print('%d random + %d authored clips listed in %s' % (len(made), len(authored), os.path.join(out, 'assets.txt')))


if __name__ == '__main__':
    main()
