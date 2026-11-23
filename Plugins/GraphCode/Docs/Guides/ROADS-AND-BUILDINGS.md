# Plan connected roads, plots and entrances with World

A road, a building and the ground between them are one authoring problem. Plan
their shared interfaces before multiplying meshes. Use this with
[ENVIRONMENTS.md](ENVIRONMENTS.md), [ASSET-QUALITY.md](ASSET-QUALITY.md) and
[ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md). Treat the following checks as part
of the first delivery, not repairs that require a separate user prompt.

## Start with a route and its destinations

Read the existing level and its `.gcw`; preserve established streets and landmarks.
Name the main destinations, entrances, intersections, service access, pedestrian
routes and intended dead ends. Decide which actor/vehicle uses each route and the
camera from which it matters. Choose a hierarchy appropriate to the setting:
main street, local street, alley, courtyard, path, loading access. A network of
equally wide strips gives neither useful circulation nor convincing urban form.

Reserve the full corridor, including carriageway, gutters/curbs, walking space,
frontage, drainage and any retaining structures. Buildings need footprints,
setbacks, entrance orientation and service clearance, not only center positions.
Place building frontage against the correct side of that corridor. Do not point a
door at the road centerline and assume the intervening space will resolve itself.

Keep the chosen widths, heights and connection names in one project plan/source of
dimensions. Derive related meshes and placements through supported references,
profiles, parameters and named anchors. Avoid separately copying the same curb
height into the road, sidewalk, stair, doorway and test. Use `gc_describe` for the
installed grammar; these planning concepts are not invented `road()` commands.

## Solve the plan view and the height profile together

Use a directed course as the common reference for a road and its edges. `offset=`
follows its bends in plan; positive values are to the right when travelling along
the course, negative values to the left. For example:

```gcw
populate {
  RightEdge = spline(along=$Block.street, offset=350)
  LeftEdge = spline(along=$Block.street, offset=-350)
}
```

These declare native spline paths for a consuming road/curb workflow; they do not
make a road mesh by themselves. On a clockwise circle viewed from above, positive
`offset` goes inward. Reversing the course swaps its sides. `right=` has a separate
meaning: shift the whole path sideways using its start-to-end direction. Use it
when the whole shape must move; use `offset=` for an edge following the bends.
`up=` raises the result in world Z. All distances are centimetres.

Sharp corners receive miter joins. If a bend is too tight and its offset folds,
collapses or crosses itself at the same elevation, GraphCode refuses it: widen
the bend, reduce the distance or author the junction explicitly. Check both edges
and their closing seams. Curve offset supplies paths, not junction meshing,
banking, crossfall, drainage, collision continuity or building entrance access.
For a smooth closed course, match the arriving and leaving tangents at its seam;
repeating the starting position alone closes the position, not the direction.
A cusp or a clamped bend with a nearly vanishing tangent may also refuse the
sampling tolerance. Use a smoother auto curve or explicit nonzero tangents there;
do not hide that refusal by accepting an unmeasured approximation.

For each segment, record its endpoints, direction/tangents, width and section,
elevations, intended slope changes and junction ownership. A smooth plan curve can
still have a vertical kink. Inspect the center and both edges: crossfall/banking
can make the far edge fail while the centerline meets perfectly.

For each plot, relate the finished road surface, gutter, curb top, sidewalk,
entrance landing, door threshold and interior floor. State whether a difference
is handled by a graded approach, ramp, stairs, retaining wall or deliberately
raised platform. Keep building walls/floors upright and coherent; terrain conformance
is not a reason to tilt the entire building or bury its door.

Grade and crossfall should explain where water would go. Reserve drains/outlets
and avoid accidental low pockets against entrances. This is visual and gameplay
planning; a terrain smoothing command does not simulate drainage. Use scene scale,
reference, pawn step/slope limits and the actual vehicle's swept envelope to choose
dimensions. Do not import one numerical street standard into every game setting.

**Illustrative example, not a built fixture:** a 20 m street rises from Z=0 to
Z=100 cm. At its midpoint a linear center profile is Z=50 cm. Suppose the section
places the sidewalk at Z=65 cm and a shop threshold at Z=80 cm. There is a real
15 cm transition to design at the shop: a chosen step/landing or a graded approach
with enough run, tested with the intended pawn. Raising the entire sidewalk by
15 cm fixes that door while breaking the shared curb. Derive the local transition
from the two agreed interface elevations and preserve the through walking route.

## Give each junction one surface owner

Do not cross two complete road slabs and leave both top faces in the intersection.
Use a junction piece or a constructed junction surface, with approaches ending at
its defined boundaries. Match endpoint position, elevation, width, tangent and
cross-section orientation; trim/transition the full section, including sidewalks,
curbs and markings. Rebuild the approach if its topology cannot meet the junction.

