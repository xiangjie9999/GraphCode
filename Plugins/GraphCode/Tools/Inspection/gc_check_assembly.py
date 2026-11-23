"""Read-only selected-assembly/street inspection in Unreal Editor (UE 5.7).

See Docs/Guides/ASSEMBLY-VALIDATION.md. Geometry Script and Python Editor Script
Plugin are required. No actor, asset, transform, selection or collision is changed.
Reports are written only under Saved/GraphCode/Inspection.
"""
# SPDX-License-Identifier: MIT
import html
import itertools
import json
import math
from pathlib import Path
import time

import unreal
import gc_inspection_geometry as geometry
import gc_inspection_support as support
import gc_coplanar as coplanar
import gc_clearance as clearance


class BudgetExceeded(RuntimeError):
    pass


def _number(value, name, positive=False):
    if not math.isfinite(value) or value < 0 or (positive and value == 0):
        raise ValueError(name + ' must be finite and ' + ('positive.' if positive else 'nonnegative.'))
    return value


def _triangles(mesh, checkpoint):
    _, ids, _ = unreal.GeometryScript_MeshQueries.get_all_triangle_i_ds(mesh)
    result = []
    for tid in unreal.GeometryScript_List.convert_index_list_to_array(ids):
        checkpoint()
        valid, a, b, c = unreal.GeometryScript_MeshQueries.get_triangle_positions(mesh, tid)
        if not valid:
            raise ValueError('Source triangle could not be read.')
        result.append(coplanar.triangle([geometry.xyz(v) for v in (a, b, c)]))
    return result


def _socket_connections(actors, known, gap):
    connections, skipped = [], []
    for actor in geometry.descendants(actors):
        for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
            parent, socket = comp.get_attach_parent(), str(comp.get_attach_socket_name())
            if (isinstance(parent, unreal.ChildActorComponent)
                    and parent.get_editor_property('child_actor') == actor
                    and actor.get_editor_property('root_component') == comp and socket in ('', 'None')):
                socket, parent = str(parent.get_attach_socket_name()), parent.get_attach_parent()
            if not parent or socket in ('', 'None'):
                continue
            if not parent.does_socket_exist(socket):
                skipped.append(dict(part=comp.get_path_name(), reason='Missing parent socket: '+socket))
                continue
            a, b = comp.get_path_name(), parent.get_path_name()
            if a not in known or b not in known:
                skipped.append(dict(part=a, reason='Socket receiver/child is outside captured static geometry: '+b))
                continue
            connections.append(dict(a=a, b=b, max_gap_cm=gap, reason='Parent socket '+socket))
    return connections, skipped


def _probe_distance(part, point, bvhs):
    mesh = part['mesh']
    debug = unreal.GeometryScriptDebug()
    if part['id'] not in bvhs:
        _, bvhs[part['id']] = unreal.GeometryScript_MeshSpatial.build_bvh_for_mesh(mesh, debug)
    _, result, _ = unreal.GeometryScript_MeshSpatial.find_nearest_point_on_mesh(
        mesh, bvhs[part['id']], unreal.Vector(*point), unreal.GeometryScriptSpatialQueryOptions(), debug)
    if geometry.debug_messages(debug) or not result.valid:
        raise ValueError('Seam probe failed: '+'; '.join(geometry.debug_messages(debug)))
    return math.dist(point, geometry.xyz(result.position))


