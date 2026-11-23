"""Read-only support-graph inspection, including disconnected islands inside a mesh."""
import itertools
import math
import time

import unreal
import gc_inspection_geometry as geometry


def surface_distance(a, b):
    """Triangle intersection first; otherwise symmetric vertex-to-surface distance."""
    geometry.memory.checkpoint()
    debug = unreal.GeometryScriptDebug()
    _, intersects = unreal.GeometryScript_MeshComparison.is_intersecting_mesh(
        a, geometry.IDENTITY, b, geometry.IDENTITY, debug)
    if geometry.debug_messages(debug):
        raise ValueError('; '.join(geometry.debug_messages(debug)))
    if intersects:
        return 0.0, 'surface_intersection'
    _, _, minimum, _, _ = unreal.GeometryScript_MeshComparison.measure_distances_between_meshes(
        a, b, unreal.GeometryScriptMeasureMeshDistanceOptions(symmetric=True), debug)
    if geometry.debug_messages(debug) or not math.isfinite(minimum):
        raise ValueError('Distance query failed: ' + '; '.join(geometry.debug_messages(debug)))
    return float(minimum), 'sampled_surface_gap'


def split_islands(parts):
    nodes = []
    for part in parts:
        geometry.memory.checkpoint()
        debug = unreal.GeometryScriptDebug()
        _, islands = unreal.GeometryScript_MeshDecomposition.split_mesh_by_vertex_overlap(
            part['mesh'], None, 0.001, debug)
        if geometry.debug_messages(debug) or not islands:
            raise ValueError('Could not split ' + part['id'])
        # Stable largest-first order makes the support anchor unambiguous.
        for i, mesh in enumerate(sorted(islands, key=lambda m: m.get_triangle_count(), reverse=True)):
            # Boolean outputs can have vertex-ID gaps. UE's distance statistics
            # include zero-filled gaps; append into a fresh mesh to compact IDs.
            mesh = geometry.mesh_copy(mesh)
            low, high = geometry.bounds(mesh)
            nodes.append(dict(id=part['id'] + '#island-' + str(i), component=part['id'],
                              mesh=mesh, low=low, high=high, triangles=mesh.get_triangle_count(),
                              signed_volume=unreal.GeometryScript_MeshQueries.get_mesh_volume_area(mesh)[1]))
    return nodes


