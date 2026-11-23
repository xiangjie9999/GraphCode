# Joint heights and the trunk's pitch at chosen frames of a glTF export (Unreal cm). Usage: python body_heights.py <export.gltf> [frame ...]
import sys
import numpy as np
import gltf_rig
rig = gltf_rig.Rig(sys.argv[1])
frames = [None] + [int(a) for a in sys.argv[2:]]
show = ['pelvis', 'hip_L', 'shoulder_L', 'elbow_L', 'scapula_border_L', 'chest', 'neck', 'head', 'hip_R', 'shoulder_R']
for f in frames:
    R, P = rig.frame(f)
    U = gltf_rig.to_unreal(P)
    at = {n: U[rig.index(n)] for n in show}
    hip = (at['hip_L'] + at['hip_R']) / 2; sh = (at['shoulder_L'] + at['shoulder_R']) / 2
    pitch = np.degrees(np.arctan2(sh[2] - hip[2], sh[1] - hip[1]))
    print('%-6s ' % ('rest' if f is None else f) + '  '.join('%s %.1f' % (n, at[n][2]) for n in show) + '  | hip-shoulder line %+.1f deg (nose up +)' % pitch)
