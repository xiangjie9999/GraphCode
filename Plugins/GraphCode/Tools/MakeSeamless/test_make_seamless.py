"""make_seamless against synthetic texture sets: photo-like non-periodic images (lighting drift,
noise, shapes), an exactly periodic one, registered map sets, normal maps, 16-bit with alpha,
TGA/EXR, regular structure and determinism. Run `python test_make_seamless.py` or pytest."""
import json
import math
import tempfile
import types
from pathlib import Path

import cv2
import numpy as np

import make_seamless as m


def photo(h=288, w=352, seed=0):
    """A non-tileable, photo-like colour image: a lighting gradient, grain and a few shapes."""
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    light = 0.45 + 0.35 * xx / w + 0.15 * yy / h
    img = np.dstack([light * 0.8, light * 0.9, light]) * 255
    img += cv2.GaussianBlur(rng.normal(0, 1, (h, w)).astype(np.float32), (0, 0), 1.5)[..., None] * 25
    for _ in range(14):
        centre = (int(rng.integers(0, w)), int(rng.integers(0, h)))
        colour = tuple(float(c) for c in rng.uniform(40, 220, 3))
        cv2.circle(img, centre, int(rng.integers(8, 30)), colour, -1, cv2.LINE_AA)
    return np.clip(img, 0, 255).astype(np.uint8)


def periodic(h=256, w=320, seed=1, band=0.04):
    """An exactly periodic texture: band-limited noise synthesised in the Fourier domain (band: the
    Gaussian width of its spectrum in cycles per pixel; larger is finer grain)."""
    rng = np.random.default_rng(seed)
    spectrum = np.fft.rfft2(rng.normal(0, 1, (h, w)))
    fy = np.fft.fftfreq(h)[:, None]
    fx = np.fft.rfftfreq(w)[None, :]
    spectrum *= np.exp(-(fx ** 2 + fy ** 2) / (2 * band ** 2))
    base = np.fft.irfft2(spectrum, s=(h, w))
    base = (base - base.mean()) / base.std()
    return np.clip(np.dstack([base * 30 + 120, base * 35 + 130, base * 40 + 140]), 0, 255).astype(np.uint8)


def normal_map(h=288, w=352, seed=2):
    """A 16-bit OpenGL normal map (B, G, R = z, y, x) of a non-periodic height field."""
    rng = np.random.default_rng(seed)
    height = cv2.GaussianBlur(rng.normal(0, 1, (h, w)).astype(np.float32), (0, 0), 4) * 40
    height += np.mgrid[0:h, 0:w][1].astype(np.float32) * 0.05
    gy, gx = np.gradient(height)
    n = np.dstack([np.ones_like(gx), -gy, -gx])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return np.round((n + 1) / 2 * 65535).astype(np.uint16)


def run(tmp, images, *flags, check=False):
    """Write images ({name: array}) into tmp, run the tool on them in order, return the report(s)."""
    paths = []
    for name, img in images.items():
        path = Path(tmp) / name
        if path.suffix == '.tga':
            m.write_tga(path, img)
        else:
            cv2.imwrite(str(path), img)
        paths.append(str(path))
    out = str(Path(tmp) / 'out')
    reports = m.main(paths + ['-o', out] + (['--check'] if check else []) + list(flags))
    return reports if check else reports[0]


def read(path):
    return cv2.imread(str(path), cv2.IMREAD_UNCHANGED)


def test_photo_becomes_seamless_and_keeps_size():
    with tempfile.TemporaryDirectory() as tmp:
        report = run(tmp, {'stone.png': photo()}, '--preview', 'none', '--physical-size', '200cm')
        entry = report['maps'][0]
        assert not entry['before']['seamless'], entry['before']
        assert entry['after']['seamless'] and report['ok'], entry['after']
        for axis in 'xy':
            assert entry['before'][f'wrap_{axis}'] > 2, entry['before']
            assert 0.6 < entry['after'][f'wrap_{axis}'] < 1.15, entry['after']
            # interior 16 px bands of an image with large shapes step this much too: judge by rank
            assert entry['after'][f'low_{axis}_rank'] < m.RANK_WARN, entry['after']
        out = read(entry['output'])
        assert out.shape == (288, 352, 3) and out.dtype == np.uint8
        # measured again from the file, independently of the report
        again = m.wrap_metrics(out.astype(np.float32) / 255)
        assert again['wrap_x'] < 1.15 and again['wrap_y'] < 1.15, again
        # the overlap crop magnifies the content; the physical size follows the cut
        cut_w, _ = report['cut_size']
        assert math.isclose(report['content_scale'][0], 352 / cut_w, rel_tol=1e-3)
        assert math.isclose(report['physical_size']['output_tile'][0], 200 * cut_w / 352, rel_tol=1e-3)
        assert report['seams'][0]['visibility'] < m.SEAM_WARN, report['seams']


