#!/usr/bin/env python3
"""Make a texture tile seamlessly (and prove it), or check one that claims to.

    python make_seamless.py walnut.png                       # walnut.seamless.png + .preview.png + .json
    python make_seamless.py T_Stone_BC.png T_Stone_N.png T_Stone_H.png   # a set: identical cuts
    python make_seamless.py swatch.png --check               # measure only: swatch.check.preview.png/.json
    python make_seamless.py photo.jpg --physical-size 200cm  # also states the size the new tile covers

Default method `overlap`, no duplicated content. The low-frequency mismatch between opposite edges
is removed first (Moisan's periodic-plus-smooth decomposition, from band means so edge structure
is not smeared). The right band of the image is then laid over its left band and the two are
joined along a cheap cut: a minimum-cost path by dynamic programming, or with --seam graphcut a
cut of any shape (Kwatra 2003, needs PyMaxflow), steered by the differences of every map of the
set and, with a height map, towards low ground with the taller side winning near the cut. A
short feather and a tone levelling across the band finish the join. The overlap is nudged so the
new wrap is an ordinary pixel boundary, or snapped so a regular texture (bricks, stripes) meets
in phase. Every row now wraps by construction. Rows get the same treatment with a cut that
closes on itself across the left/right wrap. The cut-down tile is resampled back to the input
size with a periodic Lanczos resampler, so the content is magnified by W/(W-overlap); the report
says by how much and what physical size the tile now covers.

The identical cuts, masks, crop and resample are applied to every map, so BaseColor, Normal,
Roughness and Height stay registered; normal maps are blended as vectors and renormalised. Each
run writes a preview (before/after tiled 2x2, after tiled 3x3, the tile corner at 1:1, and a
landmark heat map) and a JSON report: wrap metrics per map, cut visibility, whether the tile
reads as stochastic, directional or regular (what runtime anti-repetition is safe), and the
patches most likely to read as repeats.

--layout brick / half-drop cut the edges those layouts join; --layout rotate makes a tile any side of
which meets any side of a neighbour under any quarter turn (one point-symmetric band on every side,
a 4-fold symmetric corner patch, both joined to the interior by minimum-cost cuts); the material must
then turn the normal map's tangent XY with each cell. --variants writes tiles that share their border.
"""
import os

os.environ.setdefault('OPENCV_IO_ENABLE_OPENEXR', '1')   # must precede the cv2 import

import argparse
import glob
import importlib.util
import json
import math
import re
import struct
import sys
from pathlib import Path

import cv2
import numpy as np

# Verdict thresholds, calibrated on the fixtures in Docs/Research/seamless-texture-methods.md. A wrap is a seam when its step is
# both larger than RANK_WARN of the image's own interior boundaries and more than WRAP_WARN
# (pixel step) or LOW_WARN (16 px band means: tone, lighting) times their mean; interior pairs
# alone reach 1.2-1.45x, so the ratio by itself cannot separate them. MACRO: blurred-luminance
# std / mean above which large blotches make tiling obvious. SEAM: path cost / band cost above
# which no quiet route through the overlap was found.
WRAP_WARN = 1.15
LOW_WARN = 1.5
RANK_WARN = 0.995
MACRO_WARN = 0.05
SEAM_WARN = 1.3
REGULAR_AT = 0.5
ANISOTROPY_AT = 2.0    # orientation-bin energy max/mean below which a stochastic texture counts as isotropic
DIRECTIONAL_AT = 0.45
LANDMARK_WARN = 5.0
GRID_WARN = 0.08       # shared-pattern contrast at a 16 px mip above which the repeat reads at a distance
ALIGN_GAIN = 0.75      # --align auto morphs the bands when that leaves at most this much of their difference
FLOW_SMOOTH = 6.0      # px: alignment flow smoothing
FLOW_CAP = 1 / 3       # largest alignment shift, as a share of the overlap
SNAP_GAIN = 0.8        # an overlap whose bands differ this much less than the request's is in phase
HEIGHT_FLIP = 0.25     # height difference (of the height map's range) that hands a pixel to the taller side

IMAGE_EXTS = {'.png', '.tif', '.tiff', '.jpg', '.jpeg', '.tga', '.exr', '.webp', '.bmp'}
NORMAL_TOKENS = {'n', 'nrm', 'nor', 'normal', 'normals', 'normalgl', 'normaldx', 'normalmap'}
DATA_TOKENS = {'r', 'm', 'h', 'ao', 'rough', 'roughness', 'metal', 'metallic', 'metalness', 'height',
               'disp', 'displacement', 'occlusion', 'mask', 'orm', 'arm', 'rma', 'mra', 'spec',
               'specular', 'gloss', 'glossiness', 'opacity', 'bump', 'cavity', 'curvature'}
HEIGHT_TOKENS = {'h', 'height', 'heightmap', 'disp', 'displacement', 'bump'}


# ---- files ---------------------------------------------------------------------------------

def read_tga(path):
    """Uncompressed or RLE true-colour/greyscale TGA (8/24/32-bit) -> uint8 BGR(A) array.
    OpenCV has no TGA codec."""
    raw = path.read_bytes()
    id_len, cmap_type, kind = raw[0], raw[1], raw[2]
    cmap_len, cmap_bits = struct.unpack_from('<H', raw, 5)[0], raw[7]
    width, height = struct.unpack_from('<HH', raw, 12)
    bpp, desc = raw[16], raw[17]
    if kind not in (2, 3, 10, 11) or bpp not in (8, 24, 32):
        raise SystemExit(f'{path}: only uncompressed or RLE true-colour/greyscale TGA (8, 24 or 32 bit) is read')
    n, pos = bpp // 8, 18 + id_len + (cmap_len * ((cmap_bits + 7) // 8) if cmap_type else 0)
    need = width * height * n
    if kind in (2, 3):
        pixels = raw[pos:pos + need]
    else:
        out = bytearray()
        while len(out) < need:
            head = raw[pos]
            count = (head & 0x7F) + 1
            if head & 0x80:
                out += raw[pos + 1:pos + 1 + n] * count
                pos += 1 + n
            else:
                out += raw[pos + 1:pos + 1 + count * n]
                pos += 1 + count * n
        pixels = bytes(out[:need])
    if len(pixels) < need:
        raise SystemExit(f'{path}: truncated TGA')
    img = np.frombuffer(pixels, np.uint8).reshape(height, width, n)
    if not desc & 0x20:
        img = img[::-1]
    if desc & 0x10:
        img = img[:, ::-1]
    return np.ascontiguousarray(img)


def write_tga(path, img):
    if img.dtype != np.uint8:
        raise SystemExit(f'{path}: TGA holds 8-bit channels only')
    img = img if img.ndim == 3 else img[..., None]
    h, w, c = img.shape
    head = struct.pack('<BBBHHBHHHHBB', 0, 0, 3 if c == 1 else 2, 0, 0, 0, 0, 0, w, h, 8 * c,
                       0x20 | (8 if c == 4 else 0))
    path.write_bytes(head + np.ascontiguousarray(img).tobytes())


def exr_is_half(path):
    """True when every channel in the EXR header is HALF, so the output is written as half too."""
    try:
        head = path.read_bytes()[:1 << 16]
        if head[:4] != b'\x76\x2f\x31\x01':
            return False
        i = 8
        while head[i] != 0:
            name_end = head.index(b'\0', i)
            type_end = head.index(b'\0', name_end + 1)
            size = struct.unpack_from('<i', head, type_end + 1)[0]
            if head[i:name_end] == b'channels':
                value, j, types = head[type_end + 5:type_end + 5 + size], 0, []
                while value[j] != 0:
                    j = value.index(b'\0', j) + 1
                    types.append(struct.unpack_from('<i', value, j)[0])
                    j += 16
                return bool(types) and all(t == 1 for t in types)
            i = type_end + 5 + size
    except (OSError, ValueError, IndexError, struct.error):
        pass
    return False


def read_image(path):
    """-> float32 H x W x C array of the file's values (integers scaled to 0-1) and its format."""
    ext = path.suffix.lower()
    if not path.is_file():
        raise SystemExit(f'{path}: no such file')
    data = read_tga(path) if ext == '.tga' else cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
    if data is None:
        hint = ''
        if ext == '.exr':
            hint = (f' (OpenCV {cv2.__version__} was built without OpenEXR; '
                    'pip install "opencv-python-headless>=4.8,<5")')
        raise SystemExit(f'{path}: cannot read this image{hint}')
    data = data if data.ndim == 3 else data[..., None]
    if data.dtype == np.uint8:
        values = data.astype(np.float32) / 255
    elif data.dtype == np.uint16:
        values = data.astype(np.float32) / 65535
    elif data.dtype in (np.float16, np.float32, np.float64):
        values = data.astype(np.float32)
    else:
        raise SystemExit(f'{path}: unsupported pixel type {data.dtype}')
    fmt = {'dtype': str(data.dtype), 'channels': int(data.shape[2]),
           'half': ext == '.exr' and exr_is_half(path)}
    return values, fmt


def write_image(path, values, fmt, quality):
    """Write file values (float32, 0-1 for integer formats) back in the source's depth and channels.
    Returns the fractions of integer values clipped by more than 2 levels at the bottom and the top
    of the range."""
    clipped = (0.0, 0.0)
    if fmt['dtype'] in ('uint8', 'uint16'):
        top = 255 if fmt['dtype'] == 'uint8' else 65535
        clipped = (float((values < -2.0 / 255).mean()), float((values > 1 + 2.0 / 255).mean()))
        data = np.floor(np.clip(values, 0, 1) * top + 0.5).astype(fmt['dtype'])
    else:
        data = values.astype(np.float32)
    data = data[..., 0] if data.shape[2] == 1 else data
    ext = path.suffix.lower()
    if ext == '.tga':
        write_tga(path, data)
        return clipped
    params = []
    if ext in ('.jpg', '.jpeg'):
        params = [cv2.IMWRITE_JPEG_QUALITY, quality]
    elif ext == '.webp':
        params = [cv2.IMWRITE_WEBP_QUALITY, 101]          # lossless
    elif ext == '.exr' and fmt['half']:
        params = [cv2.IMWRITE_EXR_TYPE, cv2.IMWRITE_EXR_TYPE_HALF]
    if ext != '.exr' and data.dtype == np.float32:
        raise SystemExit(f'{path}: float data needs an .exr output')
    try:
        ok = cv2.imwrite(str(path), data, params)
    except cv2.error as e:
        raise SystemExit(f'{path}: cannot write: {e}') from None
    if not ok:
        raise SystemExit(f'{path}: cannot write this format')
    return clipped


# ---- maps ----------------------------------------------------------------------------------

def name_tokens(path):
    stem = path.stem
    words = re.split(r'[^A-Za-z0-9]+', stem)
    camel = re.findall(r'[A-Z]+(?![a-z])|[A-Z]?[a-z]+|\d+', stem)
    return {t.lower() for t in words + camel if t}


class Map:
    """One texture map. `work` is what gets cut: file values, except that normal maps hold
    unit vectors (B, G, R = z, y, x) in their first three channels."""

    def __init__(self, path, kind, write=True):
        self.path, self.write, self.hidden = path, write, kind == 'mask'
        self.values, self.fmt = read_image(path)
        self.kind = 'data' if kind == 'color' and self.values.shape[2] < 3 else kind
        self.signed = self.kind == 'normal' and self.fmt['dtype'].startswith('float') \
            and float(self.values[..., :3].min()) < -0.01
        self.warnings = []
        if self.kind == 'normal' and self.values.shape[2] < 3:
            raise SystemExit(f'{path}: a normal map needs three channels (found {self.values.shape[2]})')
        tokens = name_tokens(path)
        self.green_down = not (tokens & {'gl', 'ogl', 'opengl', 'normalgl'})     # DirectX (Unreal) unless named GL
        self.work = self.decode(self.values)
        if self.kind == 'normal' and float(self.work[..., 0].mean()) < 0.3:
            self.warnings.append('treated as a normal map, but its blue (Z) channel is not mostly '
                                 'facing out; if it is not a normal map, pass --no-auto-normal')

    @property
    def tone(self):
        """Channels that carry colour or data; a fourth (alpha) channel is only cut and resampled."""
        c = self.values.shape[2]
        if self.kind == 'mask':
            return []
        return list(range(3)) if c == 4 else list(range(min(c, 3)))

    def decode(self, values):
        work = values.copy()
        if self.kind == 'normal':
            if not self.signed:
                work[..., :3] = work[..., :3] * 2 - 1
            work[..., :3] = unit(work[..., :3])
        return work

    def encode(self, work):
        values = work.copy()
        if self.kind == 'normal':
            values[..., :3] = unit(values[..., :3])
            if not self.signed:
                values[..., :3] = (values[..., :3] + 1) / 2
        return values


def unit(v):
    return v / np.maximum(np.linalg.norm(v, axis=2, keepdims=True), 1e-6)


def luminance(a):
    return a[..., 0] * 0.0722 + a[..., 1] * 0.7152 + a[..., 2] * 0.2126 if a.shape[2] >= 3 else a[..., 0]


# ---- filters -------------------------------------------------------------------------------

def gauss_kernel(sigma):
    if sigma <= 0:
        return np.ones((1, 1), np.float32)
    r = max(1, int(math.ceil(3 * sigma)))
    return cv2.getGaussianKernel(2 * r + 1, sigma).astype(np.float32)


