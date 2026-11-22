# Measure a scene join and use it in Shape

For the full connection workflow and a paste-ready editor checker, read
[ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md). For roads, pair these measurements with
[road and building planning](ROADS-AND-BUILDINGS.md); an isolated hit cannot certify a full seam.

Render the loaded level, then use the returned capture ID to ask about pixels in
that exact image. Mesh inspection reads static mesh triangles, including meshes
with no collision. Use `mode:"legend"` first when you need to identify the actor.

```json
{
  "mode": "ask",
  "capture": "<capture returned by render>",
  "mesh": {"actor": "ExistingRoad", "component": "StaticMeshComponent0"},
  "relative_to": {"actor": "Connector"},
  "points": [{"name": "join", "pixel": [512, 380]}]
}
```

`mesh` identifies the object being measured. `relative_to` identifies the mesh
whose local coordinates you want to author in; omit it for the measured mesh's
own coordinates. An actor with several mesh components requires `component`.
An instanced component with several instances requires `instance`, starting at 0.
An explicitly selected mesh is queried even behind another object.

Each hit includes numeric world and mesh-local positions/normals, the triangle's
three local corners, and a `shape` answer in the destination mesh's local frame.
All distances are centimetres. The `shape.at` and `shape.plane` strings are
full-precision tuples that Shape already accepts:

```gcshape
cut(at=(...), plane=(...))
```

Keep the frame on the output's build line:

```gcshape
build {
  SM_Connector = $Connector [pivot=(0,0,0)]
}
```

This is also returned as `shape.build`. Shape's default `pivot=base` recentres
the cut result, which would shift the seam after placing it back in the scene.

Paste the returned tuples in place of the ellipses. The plane normal describes
the hit surface, so it is appropriate for a planar mating face. A road's top
surface normal alone does not define its end cut. Measure the end face, or derive
the intended plane from sufficient boundary points. Shape keeps the negative
side of `plane`; negate it to keep the other side. Curved joins need a curve or
cutter surface instead of one plane.

The Shape operation must run in the output mesh's local frame, before scene
placement. A recipe stage before a deformation, an individual part inside an
assembly, or a build with a different pivot can use a different frame. Do not
paste final mesh coordinates into those earlier frames without converting them.
`relative_to` handles scene component/instance placement, including rotation and
unequal scale; it does not undo a recipe's deformations.

`snap:"vertex"` chooses the nearest corner of the hit triangle. `snap:"edge"`
chooses its nearest edge point. Both measure distance in world space. An edge may
be an internal triangulation diagonal; this option does not claim to find the
mesh's boundary. `surface_position` and `barycentric` retain the original hit.

Nanite queries read full-detail editor source instead of a simplified fallback.
Other static meshes use LOD0. The answer labels which geometry was measured.
Material displacement and spline mesh deformation are not measured; deformed
spline components are refused explicitly. Old captures and captures whose mesh
geometry, visibility or placement changed must be rendered again.

## See the GCW declarations

Use the existing `highlights` option, either for the whole render or per view:

```json
{
  "asset": "/Game/Maps/RoadJunction",
  "kind": "scene",
  "highlights": ["regions", "splines"]
}
```

`regions` shows authored GCW boundaries at their declared minimum and maximum
heights. A region without both bounds shows its plan at `anchorZ`, without an
invented floor or ceiling. `splines` (also `courses`) shows authored GCW courses
at their region's `anchorZ`. Curves use the GCW solver's tessellation. Both read
the adjacent `.gcw`, draw through scene geometry, and return declaration names
and colours in the highlight legend. Omit the types to hide them.

These options inspect the scene; they do not move actors or cut meshes. Apply the
resulting Shape program through `gc_apply`, then render again and measure the
whole joining edge to check for gaps and overlap.

For an existing connector whose recipe uses a different frame, start a new Shape
output from its built mesh. This keeps the input exactly in the inspected frame:

```gcshape
gcshape 1 into:/Game/Roads
build {
  SM_ConnectorCut = $Connector [pivot=(0,0,0)]
}
Connector = mesh() {
  add(mesh('/Game/Roads/SM_Connector'))
  # Insert cut(at=<shape.at>, plane=<shape.plane>) here.
}
```

Use a separate output asset so the recipe does not read its own previous output.
