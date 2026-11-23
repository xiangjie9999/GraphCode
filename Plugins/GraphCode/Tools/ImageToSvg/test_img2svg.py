"""img2svg against ChatGPT-like fixtures: drawn at 4x and downsampled (real anti-aliasing), a soft
lighting gradient, noise and a JPEG round trip. Run `python test_img2svg.py` or pytest."""
import json
import math
import re
import tempfile
from pathlib import Path

import cv2
import numpy as np

import img2svg as m

STAR = [(512, 287), (575, 450), (737, 450), (605, 550), (655, 725), (512, 620), (369, 725), (419, 550),
        (287, 450), (449, 450)]


def badge(background='white'):
    """A ring with a star in its hole, a hole in the star, and two separate navy tabs: the sort of
    crest an author would extrude. Returns BGR, or BGRA for background='alpha'."""
    big = np.zeros((4096, 4096, 3), np.uint8)
    cv2.circle(big, (2048, 2048), 1500, (60, 90, 210), -1)
    cv2.circle(big, (2048, 2048), 1100, (0, 0, 0), -1)
    cv2.fillPoly(big, [np.int32(STAR) * 4], (30, 170, 240))
    cv2.circle(big, (2048, 2048), 120, (0, 0, 0), -1)
    cv2.rectangle(big, (200, 200), (700, 600), (120, 60, 20), -1)
    cv2.rectangle(big, (3396, 200), (3896, 600), (120, 60, 20), -1)
    cover = cv2.resize((big == 0).all(2).astype(np.float32), (1024, 1024), interpolation=cv2.INTER_AREA)
    paint = cv2.resize(big.astype(np.float32), (1024, 1024), interpolation=cv2.INTER_AREA)
    if background == 'alpha':
        colour = paint / np.maximum(1e-3, 1 - cover[..., None])
        return np.clip(np.dstack([colour, (1 - cover) * 255]), 0, 255).astype(np.uint8)
    if background == 'checker':
        yy, xx = np.mgrid[0:1024, 0:1024]
        under = np.where(((yy // 32 + xx // 32) % 2)[..., None] == 0, 255.0, 204.0) * np.ones(3)
    else:
        under = np.full((1024, 1024, 3), 248.0)
    img = paint + under * cover[..., None]
    yy, xx = np.mgrid[0:1024, 0:1024] / 1024
    img = np.where(cover[..., None] > 0.5, img, img * (1 + 0.08 * (0.5 - yy) + 0.04 * (xx - 0.5))[..., None])
    img = np.clip(img + np.random.default_rng(1).normal(0, 3, img.shape), 0, 255).astype(np.uint8)
    return cv2.imdecode(cv2.imencode('.jpg', img, [cv2.IMWRITE_JPEG_QUALITY, 82])[1], cv2.IMREAD_COLOR)


def sketch(draw):
    """Line art: draw(canvas) at 4x on near-white paper, downsampled, with noise and JPEG."""
    big = np.full((4096, 4096, 3), 248, np.uint8)
    draw(big)
    img = cv2.resize(big, (1024, 1024), interpolation=cv2.INTER_AREA).astype(np.float32)
    img = np.clip(img + np.random.default_rng(2).normal(0, 3, img.shape), 0, 255).astype(np.uint8)
    return cv2.imdecode(cv2.imencode('.jpg', img, [cv2.IMWRITE_JPEG_QUALITY, 85])[1], cv2.IMREAD_COLOR)


ZIGZAG = [(100, 225), (250, 100), (400, 225), (550, 100), (700, 225)]


def target(canvas):
    """A crosshair: three 2 px rings crossed by two lines, which must stay whole through every crossing."""
    for r in (600, 1000, 1400):
        cv2.circle(canvas, (2048, 2048), r, (30, 30, 30), 8)
    cv2.line(canvas, (400, 2048), (3700, 2048), (30, 30, 30), 8)
    cv2.line(canvas, (2048, 400), (2048, 3700), (30, 30, 30), 8)


def run(image, *flags):
    with tempfile.TemporaryDirectory() as tmp:
        source = Path(tmp) / 'crest.png'
        cv2.imwrite(str(source), image)
        report = m.main([str(source), '--preview', 'none', *flags])[0]
        svg = Path(report['svg']).read_text()
        assert json.loads(Path(report['report']).read_text())['shapes'] == report['shapes']
    return report, svg


def paths(svg):
    return {i: m.parse_path(d) for i, d in re.findall(r'<path id="([^"]+)"[^>]* d="([^"]+)"', svg)}


def by_id(report):
    return {sh['id']: sh for sh in report['shapes']}


def test_crest_traces_to_named_cutout_shapes():
    report, svg = run(badge(), '--names', 'ring,star,tab', '--split')
    shapes = by_id(report)
    assert list(shapes) == ['ring', 'star', 'tab', 'blue_2'], list(shapes)
    assert (shapes['ring']['outlines'], shapes['ring']['holes']) == (1, 1)
    assert (shapes['star']['outlines'], shapes['star']['holes']) == (1, 1)
    assert report['fidelity'] > 0.96 and min(sh['iou'] for sh in report['shapes']) > 0.95, report
    # the ring is the annulus between radius 375 and 275 source px
    assert abs(shapes['ring']['area'] / (math.pi * (375 ** 2 - 275 ** 2)) - 1) < 0.02, shapes['ring']['area']
    # few points: a circle is a handful of curves, a tab four lines, the star ten
    assert shapes['ring']['segments'] <= 10 and shapes['tab']['segments'] <= 6, shapes
    assert 10 <= shapes['star']['segments'] <= 18, shapes['star']
    assert report['segments'] <= 40, report['segments']
    # an author's file: plain closed paths, nothing an importer has to resolve
    assert not re.search(r'transform=|<g\b|stroke|gradient|<use\b', svg)
    assert all(d.rstrip().endswith('Z') for d in re.findall(r' d="([^"]+)"', svg))
    assert report['background'] and report['background'][0].startswith('#')


def test_holes_wind_against_their_outline():
    _, svg = run(badge())
    for name, subpaths in paths(svg).items():
        signs = sorted(np.sign(m.signed_area(m.flatten(sp))) for sp in subpaths)
        if name in ('red', 'orange'):
            assert signs == [-1, 1], (name, signs)


def test_fake_transparency_checkerboard_is_background():
    report, _ = run(badge('checker'))
    assert len(report['background']) == 2, report['background']
    assert any('checkerboard' in w for w in report['warnings'])
    assert {'red', 'orange', 'blue'} <= set(by_id(report)), list(by_id(report))
    assert report['fidelity'] > 0.96


def test_real_alpha_is_background():
    report, _ = run(badge('alpha'))
    assert report['background'] == ['transparent']
    assert [sh['id'] for sh in report['shapes']][:3] == ['red', 'orange', 'blue']


def test_silhouette_and_outer_outline():
    report, _ = run(badge(), '--mode', 'silhouette')
    assert [sh['id'] for sh in report['shapes']] == ['silhouette']
    assert report['shapes'][0]['holes'] == 2      # the gap round the star, the star's own hole
    report, _ = run(badge(), '--mode', 'silhouette', '--fill-holes', '--split')
    assert [(sh['outlines'], sh['holes']) for sh in report['shapes']] == [(1, 0)] * 3


def test_drop_fit_and_the_polygon_tracer():
    report, svg = run(badge(), '--drop', 'blue', '--fit', '100', '--tracer', 'opencv')
    assert [sh['id'] for sh in report['shapes']] == ['red', 'orange'] and report['dropped'] == ['blue']
    assert max(report['width'], report['height']) == 100
    assert ' C' not in svg and report['fidelity'] > 0.97


def test_tolerance_trades_points_for_accuracy():
    tight, _ = run(badge(), '--tolerance', '0.5')
    loose, _ = run(badge(), '--tolerance', '8')
    assert loose['segments'] < tight['segments'], (loose['segments'], tight['segments'])
    assert tight['fidelity'] > loose['fidelity'] > 0.93, (tight['fidelity'], loose['fidelity'])


def test_lines_keep_rings_closed_and_lines_whole_through_crossings():
    report, svg = run(sketch(target), '--mode', 'lines')
    (ink,) = report['shapes']
    assert (ink['strokes'], ink['closed']) == (5, 3), ink
    assert 1.5 <= ink['width'] <= 2.5 and ink['match'] > 0.95, ink
    assert report['segments'] <= 14, report['segments']
    assert 'fill="none"' in svg and 'stroke-width=' in svg
    (d,) = re.findall(r' d="([^"]+)"', svg)
    assert d.count('Z') == 3  # the rings close, the two lines stay open


def test_lines_open_polyline_is_its_corners():
    def zigzag(canvas):
        cv2.polylines(canvas, [np.int32(ZIGZAG) * 4], False, (40, 40, 40), 16)
    report, svg = run(sketch(zigzag), '--mode', 'lines', '--no-crop')
    (ink,) = report['shapes']
    assert (ink['strokes'], ink['closed'], ink['segments']) == (1, 0, 4), ink
    ((start, segs),) = m.parse_path(re.search(r' d="([^"]+)"', svg)[1])  # parse_path adds a closing line last
    corners = [start] + [s[-1] for s in segs[:4]]
    if corners[0][0] > corners[-1][0]:
        corners = corners[::-1]
    for got, want in zip(corners, ZIGZAG):
        assert math.dist(got, want) < 3.5, (got, want)
    assert all(s[0] == 'L' for s in segs[:4])


def test_lines_skip_filled_areas():
    def house(canvas):
        cv2.rectangle(canvas, (900, 1900), (3200, 3400), (90, 170, 240), -1)
        cv2.rectangle(canvas, (900, 1900), (3200, 3400), (40, 34, 34), 36)
        cv2.rectangle(canvas, (1750, 2500), (2350, 3400), (60, 80, 180), -1)
        cv2.rectangle(canvas, (1750, 2500), (2350, 3400), (40, 34, 34), 36)
    report, _ = run(sketch(house), '--mode', 'lines')
    assert [sh['id'] for sh in report['shapes']] == ['black'], report['shapes']
    assert report['shapes'][0]['match'] > 0.95
    assert any('skipped filled areas' in w for w in report['warnings']), report['warnings']


def test_lines_measure_a_width_profile_and_expand_to_a_leaf():
    def leaf_and_rule(canvas):
        t = np.linspace(0, 1, 800)
        for x, y, w in zip(500 + 3000 * t, 1400 - 500 * t + 500 * np.sin(np.pi * t), 4 + 64 * np.sin(np.pi * t) ** 0.8):
            cv2.circle(canvas, (int(x), int(y)), max(1, int(w / 2)), (50, 110, 40), -1, cv2.LINE_AA)
        cv2.line(canvas, (500, 3300), (3500, 3300), (50, 110, 40), 24)
    report, svg = run(sketch(leaf_and_rule), '--mode', 'lines')
    (ink,) = report['shapes']
    leaf, rule = sorted(ink['profiles'], key=len, reverse=True)
    assert len(rule) == 2 and all(abs(w - 6) < 0.8 for _, w in rule), rule        # uniform: two knots
    widest = max(w for _, w in leaf)
    assert abs(widest - 17) < 1.5 and leaf[0][1] < 4 and leaf[-1][1] < 4, leaf   # swells, tapers both ends
    assert 4 <= len(leaf) <= 12, leaf
    assert 'data-width-profile=' in svg
    expanded, svg = run(sketch(leaf_and_rule), '--mode', 'lines', '--expand')
    (shape,) = expanded['shapes']
    assert shape['expanded'] and shape['outlines'] == 2 and shape['segments'] <= 16, shape
    assert 'fill="none"' not in svg and 'fill-rule="nonzero"' in svg


def test_parse_reverse_and_area():
    (square,) = m.parse_path('m10 10 h20 v20 H10 z')
    assert m.signed_area(m.flatten(square)) == 400
    assert m.signed_area(m.flatten(m.reverse(square))) == -400
    (curve,) = m.parse_path('M0 0 Q10 10 20 0 L20 -5 Z')
    assert curve[1][0][0] == 'C' and curve[1][-1] == ('L', (0.0, 0.0))
    two = m.parse_path('M0 0 L4 0 L4 4 L0 4 Z M1 1 L3 1 L3 3 L1 3 Z')
    nested = m.organise(two, 0)
    assert [e[1] for e in nested] == [0, 1]
    assert [np.sign(m.signed_area(m.flatten(e[0]))) for e in nested] == [1, -1]


if __name__ == '__main__':
    for name, test in list(globals().items()):
        if name.startswith('test_'):
            test()
            print('pass', name)