def test_periodic_image_checks_as_seamless_and_stays_put():
    with tempfile.TemporaryDirectory() as tmp:
        img = periodic()
        (checked,) = run(tmp, {'tile.png': img}, '--preview', 'none', check=True)
        assert checked['ok'] and checked['metrics']['seamless'], checked['metrics']
        for axis in 'xy':   # the wrap is one more ordinary pixel boundary
            assert 0.6 < checked['metrics'][f'wrap_{axis}'] < 1.15, checked['metrics']
            assert checked['metrics'][f'wrap_{axis}_rank'] < 0.9, checked['metrics']
        assert checked['structure']['class'] == 'stochastic', checked['structure']
        assert not list((Path(tmp) / 'out').glob('*.seamless.*')), 'check mode must not write an image'
        # Moisan on something already periodic removes (almost) nothing
        report = run(tmp, {'tile2.png': img}, '--method', 'periodic', '--flatten', 'off', '--preview', 'none')
        out = read(report['maps'][0]['output']).astype(np.float32)
        assert np.abs(out - img).mean() < 1.0, np.abs(out - img).mean()


def test_non_tileable_input_fails_check():
    with tempfile.TemporaryDirectory() as tmp:
        (checked,) = run(tmp, {'stone.png': photo()}, '--preview', 'none', check=True)
        assert not checked['ok']
        assert any('seam' in w for w in checked['warnings']), checked['warnings']


def test_texture_set_gets_identical_cuts():
    with tempfile.TemporaryDirectory() as tmp:
        img = photo(seed=3)
        # flatten is a per-map tone edit (colour maps only), so the linear relation is checked without it
        report = run(tmp, {'wall_BC.png': img, 'wall_copy.png': img.copy(), 'wall_mask.png': 255 - img},
                     '--preview', 'none', '--flatten', 'off')
        a, b, c = (read(e['output']).astype(np.int32) for e in report['maps'])
        assert np.array_equal(a, b), 'a copy of the guide must come out identical'
        # an inverted map is cut, blended, levelled and resampled the same way: it stays the inverse
        assert np.percentile(np.abs(a + c - 255), 99.9) <= 1, np.abs(a + c - 255).max()
        assert report['seam_from'] == [e['input'] for e in report['maps']]


def test_maps_must_share_a_size():
    with tempfile.TemporaryDirectory() as tmp:
        try:
            run(tmp, {'a_BC.png': photo(), 'a_R.png': photo(h=200, w=200)[..., 0]}, '--preview', 'none')
        except SystemExit as e:
            assert '352x288' in str(e) and '200x200' in str(e), str(e)
        else:
            raise AssertionError('maps of different sizes were accepted')


def test_normal_map_stays_unit_length():
    with tempfile.TemporaryDirectory() as tmp:
        report = run(tmp, {'rock_BC.png': photo(seed=4), 'rock_NormalGL.png': normal_map()}, '--preview', 'none')
        entry = report['maps'][1]
        assert entry['kind'] == 'normal', entry['kind']
        out = read(entry['output'])
        assert out.dtype == np.uint16
        length = np.linalg.norm(out.astype(np.float64) / 65535 * 2 - 1, axis=2)
        assert np.abs(length - 1).max() < 2e-3, np.abs(length - 1).max()
        assert entry['after']['seamless'], entry['after']