def inspect_graph(nodes, root_components, tolerance_cm=0.25, joint_gaps=None,
                  max_pairs=20000, time_budget_seconds=45):
    """Only measured surface proximity creates edges; hierarchy/AABBs never do.

    joint_gaps: exact sorted component-path pairs -> (maximum gap, physical reason).
    A declared running clearance is still measured, and never a blanket exemption.
    """
    if tolerance_cm < 0 or not root_components:
        raise ValueError('Supply a nonnegative tolerance and an explicit support component.')
    start = time.monotonic()
    memory_limited = False
    links, skipped, adjacency = [], [], {n['id']: set() for n in nodes}
    # An enclosed, inward-facing shell is the boundary of a void in its owner,
    # not a floating solid. Prove containment and absence of surface crossings.
    cavities = []
    for cavity in (n for n in nodes if n['signed_volume'] < 0):
        if time.monotonic() - start > time_budget_seconds:
            skipped.append('Remaining cavity checks: inspection budget reached.')
            break
        point, valid = unreal.GeometryScript_MeshQueries.get_vertex_position(cavity['mesh'], 0)
        if not valid or not unreal.GeometryScript_MeshQueries.get_is_closed_mesh(cavity['mesh']):
            skipped.append('Invalid inward shell: ' + cavity['id'])
            continue
        for owner in (n for n in nodes if n['component'] == cavity['component'] and n['signed_volume'] > 0):
            debug = unreal.GeometryScriptDebug()
            _, bvh = unreal.GeometryScript_MeshSpatial.build_bvh_for_mesh(owner['mesh'], debug)
            _, inside, _ = unreal.GeometryScript_MeshSpatial.is_point_inside_mesh(
                owner['mesh'], bvh, point, unreal.GeometryScriptSpatialQueryOptions(), debug)
            _, intersects = unreal.GeometryScript_MeshComparison.is_intersecting_mesh(
                cavity['mesh'], geometry.IDENTITY, owner['mesh'], geometry.IDENTITY, debug)
            if geometry.debug_messages(debug):
                skipped.append('Cavity containment query: ' + '; '.join(geometry.debug_messages(debug)))
            elif inside and not intersects:
                adjacency[cavity['id']].add(owner['id'])
                adjacency[owner['id']].add(cavity['id'])
                cavities.append(dict(boundary=cavity['id'], owner=owner['id'],
                                     reason='Closed inward cavity boundary, contained without surface crossings'))
                break
    tested = 0
    for a, b in itertools.combinations(nodes, 2):
        if tested >= max_pairs or time.monotonic() - start > time_budget_seconds:
            skipped.append('Remaining island pairs: inspection budget reached.')
            break
        tested += 1
        limit, reason = (joint_gaps or {}).get(tuple(sorted((a['component'], b['component']))),
                                             (tolerance_cm, 'ordinary assembly seam'))
        if limit < 0 or not reason:
            raise ValueError('Joint allowance needs a nonnegative limit and a reason.')
        lower_bound = math.sqrt(sum(max(0, al-bh, bl-ah)**2
                                    for al, ah, bl, bh in zip(a['low'], a['high'], b['low'], b['high'])))
        if lower_bound > limit:
            continue
        try:
            distance, method = surface_distance(a['mesh'], b['mesh'])
        except geometry.memory.MemoryBudgetExceeded as exc:
            skipped.append('Remaining support pairs: ' + str(exc))
            memory_limited = True
            break
        except Exception as exc:
            skipped.append(a['id'] + ' / ' + b['id'] + ': ' + str(exc))
            continue
        if distance <= limit + 1e-5:
            adjacency[a['id']].add(b['id'])
            adjacency[b['id']].add(a['id'])
            links.append(dict(a=a['id'], b=b['id'], gap_cm=distance, limit_cm=limit,
                              reason=reason, method=method))
    anchors = {component + '#island-0' for component in root_components}
    missing = anchors - adjacency.keys()
    if missing:
        skipped.append('Missing support anchors: ' + ', '.join(sorted(missing)))
    reached = anchors & adjacency.keys()
    pending = list(reached)
    while pending:
        for other in adjacency[pending.pop()] - reached:
            reached.add(other)
            pending.append(other)
    orphans = [dict(island=n['id'], component=n['component'], triangles=n['triangles'],
                    center_cm=[(l+h)*0.5 for l, h in zip(n['low'], n['high'])])
               for n in nodes if n['id'] not in reached]
    by_id = {n['id']: n for n in nodes}
    for orphan in orphans:
        a = by_id[orphan['island']]
        distances = []
        for b in nodes:
            if memory_limited:
                break
            if time.monotonic() - start > time_budget_seconds:
                skipped.append('Remaining nearest-support queries: inspection budget reached.')
                memory_limited = True
                break
            if b['id'] in reached:
                try:
                    gap, method = surface_distance(a['mesh'], b['mesh'])
                    distances.append(dict(island=b['id'], gap_cm=gap, method=method))
                except geometry.memory.MemoryBudgetExceeded as exc:
                    skipped.append('Remaining nearest-support queries: ' + str(exc))
                    memory_limited = True
                    break
                except Exception as exc:
                    skipped.append('Orphan nearest-support query: ' + str(exc))
        orphan['nearest_support'] = min(distances, key=lambda row: row['gap_cm']) if distances else None
    return dict(complete=not skipped, islands=len(nodes), pairs_tested=tested,
                support_anchors=sorted(anchors), links=links, cavity_boundaries=cavities,
                orphans=orphans, skipped=skipped,
                tolerance_cm=tolerance_cm, elapsed_seconds=round(time.monotonic()-start, 3))