def blur(a, sigma_y, sigma_x, wrap_y=False, wrap_x=False):
    """Gaussian blur of an H x W (x C<=4) array; a wrapped axis is treated as a repeating tile."""
    ky, kx = gauss_kernel(sigma_y), gauss_kernel(sigma_x)
    py, px = (len(ky) // 2 if wrap_y else 0), (len(kx) // 2 if wrap_x else 0)
    a = np.asarray(a, np.float32)
    if py or px:
        a = np.pad(a, ((py, py), (px, px)) + ((0, 0),) * (a.ndim - 2), mode='wrap')
    out = cv2.sepFilter2D(a, cv2.CV_32F, kx, ky, borderType=cv2.BORDER_REFLECT_101)
    out = out[..., None] if out.ndim < a.ndim else out
    return out[py:out.shape[0] - py, px:out.shape[1] - px]


def smooth1d(x, sigma):
    k = gauss_kernel(sigma).ravel().astype(np.float64)
    r = len(k) // 2
    return np.convolve(np.pad(x, r, mode='reflect'), k, mode='valid') if r else x


def periodic_smooth(u, channels, band=0):
    """Moisan's periodic-plus-smooth decomposition ("Periodic plus smooth image decomposition",
    J. Math. Imaging Vis. 2011): the smooth component s whose periodic Laplacian equals the jumps
    across opposite borders. u - s wraps without its edge mismatch and keeps its mean.
    With band > 0 (equalize) each jump is measured between the means of the last and first `band`
    rows (columns) and smoothed along the border by `band`, so s carries the lighting and colour
    drift only: a mortar line or a crease that happens to run along one edge is not spread
    through the image as a dark or light band, and pixel noise is not either."""
    h, w = u.shape[:2]
    q = np.arange(h)[:, None]
    r = np.arange(w // 2 + 1)[None, :]
    den = 2 * np.cos(2 * np.pi * q / h) + 2 * np.cos(2 * np.pi * r / w) - 4
    den[0, 0] = 1.0
    s = np.zeros_like(u)
    for c in channels:
        a = u[..., c].astype(np.float64)
        if band > 0:
            dy = smooth1d(a[-band:].mean(axis=0) - a[:band].mean(axis=0), band)
            dx = smooth1d(a[:, -band:].mean(axis=1) - a[:, :band].mean(axis=1), band)
        else:
            dy, dx = a[-1] - a[0], a[:, -1] - a[:, 0]
        v = np.zeros((h, w))
        v[0] += dy
        v[-1] -= dy
        v[:, 0] += dx
        v[:, -1] -= dx
        spectrum = np.fft.rfft2(v) / den
        spectrum[0, 0] = 0
        s[..., c] = np.fft.irfft2(spectrum, s=(h, w))
    return s


def flatten(work, channels, sigma):
    """Divide out large-scale luminance falloff (vignetting, a lit centre)."""
    big = blur(luminance(work), sigma, sigma)
    gain = float(big.mean()) / np.maximum(big, 1e-4)
    out = work.copy()
    out[..., channels] *= gain[..., None]
    return out


def resample_periodic(a, height, width, shift=0):
    """Resize a tile so it still tiles: Lanczos taps read across the wrap (explicit wrap padding;
    cv2.remap's BORDER_WRAP measured identical on OpenCV 4.14 and 5.0, padding keeps it
    independent of the build)."""
    h, w = a.shape[:2]
    if (h, w) == (height, width):
        return a
    pad = 8
    if shift:       # brick rows: above row 0 lies the last row moved left by the shift, below it the first moved right
        a = np.concatenate([np.roll(a[-pad:], -shift, axis=1), a, np.roll(a[:pad], shift, axis=1)], axis=0)
        padded = np.pad(a, ((0, 0), (pad, pad), (0, 0)), mode='wrap')
    else:
        padded = np.pad(a, ((pad, pad), (pad, pad), (0, 0)), mode='wrap')
    mx = ((np.arange(width) + 0.5) * (w / width) - 0.5 + pad).astype(np.float32)
    my = ((np.arange(height) + 0.5) * (h / height) - 0.5 + pad).astype(np.float32)
    map_x = np.ascontiguousarray(np.broadcast_to(mx[None, :], (height, width)))
    map_y = np.ascontiguousarray(np.broadcast_to(my[:, None], (height, width)))
    out = np.stack([cv2.remap(np.ascontiguousarray(padded[..., c]), map_x, map_y, cv2.INTER_LANCZOS4,
                              borderMode=cv2.BORDER_REFLECT_101) for c in range(a.shape[2])], axis=2)
    return out.astype(np.float32)


# ---- seams ---------------------------------------------------------------------------------
# Every pass is written for a vertical seam: a band of O columns, a cut running down the L rows.
# The row pass transposes the images and asks for a cut that closes on itself along L.

class CostModel:
    """Which maps steer the cut. By default every map of the set, each scaled to its own
    0.5-99.5 percentile range so none dominates by its units (colour, normal vectors, height);
    with --seam-from guide only the guide. With a height map, the cut also prefers low ground
    (crevices, mortar), so tall features stay whole: Materialize's "the taller side wins"."""

    def __init__(self, maps, guide, seam_from, height, route, blend):
        self.index = [guide] if seam_from == 'guide' else [i for i, m in enumerate(maps) if not m.hidden]
        self.guide = guide
        self.height = height if (route > 0 or blend) else None
        self.route, self.blend = route, blend
        self.height_channels = maps[height].tone if self.height is not None else None
        self.scale = {}
        for i in set(self.index) | {guide} | ({self.height} if self.height is not None else set()):
            lo, hi = np.percentile(maps[i].work[::4, ::4], [0.5, 99.5])
            self.scale[i] = (float(lo), 1.0 / max(float(hi - lo), 1e-6))

    def images(self, works):
        return [(works[i] - self.scale[i][0]) * self.scale[i][1] for i in self.index]

    def guide_image(self, works):
        """The guide in 0-1 units, one channel, for estimating band alignment."""
        lo, s = self.scale[self.guide]
        return ((works[self.guide] - lo) * s).mean(axis=2)

    def height_image(self, works):
        if self.height is None:
            return None
        lo, s = self.scale[self.height]
        return ((works[self.height][..., self.height_channels] - lo) * s).mean(axis=2)


def mismatch(pairs):
    """Mean over the steering maps of the per-pixel squared difference (mean over channels)
    between two bands, in each map's 0-1 units."""
    return sum(((a - b) ** 2).mean(axis=2) for a, b in pairs) / len(pairs)


def natural_difference(images, d):
    """The same squared difference between pixels d columns apart inside the images: how much the
    texture itself changes across a blend's width."""
    return sum(float(((g[:, d:] - g[:, :-d]) ** 2).mean()) for g in images) / len(images)


def seam_path(cost, lo, hi, start=None, closed=True):
    """Minimum-cost path down the rows of cost (L x O): one column per row, moving at most one
    column per row, inside [lo, hi]. With start it begins at that column and ends within one
    column of it, so it closes on itself when the rows wrap."""
    length, width = cost.shape
    c = cost.astype(np.float64).copy()
    c[:, :lo] = np.inf
    c[:, hi + 1:] = np.inf
    acc = c[0].copy()
    if start is not None:
        acc[:] = np.inf
        acc[start] = c[0, start]
    moves = np.zeros((length, width), np.int8)
    inf = np.array([np.inf])
    cols = np.arange(width)
    for i in range(1, length):
        options = np.stack([acc, np.concatenate([inf, acc[:-1]]), np.concatenate([acc[1:], inf])])
        k = np.argmin(options, axis=0)                 # ties keep the path straight
        moves[i] = k
        acc = c[i] + options[k, cols]
    if start is not None and closed:
        keep = np.full(width, np.inf)
        keep[max(lo, start - 1):min(hi, start + 1) + 1] = 0
        acc = acc + keep
    j = int(np.argmin(acc))
    path = np.empty(length, np.int32)
    path[-1] = j
    for i in range(length - 1, 0, -1):
        j += (0, -1, 1)[moves[i, j]]
        path[i - 1] = j
    return path


def find_seam(cost, feather, cyclic):
    lo, hi = feather, cost.shape[1] - 1 - feather
    if not cyclic:
        return seam_path(cost, lo, hi)
    length = len(cost)
    # an unconstrained path over the cost repeated twice has settled by the second copy: where it
    # crosses the start is a good place for the closed path to begin
    s = int(seam_path(np.concatenate([cost, cost]), lo, hi)[length])
    best = None
    for start in sorted({min(hi, max(lo, s + d)) for d in (-1, 0, 1)}):
        path = seam_path(cost, lo, hi, start)
        total = float(cost[np.arange(length), path].sum())
        if best is None or total < best[0]:
            best = (total, path)
    return best[1]


def seam_mask(path, width, feather):
    """Weight of the right-hand source per band pixel: 0 left of the path, 1 right of it, a linear
    ramp of 2*feather px across it. Shared by every map, so the cut is identical."""
    j = np.arange(width, dtype=np.float32)[None, :]
    if feather <= 0:
        return (j > path[:, None]).astype(np.float32)
    return np.clip((j - path[:, None]) / (2 * feather) + 0.5, 0, 1).astype(np.float32)


def drop_islands(right, feather, cyclic):
    """Give every piece of the label map that does not reach its own side to the other side."""
    length, width = right.shape
    copies = 3 if cyclic else 1                         # a piece may continue across the wrap
    for label, cols in ((True, slice(width - 1 - feather, width)), (False, slice(0, feather + 1))):
        mask = np.tile((right == label).astype(np.uint8), (copies, 1))
        comp = cv2.connectedComponents(mask, connectivity=4)[1]
        keep = np.unique(comp[:, cols][mask[:, cols] > 0])
        middle = comp[(copies // 2) * length:(copies // 2 + 1) * length]
        right = np.where((middle > 0) & ~np.isin(middle, keep), not label, right)
    return right


def graph_cut(cost, feather, cyclic):
    """Kwatra et al. 2003 ("Graphcut Textures"): the cheapest cut of any shape through the band,
    edge capacity cost(p) + cost(q), the first and last feather+1 columns tied to their sides,
    rows wrapped when cyclic. Returns the label map, True where the right-hand band is kept."""
    import maxflow
    length, width = cost.shape
    c = cost.astype(np.float64) + 1e-9
    g = maxflow.Graph[float]()
    ids = g.add_grid_nodes((length, width))
    across = np.zeros_like(c)
    across[:, :-1] = c[:, :-1] + c[:, 1:]
    along = c + np.roll(c, -1, axis=0)
    g.add_grid_edges(ids, weights=across, structure=np.array([[0, 0, 0], [0, 0, 1], [0, 0, 0]]), symmetric=True)
    g.add_grid_edges(ids, weights=along, structure=np.array([[0, 0, 0], [0, 0, 0], [0, 1, 0]]), symmetric=True,
                     periodic=[bool(cyclic), False])
    big = float(c.sum()) * 4 + 1
    source, sink = np.zeros_like(c), np.zeros_like(c)
    source[:, :feather + 1] = big
    sink[:, width - 1 - feather:] = big
    g.add_grid_tedges(ids, source, sink)
    g.maxflow()
    return drop_islands(g.get_grid_segments(ids), feather, cyclic)


def taller_wins(weight, distance, heights, feather):
    """Materialize's height-driven blend near the cut: within a zone round it, the side whose
    height map is higher there wins, so a brick or stone from one band covers the mortar or
    crevice of the other instead of being sliced at an arbitrary line. Exact at the band edges."""
    a, b = heights
    width = weight.shape[1]
    zone = max(8.0, 2.0 * feather)
    j = np.arange(width, dtype=np.float32)[None, :]
    near = np.clip(1 - distance / zone, 0, 1) * np.clip(np.minimum(j, width - 1 - j) / zone, 0, 1)
    return np.clip(weight + (b - a) * near / HEIGHT_FLIP, 0, 1).astype(np.float32)


def cut(mis, route, heights, feather, cyclic, seam):
    """Choose the cut through one band. -> weight of the right-hand band, stats inputs."""
    length, width = mis.shape
    steer = blur(mis, 1.0, max(0.5, feather / 2), wrap_y=cyclic)
    if route is not None:
        steer = steer + route * float(steer.mean())
    if seam == 'graphcut':
        right = graph_cut(steer, feather, cyclic)
        weight = right.astype(np.float32)
        if feather > 0:
            weight = np.clip(blur(weight, feather / 3, feather / 3, wrap_y=cyclic), 0, 1)
        edge = right[:, 1:] != right[:, :-1]
        on = np.zeros_like(right)
        on[:, 1:] |= edge
        on[:, :-1] |= edge
        path_cost, cols = float(mis[on].mean()), np.nonzero(on)[1]
        if heights is not None:
            copies = 3 if cyclic else 1
            far = np.tile((~on).astype(np.uint8), (copies, 1))
            distance = cv2.distanceTransform(far, cv2.DIST_L2, 3)[(copies // 2) * length:(copies // 2 + 1) * length]
            weight = taller_wins(weight, distance, heights, feather)
    else:
        path = find_seam(steer, feather, cyclic)
        weight = seam_mask(path, width, feather)
        path_cost, cols = float(mis[np.arange(length), path].mean()), path
        if heights is not None:
            distance = np.abs(np.arange(width, dtype=np.float32)[None, :] - path[:, None])
            weight = taller_wins(weight, distance, heights, feather)
    return weight, path_cost, cols


def seam_stats(mis, path_cost, cols, natural, axis, offset, seam):
    """ratio: mean difference on the cut / over the whole band (how quiet a route was found).
    visibility: RMS difference on the cut / RMS difference of pixels a blend-width apart inside
    the texture; above SEAM_WARN the blend joins things that do not occur side by side."""
    band = float(mis.mean())
    return {'axis': axis, 'seam': seam, 'band_px': [int(offset), int(offset + mis.shape[1])],
            'path_cost': round(path_cost, 6), 'band_cost': round(band, 6),
            'ratio': round(path_cost / band, 3) if band > 0 else 0.0,
            'visibility': round(math.sqrt(path_cost / natural), 3) if natural > 0 else 0.0,
            'path_range': [int(cols.min() + offset), int(cols.max() + offset)]}


def stitch(a, b, weight, level, channels, cyclic):
    """Join band a (kept left of the cut, exact at column 0) to band b (kept right of it, exact at
    the last column). Levelling spreads the remaining tone difference between the bands as a
    linear ramp across the whole band instead of leaving it as a step at the cut. The difference
    is the mean over the band's width, smoothed along the seam by that width, so it follows
    lighting, never single bricks or grain lines."""
    out = a + (b - a) * weight[..., None]
    if level and channels:
        length, width = weight.shape
        step = blur((a - b).mean(axis=1), width, 0, wrap_y=cyclic).reshape(length, -1)   # L x C
        ramp = weight - np.arange(width, dtype=np.float32)[None, :] / (width - 1)
        out[..., channels] += step[:, None, channels] * ramp[..., None]
    return out


def height_route(heights, strength):
    """Extra cut cost where the two bands are high, mean `strength` (times the mean difference
    cost, applied in cut), so the cut prefers crevices, mortar and gaps between stones."""
    if strength <= 0:
        return None
    mean = (heights[0] + heights[1]) / 2
    return strength * mean / max(float(mean.mean()), 1e-6)


def ordinary_pair(images, centre, spread, multiple=1):
    """Of the column pairs (k-1 | k) with k within spread of centre, the one whose step is closest
    to the image's median column step. That pair becomes the new wrap, so it should be an
    ordinary boundary, never a JPEG block edge or a crease that happens to sit there."""
    d = sum(np.abs(np.diff(g, axis=1)).mean(axis=(0, 2)) for g in images)
    med = max(float(np.median(d)), 1e-12)
    choices = [k for k in range(centre - spread, centre + spread + 1) if k % multiple == 0] or [centre]
    return min(choices, key=lambda k: (abs(math.log(max(float(d[k - 1]), 1e-12) / med)), abs(k - centre)))


def choose_overlap(images, o, spread, least, limit, multiple=1, shift=0, cyclic=False):
    """Where the overlap lands. An exact pixel request is kept. Otherwise, within 0.75-1.5x the
    request, an overlap whose bands differ clearly less than the requested one's (SNAP_GAIN) is
    taken: the cut-down tile is then a whole number of the texture's repeats, so bricks, planks
    and stripes meet in phase. Failing that, the request moves up to `spread` px so the new wrap
    is an ordinary pixel boundary. -> overlap, how it was chosen."""
    width = images[0].shape[1]
    if not spread:
        if (width - o) % multiple:
            return o + (width - o) % multiple, 'exact, widened so the layout shift is whole pixels'
        return o, 'exact'
    sub = [np.ascontiguousarray(g[::2]) for g in images]
    lo, hi = max(o - (o + 3) // 4, least), min(int(limit), o + o // 2)
    candidates = [c for c in range(lo, hi + 1) if (width - c) % multiple == 0]
    near = min(candidates, key=lambda c: abs(c - o))
    costs = {c: float(mismatch([band_pair(g, c, None, cyclic, shift // 2) for g in sub]).mean()) for c in candidates}
    best = min(costs, key=lambda c: (costs[c], abs(c - o)))
    if costs[near] > 0 and costs[best] < SNAP_GAIN * costs[near]:
        return best, f'in phase with the texture repeat (band difference {costs[best] / costs[near]:.0%} of the request)'
    return width - ordinary_pair(images, width - o, spread, multiple), 'ordinary wrap pair'


def band_flow(guide, o, cyclic, shift=0):
    """Smooth displacement F between the two bands of the guide (Farneback optical flow), such
    that band a at p looks like band b at p + F. Rows wrap when cyclic. -> L x o x 2 (dx, dy)."""
    width = guide.shape[1]
    a, b = np.roll(guide, -shift, axis=0)[:, width - o:], guide[:, :o]
    pad = 48 if cyclic else 0
    if pad:
        a, b = (np.pad(x, ((pad, pad), (0, 0)), mode='wrap') for x in (a, b))

    def to8(x):
        return np.ascontiguousarray(np.clip(x * 255, 0, 255).astype(np.uint8))
    flow = cv2.calcOpticalFlowFarneback(to8(a), to8(b), None, 0.5, 4, 31, 5, 7, 1.5,
                                        cv2.OPTFLOW_FARNEBACK_GAUSSIAN)
    flow = flow[pad:flow.shape[0] - pad] if pad else flow
    flow = blur(flow, FLOW_SMOOTH, FLOW_SMOOTH, wrap_y=cyclic)
    size = np.linalg.norm(flow, axis=2, keepdims=True)
    return (flow * np.minimum(1, (o * FLOW_CAP) / np.maximum(size, 1e-6))).astype(np.float32)


def band_pair(full, o, flow, cyclic, shift=0):
    """The two overlapping bands of an image. With a flow they are morphed towards each other
    across the band: a by -t*F, b by +(1-t)*F, t running 0 to 1 over the band, so a is exact at
    the first column, b at the last, and in between both show the same structure. With a layout
    shift (brick rows), band a is taken from the image moved along the seam by that shift: those are
    the edges the layout puts side by side."""
    width = full.shape[1]
    source_a = np.roll(full, -shift, axis=0) if shift else full
    if flow is None:
        return source_a[:, width - o:], full[:, :o]
    length = full.shape[0]
    pad = int(np.ceil(np.abs(flow).max())) + 4
    extra = ((0, 0),) * (full.ndim - 2)

    def padded(img):
        img = np.pad(img, ((pad, pad), (0, 0)) + extra, mode='wrap' if cyclic else 'reflect')
        return np.ascontiguousarray(np.pad(img, ((0, 0), (pad, pad)) + extra, mode='reflect'), np.float32)
    pad_a, pad_b = padded(source_a), padded(full)
    t = np.linspace(0, 1, o, dtype=np.float32)[None, :]
    j = np.arange(o, dtype=np.float32)[None, :]
    rows = np.arange(length, dtype=np.float32)[:, None] + pad
    fx, fy = flow[..., 0], flow[..., 1]

    def sample(img, x, y):
        out = cv2.remap(img, np.ascontiguousarray(x, np.float32), np.ascontiguousarray(y, np.float32),
                        cv2.INTER_CUBIC, borderMode=cv2.BORDER_REFLECT_101)
        return out.reshape(length, o, *full.shape[2:])
    a = sample(pad_a, width - o + j - t * fx + pad, rows - t * fy)
    b = sample(pad_b, j + (1 - t) * fx + pad, rows + (1 - t) * fy)
    return a, b


def overlap_pass(works, maps, cm, o, opts, cyclic, axis):
    """Lay the right band over the left band and cut between them; width shrinks by o."""
    width = works[0].shape[1]
    images, height = cm.images(works), cm.height_image(works)
    shift, multiple = opts.get('shift', 0), opts.get('multiple', 1)
    o, chosen = choose_overlap(images, o, opts['spread'], 2 * opts['feather'] + 3, opts['limit'](width),
                               multiple, shift, cyclic)
    plain = mismatch([band_pair(g, o, None, cyclic, shift) for g in images])
    flow, gain, mis = None, None, plain
    if opts['align'] != 'off':
        flow = band_flow(cm.guide_image(works), o, cyclic, shift)
        aligned = mismatch([band_pair(g, o, flow, cyclic, shift) for g in images])
        gain = float(aligned.mean()) / max(float(plain.mean()), 1e-12)
        if opts['align'] == 'auto' and gain > ALIGN_GAIN:
            flow = None
        else:
            mis = aligned
    route, heights = None, None
    if height is not None:
        heights = band_pair(height, o, flow, cyclic, shift)
        route = height_route(heights, cm.route)
        heights = heights if opts['height_blend'] else None
    natural = natural_difference(images, max(1, 2 * opts['feather']))
    weight, path_cost, cols = cut(mis, route, heights, opts['feather'], cyclic, opts['seam'])
    out = []
    for a, m in zip(works, maps):
        band = stitch(*band_pair(a, o, flow, cyclic, shift), weight, opts['level'], m.tone, cyclic)
        out.append(np.ascontiguousarray(np.concatenate([band, a[:, o:width - o]], axis=1)))
    stats = seam_stats(mis, path_cost, cols, natural, axis, 0, opts['seam'])
    stats['overlap_choice'] = chosen
    stats['aligned'] = flow is not None
    stats['align_gain'] = round(gain, 3) if gain is not None else None
    if flow is not None:
        stats['align_shift_px'] = round(float(np.linalg.norm(flow, axis=2).max()), 1)
    return out, [stats], o


def offset_pass(works, maps, cm, o, opts, cyclic, axis):
    """Classic half offset: roll by half so the old edges meet in the middle, then lay the
    unrolled middle back over that meeting with two cut bands at 1/4 and 3/4. Keeps the size;
    the middle half of the source appears twice."""
    width = works[0].shape[1]
    images, height = cm.images(works), cm.height_image(works)
    shift = width - ordinary_pair(images, width // 2, opts['spread']) if opts['spread'] else width // 2
    rolled = [np.roll(a, shift, axis=1) for a in works]
    rolled_images = [np.roll(g, shift, axis=1) for g in images]
    natural = natural_difference(images, max(1, 2 * opts['feather']))
    out = [r.copy() for r in rolled]
    stats = []
    # left band: the rolled image on the left, the original on the right; right band: the reverse
    for start, left, right in ((width // 4 - o // 2, 'rolled', 'source'), ((3 * width) // 4 - o // 2, 'source', 'rolled')):
        band = slice(start, start + o)
        pick = {'rolled': rolled_images, 'source': images}
        mis = mismatch([(a[:, band], b[:, band]) for a, b in zip(pick[left], pick[right])])
        route, heights = None, None
        if height is not None:
            hs = {'rolled': np.roll(height, shift, axis=1), 'source': height}
            heights = (hs[left][:, band], hs[right][:, band])
            route = height_route(heights, cm.route)
            heights = heights if opts['height_blend'] else None
        weight, path_cost, cols = cut(mis, route, heights, opts['feather'], cyclic, opts['seam'])
        for i, m in enumerate(maps):
            src = {'rolled': rolled[i], 'source': works[i]}
            out[i][:, band] = stitch(src[left][:, band], src[right][:, band], weight, opts['level'], m.tone, cyclic)
        stats.append(seam_stats(mis, path_cost, cols, natural, axis, start, opts['seam']))
    middle = slice(width // 4 - o // 2 + o, (3 * width) // 4 - o // 2)
    for i in range(len(maps)):
        out[i][:, middle] = works[i][:, middle]
    return out, stats, o


# ---- the tiling: grid, or brick with each row of tiles moved right by s px ---------------------
# Half-drop runs as brick on transposed images. A tile T makes the field F(x, y) =
# T((x - r*s) mod W, y mod H) with r = floor(y / H); s = 0 is the plain grid.

def translate(a, dy, dx, s=0):
    """The tile seen from (dy, dx) further on in the field: out[y, x] = F(y + dy, x + dx)."""
    if not s:
        return np.roll(a, (-dy, -dx), axis=(0, 1))
    h, w = a.shape[:2]
    rows, yy = np.divmod(np.arange(h) + dy, h)
    xx = (np.arange(w)[None, :] + dx - rows[:, None] * s) % w
    return a[yy[:, None], xx]


def field_at(a, ys, xs, s=0):
    """Field values at integer positions (ys, xs), which may lie outside the tile."""
    h, w = a.shape[:2]
    rows, yy = np.divmod(ys, h)
    return a[yy, (xs - rows * s) % w]


def lattice_rows(w, s):
    """How many rows of tiles before the field repeats on a plain grid (the super-tile height)."""
    return 1 if not s else w // math.gcd(w, s % w or w)


def super_tile(a, s):
    """A rectangle of whole rows of tiles that repeats on a plain grid (plain UVs): row r is the tile
    moved right by r*s."""
    if not s:
        return a
    return np.concatenate([np.roll(a, r * s, axis=1) for r in range(lattice_rows(a.shape[1], s))], axis=0)


def embed(mask, q):
    """A tile-sized mask placed in the first row of a q-row super-tile, zeros elsewhere."""
    return mask if q == 1 else np.concatenate([mask] + [np.zeros_like(mask)] * (q - 1), axis=0)


# ---- landmarks: find and transplant ----------------------------------------------------------

def correlate(a, b):
    """Circular cross-correlation over the tile: out[d] = sum_p a[p] * b[p + d]."""
    return np.real(np.fft.ifft2(np.conj(np.fft.fft2(a)) * np.fft.fft2(b)))


def disk(r):
    r = max(1, int(round(r)))
    return cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * r + 1, 2 * r + 1))


def outlier_object(lum, region, centre, reach, s=0):
    """The bright or dark objects a landmark region touches: connected pixels more than 2 robust
    standard deviations from the median (on whichever side covers more of the region), at least a
    tenth inside the region and no wider than `reach`. -> bool mask (tile size) or None."""
    h, w = lum.shape
    to_c = (centre[0] - h // 2, centre[1] - w // 2)
    centred, inside = translate(lum, *to_c, s), translate(region, *to_c, s)
    dev = centred - np.median(lum)
    sigma = 1.4826 * max(float(np.median(np.abs(lum - np.median(lum)))), 1e-6)
    best = None
    for sign in (1, -1):
        comp = cv2.connectedComponents((dev * sign > 2 * sigma).astype(np.uint8), connectivity=8)[1]
        size = np.bincount(comp.ravel())
        within = np.bincount(comp[inside], minlength=len(size))
        keep = []
        for k in range(1, len(size)):
            if within[k] > 0 and within[k] >= 0.1 * size[k]:
                ys, xs = np.nonzero(comp == k)
                if np.ptp(ys) <= reach and np.ptp(xs) <= reach:
                    keep.append(k)
        obj = np.isin(comp, keep)
        if keep and (best is None or within[keep].sum() > best[0]):
            best = (int(within[keep].sum()), obj)
    if best is None:
        return None
    obj = cv2.dilate(best[1].astype(np.uint8), disk(4)).astype(bool)
    return translate(obj, -to_c[0], -to_c[1], s)


def landmark_regions(heat, spots, h, w, count, lum=None, s=0):
    """Regions round the top landmark spots: the hot area (heat >= max(1.8, 0.6 x the spot's score))
    connected to the spot, a little enlarged, between 1/10 and 1/3 of the tile across; with the
    tile's luminance, a bright or dark object at the spot is covered whole.
    -> list of (region, spot, box, centre (y, x))."""
    gh, gw = heat.shape
    hot = cv2.resize(heat, (w, h), interpolation=cv2.INTER_LINEAR)
    lo, hi = max(h, w) / 10, max(h, w) / 3
    regions = []
    for spot in spots:
        if len(regions) == count:
            break
        mask = hot >= max(1.8, 0.6 * spot['score'])
        to_c = (spot['y'] % h - h // 2, spot['x'] % w - w // 2)
        centred = translate(mask, *to_c, s).astype(np.uint8)
        comp = cv2.connectedComponents(centred, connectivity=8)[1]
        label = comp[h // 2, w // 2]
        area = (comp == label) if label else np.zeros_like(centred, bool)
        ys, xs = np.nonzero(area)
        if len(ys):
            bh, bw = ys.max() - ys.min() + 1, xs.max() - xs.min() + 1
            cy, cx = (ys.max() + ys.min()) / 2, (xs.max() + xs.min()) / 2
        else:
            bh = bw = lo
            cy, cx = h / 2, w / 2
        bh, bw = float(np.clip(bh * 1.4, lo, hi)), float(np.clip(bw * 1.4, lo, hi))
        region = np.zeros((h, w), np.uint8)
        cv2.ellipse(region, (int(round(cx)), int(round(cy))), (int(bw / 2), int(bh / 2)), 0, 0, 360, 1, -1)
        region = translate(region.astype(bool), -to_c[0], -to_c[1], s)
        centre = (int(round(spot['y'] + cy - h // 2)) % h, int(round(spot['x'] + cx - w // 2)) % w)
        if lum is not None:
            obj = outlier_object(lum, region, centre, int(hi), s)
            if obj is not None:
                region |= obj
        if any((region & other).sum() > 0.3 * min(region.sum(), other.sum()) for other, _, _, _ in regions):
            continue                                    # the same feature as a region already chosen
        box = [int(round(spot['x'] + cx - w // 2 - bw / 2)), int(round(spot['y'] + cy - h // 2 - bh / 2)),
               int(round(bw)), int(round(bh))]
        regions.append((region, spot, box, centre))
    return regions


def mask_cut(cost, source, sink):
    """Cheapest cut between pixels tied to the source (keep) and to the sink (replace), edge
    capacity cost(p) + cost(q); pieces that reach neither side go to the other. True = replace."""
    import maxflow
    c = cost.astype(np.float64) + 1e-9
    g = maxflow.Graph[float]()
    ids = g.add_grid_nodes(c.shape)
    across, down = np.zeros_like(c), np.zeros_like(c)
    across[:, :-1] = c[:, :-1] + c[:, 1:]
    down[:-1] = c[:-1] + c[1:]
    g.add_grid_edges(ids, weights=across, structure=np.array([[0, 0, 0], [0, 0, 1], [0, 0, 0]]), symmetric=True)
    g.add_grid_edges(ids, weights=down, structure=np.array([[0, 0, 0], [0, 0, 0], [0, 1, 0]]), symmetric=True)
    big = float(c.sum()) * 4 + 1
    g.add_grid_tedges(ids, source * big, sink * big)
    g.maxflow()
    label = g.get_grid_segments(ids)
    for side, tie in ((True, sink), (False, source)):
        mask = (label == side).astype(np.uint8)
        comp = cv2.connectedComponents(mask, connectivity=4)[1]
        keep = np.unique(comp[tie & (mask > 0)])
        label = np.where((comp > 0) & ~np.isin(comp, keep), not side, label)
    return label


def wrapped_components(mask, s=0):
    """Connected regions of a mask on the field: moved so an empty row and column sit on the edges
    first, so a region crossing the wrap stays one region. -> list of (region, (cy, cx))."""
    h, w = mask.shape
    rows, cols = np.nonzero(~mask.any(axis=1))[0], np.nonzero(~mask.any(axis=0))[0]
    dy = int(rows[0]) if len(rows) else 0
    dx = int(cols[0]) if len(cols) else 0
    moved = translate(mask, dy, dx, s).astype(np.uint8)
    count, comp = cv2.connectedComponents(moved, connectivity=8)
    out = []
    for k in range(1, count):
        ys, xs = np.nonzero(comp == k)
        cy, cx = (int(round(ys.mean())) + dy), (int(round(xs.mean())) + dx)
        rows_moved = cy // h
        out.append((translate(comp == k, -dy, -dx, s), (cy % h, (cx - rows_moved * s) % w)))
    return out


def transplant(works, maps, cm, region, centre, taken, heat_full, use_graphcut, feather, s=0):
    """Replace one region of the (periodic) tile with a donor patch from elsewhere in the wrapped
    field: the donor whose surroundings match the region's surroundings best and that is itself
    unremarkable, joined along a minimum-cost boundary (through low ground, the taller side winning
    near it, when the set has a height map). The same donor offset and weights go to every map.
    -> new works, record."""
    h, w = region.shape
    cy, cx = centre
    to_c = (cy - h // 2, cx - w // 2)                     # translate(., *to_c) centres the region
    area = int(region.sum())
    margin = max(12, int(0.5 * math.sqrt(area)))
    soft = max(1.5, feather / 2)
    region_c = translate(region, *to_c, s)
    near_c = cv2.dilate(region_c.astype(np.uint8), disk(margin)).astype(bool)
    ring_c = near_c & ~region_c
    back = (-to_c[0], -to_c[1])
    near, ring = translate(near_c, *back, s), translate(ring_c, *back, s)
    images = cm.images(works)
    q = lattice_rows(w, s)
    f = max(1, int(round(max(h, w) / 512)))

    def small(a):
        return cv2.resize(a.astype(np.float32), (w // f, (q * h) // f), interpolation=cv2.INTER_AREA)
    ring_s, region_s = small(embed(ring, q)), small(embed(region, q))
    blocked = small(super_tile(cv2.dilate(taken.astype(np.uint8), disk(margin)).astype(np.uint8), s)
                    | super_tile(near.astype(np.uint8), s)) > 0.01
    ssd = np.zeros(ring_s.shape)
    for g in images:
        for c in range(g.shape[2]):
            t = small(super_tile(g[..., c], s))
            ssd += correlate(ring_s, t * t) - 2 * correlate(ring_s * t, t) + float((ring_s * t * t).sum())
    ssd = np.maximum(ssd, 0) / max(float(ring_s.sum()), 1e-6)
    donor_heat = correlate(region_s, small(super_tile(heat_full, s))) / max(float(region_s.sum()), 1e-6)
    valid = ~(correlate(small(embed(near, q)), blocked.astype(np.float32)) > 0.5)
    if not valid.any():
        return works, None
    score = ssd / max(float(np.median(ssd[valid])), 1e-12) + 0.5 * np.maximum(donor_heat - 1, 0)
    score[~valid] = np.inf
    dy, dx = (int(v) * f for v in np.unravel_index(int(np.argmin(score)), score.shape))
    heat_at = float(donor_heat[dy // f, dx // f])
    best = None                       # refine at full resolution round the coarse offset
    ys, xs = np.nonzero(ring)
    for ry in range(-f + 1, f):
        for rx in range(-f + 1, f):
            sy, sx = (dy + ry) % (q * h), (dx + rx) % w
            cost = sum(float(((field_at(g, ys + sy, xs + sx, s) - g[ys, xs]) ** 2).sum()) for g in images)
            if best is None or cost < best[0]:
                best = (cost, sy, sx)
    _, dy, dx = best
    # the join, in a window centred on the region so it never straddles the wrap
    rows, cols = np.nonzero(cv2.dilate(near_c.astype(np.uint8), disk(3 * soft + 2)))
    win = (slice(rows.min(), rows.max() + 1), slice(cols.min(), cols.max() + 1))
    sink = cv2.dilate(region_c.astype(np.uint8), disk(2 * soft + 1)).astype(bool)[win]
    source = ~near_c[win]

    def centred(a, donor=False):
        a = translate(a, dy, dx, s) if donor else a
        return translate(a, *to_c, s)
    height = cm.height_image(works)
    heights = None if height is None else (centred(height)[win], centred(height, True)[win])
    if use_graphcut:
        steer = mismatch([(centred(g)[win], centred(g, True)[win]) for g in images])
        route = height_route(heights, cm.route) if heights is not None else None
        if route is not None:
            steer = steer + route * float(steer.mean())
        label = mask_cut(steer, source, sink)
    else:
        label = cv2.dilate(region_c.astype(np.uint8), disk(margin / 2)).astype(bool)[win]
    weight = np.clip(cv2.GaussianBlur(label.astype(np.float32), (0, 0), soft), 0, 1)
    if heights is not None and cm.blend:
        edge = label ^ cv2.erode(label.astype(np.uint8), disk(1)).astype(bool)
        distance = cv2.distanceTransform((~edge).astype(np.uint8), cv2.DIST_L2, 3)
        nearby = np.clip(1 - distance / max(8.0, 2.0 * feather), 0, 1)
        weight = np.clip(weight + (heights[1] - heights[0]) * nearby / HEIGHT_FLIP, 0, 1)
    weight[source & ~cv2.dilate(label.astype(np.uint8), disk(3 * soft)).astype(bool)] = 0
    weight[region_c[win]] = 1                                # the region itself is replaced entirely
    out = []
    for a in works:
        a_c, d_c = centred(a).copy(), centred(a, True)
        a_c[win] = a_c[win] + (d_c[win] - a_c[win]) * weight[..., None]
        out.append(np.ascontiguousarray(translate(a_c, *back, s)))
    ys, xs = np.nonzero(region_c)
    record = {'centre': [int(cx), int(cy)], 'size_px': [int(np.ptp(xs) + 1), int(np.ptp(ys) + 1)], 'area_px': area,
              'donor_offset': [int(dx if dx <= w // 2 else dx - w), int(dy if dy <= q * h // 2 else dy - q * h)],
              'donor_heat': round(heat_at, 2), 'boundary': 'graphcut' if use_graphcut else 'feather'}
    return out, record


def remove_landmarks(works, maps, cm, guide, mask_index, auto, feather, s=0):
    """--remove-mask regions, then the --auto-remove N worst landmarks, one at a time on the tile."""
    h, w = works[0].shape[:2]
    gm = maps[guide]
    heat, spots = landmarks(gm.encode(works[guide]), gm, count=max(5, 4 * auto))
    heat_full = cv2.resize(heat, (w, h), interpolation=cv2.INTER_LINEAR)
    targets = []
    if mask_index is not None:
        targets += [(r, None, c) for r, c in wrapped_components(works[mask_index][..., 0] > 0.5, s) if r.sum() >= 16]
    if auto:
        lum = detail_channels(gm.encode(works[guide]), gm).mean(axis=2)
        candidates = [spot for spot in spots if spot['score'] >= 1.8]
        targets += [(r, spot, c) for r, spot, _, c in landmark_regions(heat, candidates, h, w, auto, lum, s)]
    taken = np.zeros((h, w), bool)
    for region, _, _ in targets:
        taken |= region
    use_graphcut = importlib.util.find_spec('maxflow') is not None
    records = []
    for region, spot, centre in targets:
        works, record = transplant(works, maps, cm, region, centre, taken, heat_full, use_graphcut, feather, s)
        if record is None:
            records.append({'skipped': 'no donor far enough from the other regions'})
            continue
        record['landmark_score'] = spot['score'] if spot else None
        record['source'] = 'auto' if spot else 'mask'
        records.append(record)
    after_heat, after_spots = landmarks(gm.encode(works[guide]), gm)
    spots, after_spots = with_boxes(heat, spots[:5], h, w), with_boxes(after_heat, after_spots, h, w)
    return works, {'regions': records, 'landmarks_before': spots, 'landmarks_after': after_spots,
                   'boundary': 'graphcut' if use_graphcut else 'feather (pip install PyMaxflow for a cut)'}, \
        [r for r, _, _ in targets], heat, after_heat


def ring_offsets(images, ring, count, keep_away, f, s=0):
    """The `count` field offsets d whose view of the field matches the tile best over `ring`
    (circular correlation on the super-tile, images reduced by f), each at least `keep_away` (a share
    of the tile per axis) from every copy of the tile and from each other.
    -> list of (dy, dx) in full-size px, cost."""
    h, w = ring.shape
    q = lattice_rows(w, s)

    def small(a):
        return cv2.resize(a.astype(np.float32), (w // f, (q * h) // f), interpolation=cv2.INTER_AREA)
    ring_s = small(embed(ring, q))
    ssd = np.zeros(ring_s.shape)
    for g in images:
        for c in range(g.shape[2]):
            t = small(super_tile(g[..., c], s))
            ssd += correlate(ring_s, t * t) - 2 * correlate(ring_s * t, t) + float((ring_s * t * t).sum())
    ssd = np.maximum(ssd, 0) / max(float(ring_s.sum()), 1e-6)
    sh, sw = ssd.shape
    th = sh / q                                            # one tile's height in the reduced super-tile

    def away(y, x):
        """At least keep_away from (y, x) on the reduced super-tile, per axis in tile units."""
        ry = np.minimum(np.abs(np.arange(sh) - y), sh - np.abs(np.arange(sh) - y))[:, None] / th
        rx = np.minimum(np.abs(np.arange(sw) - x), sw - np.abs(np.arange(sw) - x))[None, :] / sw
        return np.maximum(ry, rx) >= keep_away
    allowed = np.ones_like(ssd, bool)
    for r in range(q):                                     # every copy of the tile in the super-tile
        allowed &= away(r * th, (r * s % w) / f)
    chosen = []
    for _ in range(count):
        cost = np.where(allowed, ssd, np.inf)
        if not np.isfinite(cost).any():
            break
        y, x = np.unravel_index(int(np.argmin(cost)), cost.shape)
        chosen.append(((y * f) % (q * h), (x * f) % w, float(ssd[y, x])))
        allowed &= away(y, x)
    return chosen


def make_variants(works, maps, cm, count, feather, use_graphcut, s=0):
    """Tiles that share their border and differ inside: each keeps the base tile's frame (1/16 of
    the tile) exactly, so any variant fits next to any other, and takes its interior from the
    wrapped tile at an offset whose content matches across the joining band best, joined by a
    minimum-cost cut (through low ground, taller side winning, with a height map). The same
    offsets and weights go to every map. -> list of per-map work lists, report entry."""
    h, w = works[0].shape[:2]
    frame = max(8, round(min(h, w) / 16))
    band = max(8, round(min(h, w) / 16))
    soft = max(1.5, feather / 2)
    yy, xx = np.ogrid[:h, :w]
    dist = np.minimum(np.minimum(yy, h - 1 - yy), np.minimum(xx, w - 1 - xx))
    keep = dist < frame + 2 * soft
    inner = dist >= frame + band
    ring = ~keep & ~inner
    images = cm.images(works)
    height = cm.height_image(works)
    f = max(1, int(round(max(h, w) / 256)))
    offsets = ring_offsets(images, ring, count - 1, 0.25, f, s)
    q = lattice_rows(w, s)
    variants, entries = [], []
    for dy, dx, cost in offsets:
        donors = [translate(a, dy, dx, s) for a in works]
        heights = None if height is None else (height, translate(height, dy, dx, s))
        if use_graphcut:
            steer = mismatch([(g, translate(g, dy, dx, s)) for g in images])
            route = height_route(heights, cm.route) if heights is not None else None
            if route is not None:
                steer = steer + route * float(steer.mean())
            label = mask_cut(steer, keep, inner)
            weight = np.clip(cv2.GaussianBlur(label.astype(np.float32), (0, 0), soft), 0, 1)
            if heights is not None and cm.blend:
                edge = label ^ cv2.erode(label.astype(np.uint8), disk(1)).astype(bool)
                near = np.clip(1 - cv2.distanceTransform((~edge).astype(np.uint8), cv2.DIST_L2, 3)
                               / max(8.0, 2.0 * feather), 0, 1)
                weight = np.clip(weight + (heights[1] - heights[0]) * near / HEIGHT_FLIP, 0, 1)
        else:
            weight = np.clip((dist - frame) / float(band), 0, 1).astype(np.float32)
        weight[dist < frame] = 0                       # the shared border, exactly the base tile
        variants.append([a + (d - a) * weight[..., None] for a, d in zip(works, donors)])
        entries.append({'offset': [int(dx if dx <= w // 2 else dx - w), int(dy if dy <= q * h // 2 else dy - q * h)],
                        'band_cost': round(cost, 6), 'replaced_share': round(float(weight.mean()), 3)})
    return variants, {'count': 1 + len(variants), 'border_px': frame, 'band_px': band, 'variants': entries,
                      'boundary': 'graphcut' if use_graphcut else 'ramp (pip install PyMaxflow for a cut)'}


# ---- --layout rotate: tiles that join under any quarter turn ----------------------------------
# A random quarter turn per cell, hard-edged. Any side must meet any side of a neighbour turned any
# way, so every side carries the same band B in its own frame (that side turned to the top): across
# every edge line the field then shows B under its own 180-degree turn, one strip Q, point-symmetric
# about the edge midpoint whatever the turns. Q is a natural strip of the tile on one side of a
# point-symmetric cut and the strip's 180-degree turn on the other. Around every field corner the four
# corner squares assemble one patch P with 4-fold symmetry about the corner point: four quarter turns
# of a calm, level patch, its sectors split by a minimum-cost cut from the centre outwards. Q's ends
# are P's arms, joined to the strip by a cut across it. Each side's band is pasted onto the tile and
# joined to the interior by the usual minimum-cost cut. Colour and data maps carry only detail in the
# band (their large-scale tone is flat there and eases back into the tile), so no blotch repeats along
# every edge. Same strips, patch and cuts in every map; normal vectors turn with the image.

ROT_BAND = 1 / 32      # each side's exact symmetric band, a share of the tile (the strip across an edge is twice that)
ROT_JOIN = 1 / 16      # how far the frame reaches into the tile to meet it, and a corner arm along the strip
ROT_LEVEL = 4          # weight of a net slope against calm and landmark heat when choosing the strip and corner sources
ROT_SHALLOW = 0.5      # cut cost added across a join (times its mean), rising into the tile: frame kept only where needed
ROT_KEEP_TONE = 0.5    # colour/data: the share of the frame's own large-scale tone it keeps (the rest is flat)
ROT_TONE = 2           # colour/data: the tile's own tone returns over this many join widths from the band
ROTATE_RUNTIME = ('The Unreal material must give every cell its quarter turn: turn the UVs within the cell about '
                  'its centre, and turn the sampled normal\'s tangent XY by the same quarter turn the tile shows '
                  '(the inverse of the UV turn), or turned cells are lit from the wrong side. Hard-edged: no blend '
                  'between cells. Normal maps here are DirectX (green down, Unreal) unless the file name says GL.')


def turn_tile(a, k, m=None):
    """The tile turned k quarter turns counter-clockwise; a normal map's tangent-space XY turns with
    it (DirectX green-down unless the map is named GL)."""
    k %= 4
    out = np.rot90(a, k, axes=(0, 1)).copy()
    if k and m is not None and m.kind == 'normal':
        x, g = out[..., 2].copy(), out[..., 1].copy()
        for _ in range(k):          # one CCW quarter turn: DX (x, g) -> (g, -x); GL (x, g) -> (-g, x)
            x, g = (g, -x) if m.green_down else (-g, x)
        out[..., 2], out[..., 1] = x, g
    return out


def turned_values(values, m):
    """A tile's file values turned 0-3 quarter turns counter-clockwise, normal vectors turned with it."""
    work = m.decode(values)
    return [m.encode(turn_tile(work, k, m)) for k in range(4)]


def four_fold(quarter, m=None):
    """A 2R x 2R image from its top-right quarter and that quarter's three quarter turns: exactly
    4-fold symmetric about its centre (turn_tile(out, 1, m) equals out)."""
    r = quarter.shape[0]
    out = np.empty((2 * r, 2 * r) + quarter.shape[2:], quarter.dtype)
    out[:r, r:] = quarter
    out[:r, :r] = turn_tile(quarter, 1, m)
    out[r:, :r] = turn_tile(quarter, 2, m)
    out[r:, r:] = turn_tile(quarter, 3, m)
    return out


def wrap_rows(a, start, count):
    return a[(np.arange(count) + start) % a.shape[0]]


def wrap_patch(a, cy, cx, half):
    """The 2*half square of a periodic tile centred between pixels (cy - 1, cx - 1) and (cy, cx)."""
    rows = (np.arange(2 * half) + cy - half) % a.shape[0]
    cols = (np.arange(2 * half) + cx - half) % a.shape[1]
    return a[rows][:, cols]


def frame_sources(works, maps, cm, heat_full, D, R, feather):
    """Where the edge strip and the corner patch come from. Turned copies of a patch show whatever it
    has: a feature near a corner point becomes a star, a net slope a pyramid or funnel (four turns of a
    tilted facet), a strip calmer or busier than the rest a line round every cell. So both come from
    where every map is level (least net slope: a normal map's mean XY, another map's mean signed
    gradient, over the window) and no landmark is near (least landmark heat); the strip where the maps
    are as busy as usual (gradient energy near its median), the corner patch where they are calm. The
    strip's centre line then moves up to 3 px onto an ordinary row pair, never a JPEG block edge.
    -> c: the edge line runs between rows c - 1 and c; (y, x): the corner patch's centre."""
    L = works[0].shape[0]
    images = cm.images(works)
    busy = [(np.abs(g - np.roll(g, 1, axis=0)) + np.abs(g - np.roll(g, 1, axis=1))).mean(axis=2) for g in images]
    units = [max(float(b.mean()), 1e-9) for b in busy]
    busy = sum(b / u for b, u in zip(busy, units)) / len(busy)                # every map counts alike
    heat = heat_full / max(float(heat_full.mean()), 1e-9)
    slopes = []                           # per steering map: (along y, along x), signed
    for i, g in zip(cm.index, images):
        if maps[i].kind == 'normal':      # a normal map's XY is its slope
            slope = (g[..., 1] - g[..., 1].mean(), g[..., 2] - g[..., 2].mean())
        else:
            slope = ((g - np.roll(g, 1, axis=0)).mean(axis=2), (g - np.roll(g, 1, axis=1)).mean(axis=2))
        slopes.append(tuple(a / units[len(slopes)] / len(images) for a in slope))

    def rows_around(profile, half):       # mean of profile over rows c - half .. c + half - 1, every c, wrapped
        cs = np.concatenate([[0.0], np.cumsum(np.tile(profile, 3))])
        c = np.arange(L) + L
        return (cs[c + half] - cs[c - half]) / (2 * half)

    def box(a, size):                     # wrapped mean over the size x size window round every pixel
        return cv2.blur(np.pad(a, size, mode='wrap'), (size, size))[size:size + L, size:size + L]
    reach = max(1, feather) + 3
    tilt = sum(np.hypot(rows_around(sy.mean(axis=1), R), rows_around(sx.mean(axis=1), R)) for sy, sx in slopes)
    calm = rows_around(busy.mean(axis=1), reach)          # typical, not the calmest: a calm strip outlines every cell
    score = rows_around(heat.mean(axis=1), R) + np.abs(np.log(calm / np.median(calm))) + ROT_LEVEL * tilt
    c = int(np.argmin(score))
    steps = [np.abs(g - np.roll(g, 1, axis=0)).mean(axis=(1, 2)) for g in images]       # rows i - 1 | i
    steps = [np.abs(np.log(np.maximum(st, 1e-12) / max(float(np.median(st)), 1e-12))) for st in steps]
    c = min(range(c - 3, c + 4), key=lambda k: (max(float(st[k % L]) for st in steps), abs(k - c))) % L
    tilt = sum(np.hypot(box(sy, D), box(sx, D)) for sy, sx in slopes)
    score = box(heat, 2 * D) + box(busy, D) + ROT_LEVEL * tilt
    y, x = np.unravel_index(int(np.argmin(score)), (L, L))
    return c, (int(y), int(x))


def odd_path(cost, feather):
    """A cut along a strip (rows x L, rows even) that is point-symmetric about the strip's centre: the
    half from the centre leftwards by dynamic programming, the other half its 180-degree copy, so it
    passes through the centre. -> the cut's row in every column."""
    rows, length = cost.shape
    half = length // 2
    out = seam_path(cost[:, :half][:, ::-1].T, feather, rows - 1 - feather, start=rows // 2 - 1, closed=False)
    path = np.empty(length, np.float32)
    path[:half] = out[::-1]
    path[length - half:] = (rows - 1 - path[:half])[::-1]
    if length % 2:
        path[half] = (rows - 1) / 2
    return path


def corner_patch(patches, maps, cm, feather):
    """P from one 2R x 2R patch per map: the patch and its three quarter turns, each in a sector round
    the centre. The boundary between the sector kept as it is and the one turned once is the cheapest
    path from the centre outwards between the patch and its quarter turn, laid along the diagonal
    with at most one pixel sideways per pixel, so it stays inside its quadrant; the other three
    boundaries are its turns. -> per map P (exactly 4-fold symmetric), mean cost on the boundary."""
    n = patches[0].shape[0]
    r, c = n // 2, n // 2 - 0.5
    mis = mismatch(list(zip(cm.images(patches), cm.images([turn_tile(p, 1, m) for p, m in zip(patches, maps)]))))
    # the quadrant between the right arm (angle 0) and the top arm (90 degrees): s runs along the
    # diagonal from the centre, v across it (v = +s on the right arm, -s on the top arm)
    length = int(math.ceil(r * math.sqrt(2)))
    s = np.arange(length, dtype=np.float32)[:, None]
    v = np.arange(1 - length, length, dtype=np.float32)[None, :]
    k = 1 / math.sqrt(2)
    map_x, map_y = (c + (s + v) * k).astype(np.float32), (c + (v - s) * k).astype(np.float32)

    def along(a):
        return cv2.remap(np.ascontiguousarray(a, np.float32), map_x, map_y, cv2.INTER_LINEAR,
                         borderMode=cv2.BORDER_REPLICATE)
    path = seam_path(along(blur(mis, 1.0, 1.0)), 0, 2 * length - 2, start=length - 1, closed=False)
    cost = float(along(mis)[np.arange(length), path].mean())
    yy, xx = np.mgrid[0:r, r:n].astype(np.float32)
    keep = ((xx - c) + (yy - c)) * k > np.interp(((xx - c) - (yy - c)) * k, np.arange(length), path - (length - 1))
    sector = np.zeros((n, n), np.float32)          # where the patch is kept as it is (turn 0)
    sector[:r, r:] = keep
    sector[r:, r:] = turn_tile(~keep, 3)
    w0 = cv2.GaussianBlur(sector, (0, 0), max(1.0, feather / 2), borderType=cv2.BORDER_REFLECT)
    weights = [turn_tile(w0, j) for j in range(4)]
    total = sum(weights)
    out = []
    for p, m in zip(patches, maps):
        quarter = sum((wj / total)[:r, r:, None] * turn_tile(p, j, m)[:r, r:] for j, wj in enumerate(weights))
        out.append(four_fold(np.ascontiguousarray(quarter, np.float32), m))
    return out, cost


def wrap_blur(a, sigma):
    """Gaussian blur of a periodic tile by FFT: the wrap is exact and the cost does not grow with sigma."""
    h, w = a.shape[:2]
    fy, fx = np.fft.fftfreq(h)[:, None], np.fft.rfftfreq(w)[None, :]
    gain = np.exp(-2 * (math.pi * sigma) ** 2 * (fy ** 2 + fx ** 2))
    out = np.empty(a.shape, np.float32)
    for c in range(a.shape[2]):
        out[..., c] = np.fft.irfft2(np.fft.rfft2(a[..., c]) * gain, s=(h, w))
    return out


def tone_split(a, m, sigma):
    """The part of a map's large-scale tone (a wrapped blur) a rotatable frame gives up, the rest (what
    the frame is cut from), and the flat tone it gets instead: the tile's mean. The frame keeps
    ROT_KEEP_TONE of its own tone: all of it repeats a tone pattern along every edge and a rosette at
    every corner, none of it leaves a calm corridor round every cell (both measured on the sand set).
    A normal map is not split: its flattened slopes read as arcs along the joins once lit, while the
    level sources (frame_sources) and the cuts already keep its frame level. -> low, rest, flat."""
    if m.kind == 'normal':
        zero = np.zeros_like(a)
        return zero, a, zero[0, 0]
    low = wrap_blur(a, sigma) * (1 - ROT_KEEP_TONE)
    return low, a - low, a.mean(axis=(0, 1)).astype(np.float32) * (1 - ROT_KEEP_TONE)


def frame_sizes(L, feather):
    """D: each side's exact symmetric band; g: how far the frame reaches into the tile to meet it (also
    the room of a corner arm); sigma: the scale above which tone is flattened in the frame."""
    f = max(1, feather)
    D, g = max(f + 4, round(L * ROT_BAND)), max(2 * f + 3, round(L * ROT_JOIN))
    if 4 * (D + g) > L:
        raise SystemExit(f'--layout rotate: a {L} px tile is too small for a {D} px band and a {g} px join (needs '
                         f'{4 * (D + g)} px); use a larger tile or a smaller --feather')
    return D, g, g / 2


def rotation_frame(works, maps, cm, heat_full, feather):
    """The band every side of a rotatable tile carries, per map, in the side's own frame (the side at
    the top): D rows of exact band and g rows of how it continues into the tile; colour and data maps
    give up part of their tone above sigma (tone_split). The strip and the corner patch come from calm places with no
    landmark near (frame_sources), so nothing distinctive is copied to every edge. -> bands ((D + g) x L), report entry."""
    L = works[0].shape[0]
    D, g, sigma = frame_sizes(L, feather)
    R = D + g
    works = [tone_split(a, m, sigma)[1] for a, m in zip(works, maps)]
    natural = natural_difference(cm.images(works), max(1, 2 * feather))
    soft = max(0.5, feather / 2)

    def visibility(cost):
        return round(math.sqrt(cost / natural), 3) if natural > 0 else 0.0
    # 1. the edge strip Q, 2R rows across the edge line: a calm strip of the tile on one side of a
    #    point-symmetric cut, its 180-degree turn on the other. The cut must cross the edge line at the
    #    strip's centre; elsewhere it is kept off the line, so the line itself runs through natural content
    line, (qy, qx) = frame_sources(works, maps, cm, heat_full, D, R, feather)
    strips = [wrap_rows(a, line - R, 2 * R) for a in works]
    turned = [turn_tile(s, 2, m) for s, m in zip(strips, maps)]
    mis = mismatch(list(zip(cm.images(strips), cm.images(turned))))
    steer = blur(mis, 1.0, soft)
    on_line = np.exp(-((np.arange(2 * R) - (R - 0.5)) / (2 * max(1, feather) + 2)) ** 2)[:, None]
    path = odd_path(steer + on_line * float(steer.mean()), feather)
    odd_cost = float(mis[np.round(path).astype(int), np.arange(L)].mean())
    w = seam_mask(path, 2 * R, feather).T[..., None]            # 1 on the strip's own side of the cut
    strips = [s * w + t * (1 - w) for s, t in zip(strips, turned)]
    # 2. the corner patch P, 2R across like Q is tall
    corners, corner_cost = corner_patch([wrap_patch(a, qy, qx, R) for a in works], maps, cm, feather)
    # 3. Q's left end is P's right half and its right end P's left half (the left end turned 180
    #    degrees), each joined to the strip along a cut across it, short arms preferred
    mis = mismatch(list(zip(cm.images([p[:, R + D:] for p in corners]), cm.images([q[:, D:R] for q in strips]))))
    steer = blur(mis, 1.0, soft)
    arm = seam_path(steer + float(steer.mean()) * np.arange(g)[None, :] / g, feather, g - 1 - feather)
    arm_cost = float(mis[np.arange(2 * R), arm].mean())
    ends = np.zeros((2 * R, L), np.float32)
    ends[:, :D] = 1
    ends[:, D:R] = 1 - seam_mask(arm, g, feather)
    ends = (ends + ends[::-1, ::-1])[..., None]
    bands = []
    for p, q in zip(corners, strips):
        layer = np.zeros_like(q)
        layer[:, :R], layer[:, L - R:] = p[:, R:], p[:, :R]
        bands.append(np.ascontiguousarray((layer * ends + q * (1 - ends))[R:]))
    info = {'band_px': D, 'strip_px': 2 * D, 'corner_px': 2 * D, 'join_px': g, 'arm_px': [int(D + arm.min()), int(D + arm.max())],
            'tone_sigma_px': round(sigma, 1), 'strip_line_row': int(line), 'corner_source': [int(qx), int(qy)],
            'cuts': [{'cut': 'edge strip, point-symmetric', 'visibility': visibility(odd_cost),
                      'off_line_px': int(np.abs(path - (R - 0.5)).max())},
                     {'cut': 'corner patch sectors, 4-fold', 'visibility': visibility(corner_cost)},
                     {'cut': 'corner arm to edge strip', 'visibility': visibility(arm_cost)}]}
    return bands, info


def paste_frame(works, maps, cm, bands, feather, seam, flats=None):
    """Paste the band onto every side of the tile's detail (each side turned to the top in turn), join it
    to the interior by the minimum-cost cut through the next g rows (through low ground, the taller side
    winning, with a height map), then add the tile's own tone back: flat in the band, easing into the
    interior's over the next 2g px. Same bands and cuts in every map; variants pass the base tile's
    flat tones (`flats`) so their bands match it exactly. -> works, cut stats, flats."""
    L = works[0].shape[0]
    D, g, sigma = frame_sizes(L, feather)
    yy, xx = np.ogrid[:L, :L]
    dist = np.minimum(np.minimum(yy, L - 1 - yy), np.minimum(xx, L - 1 - xx))
    ease = np.clip((dist - D) / (ROT_TONE * g), 0, 1).astype(np.float32)
    ease = (ease * ease * (3 - 2 * ease))[..., None]
    parts = [tone_split(a, m, sigma) for a, m in zip(works, maps)]
    flats = flats or [flat for _, _, flat in parts]
    tones = [flat + (low - flat) * ease for (low, _, _), flat in zip(parts, flats)]   # given back at the end
    out = [d for _, d, _ in parts]
    natural = natural_difference(cm.images(out), max(1, 2 * feather))
    stats = []
    win = (slice(D, D + g), slice(D, L - D))
    for k, side in enumerate(('top', 'right', 'bottom', 'left')):
        turned = [turn_tile(a, k, m) for a, m in zip(out, maps)]
        inner = [a[win] for a in turned]
        ext = [b[D:, D:L - D] for b in bands]
        mis = np.ascontiguousarray(mismatch(list(zip(cm.images(ext), cm.images(inner)))).T)   # along the side x depth
        route = heights = None
        if cm.height is not None:           # heights as they will be: detail plus the tone given back
            h = cm.height
            tone = turn_tile(tones[h], k, maps[h])[win]
            side_maps = [[e + tone if i == h else e for i, e in enumerate(ext)],
                         [x + tone if i == h else x for i, x in enumerate(inner)]]
            heights = tuple(np.ascontiguousarray(cm.height_image(a).T) for a in side_maps)
            route = height_route(heights, cm.route)
            heights = heights if cm.blend else None
        shallow = ROT_SHALLOW * np.arange(g, dtype=np.float32)[None, :] / g          # keep only the frame a join needs
        route = shallow if route is None else route + shallow
        weight, path_cost, cols = cut(mis, route, heights, feather, False, seam)
        weight = weight.T[..., None]
        for a, b, e, x in zip(turned, bands, ext, inner):
            a[:D] = b[:D]
            a[win] = e + (x - e) * weight
        out = [turn_tile(a, -k, m) for a, m in zip(turned, maps)]
        stats.append(seam_stats(mis, path_cost, cols, natural, side, D, seam))
    return [d + t for d, t in zip(out, tones)], stats, flats


def rotation_guard(s, name, allow):
    """--layout rotate on a texture with a direction or a construction is refused (with --allow-structured,
    loudly warned); a mildly oriented one is warned. -> warnings."""
    if s['class'] in ('directional', 'regular'):
        if s['class'] == 'directional':
            detail = f'coherence {s["coherence"]:.2f}, grain {s["grain_degrees"]:g} deg'
            what = 'grain, ripples or streaks would run two ways across the field'
        else:
            detail = f'regularity {s["regularity"]:.2f}'
            what = 'courses, rows or pattern would turn from cell to cell'
        text = f'{name} reads as {s["class"]} ({detail}): under quarter turns its {what}'
        if not allow:
            raise SystemExit(f'--layout rotate refused: {text}. Use --layout grid, brick or half-drop, or pass '
                             f'--allow-structured to turn it anyway')
        return [f'ROTATED A STRUCTURED TEXTURE (--allow-structured): {text}; judge the field preview']
    if not s['random_rotation_safe']:
        return [f'rotate: {name} is mildly oriented (anisotropy {s["anisotropy"]:.1f} >= {ANISOTROPY_AT}); quarter '
                f'turns may show; judge the field preview']
    return []


def turn_metrics(values, m):
    """Every side-to-side pairing a turned field produces: the tile turned a on the left of the tile
    turned b (quarter turns counter-clockwise). The 16 pairings cover every side against every side;
    the vertical joins are the same pairs turned. Each step is compared, like wrap_x, with the tile's
    own neighbouring column and row pairs, and its 16 px band means likewise (low)."""
    base = m.decode(values).astype(np.float32)
    turned = [turn_tile(base, k, m) for k in range(4)]
    h = base.shape[0]
    b = max(2, min(16, h // 8))

    def bands(a):            # means of b x b blocks: block rows x every b-column window
        rows = a[:h // b * b].reshape(h // b, b, a.shape[1], -1).mean(axis=1, dtype=np.float64)
        cum = np.concatenate([np.zeros_like(rows[:, :1]), np.cumsum(rows, axis=1)], axis=1)
        return (cum[:, b:] - cum[:, :-b]) / b
    inner, inner_low = [], []
    for a in turned[:2]:     # neighbouring columns, then neighbouring rows
        inner.append(np.abs(np.diff(a, axis=1)).mean(axis=(0, 2)))
        band = bands(a)
        inner_low.append(np.abs(band[:, b:] - band[:, :-b]).mean(axis=(0, 2)))
    inner, inner_low = np.concatenate(inner), np.concatenate(inner_low)
    ends = [(band[:, 0], band[:, -1]) for band in map(bands, turned)]
    pairs = []
    for a in range(4):
        for c in range(4):
            step = float(np.abs(turned[a][:, -1] - turned[c][:, 0]).mean())
            low = float(np.abs(ends[a][1] - ends[c][0]).mean())
            pair = {'turns': [a, c], 'wrap': round(ratio(step, float(inner.mean())), 3),
                    'rank': round(float((inner < step).mean()), 4),
                    'low': round(ratio(low, float(inner_low.mean())), 3),
                    'low_rank': round(float((inner_low < low).mean()), 4)}
            pair['seam'] = bool(pair['wrap'] > WRAP_WARN and pair['rank'] >= RANK_WARN
                                or pair['low'] > LOW_WARN and pair['low_rank'] >= RANK_WARN)
            pairs.append(pair)
    worst = max(pairs, key=lambda p: p['wrap'])
    return {'pairs': len(pairs), 'worst_wrap': worst['wrap'], 'worst_rank': worst['rank'], 'worst_turns': worst['turns'],
            'mean_wrap': round(float(np.mean([p['wrap'] for p in pairs])), 3),
            'worst_low': max(p['low'] for p in pairs), 'seams': [p['turns'] for p in pairs if p['seam']],
            'seamless': not any(p['seam'] for p in pairs),
            'note': 'turns [a, b]: the tile turned a quarter turns counter-clockwise on the left of the tile turned b'}


# ---- measurement ---------------------------------------------------------------------------

def ratio(edge, inner):
    if inner < 1e-9:
        return 1.0 if edge < 1e-9 else 99.0
    return edge / inner


def wrap_metrics(values):
    """How natural the wrap is, on the file's own values (integer files as 0-1), per axis.
    wrap_x: mean |first column - last column| / mean |neighbouring-column difference inside|.
    low_x: the same for 16 px bands (means over 16 x 16 blocks): the first 16 columns against the
    last 16, over every pair of adjacent bands inside (tone and lighting drift). ~1 means the
    wrap is as natural as any interior boundary. *_rank: the share of interior boundaries with a
    smaller step; 1.0 means the wrap is the sharpest boundary in the image. step_*: absolute
    steps in levels of 255."""
    a = values.astype(np.float32)
    out = {}
    for axis, name in ((1, 'x'), (0, 'y')):
        x = a if axis == 1 else a.transpose(1, 0, 2)
        h, w = x.shape[:2]
        inner = np.abs(np.diff(x, axis=1)).mean(axis=(0, 2))
        edge = float(np.abs(x[:, 0] - x[:, -1]).mean())
        out[f'wrap_{name}'] = round(ratio(edge, float(inner.mean())), 3)
        out[f'wrap_{name}_rank'] = round(float((inner < edge).mean()), 4)
        out[f'step_{name}'] = round(edge * 255, 2)
        b = max(2, min(16, min(h, w) // 8))
        rows = x[:h // b * b].reshape(h // b, b, w, -1).mean(axis=1, dtype=np.float64)
        cum = np.concatenate([np.zeros_like(rows[:, :1]), np.cumsum(rows, axis=1)], axis=1)
        band = (cum[:, b:] - cum[:, :-b]) / b                  # band k = columns k .. k+b-1
        inner = np.abs(band[:, b:] - band[:, :-b]).mean(axis=(0, 2))
        edge = float(np.abs(band[:, 0] - band[:, -1]).mean())
        out[f'low_{name}'] = round(ratio(edge, float(inner.mean())), 3)
        out[f'low_{name}_rank'] = round(float((inner < edge).mean()), 4)
        out[f'low_step_{name}'] = round(edge * 255, 2)
    for name in 'xy':
        out[f'seam_{name}'] = bool(out[f'wrap_{name}'] > WRAP_WARN and out[f'wrap_{name}_rank'] >= RANK_WARN)
        out[f'tone_seam_{name}'] = bool(out[f'low_{name}'] > LOW_WARN and out[f'low_{name}_rank'] >= RANK_WARN)
    return out


def macro_variation(values, m):
    """Std of heavily blurred (sigma = tile/16, wrapped) luminance over its mean: large blotches
    that every repeat of the tile shows in the same place. A hint, not proof: look at the 3x3."""
    if m.kind == 'normal':
        return None
    lum = luminance(values) if m.kind == 'color' else values[..., m.tone].mean(axis=2)
    h, w = lum.shape
    f = max(1, min(h, w) // 256)
    small = cv2.resize(lum, (max(1, w // f), max(1, h // f)), interpolation=cv2.INTER_AREA) if f > 1 else lum
    s = min(small.shape) / 16
    big = blur(small, s, s, wrap_y=True, wrap_x=True)
    return round(float(big.std() / max(abs(float(lum.mean())), 1e-6)), 4)


def detail_channels(values, m):
    """The channels that show the texture's look: luminance for colour, the tone channels for data,
    the encoded vector for normals (alpha left out)."""
    if m.kind == 'color':
        return luminance(values)[..., None]
    return values[..., [0, 1, 2] if m.kind == 'normal' else m.tone]


def texture_structure(values, m):
    """Whether the tile reads as stochastic (random rotation and offset blending at runtime are
    safe), directional (grain, brushing: offsets are fine, rotation is not) or regular (bricks,
    planks, tiles: keep the grid). regularity is the highest secondary peak of the wrapped
    autocorrelation of the detail (1 = an exact repeat inside the tile); coherence is how much
    one gradient direction dominates (0 none, 1 straight parallel lines)."""
    lum = detail_channels(values, m).mean(axis=2)
    h, w = lum.shape
    f = max(1, max(h, w) // 512)
    small = cv2.resize(lum, (max(8, w // f), max(8, h // f)), interpolation=cv2.INTER_AREA) if f > 1 else lum
    sh, sw = small.shape
    detail = small - blur(small, min(sh, sw) / 32, min(sh, sw) / 32, wrap_y=True, wrap_x=True)
    power = np.abs(np.fft.rfft2(detail.astype(np.float64))) ** 2
    ac = np.fft.fftshift(np.fft.irfft2(power, s=detail.shape))
    ac = (ac / max(float(ac[sh // 2, sw // 2]), 1e-12)).astype(np.float32)
    yy, xx = np.mgrid[0:sh, 0:sw]
    far = np.hypot(yy - sh // 2, xx - sw // 2) >= max(3.0, min(sh, sw) / 32)
    peaks = far & (ac >= cv2.dilate(ac, np.ones((5, 5), np.uint8)))
    regularity = float(ac[peaks].max()) if peaks.any() else 0.0
    gx, gy = cv2.Sobel(detail, cv2.CV_32F, 1, 0), cv2.Sobel(detail, cv2.CV_32F, 0, 1)
    jxx, jyy, jxy = float((gx * gx).mean()), float((gy * gy).mean()), float((gx * gy).mean())
    coherence = math.sqrt((jxx - jyy) ** 2 + 4 * jxy ** 2) / max(jxx + jyy, 1e-12)
    grain = (math.degrees(0.5 * math.atan2(2 * jxy, jxx - jyy)) + 90) % 180     # along the features
    # power-spectrum anisotropy: the strongest of 12 orientation bins over their mean, mid frequencies
    spectrum = np.fft.fftshift(np.abs(np.fft.fft2(detail.astype(np.float64))) ** 2)
    radius = np.hypot(yy - sh // 2, xx - sw // 2)
    ring = (radius >= 2) & (radius <= min(sh, sw) / 4)
    bins = (((np.arctan2(yy - sh // 2, xx - sw // 2) % np.pi) / np.pi) * 12).astype(int) % 12
    energy = np.bincount(bins[ring], weights=spectrum[ring], minlength=12)
    anisotropy = float(energy.max() / max(energy.mean(), 1e-12))
    kind = 'regular' if regularity >= REGULAR_AT else 'directional' if coherence >= DIRECTIONAL_AT else 'stochastic'
    isotropic = kind == 'stochastic' and anisotropy < ANISOTROPY_AT
    guide = 'Plugins/GraphCode/Docs/Guides/TEXTURE-VARIATION.md'
    steps = [f'break repetition in the Unreal material: UV variation (TextureVariation), a near/far '
             f'CameraDepthFade blend and macro variation at a separate scale ({guide})']
    if isotropic:
        steps.append('random rotation and hex/stochastic tiling suit this texture; verify the normal handling')
    elif kind == 'directional':
        steps.append('keep the grain direction: offset-only variation, no random rotation or rotating '
                     'hex/stochastic tiling; macro variation and decals for the rest')
    elif kind == 'regular':
        steps.append('keep the construction: no random rotation, offsets or stochastic tiling (they break the '
                     'courses); vary the finish with masks, vertex paint and decals')
    else:
        steps.append('mildly oriented: prefer offset-only variation; test rotation on the real surface first')
    return {'class': kind, 'texture_type': 'isotropic' if isotropic else 'structured',
            'regularity': round(regularity, 3), 'coherence': round(coherence, 3),
            'anisotropy': round(anisotropy, 2), 'grain_degrees': round(grain, 1) if kind == 'directional' else None,
            'stochastic_tiling_safe': isotropic, 'random_rotation_safe': isotropic,
            'offset_blend_safe': kind != 'regular', 'next_steps': steps, 'runtime_guide': guide}


def landmarks(values, m, count=5):
    """Patches most likely to read as repeats, 1/16 of the tile across. A patch scores the larger of
    its rarity (distance to its nearest non-overlapping patch elsewhere in the wrapped tile, over
    the median of those distances) and its brightness outlier (|patch mean - median| over twice the
    median deviation: the bright pebble, the dark hollow, which is also what makes a field of tiles
    read as a grid). 1 is ordinary. -> heat map, top spots."""
    a = detail_channels(values, m)
    h, w = a.shape[:2]
    gh, gw = max(8, round(64 * h / max(h, w))), max(8, round(64 * w / max(h, w)))
    small = cv2.resize(a, (gw, gh), interpolation=cv2.INTER_AREA).reshape(gh, gw, -1)
    p = 4
    padded = np.pad(small, ((0, p - 1), (0, p - 1), (0, 0)), mode='wrap')
    x = np.lib.stride_tricks.sliding_window_view(padded, (p, p), axis=(0, 1)).reshape(gh * gw, -1)
    x = x.astype(np.float32)
    sq = (x * x).sum(axis=1)
    dist = np.maximum(sq[:, None] + sq[None, :] - 2 * (x @ x.T), 0)
    iy, ix = (v.astype(np.int16) for v in np.divmod(np.arange(gh * gw), gw))
    dy = np.abs(iy[:, None] - iy[None, :])
    dx = np.abs(ix[:, None] - ix[None, :])
    dist[(np.minimum(dy, gh - dy) < p) & (np.minimum(dx, gw - dx) < p)] = np.inf
    unique = np.sqrt(dist.min(axis=1)).reshape(gh, gw)
    unique = unique / max(float(np.median(unique)), 1e-9)
    bright = x.reshape(gh * gw, -1).mean(axis=1).reshape(gh, gw)
    deviation = np.abs(bright - np.median(bright))
    outlier = deviation / max(2 * float(np.median(deviation)), 1e-9)
    score = np.roll(np.maximum(unique, outlier), (p // 2, p // 2), axis=(0, 1))
    spots = []
    for k in np.argsort(-score.ravel(), kind='stable'):
        y, x_ = divmod(int(k), gw)
        if all(min(abs(y - sy), gh - abs(y - sy)) >= p or min(abs(x_ - sx), gw - abs(x_ - sx)) >= p
               for sy, sx, _ in spots):
            spots.append((y, x_, float(score[y, x_])))
        if len(spots) == count:
            break
    top = [{'x': round((sx + 0.5) * w / gw), 'y': round((sy + 0.5) * h / gh), 'score': round(sc, 2)}
           for sy, sx, sc in spots]
    return score.astype(np.float32), top


def measure(values, m):
    out = wrap_metrics(values)
    out['macro'] = macro_variation(values, m)
    out['seamless'] = not any(out[f'{k}_{n}'] for k in ('seam', 'tone_seam') for n in 'xy')
    return out


def metric_warnings(name, before, after):
    """Warnings for the state a file is left in (after, or before in --check)."""
    s = after if after is not None else before
    warnings = []
    for axis, edge in (('x', 'left/right'), ('y', 'top/bottom')):
        if s[f'seam_{axis}']:
            grade = 'a faint' if s[f'wrap_{axis}'] < 2 else 'a clear'
            warnings.append(f'{name}: {grade} seam at the {edge} edge: wrap_{axis} {s[f"wrap_{axis}"]:.2f}, sharper '
                            f'than {s[f"wrap_{axis}_rank"]:.1%} of interior boundaries ({s[f"step_{axis}"]:.1f} levels)')
        if s[f'tone_seam_{axis}']:
            warnings.append(f'{name}: tone or lighting changes across the {edge} edge: low_{axis} '
                            f'{s[f"low_{axis}"]:.2f} ({s[f"low_step_{axis}"]:.1f} levels)')
    if s['macro'] is not None and s['macro'] > MACRO_WARN:
        warnings.append(f'{name}: macro variation {s["macro"]:.3f} > {MACRO_WARN}: large blotches will repeat '
                        f'visibly when tiled; judge the 3x3 preview')
    return warnings


def lay_out(cells, rows, cols, layout, shift):
    """Assemble equally sized cell images (cells[r][c], H x W x 3) in a layout: 'grid'; 'brick' (row r
    moved right by r*shift px); 'half-drop' (column c moved down by c*shift px). Each row (column) is
    one continuous strip, so the shifted copies wrap exactly as the tiles do."""
    if layout == 'half-drop':
        flipped = [[cells[r][c].transpose(1, 0, 2) for r in range(rows)] for c in range(cols)]
        return lay_out(flipped, cols, rows, 'brick', shift).transpose(1, 0, 2)
    h, w = cells[0][0].shape[:2]
    out = []
    for r in range(rows):
        strip = np.concatenate([cells[r][c] for c in range(cols)], axis=1)
        if layout == 'brick' and shift:
            strip = np.roll(strip, (r * shift) % (cols * w), axis=1)
        out.append(strip)
    return np.concatenate(out, axis=0)


def field_image(tiles8, layout, shift_frac, n=10, cell=200, seed=0):
    """n x n tiles at `cell` px each (INTER_AREA, like a distant mip level), in the layout; with
    several tiles (variants) every cell picks one at random (fixed seed). For rotate, tiles8[v] holds
    variant v's four quarter turns and every cell also picks a turn at random."""
    rotate = layout == 'rotate'
    h, w = (tiles8[0][0] if rotate else tiles8[0]).shape[:2]
    f = cell / max(h, w)
    size = (max(1, round(w * f)), max(1, round(h * f)))

    def shrink(t):
        return cv2.resize(t, size, interpolation=cv2.INTER_AREA)
    small = [[shrink(t) for t in v] for v in tiles8] if rotate else [shrink(t) for t in tiles8]
    rng = np.random.default_rng(seed)
    pick = rng.integers(0, len(small), (n, n)) if len(small) > 1 else np.zeros((n, n), int)
    if rotate:
        turns = rng.integers(0, 4, (n, n))
        return lay_out([[small[pick[r, c]][turns[r, c]] for c in range(n)] for r in range(n)], n, n, 'grid', 0)
    shift = round(shift_frac * (size[1] if layout == 'half-drop' else size[0]))
    return lay_out([[small[pick[r, c]] for c in range(n)] for r in range(n)], n, n, layout, shift)


def grid_read(tiles_values, m):
    """How strongly the repeat reads at a distance: the luminance contrast (std / mean) of the pattern
    every cell shares, seen at a distant mip where a tile is 16 px (and 32 px). With variants the shared
    pattern is their mean. A hint, not proof: judge the field preview."""
    out = {}
    for px in (16, 32):
        mips = []
        for values in tiles_values:
            lum = luminance(values) if m.kind == 'color' else detail_channels(values, m).mean(axis=2)
            h, w = lum.shape
            f = px / min(h, w)
            mips.append(cv2.resize(lum, (max(1, round(w * f)), max(1, round(h * f))), interpolation=cv2.INTER_AREA))
        shared = np.mean(mips, axis=0)
        out[f'mip{px}'] = round(float(shared.std() / max(abs(float(shared.mean())), 1e-6)), 4)
    return out


# ---- preview -------------------------------------------------------------------------------

BG, INK, TICK = (38, 38, 38), (235, 235, 235), (0, 210, 255)
FONT, LABEL_H, MARGIN, GAP = cv2.FONT_HERSHEY_SIMPLEX, 26, 12, 18


def display8(values, m):
    a = values[..., :3] if values.shape[2] >= 3 else np.repeat(values[..., :1], 3, axis=2)
    if m.fmt['dtype'].startswith('float') and m.kind != 'normal':
        a = np.clip(a, 0, None)
        if m.kind == 'color':
            a = np.clip(a, 0, 1) ** (1 / 2.2)
        else:
            a = a / max(float(np.percentile(a, 99.5)), 1e-6)
    return np.floor(np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)


def panel(label, img, ticks_x, ticks_y, ticks_x_bottom=None, ticks_y_right=None):
    """The image in a margin, a label above, and tick marks in the margin only (never over the
    image) at the tile boundaries (bottom and right ones may differ for shifted layouts)."""
    h, w = img.shape[:2]
    canvas = np.full((LABEL_H + h + 2 * MARGIN, w + 2 * MARGIN, 3), BG, np.uint8)
    top, left = LABEL_H + MARGIN, MARGIN
    canvas[top:top + h, left:left + w] = img
    cv2.putText(canvas, label, (left, LABEL_H - 6), FONT, 0.5, INK, 1, cv2.LINE_AA)
    for x in ticks_x:
        cv2.line(canvas, (left + int(x), top - MARGIN + 2), (left + int(x), top - 3), TICK, 1)
    for x in (ticks_x if ticks_x_bottom is None else ticks_x_bottom):
        cv2.line(canvas, (left + int(x), top + h + 2), (left + int(x), top + h + MARGIN - 2), TICK, 1)
    for y in ticks_y:
        cv2.line(canvas, (2, top + int(y)), (left - 3, top + int(y)), TICK, 1)
    for y in (ticks_y if ticks_y_right is None else ticks_y_right):
        cv2.line(canvas, (left + w + 2, top + int(y)), (left + w + MARGIN - 2, top + int(y)), TICK, 1)
    return canvas


def tiled_panel(label, img8, n, size, layout='grid', shift_frac=0.0):
    """The tile n x n in the layout; for rotate img8 is the tile's four quarter turns and every cell
    takes one at random (fixed seed)."""
    turns = img8 if layout == 'rotate' else None
    img8 = turns[0] if turns else img8
    h, w = img8.shape[:2]
    f = min(1.0, size / (n * max(h, w)))
    tw, th = max(1, round(w * f)), max(1, round(h * f))
    tile = cv2.resize(img8, (tw, th), interpolation=cv2.INTER_AREA) if f < 1 else img8
    across, down = [k * tw for k in range(n + 1)], [k * th for k in range(n + 1)]
    if turns:
        cells = [cv2.resize(t, (tw, th), interpolation=cv2.INTER_AREA) if f < 1 else t for t in turns]
        pick = np.random.default_rng(n).integers(0, 4, (n, n))
        return panel(label, lay_out([[cells[pick[r, c]] for c in range(n)] for r in range(n)], n, n, 'grid', 0),
                     across, down)
    if layout == 'grid':
        return panel(label, np.tile(tile, (n, n, 1)), across, down)
    shift = round(shift_frac * (th if layout == 'half-drop' else tw))
    image = lay_out([[tile] * n for _ in range(n)], n, n, layout, shift)
    if layout == 'brick':      # the first row's joints along the top, the last row's along the bottom
        last = [(k * tw + (n - 1) * shift) % (n * tw) for k in range(n)]
        return panel(label, image, across, down, ticks_x_bottom=last)
    last = [(k * th + (n - 1) * shift) % (n * th) for k in range(n)]
    return panel(label, image, across, down, ticks_y_right=last)


def crossing_panel(label, img8, size):
    """Where four tiles meet, at 1:1: the tile rolled by half so its wrap runs through the middle."""
    h, w = img8.shape[:2]
    rolled = np.roll(img8, (h // 2, w // 2), axis=(0, 1))
    sh, sw = min(size, h), min(size, w)
    y0, x0 = h // 2 - sh // 2, w // 2 - sw // 2
    return panel(label, rolled[y0:y0 + sh, x0:x0 + sw], [w // 2 - x0], [h // 2 - y0])


def stack(parts, axis):
    """Canvases side by side (axis 1) or one under another (axis 0) on the background colour."""
    other = 1 - axis
    shape = [0, 0, 3]
    shape[other] = max(p.shape[other] for p in parts)
    shape[axis] = sum(p.shape[axis] for p in parts) + GAP * (len(parts) - 1)
    canvas = np.full(shape, BG, np.uint8)
    pos = 0
    for p in parts:
        if axis == 0:
            canvas[pos:pos + p.shape[0], :p.shape[1]] = p
        else:
            canvas[:p.shape[0], pos:pos + p.shape[1]] = p
        pos += p.shape[axis] + GAP
    return canvas


def text_strip(lines, width):
    strip = np.full((22 * len(lines) + 10, width, 3), BG, np.uint8)
    for i, line in enumerate(lines):
        cv2.putText(strip, line, (MARGIN, 22 * (i + 1)), FONT, 0.55, INK, 1, cv2.LINE_AA)
    return strip


def landmark_panel(label, img8, heat, spots, size):
    """The tile in grey under a heat map of patch uniqueness (score 1 dark, 4 and above bright),
    with the top spots numbered. An analysis panel: it is never tiled, so nothing is drawn over
    a seam."""
    h, w = img8.shape[:2]
    f = min(1.0, size / max(h, w))
    tw, th = max(1, round(w * f)), max(1, round(h * f))
    base = cv2.resize(img8, (tw, th), interpolation=cv2.INTER_AREA) if f < 1 else img8
    grey = cv2.cvtColor(cv2.cvtColor(base, cv2.COLOR_BGR2GRAY), cv2.COLOR_GRAY2BGR)
    heat = cv2.resize(heat, (tw, th), interpolation=cv2.INTER_CUBIC)
    colour = cv2.applyColorMap((np.clip((heat - 1) / 3, 0, 1) * 255).astype(np.uint8), cv2.COLORMAP_INFERNO)
    out = cv2.addWeighted(grey, 0.45, colour, 0.55, 0)
    for i, spot in enumerate(spots, 1):
        centre = (round(spot['x'] * f), round(spot['y'] * f))
        cv2.circle(out, centre, max(6, round(max(tw, th) / 32)), INK, 1, cv2.LINE_AA)
        cv2.putText(out, str(i), (centre[0] + 5, centre[1] - 5), FONT, 0.45, INK, 1, cv2.LINE_AA)
    return panel(label, out, [], [])


def write_field(path, tiles8, layout, shift_frac, label):
    """The field preview: 10 x 10 tiles as a distant view, with a one-line header."""
    field = field_image(tiles8, layout, shift_frac)
    cv2.imwrite(str(path), np.concatenate([text_strip([label], field.shape[1]), field], axis=0))


def lit8(values, m):
    """A normal map lit from the upper left (Lambert), in grey: what the material makes of it. A cell
    whose normals were not turned with it is lit from another side than its neighbours."""
    n = m.decode(values)[..., :3]                                      # B, G, R = z, y, x
    up = -n[..., 1] if m.green_down else n[..., 1]
    shade = np.clip(0.7 * n[..., 0] + 0.5 * up - 0.5 * n[..., 2], 0, 1) / 0.995
    return np.repeat(np.floor(np.clip(shade, 0, 1) * 255 + 0.5).astype(np.uint8)[..., None], 3, axis=2)


def verify_at_scale(values_list, m, path, layout='grid', shift_frac=0.0, what='the tile', show=None):
    """Field preview and grid-read hint for one tile or a set of variants -> report entry, warnings.
    For rotate every cell also takes a random quarter turn (normal vectors turned with it); `show`
    draws a tile (default display8: the map as it is; lit8: a normal map lit)."""
    show = show or display8
    if layout == 'rotate':
        views = [turned_values(v, m) for v in values_list]
        read = grid_read([x for vs in views for x in vs], m)
        tiles8 = [[show(x, m) for x in vs] for vs in views]
    else:
        read = grid_read(values_list, m)
        tiles8 = [show(v, m) for v in values_list]
    turns = ', a random quarter turn per tile' if layout == 'rotate' else ''
    label = (f'{what}: 10 x 10 at 200 px per tile (a distant mip){turns}   grid read {read["mip16"]:.3f} at a 16 px '
             f'mip (a hint: judge this view)')
    if path:
        write_field(path, tiles8, layout, shift_frac, label)
    warnings = []
    if read['mip16'] > GRID_WARN:
        warnings.append(f'{m.path.name}: {what} reads as a grid at a distance (shared-pattern contrast '
                        f'{read["mip16"]:.3f} > {GRID_WARN} at a 16 px mip); look at the field preview; consider '
                        f'--auto-remove, --variants or runtime macro variation')
    return {'field': str(path) if path else None, 'grid_read': read,
            'note': 'grid_read is a hint, not proof: judge the field preview'}, warnings


def write_preview(path, before8, after8, header, heat, spots, layout='grid', shift_frac=0.0, tiling8=None,
                  turned8=None):
    """turned8 (rotate): the after tile's four quarter turns; tiling8: what the 1:1 corner shows (the
    super-tile for brick/half-drop, four differently turned tiles for rotate)."""
    big, small = 960, 470
    marks = landmark_panel('landmarks: spots most likely to read as repeats', after8 if after8 is not None else before8,
                           heat, spots, small)
    if after8 is None:
        rows = [stack([tiled_panel('input tiled 2x2', before8, 2, big),
                       tiled_panel('input tiled 3x3: look for repeating landmarks', before8, 3, big)], 1),
                stack([crossing_panel('input: the corner where four tiles meet, 1:1', before8, small), marks], 1)]
    else:
        named = {'grid': '', 'rotate': ' (random quarter turns)'}.get(layout, f' ({layout} {shift_frac:g})')
        corner = after8 if tiling8 is None else tiling8
        tiles = turned8 if layout == 'rotate' else after8
        crossings = stack([crossing_panel('BEFORE: tile corner at 1:1', before8, small),
                           crossing_panel('AFTER: tile corner at 1:1' + {'grid': '', 'rotate': ': four turns meet'}.get(
                               layout, ' (super-tile)'), corner, small)], 0)
        rows = [stack([tiled_panel('BEFORE tiled 2x2', before8, 2, big),
                       tiled_panel('AFTER tiled 2x2' + named, tiles, 2, big, layout, shift_frac)], 1),
                stack([tiled_panel('AFTER tiled 3x3' + named + ': look for repeating landmarks', tiles, 3, big,
                                   layout, shift_frac), crossings, marks], 1)]
    body = stack(rows, 0)
    canvas = np.concatenate([text_strip(header, body.shape[1]), body], axis=0)
    if canvas.shape[1] > 2048:
        f = 2048 / canvas.shape[1]
        canvas = cv2.resize(canvas, (2048, round(canvas.shape[0] * f)), interpolation=cv2.INTER_AREA)
    cv2.imwrite(str(path), canvas)


# ---- the run -------------------------------------------------------------------------------

def parse_length(text):
    m = re.fullmatch(r'\s*([0-9]*\.?[0-9]+)\s*(mm|cm|m|in)?\s*', text)
    if not m:
        raise SystemExit(f'--physical-size: not a length: {text!r} (examples: 200cm, 2m, 200cm,150cm)')
    return float(m[1]), m[2]


def parse_physical(text, width, height):
    parts = re.split(r'[x,]', text)
    if len(parts) not in (1, 2):
        raise SystemExit(f'--physical-size: give one length (the width) or width,height: {text!r}')
    pw, unit_w = parse_length(parts[0])
    ph, unit_h = parse_length(parts[1]) if len(parts) == 2 else (pw * height / width, unit_w)
    if unit_w and unit_h and unit_w != unit_h:
        raise SystemExit('--physical-size: use one unit for width and height')
    return pw, ph, unit_w or unit_h or 'cm'


def parse_overlap(text, width, height):
    t = text.strip().lower()
    try:
        if t.endswith('px'):
            px = round(float(t[:-2]))
            return px, px
        frac = float(t[:-1]) / 100 if t.endswith('%') else float(t)
    except ValueError:
        raise SystemExit(f'--overlap: not a fraction, percentage or pixel count: {text!r}') from None
    if frac >= 1:
        return round(frac), round(frac)
    return round(frac * width), round(frac * height)


def same_file(a, b):
    return Path(a).resolve() == Path(b).resolve()


def classify(path, args):
    if any(same_file(p, path) for p in args.normal):
        return 'normal'
    if any(same_file(p, path) for p in args.data) or (args.height and same_file(args.height, path)):
        return 'data'
    tokens = name_tokens(path)
    if args.auto_normal and tokens & NORMAL_TOKENS:
        return 'normal'
    return 'data' if tokens & DATA_TOKENS else 'color'


def pick_height(maps, args):
    """The map whose low ground the cut prefers: --height, else a data map named like one."""
    if args.height:
        for i, m in enumerate(maps):
            if same_file(m.path, args.height):
                return i
        raise SystemExit(f'--height {args.height} is not one of the inputs')
    for i, m in enumerate(maps):
        if m.kind == 'data' and name_tokens(m.path) & HEIGHT_TOKENS:
            return i
    return None


def output_path(source, args, single):
    if args.out and single and Path(args.out).suffix.lower() in IMAGE_EXTS:
        return Path(args.out)
    folder = Path(args.out) if args.out else source.parent
    return folder / f'{source.stem}.seamless{source.suffix}'


def side_path(base, suffix):
    return base.with_name(base.name + suffix)


def with_boxes(heat, spots, h, w):
    """Give each landmark spot the box of the area it stands for: [x, y, width, height] px, x and y
    the top-left corner (they may lie outside the tile: the box wraps)."""
    for _, spot, box, _ in landmark_regions(heat, spots, h, w, len(spots)):
        spot['box'] = box
    return spots


def landmark_overlay(label, img8, heat, spots, removed, size=900):
    """The tile in grey under its landmark heat map, the top spots boxed and numbered, replaced
    regions outlined in green."""
    h, w = img8.shape[:2]
    f = min(1.0, size / max(h, w))
    tw, th = max(1, round(w * f)), max(1, round(h * f))
    base = cv2.resize(img8, (tw, th), interpolation=cv2.INTER_AREA) if f < 1 else img8
    grey = cv2.cvtColor(cv2.cvtColor(base, cv2.COLOR_BGR2GRAY), cv2.COLOR_GRAY2BGR)
    colour = cv2.applyColorMap((np.clip((cv2.resize(heat, (tw, th)) - 1) / 3, 0, 1) * 255).astype(np.uint8),
                               cv2.COLORMAP_INFERNO)
    out = cv2.addWeighted(grey, 0.55, colour, 0.45, 0)
    for region in removed:
        small = cv2.resize(region.astype(np.uint8), (tw, th), interpolation=cv2.INTER_NEAREST)
        contours = cv2.findContours(small, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)[0]
        cv2.drawContours(out, contours, -1, (80, 220, 80), 2, cv2.LINE_AA)
    for i, spot in enumerate(spots, 1):
        if 'box' in spot:
            x, y, bw, bh = (round(v * f) for v in spot['box'])
            cv2.rectangle(out, (x, y), (x + bw, y + bh), TICK, 1, cv2.LINE_AA)
            cv2.putText(out, f'{i} {spot["score"]:.1f}', (x + 3, y + 14), FONT, 0.45, INK, 1, cv2.LINE_AA)
    return panel(label, out, [], [])


def analyse(values, m):
    """Structure class and landmarks of a tile, with their warnings."""
    heat, spots = landmarks(values, m)
    spots = with_boxes(heat, spots, *values.shape[:2])
    structure = texture_structure(values, m)
    warnings = []
    if not structure['stochastic_tiling_safe']:
        warnings.append(f'{m.path.name}: reads as structured ({structure["class"]}, anisotropy '
                        f'{structure["anisotropy"]:.1f}): random rotation and hex/stochastic tiling at runtime would '
                        f'break it; see structure.next_steps')
    if spots and spots[0]['score'] > LANDMARK_WARN:
        warnings.append(f'{m.path.name}: a patch near ({spots[0]["x"]}, {spots[0]["y"]}) px is unlike anything else in '
                        f'the tile (landmark score {spots[0]["score"]:.1f}); every repeat will show it')
    return structure, heat, spots, warnings


def edge_jump(work, channels, band):
    """Mean step, in levels, between the first and last `band` columns and rows (band means)."""
    a = work[..., channels]
    jx = np.abs(a[:, -band:].mean(axis=1) - a[:, :band].mean(axis=1)).mean()
    jy = np.abs(a[-band:].mean(axis=0) - a[:band].mean(axis=0)).mean()
    return round(float(jx + jy) / 2 * 255, 2)


def prepare(maps, args, width, height):
    """Tone preparation, per map and without geometry (registration is untouched), reported as
    named steps: flatten (de-light large-scale shading on colour maps), then equalize (Moisan:
    the low-frequency mismatch between opposite edges), or the full Moisan for --method periodic."""
    steps = []
    text = str(args.flatten).strip().lower()
    if text in ('auto', 'on'):
        sigma = min(width, height) / 4
    elif text in ('off', '0', 'none', ''):
        sigma = None
    else:
        try:
            value = float(text)
        except ValueError:
            raise SystemExit(f"--flatten: 'auto', 'off' or a sigma in px, not {args.flatten!r}") from None
        sigma = value if value >= 1 else value * min(width, height)
    flattened = [m for m in maps if sigma and m.kind == 'color']
    if flattened:
        entry = {'step': 'flatten', 'sigma_px': round(sigma, 1), 'maps': {}}
        for m in flattened:
            before = macro_variation(m.encode(m.work), m)
            m.work = flatten(m.work, m.tone, sigma)
            entry['maps'][m.path.name] = {'macro_before': before, 'macro_after': macro_variation(m.encode(m.work), m)}
        steps.append(entry)
    band = max(4, min(width, height) // 16)
    if args.method == 'periodic' or args.equalize:
        entry = {'step': 'periodic_smooth' if args.method == 'periodic' else 'equalize',
                 'band_px': None if args.method == 'periodic' else band, 'maps': {}}
        for m in maps:
            if m.hidden:
                continue
            vector = [0, 1, 2] if m.kind == 'normal' else m.tone
            before = edge_jump(m.work, vector, band)
            if args.method == 'periodic':
                m.work = m.work - periodic_smooth(m.work, vector)
            else:
                m.work = m.work - periodic_smooth(m.work, vector, band=band)
            entry['maps'][m.path.name] = {'edge_jump_before': before, 'edge_jump_after': edge_jump(m.work, vector, band)}
        steps.append(entry)
    for m in maps:
        if m.kind == 'normal':
            m.work[..., :3] = unit(m.work[..., :3])
    return steps


def parse_layout(args):
    """--layout and --offset -> (layout, offset fraction, q) where q tiles make the shift whole."""
    layout = args.layout
    if layout in ('grid', 'rotate'):           # rotate: the grid cut first, then the rotatable frame
        return layout, 0.0, 1
    if args.method != 'overlap':
        raise SystemExit(f'--layout {layout} needs --method overlap')
    frac = float(args.offset) % 1
    for q in (2, 3, 4):
        if abs(frac * q - round(frac * q)) < 1e-6 and round(frac * q) % q:
            return layout, round(frac * q) / q, q
    raise SystemExit(f'--offset {args.offset}: use a fraction with a denominator of 2, 3 or 4 (0.5, 1/3 = 0.3333, '
                     f'0.25, 0.75, ...) so the layout repeats after a few rows')


def make(args):
    paths = [Path(p) for p in args.inputs]
    guide_path = Path(args.guide) if args.guide else paths[0]
    maps = [Map(p, classify(p, args)) for p in paths]
    found = [i for i, p in enumerate(paths) if same_file(p, guide_path)]
    if found:
        guide = found[0]
    else:
        maps.append(Map(guide_path, classify(guide_path, args), write=False))
        guide = len(maps) - 1
    mask_index = None
    if args.remove_mask:            # a hidden map: it follows the same cuts into tile coordinates
        mask = Map(Path(args.remove_mask), 'mask', write=False)
        mask.values = mask.values[..., :1].copy()
        mask.work = mask.values.copy()
        maps.append(mask)
        mask_index = len(maps) - 1
    if len({m.values.shape[:2] for m in maps}) > 1:
        listed = ', '.join(f'{m.path} {m.values.shape[1]}x{m.values.shape[0]}' for m in maps)
        raise SystemExit(f'the maps of a set must share one size to be cut identically: {listed}')
    height, width = maps[0].values.shape[:2]
    feather = int(args.feather)
    warnings = []
    seam = args.seam
    if seam == 'graphcut' and importlib.util.find_spec('maxflow') is None:
        warnings.append('--seam graphcut needs PyMaxflow (pip install PyMaxflow); the dp seam was used')
        seam = 'dp'

    for m in maps:
        m.before = None if m.hidden else measure(m.values, m)
    prep = prepare(maps, args, width, height)

    height_index = pick_height(maps, args)
    cm = CostModel(maps, guide, args.seam_from, height_index, args.height_route, args.height_blend)
    seams, ox, oy = [], 0, 0
    layout, s_frac, q = parse_layout(args)
    if layout == 'rotate' and (width != height or (args.method == 'overlap' and not args.keep_size)):
        raise SystemExit(f'--layout rotate needs a square tile ({width}x{height} given'
                         + ('' if width != height else '; --no-keep-size can leave it oblong') + ')')
    flip = layout == 'half-drop'                       # half-drop runs as brick on transposed images

    def turn(a):
        return np.ascontiguousarray(a.transpose(1, 0, 2) if a.ndim == 3 else a.T) if flip else a
    works = [turn(m.work) for m in maps]
    tw, th = (height, width) if flip else (width, height)          # the tile in the brick frame
    axes = ('y', 'x') if flip else ('x', 'y')
    s_cut = 0
    if args.method in ('overlap', 'offset'):
        if min(width, height) < 32:
            raise SystemExit(f'the image is too small to cut ({width}x{height}); use --method periodic')
        ox, oy = parse_overlap(args.overlap, tw, th)
        spread = 0 if args.overlap.strip().lower().endswith('px') else 3

        def limit(size):
            return 0.45 * size if args.method == 'overlap' else size / 2 - 8
        for o, size, axis in ((ox, tw, axes[0]), (oy, th, axes[1])):
            if o < 2 * feather + 3 + spread:
                raise SystemExit(f'--overlap: {o} px along {axis} is too narrow for a {feather} px feather '
                                 f'(needs at least {2 * feather + 3 + spread})')
            if o + spread > limit(size):
                raise SystemExit(f'--overlap: {o} px along {axis} is too wide for {size} px '
                                 f'(at most {int(limit(size)) - spread})')
        opts = {'feather': feather, 'level': args.level, 'seam': seam, 'spread': spread,
                'height_blend': args.height_blend, 'limit': limit, 'align': args.align}
        run_pass = overlap_pass if args.method == 'overlap' else offset_pass
        works, stats, ox = run_pass(works, maps, cm, ox, dict(opts, multiple=q), False, axes[0])
        seams += stats
        s_cut = round(s_frac * works[0].shape[1]) if layout != 'grid' else 0
        works = [np.ascontiguousarray(a.transpose(1, 0, 2)) for a in works]
        works, stats, oy = run_pass(works, maps, cm, oy, dict(opts, shift=s_cut), True, axes[1])
        seams += stats
        works = [np.ascontiguousarray(a.transpose(1, 0, 2)) for a in works]
    cut_h, cut_w = works[0].shape[:2]
    if args.keep_size:
        works = [resample_periodic(a, th, tw - tw % q, s_cut) for a in works]
    s_out = round(s_frac * works[0].shape[1]) if layout != 'grid' else 0
    if layout == 'rotate':          # measured on the seamless tile: an input's own wrap jump reads as structure
        warnings += rotation_guard(texture_structure(maps[guide].encode(works[guide]), maps[guide]),
                                   maps[guide].path.name, args.allow_structured)
    removal, removed, heat_before = None, [], None
    if mask_index is not None or args.auto_remove:
        removal_preview = display8(maps[guide].encode(turn(works[guide])), maps[guide])
        works, removal, removed, heat_before, _ = remove_landmarks(works, maps, cm, guide, mask_index,
                                                                   args.auto_remove, feather, s_out)
        removed, heat_before = [turn(r) for r in removed], turn(heat_before)
        skipped = [r for r in removal['regions'] if 'skipped' in r]
        if skipped:
            warnings.append(f'{len(skipped)} landmark region(s) could not be replaced: no donor far enough from '
                            f'the other regions; remove fewer at once')

    variant_works, variants = [], None
    if args.variants > 1:
        variant_works, variants = make_variants(works, maps, cm, args.variants, feather,
                                                importlib.util.find_spec('maxflow') is not None, s_out)
        variant_works = [[turn(a) for a in vw] for vw in variant_works]
        if variants['count'] < args.variants:
            warnings.append(f'only {variants["count"]} variants: no further offsets far enough from the others')

    rotation = None
    if layout == 'rotate':          # one frame for the tile and every variant, each joined by its own cut
        gm = maps[guide]
        heat = landmarks(gm.encode(works[guide]), gm)[0]
        heat = cv2.resize(heat, (works[0].shape[1], works[0].shape[0]), interpolation=cv2.INTER_LINEAR)
        bands, rotation = rotation_frame(works, maps, cm, heat, feather)
        works, rotation['joins'], flats = paste_frame(works, maps, cm, bands, feather, seam)
        variant_works = [paste_frame(vw, maps, cm, bands, feather, seam, flats)[0] for vw in variant_works]
        if variants:
            variants['shared_band_px'] = rotation['band_px']
        for s in rotation['cuts'] + rotation['joins']:
            if s['visibility'] > SEAM_WARN:
                warnings.append(f'rotate: the {s.get("cut") or s["axis"] + " join"} cut has visibility {s["visibility"]:.2f} > '
                                f'{SEAM_WARN}; look along the tile edges and corners in the field preview')

    works = [turn(a) for a in works]
    if flip:
        cut_h, cut_w = cut_w, cut_h
    out_h, out_w = works[0].shape[:2]

    def tiling(values):
        """What a plain grid of UVs repeats: the tile, or for brick/half-drop the super-tile."""
        if layout in ('grid', 'rotate'):
            return values
        return turn(super_tile(turn(values), s_out))
    content_scale = [round(width / cut_w, 4), round(height / cut_h, 4)]
    physical = None
    if args.physical_size:
        pw, ph, unit_name = parse_physical(args.physical_size, width, height)
        physical = {'unit': unit_name, 'input': [round(pw, 3), round(ph, 3)],
                    'output_tile': [round(pw * cut_w / width, 3), round(ph * cut_h / height, 3)]}

    for s in seams:
        if s['visibility'] > SEAM_WARN:
            warnings.append(f'{s["axis"]} cut ({s["band_px"][0]}-{s["band_px"][1]} px) visibility {s["visibility"]:.2f} > '
                            f'{SEAM_WARN}: it joins pixels that differ more than the texture does across a blend; '
                            f'structure crosses the cut: look at the AFTER 2x2 and 1:1 corner; try --seam graphcut, '
                            f'a larger --overlap or a height map')
    if args.method == 'overlap' and max(ox / width, oy / height) > 0.25:
        warnings.append(f'overlap {ox}x{oy} px crops more than a quarter of the tile; the content is magnified '
                        f'{content_scale[0]}x')
    if args.method == 'offset':
        warnings.append('offset: the middle half of the source (1/4 to 3/4 of each axis) now appears twice per '
                        'tile, so repetition is likely visible; judge the 3x3 preview')
    if args.method == 'periodic':
        warnings.append('periodic: only colour and lighting were matched across the edges; structure that '
                        'crosses an edge is still cut; check the 1:1 corner')

    single = len(paths) == 1
    report_maps, guide_after, normal_after = [], None, None
    for m, a in zip(maps, works):
        if m.hidden:
            continue
        values = m.encode(a)
        entry = {'input': str(m.path), 'kind': m.kind, 'format': m.fmt, 'before': m.before, 'output': None}
        if m.write:
            out = output_path(m.path, args, single)
            out.parent.mkdir(parents=True, exist_ok=True)
            clipped = write_image(out, values, m.fmt, args.quality)
            if m.fmt['dtype'] in ('uint8', 'uint16'):
                top = 255 if m.fmt['dtype'] == 'uint8' else 65535
                values = np.floor(np.clip(values, 0, 1) * top + 0.5) / top
            entry['output'] = str(out)
            if layout in ('brick', 'half-drop'):
                big = tiling(m.encode(a))
                entry['super_output'] = str(out.with_name(out.stem + '.super' + out.suffix))
                write_image(Path(entry['super_output']), big, m.fmt, args.quality)
            # a map already saturated at a bound (a roughness of 1) overshoots it harmlessly
            excess = sum(max(0.0, c - float(at)) for c, at in zip(clipped, ((m.values <= 0).mean(), (m.values >= 1).mean())))
            if excess > 0.005:
                m.warnings.append(f'{m.path.name}: {excess:.1%} of values were clipped to the format range beyond '
                                  'what the input already had at its limits; try --no-equalize')
            if out.suffix.lower() in ('.jpg', '.jpeg'):
                m.warnings.append(f'{out.name}: JPEG is lossy; prefer PNG or TIF for texture sources')
        values = values.astype(np.float32)
        entry['after'] = measure(tiling(values), m)
        if layout == 'rotate':
            entry['turns'] = turn_metrics(values, m)
            if not entry['turns']['seamless']:
                m.warnings.append(f'{m.path.name}: {len(entry["turns"]["seams"])} of 16 turned pairings show a seam '
                                  f'(worst wrap {entry["turns"]["worst_wrap"]:.2f} at turns {entry["turns"]["worst_turns"]})')
        if m.kind == 'normal':
            length = np.linalg.norm(values[..., :3] if m.signed else values[..., :3] * 2 - 1, axis=2)
            entry['normal_length'] = [round(float(length.min()), 4), round(float(length.max()), 4)]
        m.warnings += metric_warnings(m.path.name, m.before, entry['after'])
        entry['warnings'] = m.warnings
        report_maps.append(entry)
        if m is maps[guide]:
            guide_after = values
        if m.kind == 'normal' and normal_after is None:
            normal_after = (m, [values])
    variant_guides = []
    if variants:
        variants['outputs'] = {}
        index = {id(m): i for i, m in enumerate(maps)}
        for k, vw in enumerate(variant_works, 2):
            for m in maps:
                if m.hidden:
                    continue
                values = m.encode(vw[index[id(m)]])
                if m.write:
                    out = output_path(m.path, args, single)
                    out = out.with_name(out.stem + f'.v{k}' + out.suffix)
                    write_image(out, values, m.fmt, args.quality)
                    variants['outputs'].setdefault(m.path.name, []).append(str(out))
                    if m.fmt['dtype'] in ('uint8', 'uint16'):
                        top = 255 if m.fmt['dtype'] == 'uint8' else 65535
                        values = np.floor(np.clip(values, 0, 1) * top + 0.5) / top
                if m is maps[guide]:
                    variant_guides.append(values.astype(np.float32))
                if normal_after and m is normal_after[0]:
                    normal_after[1].append(values.astype(np.float32))

    guide_map = maps[guide]
    structure, heat, spots, extra = analyse(guide_after, guide_map)
    warnings += extra
    base = output_path(guide_map.path, args, single).with_suffix('')
    preview = None if args.preview == 'none' else Path(args.preview) if args.preview else side_path(base, '.preview.png')
    report_path = side_path(base, '.json')
    report_path.parent.mkdir(parents=True, exist_ok=True)
    g_entry = report_maps[guide]
    if preview:
        header = [f'{guide_map.path.name}  {args.method}  {width}x{height} -> {out_w}x{out_h}'
                  + (f'  overlap {ox}x{oy} px  content scale {content_scale[0]:g}' if args.method == 'overlap' else '')
                  + f'  reads as {structure["class"]}',
                  summary_metrics(g_entry['before'], g_entry['after']) + '   ticks = tile boundaries']
        preview.parent.mkdir(parents=True, exist_ok=True)
        turned8 = corner8 = None
        if layout == 'rotate':           # the 1:1 corner: four tiles, four different turns
            turned8 = [display8(v, guide_map) for v in turned_values(guide_after, guide_map)]
            corner8 = np.concatenate([np.concatenate(turned8[:2], axis=1),
                                      np.concatenate([turned8[3], turned8[2]], axis=1)], axis=0)
        write_preview(preview, display8(guide_map.values, guide_map), display8(guide_after, guide_map), header,
                      heat, spots, layout, s_frac, corner8 if turned8 else display8(tiling(guide_after), guide_map),
                      turned8)
    field = None if args.preview == 'none' else side_path(base, '.field.png')
    verify, extra = verify_at_scale([guide_after], guide_map, field, layout, s_frac)
    warnings += extra
    if variants:
        vfield = None if args.preview == 'none' else side_path(base, '.variants.field.png')
        variants['verify'], extra = verify_at_scale([guide_after] + variant_guides, guide_map, vfield, layout, s_frac,
                                                    what=f'{variants["count"]} variants, one at random per tile')
        warnings += extra
    if rotation is not None and normal_after and args.preview != 'none':
        nm, nvalues = normal_after          # the same cells, variants and turns as the colour field, lit
        rotation['normal_field'] = str(side_path(base, '.normal-field.png'))
        verify_at_scale(nvalues, nm, Path(rotation['normal_field']), layout,
                        what=f'{nm.path.name} lit from the upper left, normals turned with each cell', show=lit8)
    overlay = None if args.preview == 'none' else side_path(base, '.landmarks.png')
    if overlay:
        panels = []
        if removal:
            panels.append(landmark_overlay('before removal: numbered = most likely to read as repeats',
                                           removal_preview, heat_before, removal['landmarks_before'], []))
        panels.append(landmark_overlay(('after removal: green = replaced from a donor elsewhere in the tile'
                                        if removal else 'numbered = most likely to read as repeats'),
                                       display8(guide_after, guide_map), heat, spots, removed))
        cv2.imwrite(str(overlay), stack(panels, 1))

    shifted = layout in ('brick', 'half-drop')
    report = {'tool': 'make_seamless', 'mode': 'make', 'method': args.method, 'guide': str(guide_map.path),
              'input_size': [width, height], 'cut_size': [cut_w, cut_h], 'output_size': [out_w, out_h],
              'overlap_px': [ox, oy] if args.method != 'periodic' else None, 'feather_px': feather,
              'seam': seam if args.method != 'periodic' else None,
              'seam_from': [str(maps[i].path) for i in cm.index],
              'height_map': str(maps[height_index].path) if cm.height is not None else None,
              'height_route': args.height_route if cm.height is not None else None,
              'height_blend': bool(args.height_blend) if cm.height is not None else None,
              'prep': prep,
              'level': bool(args.level), 'content_scale': content_scale, 'physical_size': physical,
              'seams': seams, 'structure': structure, 'landmarks': spots, 'landmarks_overlay': str(overlay) if overlay else None,
              'removal': removal, 'variants': variants, 'verify': verify,
              'layout': dict({'name': layout, 'offset': s_frac if shifted else None, 'shift_px': s_out if shifted else None,
                              'super_tiles': lattice_rows(out_h if flip else out_w, s_out) if shifted else 1},
                             **({'turns': 'a random quarter turn per cell, hard-edged', **rotation,
                                 'runtime': ROTATE_RUNTIME} if rotation else {})),
              'maps': report_maps,
              'preview': str(preview) if preview else None, 'report': str(report_path), 'warnings': warnings,
              'ok': all(e['after']['seamless'] and e.get('turns', {'seamless': True})['seamless'] for e in report_maps)}
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    return report


def summary_metrics(before, after):
    def pair(key):
        return f'{before[key]:.2f}->{after[key]:.2f}' if after else f'{before[key]:.2f}'
    s = after or before
    macro = f'  macro {s["macro"]:.3f}' if s['macro'] is not None else ''
    return f'wrap x {pair("wrap_x")} y {pair("wrap_y")}  low x {pair("low_x")} y {pair("low_y")}{macro}'


def check(args):
    reports = []
    for p in map(Path, args.inputs):
        m = Map(p, classify(p, args))
        before = measure(m.values, m)
        structure, heat, spots, extra = analyse(m.values, m)
        folder = Path(args.out) if args.out else p.parent
        folder.mkdir(parents=True, exist_ok=True)
        base = folder / f'{p.stem}.check'
        if args.preview == 'none':
            preview = None
        else:
            preview = Path(args.preview) if args.preview and len(args.inputs) == 1 else side_path(base, '.preview.png')
        h, w = m.values.shape[:2]
        if preview:
            write_preview(preview, display8(m.values, m), None,
                          [f'{p.name}  check  {w}x{h}  ' + ('SEAMLESS' if before['seamless'] else 'NOT SEAMLESS')
                           + f'  reads as {structure["class"]}',
                           summary_metrics(before, None) + '   ticks = tile boundaries'], heat, spots)
        verify, more = verify_at_scale([m.values], m, side_path(base, '.field.png') if preview else None)
        report = {'tool': 'make_seamless', 'mode': 'check', 'input': str(p), 'kind': m.kind, 'format': m.fmt,
                  'size': [w, h], 'metrics': before, 'structure': structure, 'landmarks': spots, 'verify': verify,
                  'warnings': m.warnings + metric_warnings(p.name, before, None) + extra + more,
                  'preview': str(preview) if preview else None, 'report': str(side_path(base, '.json')),
                  'ok': before['seamless']}
        Path(report['report']).write_text(json.dumps(report, indent=2) + '\n')
        reports.append(report)
    return reports


def parse_args(argv):
    p = argparse.ArgumentParser(
        description='Make texture maps tile seamlessly with identical cuts across a texture set, or --check '
                    'one. Writes <name>.seamless.<ext>, a preview PNG and a JSON report.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    p.add_argument('inputs', nargs='+', help='image files of one texture set (PNG/TIF 8/16-bit, JPG, TGA, EXR); '
                                             'the first is the guide; globs are expanded')
    p.add_argument('-o', '--out', help='output directory (or an image path when there is one input)')
    p.add_argument('--method', choices=['overlap', 'offset', 'periodic'], default='overlap',
                   help='overlap: cut the right band over the left, no duplicated content, the tile shrinks by '
                        'the overlap and is resampled back; offset: half offset with cut bands at 1/4 and 3/4, '
                        'keeps size, middle half appears twice; periodic: colour/lighting match only (Moisan)')
    p.add_argument('--check', action='store_true',
                   help='only measure each input as it is and write <name>.check.preview.png/.json')
    p.add_argument('--overlap', default='0.125',
                   help='band width: a fraction of each side (0.125), a percentage (12.5%%) or exact pixels '
                        '(160px); a fraction or percentage may move up to 3 px so the new wrap is an ordinary '
                        'pixel boundary')
    p.add_argument('--feather', type=int, default=4, help='half-width in px of the blend across a cut')
    p.add_argument('--seam', choices=['dp', 'graphcut'], default='dp',
                   help='dp: minimum-cost path, one column per row; graphcut: cut of any shape (Kwatra 2003), '
                        'needs pip install PyMaxflow, falls back to dp without it')
    p.add_argument('--align', choices=['auto', 'on', 'off'], default='auto',
                   help='morph the two overlapping bands towards each other along a smooth optical-flow field '
                        'before cutting (overlap method), so ripples, grain and cracks meet instead of breaking; '
                        'auto uses it when it removes at least a quarter of the band difference')
    p.add_argument('--seam-from', choices=['all', 'guide'], default='all',
                   help='maps whose differences steer the cut: every map of the set, or only the guide')
    p.add_argument('--height', help='height map of the set whose low ground the cut prefers; without it, a '
                                    'data map named _H, _Height, _Disp, ... is used')
    p.add_argument('--height-route', type=float, default=1.0,
                   help='how strongly the cut prefers low height, relative to the mean difference cost; 0 is off')
    p.add_argument('--height-blend', action=argparse.BooleanOptionalAction, default=True,
                   help='near a cut, let the side with the higher height win (bricks and stones stay whole)')
    p.add_argument('--remove-mask', help='a mask image the size of the inputs: white areas are replaced by '
                                         'donor patches from elsewhere in the finished tile, the same in every map')
    p.add_argument('--auto-remove', type=int, default=0,
                   help='replace the N most distinctive landmarks (those likeliest to read as repeats) the same way')
    p.add_argument('--variants', type=int, default=1,
                   help='write N (2-4) tiles that share their border strip and differ inside, so any fits next to '
                        'any other: <name>.seamless.v2.<ext> ... and a field preview with a random one per tile')
    p.add_argument('--layout', choices=['grid', 'brick', 'half-drop', 'rotate'], default='grid',
                   help='how the tiles are laid: grid; brick (each row of tiles moved sideways by --offset); '
                        'half-drop (each column moved down); rotate (a random quarter turn per tile: every side '
                        'gets one point-symmetric band and every corner a 4-fold symmetric patch, so any side '
                        'meets any side; the material must turn normal XY with the tile). The cut pairs the '
                        'edges that layout joins and previews are drawn in it; brick and half-drop also write a '
                        '<name>.seamless.super.<ext> that repeats on plain UVs')
    p.add_argument('--offset', type=float, default=0.5, help='brick/half-drop offset, a share of the tile (0.5, 0.25, 1/3)')
    p.add_argument('--allow-structured', action='store_true',
                   help='--layout rotate on a texture the structure check reads as directional or regular (grain, '
                        'ripples, bricks): warn loudly instead of refusing')
    p.add_argument('--equalize', action=argparse.BooleanOptionalAction, default=True,
                   help='remove low-frequency edge mismatch (Moisan periodic+smooth) before cutting')
    p.add_argument('--flatten', default='auto',
                   help="de-light: divide out large-scale luminance (shading, vignetting, broad blotches) on colour "
                        "maps. 'auto' uses a blur of a quarter of the shorter side on colour maps only (data, height "
                        "and normal maps are left alone); 'off'; or a sigma in px (a fraction of the side when < 1)")
    p.add_argument('--level', action=argparse.BooleanOptionalAction, default=True,
                   help='spread any remaining tone difference at a cut across its band')
    p.add_argument('--keep-size', action=argparse.BooleanOptionalAction, default=True,
                   help='resample an overlap result back to the input size (periodic Lanczos)')
    p.add_argument('--guide', help='the map shown in the preview, analysed and used to align bands; the first '
                                   'input when omitted')
    p.add_argument('--normal', action='append', default=[], help='treat this file as a normal map (repeatable)')
    p.add_argument('--data', action='append', default=[], help='treat this file as a data map (repeatable)')
    p.add_argument('--auto-normal', action=argparse.BooleanOptionalAction, default=True,
                   help='detect normal maps by name (_N, _Normal, NormalGL, NormalDX, -normal-dx)')
    p.add_argument('--physical-size', help='real width (or width,height) the input covers, e.g. 200cm; the '
                                           'report states what the output tile covers')
    p.add_argument('--quality', type=int, default=95, help='JPEG quality for .jpg outputs')
    p.add_argument('--preview', help="preview PNG path, or 'none' (default: <guide>.seamless.preview.png)")
    p.add_argument('--json', action='store_true', help='print the report as JSON instead of a summary')
    args = p.parse_args(argv)
    args.inputs = [f for pattern in args.inputs for f in (sorted(glob.glob(pattern)) or [pattern])]
    if args.feather < 0:
        p.error('--feather must be 0 or more')
    if args.height_route < 0:
        p.error('--height-route must be 0 or more')
    if not 1 <= args.variants <= 4:
        p.error('--variants takes 1 to 4')
    return args


def print_structure(structure, spots):
    s = structure
    detail = f'regularity {s["regularity"]:.2f}' if s['class'] == 'regular' else f'coherence {s["coherence"]:.2f}'
    detail += f', anisotropy {s["anisotropy"]:.1f}'
    if s['grain_degrees'] is not None:
        detail += f', grain {s["grain_degrees"]:g} deg'
    tiling = 'stochastic tiling safe' if s['stochastic_tiling_safe'] else 'no random rotation or stochastic tiling'
    offsets = '' if s['offset_blend_safe'] else ', keep the grid'
    marks = ', '.join(f'{t["score"]:.1f} at ({t["x"]}, {t["y"]})' for t in spots[:3])
    print(f'  reads as {s["texture_type"]} {s["class"]} ({detail}): {tiling}{offsets}  landmarks {marks}')


def print_summary(report):
    if report['mode'] == 'check':
        print(f'{report["input"]}  {report["size"][0]}x{report["size"][1]}  '
              f'{"seamless" if report["ok"] else "NOT seamless"}  {summary_metrics(report["metrics"], None)}')
        print_structure(report['structure'], report['landmarks'])
        for w in report['warnings']:
            print(f'  warning: {w}')
        return
    for e in report['maps']:
        if e['output'] is None:
            continue
        print(f'{e["input"]} -> {e["output"]}  {e["kind"]}  {report["input_size"][0]}x{report["input_size"][1]}'
              f' -> {report["output_size"][0]}x{report["output_size"][1]}  {summary_metrics(e["before"], e["after"])}')
    line = f'  method {report["method"]}'
    if report['method'] == 'overlap':
        line += (f'  overlap {report["overlap_px"][0]}x{report["overlap_px"][1]} px  content scale '
                 f'{report["content_scale"][0]:g} x {report["content_scale"][1]:g}')
    if report['seams']:
        line += f'  {report["seam"]} cut visibility ' + ' '.join(f'{s["axis"]} {s["visibility"]:.2f}' for s in report['seams'])
    if report['height_map']:
        uses = [f'route {report["height_route"]:g}'] if report['height_route'] else []
        uses += ['taller wins'] if report['height_blend'] else []
        line += f'  height {Path(report["height_map"]).name} ({", ".join(uses)})'
    print(line)
    if report['physical_size']:
        ps = report['physical_size']
        print(f'  physical: input {ps["input"][0]:g} x {ps["input"][1]:g} {ps["unit"]}, output tile covers '
              f'{ps["output_tile"][0]:g} x {ps["output_tile"][1]:g} {ps["unit"]}')
    print_structure(report['structure'], report['landmarks'])
    for w in report['warnings'] + [w for e in report['maps'] for w in e['warnings']]:
        print(f'  warning: {w}')
    print(f'  grid read {report["verify"]["grid_read"]["mip16"]:.3f} (hint)  field {report["verify"]["field"]}')
    lay = report['layout']
    if lay.get('turns'):
        turns = [e['turns'] for e in report['maps'] if 'turns' in e]
        worst = max(turns, key=lambda t: t['worst_wrap'])
        print(f'  rotate: band {lay["band_px"]} px a side (a {lay["strip_px"]} px point-symmetric strip on every edge), '
              f'corner patch {lay["corner_px"]} px (4-fold), join {lay["join_px"]} px; 16 turned pairings: worst wrap '
              f'{worst["worst_wrap"]:.2f} at turns {worst["worst_turns"]}, mean {worst["mean_wrap"]:.2f}, '
              f'{"no seam" if all(t["seamless"] for t in turns) else "SEAMS"}')
        print('  rotate: the material must turn the UVs and the normal map\'s tangent XY by the same quarter turn per cell'
              + (f'  normal field {lay["normal_field"]}' if lay.get('normal_field') else ''))
    if report.get('variants'):
        v = report['variants']
        print(f'  variants {v["count"]}, shared border {v.get("shared_band_px", v["border_px"])} px: grid read '
              f'{v["verify"]["grid_read"]["mip16"]:.3f} (hint)  field {v["verify"]["field"]}')
    print(f'  preview {report["preview"]}  report {report["report"]}')


def main(argv=None):
    args = parse_args(argv)
    reports = check(args) if args.check else [make(args)]
    if args.json:
        print(json.dumps(reports[0] if len(reports) == 1 else reports, indent=2))
    else:
        for report in reports:
            print_summary(report)
    return reports


if __name__ == '__main__':
    sys.exit(0 if all(r['ok'] for r in main()) else 1)