def scan(actors=None, neighbors=(), connections=(), support_roots=(), seam_probes=(),
         allowed_penetrations=(), gap_cm=0.25, coincident_cm=0.01,
         volume_epsilon_cm3=0.1, max_parts=128, max_triangles=250000,
         max_pairs=20000, max_triangle_tests=1000000, time_budget_seconds=45,
         report_name='assembly-check'):
    """Inspect selected actors + child actors, and only explicitly supplied neighbors.

    connections: [{a: component ID, b: ID, max_gap_cm: number, reason: text}].
    seam_probes: [{a: ID, b: ID, points_world_cm: [[x,y,z],...], max_gap_cm: number}].
    support_roots: component IDs whose largest island is an assumed support anchor.
    allowed_penetrations: [{a: ID, b: ID, max_volume_cm3: number, reason: text}].
    IDs (including instance suffixes) are returned in report['parts']; no label guessing.
    Time limits are checked BETWEEN native calls, not a preemption of one Boolean.
    """
    for key, value in dict(gap_cm=gap_cm, coincident_cm=coincident_cm,
                           volume_epsilon_cm3=volume_epsilon_cm3).items():
        _number(value, key)
    for key, value in dict(max_parts=max_parts, max_triangles=max_triangles,
                           max_pairs=max_pairs, max_triangle_tests=max_triangle_tests,
                           time_budget_seconds=time_budget_seconds).items():
        _number(value, key, True)
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    roots = list(subsystem.get_selected_level_actors() if actors is None else actors)
    if not roots:
        raise ValueError('Select an assembly or street section first; the entire level is never implicit.')
    actors = geometry.descendants(roots + list(neighbors))
    selected = {a.get_path_name() for a in geometry.descendants(roots)}
    start, work = time.monotonic(), 0
    def checkpoint():
        nonlocal work
        work += 1
        if work > max_triangle_tests or time.monotonic()-start > time_budget_seconds:
            raise BudgetExceeded('Time/triangle work budget reached; remaining checks were not run.')
    parts, triangles, bvhs, islands = [], {}, {}, []
    report = dict(schema=1, scope='selected actors, child actors and explicit neighbors; current editor pose',
                  actors=[a.get_path_name() for a in actors], findings=[], skipped=[],
                  connections=[], seams=[], parts=[], pairs_tested=0,
                  support={'status': 'not_requested'},
                  tolerances=dict(gap_cm=gap_cm, coincident_cm=coincident_cm,
                                  volume_epsilon_cm3=volume_epsilon_cm3))
    try:
        geometry.memory.checkpoint()
        parts, skipped = geometry.collect(actors, selected, max_parts, max_triangles, checkpoint)
        report['skipped'].extend(skipped)
        by_id = {p['id']: p for p in parts}
        if not any(p['selected'] for p in parts):
            report['skipped'].append(dict(part='selection', reason='No supported selected static geometry.'))
        for actor in actors:
            if isinstance(actor, unreal.LandscapeProxy):
                report['skipped'].append(dict(part=actor.get_path_name(), reason='Landscape surface is not inspected.'))
        for p in parts:
            p['low'], p['high'] = geometry.bounds(p['mesh'])
            report['parts'].append({k: p[k] for k in ('id', 'actor', 'label', 'asset', 'selected', 'low', 'high')})
        automatic, skipped = _socket_connections(actors, by_id, gap_cm)
        report['skipped'].extend(skipped)
        expected = {tuple(sorted((c['a'], c['b']))): dict(c) for c in automatic}
        for c in connections:
            expected[tuple(sorted((c['a'], c['b'])))] = dict(c)
        allowed = {}
        for item in allowed_penetrations:
            _number(item['max_volume_cm3'], 'max_volume_cm3')
            if not item.get('reason') or item['a'] not in by_id or item['b'] not in by_id:
                raise ValueError('A penetration allowance needs two captured IDs and a reason.')
            allowed[tuple(sorted((item['a'], item['b'])))] = item
        for a, b in itertools.combinations(parts, 2):
            checkpoint()
            if not (a['selected'] or b['selected']):
                continue
            if report['pairs_tested'] >= max_pairs:
                raise BudgetExceeded('Pair budget reached.')
            report['pairs_tested'] += 1
            if coplanar.bounds_gap(a, b) > coincident_cm:
                continue
            finding = geometry.inspect_pair(a['mesh'], b['mesh'], volume_epsilon_cm3)
            if finding['status'] in ('penetration', 'inconclusive', 'below_volume_tolerance'):
                finding.update(a=a['id'], b=b['id'])
                permit = allowed.get(tuple(sorted((a['id'], b['id']))))
                if (permit and finding['status'] == 'penetration'
                        and finding['volume_cm3'] <= permit['max_volume_cm3']):
                    finding['accepted_reason'] = permit['reason']
                report['findings'].append(finding)
                if finding['status'] == 'inconclusive':
                    report['skipped'].append(dict(part=a['id']+' / '+b['id'], reason=finding['reason']))
            for p in (a, b):
                if p['id'] not in triangles:
                    triangles[p['id']] = _triangles(p['mesh'], checkpoint)
            coincidence = coplanar.find(triangles[a['id']], triangles[b['id']], coincident_cm, checkpoint)
            if coincidence:
                report['findings'].append(dict(status='possible_z_fighting', a=a['id'], b=b['id'], **coincidence))
        for c in expected.values():
            checkpoint()
            _number(c['max_gap_cm'], 'connection max_gap_cm')
            if not c.get('reason') or c['a'] not in by_id or c['b'] not in by_id:
                raise ValueError('Expected connection needs two captured IDs and a reason: '+str(c))
            distance, method = support.surface_distance(by_id[c['a']]['mesh'], by_id[c['b']]['mesh'])
            row = dict(c, gap_cm=distance, method=method,
                       status='gap' if distance > c['max_gap_cm']+1e-5 else 'within_proximity_tolerance')
            report['connections'].append(row)
            if row['status'] == 'gap':
                report['findings'].append(row)
        for seam in seam_probes:
            checkpoint()
            _number(seam['max_gap_cm'], 'seam max_gap_cm')
            if seam['a'] not in by_id or seam['b'] not in by_id or not seam['points_world_cm']:
                raise ValueError('A seam needs two captured IDs and nonempty points_world_cm.')
            samples = []
            for point in seam['points_world_cm']:
                checkpoint()
                if len(point) != 3 or not all(math.isfinite(v) for v in point):
                    raise ValueError('Invalid world-space seam point.')
                da, db = (_probe_distance(by_id[seam[k]], point, bvhs) for k in ('a', 'b'))
                samples.append(dict(world_cm=point, distance_a_cm=da, distance_b_cm=db))
            worst = max(max(s['distance_a_cm'], s['distance_b_cm']) for s in samples)
            row = dict(a=seam['a'], b=seam['b'], samples=samples, maximum_distance_cm=worst,
                       max_gap_cm=seam['max_gap_cm'], status='seam_gap' if worst > seam['max_gap_cm'] else 'sampled_seam_within_tolerance')
            report['seams'].append(row)
            if row['status'] == 'seam_gap':
                report['findings'].append(row)
        if support_roots:
            checkpoint()
            islands = support.split_islands(parts)
            allowances = {pair: (c['max_gap_cm'], c['reason']) for pair, c in expected.items()}
            report['support'] = support.inspect_graph(islands, list(support_roots), gap_cm, allowances,
                max_pairs, max(0., time_budget_seconds-(time.monotonic()-start)))
            report['skipped'].extend(dict(part='support', reason=str(s)) for s in report['support']['skipped'])
            report['findings'].extend(dict(status='possible_orphan', **o) for o in report['support']['orphans'])
    except Exception as exc:
        report['skipped'].append(dict(part='remaining inspection', reason=str(exc)))
    finally:
        for p in parts + islands:
            p.pop('mesh', None)
        triangles.clear()
        bvhs.clear()
        if geometry._boolean_scratch is not None:
            geometry._boolean_scratch.reset()
        geometry.memory.release()
    report['complete'] = not report['skipped']
    report['status'] = ('incomplete' if not report['complete'] else
                        'findings' if any(not f.get('accepted_reason') for f in report['findings']) else 'no_findings')
    report['coverage'] = dict(expected_connections=len(report['connections']), seam_samples=sum(len(s['samples']) for s in report['seams']),
                              support_requested=bool(support_roots), neighbors_explicit=True)
    report['limitations'] = [
        'No findings is not a whole-world, load-bearing, complete-seam or collision/navigation certification.',
        'Selected/static source geometry only; current placed transforms, no animation sweep or material displacement.',
        'Spline/skeletal meshes and Landscape are unsupported; unselected neighbors and unloaded cells are outside scope.',
        'Coplanar areas are candidate source-triangle overlaps, including occluded faces; two-sided/back-facing materials are not evaluated.',
        'Gap queries sample vertices against triangles; closest edge interiors can be missed. Seam probes cover only their named points.',
        'Support roots are assumptions; proximity can be incidental and penetration must pass its separate check.',
        'Volume needs valid closed solids; source topology and overlaps within one component need Shape diagnostics.',
        'The time budget cannot interrupt a single native geometry call.']
    report['elapsed_seconds'] = round(time.monotonic()-start, 3)
    folder = Path(unreal.Paths.project_saved_dir())/'GraphCode'/'Inspection'
    folder.mkdir(parents=True, exist_ok=True)
    destination = folder/Path(report_name).name
    destination = destination.with_suffix('.json')
    report['report_path'] = str(destination.resolve())
    payload = json.dumps(report, indent=2)
    destination.write_text(payload, encoding='utf-8')
    destination.with_suffix('.html').write_text('<!doctype html><meta charset="utf-8"><title>Assembly check</title>'
        '<style>body{font:15px system-ui;margin:32px}pre{white-space:pre-wrap}</style>'
        '<h1>Assembly check: '+report['status']+'</h1><pre>'+html.escape(payload)+'</pre>', encoding='utf-8')
    unreal.log('Assembly check: '+report['status']+'; '+str(destination))
    return report


