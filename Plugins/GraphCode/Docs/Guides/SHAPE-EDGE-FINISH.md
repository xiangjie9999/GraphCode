# Edge finish: rounded, chamfered or crisp, by material

## What this guide is for

Every edge of a part is either softened or kept crisp, and the right choice depends on the material, how the
part is used and how far away the camera is. This guide helps you:

1. choose the finish for each kind of part;
2. write it with `round` and the edge selectors;
3. keep a crisp line where you want one, including a line inside a surface;
4. do the work in an order that the finish survives.

Use it with [ASSET-QUALITY.md](ASSET-QUALITY.md). A table, a building or a sofa should not inherit razor-sharp
primitive edges just because they are easy to generate. Choose the finish while planning the part, then inspect
it in the final assembly. For the full spelling list see [GCSHAPE.md](GCSHAPE.md); for artwork-led surfaces see
[ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md).

## 1. Choose the edge by material, use and viewing distance

| Part | Intended treatment | Preserve |
|---|---|---|
| Timber tabletop, chair rail, cabinet | A small curved edge break or a designed larger round-over where hands meet it; finish the corners as well as the long edges. | Joinery, flat mounting pads and the chosen board thickness. |
| Cast metal housing or building fitting | Rounded exposed transitions with a smaller edge break on machined surfaces. | Bearing seats, flanges, seals, bolt seating and mating datums. |
| Building plinth, masonry trim, stair nosing | Profile the moulding or stone first; soften exposed arrises at a scale appropriate to the reference. | Wall junctions, tread dimensions, thresholds and module joins. |
| Upholstered sofa, armrest or cushion | Broad, soft volumes, rounded corners and shaped cross-sections; add seam/piping and compression where pieces meet. | Contact with the frame and believable gaps between separate cushions. A small bevel on a box does not make upholstery. |
| Sheet metal or thin panel | A thickness-aware edge break, folded hem or rolled lip where construction calls for it. | Actual thickness and fastening surfaces; do not consume the panel with an oversized bevel. |
| Moulded shell, car panel or product body | Smooth surfaces with a few declared crisp lines: a parting line, a chamfer, a shoulder (see [section 4](#4-keep-a-crisp-edge-inside-a-surface)). | The designed lines; do not round them away. |
| Deliberately crisp or faceted part | Retain selected sharp edges or use a deliberate flat chamfer. | The reference's intended silhouette; do not round every edge indiscriminately. |

Choose the radius or inset in physical units from the reference, the material and the local thickness. There is
no useful universal radius for both a sofa and a metal bolt. When you reuse a part, treat edge dimensions as named
recipe inputs and derive repeated parts from them. Do not enlarge a bevel just to make its highlight visible in
one shot.

## 2. Write the round and select the right edges

Ask the installed grammar for `round`, selections and `smooth:` before editing. These are instruction fragments
for a suitable existing mesh, not a complete asset recipe; values are illustrative centimetres.

```gcshape
round(0.3, segments=4) [sharp]
```

This selects sharp edges and requests a curved profile with four segments. Put `segments=` in the argument list,
not the selection brackets. Check whether four segments are enough for the actual camera; it is not a universal
quality setting. For automatic resolution, omit the count:

```gcshape
round(0.3) [sharp]
```

Shape derives the count from `smooth:` and the bevel size, and never so few that two of the round's own facets
meet above `shade:`, so a round shades as one curve. A coarse `smooth:` on a small bevel can still leave its
silhouette visibly polygonal up close (two segments on a small right-angle edge at the defaults). Tighten the
tolerance or request more segments when the camera needs it. A deliberate single flat chamfer is a different choice:

```gcshape
round(0.3, segments=1) [sharp]
```

### Selecting the edges

- Bare `round(0.3)` follows **group boundaries**, not every hard edge. An extruded rectangle can have all four
  walls in one side group, so its vertical corners survive a bare round. Use `[sharp]` when those corners are
  intended targets.
- A face selection such as `[top]` rounds the border of that region. It does not necessarily select every corner or
  crease elsewhere on the part.
- `[sharp]` selects every hard edge: every edge harder than `shade:` and every edge made by a `@sharp` key or a
  declared crease. It can also catch functional mating edges. For those parts, use narrower supported selections or
  named regions, and check what the selection includes.
- An open `[rim]` has only one incident face. Give it the thickness or wall a bevel needs before attempting one;
  changing shading cannot supply that geometry.
- `add(join=fillet, radius=...)` softens a merged junction. It is not a substitute for treating the exposed edges
  of each part, or for attaching separate moving meshes.

Normals, `shade:` and smooth shading change lighting; they do not create a curved silhouette. A normal map can carry
tiny finish detail when the camera permits it, but broad round-overs and cushion curvature need the corresponding
shape.

## 3. Order the work so the finish survives

Establish the primary profile, thickness, cuts and intended joins first. Apply the edge treatment while those edges
are still identifiable, before tiny fasteners, wear and surface noise complicate selection.

- **Add touching pieces into one entry before the round.** Pieces that touch and should read as one surface (lawn
  bands, paving runs, a counter of several slabs) are `add`ed into one entry first: the faces they share disappear
  and the round finds only the outer edge. Rounded one by one, each keeps its own rounded edge and the join reads as
  a groove, like tiles.
- **Revisit edges after a boolean.** A later boolean can create new sharp edges. Round or review them rather than
  assuming one early round finished the asset.
- **Declare creases after a `remesh`.** `remesh` and `subdivide` drop crease marks (section 4).
- **Narrow rounds are reported.** Where a face is narrower than the radius, Shape narrows the round at that place only
  and eases back to the full radius beside it; an outline corner without room takes the widest radius that fits. The
  build note names how many edges or corners narrowed and the tightest place. If the narrowed place is visible and the
  design needs the full round there, widen that face or round before the instruction that made it narrow.
- **Do not round the same region repeatedly without looking.** Check what the previous operation left.

## 4. Keep a crisp edge inside a surface

A round softens an edge that exists. Sometimes you want the opposite: a line that stays crisp in the middle of an
otherwise smooth surface, such as a moulded shell's parting line, a chamfer that runs out, a brow ridge or a tyre
groove. Declare it where the shape is described, and the build puts a hard edge there.

There are two places to declare one.

**On an outline.** `@sharp` on a key of a `curve()` or `points()` outline makes a corner, and the edge swept from
it is hard whatever `shade:` says:

```gcshape
BladeOutline = plane() {
  curve((0, 0)@sharp, (17, 0)@sharp, (21, 1.2)@sharp, (14, -2.6)@(1, 0.15), (0, -3)@sharp, loop=true)
}
Blade = mesh() {
  add($BladeOutline)
  extrude(height=0.25)
}
```

The spine, tip and heel are straight with crisp edges. The belly key has a heading (`@(1, 0.15)`) instead of a mark,
so it stays smooth and sweeps up into the tip. In a loft, a named `@sharp` key gives a hard column along the body.

**On a profile that a `warp` reads.** `@sharp` on a stop of a `profile()` makes that stop a corner. Wherever a
`warp` formula reads such a profile, the build cuts a crisp line at the stop and marks it hard where the surface
visibly turns:

```gcshape
Side = profile(0 = 0, 0.2 = 0.15, 0.35 = 0.4@sharp, 0.7 = 0.9, 1 = 1.2)
Tub = mesh() {
  rect(w=30, h=20)  round(4)  extrude(height=12)  remesh(0.6)
  warp(to=(point.x * (1 + $Side(point.t.z) / 15), point.y * (1 + $Side(point.t.z) / 20), point.z))
}
```

The tub's wall leans out smoothly, then turns crisply at 35% of its height. A `round` cannot draw this kind of line:
it softens edges that exist, and this one is made by the formula. There is nothing to switch on; the declaration is
the opt-in. [GCSHAPE.md](GCSHAPE.md#crisp-lines-inside-a-surface-declared-creases) has the full rules, with a
moulded shell, a stopped chamfer and a brow ridge.

How the two finishes work together:

- A declared crease is an ordinary hard edge afterwards. `[sharp]` selects it, so `round(0.4) [sharp]` rolls it into a
  small radius when you want a crisp line softened a little.
- `shade:` cannot soften a declared crease. A profile with no `@sharp` stop makes no hard edge: softness there is
  the surface's own.
- Where the surface barely turns at a crease (under about a quarter of a degree), the line is cut but not marked, and
  it shades by `shade:` like the rest. The build note lists it. That is how a ridge fades out toward its ends.
- `remesh` and `subdivide` drop the marks. Declare creases after the last `remesh`.
- A line that only touches a corner without crossing it makes no crease. Write a signed argument with the corner
  inside the profile.

## 5. Shape the section, not only the edge

Rounding softens edges; it does not change a flat-sided section. Before reaching for a bigger round, ask the grammar
for these (all centimetres, illustrative):

| Want | Instruction |
|---|---|
| A slight dome across a top face | `displace(by=0.6) [top]` — a smooth mound, border fixed |
| Fuller in the middle, narrower at one end, in width only | `taper(to=profile(0 = 90%, 0.5 = 110%, 1 = 70%), along=X, across=Y)` |
| One section flowing into a different one | `loft(to=$NextOutline)` from the entry's top face |
| A wrap, cap or binding that exactly follows the part | `coat(by=0.3) [in=$Band]` — the surface itself moved out, one skin |
| Soften a bumpy or re-melted area | `relax(0.8) [in=$Region, fade=2]` |
| Round the edges a cut or boolean left | `round(0.4) [cut]` after the cut |
| A bullnose end on a swept bar | `round(<section radius>) [top]` |
| A crisp line inside a smooth surface | `@sharp` on a stop of a profile that a `warp` reads (section 4) |
| Close an imported part's openings | `fill() [topology=adaptive]`, or `[rim, above=85%, topology=adaptive]` for one of them |

Rounding an end that must meet another mesh can open a seam. Keep mating faces, module ends and sockets consistent
with the actual contact geometry. If a visible joint intentionally has two rounded edges, model its seam, backing or
reveal as part of the design. Recheck it through [ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md).

## 6. Finish with evidence, before repeating the part

Render the representative part close up in neutral and final materials, under grazing light, then at the game camera
distance. Check all corner types, the silhouette, continuous highlights, UV and material transitions, and protected
contacts. Check topology findings and triangle cost after the operation and after baking or LOD changes. Finally
inspect the assembled result: an isolated rounded part can still leave a hole against its neighbour. Reuse it only
after that connection passes.

Record the selected edge family, treatment and dimension in the part plan, so that every table, building trim and
sofa gets its edges softened without a separate request.

## Common mistakes

- **Rounding a box with a bare `round(r)` and expecting every corner.** A bare round follows group boundaries. Use
  `[sharp]` for the corners.
- **One round for every material.** A radius that suits a sofa ruins a bolt. Take it from the reference.
- **Rounding each slab of a counter separately.** Each keeps its own rounded edge and the join reads as a groove. Add
  them into one entry first.
- **Beveling a thin panel past its thickness.** Keep the real thickness and the fastening surfaces.
- **Rounding before a boolean.** The cut leaves new sharp edges. Round after it with `round(r) [cut]`.
- **Using shading to fake an edge.** `shade:` and normals change lighting, not the silhouette.
- **Declaring a crease, then `remesh`ing.** The mark is dropped. Declare after the last `remesh`.
- **Expecting `abs(...)` to make a crease.** Declare the corner as a `@sharp` stop and read it with a signed argument.
- **Enlarging a bevel to make one highlight show.** Judge at the real camera distance.
- **Bevelling an open rim.** Give it a wall first.

## Reference

| Spelling | What it does |
|---|---|
| `round(r) [sharp]` | rounds every hard edge, including marked creases |
| `round(r)` | rounds group boundaries only |
| `round(r) [top]` | rounds the border of that face region |
| `round(r) [cut]` | rounds the edges a cut or boolean left |
| `round(r, segments=n)` | `n` curved segments; `segments=1` is a flat chamfer |
| `add($B, join=fillet, radius=r)` | softens a merged junction |
| `curve(…, K: (x, y)@sharp, …)` | a corner in an outline; a hard swept edge |
| `v@sharp` on a `profile()` stop | a declared crease that a `warp` reading it cuts and marks |
| `v@(dx, dy)` on a stop or key | a heading through it; smooth, never a corner |
| `displace(by=…) [top]` | a smooth dome across a face |
| `coat(by=…) [in=$Band]` | a skin that follows the part |