def test_16_bit_and_alpha_round_trip():
    with tempfile.TemporaryDirectory() as tmp:
        colour = photo(seed=5).astype(np.uint16) * 257
        alpha = np.full(colour.shape[:2] + (1,), 40000, np.uint16)
        report = run(tmp, {'decal.png': np.concatenate([colour, alpha], axis=2)}, '--preview', 'none')
        out = read(report['maps'][0]['output'])
        assert out.dtype == np.uint16 and out.shape == (288, 352, 4), (out.dtype, out.shape)
        assert np.abs(out[..., 3].astype(np.int32) - 40000).max() <= 1, 'alpha is data: only cut and resampled'
        assert len(np.unique(out[..., :3])) > 1000, 'kept 16-bit precision'


def test_tga_and_exr():
    with tempfile.TemporaryDirectory() as tmp:
        img = photo(seed=6)
        report = run(tmp, {'ground.tga': img}, '--preview', 'none')
        out = m.read_tga(Path(report['maps'][0]['output']))
        assert out.shape == img.shape and report['maps'][0]['after']['seamless']
        # an RLE packet header: one run of 3 identical pixels, then 2 raw ones
        rle = (bytes([0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 1, 0, 24, 0x20])
               + bytes([0x82, 1, 2, 3]) + bytes([0x01, 4, 5, 6, 7, 8, 9]))
        (Path(tmp) / 'rle.tga').write_bytes(rle)
        assert m.read_tga(Path(tmp) / 'rle.tga').reshape(-1, 3).tolist() == [[1, 2, 3]] * 3 + [[4, 5, 6], [7, 8, 9]]
        if cv2.haveImageWriter('x.exr'):
            hdr = (photo(seed=7).astype(np.float32) / 255) ** 2.2 * 4
            report = run(tmp, {'sky.exr': hdr}, '--preview', 'none')
            out = read(report['maps'][0]['output'])
            assert out.dtype == np.float32 and out.max() > 1.5, 'float and HDR range kept'
            assert report['maps'][0]['after']['seamless']


def test_regular_structure_snaps_in_phase():
    with tempfile.TemporaryDirectory() as tmp:
        h, w = 256, 384
        yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
        stripes = 0.5 + 0.35 * np.sin(2 * np.pi * (0.8 * xx + yy) / 37)
        img = np.clip(np.dstack([stripes * 200, stripes * 170, stripes * 120]), 0, 255).astype(np.uint8)
        report = run(tmp, {'stripes.png': img}, '--preview', 'none')
        assert report['structure']['class'] == 'regular', report['structure']
        assert not report['structure']['random_rotation_safe']
        assert all('in phase' in s['overlap_choice'] for s in report['seams']), report['seams']
        assert all(s['visibility'] < 0.5 for s in report['seams']), report['seams']


def test_height_map_and_graph_cut():
    with tempfile.TemporaryDirectory() as tmp:
        img = photo(seed=8)
        height = cv2.GaussianBlur(img[..., 1], (0, 0), 3)
        flags = ['--preview', 'none'] + (['--seam', 'graphcut'] if m.importlib.util.find_spec('maxflow') else [])
        report = run(tmp, {'stones_BC.png': img, 'stones_H.png': height}, *flags)
        assert report['height_map'].endswith('stones_H.png') and report['maps'][1]['kind'] == 'data'
        assert report['ok'], [e['after'] for e in report['maps']]
        if '--seam' in flags:
            assert report['seam'] == 'graphcut' and all(s['seam'] == 'graphcut' for s in report['seams'])


def test_graph_cut_falls_back_without_pymaxflow():
    real = m.importlib.util.find_spec
    m.importlib.util.find_spec = lambda name, *rest: None if name == 'maxflow' else real(name, *rest)
    try:
        with tempfile.TemporaryDirectory() as tmp:
            report = run(tmp, {'stone.png': photo(seed=11)}, '--seam', 'graphcut', '--preview', 'none')
    finally:
        m.importlib.util.find_spec = real
    assert report['seam'] == 'dp' and any('PyMaxflow' in w for w in report['warnings']), report['warnings']
    assert report['ok']


def test_offset_method_keeps_size_and_warns():
    with tempfile.TemporaryDirectory() as tmp:
        report = run(tmp, {'stone.png': photo(seed=9)}, '--method', 'offset', '--preview', 'none')
        assert report['output_size'] == report['input_size'] == [352, 288]
        assert report['ok'] and any('appears twice' in w for w in report['warnings'])


