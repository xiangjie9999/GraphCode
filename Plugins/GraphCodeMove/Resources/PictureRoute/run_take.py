# One take end to end: joint balls (obs.json) -> 3D (triangulate) -> bone turns (fit_rig) -> a glTF on the character's skeleton
# (write_gltf) -> an AnimSequence (gc_import, root motion and loop as the source clip has them). Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
# Usage: python run_take.py <take_dir> <template.gltf> <ikrig.gc> <calibration.gltf> <destination /Game/.../A_Name> [truth.gltf]
import json, os, re, subprocess, sys, time, urllib.request, urllib.error
import numpy as np
import characters, triangulate, fit_rig, gltf_rig

HEADERS = {'Content-Type': 'application/json', 'X-MCP-Client-Id': '81ad12d6-3837-4c25-80af-9b138f267a3d'}
SKELETON = characters.ACTIVE.skeleton     # the skeleton gc_import puts a clip on: characters.py


def tool(name, body, timeout=280):
    req = urllib.request.Request('http://localhost:3000/mcp/tool/' + name, data=json.dumps(body).encode(), headers=HEADERS)
    try: return json.load(urllib.request.urlopen(req, timeout=timeout))
    except urllib.error.HTTPError as e: return json.loads(e.read() or b'{}')


def import_clip(gltf_path, destination):
    # An existing destination makes Interchange ask "override asset?" in a modal that holds the editor's bridge until answered
    # (2026-10-18, twice): refuse it here and let the caller name a new clip.
    if os.path.exists(os.path.join(characters.ROOT, 'Content', destination[len('/Game/'):] + '.uasset')):
        raise RuntimeError('%s exists: import under a new name (an import over it stops on a modal question)' % destination)
    plan = tool('gc_import', {'source': gltf_path.replace('\\', '/'), 'destination': destination + '.' + destination.rsplit('/', 1)[1]})
    doc = (plan.get('data') or {}).get('document')
    if not doc: raise RuntimeError('import plan failed: ' + json.dumps(plan)[:400])
    doc = re.sub(r'bAssetTypeSubFolders=true, bSceneNameSubFolder=true', 'bAssetTypeSubFolders=false, bSceneNameSubFolder=false', doc)
    doc = doc.replace('InterchangeGenericCommonSkeletalMeshesAndAnimationsProperties(',
                      'InterchangeGenericCommonSkeletalMeshesAndAnimationsProperties(bImportOnlyAnimations=true, Skeleton=%s, ' % SKELETON)
    doc = doc.replace('InterchangeGenericMaterialPipeline(SearchLocation=DoNotSearch)', 'InterchangeGenericMaterialPipeline(SearchLocation=DoNotSearch, MaterialImport=DoNotImport)')
    doc = doc.replace('InterchangeGenericTexturePipeline(bAllowNonPowerOfTwo=true)', 'InterchangeGenericTexturePipeline(bAllowNonPowerOfTwo=true, bImportTextures=false)')
    r = tool('gc_import', {'document': doc})
    if not r.get('success'): raise RuntimeError('import failed: ' + json.dumps(r)[:600])
    return r


def play_like_the_source(destination, project_content):
    """Root motion and loop on, as the walk is built ([loop, root_motion]); the import does not carry them."""
    sidecar = os.path.join(project_content, destination[len('/Game/'):] + '.gc')
    for _ in range(30):                                   # the import writes the sidecar a moment after it returns
        if os.path.exists(sidecar): break
        time.sleep(1)
    s = open(sidecar, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    if 'bEnableRootMotion' not in s:
        s = s.replace('settings {' + nl, 'settings {' + nl + '  bEnableRootMotion = true' + nl + '  bLoop = true' + nl, 1)
        open(sidecar, 'w', encoding='utf-8', newline='').write(s)
    r = tool('gc_apply', {'asset_path': destination})
    if not r.get('success'): raise RuntimeError('apply failed: ' + json.dumps(r)[:400])


def main():
    take, template, ikrig, calib, destination = sys.argv[1:6]
    truth = sys.argv[6] if len(sys.argv) > 6 else None
    here = os.path.dirname(os.path.abspath(__file__))
    content = os.path.normpath(os.path.join(characters.ROOT, 'Content'))
    obs = json.load(open(os.path.join(take, 'obs.json')))
    points = triangulate.triangulate(obs)
    json.dump(points, open(os.path.join(take, 'points.json'), 'w'), indent=1)
    if truth:
        T = triangulate.truth_points(truth, points['bones'], points['frames'])
        err = [np.linalg.norm(np.array(p) - t) for b in points['bones'] for p, t in zip(points['points'][b], T[b]) if p is not None]
        print('E3 joints: mean %.3f cm, 95%% %.3f cm, max %.3f cm over %d joint-frames' % (np.mean(err), np.percentile(err, 95), max(err), len(err)))
    rig = gltf_rig.Rig(template)
    axes = fit_rig.ik_rig_axes(ikrig)
    orders = fit_rig.rig_orders(gltf_rig.Rig(calib), axes, rig.names)
    R, Tt, res = fit_rig.run(points, rig, axes, orders, fit_rig.ik_rig_limits(ikrig))
    allres = [x for row in res for x in row]
    print('fit: joint residual mean %.3f cm, max %.3f cm' % (np.mean(allres), max(allres)))
    fitted = os.path.join(take, 'fit.json')
    json.dump({'names': rig.names, 'rotations': R.tolist(), 'translations': Tt.tolist(), 'residual_cm': res, 'orders': orders}, open(fitted, 'w'))
    if truth:
        tr = gltf_rig.Rig(truth)
        rows = []
        for i, name in enumerate(rig.names):
            if name in ('#0', 'root') or 'twist' in name: continue
            e = [np.degrees(2 * np.arccos(min(1.0, abs(float(np.dot(R[f, i], tr.anim_r[f, i])))))) for f in range(len(R))]
            rows.append((max(e), name, np.mean(e)))
        rows.sort(reverse=True)
        print('E4 bone turns: median of each bone\'s worst %.2f deg; worst bones: %s' % (np.median([r[0] for r in rows]), ', '.join('%s %.1f (mean %.1f)' % (n, w, m) for w, n, m in rows[:6])))
    out_gltf = os.path.join(take, 'capture.gltf')
    subprocess.check_call([sys.executable, os.path.join(here, 'write_gltf.py'), template, fitted, out_gltf, calib])
    import_clip(os.path.abspath(out_gltf), destination)
    play_like_the_source(destination, content)
    print('imported', destination)


if __name__ == '__main__':
    main()
