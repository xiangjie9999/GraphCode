"""Offline controls for gc_clearance (plain Python, no Unreal): python -I test_gc_clearance.py"""
# SPDX-License-Identifier: MIT
import math
from pathlib import Path
import sys
import unittest
sys.dont_write_bytecode = True

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gc_clearance as c


def part(name, low, high, **extra):
    return dict(id=name, low=list(low), high=list(high), **extra)


# A door hinged at the origin, closed along +X, opening 90 degrees to lie along +Y,
# against a wall whose face is the plane x = 0 (the wall is on the -X side).
DOOR = dict(name='hall door', hinge=[0, 0, 0], leaf_cm=90, height_cm=210, from_deg=0, to_deg=90,
            reason='The hall door must open fully')


class SweptVolumeTests(unittest.TestCase):
    def test_picture_behind_the_door_swing(self):
        picture = part('Picture', (0, 40, 140), (3, 100, 190))
        (row,) = c.swept_volumes([picture], [DOOR])
        self.assertEqual((row['status'], row['a'], row['b']), ('inside_clear_volume', 'Picture', 'hall door'))
        self.assertAlmostEqual(row['radial_depth_cm'], 90 - math.hypot(0, 40))
        self.assertAlmostEqual(row['vertical_overlap_cm'], 50)
        self.assertAlmostEqual(row['depth_cm'], 50)
        self.assertAlmostEqual(row['opened_deg'], math.degrees(math.atan2(40, 3)), places=6)
        self.assertAlmostEqual(row['strike_yaw_deg'][1], 90, places=6)
        self.assertEqual(row['volume_reason'], 'The hall door must open fully')

    def test_picture_beyond_the_leaf_is_clear(self):
        self.assertEqual(c.swept_volumes([part('Picture', (0, 95, 140), (3, 150, 190))], [DOOR]), [])

    def test_part_outside_the_angle_is_clear(self):
        # Behind the hinge (negative Y) the closed-to-open sweep never reaches.
        self.assertEqual(c.swept_volumes([part('Chair', (10, -60, 0), (50, -20, 90))], [DOOR]), [])

    def test_clockwise_swing(self):
        door = dict(DOOR, to_deg=-90)
        rows = c.swept_volumes([part('Left', (0, -100, 140), (3, -40, 190)),
                                part('Right', (0, 40, 140), (3, 100, 190))], [door])
        self.assertEqual([r['a'] for r in rows], ['Left'])
        self.assertAlmostEqual(rows[0]['strike_yaw_deg'][1], -90, places=6)
        self.assertAlmostEqual(rows[0]['opened_deg'], math.degrees(math.atan2(40, 3)), places=6)

    def test_above_the_door_height_is_clear(self):
        self.assertEqual(c.swept_volumes([part('Shelf', (0, 40, 215), (30, 100, 240))], [DOOR]), [])

    def test_large_world_coordinates(self):
        far = dict(DOOR, hinge=[1e6, 2e6, 100])
        (row,) = c.swept_volumes([part('Picture', (1e6, 2e6+40, 240), (1e6+3, 2e6+100, 290))], [far])
        self.assertAlmostEqual(row['radial_depth_cm'], 50, places=6)

    def test_wide_swing_over_180_degrees(self):
        door = dict(DOOR, from_deg=-100, to_deg=100)
        rows = c.swept_volumes([part('Back', (-60, -5, 0), (-40, 5, 90))], [door])
        self.assertEqual(rows, [])  # straight behind the hinge, outside -100..100
        rows = c.swept_volumes([part('Side', (-10, -60, 0), (10, -40, 90))], [door])
        self.assertEqual(len(rows), 1)

    def test_fireplace_in_the_stair_reports_the_deepest_step(self):
        steps = [dict(low=[100+25*k, 0, 0], high=[125+25*k, 100, 18*(k+1)]) for k in range(8)]
        fireplace = part('Fireplace', (0, 0, 0), (150, 60, 120))
        (row,) = c.swept_volumes([fireplace], [dict(name='stair', boxes=steps)])
        self.assertEqual((row['box_index'], row['depth_cm']), (1, 25))
        self.assertEqual(row['region_min_cm'], [125, 0, 0])
        self.assertEqual(row['region_max_cm'], [150, 60, 36])

    def test_turned_box_is_not_its_bounds(self):
        diamond = dict(name='landing', center=[0, 0, 100], size=[100, 100, 200], yaw_deg=45)
        self.assertEqual(c.swept_volumes([part('Corner', (60, 60, 0), (70, 70, 10))], [diamond]), [])
        (row,) = c.swept_volumes([part('Inside', (-10, -10, 0), (10, 10, 10))], [diamond])
        self.assertAlmostEqual(row['depth_cm'], 10)

    def test_touching_or_within_tolerance_is_clear(self):
        opening = dict(name='opening', low=[0, 0, 0], high=[20, 100, 210])
        self.assertEqual(c.swept_volumes([part('Jamb', (20, 0, 0), (30, 100, 210))], [opening]), [])
        self.assertEqual(c.swept_volumes([part('Trim', (19.7, 0, 0), (30, 100, 210))], [opening]), [])
        self.assertEqual(len(c.swept_volumes([part('Trim', (19.0, 0, 0), (30, 100, 210))], [opening])), 1)

    def test_allowance_accepts_only_up_to_its_depth(self):
        leaf = part('Leaf', (0, -2, 0), (90, 2, 205))
        ok = c.swept_volumes([leaf], [DOOR], allowed=[dict(a='Leaf', b='hall door', max_depth_cm=95,
                                                          reason='The leaf sweeps its own swing')])
        self.assertEqual(ok[0]['accepted_reason'], 'The leaf sweeps its own swing')
        short = c.swept_volumes([leaf], [DOOR], allowed=[dict(a='Leaf', b='hall door', max_depth_cm=5,
                                                             reason='too small')])
        self.assertNotIn('accepted_reason', short[0])

    def test_bad_input_is_an_error_not_a_pass(self):
        with self.assertRaises(ValueError):
            c.swept_volumes([part('A', (0, 0, 0), (1, 1, 1))], [DOOR],
                            allowed=[dict(a='Missing', b='hall door', max_depth_cm=1, reason='x')])
        with self.assertRaises(ValueError):
            c.swept_volumes([], [dict(DOOR, to_deg=0)])
        with self.assertRaises(ValueError):
            c.swept_volumes([], [DOOR, DOOR])