def test_prep_steps_are_reported():
    with tempfile.TemporaryDirectory() as tmp:
        h, w = 288, 352
        yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
        lit = 1.25 - 0.6 * (((xx - w / 2) / w) ** 2 + ((yy - h / 2) / h) ** 2) * 2      # a lit centre
        img = np.clip(periodic(h, w).astype(np.float32) * lit[..., None], 0, 255).astype(np.uint8)
        report = run(tmp, {'soil_BC.png': img, 'soil_R.png': img[..., 1]}, '--preview', 'none')
        steps = {s['step']: s for s in report['prep']}
        assert list(steps) == ['flatten', 'equalize'], list(steps)
        assert list(steps['flatten']['maps']) == ['soil_BC.png'], 'only colour maps are de-lit by default'
        flat = steps['flatten']['maps']['soil_BC.png']
        assert flat['macro_after'] < 0.8 * flat['macro_before'], flat
        for name, jump in steps['equalize']['maps'].items():
            assert jump['edge_jump_after'] <= jump['edge_jump_before'], (name, jump)
        off = run(tmp, {'soil_BC.png': img}, '--preview', 'none', '--flatten', 'off')
        assert [s['step'] for s in off['prep']] == ['equalize']


def test_runtime_hand_off():
    with tempfile.TemporaryDirectory() as tmp:
        (iso,) = run(tmp, {'noise.png': periodic()}, '--preview', 'none', check=True)
        assert iso['structure']['texture_type'] == 'isotropic' and iso['structure']['stochastic_tiling_safe']
        h, w = 256, 320
        yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
        grain = 0.5 + 0.3 * np.sin(2 * np.pi * (yy + 12 * np.sin(2 * np.pi * xx / w)) / 23)
        grain += np.random.default_rng(3).normal(0, 0.04, (h, w))
        img = np.clip(np.dstack([grain * 120, grain * 160, grain * 210]), 0, 255).astype(np.uint8)
        (wood,) = run(tmp, {'plank.png': img}, '--preview', 'none', check=True)
        s = wood['structure']
        assert s['texture_type'] == 'structured' and not s['stochastic_tiling_safe'], s
        assert any('TEXTURE-VARIATION.md' in step for step in s['next_steps'])
        assert any('stochastic tiling' in w for w in wood['warnings']), wood['warnings']


def test_field_preview_and_grid_read():
    with tempfile.TemporaryDirectory() as tmp:
        blob = periodic(256, 256, band=0.25).astype(np.float32)
        cv2.circle(blob, (128, 128), 40, (250, 250, 250), -1)            # one bright landmark per tile
        report = run(tmp, {'blob.png': blob.astype(np.uint8)})
        field = read(report['verify']['field'])
        assert field.shape[1] == 2000 and field.shape[0] > 2000, field.shape
        plain = run(tmp, {'plain.png': periodic(256, 256, band=0.25)})
        assert report['verify']['grid_read']['mip16'] > GRID_HINT > plain['verify']['grid_read']['mip16']
        assert any('reads as a grid' in w for w in report['warnings']), report['warnings']


GRID_HINT = m.GRID_WARN


def blob_tile(seed=4):
    """A fine-grained periodic texture with one bright landmark."""
    img = periodic(256, 256, seed=seed, band=0.25).astype(np.float32)
    cv2.circle(img, (90, 150), 22, (245, 245, 245), -1, cv2.LINE_AA)
    return img.astype(np.uint8)


