"""Bounds-level clearance checks: keep-clear volumes, capsule openings, hidden faces, mounts.

Pure Python, centimetres, Unreal world axes: X forward, Y right, Z up, and yaw in
degrees from +X toward +Y. A part is dict(id=..., low=[x,y,z], high=[x,y,z]): its
world axis-aligned bounds, optionally with label, asset and selected. No Unreal or
project dependency; gc_check_assembly gathers the parts in the editor.

Every check here reads bounds, never triangles. A turned part's bounds are larger
than the part, and a part with a hole (a wall around a window) reads as solid. A
finding is a place to look, not a measured contact; scan() measures the meshes.
"""
# SPDX-License-Identifier: MIT
import fnmatch
import math

from gc_coplanar import bounds_gap

EPS = 1e-9
AXES = 'XYZ'


# -- validation ---------------------------------------------------------------

def _real(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def _finite(value, name, positive=False):
    if not _real(value) or value < 0 or (positive and value == 0):
        raise ValueError(name + ' must be finite and ' + ('positive.' if positive else 'nonnegative.'))
    return float(value)


def _point(value, name):
    if value is None or len(value) != 3 or not all(_real(v) for v in value):
        raise ValueError(name + ' must be three finite numbers in cm.')
    return [float(v) for v in value]


def _box(item, name):
    low, high = _point(item.get('low'), name + ' low'), _point(item.get('high'), name + ' high')
    if any(l > h for l, h in zip(low, high)):
        raise ValueError(name + ' has a low corner above its high corner.')
    return low, high


def _parts(parts):
    out, seen = [], set()
    for part in parts:
        if not part.get('id'):
            raise ValueError('Every part needs an id.')
        if part['id'] in seen:
            raise ValueError('Duplicate part id: ' + str(part['id']))
        seen.add(part['id'])
        low, high = _box(part, 'Part ' + str(part['id']))
        out.append(dict(part, low=low, high=high))
    return out


def _names(items, what):
    names = [item.get('name') for item in items]
    if not all(names) or len(set(names)) != len(names):
        raise ValueError('Every ' + what + ' needs a unique name.')
    return set(names)


def _allowances(allowed, keys, known, limit=None):
    """Exact (key...) -> allowance. Each needs captured names and a reason, as in scan()."""
    table = {}
    for item in allowed:
        if not item.get('reason') or any(item.get(k) not in known[k] for k in keys):
            raise ValueError('An allowance needs captured ' + ' and '.join(keys) + ' and a reason: ' + str(item))
        if limit:
            _finite(item.get(limit), limit)
        table[tuple(item[k] for k in keys)] = item
    return table


def axis_of(direction, name='direction'):
    """(axis index, sign) of a world-axis direction. Bounds are axis-aligned, so the
    direction must be too (within about 0.8 degrees)."""
    d = _point(direction, name)
    length = math.sqrt(sum(v * v for v in d))
    if length <= EPS:
        raise ValueError(name + ' must not be zero.')
    i = max(range(3), key=lambda k: abs(d[k]))
    if abs(d[i]) / length < 0.9999:
        raise ValueError(name + ' must lie along a world axis (bounds are axis-aligned): ' + str(direction))
    return i, (1 if d[i] > 0 else -1)


def axis_label(axis, sign):
    return ('+' if sign > 0 else '-') + AXES[axis]


# -- plan geometry (XY) -------------------------------------------------------

def _rect(low, high):
    return [(low[0], low[1]), (high[0], low[1]), (high[0], high[1]), (low[0], high[1])]


def _turned_rect(center, size, yaw_deg):
    c, s = math.cos(math.radians(yaw_deg)), math.sin(math.radians(yaw_deg))
    hx, hy = size[0] * .5, size[1] * .5
    return [(center[0] + x * c - y * s, center[1] + x * s + y * c)
            for x, y in ((-hx, -hy), (hx, -hy), (hx, hy), (-hx, hy))]


def _sat_depth(a, b):
    """Smallest overlap of two convex plan polygons over their edge normals; <= 0 is apart."""
    depth = math.inf
    for poly in (a, b):
        for p, q in zip(poly, poly[1:] + poly[:1]):
            n = (q[1] - p[1], p[0] - q[0])
            length = math.hypot(*n)
            if length <= EPS:
                continue
            n = (n[0] / length, n[1] / length)
            pa = [x * n[0] + y * n[1] for x, y in a]
            pb = [x * n[0] + y * n[1] for x, y in b]
            depth = min(depth, min(max(pa), max(pb)) - max(min(pa), min(pb)))
    return depth


def _clip(poly, side):
    """Keep the part of a convex polygon where the linear function side(p) >= 0."""
    out = []
    for p, q in zip(poly, poly[1:] + poly[:1]):
        sp, sq = side(p), side(q)
        if sp >= 0:
            out.append(p)
        if (sp >= 0) != (sq >= 0):
            t = sp / (sp - sq)
            out.append((p[0] + t * (q[0] - p[0]), p[1] + t * (q[1] - p[1])))
    return out


def _signed_area(poly):
    return .5 * sum(p[0] * q[1] - p[1] * q[0] for p, q in zip(poly, poly[1:] + poly[:1]))


def _closest_to_origin(poly):
    """Distance and nearest point of a convex polygon (or point/segment) to the origin."""
    area = _signed_area(poly) if len(poly) >= 3 else 0.
    if abs(area) > EPS:
        orient = 1 if area > 0 else -1
        if all(orient * ((q[0] - p[0]) * -p[1] - (q[1] - p[1]) * -p[0]) >= -1e-7
               for p, q in zip(poly, poly[1:] + poly[:1])):
            return 0., (0., 0.)
    best = (math.inf, None)
    for p, q in zip(poly, poly[1:] + poly[:1]):
        dx, dy = q[0] - p[0], q[1] - p[1]
        span = dx * dx + dy * dy
        t = 0. if span <= EPS else min(1., max(0., -(p[0] * dx + p[1] * dy) / span))
        point = (p[0] + t * dx, p[1] + t * dy)
        best = min(best, (math.hypot(*point), point))
    return best


def _circle_hits(p, q, radius):
    dx, dy = q[0] - p[0], q[1] - p[1]
    a = dx * dx + dy * dy
    if a <= EPS:
        return []
    b = 2 * (p[0] * dx + p[1] * dy)
    c = p[0] * p[0] + p[1] * p[1] - radius * radius
    disc = b * b - 4 * a * c
    if disc < 0:
        return []
    root = math.sqrt(disc)
    return [(p[0] + t * dx, p[1] + t * dy) for t in ((-b - root) / (2 * a), (-b + root) / (2 * a))
            if 0 <= t <= 1]


# -- 1. keep-clear volumes ----------------------------------------------------

def _shape(box, name):
    if 'low' in box:
        low, high = _box(box, name)
        return dict(plan=_rect(low, high), z=(low[2], high[2]), low=low, high=high)
    center = _point(box.get('center'), name + ' center')
    size = _point(box.get('size'), name + ' size')
    for v in size:
        _finite(v, name + ' size', True)
    yaw = box.get('yaw_deg', 0.)
    if not _real(yaw):
        raise ValueError(name + ' yaw_deg must be finite.')
    plan = _turned_rect(center, size, yaw)
    z = (center[2] - size[2] * .5, center[2] + size[2] * .5)
    return dict(plan=plan, z=z, low=[min(p[0] for p in plan), min(p[1] for p in plan), z[0]],
                high=[max(p[0] for p in plan), max(p[1] for p in plan), z[1]])


def _volume(volume):
    name = volume['name']
    if 'hinge' in volume:
        sweep_from, sweep_to = volume.get('from_deg'), volume.get('to_deg')
        if not _real(sweep_from) or not _real(sweep_to):
            raise ValueError('Door swing ' + name + ' needs finite from_deg and to_deg.')
        sweep = float(sweep_to) - float(sweep_from)
        if sweep == 0 or abs(sweep) > 360:
            raise ValueError('Door swing ' + name + ' must turn by more than 0 and at most 360 degrees.')
        return dict(volume, kind='door_swing', hinge=_point(volume['hinge'], name + ' hinge'),
                    leaf_cm=_finite(volume.get('leaf_cm'), name + ' leaf_cm', True),
                    height_cm=_finite(volume.get('height_cm'), name + ' height_cm', True),
                    from_deg=float(sweep_from), sweep_deg=sweep)
    boxes = volume['boxes'] if 'boxes' in volume else [volume]
    if not boxes:
        raise ValueError('Volume ' + name + ' has an empty boxes list.')
    return dict(volume, kind='box', shapes=[_shape(b, name + ' box ' + str(i)) for i, b in enumerate(boxes)],
                listed='boxes' in volume)


def _door_entry(low, high, door):
    hx, hy, hz = door['hinge']
    radius = door['leaf_cm']
    vertical = min(high[2], hz + door['height_cm']) - max(low[2], hz)
    if vertical <= 0:
        return None
    # Work about the hinge so large world coordinates do not cancel.
    rect = [(x - hx, y - hy) for x, y in _rect(low, high)]
    sign = 1 if door['sweep_deg'] > 0 else -1
    total = abs(door['sweep_deg'])
    pieces = max(1, math.ceil(total / 180. - 1e-12))
    step = total / pieces
    nearest, angles = (math.inf, None), []
    for k in range(pieces):
        start, end = k * step, (k + 1) * step
        a = math.radians(door['from_deg'] + sign * start)
        b = math.radians(door['from_deg'] + sign * end)
        if sign < 0:
            a, b = b, a
        ua, ub = (math.cos(a), math.sin(a)), (math.cos(b), math.sin(b))
        poly = _clip(rect, lambda p: ua[0] * p[1] - ua[1] * p[0])
        poly = _clip(poly, lambda p: p[0] * ub[1] - p[1] * ub[0]) if poly else poly
        if not poly:
            continue
        distance, point = _closest_to_origin(poly)
        if distance >= radius:
            continue
        nearest = min(nearest, (distance, point))
        if distance == 0:
            angles += [start, end]
            continue
        candidates = [p for p in poly if math.hypot(*p) <= radius]
        for p, q in zip(poly, poly[1:] + poly[:1]):
            candidates += _circle_hits(p, q, radius)
        span = end - start
        for p in candidates:
            rel = ((math.degrees(math.atan2(p[1], p[0])) - door['from_deg']) * sign - start) % 360.
            if rel > span:  # floating spill just outside this wedge
                rel = 0. if rel > (360. + span) * .5 else span
            angles.append(start + rel)
    if nearest[1] is None:
        return None
    radial = radius - nearest[0]
    first, last = min(angles), max(angles)
    return dict(depth_cm=min(radial, vertical), radial_depth_cm=radial, vertical_overlap_cm=vertical,
                opened_deg=first,
                strike_yaw_deg=[door['from_deg'] + sign * first, door['from_deg'] + sign * last],
                nearest_world_cm=[hx + nearest[1][0], hy + nearest[1][1],
                                  max(low[2], hz) + vertical * .5])


def _box_entry(low, high, volume):
    best = None
    plan = _rect(low, high)
    for index, shape in enumerate(volume['shapes']):
        vertical = min(high[2], shape['z'][1]) - max(low[2], shape['z'][0])
        depth = min(_sat_depth(plan, shape['plan']), vertical)
        if depth <= 0 or (best and depth <= best['depth_cm']):
            continue
        best = dict(depth_cm=depth,
                    region_min_cm=[max(l, s) for l, s in zip(low, shape['low'])],
                    region_max_cm=[min(h, s) for h, s in zip(high, shape['high'])])
        if volume['listed']:
            best['box_index'] = index
    return best


def swept_volumes(parts, volumes, tolerance_cm=0.5, allowed=()):
    """Report each part whose bounds enter a volume the caller says must stay clear.

    volumes: [dict(name=..., reason=optional text, and ONE shape)]:
      door swing: hinge=[x,y,z] (the hinge axis at the floor), leaf_cm (hinge to the
        leaf's free edge), height_cm, from_deg, to_deg (world yaw of the leaf, closed
        to open; to_deg < from_deg swings clockwise). The swept plan is the pie slice
        the leaf covers. Leaf thickness is not added: widen leaf_cm or tolerance_cm.
      box: low=[...], high=[...], or center=[...], size=[x,y,z], yaw_deg (a turned box).
      boxes: [box, ...]: a stair as its flights or steps; the deepest entry is reported.
    allowed: [dict(a=part id, b=volume name, max_depth_cm=..., reason=...)]. A deeper
      entry still fails; the door leaf itself is the usual allowance.
    depth_cm is how far the part must move to clear the volume: for a box the smallest
    overlap over the box and part axes, for a door min(radial, vertical) where radial is
    how far inside the leaf's reach the part's nearest point lies. A door finding also
    gives opened_deg (degrees open when the leaf first meets it) and strike_yaw_deg.
    Bounds only: the part's axis-aligned bounds, not its triangles. Doors are not
    inferred from meshes; the caller states every volume. Returns findings in order of
    parts, then volumes.
    """
    tolerance = _finite(tolerance_cm, 'tolerance_cm')
    parts = _parts(parts)
    names = _names(volumes, 'volume')
    shapes = [_volume(v) for v in volumes]
    permits = _allowances(allowed, ('a', 'b'), dict(a={p['id'] for p in parts}, b=names), 'max_depth_cm')
    findings = []
    for part in parts:
        for volume in shapes:
            entry = (_door_entry if volume['kind'] == 'door_swing' else _box_entry)(
                part['low'], part['high'], volume)
            if entry is None or entry['depth_cm'] <= tolerance:
                continue
            finding = dict(status='inside_clear_volume', a=part['id'], b=volume['name'],
                           volume_kind=volume['kind'], **entry)
            if volume.get('reason'):
                finding['volume_reason'] = volume['reason']
            permit = permits.get((part['id'], volume['name']))
            if permit and entry['depth_cm'] <= permit['max_depth_cm']:
                finding['accepted_reason'] = permit['reason']
            findings.append(finding)
    return findings


# -- 2. capsule through an opening -------------------------------------------

def opening_frame(opening):
    """Normalise a stated opening to its centre, passage yaw, clear width and height.

    Rectangle: center=[x,y,z], normal=[x,y,z] (horizontal, the way through),
      width_cm, height_cm, optional depth_cm (wall thickness, default 0).
    Box: low=[...], high=[...], optional normal along X or Y; without one the passage
      runs along the thinner horizontal side (the wall's thickness).
    Optional: floor_z (the floor the character stands on before the opening) and
      required ('standing', the default, or 'crouched').
    """
    name = opening.get('name')
    if not name:
        raise ValueError('Every opening needs a name.')
    required = opening.get('required', 'standing')
    if required not in ('standing', 'crouched'):
        raise ValueError('Opening ' + name + ' required must be "standing" or "crouched".')
    floor_z = opening.get('floor_z')
    if floor_z is not None and not _real(floor_z):
        raise ValueError('Opening ' + name + ' floor_z must be finite.')
    if 'low' in opening:
        low, high = _box(opening, 'Opening ' + name)
        extent = [h - l for l, h in zip(low, high)]
        if 'normal' in opening:
            axis, _ = axis_of(opening['normal'], 'Opening ' + name + ' normal')
            if axis == 2:
                raise ValueError('Opening ' + name + ': a box opening passes along X or Y.')
        else:
            axis = 0 if extent[0] <= extent[1] else 1
        center = [(l + h) * .5 for l, h in zip(low, high)]
        yaw, width, height, depth = (0. if axis == 0 else 90.), extent[1 - axis], extent[2], extent[axis]
        if width <= 0 or height <= 0:
            raise ValueError('Opening ' + name + ' has no clear width or height.')
        sill = low[2]
    else:
        center = _point(opening.get('center'), 'Opening ' + name + ' center')
        normal = _point(opening.get('normal'), 'Opening ' + name + ' normal')
        length = math.sqrt(sum(v * v for v in normal))
        if length <= EPS or abs(normal[2]) / length > 0.01:
            raise ValueError('Opening ' + name + ' normal must be horizontal: passable reads doors and '
                             'windows in walls, not hatches or skylights.')
        yaw = math.degrees(math.atan2(normal[1], normal[0]))
        width = _finite(opening.get('width_cm'), 'Opening ' + name + ' width_cm', True)
        height = _finite(opening.get('height_cm'), 'Opening ' + name + ' height_cm', True)
        depth = _finite(opening.get('depth_cm', 0.), 'Opening ' + name + ' depth_cm')
        sill = center[2] - height * .5
    return dict(name=name, center=center, yaw_deg=yaw, width_cm=width, height_cm=height,
                depth_cm=depth, sill_z_cm=sill, floor_z=None if floor_z is None else float(floor_z),
                required=required)


def capsule_box(frame, radius_cm, half_height_cm):
    """The box a standing capsule sweeps through the opening: centred across it, feet on
    the sill, as long as the wall's depth plus one capsule diameter. It is the capsule's
    bounding box, so it is slightly larger than the capsule at its four rounded ends."""
    size = [frame['depth_cm'] + 2 * radius_cm, 2 * radius_cm, 2 * half_height_cm]
    center = [frame['center'][0], frame['center'][1], frame['sill_z_cm'] + half_height_cm]
    plan = _turned_rect(center, size, frame['yaw_deg'])
    return dict(center=center, size=size, yaw_deg=frame['yaw_deg'],
                low=[min(p[0] for p in plan), min(p[1] for p in plan), center[2] - half_height_cm],
                high=[max(p[0] for p in plan), max(p[1] for p in plan), center[2] + half_height_cm])


def passable(openings, radius_cm, half_height_cm, crouched_half_height_cm=None,
             tolerance_cm=0.0, max_step_cm=45.0, allowed=(), obstructed=None):
    """Whether an upright capsule passes through each stated opening, per posture.

    Unreal capsule convention: half_height_cm includes the hemispheres, so the capsule
    is 2*radius wide and 2*half_height tall (the default Character is 34 and 88, its
    crouch 40; CharacterMovement steps up 45). The capsule stands on the sill, centred
    across the opening, and moves along the opening's normal. Pure geometry on the
    opening the caller states: margin_cm per axis is clear size minus capsule size,
    positive a margin and negative a shortfall. A shortfall within tolerance_cm passes.
    obstructed: optional {opening name: {posture: [rows]}} from a mesh test; any row
      makes that posture fail. Without it nothing in the scene is consulted.
    allowed: [dict(a=opening name, reason=...)]: an opening known not to pass.
    Returns one row per opening; a row whose status is not 'passable' ('crouch_only'
    or 'not_passable', judged on the opening's required posture) is a finding.
    No walk route, nav mesh, mantle, camera boom or tilted opening is evaluated;
    sill_rise_cm and needs_climb only report the step up from floor_z.
    """
    radius = _finite(radius_cm, 'radius_cm', True)
    half = _finite(half_height_cm, 'half_height_cm', True)
    tolerance = _finite(tolerance_cm, 'tolerance_cm')
    step = _finite(max_step_cm, 'max_step_cm')
    postures = dict(standing=half)
    if crouched_half_height_cm is not None:
        postures['crouched'] = _finite(crouched_half_height_cm, 'crouched_half_height_cm', True)
    if any(h < radius for h in postures.values()):
        raise ValueError('A capsule half height includes its hemispheres, so it is at least the radius.')
    frames = [opening_frame(o) for o in openings]
    names = _names(frames, 'opening')
    if any(f['required'] == 'crouched' for f in frames) and 'crouched' not in postures:
        raise ValueError('An opening requires crouching: give crouched_half_height_cm.')
    permits = _allowances(allowed, ('a',), dict(a=names))
    label = 'capsule r=%g hh=%g' % (radius, half) + (
        ' crouched hh=%g' % postures['crouched'] if 'crouched' in postures else '')
    rows = []
    for frame in frames:
        row = dict(a=frame['name'], b=label, required=frame['required'],
                   width_cm=frame['width_cm'], height_cm=frame['height_cm'], depth_cm=frame['depth_cm'],
                   sill_z_cm=frame['sill_z_cm'], yaw_deg=frame['yaw_deg'])
        for posture, posture_half in postures.items():
            margin = dict(width_cm=frame['width_cm'] - 2 * radius,
                          height_cm=frame['height_cm'] - 2 * posture_half)
            blocked = list(((obstructed or {}).get(frame['name']) or {}).get(posture) or [])
            row[posture] = dict(half_height_cm=posture_half, margin_cm=margin,
                                fits=all(v >= -tolerance for v in margin.values()) and not blocked)
            if blocked:
                row[posture]['obstructed_by'] = blocked
        row['passes'] = next((p for p in ('standing', 'crouched') if p in postures and row[p]['fits']), None)
        if frame['floor_z'] is not None:
            row['sill_rise_cm'] = frame['sill_z_cm'] - frame['floor_z']
            row['needs_climb'] = row['sill_rise_cm'] > step + EPS
        if row[frame['required']]['fits']:
            row['status'] = 'passable'
        elif frame['required'] == 'standing' and row.get('crouched', {}).get('fits'):
            row['status'] = 'crouch_only'
        else:
            row['status'] = 'not_passable'
        permit = permits.get((frame['name'],))
        if permit and row['status'] != 'passable':
            row['accepted_reason'] = permit['reason']
        rows.append(row)
    return rows


# -- 3. a face hidden behind another part ------------------------------------

def occluded(parts, view_direction, tolerance_cm=0.5, max_gap_cm=25.0,
             min_covered_fraction=1.0, allowed=()):
    """Report a part whose viewer-facing bounds face is covered by another part's bounds
    lying just in front of it: the garage door behind a trim panel 2.5 cm proud of it.

    view_direction: the way the viewer looks, along a world axis ([1,0,0] looks toward
      +X, so the hidden face is the part's low-X face). Bounds only: a cover with a hole
      in it (a wall around the opening) reads as solid, and a turned part's bounds are
      larger than the part, so read the finding against the meshes before acting.
    A cover counts when its front lies in front of the hidden face and the gap from its
    back to that face is between -tolerance_cm and max_gap_cm. The cover's outline is
    widened by tolerance_cm before the covered fraction is measured.
    Pairs are checked when either part is selected (a part with no 'selected' is).
    allowed: [dict(a=hidden id, b=cover id, reason=...)].
    """
    axis, sign = axis_of(view_direction, 'view_direction')
    tolerance = _finite(tolerance_cm, 'tolerance_cm')
    gap_limit = _finite(max_gap_cm, 'max_gap_cm')
    fraction_limit = _finite(min_covered_fraction, 'min_covered_fraction', True)
    if fraction_limit > 1:
        raise ValueError('min_covered_fraction is at most 1.')
    parts = _parts(parts)
    ids = {p['id'] for p in parts}
    permits = _allowances(allowed, ('a', 'b'), dict(a=ids, b=ids))
    u, v = [i for i in range(3) if i != axis]
    findings = []
    for hidden in parts:
        area = (hidden['high'][u] - hidden['low'][u]) * (hidden['high'][v] - hidden['low'][v])
        if area <= EPS:
            continue
        face = hidden['low'][axis] if sign > 0 else hidden['high'][axis]
        for cover in parts:
            if cover is hidden or not (hidden.get('selected', True) or cover.get('selected', True)):
                continue
            if sign > 0:
                gap, in_front = face - cover['high'][axis], cover['low'][axis] < face - EPS
            else:
                gap, in_front = cover['low'][axis] - face, cover['high'][axis] > face + EPS
            if not in_front or gap < -tolerance or gap > gap_limit:
                continue
            covered = 1.
            for k in (u, v):
                covered *= max(0., min(hidden['high'][k], cover['high'][k] + tolerance)
                               - max(hidden['low'][k], cover['low'][k] - tolerance))
            fraction = min(1., covered / area)
            if fraction < fraction_limit - 1e-9:
                continue
            face_min, face_max = list(hidden['low']), list(hidden['high'])
            face_min[axis] = face_max[axis] = face
            finding = dict(status='hidden_face', a=hidden['id'], b=cover['id'],
                           view=axis_label(axis, sign), gap_cm=gap, covered_fraction=fraction,
                           face_min_cm=face_min, face_max_cm=face_max)
            permit = permits.get((hidden['id'], cover['id']))
            if permit:
                finding['accepted_reason'] = permit['reason']
            findings.append(finding)
    return findings


# -- 4. a wall-mounted part touching nothing ---------------------------------

def _matches(part, patterns):
    texts = [str(part.get(k) or '').lower() for k in ('id', 'label', 'asset')]
    return any(fnmatch.fnmatchcase(t, p.lower()) for p in patterns for t in texts if t)


def unmounted(parts, mounted=(), pattern=None, facing=None, tolerance_cm=0.5,
              count_mounted_as_support=False, allowed=()):
    """Report wall-mounted parts whose bounds touch no other part within tolerance_cm.

    mounted: part ids; pattern: a glob or list of globs matched, without case, against
      each part's id, actor label and asset path ('*Lettering*', '*Picture*'). At least
      one must name a part; naming nothing is an error, never a pass.
    facing: optional world-axis direction from the part toward its mount ([1,0,0]: the
      wall is on the part's +X side). Then only a part overlapping its outline and
      lying across or behind that back face counts. Without facing, any side counts.
    Other mounted parts are not supports unless count_mounted_as_support, so a row of
    letters touching each other still reads as standing off the fascia.
    allowed: [dict(a=part id, reason=...)]: a part meant to stand free.
    Bounds only: a wall's bounds include its openings, so a picture inside a window's
    outline reads as mounted; scan(support_roots=...) measures the meshes.
    A finding names the nearest support candidate (b, or None) and its gap_cm.
    """
    tolerance = _finite(tolerance_cm, 'tolerance_cm')
    parts = _parts(parts)
    ids = {p['id'] for p in parts}
    missing = [m for m in mounted if m not in ids]
    if missing:
        raise ValueError('Mounted ids are not captured parts: ' + ', '.join(map(str, missing)))
    patterns = [pattern] if isinstance(pattern, str) else list(pattern or ())
    named = [p for p in parts if p['id'] in set(mounted) or (patterns and _matches(p, patterns))]
    if not named:
        raise ValueError('No captured part is named as mounted (mounted ids or pattern).')
    named_ids = {p['id'] for p in named}
    permits = _allowances(allowed, ('a',), dict(a=named_ids))
    side = axis_of(facing, 'facing') if facing is not None else None
    findings = []
    for part in named:
        best = (math.inf, None)
        for other in parts:
            if other is part or (other['id'] in named_ids and not count_mounted_as_support):
                continue
            if side is None:
                gap = bounds_gap(part, other)
            else:
                axis, sign = side
                if any(min(part['high'][k], other['high'][k]) - max(part['low'][k], other['low'][k]) <= 0
                       for k in range(3) if k != axis):
                    continue
                if sign > 0:
                    back = part['high'][axis]
                    if other['high'][axis] < back - tolerance:
                        continue
                    gap = max(0., other['low'][axis] - back)
                else:
                    back = part['low'][axis]
                    if other['low'][axis] > back + tolerance:
                        continue
                    gap = max(0., back - other['high'][axis])
            best = min(best, (gap, other['id']))
        if best[0] <= tolerance:
            continue
        finding = dict(status='unmounted', a=part['id'], b=best[1],
                       gap_cm=None if best[1] is None else best[0],
                       side='any' if side is None else axis_label(*side))
        permit = permits.get((part['id'],))
        if permit:
            finding['accepted_reason'] = permit['reason']
        findings.append(finding)
    return findings