class PassableTests(unittest.TestCase):
    HERO = dict(radius_cm=34, half_height_cm=88, crouched_half_height_cm=40)

    def test_window_passes_only_crouched_and_needs_a_climb(self):
        window = dict(name='upper window', center=[0, 0, 105 + 83.75], normal=[1, 0, 0],
                      width_cm=91, height_cm=167.5, depth_cm=27, floor_z=0)
        (row,) = c.passable([window], **self.HERO)
        self.assertEqual(row['status'], 'crouch_only')
        self.assertEqual(row['passes'], 'crouched')
        self.assertAlmostEqual(row['standing']['margin_cm']['width_cm'], 23)
        self.assertAlmostEqual(row['standing']['margin_cm']['height_cm'], -8.5)
        self.assertAlmostEqual(row['crouched']['margin_cm']['height_cm'], 87.5)
        self.assertAlmostEqual(row['sill_rise_cm'], 105)
        self.assertTrue(row['needs_climb'])

    def test_required_crouch_passes(self):
        window = dict(name='w', center=[0, 0, 100], normal=[0, 1, 0], width_cm=91, height_cm=167.5,
                      required='crouched')
        self.assertEqual(c.passable([window], **self.HERO)[0]['status'], 'passable')

    def test_tight_door_fails_both_postures(self):
        door = dict(name='closet', center=[0, 0, 100], normal=[0, 1, 0], width_cm=60, height_cm=200)
        (row,) = c.passable([door], **self.HERO)
        self.assertEqual((row['status'], row['passes']), ('not_passable', None))
        self.assertAlmostEqual(row['standing']['margin_cm']['width_cm'], -8)

    def test_ordinary_door_passes_standing(self):
        door = dict(name='front', center=[0, 0, 100], normal=[-1, 0, 0], width_cm=90, height_cm=200,
                    floor_z=0)
        (row,) = c.passable([door], 34, 88)
        self.assertEqual((row['status'], row['passes'], row['needs_climb']), ('passable', 'standing', False))
        self.assertNotIn('crouched', row)

    def test_tolerance_and_obstruction(self):
        door = dict(name='d', center=[0, 0, 100], normal=[1, 0, 0], width_cm=67.7, height_cm=200)
        self.assertEqual(c.passable([door], 34, 88)[0]['status'], 'not_passable')
        self.assertEqual(c.passable([door], 34, 88, tolerance_cm=0.5)[0]['status'], 'passable')
        blocked = c.passable([door], 34, 88, tolerance_cm=0.5,
                             obstructed={'d': {'standing': [dict(part='Glass', volume_cm3=900)]}})
        self.assertEqual(blocked[0]['status'], 'not_passable')
        self.assertEqual(blocked[0]['standing']['obstructed_by'][0]['part'], 'Glass')

    def test_box_opening_runs_through_the_wall_thickness(self):
        frame = c.opening_frame(dict(name='b', low=[0, 0, 10], high=[20, 100, 220]))
        self.assertEqual((frame['yaw_deg'], frame['width_cm'], frame['height_cm'], frame['depth_cm'],
                          frame['sill_z_cm']), (0., 100, 210, 20, 10))
        turned = c.opening_frame(dict(name='t', low=[0, 0, 0], high=[100, 20, 200]))
        self.assertEqual((turned['yaw_deg'], turned['width_cm']), (90., 100))
        box = c.capsule_box(turned, 34, 88)
        self.assertEqual(box['size'], [88, 68, 176])
        for got, want in zip(box['low'] + box['high'], [16, -34, 0, 84, 54, 176]):
            self.assertAlmostEqual(got, want)

    def test_allowance_and_errors(self):
        door = dict(name='closet', center=[0, 0, 100], normal=[0, 1, 0], width_cm=60, height_cm=200)
        row = c.passable([door], 34, 88, allowed=[dict(a='closet', reason='Not meant to be entered')])[0]
        self.assertEqual(row['accepted_reason'], 'Not meant to be entered')
        with self.assertRaises(ValueError):
            c.passable([dict(door, normal=[0, 1, 1])], 34, 88)  # a hatch, not a wall opening
        with self.assertRaises(ValueError):
            c.passable([door], 34, 20)  # half height below radius
        with self.assertRaises(ValueError):
            c.passable([dict(door, required='crouched')], 34, 88)  # no crouch given