def test_auto_remove_replaces_the_landmark_in_every_map():
    with tempfile.TemporaryDirectory() as tmp:
        img = blob_tile()
        plain = run(tmp, {'blob_BC.png': img, 'blob_copy.png': img.copy()}, '--flatten', 'off')
        before = read(plain['maps'][0]['output']).astype(np.float32)          # read before the next run
        removed = run(tmp, {'blob_BC.png': img, 'blob_copy.png': img.copy()}, '--flatten', 'off', '--auto-remove', '1')
        regions = removed['removal']['regions']
        assert len(regions) == 1 and regions[0]['source'] == 'auto', regions
        a, b = (read(e['output']) for e in removed['maps'])
        assert np.array_equal(a, b), 'the same donor and weights go to every map'
        after = a.astype(np.float32)
        glow_before, glow_after = (cv2.GaussianBlur(x.mean(axis=2), (0, 0), 8) for x in (before, after))
        y, x = np.unravel_index(np.argmax(glow_before), glow_before.shape)      # where the blob ended up
        assert glow_after[y, x] < glow_before[y, x] - 30, (glow_before[y, x], glow_after[y, x])
        assert removed['landmarks'][0]['score'] < plain['landmarks'][0]['score']
        assert Path(removed['landmarks_overlay']).is_file() and 'box' in removed['landmarks'][0]
        assert removed['ok'], 'removal keeps the tile seamless'


def test_remove_mask_follows_the_cut_into_the_tile():
    with tempfile.TemporaryDirectory() as tmp:
        img = periodic(256, 256, seed=6, band=0.25)
        mask = np.zeros((256, 256), np.uint8)
        cv2.circle(mask, (128, 128), 20, 255, -1)
        mask_path = Path(tmp) / 'mask.png'
        cv2.imwrite(str(mask_path), mask)
        plain = run(tmp, {'tile.png': img}, '--flatten', 'off', '--preview', 'none')
        a = read(plain['maps'][0]['output']).astype(np.int32)                  # read before the next run
        masked = run(tmp, {'tile.png': img}, '--flatten', 'off', '--preview', 'none', '--remove-mask', str(mask_path))
        assert [r['source'] for r in masked['removal']['regions']] == ['mask'], masked['removal']
        b = read(masked['maps'][0]['output']).astype(np.int32)
        cx, cy = masked['removal']['regions'][0]['centre']
        assert abs(cx - 128) < 24 and abs(cy - 128) < 24, (cx, cy)
        changed = np.abs(a - b).max(axis=2) > 0
        assert changed[cy - 10:cy + 10, cx - 10:cx + 10].all(), 'the masked area is replaced'
        far = np.ones_like(changed)
        far[max(0, cy - 90):cy + 90, max(0, cx - 90):cx + 90] = False
        assert not changed[far].any(), 'nothing far from the mask changes'


def test_variants_share_their_border():
    with tempfile.TemporaryDirectory() as tmp:
        img = photo(seed=12)
        report = run(tmp, {'gravel_BC.png': img, 'gravel_copy.png': img.copy()}, '--variants', '3')
        v = report['variants']
        assert v['count'] == 3 and len(v['outputs']['gravel_BC.png']) == 2, v
        base = read(report['maps'][0]['output']).astype(np.int32)
        frame = v['border_px']
        border = np.zeros(base.shape[:2], bool)
        border[:frame] = border[-frame:] = True
        border[:, :frame] = border[:, -frame:] = True
        for path, copy in zip(v['outputs']['gravel_BC.png'], v['outputs']['gravel_copy.png']):
            tile = read(path).astype(np.int32)
            assert np.array_equal(tile, read(copy).astype(np.int32)), 'identical cuts in every map'
            assert np.array_equal(tile[border], base[border]), 'the border strip is shared exactly'
            assert np.abs(tile[~border] - base[~border]).mean() > 10, 'the interior differs'
            metrics = m.wrap_metrics(tile.astype(np.float32) / 255)
            assert metrics['wrap_x'] < 1.15 and metrics['wrap_y'] < 1.15, metrics
        assert Path(v['verify']['field']).is_file()
        assert v['verify']['grid_read']['mip16'] <= report['verify']['grid_read']['mip16'] + 1e-3


