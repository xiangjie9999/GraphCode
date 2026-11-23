# A starting .gcmove for a take read off video (MOVE-CAP1 E19): whole-body keys taken from the take in place at chosen moments
# (`pose(source=<take>_InPlace, at=frame(n), from=reference)`), the paws down at each moment planted inside it, linked by pose(steps) and
# timed by travel(root, velocity=) at the speed that keeps the take's own timing - the shipped travel walk's shape
# (Intent/CatWalkTravel.gcmove). Every number comes from the take's report (dino_take.py: take_report.json); none is typed. The moments:
# `landings` (one key where each paw lands: GraphCode draws the motion between) or `every:N` (a key every N frames: the take's own motion,
# its contacts held by GraphCode). Plants are written about-to-lift first and the paw that stays down longest LAST: the travel carries the
# body by the paw written last in the later key that is planted at both ends of a stretch (W30-W32).
# The paw names, their legs and contact regions are the active character's (characters.py: the cat's by default, `--character metahuman` its feet).
# Usage: python take_gcmove.py <take_dir> <take clip /Game/...> <out.gcmove> [landings|every:N] [name A_...]
import json, os, sys
import numpy as np
import characters, gltf_rig, travel

LEGS = characters.ACTIVE.legs     # planted toe -> (IK chain, roll contacts): the cat's on the British Shorthair rig (Intent/CatWalkTravel.gcmove), the MetaHuman's on KitchenCupBody
HEADER = characters.ACTIVE.header     # the .gcmove's first lines (the cat's skeleton and IK rig; the MetaHuman's character, part, IK rig, controls, physics asset)


def moments(report, how):
    """Key frames in time order, each with its paws down ordered about-to-lift first, longest-staying last."""
    n, loop = report['frames'] - (1 if report['loop'] else 0), report['loop']
    down = {paw: set(frames) for paw, frames in report['stance'].items()}
    def stays(paw, f):                                        # steps it stays down from f on
        g = 0
        while g < n and ((f + g) % n if loop else f + g) in down[paw]: g += 1
        return g
    if how == 'landings': frames = [l['frame'] for l in report['landings']]
    else: frames = list(range(0, n, int(how.split(':')[1])))
    out = []
    for f in sorted(set(frames)):
        paws = sorted([p for p in down if f in down[p]], key=lambda p: stays(p, f))
        out.append((f, paws))
    return out


def write(take_dir, clip, out, how='landings', name=None):
    report = json.load(open(os.path.join(take_dir, 'take_report.json')))
    keys = moments(report, how)
    bare = [k for k in keys if len(k[1]) == 0]
    if bare: raise SystemExit('no paw is down at frame(s) %s: a travel needs a planted paw in every key' % [k[0] for k in bare])
    # The travel's speed for THESE keys: their drawn steps over the time they span (travel.key_speed on the take in place).
    rig = gltf_rig.Rig(os.path.join(take_dir, 'capture_inplace.gltf'))
    fps = report['fps']
    triples = [(f / fps, paws[-1], paws) for f, paws in keys]
    velocity, distance, seconds = travel.key_speed(rig, rig.anim_r, rig.anim_t, triples, fps, report['loop'])
    name = name or 'A_' + os.path.splitext(os.path.basename(out))[0]
    into = clip.rsplit('/', 1)[0]
    lines = [HEADER + '         into:%s fps:%d' % (into, round(fps)), 'build {',
             '  %s = $Walk [%sroot_motion, readings]' % (name, 'loop, ' if report['loop'] else ''), '}',
             '# Written by Tools/VideoCapture/take_gcmove.py from %s (MOVE-CAP1 E19): %d keys of the take (%s), each a moment of it in place,' % (
                 os.path.join(take_dir, 'take_report.json').replace('\\', '/'), len(keys), how),
             '# the paws down then planted (about to lift first, the one staying down longest last), timed by the travel that keeps the take\'s',
             '# timing (%.1f cm over %.3f s). Edit freely: it is an ordinary .gcmove.' % (distance * 100, seconds),
             'Floor = ground(z=0)']
    for k, (f, paws) in enumerate(keys, 1):
        lines.append('K%d = pose(source=%s, at=frame(%d), from=reference) {' % (k, clip, f))
        for p in paws: lines.append('  plant(%s, on=$Floor, roll=(%s))' % LEGS[p])
        lines.append('}')
    steps = ', '.join('$K%d' % k for k in range(1, len(keys) + 1)) + (', $K1' if report['loop'] else '')
    lines += ['Steps = pose(steps) { %s }' % steps, 'Walk = clip() {', '  $Steps [clock=root]', '  travel(root, velocity=%.3fm/s)' % velocity, '}', '']
    open(out, 'w', newline='\n').write('\n'.join(lines))
    print('wrote %s: %d keys (%s), travel(root, velocity=%.3fm/s)' % (out, len(keys), how, velocity))


if __name__ == '__main__':
    write(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else 'landings', sys.argv[5] if len(sys.argv) > 5 else None)
