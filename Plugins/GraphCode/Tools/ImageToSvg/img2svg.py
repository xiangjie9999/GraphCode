#!/usr/bin/env python3
"""Generated image (ChatGPT and the like) -> a clean SVG of closed, filled, named shapes.

The output suits GraphCode's SVG route (Epic's SVGImporter -> USVGData -> a shape by its id):
one <path> per shape with an `id`, absolute coordinates, no transforms, groups, strokes or
gradients, and every hole wound opposite its outline so the non-zero fill rule cuts it. A JSON
report and a preview PNG say how faithful the trace is, so an author or agent can check the
result without opening an editor.

    python img2svg.py logo.png                       # logo.svg, logo.svg.json, logo.preview.png
    python img2svg.py logo.png --mode silhouette     # one shape: everything that is not background
    python img2svg.py crest.png --colors 3 --names shield,ring,star

Pipeline: background -> edge-preserving denoise -> upscale -> Lab k-means -> drop speckle and
anti-aliasing fringe -> smooth region edges -> sub-pixel outline of each region -> corners, then
the fewest lines and cubics within --tolerance -> fix winding and ids -> re-rasterise and score
against the cleaned regions. A circle is three curves and a rectangle four lines.
"""
import argparse
import glob
import itertools
import json
import math
import re
import sys
from pathlib import Path

import cv2
import numpy as np

try:
    import vtracer
except ImportError:
    vtracer = None

# sigma: edge smoothing; min_area: smallest region kept (px^2); min_width: thinner regions are
# anti-aliasing fringe; corner: a sharper turn (degrees) is a corner; tolerance: how far a curve
# may stray from the traced edge. Every length is in source pixels.
DETAIL = {
    'low':    dict(sigma=1.6, min_area=64, min_width=3.0, corner=40, tolerance=6.0),
    'medium': dict(sigma=1.0, min_area=24, min_width=2.0, corner=35, tolerance=3.0),
    'high':   dict(sigma=0.6, min_area=8, min_width=1.0, corner=30, tolerance=1.0),
}


# ---- colour -------------------------------------------------------------------------------

def to_lab(bgr):
    img = np.multiply(bgr, np.float32(1 / 255), dtype=np.float32)
    return cv2.cvtColor(img, cv2.COLOR_BGR2Lab, dst=img)


def lab_to_hex(lab):
    b, g, r = (np.clip(cv2.cvtColor(np.float32(lab).reshape(1, 1, 3), cv2.COLOR_Lab2BGR)[0, 0], 0, 1)
               * 255).round().astype(int)
    return f'#{r:02x}{g:02x}{b:02x}'


def hex_to_lab(text):
    m = re.fullmatch(r'#?([0-9a-fA-F]{6})', text.strip())
    if not m:
        raise SystemExit(f'not a #rrggbb colour: {text}')
    r, g, b = (int(m[1][i:i + 2], 16) for i in (0, 2, 4))
    return to_lab(np.uint8([[[b, g, r]]]))[0, 0]


def colour_name(lab):
    L, a, b = (float(v) for v in lab)
    chroma, hue = math.hypot(a, b), math.degrees(math.atan2(b, a)) % 360
    if chroma < 12:
        return 'black' if L < 25 else 'white' if L > 88 else 'gray'
    for limit, name in ((20, 'pink' if L > 65 else 'red'), (55, 'red'), (85, 'brown' if L < 50 else 'orange'),
                        (115, 'yellow'), (170, 'green'), (230, 'cyan'), (315, 'blue'), (345, 'purple')):
        if hue < limit:
            return name
    return 'pink' if L > 65 else 'red'


def kmeans(samples, k):
    k = max(1, min(k, len(samples)))
    cv2.setRNGSeed(0)
    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 50, 0.1)
    _, labels, centres = cv2.kmeans(np.float32(samples), k, None, criteria, 3, cv2.KMEANS_PP_CENTERS)
    return merge_centres(centres, np.bincount(labels.ravel(), minlength=k).astype(float), 0)


def merge_centres(centres, counts, delta_e):
    """Fold together centres closer than delta_e, nearest pair first, weighted by pixel count."""
    centres, counts = [np.float32(c) for c in centres], list(counts)
    while len(centres) > 1:
        d, i, j = min((np.linalg.norm(centres[i] - centres[j]), i, j)
                      for i in range(len(centres)) for j in range(i + 1, len(centres)))
        if d >= delta_e:
            break
        total = counts[i] + counts[j]
        centres[i] = (centres[i] * counts[i] + centres[j] * counts[j]) / total
        counts[i] = total
        del centres[j], counts[j]
    keep = [i for i, c in enumerate(counts) if c > 0]
    return np.float32([centres[i] for i in keep]), np.float64([counts[i] for i in keep])


def blends(centres, counts, ends, tolerance, limit=0.02):
    """Small colours lying between two bigger ones (or a background colour): the in-between
    shades anti-aliasing paints along an edge rather than a colour of their own. A thin line is
    mostly edge, so line mode counts a colour as a blend however much of it there is."""
    share = counts / counts.sum()
    out = []
    for i, c in enumerate(centres):
        if share[i] >= limit:
            continue
        others = [e for j, e in enumerate(centres) if share[j] > share[i]] + list(ends)
        for a, b in itertools.combinations(others, 2):
            ab = b - a
            t = np.clip(np.dot(c - a, ab) / max(float(np.dot(ab, ab)), 1e-6), 0.1, 0.9)
            if np.linalg.norm(a + t * ab - c) < tolerance:
                out.append(i)
                break
    return out


def sqdist(lab, c):
    d = lab - np.float32(c)
    return np.einsum('...k,...k->...', d, d)


def nearest(lab, centres):
    """Index of the nearest centre for every pixel, and the distance to it. Squared distances come
    from one matrix product per block of pixels (|x|^2 - 2 x.c + |c|^2), not a pass per centre."""
    flat = np.ascontiguousarray(lab, np.float32).reshape(-1, 3)
    c = np.float32(centres).reshape(-1, 3)
    cc = (c * c).sum(1)
    index = np.empty(len(flat), np.int32)
    best = np.empty(len(flat), np.float32)
    for i in range(0, len(flat), 1 << 18):
        x = flat[i:i + (1 << 18)]
        d = cc[None] - 2 * (x @ c.T)
        j = d.argmin(1)
        index[i:i + len(x)] = j
        best[i:i + len(x)] = d[np.arange(len(x)), j] + np.einsum('ij,ij->i', x, x)
    shape = lab.shape[:2]
    return index.reshape(shape), np.sqrt(np.maximum(best, 0)).reshape(shape)


# ---- regions ------------------------------------------------------------------------------