# Bounds-level clearance checks. gc_clearance holds the geometry (plain Python, tested
# offline by test_gc_clearance.py); these gather the parts in the editor and write the
# report in scan()'s shape, so findings from every check can share one list.

BOUNDS_LIMITATIONS = [
    'Bounds only: each part is the world axis-aligned box of its static mesh asset bounds (including any '
    'bounds extension) at its current transform. A turned part reads larger than it is; a part with a hole reads solid.',
    'No findings is not a whole-world, collision or navigation certification.',
    'Spline/skeletal meshes and Landscape are unsupported; unselected neighbors and unloaded cells are outside scope.',
    'Current editor pose only; a moving part needs one check per pose.']


def _roots(actors):
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    roots = list(subsystem.get_selected_level_actors() if actors is None else actors)
    if not roots:
        raise ValueError('Select an assembly or street section first; the entire level is never implicit.')
    return roots


def _bounds_parts(roots, neighbors, max_parts):
    """Static mesh component and instance bounds under scan()'s IDs, without copying meshes."""
    _number(max_parts, 'max_parts', True)
    captured = geometry.descendants(roots + list(neighbors))
    selected = {a.get_path_name() for a in geometry.descendants(roots)}
    parts, skipped = [], []
    for actor in captured:
        if isinstance(actor, unreal.LandscapeProxy):
            skipped.append(dict(part=actor.get_path_name(), reason='Landscape surface is not inspected.'))
        for comp in actor.get_components_by_class(unreal.MeshComponent):
            name = comp.get_path_name()
            if not isinstance(comp, unreal.StaticMeshComponent) or isinstance(comp, unreal.SplineMeshComponent):
                skipped.append(dict(part=name, reason='Deformed or non-static mesh is not supported.'))
                continue
            asset = comp.get_editor_property('static_mesh')
            if not asset:
                skipped.append(dict(part=name, reason='No static mesh.'))
                continue
            try:
                box = asset.get_bounding_box()
                corners = [unreal.Vector(x, y, z) for x in (box.min.x, box.max.x)
                           for y in (box.min.y, box.max.y) for z in (box.min.z, box.max.z)]
                if isinstance(comp, unreal.InstancedStaticMeshComponent):
                    transforms = [(f'{name}[{i}]', comp.get_instance_transform(i, world_space=True))
                                  for i in range(comp.get_instance_count())]
                else:
                    transforms = [(name, comp.get_world_transform())]
                for part_name, transform in transforms:
                    if len(parts) >= max_parts:
                        skipped.append(dict(part='remaining geometry', reason='Part budget reached.'))
                        return captured, parts, skipped
                    points = [geometry.xyz(transform.transform_location(c)) for c in corners]
                    parts.append(dict(id=part_name, actor=actor.get_path_name(), label=actor.get_actor_label(),
                                      asset=asset.get_path_name(), selected=actor.get_path_name() in selected,
                                      low=[min(p[i] for p in points) for i in range(3)],
                                      high=[max(p[i] for p in points) for i in range(3)]))
            except Exception as exc:
                skipped.append(dict(part=name, reason=str(exc)))
    return captured, parts, skipped


