# Check the connections between meshes

A clean Shape build proves neither a good Blueprint assembly nor a good level.
The Blueprint owns reusable component relationships; the placed actors supply the
final transforms and neighbors. Inspect that realized geometry after construction
and after any relevant pose or instance override. Follow [ASSET-QUALITY.md](ASSET-QUALITY.md)
and [road/building integration](ROADS-AND-BUILDINGS.md) for the visible result.

## Declare what a connection is meant to do

For each connection family, name the two parts, their mounting faces or sockets,
the intended contact region, permitted gap/penetration and physical reason. Use
actual dimensions from the design. A socket is an attachment frame, not proof that
surfaces meet. The root of an Actor is not necessarily the part that supports it.

Use these as separate checks:

| Question | Evidence | Insufficient evidence |
|---|---|---|
| Do the intended faces meet? | Gap across the mating region, including corners and endpoints, in the actual pose. | Matching pivots, sockets or one closest point elsewhere on the mesh. |
| Does solid geometry collide where it should not? | Triangle/solid comparison, with measured intentional seats kept visible. | Overlapping bounds, a physics overlap event or a clean topology count per asset. |
| Could exposed surfaces flicker? | Positive-area, nearly coplanar overlap; inspect camera motion and actual materials. | Only checking volume, intersections or the world Z coordinate. |
| Is every piece supported? | A geometric contact chain from an explicit support, including disconnected islands inside components. | Being a child component, or merely intersecting an unrelated neighbor. |
| Does the assembly still work when it moves? | Repeated checks through rest, extremes and intermediate poses, plus playback. | A single rest pose or a collision proxy much simpler than the visible mesh. |

Opposing faces can make a legitimate butt joint. Two same-facing skins competing
for the same visible surface are different. Z-fighting can occur on a wall, curved
fitting or road in any orientation. A small deliberate offset can help an overlay,
but no fixed 0.1 cm gap guarantees every distance, material and camera. Prefer one
owned surface, a real recess/step, a decal for surface-only detail, or a clean weld
when those match the construction. Do not create daylight under every contacting part.

## Run the reusable editor check

The optional helper ships at `GraphCode/Tools/Inspection/`. It needs Unreal's
**Python Editor Script Plugin** and **Geometry Script**. The current memory guard
targets Windows. It reads static source meshes at their real component/instance
world transforms, including child actors, and writes JSON/HTML under
`Saved/GraphCode/Inspection`. It changes no actors, assets, selection or collision.

The call below uses the installed plugin automatically. To reuse the checker in a
separate project without that installation, copy the **whole `Tools/Inspection`
folder** and point `helper` at that copy; its six Python modules have no
Steampunk, OraStudio asset or machine-path dependency.

Select a bounded assembly or street section in the level. Include the receiving
road, sidewalk, buildings and relevant neighbors in that selection. Paste this
into Unreal's Python console, or execute it through the installed script tool
after reading that tool's help:

```python
from pathlib import Path
import sys, unreal
helper = Path(unreal.PluginBlueprintLibrary.get_plugin_base_dir("GraphCode")) / "Tools" / "Inspection"
assert helper.is_dir(), "Locate the installed GraphCode Tools/Inspection directory."
if str(helper) not in sys.path:
    sys.path.insert(0, str(helper))
import gc_check_assembly
report = gc_check_assembly.scan()
print(report["status"], report["report_path"])
```

The first report inventories exact component paths and instance IDs in `parts`.
It checks selected mesh pairs for solid penetration and possible coplanar overlap,
and uses named parent-socket attachments as expected proximity checks. It does not
infer that every nearby part ought to touch. Unselected actors and unloaded cells
are outside its scope. Supply `neighbors=[actor, ...]` for explicit extra actors.

Use IDs from that report to configure the assembly's intent; the following is a
template whose names and tolerances the author must replace with measured/design
values. These are checker arguments, not new `.gc` or `.gcw` grammar:

