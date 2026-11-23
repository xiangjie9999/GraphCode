# How well a clip's paws hold the floor while they are down (MOVE-CAP1 E19: what GraphCode's plants add to a take read off video): each
# clip read through the editor's own pose reader (rig_now.read_clip, the root's travel included), and per paw, over every run of frames
# where its toe is within `band` of its own lowest, how far it slides along the floor (the run's horizontal spread) and how low it goes
# (its lowest height; against the true walk's, a paw that sinks or floats). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E19).
# Usage: python contact_quality.py <out_dir> <frames> <clip> [<clip> ...]
import json, os, sys
import numpy as np
import capture, rig_now

PAWS = ['toe_F_L', 'toe_F_R', 'toe_B_L', 'toe_B_R']


def measure(dump, near=3.0, least=3):
    """{paw: (worst slide cm, median slide cm, lowest height cm, frames down)} from a rig_now dump (component space, cm, the root's travel
    included). A paw is down where, seen in place (less the root's travel), it moves back with the body within `near` of its lowest, in runs
    of `least` frames (travel.stance's test, which holds on the take and on the truth alike); its slide is how far the toe moves over the
    floor in that run. "Toe within 1 cm of its lowest" read 12 cm slides on the true walk: it counted the toe sweeping low through swing."""
    out = {}
    root = np.array([f['root']['t'] for f in dump['component']])
    for paw in PAWS:
        P = np.array([f[paw]['t'] for f in dump['component']])
        Q = P - root                                               # in place
        step = np.diff(Q[:, 1])
        down = np.zeros(len(P), bool)
        down[:-1] = (step < -0.3 * np.median(np.abs(step))) & (Q[:-1, 2] <= Q[:, 2].min() + near)
        low = P[:, 2].min()
        runs, f = [], 0
        while f < len(down):
            if down[f]:
                g = f
                while g < len(down) and down[g]: g += 1
                if g - f >= least: runs.append((f, g))
                f = g
            else: f += 1
        slides = [float(np.ptp(P[a:b, 0]) ** 2 + np.ptp(P[a:b, 1]) ** 2) ** 0.5 for a, b in runs]
        out[paw] = (max(slides) if slides else float('nan'), float(np.median(slides)) if slides else float('nan'), float(low),
                    int(sum(b - a for a, b in runs)))
    return out


def main():
    out, frames, clips = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
    os.makedirs(out, exist_ok=True)
    rows = {}
    for clip in clips:
        dump = rig_now.read_clip(clip, capture.MESH, frames, os.path.join(out, clip.rsplit('/', 1)[1] + '.pose.json'))
        rows[clip] = measure(dump)
        print('== %s (%.3f s)' % (clip, dump['length']))
        for paw, (worst, median, low, n) in rows[clip].items():
            print('  %-8s slides %.2f cm at most, %.2f median, while down (%d frames); lowest %.2f cm' % (paw, worst, median, n, low))
    json.dump(rows, open(os.path.join(out, 'contact_quality.json'), 'w'), indent=1)


if __name__ == '__main__':
    main()