class OccludedTests(unittest.TestCase):
    # The garage case: a 5 x 2.2 m trim panel 2.5 cm in front of the door, seen from -X.
    DOOR = part('GarageDoor', (0, 0, 0), (5, 500, 220))
    PANEL = part('TrimPanel', (-7.5, 0, 0), (-2.5, 500, 220))

    def test_trim_panel_hides_the_garage_door(self):
        rows = c.occluded([self.DOOR, self.PANEL], [1, 0, 0])
        self.assertEqual([(r['a'], r['b']) for r in rows], [('GarageDoor', 'TrimPanel')])
        self.assertAlmostEqual(rows[0]['gap_cm'], 2.5)
        self.assertEqual((rows[0]['covered_fraction'], rows[0]['view']), (1., '+X'))
        self.assertEqual(rows[0]['face_min_cm'][0], 0)

    def test_from_the_other_side_the_door_hides_the_panel(self):
        rows = c.occluded([self.DOOR, self.PANEL], [-1, 0, 0])
        self.assertEqual([(r['a'], r['b']) for r in rows], [('TrimPanel', 'GarageDoor')])

    def test_partial_cover_and_tolerance(self):
        half = part('Header', (-7.5, 0, 110), (-2.5, 500, 220))
        self.assertEqual(c.occluded([self.DOOR, half], [1, 0, 0]), [])
        rows = c.occluded([self.DOOR, half], [1, 0, 0], tolerance_cm=0, min_covered_fraction=.5)
        self.assertAlmostEqual(rows[0]['covered_fraction'], .5)
        narrow = part('Panel', (-7.5, .3, .3), (-2.5, 499.7, 219.7))
        self.assertEqual(c.occluded([self.DOOR, narrow], [1, 0, 0], tolerance_cm=0), [])
        self.assertEqual(len(c.occluded([self.DOOR, narrow], [1, 0, 0], tolerance_cm=.5)), 1)

    def test_far_or_enclosing_parts_do_not_count(self):
        car = part('Car', (-400, 0, 0), (-100, 500, 220))
        wall = part('Wall', (-10, -100, 0), (20, 600, 300))  # the door sits inside its thickness
        self.assertEqual(c.occluded([self.DOOR, car, wall], [1, 0, 0]), [])

    def test_selection_allowance_and_axis(self):
        unselected = [dict(self.DOOR, selected=False), dict(self.PANEL, selected=False)]
        self.assertEqual(c.occluded(unselected, [1, 0, 0]), [])
        rows = c.occluded([self.DOOR, self.PANEL], [1, 0, 0], allowed=[dict(
            a='GarageDoor', b='TrimPanel', reason='Closed garage, door never seen')])
        self.assertEqual(rows[0]['accepted_reason'], 'Closed garage, door never seen')
        with self.assertRaises(ValueError):
            c.occluded([self.DOOR, self.PANEL], [1, 1, 0])