def test_brick_and_half_drop_layouts():
    with tempfile.TemporaryDirectory() as tmp:
        img = photo(seed=13)
        for layout, axis in (('brick', 0), ('half-drop', 1)):
            report = run(tmp, {f'{layout}.png': img}, '--layout', layout, '--preview', 'none')
            lay = report['layout']
            assert lay['name'] == layout and lay['offset'] == 0.5 and lay['super_tiles'] == 2, lay
            tile = read(report['maps'][0]['output']).astype(np.float32)
            if axis:                                    # half-drop: look at it as brick on its side
                tile = tile.transpose(1, 0, 2)
            s = lay['shift_px']
            assert s == tile.shape[1] // 2, (s, tile.shape)
            inner = np.abs(np.diff(tile, axis=0)).mean()
            joined = np.abs(tile[0] - np.roll(tile[-1], -s, axis=0)).mean() / inner      # the edges brick joins
            plain = np.abs(tile[0] - tile[-1]).mean() / inner                              # a plain grid's edges
            assert joined < 1.2 < plain, (layout, joined, plain)
            big = read(report['maps'][0]['super_output'])
            assert big.shape[axis] == 2 * read(report['maps'][0]['output']).shape[axis]
            assert report['maps'][0]['after']['seamless'], 'the super-tile repeats on plain UVs'
        try:
            run(tmp, {'odd.png': img}, '--layout', 'brick', '--offset', '0.3', '--preview', 'none')
        except SystemExit as e:
            assert 'denominator' in str(e)
        else:
            raise AssertionError('an offset that never repeats was accepted')


def normal_from_height(height, green_down):
    """Encoded 16-bit normal map (B, G, R = z, y, x) of a height field: DirectX stores the normal's
    component down the image (-dh/drow), OpenGL its component up it (+dh/drow)."""
    gy, gx = np.gradient(height)
    n = np.dstack([np.ones_like(gx), -gy if green_down else gy, -gx])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return np.round((n + 1) / 2 * 65535).astype(np.uint16)


def test_turn_tile_turns_normal_xy_with_the_image():
    rng = np.random.default_rng(14)
    height = cv2.GaussianBlur(rng.normal(0, 1, (96, 96)).astype(np.float32), (0, 0), 5) * 60
    for green_down in (True, False):
        kind = types.SimpleNamespace(kind='normal', green_down=green_down)
        decode = m.Map.decode.__get__(types.SimpleNamespace(kind='normal', signed=False))
        before = decode(normal_from_height(height, green_down).astype(np.float32) / 65535)
        for k in range(1, 4):
            truth = decode(normal_from_height(np.rot90(height, k).copy(), green_down).astype(np.float32) / 65535)
            turned = m.turn_tile(before, k, kind)
            err = np.abs(turned - truth)[2:-2, 2:-2].max()          # np.gradient is one-sided at the border
            assert err < 1e-3, (green_down, k, err)


def test_rotate_layout_joins_under_every_turn():
    with tempfile.TemporaryDirectory() as tmp:
        img = photo(256, 256, seed=15)
        report = run(tmp, {'stone_BC.png': img, 'stone_copy.png': img.copy(),
                           'stone_NormalGL.png': normal_map(256, 256, seed=15)}, '--layout', 'rotate')
        lay = report['layout']
        assert lay['name'] == 'rotate' and lay['super_tiles'] == 1
        assert 'normal' in lay['runtime'] and 'quarter turn' in lay['runtime']
        band = lay['band_px']
        assert lay['corner_px'] == 2 * band and lay['join_px'] > 0, lay
        tile = read(report['maps'][0]['output'])
        for k in range(4):           # every side, turned to the top, carries the same band
            assert np.array_equal(np.rot90(tile, k)[:band], tile[:band]), k
        assert np.array_equal(tile, read(report['maps'][1]['output'])), 'identical cuts in every map'
        nm = m.Map(Path(report['maps'][2]['output']), 'normal')
        work = nm.decode(nm.values)
        for k in range(4):           # and the normal map's band too, its vectors turned with it
            assert np.abs(m.turn_tile(work, k, nm)[:band] - work[:band]).max() < 1e-3, k
        for e in report['maps']:
            t = e['turns']
            assert t['pairs'] == 16 and t['seamless'] and t['worst_wrap'] < 1.3, (e['input'], t)
        assert report['ok'] and 'super_output' not in report['maps'][0]
        assert Path(report['verify']['field']).is_file() and Path(lay['normal_field']).is_file()