def _clearance_report(check, title, start, captured, parts, skipped, findings, tolerances,
                      limitations, report_name, **extra):
    report = dict(schema=1, check=check,
                  scope='selected actors, child actors and explicit neighbors; current editor pose',
                  actors=[a.get_path_name() for a in captured], findings=findings, skipped=skipped,
                  connections=[], seams=[], parts=parts, pairs_tested=0,
                  support={'status': 'not_requested'}, tolerances=tolerances, **extra)
    report['complete'] = not report['skipped']
    report['status'] = ('incomplete' if not report['complete'] else
                        'findings' if any(not f.get('accepted_reason') for f in report['findings']) else 'no_findings')
    report['coverage'] = dict(check=check, parts=len(parts), neighbors_explicit=True)
    report['limitations'] = limitations
    report['elapsed_seconds'] = round(time.monotonic()-start, 3)
    folder = Path(unreal.Paths.project_saved_dir())/'GraphCode'/'Inspection'
    folder.mkdir(parents=True, exist_ok=True)
    destination = (folder/Path(report_name).name).with_suffix('.json')
    report['report_path'] = str(destination.resolve())
    payload = json.dumps(report, indent=2)
    destination.write_text(payload, encoding='utf-8')
    destination.with_suffix('.html').write_text('<!doctype html><meta charset="utf-8"><title>'+title+'</title>'
        '<style>body{font:15px system-ui;margin:32px}pre{white-space:pre-wrap}</style>'
        '<h1>'+title+': '+report['status']+'</h1><pre>'+html.escape(payload)+'</pre>', encoding='utf-8')
    unreal.log(title+': '+report['status']+'; '+str(destination))
    return report


