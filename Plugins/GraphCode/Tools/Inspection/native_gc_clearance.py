"""Transient editor controls for the four clearance checks; no level/asset writes, no selection change.

Run inside the editor (Python). Plain-geometry controls are in test_gc_clearance.py.
"""
# SPDX-License-Identifier: MIT
# @Description: Verify swept_volumes, passable, occluded and unmounted on transient components.
from pathlib import Path
import importlib
import sys
import unreal
sys.dont_write_bytecode = True

HELPER = Path(unreal.PluginBlueprintLibrary.get_plugin_base_dir('GraphCode'))/'Tools'/'Inspection'
sys.path.insert(0, str(HELPER))
import gc_clearance
import gc_check_assembly as checker
import gc_inspection_geometry as geometry
for module in (gc_clearance, checker):
    importlib.reload(module)

CUBE = unreal.load_asset('/Engine/BasicShapes/Cube')  # 100 cm, pivot at its centre


def box(low, high):
    comp = unreal.new_object(unreal.StaticMeshComponent)
    comp.set_static_mesh(CUBE)
    comp.set_world_transform(unreal.Transform(
        location=unreal.Vector(*[(l+h)*.5 for l, h in zip(low, high)]),
        scale=unreal.Vector(*[(h-l)/100. for l, h in zip(low, high)])), False, True)
    return comp


def host(*components):
    class FixtureHost:
        def get_path_name(self): return '/Transient/ClearanceFixture'
        def get_actor_label(self): return 'Clearance fixture'
        def get_all_child_actors(self, include_descendants=True): return []
        def get_components_by_class(self, kind): return list(components)
    return FixtureHost()


def near(a, b, tol=.05):
    return abs(a-b) <= tol


def run():
    selected_before = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_selected_level_actors())
    results = []

    # P018: a picture hung where the hall door opens against the wall.
    picture, leaf = box((0, 40, 140), (3, 100, 190)), box((0, -2, 0), (90, 2, 205))
    door = dict(name='hall door', hinge=[0, 0, 0], leaf_cm=90, height_cm=210, from_deg=0, to_deg=90)
    report = checker.swept_volumes([door], actors=[host(picture, leaf)], allowed_intrusions=[dict(
        a=leaf.get_path_name(), b='hall door', max_depth_cm=95, reason='The leaf sweeps its own swing')],
        report_name='fixture-door-swing')
    rows = {f['a']: f for f in report['findings']}
    assert report['status'] == 'findings' and picture.get_path_name() in rows, report
    assert near(rows[picture.get_path_name()]['radial_depth_cm'], 50), report
    assert rows[leaf.get_path_name()].get('accepted_reason'), report
    results.append(dict(case='picture_in_door_swing', path=report['report_path']))

    # P104: a trim panel 2.5 cm in front of the garage door, seen from the street (-X).
    garage, trim = box((0, 0, 0), (5, 500, 220)), box((-7.5, 0, 0), (-2.5, 500, 220))
    report = checker.occluded([1, 0, 0], actors=[host(garage, trim)], report_name='fixture-garage-trim')
    assert [(f['a'], f['b']) for f in report['findings']] == [(garage.get_path_name(), trim.get_path_name())], report
    assert near(report['findings'][0]['gap_cm'], 2.5), report
    results.append(dict(case='garage_door_hidden', path=report['report_path']))

    # P108: lettering 3 cm off its fascia; the wall is on the letters' +X side.
    fascia, letter = box((0, 0, 300), (30, 400, 360)), box((-5, 40, 310), (-3, 80, 350))
    report = checker.unmounted(mounted=[letter.get_path_name()], facing=[1, 0, 0],
                               actors=[host(fascia, letter)], report_name='fixture-lettering')
    assert report['status'] == 'findings' and near(report['findings'][0]['gap_cm'], 3), report
    results.append(dict(case='lettering_off_fascia', path=report['report_path']))

    # P089: a 91 x 167.5 cm window 105 cm up; then a pane left in a door opening.
    window = dict(name='upper window', center=[0, 0, 188.75], normal=[1, 0, 0], width_cm=91,
                  height_cm=167.5, depth_cm=27, floor_z=0)
    report = checker.passable([window], 34, 88, 40, report_name='fixture-window')
    assert report['openings'][0]['status'] == 'crouch_only' and report['openings'][0]['needs_climb'], report
    opening = dict(name='door', center=[1000, 0, 105], normal=[1, 0, 0], width_cm=100, height_cm=210)
    pane = box((999, -45, 0), (1001, 45, 210))
    clear = checker.passable([opening], 34, 88, test_meshes=True, actors=[host(box((980, 60, 0), (1020, 80, 210)))],
                             max_parts=8, max_triangles=200, report_name='fixture-door-clear')
    assert clear['status'] == 'no_findings', clear
    blocked = checker.passable([opening], 34, 88, test_meshes=True, actors=[host(pane)],
                               max_parts=8, max_triangles=200, report_name='fixture-door-pane')
    assert blocked['openings'][0]['status'] == 'not_passable', blocked
    assert blocked['openings'][0]['standing']['obstructed_by'][0]['part'] == pane.get_path_name(), blocked
    results.append(dict(case='capsule_through_openings', path=blocked['report_path']))

    assert selected_before == list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_selected_level_actors())
    unreal.log('CLEARANCE_NATIVE passed=%d %s' % (len(results), results))
    return results


if __name__ == '__main__':
    # Same bounded fixture guard as Tools/Tests/native_assembly_inspection.py.
    previous = geometry.memory.MIN_FREE_RAM, geometry.memory.MIN_FREE_COMMIT
    try:
        geometry.memory.MIN_FREE_RAM = geometry.memory.MIN_FREE_COMMIT = 2*geometry.memory.GIB
        run()
    finally:
        geometry.memory.MIN_FREE_RAM, geometry.memory.MIN_FREE_COMMIT = previous
        geometry.memory.release()