```python
report = gc_check_assembly.scan(
    connections=[dict(a=child_id, b=receiver_id, max_gap_cm=mount_tolerance,
                      reason="Flange seats on its mounting face")],
    support_roots=[base_component_id],
    seam_probes=[dict(a=road_id, b=landing_id, points_world_cm=measured_seam_points,
                     max_gap_cm=seam_tolerance)],
    report_name="entrance-connection")
```

Take seam points from the intended joining edge in the final world frame, using
[scene inspection](SCENE-INSPECTION.md) or the authored interface transformed into
world space. Include both ends, corners, height changes and enough intervening
points to reveal bowing. A probe measures the distance from that intended point to
**each** surface. It only certifies those samples; a seam can fail between them.
Moving the sample points to some unrelated nearest surface would test the wrong joint.

For intentional solid insertion, `allowed_penetrations` takes exact `a`, `b`,
`max_volume_cm3` and `reason` fields. The measured result stays in the report; a
larger penetration still fails. An accepted insertion does not suppress a
coplanar-overlap finding. Never exempt every pair within one Actor or socket family.

The default proximity (0.25 cm), coincidence (0.01 cm) and volume (0.1 cm3)
tolerances are diagnostic settings, not construction specifications. Set them for
the interface and camera, retaining the chosen values with the report. Memory,
part, triangle, pair and time guards bound the work; one native Boolean cannot be
interrupted mid-call. Inspect small sections before attempting a detailed district.

## Check clearances, openings, hidden faces and mounts

Four more checks sit beside `scan` in `gc_check_assembly`. Each runs alone, reads
the same selection and `neighbors`, and writes a report in the shape `scan` writes,
so findings from all five can share one list. Each has a tolerance and an allow
list that needs exact names and a reason, like `allowed_penetrations`. These four
read **bounds**, not triangles: a turned part's box is larger than the part, and a
wall around a window reads as solid. Treat a finding as the place to look, then
confirm it with `scan` or a render. Directions are world axes; distances are cm.

### Keep doors, openings and stairs clear

`swept_volumes` reports every part whose bounds enter a volume you state. You state
each door, opening and stair; the checker never guesses them from meshes. A door is
its hinge at the floor, the leaf width, the height and the yaw range from closed to
open. A stair is one box or a list of boxes. `depth_cm` is how far the part must move
to clear the volume; a door finding also says how many degrees the door opens
before the leaf meets the part (`opened_deg`).

```python
report = gc_check_assembly.swept_volumes(
    volumes=[dict(name="hall door", hinge=[410, 220, 0], leaf_cm=90, height_cm=210,
                  from_deg=0, to_deg=90, reason="The hall door opens flat to the wall"),
             dict(name="stair", boxes=stair_flight_boxes)],
    tolerance_cm=0.5,
    allowed_intrusions=[dict(a=leaf_id, b="hall door", max_depth_cm=95,
                             reason="The leaf sweeps its own swing")])
```

This finds the picture hung on the wall the door opens against, the chair pushed
into the table legs' clearance box and the fireplace placed into the stair. Leaf
thickness is not added: widen `leaf_cm` or the tolerance.

### Pass a character through an opening

`passable` says whether an upright capsule passes each opening you state, standing
and optionally crouched, with the margin per axis (`margin_cm`: positive is room to
spare, negative is the shortfall). Give the capsule as Unreal does: the half height
includes the rounded ends. The default Character is radius 34 and half height 88,
crouched 40, and steps up 45. An opening is a box, or a rectangle with its center,
a horizontal normal, width and height. `floor_z` adds the step up to the sill.

```python
report = gc_check_assembly.passable(
    openings=[dict(name="upstairs window", center=[1260, 40, 388.75], normal=[1, 0, 0],
                   width_cm=91, height_cm=167.5, depth_cm=27, floor_z=200,
                   required="crouched")],
    radius_cm=34, half_height_cm=88, crouched_half_height_cm=40)
```

That window fails standing by 8.5 cm, passes crouched and reports `needs_climb`
because its sill is 105 cm up. State every door and hall opening of a house as a
box and run them with the hero's own capsule before promising that the hero can go in.