def test_rotate_variants_share_the_band():
    with tempfile.TemporaryDirectory() as tmp:
        report = run(tmp, {'gravel_BC.png': photo(256, 256, seed=16)}, '--layout', 'rotate', '--variants', '2',
                     '--preview', 'none')
        band = report['layout']['band_px']
        assert report['variants']['shared_band_px'] == band
        base = read(report['maps'][0]['output']).astype(np.int32)
        other = read(report['variants']['outputs']['gravel_BC.png'][0]).astype(np.int32)
        for k in range(4):
            assert np.array_equal(np.rot90(other, k)[:band], base[:band]), k
        assert np.abs(other[64:-64, 64:-64] - base[64:-64, 64:-64]).mean() > 10, 'the interior differs'
        assert report['ok']


def test_turn_metrics_catch_a_tile_that_only_tiles_on_a_grid():
    img = periodic(256, 256, band=0.08).astype(np.float32) / 255
    kind = m.Map.__new__(m.Map)
    kind.kind, kind.signed, kind.green_down = 'color', False, True
    grid, turns = m.wrap_metrics(img), m.turn_metrics(img, kind)
    assert not grid['seam_x'] and not grid['seam_y'], grid
    assert not turns['seamless'] and turns['worst_wrap'] > 1.5 and turns['seams'], turns
    assert [0, 0] not in turns['seams'], 'the un-turned pairing is the grid wrap, which is seamless'


def test_rotate_refuses_structured_textures_and_oblong_tiles():
    with tempfile.TemporaryDirectory() as tmp:
        yy, xx = np.mgrid[0:256, 0:256].astype(np.float32)
        grain = 0.5 + 0.3 * np.sin(2 * np.pi * (yy + 12 * np.sin(2 * np.pi * xx / 256)) / 23)
        grain += np.random.default_rng(3).normal(0, 0.04, (256, 256))
        img = np.clip(np.dstack([grain * 120, grain * 160, grain * 210]), 0, 255).astype(np.uint8)
        try:
            run(tmp, {'plank.png': img}, '--layout', 'rotate', '--preview', 'none')
        except SystemExit as e:
            assert 'refused' in str(e) and '--allow-structured' in str(e), e
        else:
            raise AssertionError('grain was turned without --allow-structured')
        forced = run(tmp, {'plank.png': img}, '--layout', 'rotate', '--preview', 'none', '--allow-structured')
        assert any('STRUCTURED' in w for w in forced['warnings']), forced['warnings']
        try:
            run(tmp, {'oblong.png': photo(seed=17)}, '--layout', 'rotate', '--preview', 'none')
        except SystemExit as e:
            assert 'square' in str(e), e
        else:
            raise AssertionError('an oblong tile was turned')


def test_resampler_is_periodic():
    h, w = 24, 37
    x = np.arange(w)
    row = (np.sin(2 * np.pi * 3 * x / w) + 0.3 * np.cos(2 * np.pi * 5 * x / w)).astype(np.float32)
    out = m.resample_periodic(np.tile(row, (h, 1))[..., None], h, 50)[0, :, 0]
    t = (np.arange(50) + 0.5) * w / 50 - 0.5
    truth = np.sin(2 * np.pi * 3 * t / w) + 0.3 * np.cos(2 * np.pi * 5 * t / w)
    err = np.abs(out - truth)
    # the samples next to the wrap are as good as any inside (a clamped border would be far worse)
    assert err[[0, 1, -2, -1]].max() <= err[2:-2].max() + 1e-4, (err[[0, 1, -2, -1]], err[2:-2].max())
    assert err.max() < 0.02, err.max()


def test_same_input_same_bytes():
    with tempfile.TemporaryDirectory() as tmp:
        images = {'rock_BC.png': photo(seed=10), 'rock_NormalGL.png': normal_map(seed=10)}
        first = run(tmp, images)
        files = [Path(e['output']) for e in first['maps']] + [Path(first['preview']), Path(first['report'])]
        before = [f.read_bytes() for f in files]
        second = run(tmp, images)
        assert [f.read_bytes() for f in files] == before
        assert json.loads(Path(second['report']).read_text())['maps'][0]['after'] == first['maps'][0]['after']


if __name__ == '__main__':
    for name, test in list(globals().items()):
        if name.startswith('test_'):
            test()
            print('pass', name)