def swept_volumes(volumes, actors=None, neighbors=(), tolerance_cm=0.5, allowed_intrusions=(),
                  max_parts=2048, report_name='clear-volume-check'):
    """Parts whose bounds enter a door swing, an opening or a stair the caller states.

    volumes, allowed_intrusions: see gc_clearance.swept_volumes. Every captured part
    (selection, child actors and explicit neighbors) is tested against every volume;
    doors are never inferred from meshes. Findings: status 'inside_clear_volume',
    a = part ID, b = volume name, depth_cm. Bounds only.
    """
    start = time.monotonic()
    roots = _roots(actors)
    captured, parts, skipped, findings = [], [], [], []
    try:
        captured, parts, skipped = _bounds_parts(roots, neighbors, max_parts)
        findings = clearance.swept_volumes(parts, volumes, tolerance_cm, allowed_intrusions)
    except Exception as exc:
        skipped.append(dict(part='remaining inspection', reason=str(exc)))
    return _clearance_report(
        'swept_volumes', 'Clear-volume check', start, captured, parts, skipped, findings,
        dict(tolerance_cm=tolerance_cm),
        BOUNDS_LIMITATIONS + ['Only the volumes the caller states are checked; leaf thickness is not added.'],
        report_name, volumes=[v.get('name') for v in volumes])


def _box_mesh(box):
    geometry.memory.checkpoint()
    mesh = unreal.DynamicMesh()
    debug = unreal.GeometryScriptDebug()
    unreal.GeometryScript_Primitives.append_box(
        mesh, unreal.GeometryScriptPrimitiveOptions(),
        unreal.Transform(location=unreal.Vector(*box['center']), rotation=unreal.Rotator(yaw=box['yaw_deg'])),
        box['size'][0], box['size'][1], box['size'][2], 0, 0, 0,
        unreal.GeometryScriptPrimitiveOriginMode.CENTER, debug)
    if geometry.debug_messages(debug) or mesh.get_triangle_count() == 0:
        raise ValueError('Capsule box could not be built: '+'; '.join(geometry.debug_messages(debug)))
    return mesh