Without `test_meshes` nothing in the level is read and no selection is needed. With
`test_meshes=True` the checker also measures solid overlap between each selected or
neighbor mesh and the capsule's box through the opening, so a pane, a mullion or a jamb in the
way fails that posture. That reads visible meshes, not collision: a simple collision
box filling the window is not seen. The checker has no collision query; walk the
opening in Play to prove collision.

### Find a part hidden behind another

`occluded` reports a part whose face toward the viewer is covered by another part's
bounds lying just in front of it. Give the view direction along a world axis; a
cover counts when it is no more than `max_gap_cm` in front of the face.

```python
report = gc_check_assembly.occluded(view_direction=[1, 0, 0], tolerance_cm=0.5,
                                    max_gap_cm=25, min_covered_fraction=1.0)
```

This finds the garage door behind a solid trim panel 2.5 cm in front of it, and
reports the gap and the covered fraction. Lower `min_covered_fraction` to find a
notice half hidden behind a stair. A cover with a hole in it reads as solid, so look
at every finding before you move anything.

### Find a wall-mounted part that touches nothing

`unmounted` takes the parts you name as wall-mounted, by ID or by a name pattern,
and reports each one whose bounds touch no other part. With `facing` (the direction
from the part toward its wall) only a part behind it counts. Other named parts do not
count as a mount, so letters touching each other still read as floating.

```python
report = gc_check_assembly.unmounted(pattern="*Lettering*", facing=[1, 0, 0],
                                     tolerance_cm=0.5)
```

This finds lettering standing 3 cm off its fascia and a picture on no wall, and
names the nearest part and the gap. Touching bounds is not a fixing; use
`scan(support_roots=...)` to measure the meshes.

## Read the result without claiming more than it measured

- `findings`: inspect each named pair. `possible_z_fighting` is a same-facing
  triangle-overlap candidate; the helper deliberately includes hidden/internal
  faces and does not evaluate materials or visibility. A reported area can include
  duplicate source faces and is not a screen-space pixel count.
- `incomplete`: some work was skipped, unsupported, failed or exceeded a budget.
  Read `skipped`; reduce the scope or fix the cause and rerun. Never treat the
  absence of later findings as a pass.
- `no_findings`: only the requested, completed checks found no issue. Read `coverage`:
  no support roots means support is unchecked; no seam probes means no seam-wide
  check was performed. A connection's whole-mesh minimum gap only establishes proximity.

Support uses triangle intersection or sampled vertex-to-surface proximity and
explicit root assumptions. It can miss edge-only near contacts or find incidental
contact in the wrong place. Inspect the contact chain and actual purpose; it is not
a structural/load-bearing solver. The checker distinguishes enclosed cavity
boundaries from loose solid islands, but malformed/open geometry remains a limit.

Static source meshes do not include spline deformation, skeletal animation or
material displacement. Landscape is not a static-mesh substitute. Unsupported
collected geometry is reported; unselected terrain/material effects are outside
the captured scope. Check the actual collision and route separately. Repeat for
moving poses; a set of samples does not certify continuous motion between them.

### What GraphCode already checks automatically

Shape's build diagnostics cover its own output, including topology and candidate
coincident shells. Blueprint `gc_apply` also compares construction-script static
mesh templates for coincidence. That class-space check currently omits named
socket placement, native components and inherited overrides; it does not inspect
the placed level. The optional helper above reads actual placed static components
to cover those assembly placements. Neither check replaces visual review.

## Repair the authored cause and rerun

Show the offending pair and surrounding support in a close-up; use isolation or a
section only as a diagnostic, then restore and render the complete assembly.
Repair the owning recipe, socket/interface, Blueprint relationship or World
placement. Do not silently nudge children until the warning disappears, break a
mounting relationship, or hide a gap behind decorative trim without real backing.

Keep an inspection table with pair, expected relationship, measured finding,
pose/revision, repair and latest capture. Recheck affected neighbors after edits,
then run the game camera through the result. The AI should execute this loop before
delivery, without waiting for the user to point out each floating pipe or entrance.
