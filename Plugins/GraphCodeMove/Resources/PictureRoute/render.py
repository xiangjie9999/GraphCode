# Renders frames of an animation over the editor's HTTP bridge (gc_render), with or without one bone drawn. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
import base64, io, json, time, urllib.request, urllib.error
from PIL import Image
URL = 'http://localhost:3000/mcp/tool/gc_render'
HEADERS = {'Content-Type': 'application/json', 'X-MCP-Client-Id': '81ad12d6-3837-4c25-80af-9b138f267a3d'}


def call(body, timeout=240):
    req = urllib.request.Request(URL, data=json.dumps(body).encode(), headers=HEADERS)
    for attempt in range(5):
        try:
            return json.load(urllib.request.urlopen(req, timeout=timeout))
        except urllib.error.HTTPError as e:
            r = json.loads(e.read() or b'{}')
            if 'busy' in json.dumps(r).lower() or 'queued' in json.dumps(r).lower(): time.sleep(3); continue
            raise RuntimeError(json.dumps(r)[:400])
        except urllib.error.URLError:
            time.sleep(3)
    raise RuntimeError('editor unreachable')


def frames(asset, camera, times, width, height, bones=None, view_mode='unlit'):
    """One image per time; bones=[name] draws that bone's joint ball and ray over the mesh. Returns PIL images in order."""
    images = []
    for start in range(0, len(times), 8):
        chunk = times[start:start + 8]
        views = [{'name': 'f%03d' % (start + k), 'width': width, 'height': height, 'camera': camera, 'at': t, 'to': t, 'count': 1}
                 for k, t in enumerate(chunk)]
        body = {'asset': asset, 'views': views, 'view_mode': view_mode}
        if bones: body['highlights'] = [{'type': 'bones', 'of': bones, 'draw': ['ray']}]
        r = call(body)
        paths = [v['path'] for v in (r.get('data') or {}).get('views', [])] if r.get('data') else []
        if not paths:
            manifest = json.loads(json.dumps(r)).get('data', {})
        if paths:
            images += [Image.open(p).convert('RGB') for p in paths]
        elif r.get('base64') and len(chunk) == 1:
            images.append(Image.open(io.BytesIO(base64.b64decode(r['base64']))).convert('RGB'))
        else:
            raise RuntimeError('no image paths in the render reply: ' + json.dumps(r)[:300])
    return images