def passable(openings, radius_cm, half_height_cm, crouched_half_height_cm=None, tolerance_cm=0.0,
             max_step_cm=45.0, allowed_blocked=(), test_meshes=False, actors=None, neighbors=(),
             volume_epsilon_cm3=0.1, max_parts=128, max_triangles=250000, time_budget_seconds=45,
             report_name='opening-capsule-check'):
    """Whether an upright capsule passes each stated opening, standing and optionally crouched.

    openings, allowed_blocked and the capsule: see gc_clearance.passable. The result
    lists every opening in report['openings'] with margin_cm per axis; a row whose
    status is 'crouch_only' or 'not_passable' is also a finding (a = opening name).
    Without test_meshes this is geometry on the stated openings: no level is read and
    no selection is needed. The existing helpers have no physics collision query; they
    read render triangles. test_meshes=True reuses scan()'s solid-overlap test between
    each captured static mesh and the capsule's box through the opening (glass, a
    mullion, a jamb set too close). It is render geometry, not the collision a character
    meets: a simple collision box filling a window is not seen. Check that in Play.
    """
    start = time.monotonic()
    roots = _roots(actors) if test_meshes else []
    captured, report_parts, skipped, rows, mesh_parts = [], [], [], [], []
    def checkpoint():
        if time.monotonic()-start > time_budget_seconds:
            raise BudgetExceeded('Time budget reached; remaining openings were not mesh-tested.')
    try:
        # Validate everything before any mesh is read; the geometry-only rows stand alone.
        rows = clearance.passable(openings, radius_cm, half_height_cm, crouched_half_height_cm,
                                  tolerance_cm, max_step_cm, allowed_blocked)
        if test_meshes:
            for key, value in dict(volume_epsilon_cm3=volume_epsilon_cm3).items():
                _number(value, key)
            for key, value in dict(max_parts=max_parts, max_triangles=max_triangles,
                                   time_budget_seconds=time_budget_seconds).items():
                _number(value, key, True)
            captured = geometry.descendants(roots + list(neighbors))
            selected = {a.get_path_name() for a in geometry.descendants(roots)}
            geometry.memory.checkpoint()
            mesh_parts, more = geometry.collect(captured, selected, max_parts, max_triangles, checkpoint)
            skipped.extend(more)
            for p in mesh_parts:
                p['low'], p['high'] = geometry.bounds(p['mesh'])
                report_parts.append({k: p[k] for k in ('id', 'actor', 'label', 'asset', 'selected', 'low', 'high')})
            postures = [('standing', half_height_cm)]
            if crouched_half_height_cm is not None:
                postures.append(('crouched', crouched_half_height_cm))
            obstructed = {}
            for opening in openings:
                frame = clearance.opening_frame(opening)
                for posture, half in postures:
                    box, probe = clearance.capsule_box(frame, radius_cm, half), None
                    for p in mesh_parts:
                        checkpoint()
                        if coplanar.bounds_gap(p, box) > 0:
                            continue
                        if probe is None:
                            probe = _box_mesh(box)
                        result = geometry.inspect_pair(p['mesh'], probe, volume_epsilon_cm3)
                        if result['status'] == 'penetration':
                            obstructed.setdefault(frame['name'], {}).setdefault(posture, []).append(dict(
                                part=p['id'], volume_cm3=result['volume_cm3'],
                                region_center_cm=result['region_center_cm']))
                        elif result['status'] == 'inconclusive':
                            skipped.append(dict(part=p['id']+' / '+frame['name'], reason=result['reason']))
            rows = clearance.passable(openings, radius_cm, half_height_cm, crouched_half_height_cm,
                                      tolerance_cm, max_step_cm, allowed_blocked, obstructed)
    except Exception as exc:
        skipped.append(dict(part='remaining inspection', reason=str(exc)))
    finally:
        for p in mesh_parts:
            p.pop('mesh', None)
        if geometry._boolean_scratch is not None:
            geometry._boolean_scratch.reset()
        if test_meshes:
            geometry.memory.release()
    limitations = ['Pure geometry on the stated openings: no walk route, nav mesh, mantle, camera boom or tilted opening.',
                   'The capsule stands on the sill, centred across the opening; sill_rise_cm only reports the step up.']
    if test_meshes:
        limitations += ['Mesh test: render triangles against the capsule\'s bounding box (slightly larger than the '
                        'capsule at its rounded ends), not collision geometry.'] + BOUNDS_LIMITATIONS[2:]
    return _clearance_report(
        'passable', 'Opening capsule check', start, captured, report_parts, skipped,
        [r for r in rows if r['status'] != 'passable'],
        dict(tolerance_cm=tolerance_cm, radius_cm=radius_cm, half_height_cm=half_height_cm,
             crouched_half_height_cm=crouched_half_height_cm, max_step_cm=max_step_cm,
             volume_epsilon_cm3=volume_epsilon_cm3 if test_meshes else None),
        limitations, report_name, openings=rows, meshes_tested=bool(test_meshes))