class UnmountedTests(unittest.TestCase):
    # The fascia's front is the plane x = 0; it lies on the letters' +X side.
    FASCIA = part('Fascia', (0, 0, 300), (30, 400, 360), label='HQ_Fascia')

    def letters(self, front, back):
        return [part('Letter%d' % i, (front, 40*i, 310), (back, 40*i + 40, 350), label='HQ_Lettering')
                for i in range(3)]

    def test_lettering_standing_off_its_fascia(self):
        rows = c.unmounted([self.FASCIA] + self.letters(-5, -3), pattern='*lettering*', facing=[1, 0, 0])
        self.assertEqual([r['a'] for r in rows], ['Letter0', 'Letter1', 'Letter2'])
        self.assertEqual({(r['b'], r['gap_cm'], r['side']) for r in rows}, {('Fascia', 3, '+X')})

    def test_lettering_on_its_fascia(self):
        self.assertEqual(c.unmounted([self.FASCIA] + self.letters(-2, 0), pattern='*Lettering*',
                                     facing=[1, 0, 0]), [])

    def test_letters_touching_each_other_are_not_a_mount(self):
        parts = [self.FASCIA] + self.letters(-5, -3)
        self.assertEqual(len(c.unmounted(parts, pattern='*lettering*')), 3)
        self.assertEqual(c.unmounted(parts, pattern='*lettering*', count_mounted_as_support=True), [])

    def test_picture_on_no_wall(self):
        floor = part('Floor', (-500, -500, -10), (500, 500, 0))
        picture = part('Picture', (100, 100, 140), (103, 160, 190))
        (row,) = c.unmounted([floor, picture], mounted=['Picture'])
        self.assertEqual((row['b'], row['gap_cm'], row['side']), ('Floor', 140, 'any'))
        (row,) = c.unmounted([floor, picture], mounted=['Picture'], facing=[-1, 0, 0])
        self.assertEqual((row['b'], row['gap_cm']), (None, None))

    def test_wall_on_the_wrong_side_does_not_count(self):
        wall = part('Wall', (-20, 0, 0), (0, 400, 300))  # on the picture's -X side
        picture = part('Picture', (0, 100, 140), (3, 160, 190))
        self.assertEqual(c.unmounted([wall, picture], mounted=['Picture'], facing=[-1, 0, 0]), [])
        (row,) = c.unmounted([wall, picture], mounted=['Picture'], facing=[1, 0, 0])
        self.assertIsNone(row['b'])

    def test_allowance_and_naming_nothing(self):
        rows = c.unmounted([self.FASCIA] + self.letters(-5, -3), mounted=['Letter0'], facing=[1, 0, 0],
                           allowed=[dict(a='Letter0', reason='Free-standing sample')])
        self.assertEqual(rows[0]['accepted_reason'], 'Free-standing sample')
        with self.assertRaises(ValueError):
            c.unmounted([self.FASCIA], pattern='*sign*')
        with self.assertRaises(ValueError):
            c.unmounted([self.FASCIA], mounted=['Missing'])


if __name__ == '__main__':
    unittest.main()