def find_background(lab, alpha, args, warnings):
    """Confident background pixels (or None) and the colours they were matched against."""
    if args.bg == 'none':
        return None, []
    if alpha is not None and args.bg == 'auto':
        return alpha < 0.5, ['transparent']
    connected, checker = args.bg_connected, False
    if args.bg != 'auto':
        colours = [hex_to_lab(c) for c in args.bg.split(',')]
    else:
        h, w = lab.shape[:2]
        b = max(2, min(h, w) // 100)
        ring = np.concatenate([lab[:b].reshape(-1, 3), lab[-b:].reshape(-1, 3),
                               lab[:, :b].reshape(-1, 3), lab[:, -b:].reshape(-1, 3)])
        centres, counts = kmeans(ring, 4)
        centres, counts = merge_centres(centres, counts, 8)
        order = np.argsort(-counts)
        share = counts[order] / counts.sum()
        colours = [centres[order[0]]]
        if len(order) > 1 and share[0] + share[1] > 0.8 and share[1] > 0.2 and \
                all(np.hypot(*centres[i][1:]) < 8 and centres[i][0] > 50 for i in order[:2]):
            colours.append(centres[order[1]])
            connected = checker = True
            warnings.append('border is a two-tone light checkerboard (fake transparency): it is background '
                            'where it reaches the border or shows both squares; plain enclosed areas stay shapes')
        elif share[0] < 0.5:
            warnings.append('no dominant border colour, so nothing is background; pass --bg #rrggbb')
            return None, []
    distances = np.stack([sqdist(lab, c) for c in colours])
    mask = distances.min(0) < args.bg_tolerance ** 2
    if connected:
        n, comp = cv2.connectedComponents(mask.astype(np.uint8), connectivity=4)
        keep = np.zeros(n, bool)
        keep[np.unique(np.concatenate([comp[0], comp[-1], comp[:, 0], comp[:, -1]]))] = True
        if checker:  # checkerboard showing through a hole: an enclosed area made of both squares
            total = np.bincount(comp[mask], minlength=n)
            first = np.bincount(comp[mask & (distances.argmin(0) == 0)], minlength=n)
            keep |= (first > 0.15 * total) & (first < 0.85 * total)
        keep[0] = False
        mask = keep[comp]
    return mask, [lab_to_hex(c) for c in colours]


def drop_fragments(labels, count, min_area, min_width):
    """Mark as unknown (-1) every region too small or too thin to be a shape: speckle, and the
    in-between colours that anti-aliasing leaves along every edge."""
    out = labels.copy()
    for k in range(count):
        whole = (labels == k).astype(np.uint8)
        x, y, w, h = cv2.boundingRect(whole)
        if w == 0:
            continue
        box = (slice(max(y - 1, 0), y + h + 1), slice(max(x - 1, 0), x + w + 1))
        m = whole[box]
        n, comp, stats, _ = cv2.connectedComponentsWithStats(m, connectivity=8)
        deep = cv2.distanceTransform(m, cv2.DIST_L2, 3) * 2 >= min_width  # a region needs one such pixel
        thick = np.bincount(comp[deep], minlength=n) > 0
        bad = (stats[:, cv2.CC_STAT_AREA] < min_area) | ~thick
        bad[0] = False
        out[box][bad[comp] & (m > 0)] = -1
    return out


def fill_unknown(labels, fallback, reach=12):
    """Give each unknown pixel the label of the nearest known pixel, in one distance-transform
    pass; one further than `reach` takes its nearest-colour label instead."""
    unknown = labels < 0
    if not unknown.any():
        return labels
    known = np.flatnonzero(~unknown.ravel())
    if not len(known):
        return fallback.copy()
    # DIST_LABEL_PIXEL numbers the zero (known) pixels in raster order
    dist, near = cv2.distanceTransformWithLabels(unknown.astype(np.uint8), cv2.DIST_L2, 5,
                                                 labelType=cv2.DIST_LABEL_PIXEL)
    out = labels.copy()
    out[unknown] = labels.ravel()[known[near[unknown] - 1]]
    far = unknown & (dist > reach)
    out[far] = fallback[far]
    return out


def smooth_labels(labels, count, sigma):
    """Per-label Gaussian vote: rounds pixel staircases and nibbles one-pixel bumps."""
    if sigma <= 0:
        return labels
    best = np.full(labels.shape, -1, np.float32)
    out = labels.copy()
    r = int(np.ceil(3 * sigma)) + 1
    for k in range(count):
        whole = (labels == k).astype(np.uint8)
        x, y, w, h = cv2.boundingRect(whole)
        if w == 0:
            continue
        box = (slice(max(y - r, 0), y + h + r), slice(max(x - r, 0), x + w + r))
        vote = cv2.GaussianBlur(whole[box].astype(np.float32), (0, 0), sigma)
        take = vote > best[box]
        best[box][take] = vote[take]
        out[box][take] = k
    return out


def fill_enclosed(labels):
    """Hand each background area that does not reach the border to the colour around it."""
    mask = (labels == 0).astype(np.uint8)
    n, comp, stats, _ = cv2.connectedComponentsWithStats(mask, connectivity=4)
    edge = set(np.unique(np.concatenate([comp[0], comp[-1], comp[:, 0], comp[:, -1]])).tolist())
    out = labels.copy()
    for c in range(1, n):
        if c in edge:
            continue
        x, y, w, h = stats[c, :4]
        box = (slice(y - 1, y + h + 1), slice(x - 1, x + w + 1))
        m = comp[box] == c
        ring = cv2.dilate(m.astype(np.uint8), np.ones((3, 3), np.uint8)).astype(bool) & ~m
        around = labels[box][ring]
        around = around[around > 0]
        if len(around):
            out[box][m] = np.bincount(around).argmax()
    return out


# ---- paths --------------------------------------------------------------------------------
# A subpath is (start, segments); a segment is ('L', end) or ('C', c1, c2, end); points are
# (x, y) tuples. Every subpath is closed.

def parse_path(d):
    tokens = re.findall(r'[MmLlHhVvCcQqZz]|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?', d)
    if re.search(r'[SsTtAa]', d):
        raise ValueError('path uses S/T/A commands, which this parser does not read')
    subpaths, segs, start, cur, i, cmd = [], None, (0.0, 0.0), (0.0, 0.0), 0, None

    def num():
        nonlocal i
        i += 1
        return float(tokens[i - 1])

    def point(rel):
        x, y = num(), num()
        return (cur[0] + x, cur[1] + y) if rel else (x, y)

    while i < len(tokens):
        if re.fullmatch(r'[A-Za-z]', tokens[i]):
            cmd = tokens[i]
            i += 1
        rel, op = cmd.islower(), cmd.upper()
        if op == 'Z':
            if segs is not None:
                subpaths.append((start, segs))
            segs, cur = None, start
            cmd = None if i >= len(tokens) or re.fullmatch(r'[A-Za-z]', tokens[i]) else 'L'
            continue
        if op == 'M':
            if segs:
                subpaths.append((start, segs))
            start = cur = point(rel)
            segs = []
            cmd = 'l' if rel else 'L'
        elif op == 'L':
            cur = point(rel)
            segs.append(('L', cur))
        elif op in 'HV':
            v = num()
            cur = ((cur[0] + v if rel else v), cur[1]) if op == 'H' else (cur[0], (cur[1] + v if rel else v))
            segs.append(('L', cur))
        elif op == 'C':
            c1, c2, end = point(rel), point(rel), point(rel)
            segs.append(('C', c1, c2, end))
            cur = end
        elif op == 'Q':
            q, end = point(rel), point(rel)
            segs.append(('C', tuple(cur[k] + 2 / 3 * (q[k] - cur[k]) for k in (0, 1)),
                         tuple(end[k] + 2 / 3 * (q[k] - end[k]) for k in (0, 1)), end))
            cur = end
    if segs:
        subpaths.append((start, segs))
    return [close(sp) for sp in subpaths if sp[1]]


def close(sp):
    start, segs = sp
    end = segs[-1][-1]
    if math.dist(start, end) > 1e-6:
        segs = segs + [('L', start)]
    return start, segs


def transform(sp, fn):
    start, segs = sp
    return fn(start), [(s[0],) + tuple(fn(p) for p in s[1:]) for s in segs]


def reverse(sp):
    start, segs = sp
    nodes = [start] + [s[-1] for s in segs]
    out = [('L', nodes[i]) if s[0] == 'L' else ('C', s[2], s[1], nodes[i])
           for i, s in reversed(list(enumerate(segs)))]
    return nodes[-1], out


def flatten(sp, spacing=1.0):
    """The subpath as a polygon, each curve cut into chords about `spacing` px long."""
    start, segs = sp
    pts, cur = [start], start
    for s in segs:
        if s[0] == 'L':
            pts.append(s[1])
        else:
            b = np.float64([cur, s[1], s[2], s[3]])
            steps = int(np.clip(np.linalg.norm(np.diff(b, axis=0), axis=1).sum() / spacing, 4, 400))
            pts.extend(map(tuple, bezier(b, np.linspace(0, 1, steps + 1)[1:])))
        cur = s[-1]
    return np.float64(pts)


def signed_area(poly):
    x, y = poly[:, 0], poly[:, 1]
    return 0.5 * float(np.dot(x, np.roll(y, -1)) - np.dot(y, np.roll(x, -1)))


def organise(subpaths, min_area):
    """Drop slivers, find each subpath's container, and wind even depths positive and odd depths
    negative, so a non-zero fill shows outlines and cuts holes. Returns [(subpath, depth, parent)]."""
    polys = [flatten(sp) for sp in subpaths]
    area = [signed_area(p) for p in polys]
    keep = [i for i in range(len(subpaths)) if abs(area[i]) >= min_area]
    box = {i: (*polys[i].min(0), *polys[i].max(0)) for i in keep}
    parent = {}
    for i in keep:
        probe = polys[i][[0, len(polys[i]) // 3, 2 * len(polys[i]) // 3]]
        containers = []
        for j in keep:
            if j == i or abs(area[j]) <= abs(area[i]) or not (box[j][0] <= box[i][0] and box[j][1] <= box[i][1]
                                                               and box[j][2] >= box[i][2] and box[j][3] >= box[i][3]):
                continue
            contour = polys[j].astype(np.float32)
            if sum(cv2.pointPolygonTest(contour, (float(x), float(y)), False) > 0 for x, y in probe) >= 2:
                containers.append(j)
        parent[i] = min(containers, key=lambda j: abs(area[j])) if containers else None

    def depth(i):
        return 0 if parent[i] is None else 1 + depth(parent[i])

    out = []
    for i in keep:
        dep = depth(i)
        sp = subpaths[i] if (area[i] > 0) == (dep % 2 == 0) else reverse(subpaths[i])
        out.append((sp, dep, parent[i], i))
    return out


def fmt(v):
    s = f'{v:.2f}'.rstrip('0').rstrip('.')
    return '0' if s in ('-0', '') else s


def path_d(subpaths, closed=None):
    parts = []
    for i, (start, segs) in enumerate(subpaths):
        parts.append(f'M{fmt(start[0])} {fmt(start[1])}')
        parts += [s[0] + ' '.join(f'{fmt(x)} {fmt(y)}' for x, y in s[1:]) for s in segs]
        if closed is None or closed[i]:
            parts.append('Z')
    return ' '.join(parts)


# ---- tracing ------------------------------------------------------------------------------

def outlines(mask, scale):
    """Dense closed outlines of mask's True pixels, in source pixels. The mask is softened,
    upsampled bilinearly and cut at one half, so the outline runs between pixels instead of
    along their staircase; a light blur along the outline takes out what is left."""
    ys, xs = np.nonzero(mask)
    pad = 3
    y0, x0 = ys.min(), xs.min()
    crop = mask[y0:ys.max() + 1, x0:xs.max() + 1]
    soft = np.zeros((crop.shape[0] + 2 * pad, crop.shape[1] + 2 * pad), np.float32)
    soft[pad:-pad, pad:-pad][crop] = 1
    soft = cv2.GaussianBlur(soft, (0, 0), 0.7)
    f = 4 if soft.size <= 2_000_000 else 2
    big = cv2.resize((soft * 255).astype(np.uint8), None, fx=f, fy=f, interpolation=cv2.INTER_LINEAR)
    contours, _ = cv2.findContours((big > 127).astype(np.uint8), cv2.RETR_CCOMP, cv2.CHAIN_APPROX_NONE)
    out, k = [], 2 * f
    for c in contours:
        pts = c.reshape(-1, 1, 2).astype(np.float32)
        if len(pts) < 8:
            continue
        if len(pts) > 2 * k:
            wrap = np.concatenate([pts[-k:], pts, pts[:k]])
            pts = cv2.GaussianBlur(wrap, (1, 2 * k + 1), 0, sigmaY=f / 2)[k:-k]
        out.append(((pts.reshape(-1, 2).astype(np.float64) + 0.5) / f + (x0 - pad, y0 - pad)) / scale)
    return out


def trace_mask(mask, tracer, knobs, scale):
    """Closed subpaths around the True pixels of mask, in source pixels."""
    if tracer == 'fit':
        return [fit_outline(p, knobs['tolerance'], knobs['corner']) for p in outlines(mask, scale)]
    if tracer == 'opencv':
        polys = [cv2.approxPolyDP(np.float32(p).reshape(-1, 1, 2), knobs['tolerance'], True).reshape(-1, 2)
                 for p in outlines(mask, scale)]
        return [close((tuple(q[0]), [('L', tuple(v)) for v in q[1:]])) for q in polys if len(q) >= 3]
    ys, xs = np.nonzero(mask)
    pad = 2
    y0, x0 = ys.min(), xs.min()
    crop = mask[y0:ys.max() + 1, x0:xs.max() + 1]
    image = np.full((crop.shape[0] + 2 * pad, crop.shape[1] + 2 * pad, 3), 255, np.uint8)
    image[pad:-pad, pad:-pad][crop] = 0
    svg = vtracer.convert_raw_image_to_svg(
        cv2.imencode('.png', image)[1].tobytes(), img_format='png', colormode='binary', mode='spline',
        filter_speckle=2, corner_threshold=60, length_threshold=4.0 * scale,
        splice_threshold=45, max_iterations=10, path_precision=3)
    subpaths = []
    for tag in re.findall(r'<path\b[^>]*>', svg):
        t = re.search(r'translate\(\s*([-\d.eE+]+)[\s,]+([-\d.eE+]+)\s*\)', tag)
        tx, ty = (float(t[1]) + x0 - pad, float(t[2]) + y0 - pad) if t else (x0 - pad, y0 - pad)
        for sp in parse_path(re.search(r'\bd="([^"]*)"', tag)[1]):
            subpaths.append(transform(sp, lambda q: ((q[0] + tx) / scale, (q[1] + ty) / scale)))
    return subpaths


# ---- fitting: few curves through a dense outline ---------------------------------------------
# Corners split the outline into runs; a straight run is one line; the rest is Schneider's
# least-squares cubic fit ("An Algorithm for Automatically Fitting Digitized Curves", Graphics
# Gems, 1990): fit one cubic with fixed end tangents, refine the parameters by Newton-Raphson,
# and split at the worst point only if it is still further than the tolerance.

def bezier(b, t):
    t = t[:, None]
    u = 1 - t
    return u ** 3 * b[0] + 3 * u * u * t * b[1] + 3 * u * t * t * b[2] + t ** 3 * b[3]


def resample(p, step):
    q = np.vstack([p, p[:1]])
    d = np.r_[0, np.cumsum(np.linalg.norm(np.diff(q, axis=0), axis=1))]
    t = np.linspace(0, d[-1], max(8, int(d[-1] / step)), endpoint=False)
    return np.column_stack([np.interp(t, d, q[:, 0]), np.interp(t, d, q[:, 1])])


def turning(p, k):
    v1, v2 = p - np.roll(p, k, 0), np.roll(p, -k, 0) - p
    cos = (v1 * v2).sum(1) / (np.linalg.norm(v1, axis=1) * np.linalg.norm(v2, axis=1) + 1e-12)
    return np.degrees(np.arccos(np.clip(cos, -1, 1)))


def find_corners(p, step, angle, window, closed=True):
    """Indices where the outline turns by more than angle within window px either side, and
    still turns sharply at half that window, which a tight smooth curve does not."""
    k = max(2, int(round(window / step)))
    if len(p) < 4 * k:
        return []
    wide, narrow = turning(p, k), turning(p, max(1, k // 2))
    if not closed:  # an open line has no turn at its ends
        wide[:k] = wide[-k:] = 0
    gap = (lambda i, j: min(abs(i - j), len(p) - abs(i - j))) if closed else (lambda i, j: abs(i - j))
    found = []
    for i in np.argsort(-wide):
        if wide[i] <= angle:
            break
        if narrow[i] > 0.6 * angle and all(gap(i, j) > k for j in found):
            found.append(int(i))
    return sorted(found)


def unit(v):
    n = np.linalg.norm(v)
    return v / n if n > 1e-12 else v


def end_tangent(run, reach):
    """Direction from run[0] into the run, measured over `reach` px so a rounded corner does not bend it."""
    d = np.cumsum(np.linalg.norm(np.diff(run, axis=0), axis=1))
    j = int(np.searchsorted(d, min(reach, d[-1] / 3))) + 1
    return unit(run[min(j, len(run) - 1)] - run[0])


def one_cubic(p, t1, t2):
    """The least-squares cubic from p[0] to p[-1] with fixed end directions (t2 points back from
    p[-1]), its parameters refined by Newton-Raphson. Returns (control points, worst miss in px)."""
    first, last = p[0], p[-1]
    chord = np.linalg.norm(last - first)
    d = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    if len(p) < 3 or d[-1] < 1e-9:
        b = np.array([first, first + t1 * chord / 3, last + t2 * chord / 3, last])
        return b, 0.0
    u = d / d[-1]
    best = None
    for _ in range(6):
        a1, a2 = t1[None] * (3 * (1 - u) ** 2 * u)[:, None], t2[None] * (3 * (1 - u) * u ** 2)[:, None]
        rest = p - bezier(np.array([first, first, last, last]), u)
        c00, c01, c11 = (a1 * a1).sum(), (a1 * a2).sum(), (a2 * a2).sum()
        x0, x1 = (a1 * rest).sum(), (a2 * rest).sum()
        det = c00 * c11 - c01 * c01
        al, ar = ((x0 * c11 - x1 * c01) / det, (c00 * x1 - c01 * x0) / det) if abs(det) > 1e-12 else (0, 0)
        if al < 1e-3 * chord or ar < 1e-3 * chord or max(al, ar) > 2 * d[-1]:
            al = ar = chord / 3
        b = np.array([first, first + t1 * al, last + t2 * ar, last])
        q = bezier(b, u) - p
        err = float(np.sqrt((q * q).sum(1).max()))
        if best is None or err < best[1]:
            best = (b, err)
        t = u[:, None]
        q1 = 3 * ((1 - t) ** 2 * (b[1] - b[0]) + 2 * (1 - t) * t * (b[2] - b[1]) + t ** 2 * (b[3] - b[2]))
        q2 = 6 * ((1 - t) * (b[2] - 2 * b[1] + b[0]) + t * (b[3] - 2 * b[2] + b[1]))
        den = (q1 * q1).sum(1) + (q * q2).sum(1)
        u = np.clip(u - (q * q1).sum(1) / np.where(np.abs(den) > 1e-12, den, np.inf), 0, 1)
        u = np.maximum.accumulate(u)
    return best


def fit_run(p, t_start, t_end, tol, tangent):
    """Cubics from p[0] to p[-1], each reaching as far along p as it can while staying within tol;
    neighbours share the tangent where they meet, so the joins are smooth."""
    out, i, t1, n = [], 0, t_start, len(p)

    def attempt(j):
        return one_cubic(p[i:j + 1], t1, t_end if j == n - 1 else -tangent(j))

    while i < n - 1:
        fit = attempt(n - 1)
        if fit[1] <= tol:
            out.append(fit[0])
            break
        good, bad, step = (i + 1, attempt(i + 1)), n - 1, 8
        while good[0] + step < bad:  # gallop out, then bisect, for the furthest end that fits
            trial = attempt(good[0] + step)
            if trial[1] > tol:
                bad = good[0] + step
                break
            good, step = (good[0] + step, trial), step * 2
        while bad - good[0] > 1:
            mid = (good[0] + bad) // 2
            trial = attempt(mid)
            good, bad = ((mid, trial), bad) if trial[1] <= tol else (good, mid)
        j, (b, _) = good
        out.append(b)
        t1, i = tangent(j), j
    return out


def straight(run, tol):
    a, b = run[0], run[-1]
    ab = b - a
    n = np.linalg.norm(ab)
    if n < 1e-9:
        return False
    return float(np.abs(ab[0] * (run[:, 1] - a[1]) - ab[1] * (run[:, 0] - a[0])).max() / n) <= tol


def fit_outline(p, tol, corner):
    """Closed subpath of lines and cubics within tol px of the outline p, with as few pieces as
    that tolerance allows. A small outline (a letter, a rivet) is held to 3% of its own size."""
    tol = min(tol, max(0.4, 0.03 * float(np.linalg.norm(p.max(0) - p.min(0)))))
    step = min(0.5, tol / 2)
    p = resample(p, step)
    n = len(p)
    reach = max(3.0, 2 * tol)
    corners = find_corners(p, step, corner, reach)
    m = max(1, int(reach / step))  # tangents read across +-reach px, so edge wobble does not tilt them
    if corners:
        p = np.roll(p, -corners[0], 0)
        cuts = [c - corners[0] for c in corners] + [n]
    else:
        cuts = [0, n]
    loop = np.vstack([p, p[:1]])
    segs = []
    for a, b in zip(cuts, cuts[1:]):
        run = loop[a:b + 1]
        tangent = lambda j, a=a: unit(p[(a + j + m) % n] - p[(a + j - m) % n])
        if corners:
            if len(run) < 4 or straight(run, tol):
                segs.append(('L', tuple(run[-1])))
                continue
            t1, t2 = end_tangent(run, reach), end_tangent(run[::-1], reach)
        else:  # a smooth loop starts and ends on the same tangent
            t1 = tangent(0)
            t2 = -t1
        segs += [('C', tuple(c[1]), tuple(c[2]), tuple(c[3])) for c in fit_run(run, t1, t2, tol, tangent)]
    return close((tuple(p[0]), segs))


def resample_open(p, step):
    d = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    t = np.linspace(0, d[-1], max(2, int(round(d[-1] / step)) + 1))
    return np.column_stack([np.interp(t, d, p[:, 0]), np.interp(t, d, p[:, 1])])


def fit_open(p, tol, corner):
    """Open subpath of lines and cubics within tol px of the polyline p, with as few pieces as
    that tolerance allows; the twin of fit_outline for a line that does not close."""
    tol = min(tol, max(0.4, 0.03 * float(np.linalg.norm(p.max(0) - p.min(0)))))
    step = min(0.5, tol / 2)
    p = resample_open(p, step)
    n = len(p)
    if n < 4 or straight(p, tol):
        return tuple(p[0]), [('L', tuple(p[-1]))]
    reach = max(3.0, 2 * tol)
    m = max(1, int(reach / step))
    cuts = [0] + find_corners(p, step, corner, reach, closed=False) + [n - 1]
    segs = []
    for a, b in zip(cuts, cuts[1:]):
        run = p[a:b + 1]
        if len(run) < 4 or straight(run, tol):
            segs.append(('L', tuple(run[-1])))
            continue
        tangent = lambda j, a=a: unit(p[min(a + j + m, n - 1)] - p[max(a + j - m, 0)])
        t1, t2 = end_tangent(run, reach), end_tangent(run[::-1], reach)
        segs += [('C', tuple(c[1]), tuple(c[2]), tuple(c[3])) for c in fit_run(run, t1, t2, tol, tangent)]
    return tuple(p[0]), segs


def rasterise(entries, shape, to_px):
    """Paint a shape's subpaths with the non-zero rule they were wound for: depths in order,
    even depths fill and odd depths cut."""
    canvas = np.zeros(shape, np.uint8)
    for sp, dep, *_ in sorted(entries, key=lambda e: e[1]):
        pts = np.round(to_px(flatten(sp)) * 16).astype(np.int32)
        cv2.fillPoly(canvas, [pts], 0 if dep % 2 else 1, lineType=cv2.LINE_8, shift=4)
    return canvas.astype(bool)


# ---- lines: the middle of each stroke ------------------------------------------------------
# A stroke is thinned to a one-pixel skeleton (Zhang and Suen, "A Fast Parallel Algorithm for
# Thinning Digital Patterns", CACM 1984), cut at its ends and junctions into chains, tidied, and
# each chain becomes one open (or closed) path with the stroke's width.

STEPS = ((0, 1), (1, 0), (0, -1), (-1, 0), (1, 1), (1, -1), (-1, 1), (-1, -1))  # (dy, dx), sides first


def _thinning_tables():
    """For each 8-neighbourhood (bit i is P2..P9, clockwise from north), whether Zhang-Suen's two
    sub-steps peel the centre, and whether each staircase clean-up pass removes it."""
    peel = np.zeros((2, 256), bool)
    stair = np.zeros((4, 256), bool)
    for code in range(256):
        n = [(code >> i) & 1 for i in range(8)]
        count = sum(n)
        rises = sum(n[i] == 0 and n[(i + 1) % 8] == 1 for i in range(8))
        if 2 <= count <= 6 and rises == 1:
            peel[0, code] = n[0] * n[2] * n[4] == 0 and n[2] * n[4] * n[6] == 0
            peel[1, code] = n[0] * n[2] * n[6] == 0 and n[0] * n[4] * n[6] == 0
        x = [1 - n[i] for i in (2, 1, 0, 7, 6, 5, 4, 3)]  # E, NE, N, NW, W, SW, S, SE
        yokoi = sum(x[k] - x[k] * x[(k + 1) % 8] * x[(k + 2) % 8] for k in (0, 2, 4, 6))
        for i, (a, b) in enumerate(((0, 2), (2, 4), (4, 6), (6, 0))):  # N&E, E&S, S&W, W&N
            stair[i, code] = n[a] == 1 and n[b] == 1 and yokoi == 1 and count >= 2
    return peel, stair


PEEL, STAIR = _thinning_tables()
CODE_KERNEL = np.float32([[128, 1, 2], [64, 0, 4], [32, 16, 8]])  # P9 P2 P3 / P8 . P4 / P7 P6 P5


def thin(mask):
    """Zhang-Suen thinning: peel boundary pixels, alternately from two sides, until every stroke
    is one pixel wide and still connected. Each pass reads every pixel's neighbourhood as one
    byte (a single filter) and looks the decision up in a table."""
    img = np.pad(mask.astype(np.uint8), 1)
    code = lambda: cv2.filter2D(img, -1, CODE_KERNEL, borderType=cv2.BORDER_CONSTANT)
    while True:
        changed = False
        for step in (0, 1):
            peel = PEEL[step][code()] & (img == 1)
            if peel.any():
                img[peel] = 0
                changed = True
        if not changed:
            break
    # Zhang-Suen leaves diagonals as two-pixel staircases, where every pixel has three neighbours
    # and would read as a junction. Remove each staircase step whose removal keeps the line
    # connected (Yokoi's 8-connectivity number is 1), one orientation at a time.
    for table in STAIR:
        img[table[code()] & (img == 1)] = 0
    return img[1:-1, 1:-1].astype(bool)


def arc_length(pts):
    return float(np.linalg.norm(np.diff(pts, axis=0), axis=1).sum()) if len(pts) > 1 else 0.0


def centre_lines(mask):
    """Strokes along the middle of mask, in working pixels: ([(points, closed, width)], junction
    points). Spurs that
    thinning leaves at corners are pruned, chains meeting two at a time are joined, a line that
    carries straight on through a crossing stays one line, and free ends reach the ink's end."""
    ys, xs = np.nonzero(mask)
    pad = 2
    ox, oy = xs.min() - pad, ys.min() - pad
    crop = np.pad(mask[ys.min():ys.max() + 1, xs.min():xs.max() + 1], pad)
    dist = cv2.distanceTransform(crop.astype(np.uint8), cv2.DIST_L2, 5)
    sk = thin(crop)
    ones = np.ones((3, 3), np.float32)
    n8 = cv2.filter2D(sk.astype(np.uint8), -1, ones, borderType=cv2.BORDER_CONSTANT).astype(int) - sk
    junction = sk & (n8 >= 3)
    nj, jcomp, _, jcentre = cv2.connectedComponentsWithStats(junction.astype(np.uint8), connectivity=8)
    nodes = [(float(x) + 0.5, float(y) + 0.5) for x, y in jcentre[1:]]
    free = [False] * len(nodes)
    chain = sk & ~junction
    cn8 = cv2.filter2D(chain.astype(np.uint8), -1, ones, borderType=cv2.BORDER_CONSTANT).astype(int) - chain
    seen = np.zeros_like(chain)

    def walk(y, x):
        pts = [(x, y)]
        seen[y, x] = True
        while True:
            for dy, dx in STEPS:
                if chain[y + dy, x + dx] and not seen[y + dy, x + dx]:
                    y, x = y + dy, x + dx
                    seen[y, x] = True
                    pts.append((x, y))
                    break
            else:
                return pts

    def node_at(x, y, skip=None):
        for dy, dx in STEPS:
            j = jcomp[y + dy, x + dx]
            if j and j - 1 != skip:
                return j - 1
        if n8[y, x] <= 1 and skip is None:  # a free end of the stroke
            nodes.append((x + 0.5, y + 0.5))
            free.append(True)
            return len(nodes) - 1
        return None

    edges = []
    for y, x in np.argwhere(chain & (cn8 <= 1)):
        if seen[y, x]:
            continue
        pts = walk(y, x)
        a = node_at(*pts[0])
        b = node_at(*pts[-1], skip=a if len(pts) == 1 else None)
        if len(pts) == 1 and b is None and a is not None and free[a]:
            continue  # a lone pixel
        body = [(px + 0.5, py + 0.5) for px, py in pts]
        if a is not None and not free[a]:
            body.insert(0, nodes[a])
        if b is not None and not free[b]:
            body.append(nodes[b])
        edges.append(dict(pts=np.float64(body), a=a, b=b, closed=False))
    for y, x in np.argwhere(chain & ~seen):  # rings with no ends
        if not seen[y, x]:
            pts = walk(y, x)
            if len(pts) >= 8:
                edges.append(dict(pts=np.float64(pts) + 0.5, a=None, b=None, closed=True))
    at = lambda q: dist[min(max(int(q[1]), 0), dist.shape[0] - 1), min(max(int(q[0]), 0), dist.shape[1] - 1)]
    widths = [2 * at(q) - 0.5 for e in edges for q in e['pts'][::4]]
    width = float(np.median(widths)) if widths else 1.0
    reach = max(4.0, 3.0 * width)
    spur = lambda node: max(3.0, 1.2 * (2 * at(nodes[node]) - 0.5))  # thinning leaves spurs about one width long

    def ends(node):
        return [(e, side) for e in edges for side in ('a', 'b') if e[side] == node and not e['closed']]

    def drop(e):  # by identity: edges hold arrays, so == between them is not a truth value
        edges[:] = [x for x in edges if x is not e]

    def join(e1, e2, node):
        p1 = e1['pts'] if e1['b'] == node else e1['pts'][::-1]
        start = e1['a'] if e1['b'] == node else e1['b']
        drop(e1)
        if e2 is e1:
            edges.append(dict(pts=p1[:-1] if np.allclose(p1[0], p1[-1]) else p1, a=None, b=None, closed=True))
            return
        p2 = e2['pts'] if e2['a'] == node else e2['pts'][::-1]
        end = e2['b'] if e2['a'] == node else e2['a']
        drop(e2)
        # thinning bends both lines within about a width of a junction: leave that stretch out and
        # let the fit bridge it, so a line carried through a crossing gains no kink there
        r = max(2.0, 2 * at(nodes[node]) - 0.5)
        far1 = np.linalg.norm(p1 - nodes[node], axis=1) >= r
        far2 = np.linalg.norm(p2 - nodes[node], axis=1) >= r
        if far1.sum() >= 2 and far2.sum() >= 2:
            p1, p2 = p1[:len(p1) - int(np.argmax(far1[::-1]))], p2[int(np.argmax(far2)):]
            edges.append(dict(pts=np.vstack([p1, p2]), a=start, b=end, closed=False))
        else:
            edges.append(dict(pts=np.vstack([p1, p2[1:]]), a=start, b=end, closed=False))

    def outward(e, side):
        q = e['pts'] if side == 'a' else e['pts'][::-1]
        d = np.cumsum(np.linalg.norm(np.diff(q, axis=0), axis=1))
        i = min(int(np.searchsorted(d, reach)) + 1, len(q) - 1)
        return unit(q[i] - q[0])

    for e in list(edges):  # a crossing thinned into two nearby junctions is one junction
        if any(x is e for x in edges) and not e['closed'] and e['a'] is not None and e['b'] is not None \
                and e['a'] != e['b'] and not free[e['a']] and not free[e['b']] \
                and arc_length(e['pts']) < max(3.0, 2 * width):
            keep, gone = e['a'], e['b']
            drop(e)
            nodes[keep] = tuple((np.float64(nodes[keep]) + nodes[gone]) / 2)
            for x in edges:
                for side, row in (('a', 0), ('b', -1)):
                    if x[side] in (keep, gone):
                        x[side] = keep
                        x['pts'] = x['pts'].copy()
                        x['pts'][row] = nodes[keep]
    for _ in range(3):
        for e in list(edges):  # spurs: a short branch from a junction to a free end
            if e['closed'] or not any(x is e for x in edges):
                continue
            for tip, root in (('a', 'b'), ('b', 'a')):
                if e[tip] is not None and free[e[tip]] and e[root] is not None and not free[e[root]] \
                        and len(ends(e[root])) >= 3 and arc_length(e['pts']) < spur(e[root]):
                    drop(e)
                    break
        for node in range(len(nodes)):  # a node with two chains is no junction
            if not free[node]:
                meet = ends(node)
                if len(meet) == 2:
                    join(meet[0][0], meet[1][0], node)
    for node in range(len(nodes)):  # carry straight on through crossings
        if free[node]:
            continue
        while True:
            meet = ends(node)
            pairs = sorted(((float(np.dot(outward(*u), outward(*v))), u, v)
                            for i, u in enumerate(meet) for v in meet[i + 1:] if u[0] is not v[0] or u[1] != v[1]),
                           key=lambda t: t[0])
            if len(meet) < 2 or not pairs or pairs[0][0] > -0.87:  # at least 150 degrees apart
                break
            join(pairs[0][1][0], pairs[0][2][0], node)
        meet = ends(node)
        if len(meet) == 2:  # what is left of a crossing, or a corner, is one line
            join(meet[0][0], meet[1][0], node)

    out = []
    for e in edges:
        pts = e['pts']
        if len(pts) < 2 or (not e['closed'] and arc_length(pts) < max(3.0, width)):  # a dot, not a line
            continue
        if not e['closed']:  # free ends run on to where the ink stops
            for side in ('a', 'b'):
                if e[side] is not None and free[e[side]]:
                    q = pts if side == 'a' else pts[::-1]
                    d = -outward(dict(pts=q), 'a')
                    tip = q[0].copy()
                    for _ in range(int(4 * width) + 4):
                        nxt = tip + 0.5 * d
                        yy, xx = int(nxt[1]), int(nxt[0])
                        if not (0 <= yy < crop.shape[0] and 0 <= xx < crop.shape[1] and crop[yy, xx]):
                            break
                        tip = nxt
                    q = np.vstack([tip, q])
                    pts = q if side == 'a' else q[::-1]
        k = 2
        if len(pts) > 2 * k + 1:  # smooth the pixel steps, keeping the ends where they are
            wrap = np.vstack([pts[-k:], pts, pts[:k]]) if e['closed'] else \
                np.vstack([np.repeat(pts[:1], k, 0), pts, np.repeat(pts[-1:], k, 0)])
            smooth = cv2.GaussianBlur(wrap.reshape(-1, 1, 2), (1, 2 * k + 1), 0, sigmaY=1.0).reshape(-1, 2)[k:-k]
            if not e['closed']:
                smooth[0], smooth[-1] = pts[0], pts[-1]
            pts = smooth
        local = float(np.median([2 * at(q) - 0.5 for q in pts[::2]]))
        out.append((pts + (ox, oy), e['closed'], max(1.0, local)))
    junctions = np.float64([nodes[i] for i in range(len(nodes)) if not free[i]]).reshape(-1, 2) + (ox, oy)
    return out, junctions


def width_profile(pts, closed, guess, cover, tol, junctions=()):
    """The stroke's width along its length, as knots [(fraction of length, width)]. The width at a
    point is the ink coverage (0 to 1 per pixel) summed across the line, so anti-aliasing gives a
    sub-pixel answer; only the run of ink touching the centre counts, so a parallel line nearby
    does not; a running median removes the bump where another line crosses; and the profile is
    then cut to the fewest knots that stay within tol of it. Near a junction the cross-section runs
    along the other line, so those samples are bridged from either side instead."""
    p = resample_open(np.vstack([pts, pts[:1]]) if closed else pts, 1.0)
    n = len(p)
    if n < 3:
        return [(0.0, guess), (1.0, guess)]
    m = max(1, int(round(guess)))
    i = np.arange(n)
    ahead = p[(i + m) % n] if closed else p[np.minimum(i + m, n - 1)]
    behind = p[(i - m) % n] if closed else p[np.maximum(i - m, 0)]
    tangent = ahead - behind
    tangent /= np.maximum(np.linalg.norm(tangent, axis=1, keepdims=True), 1e-9)
    normal = np.column_stack([-tangent[:, 1], tangent[:, 0]])
    reach = 1.5 * guess + 3
    offs = np.arange(-reach, reach + 0.25, 0.5, dtype=np.float32)
    xs = (p[:, 0, None] + normal[:, 0, None] * offs[None]).astype(np.float32) - 0.5  # pixel centres
    ys = (p[:, 1, None] + normal[:, 1, None] * offs[None]).astype(np.float32) - 0.5
    across = cv2.remap(cover, xs, ys, cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT, borderValue=0)
    c = len(offs) // 2
    inked = across > 0.02
    touching = np.zeros_like(inked)
    touching[:, c:] = np.cumprod(inked[:, c:], axis=1)
    touching[:, :c + 1] |= np.cumprod(inked[:, c::-1], axis=1)[:, ::-1].astype(bool)
    widths = (across * touching).sum(1) * 0.5
    x = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    if not closed:  # a round end reads as a narrowing; hold the width the stroke had before its cap
        cap = np.flatnonzero(x >= 0.5 * widths)
        if len(cap):
            widths[:cap[0]] = widths[cap[0]]
        cap = np.flatnonzero(x[-1] - x >= 0.5 * widths)
        if len(cap):
            widths[cap[-1] + 1:] = widths[cap[-1]]
    if len(junctions):
        near = np.min(np.linalg.norm(p[:, None] - np.float64(junctions)[None], axis=2), axis=1) < 2 * guess + 2
        if near.any() and not near.all():
            widths[near] = np.interp(x[near], x[~near], widths[~near])
    k = min(n - (1 - n % 2), (int(4 * guess) + 5) | 1)  # longer than a corner's bulge
    padded = np.concatenate([widths[-(k // 2):], widths, widths[:k // 2]]) if closed else \
        np.pad(widths, k // 2, mode='edge')
    widths = np.median(np.lib.stride_tricks.sliding_window_view(padded, k), axis=1)
    x /= max(x[-1], 1e-9)
    keep = np.zeros(n, bool)
    keep[[0, -1]] = True
    todo = [(0, n - 1)]
    while todo:
        a, b = todo.pop()
        if b > a + 1:
            line = widths[a] + (widths[b] - widths[a]) * (x[a + 1:b] - x[a]) / max(x[b] - x[a], 1e-9)
            miss = np.abs(widths[a + 1:b] - line)
            j = int(miss.argmax())
            if miss[j] > tol:
                keep[a + 1 + j] = True
                todo += [(a, a + 1 + j), (a + 1 + j, b)]
    return [(float(x[j]), float(widths[j])) for j in np.flatnonzero(keep)]


def mean_width(profile):
    """Length-weighted mean of a piecewise-linear width profile."""
    x, w = np.float64(profile).T
    return float(np.sum((w[1:] + w[:-1]) / 2 * np.diff(x))) if len(x) > 1 else float(w[0])


def stroke_raster(strokes, shape, scale):
    """Strokes drawn at the width their profile gives at each point along them."""
    canvas = np.zeros(shape, np.uint8)
    for st in strokes:
        sp, closed, profile = st[1], st[2], np.float64(st[5])
        pts = flatten(sp, 0.5)
        if closed:
            pts = np.vstack([pts, pts[:1]])
        along = np.r_[0, np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))]
        width = np.interp(along / max(along[-1], 1e-9), profile[:, 0], profile[:, 1])
        ends = np.round(pts * scale * 16).astype(np.int32)
        for i in range(len(pts) - 1):
            cv2.line(canvas, tuple(ends[i]), tuple(ends[i + 1]), 1,
                     max(1, int(round((width[i] + width[i + 1]) / 2 * scale))), cv2.LINE_8, 4)
    return canvas.astype(bool)


# ---- the conversion -----------------------------------------------------------------------

def unique_ids(bases):
    """Valid, distinct SVG ids: red, red_2, red_3, ..."""
    used, out = set(), []
    for base in bases:
        base = re.sub(r'[^A-Za-z0-9_-]', '_', base)
        base = base if re.match(r'[A-Za-z_]', base) else '_' + base
        name, n = base, 2
        while name in used:
            name, n = f'{base}_{n}', n + 1
        used.add(name)
        out.append(name)
    return out


def read_image(path):
    if not Path(path).is_file():
        raise SystemExit(f'no such image: {path}')
    image = cv2.imdecode(np.fromfile(str(path), np.uint8), cv2.IMREAD_UNCHANGED)
    if image is None:
        raise SystemExit(f'cannot read image: {path}')
    if image.dtype == np.uint16:
        image = (image >> 8).astype(np.uint8)
    if image.ndim == 2:
        image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    alpha = None
    if image.shape[2] == 4:
        alpha = image[:, :, 3].astype(np.float32) / 255
        image = image[:, :, :3].copy()
        if alpha.min() > 0.99:
            alpha = None
    return image, alpha


def write_png(path, image):
    cv2.imencode('.png', image)[1].tofile(str(path))


def convert(source, out, args):
    knobs = dict(DETAIL[args.detail])
    for knob in ('min_area', 'tolerance', 'corner'):
        if getattr(args, knob) is not None:
            knobs[knob] = getattr(args, knob)
    warnings = []
    bgr, alpha = read_image(source)
    h, w = bgr.shape[:2]
    if args.denoise > 0:
        bgr = cv2.bilateralFilter(bgr, 0, args.denoise, 1.5)
    s = args.upscale if args.upscale else (2 if args.detail == 'high' and max(h, w) <= 2048 else 1)
    if s != 1:
        bgr = cv2.resize(bgr, None, fx=s, fy=s, interpolation=cv2.INTER_CUBIC)
        alpha = None if alpha is None else cv2.resize(alpha, None, fx=s, fy=s, interpolation=cv2.INTER_LINEAR)
    lab = to_lab(bgr)

    background, bg_colours = find_background(lab, alpha, args, warnings)
    has_bg = background is not None
    bg = background if has_bg else np.zeros(lab.shape[:2], bool)
    foreground = ~bg
    if not foreground.any():
        raise SystemExit('the whole image matched the background colour')
    samples = lab[foreground]
    samples = samples[np.random.default_rng(0).choice(len(samples), min(len(samples), 100_000), replace=False)]
    if args.colors:
        centres, _ = kmeans(samples, args.colors)
    else:
        centres, counts = kmeans(samples, 16)
        centres, counts = merge_centres(centres, counts, args.merge)
        ends = [hex_to_lab(c) for c in bg_colours if c.startswith('#')]
        drop = blends(centres, counts, ends, args.confidence * 0.75, 1.0 if args.mode == 'lines' else 0.02)
        centres = np.float32([c for i, c in enumerate(centres) if i not in drop])
    if args.mode == 'lines':
        if not has_bg:
            raise SystemExit('lines mode needs a background: pass --bg #rrggbb')
        return convert_lines(source, out, args, knobs, warnings, lab, alpha, bg, bg_colours, centres, s, w, h)
    index, distance = nearest(lab, centres)

    # label 0 is background; 1..K are the colour centres
    count = len(centres) + 1
    raw = np.where(bg, 0, index + 1)
    labels = np.where(foreground & (distance > args.confidence), -1, raw)
    if alpha is not None:
        labels[(alpha > 0.1) & (alpha < 0.9) & foreground] = -1
    min_area, min_width = knobs['min_area'] * s * s, knobs['min_width'] * s
    for _ in range(2):
        labels = fill_unknown(drop_fragments(labels, count, min_area, min_width), raw)
        labels = smooth_labels(labels, count, knobs['sigma'] * s)
    if args.fill_holes and has_bg:
        labels = fill_enclosed(labels)
    found = int(sum((labels == k).sum() >= min_area for k in range(1, count)))
    if not args.colors and found > 8:
        warnings.append(f'{found} colours survived: shading or gradients? pass --colors N, '
                        'or ask the image generator for flat colours')
    colour = {k: centres[k - 1] for k in range(1, count)}
    colour[0] = hex_to_lab(bg_colours[0]) if bg_colours and bg_colours[0].startswith('#') else \
        to_lab(np.uint8([[[255, 255, 255]]]))[0, 0]
    if args.mode == 'silhouette':
        if not has_bg:
            raise SystemExit('silhouette mode needs a background: pass --bg #rrggbb')
        sizes = np.bincount(labels.ravel(), minlength=count)
        sizes[0] = 0
        colour[1] = colour[int(sizes.argmax())]
        labels, count = (labels != 0).astype(np.int32), 2

    # trace every region, biggest first
    regions = [k for k in range(count) if (k != 0 or (args.keep_background and has_bg)) and (labels == k).any()]
    regions.sort(key=lambda k: -int((labels == k).sum()))
    tracer = args.tracer
    if tracer == 'vtracer' and vtracer is None:
        raise SystemExit('vtracer is not installed: pip install vtracer, or pass --tracer opencv')
    shapes = []
    for k in regions:
        mask = labels == k
        subpaths = trace_mask(mask, tracer, knobs, s)
        entries = organise(subpaths, knobs['min_area'])
        if not entries:
            continue
        parts = [entries]
        if args.split:  # each outline with its own holes; an island inside a hole is its own part
            roots = {e[3]: [e] for e in entries if e[1] % 2 == 0}
            for e in entries:
                if e[1] % 2:
                    roots[e[2]].append(e)
            parts = list(roots.values())
        for part in parts:
            shapes.append(dict(label=k, entries=part, colour=colour[k],
                               area=sum(signed_area(flatten(e[0])) for e in part)))

    if not shapes:
        raise SystemExit('nothing was traced: every region was smaller than --min-area')
    shapes.sort(key=lambda sh: -sh['area'])

    # ids: colour names, then --drop by those ids, then --names over what is left, biggest first
    unique = unique_ids
    auto = (('silhouette' if args.mode == 'silhouette' else colour_name(sh['colour'])) for sh in shapes)
    for sh, name in zip(shapes, unique(auto)):
        sh['id'] = name
    dropped = []
    if args.drop:
        gone = {d.strip() for d in args.drop.split(',')}
        for sh in [sh for sh in shapes if sh['id'] in gone or re.sub(r'_\d+$', '', sh['id']) in gone]:
            shapes.remove(sh)
            dropped.append(sh['id'])
            labels[rasterise(sh['entries'], labels.shape, lambda pts: pts * s) & (labels == sh['label'])] = 0
        if not shapes:
            raise SystemExit('--drop removed every shape')
    names = [n.strip() for n in args.names.split(',')] if args.names else []
    for sh, name in zip(shapes, unique(names[i] if i < len(names) and names[i] else sh['id']
                                       for i, sh in enumerate(shapes))):
        sh['id'] = name
    if len(names) > len(shapes):
        warnings.append(f'{len(names)} names given for {len(shapes)} shapes; the extra names were unused')

    # fidelity: re-rasterise each shape from its final paths and score against the cleaned regions
    to_work = lambda pts: pts * s
    rendered = np.full(labels.shape, -1, np.int32)
    for i, sh in enumerate(shapes):
        painted = rasterise(sh['entries'], labels.shape, to_work)
        rendered[painted & (rendered < 0)] = i
        sh['painted'] = painted
    for k in set(sh['label'] for sh in shapes):
        target = labels == k
        painted = np.any([sh['painted'] for sh in shapes if sh['label'] == k], axis=0)
        iou = float((target & painted).sum() / max(1, (target | painted).sum()))
        for sh in shapes:
            if sh['label'] == k:
                sh['iou'] = round(iou, 4)
    total = sum(sh['area'] for sh in shapes)
    fidelity = sum(sh['iou'] * sh['area'] for sh in shapes) / total
    if fidelity < 0.94:
        warnings.append(f'trace matches the cleaned regions at IoU {fidelity:.3f}; look at the preview, '
                        'try a smaller --tolerance, --detail high or fewer --colors')
    hues = {}
    for sh in shapes:
        hues.setdefault(colour_name(sh['colour']), set()).add(sh['label'])
    shaded = sorted(name for name, found in hues.items() if len(found) > 1 and name not in ('white', 'gray', 'black'))
    if shaded and args.mode == 'colors':
        warnings.append(f'several shades of {", ".join(shaded)}: if that is lighting rather than design, pass '
                        f'--colors {len({sh["label"] for sh in shapes}) - 1} or --mode silhouette')
    noisy = [sh['id'] for sh in shapes if len(sh['entries']) > 60]
    if noisy:
        warnings.append(f'many small pieces in {", ".join(noisy)}: noise or texture? try --detail low or --colors')

    # output frame: tight crop (or the whole image), optional --fit
    pts = np.concatenate([flatten(e[0]) for sh in shapes for e in sh['entries']])
    if args.no_crop:
        x0, y0, x1, y1 = 0.0, 0.0, float(w), float(h)
    else:
        x0, y0 = np.floor(pts.min(0)) - args.margin
        x1, y1 = np.ceil(pts.max(0)) + args.margin
    k = args.fit / max(x1 - x0, y1 - y0) if args.fit else 1.0
    to_out = lambda p: ((p[0] - x0) * k, (p[1] - y0) * k)
    W, H = (x1 - x0) * k, (y1 - y0) * k

    lines = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{fmt(W)}" height="{fmt(H)}" '
             f'viewBox="0 0 {fmt(W)} {fmt(H)}">']
    report_shapes = []
    for sh in shapes:
        subs = [transform(e[0], to_out) for e in sh['entries']]
        flat = np.concatenate([flatten(sp) for sp in subs])
        bx0, by0 = flat.min(0)
        bx1, by1 = flat.max(0)
        hexc = lab_to_hex(sh['colour'])
        lines.append(f'  <path id="{sh["id"]}" fill="{hexc}" fill-rule="nonzero" d="{path_d(subs)}"/>')
        report_shapes.append(dict(
            id=sh['id'], colour=hexc, area=round(sh['area'] * k * k, 2),
            bbox=[round(float(v), 2) for v in (bx0, by0, bx1 - bx0, by1 - by0)],
            outlines=sum(1 for e in sh['entries'] if e[1] % 2 == 0),
            holes=sum(1 for e in sh['entries'] if e[1] % 2),
            segments=sum(len(e[0][1]) for e in sh['entries']), iou=sh['iou']))
    lines.append('</svg>')
    out.write_text('\n'.join(lines) + '\n', encoding='utf-8')

    report = dict(
        source=str(source), svg=str(out), width=round(float(W), 2), height=round(float(H), 2),
        units='source pixels' if k == 1 else f'source pixels x {k:.6g} (--fit {args.fit})',
        crop=None if args.no_crop else [round(float(v), 2) for v in (x0, y0, x1 - x0, y1 - y0)],
        mode=args.mode, tracer=tracer, tolerance=knobs['tolerance'], upscale=s, background=bg_colours,
        segments=sum(sh['segments'] for sh in report_shapes),
        fidelity=round(fidelity, 4), shapes=report_shapes, dropped=dropped, warnings=warnings)

    if args.preview != 'none':
        preview_path = Path(args.preview) if args.preview else out.with_suffix('.preview.png')
        write_png(preview_path, preview(source, labels, rendered, shapes, s))
        report['preview'] = str(preview_path)
    report_path = Path(str(out) + '.json')
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    report['report'] = str(report_path)
    return report


def convert_lines(source, out, args, knobs, warnings, lab, alpha, bg, bg_colours, inks, s, w, h):
    """--mode lines: every ink's strokes become centre lines, open or closed, with a width."""
    if args.tracer == 'vtracer':
        raise SystemExit('lines mode traces centre lines itself: use --tracer fit or opencv')
    # each pixel belongs to the ink whose ramp to the paper it lies on, and is ink once it is past
    # halfway along that ramp: a stroke's edge is where it is half ink, and the grey fringe of a
    # dark line is not mistaken for a light ink that happens to be a similar colour
    flat = lab.reshape(-1, 3)
    labels = np.zeros(len(flat), np.int32)
    cover = np.zeros(len(flat), np.float32)  # how far along its ramp each pixel is: 0 paper, 1 ink
    if bg_colours and bg_colours[0].startswith('#'):
        papers = [hex_to_lab(c) for c in bg_colours]
        paper_index = nearest(lab, papers)[0].ravel() if len(papers) > 1 else np.zeros(len(flat), np.int32)
        for j, paper in enumerate(papers):
            sel = np.flatnonzero(paper_index == j)
            x = flat[sel] - paper
            ramps = np.float32(inks) - paper                      # one row per ink
            along = x @ ramps.T                                   # x . ramp, for every ink at once
            length = np.maximum((ramps * ramps).sum(1), 1e-6)
            t = np.clip(along / length, 0, 1)
            off = np.einsum('ij,ij->i', x, x)[:, None] - 2 * t * along + t * t * length  # |x - t ramp|^2
            k = off.argmin(1)
            tk = t[np.arange(len(sel)), k]
            labels[sel] = np.where(tk >= 0.5, k + 1, 0)
            cover[sel] = np.clip((tk - 0.1) / 0.8, 0, 1)  # paper noise and ink noise are not partial pixels
    else:
        index, _ = nearest(lab, inks)
        labels = np.where(alpha.ravel() >= 0.5, index.ravel() + 1, 0)
        cover = alpha.ravel().astype(np.float32)
    labels, cover = labels.reshape(lab.shape[:2]), cover.reshape(lab.shape[:2])
    labels[bg] = 0
    min_area = knobs['min_area'] * s * s
    for k in range(1, len(inks) + 1):  # specks are not strokes
        n, comp, stats, _ = cv2.connectedComponentsWithStats((labels == k).astype(np.uint8), connectivity=8)
        small = stats[:, cv2.CC_STAT_AREA] < min_area
        small[0] = False
        labels[small[comp]] = 0

    strokes = []  # (label, subpath in source px, closed, width in source px, length)
    widest, wide = args.max_width or 0.04 * max(w, h), set()
    for k in range(1, len(inks) + 1):
        mask = labels == k
        if not mask.any():
            continue
        deep = cv2.distanceTransform(mask.astype(np.uint8), cv2.DIST_L2, 5) > widest * s / 2
        if deep.any():  # most of this ink lies inside areas wider than any line: a fill colour
            reach = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (int(widest * s) | 1,) * 2)
            if (cv2.dilate(deep.astype(np.uint8), reach).astype(bool) & mask).sum() > 0.5 * mask.sum():
                wide.add(colour_name(inks[k - 1]))
                continue
        found, junctions = centre_lines(mask)
        thin_ones = [f for f in found if f[2] / s <= widest]  # wider is the middle of a filled area
        if len(thin_ones) < len(found):
            drawn = np.zeros(mask.shape, np.uint8)
            for pts, closed, width in thin_ones:
                cv2.polylines(drawn, [np.int32(np.round(pts))], closed, 1, max(1, int(round(width))))
            if (drawn.astype(bool) & mask).sum() < 0.3 * mask.sum():  # a fill colour: its thin bits are edges
                thin_ones = []
            wide.add(colour_name(inks[k - 1]))
        own = cover * ((labels == k) | ((labels == 0) & ~bg))
        for pts, closed, width in thin_ones:
            p = pts / s
            if args.tracer == 'opencv':
                q = cv2.approxPolyDP(np.float32(p).reshape(-1, 1, 2), knobs['tolerance'], closed).reshape(-1, 2)
                sp = (tuple(q[0]), [('L', tuple(v)) for v in q[1:]])
                sp = close(sp) if closed and len(q) >= 3 else sp
            else:
                sp = fit_outline(p, knobs['tolerance'], knobs['corner']) if closed else \
                    fit_open(p, knobs['tolerance'], knobs['corner'])
            if closed:  # the profile starts where the fitted subpath starts
                pts = np.roll(pts, -int(np.linalg.norm(pts - np.float64(sp[0]) * s, axis=1).argmin()), 0)
            profile = width_profile(pts, closed, width, own, max(0.25 * s, 0.1 * width), junctions)
            profile = [(f, wd / s) for f, wd in profile]
            flat = flatten(sp)
            strokes.append((k, sp, closed, mean_width(profile), arc_length(flat), profile))
    if wide:
        warnings.append(f'skipped filled areas in {", ".join(sorted(wide))} (wider than {widest:.0f} px): '
                        'trace those with --mode colors')
    if not strokes:
        raise SystemExit('no strokes found: is the ink lighter than --bg-tolerance from the background?')

    groups = [[st] for st in strokes] if args.split else \
        [[st for st in strokes if st[0] == k] for k in sorted({st[0] for st in strokes})]
    shapes = []
    for g in groups:
        length = sum(st[4] for st in g)
        width = sum(st[3] * st[4] for st in g) / max(length, 1e-9)
        shapes.append(dict(label=g[0][0], colour=inks[g[0][0] - 1], strokes=g, length=length, width=width))
    shapes.sort(key=lambda sh: -sh['length'])
    for sh, name in zip(shapes, unique_ids(colour_name(sh['colour']) for sh in shapes)):
        sh['id'] = name
    dropped = []
    if args.drop:
        gone = {d.strip() for d in args.drop.split(',')}
        for sh in [sh for sh in shapes if sh['id'] in gone or re.sub(r'_\d+$', '', sh['id']) in gone]:
            shapes.remove(sh)
            dropped.append(sh['id'])
            labels[stroke_raster(sh['strokes'], labels.shape, s) & (labels == sh['label'])] = 0
        if not shapes:
            raise SystemExit('--drop removed every shape')
    names = [n.strip() for n in args.names.split(',')] if args.names else []
    for sh, name in zip(shapes, unique_ids(names[i] if i < len(names) and names[i] else sh['id']
                                           for i, sh in enumerate(shapes))):
        sh['id'] = name

    # fidelity: strokes drawn at their width against the ink they came from, allowing --tolerance
    # either way (overlap would punish a 2 px line moved by an allowed pixel as if it were missing)
    rendered = np.full(labels.shape, -1, np.int32)
    for i, sh in enumerate(shapes):
        sh['painted'] = stroke_raster(sh['strokes'], labels.shape, s)
        rendered[sh['painted'] & (rendered < 0)] = i
    r = max(1, int(round(max(1.0, knobs['tolerance']) * s)))
    slack = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * r + 1, 2 * r + 1))
    grow = lambda b: cv2.dilate(b.astype(np.uint8), slack).astype(bool)
    for k in {sh['label'] for sh in shapes}:
        ink = labels == k
        drawn = np.any([sh['painted'] for sh in shapes if sh['label'] == k], axis=0)
        found = (ink & grow(drawn)).sum() / max(1, ink.sum())
        right = (drawn & grow(ink)).sum() / max(1, drawn.sum())
        score = float(2 * found * right / max(found + right, 1e-9))
        for sh in shapes:
            if sh['label'] == k:
                sh['match'] = round(score, 4)
    fidelity = sum(sh['match'] * sh['length'] for sh in shapes) / sum(sh['length'] for sh in shapes)
    if fidelity < 0.9:
        warnings.append(f'strokes drawn at their width match the ink at {fidelity:.3f}; look at the preview '
                        'for missing or extra lines')
    thick = [sh['id'] for sh in shapes if sh['width'] > 0.04 * max(w, h)]
    if thick:
        warnings.append(f'{", ".join(thick)} {"is" if len(thick) == 1 else "are"} very wide for a line: '
                        'filled shapes trace better with --mode colors')

    pad = max(sh['width'] for sh in shapes) / 2
    pts = np.concatenate([flatten(st[1]) for sh in shapes for st in sh['strokes']])
    if args.no_crop:
        x0, y0, x1, y1 = 0.0, 0.0, float(w), float(h)
    else:
        x0, y0 = np.floor(pts.min(0) - pad) - args.margin
        x1, y1 = np.ceil(pts.max(0) + pad) + args.margin
    k = args.fit / max(x1 - x0, y1 - y0) if args.fit else 1.0
    to_out = lambda p: ((p[0] - x0) * k, (p[1] - y0) * k)
    W, H = (x1 - x0) * k, (y1 - y0) * k
    lines = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{fmt(W)}" height="{fmt(H)}" '
             f'viewBox="0 0 {fmt(W)} {fmt(H)}">']
    report_shapes = []
    for sh in shapes:
        subs = [transform(st[1], to_out) for st in sh['strokes']]
        closed = [st[2] for st in sh['strokes']]
        flat = np.concatenate([flatten(sp) for sp in subs])
        bx0, by0 = flat.min(0)
        bx1, by1 = flat.max(0)
        hexc = lab_to_hex(sh['colour'])
        profiles = [[[round(f, 3), round(wd * k, 2)] for f, wd in st[5]] for st in sh['strokes']]
        every = [wd for pr in profiles for _, wd in pr]
        extra = {}
        if args.expand:  # the strokes drawn out to their widths, traced as filled outlines
            e = 2 * s
            body = stroke_raster(sh['strokes'], (int(h * e) + 4, int(w * e) + 4), e)
            entries = organise(trace_mask(body, 'fit', knobs, e), knobs['min_area'])
            outline = [transform(en[0], to_out) for en in entries]
            lines.append(f'  <path id="{sh["id"]}" fill="{hexc}" fill-rule="nonzero" d="{path_d(outline)}"/>')
            extra = dict(expanded=True, outline_segments=sum(len(sp[1]) for sp in outline),
                         outlines=sum(1 for en in entries if en[1] % 2 == 0),
                         holes=sum(1 for en in entries if en[1] % 2))
        else:
            attr = ' | '.join(', '.join(f'{fmt(f)} {fmt(wd)}' for f, wd in pr) for pr in profiles)
            lines.append(f'  <path id="{sh["id"]}" fill="none" stroke="{hexc}" stroke-width="{fmt(sh["width"] * k)}" '
                         f'stroke-linecap="round" stroke-linejoin="round" data-width-profile="{attr}" '
                         f'd="{path_d(subs, closed)}"/>')
        report_shapes.append(dict(
            id=sh['id'], colour=hexc, kind='stroke', width=round(sh['width'] * k, 2),
            width_range=[min(every), max(every)],
            length=round(sh['length'] * k, 1), strokes=len(subs), closed=sum(closed),
            segments=extra.pop('outline_segments', sum(len(sp[1]) for sp in subs)), **extra,
            bbox=[round(float(v), 2) for v in (bx0, by0, bx1 - bx0, by1 - by0)], match=sh['match'],
            profiles=profiles))
    lines.append('</svg>')
    out.write_text('\n'.join(lines) + '\n', encoding='utf-8')

    report = dict(
        source=str(source), svg=str(out), width=round(float(W), 2), height=round(float(H), 2),
        units='source pixels' if k == 1 else f'source pixels x {k:.6g} (--fit {args.fit})',
        crop=None if args.no_crop else [round(float(v), 2) for v in (x0, y0, x1 - x0, y1 - y0)],
        mode='lines', tracer=args.tracer, tolerance=knobs['tolerance'], upscale=s, background=bg_colours,
        segments=sum(sh['segments'] for sh in report_shapes),
        fidelity=round(fidelity, 4), shapes=report_shapes, dropped=dropped, warnings=warnings)
    if args.preview != 'none':
        preview_path = Path(args.preview) if args.preview else out.with_suffix('.preview.png')
        write_png(preview_path, preview(source, labels, rendered, shapes, s))
        report['preview'] = str(preview_path)
    report_path = Path(str(out) + '.json')
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    report['report'] = str(report_path)
    return report


def preview(source, labels, rendered, shapes, s):
    """source | traced SVG, flat colours | disagreements in red over the source, all at source size."""
    image, alpha = read_image(source)
    h, w = image.shape[:2]
    checker = (((np.indices((h, w)) // 16).sum(0) % 2) * 40 + 200).astype(np.uint8)[..., None].repeat(3, 2)
    if alpha is not None:
        image = (image * alpha[..., None] + checker * (1 - alpha[..., None])).astype(np.uint8)
    traced = checker.copy()
    small, labels = rendered[s // 2::s, s // 2::s][:h, :w], labels[s // 2::s, s // 2::s][:h, :w]
    for i, sh in enumerate(shapes):
        rgb = cv2.cvtColor(np.float32(sh['colour']).reshape(1, 1, 3), cv2.COLOR_Lab2BGR)[0, 0]
        traced[small == i] = np.clip(rgb * 255, 0, 255).astype(np.uint8)
    label_of = np.array([sh['label'] for sh in shapes] + [0], np.int32)
    wrong = label_of[np.where(small >= 0, small, len(shapes))] != labels
    diff = cv2.cvtColor(cv2.cvtColor(image, cv2.COLOR_BGR2GRAY), cv2.COLOR_GRAY2BGR) // 2 + 100
    diff[wrong] = (40, 40, 230)
    strip = np.concatenate([image, np.full((h, 8, 3), 255, np.uint8), traced,
                            np.full((h, 8, 3), 255, np.uint8), diff], axis=1)
    scale = min(1.0, 1600 / strip.shape[1])
    if scale < 1:
        strip = cv2.resize(strip, None, fx=scale, fy=scale, interpolation=cv2.INTER_AREA)
    return strip


def parse_args(argv):
    p = argparse.ArgumentParser(
        description='Convert generated images into clean SVG shapes with ids (one <path> per shape).',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    p.add_argument('inputs', nargs='+', help='image files (PNG/JPEG/WebP); globs are expanded')
    p.add_argument('-o', '--out', help='output .svg (one input) or directory (several)')
    p.add_argument('--mode', choices=['colors', 'silhouette', 'lines'], default='colors',
                   help='colors: one filled shape per flat colour; silhouette: one filled shape for everything not '
                        'background; lines: the centre line of each stroke, as open or closed paths with a width')
    p.add_argument('--colors', type=int, help='exact number of foreground colours (default: found automatically)')
    p.add_argument('--names', help='comma-separated ids, given to shapes biggest first')
    p.add_argument('--split', action='store_true', help='one shape per separate piece instead of per colour')
    p.add_argument('--drop', help='comma-separated colour ids to leave out (white drops white, white_2, ...)')
    p.add_argument('--detail', choices=list(DETAIL), default='medium', help='smoothing and smallest-feature preset')
    p.add_argument('--bg', default='auto', help="'auto' (border colour or alpha), 'none', or #rrggbb[,#rrggbb]")
    p.add_argument('--bg-tolerance', type=float, default=12,
                   help='colour distance (CIE dE) still counted as background')
    p.add_argument('--bg-connected', action='store_true',
                   help='background only where connected to the border; enclosed background-coloured areas stay shapes')
    p.add_argument('--fill-holes', action='store_true', help='fill enclosed background (outer outline only)')
    p.add_argument('--keep-background', action='store_true', help='also emit the background as a shape')
    p.add_argument('--merge', type=float, default=14, help='auto colours closer than this dE are one colour')
    p.add_argument('--confidence', type=float, default=18,
                   help='pixels further than this dE from every colour are edge blends, re-assigned to a neighbour')
    p.add_argument('--min-area', type=float, help='smallest shape or hole kept, in source px^2 (overrides --detail)')
    p.add_argument('--denoise', type=float, default=20, help='bilateral filter colour sigma; 0 turns it off')
    p.add_argument('--upscale', type=int,
                   help='work at this multiple of the source size (default 1; 2 with --detail high, up to 2048 px)')
    p.add_argument('--expand', action='store_true',
                   help='lines mode: draw each stroke out to its measured width along its length and output the '
                        'filled outline (a leaf from its midrib) instead of the centre line')
    p.add_argument('--max-width', type=float,
                   help='lines mode: ink wider than this (source px) is a filled area, not a line (default 4%% of '
                        'the image)')
    p.add_argument('--tolerance', type=float,
                   help='how far a curve may stray from the traced edge, in source px (overrides --detail); '
                        'bigger means fewer points')
    p.add_argument('--corner', type=float,
                   help='a turn sharper than this many degrees is a corner (overrides --detail)')
    p.add_argument('--tracer', choices=['fit', 'opencv', 'vtracer'], default='fit',
                   help='fit: fewest lines and curves within --tolerance; opencv: polygon within --tolerance; '
                        'vtracer: dense splines (pip install vtracer)')
    p.add_argument('--fit', type=float, help='scale so the longer side of the output is this many units')
    p.add_argument('--no-crop', action='store_true', help='keep the full image frame instead of cropping to the shapes')
    p.add_argument('--margin', type=float, default=0, help='space kept around the shapes when cropping, in source px')
    p.add_argument('--preview', help="preview PNG path, or 'none' (default: <out>.preview.png)")
    p.add_argument('--json', action='store_true', help='print the report as JSON instead of a summary')
    args = p.parse_args(argv)
    args.inputs = [f for pattern in args.inputs for f in (sorted(glob.glob(pattern)) or [pattern])]
    if args.out and len(args.inputs) > 1 and Path(args.out).suffix.lower() == '.svg':
        p.error('-o must be a directory when converting several images')
    return args


def main(argv=None):
    args = parse_args(argv)
    reports = []
    for source in map(Path, args.inputs):
        if args.out and Path(args.out).suffix.lower() == '.svg':
            out = Path(args.out)
        elif args.out:
            Path(args.out).mkdir(parents=True, exist_ok=True)
            out = Path(args.out) / (source.stem + '.svg')
        else:
            out = source.with_suffix('.svg')
        out.parent.mkdir(parents=True, exist_ok=True)
        report = convert(source, out, args)
        reports.append(report)
        if not args.json:
            print(f'{source} -> {out}  {report["width"]:g} x {report["height"]:g}  {report["segments"]} segments'
                  f'  fidelity {report["fidelity"]:.3f}')
            for sh in report['shapes']:
                if sh.get('kind') == 'stroke':
                    lo, hi = sh['width_range']
                    print(f'  {sh["id"]:<16} {sh["colour"]}  width {sh["width"]:<5g} ({lo:g}-{hi:g})'
                          f'  strokes {sh["strokes"]}  closed {sh["closed"]}  segments {sh["segments"]:<4}'
                          f' match {sh["match"]:.3f}')
                else:
                    print(f'  {sh["id"]:<16} {sh["colour"]}  area {sh["area"]:<10g} outlines {sh["outlines"]}'
                          f'  holes {sh["holes"]}  segments {sh["segments"]:<4} iou {sh["iou"]:.3f}')
            for warning in report['warnings']:
                print(f'  warning: {warning}')
    if args.json:
        print(json.dumps(reports[0] if len(reports) == 1 else reports, indent=2))
    return reports


if __name__ == '__main__':
    sys.exit(0 if main() else 1)
