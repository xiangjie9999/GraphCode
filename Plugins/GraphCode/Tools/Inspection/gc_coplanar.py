"""Potential z-fighting between same-facing source triangles; no visibility claim.

Pure Python, centimetres. Positive projected overlap is required: touching edges and
overlapping bounding boxes alone are not findings. No Unreal or project dependency.
"""
# SPDX-License-Identifier: MIT
import math


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def triangle(points):
    normal = cross(sub(points[1], points[0]), sub(points[2], points[0]))
    length = math.sqrt(dot(normal, normal))
    if not math.isfinite(length) or length <= 1e-12:
        raise ValueError('Non-finite or degenerate triangle; coverage is incomplete.')
    return dict(points=points, normal=tuple(x/length for x in normal),
                low=tuple(min(p[i] for p in points) for i in range(3)),
                high=tuple(max(p[i] for p in points) for i in range(3)))


def bounds_gap(a, b):
    return math.sqrt(sum(max(0., a['low'][i]-b['high'][i],
                            b['low'][i]-a['high'][i])**2 for i in range(3)))


def overlap(a, b, separation_cm=0.01, normal_cos=0.9999):
    """Return area (cm2), maximum plane separation and an overlap point, or None."""
    if bounds_gap(a, b) > separation_cm or dot(a['normal'], b['normal']) < normal_cos:
        return None
    gap = max(abs(dot(sub(p, a['points'][0]), a['normal'])) for p in b['points'])
    gap = max(gap, max(abs(dot(sub(p, b['points'][0]), b['normal'])) for p in a['points']))
    if gap > separation_cm:
        return None
    axis = max(range(3), key=lambda i: abs(a['normal'][i]))
    axes = [i for i in range(3) if i != axis]
    # Translate before clipping to avoid cancellation at large world coordinates.
    origin = a['points'][0]
    project = lambda p: tuple(p[i]-origin[i] for i in axes)
    polygon = [project(p) for p in a['points']]
    clip = [project(p) for p in b['points']]
    side = lambda u, v, p: (v[0]-u[0])*(p[1]-u[1])-(v[1]-u[1])*(p[0]-u[0])
    if side(clip[0], clip[1], clip[2]) < 0:
        clip.reverse()
    for u, v in zip(clip, clip[1:]+clip[:1]):
        before, polygon = polygon, []
        if not before:
            return None
        for p, q in zip(before, before[1:]+before[:1]):
            dp, dq = side(u, v, p), side(u, v, q)
            if dp >= 0:
                polygon.append(p)
            if (dp >= 0) != (dq >= 0):
                t = dp / (dp-dq)
                polygon.append(tuple(p[i]+t*(q[i]-p[i]) for i in range(2)))
    if len(polygon) < 3:
        return None
    area = abs(sum(p[0]*q[1]-p[1]*q[0] for p, q in
                   zip(polygon, polygon[1:]+polygon[:1]))) * .5 / abs(a['normal'][axis])
    if area <= 1e-8:
        return None
    # A vertex mean lies inside this convex clipped polygon. Report a point in
    # the actual overlap, not the centre of a potentially enormous source face.
    centre = [sum(p[i] for p in polygon)/len(polygon) for i in range(2)]
    location = list(origin)
    for i, coord in enumerate(axes):
        location[coord] += centre[i]
    location[axis] -= sum(a['normal'][coord]*centre[i] for i, coord in enumerate(axes))/a['normal'][axis]
    return area, gap, location


def find(a, b, separation_cm, checkpoint):
    """Sweep one axis; caller's checkpoint bounds every candidate, including rejects."""
    axis = max(range(3), key=lambda i: max(t['high'][i] for t in a+b)
               - min(t['low'][i] for t in a+b))
    events = []
    for owner, triangles in enumerate((a, b)):
        for index, tri in enumerate(triangles):
            events.append((tri['low'][axis], 0, owner, index))
            events.append((tri['high'][axis]+separation_cm, 1, owner, index))
    active = ({}, {})
    area, gap, pairs, location = 0., 0., 0, None
    for _, ending, owner, index in sorted(events):
        checkpoint()
        if ending:
            active[owner].pop(index, None)
            continue
        first = (a, b)[owner][index]
        for second in active[1-owner].values():
            checkpoint()
            result = overlap(first, second, separation_cm)
            if result:
                area += result[0]
                gap = max(gap, result[1])
                pairs += 1
                if location is None:
                    location = result[2]
        active[owner][index] = first
    return dict(area_cm2=area, separation_cm=gap, triangle_pairs=pairs,
                near_world_cm=location) if pairs else None