Check tight corners for inward folding, stretched paving, abrupt width changes
and pinched sidewalks. Place crossing routes and curb transitions where people
actually go. A bridge and a road below can cross in plan without connecting:
declare the elevation/clearance difference and keep their terrain edits separate.
Overpasses need actual structure; a heightfield cannot represent both decks.

Epic's [Landscape Splines documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-splines-in-unreal-engine?application_version=5.7)
describes native spline mesh axes, width/falloff, tangents, collision and terrain
fitting. Native support and GraphCode's exposed authoring are separate checks.
Also distinguish native half-width from a GCW terrain course's full corridor
width; inspect the installed operation rather than transferring numbers by name.
A GCW course highlight is a declaration overlay, not proof of the road's final
height or a finished road mesh. See [scene inspection](SCENE-INSPECTION.md).

## Build the complete street-to-door connection

Trace the route in section and on foot:

`road / crossing → curb transition → sidewalk → frontage / landing → threshold → interior`

Keep a continuous clear walking strip. Door swing, steps, columns, planters, pipes
and signs need their own space; do not make pedestrians weave through them.
[NACTO's sidewalk design reference](https://nacto.org/publication/urban-street-design-guide/street-design-elements/sidewalks/sidewalk-design/)
distinguishes frontage from the pedestrian through zone; adapt that spatial logic
to the game's setting and character scale.

Fit both ends of stairs/ramps: the top must meet the threshold and the bottom must
meet the actual sidewalk. Check nosings, side returns, headroom, railing/support
and door motion. For behavior and collision use [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md).
A plausible render can still conceal a collision lip or inaccessible entrance.

Seat foundations into a planned plot/platform; provide retaining or infill where
terrain falls away. Match visible road/terrain edges without leaving a buried
road, a floating shoulder or two almost coincident skins. Cut/fill or sculpt the
supporting ground as appropriate. A blend material or RVT can soften appearance,
but cannot repair a physical hole, collision step or unsupported platform.

Protect roads, door approaches, drainage and sightlines from foliage/PCG placement.
Inspect the exclusions after regeneration and after moving an entrance. Keep
terrain edits, plot leveling and repeated generation from undoing finished joins.

## Prove one difficult street section before repetition

Choose a representative section with a bend or junction, changing elevation and
an entrance. Finish its road surface, curbs, sidewalk, landing, foundation and
materials before extending the district. Use the reusable
[assembly checker](ASSEMBLY-VALIDATION.md#run-the-reusable-editor-check) on supported
placed static meshes; name the road/landing interfaces and sample the full seams.
Inspect spline-deformed roads and Landscape through their supported native paths
and direct visual/collision checks; do not pass their undeformed source to a checker.

| Symptom | Check first | Repair the owning relationship |
|---|---|---|
| Flickering junction or paving strip | Same-facing surface overlap, including the road under a decorative overlay. | Trim duplicate faces, assign one junction owner or use a proper surface decal. |
| Crack at one end of a join | Full edge heights, widths, tangents and component scales. | Match the shared boundary/cross-section; one-point snapping is insufficient. |
| Road hovering or terrain poking through | Actual road top/bottom versus terrain along both edges. | Fit the support/grade; do not repeatedly move the whole road up. |
| Door floats or is buried | Threshold, landing, plot datum and foundation. | Rebuild the local approach/plot interface from those heights. |
| Pawn catches despite a clean image | Actual collision seam, step height, headroom and chosen navigation agent. | Repair collision and physical dimensions, then rebuild and traverse. |
| Vehicle clips on a bend/crest | Swept width, overhang, turning radius, clearance and grade transition. | Give the route the required envelope and vertical transition. |
| Foliage blocks a repaired entrance | PCG/exclusion regions after regeneration. | Update the shared route/entrance exclusion, not individual random plants. |

Capture an overhead network view, a long view along the street, sections at height
changes, and close grazing views of every interface. Then walk from the road into
each intended entrance using the actual pawn; drive the representative vehicle
where applicable. Move the camera to reveal shimmer. Rebuild navigation for the
correct agent and verify both endpoints and the intervening route. A connected
navigation graph is not proof of good visible joins, and a clean image is not a
navigation test. Recheck loaded/streamed boundaries and HLOD/LOD transitions in the
target setup. Save, reload and repeat the critical traversal before delivery.

Keep the connection table and captures current after terrain, height, width,
Blueprint, collision or mesh changes. Reuse the proven interface as a parameterized
pattern while preserving deliberate street and building variation.