def occluded(view_direction, actors=None, neighbors=(), tolerance_cm=0.5, max_gap_cm=25.0,
             min_covered_fraction=1.0, allowed_hidden=(), max_parts=2048, report_name='hidden-face-check'):
    """Parts whose viewer-facing bounds face is covered by another part just in front of it.

    view_direction, max_gap_cm, min_covered_fraction, allowed_hidden: see
    gc_clearance.occluded. Pairs where at least one part is selected are checked, as in
    scan(). Findings: status 'hidden_face', a = hidden part, b = the part hiding it,
    gap_cm, covered_fraction. Bounds only: a cover with a hole reads as solid.
    """
    start = time.monotonic()
    roots = _roots(actors)
    captured, parts, skipped, findings = [], [], [], []
    try:
        captured, parts, skipped = _bounds_parts(roots, neighbors, max_parts)
        findings = clearance.occluded(parts, view_direction, tolerance_cm, max_gap_cm,
                                      min_covered_fraction, allowed_hidden)
    except Exception as exc:
        skipped.append(dict(part='remaining inspection', reason=str(exc)))
    return _clearance_report(
        'occluded', 'Hidden-face check', start, captured, parts, skipped, findings,
        dict(tolerance_cm=tolerance_cm, max_gap_cm=max_gap_cm, min_covered_fraction=min_covered_fraction),
        BOUNDS_LIMITATIONS + ['One view direction along a world axis; materials, translucency and other views are not evaluated.'],
        report_name, view_direction=list(view_direction))


def unmounted(mounted=(), pattern=None, facing=None, actors=None, neighbors=(), tolerance_cm=0.5,
              count_mounted_as_support=False, allowed_unmounted=(), max_parts=2048,
              report_name='mount-check'):
    """Wall-mounted parts (listed IDs or a name pattern) whose bounds touch no other part.

    mounted, pattern, facing, count_mounted_as_support, allowed_unmounted: see
    gc_clearance.unmounted. Supports may be any captured part, neighbors included.
    Findings: status 'unmounted', a = the part, b = the nearest candidate or None, gap_cm.
    Bounds only; scan(support_roots=...) measures the meshes.
    """
    start = time.monotonic()
    roots = _roots(actors)
    captured, parts, skipped, findings = [], [], [], []
    try:
        captured, parts, skipped = _bounds_parts(roots, neighbors, max_parts)
        findings = clearance.unmounted(parts, mounted, pattern, facing, tolerance_cm,
                                       count_mounted_as_support, allowed_unmounted)
    except Exception as exc:
        skipped.append(dict(part='remaining inspection', reason=str(exc)))
    return _clearance_report(
        'unmounted', 'Mount check', start, captured, parts, skipped, findings,
        dict(tolerance_cm=tolerance_cm),
        BOUNDS_LIMITATIONS + ['Touching bounds is not a fixing; a wall\'s bounds include its openings.'],
        report_name, mounted=list(mounted), pattern=pattern, facing=None if facing is None else list(facing))
