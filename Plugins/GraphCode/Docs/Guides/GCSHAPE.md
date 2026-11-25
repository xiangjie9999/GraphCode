# Writing a `.gcshape`: spellings, crisp lines, lofts, joins

## What this guide is for

A `.gcshape` file describes a mesh as a short program: outlines, extrusions, lofts, warps, cuts and joins.
This guide is the first page to read before you write one. It does four jobs:

1. It tells you what to read for the kind of shape you are making.
2. It lists the one-line spelling for most things you will want (the lookup table).
3. It explains the features that matter most for car, product and hard-surface bodies: crisp lines inside a
   surface, lofts that follow blueprint lines, joins that hold where shells cross, and builds that repeat exactly.
4. It ends with common mistakes and a reference table.

Related guides: [SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md) (rounding and crisp edges by material) and
[ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md) (SVG outlines, texture brushes, render-and-brush work).
If the tool's help and this guide disagree, the help is right: ask it with
`gc_describe kind:"shape"` (add `block:"<block>"` for one block, `preset:` for a library entry).

## Before the first line

Start with the [GraphCode Marketplace](MARKET.md). Find the closest precedent and adapt its
recipe or assets where this shape needs changes.

Read this page, then the guides listed for your kind of shape (the table further down), then the nearest
shipped example. Ask the tool about each entry you plan to use. Write in your plan which guides you read.

## Your first shape

A small moulded tub with a crisp shoulder. It uses a plain outline, an extrusion, a `remesh`, and one `warp`
that reads a profile with a declared crease.

```gcshape
gcshape 1  into:/Game/GraphCodeExamples/SharpWarp  smooth:0.2  shade:40

build {
  SM_Tub = $Tub
}

Side = profile(0 = 0, 0.2 = 0.15, 0.35 = 0.4@sharp, 0.7 = 0.9, 1 = 1.2)
Tub = mesh() {
  rect(w=30, h=20)  round(4)  extrude(height=12)  remesh(0.6)
  warp(to=(point.x * (1 + $Side(point.t.z) / 15), point.y * (1 + $Side(point.t.z) / 20), point.z))
}
```

How to read it:

- The first line names the asset (`into:`), the surface tolerance (`smooth:`, in cm) and the angle above which an
  edge shades hard (`shade:`, in degrees).
- `build { SM_Tub = $Tub }` says which entry becomes which static mesh.
- `Side` is a `profile`: a curve of numbers over `t` from 0 to 1. `$Side(t)` reads it.
- `point.t.z` is how far up the body a point is, from 0 at the bottom to 1 at the top.
- `0.4@sharp` declares a crease. The wall leans out gradually, then turns crisply at 35% of the height.
  [Declared creases](#crisp-lines-inside-a-surface-declared-creases) explains what the build does with it.

## Find the spelling

Most "Shape cannot do this" reports turn out to be a spelling that already exists. Look here before you write a
workaround or ask for new grammar. The last column is where the tool explains the entry
(`gc_describe kind:"shape" block:"<block>"`).

### Joining and cutting

| You want | Write | Ask |
|---|---|---|
| two parts melted into one smooth surface, no hard seam (a wing into a body, a fender into a door) | `add($B, join=blend, falloff=<cm>)`; it rebuilds the whole combined surface, so fine detail can go | `block:"joins"` |
| a constant-radius fillet along a seam | `add($B, join=fillet, radius=<cm>)` | `block:"joins"` |
| a part that sits beside another with no weld (glass, a trim strip, a lens) | `add($B, join=none)`: a separate shell | `block:"joins"` |
| a part cut back by what is already there, or run through to the real surface | `join=clip`, `join=full` | `block:"joins"` |
| a part that crosses another closed shape and must weld to it (a canopy into a body) | `add($B, join=hard)`, which is also the default; see [Welding shells that cross](#welding-shells-that-cross) | `block:"joins"` |
| a pipe end, limb or handle that lands on a named socket | `add($Part, into=$body.Socket, using=End)`; the build reports the joint as touching, gapped or overlapping | `block:"joins"` |
| windows with glass | `subtract` a closed window cutter from the body, then `add($Glass, join=none)` with a thin closed glass solid and its own material | `block:"joins"` |
| a door, or any cut-out, as its own capped piece | the part inside a volume is `intersect($Volume)` and the remainder is `subtract($Volume)`; both close their cut faces and tag them `[cut]` | `block:"combine"` |
| an open surface, or faces deleted | `trim() [!cut]` keeps only what a carve made; the bracket is required | `block:"trim"` |
| creature parts, or a 3D-AI head and body, joined into one | `join=organic` between typed sockets | [ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md) |

### Edges and rounding

| You want | Write | Ask |
|---|---|---|
| a rounded edge on a window, a drawer, a flute or any hole you cut | `round(<r>) [cut]` after the `subtract` | `block:"round"`, `block:"modifiers"` |
| the rim of a column or the border of one face softened | `round(<r>) [top]`; no bracket means every group boundary, not every hard edge | `block:"round"` |
| the corners of a flat outline rounded | `round(<r>)` before the outline gains depth; a corner that cuts in by less than the header's `smooth:` is left as drawn and the build says "skipped K of M corners"; lower `smooth:` below the depth it shows to round them | `block:"round"`, `block:"header"` |
| a cheap bevel on a part | `round(<r>, segments=1) [region]`: one segment is a chamfer, and `segments=` sets the count | `block:"round"` |
| a crisp corner in a smooth outline (a blade's tip, a bonnet's V), shading hard whatever `shade:` says | `curve((0, 0), (17, 0)@sharp, …)`: the curve ends at the key and starts again, and the edge swept from it is hard | `block:"curve"` |
| a curve level or upright through a key, or a tighter turn there | `(14, -2.6)@(1, 0)`: the way the curve runs through the key, a line not an arrow; `@(0.5, 0)` is tighter | `block:"curve"` |
| a crisp line inside a surface (a parting line, a chamfer that runs out, a brow ridge, a tyre groove) | `@sharp` on a stop of a `profile()` that a `warp` reads; see [Declared creases](#crisp-lines-inside-a-surface-declared-creases) | `block:"curve"`, `block:"warp"` |

### Outlines, bodies and lofts

| You want | Write | Ask |
|---|---|---|
| a cat or dog body foundation, or a longer individual | `Dog = shape('/GraphCodeShape/Presets/Dog', LoinExtra=12)` then `body = add($Dog.Torso)`. Cat and Dog supply different proportions to the shared QuadrupedTorso. Both carry neck, tail, paired hip and paired shoulder frames. | `gc_describe preset:"Dog.Torso"`; `preset:"Cat.Torso"` |
| the other half of a symmetric thing | build one half, then `add($Half)` and `add($Half, mirror=Y)`. **Negating `at=` is not a mirror** unless the part is centred on its origin: `extrude` grows from the outline (z 0..h), so a wheel turned on its side and placed at y=±90 is two different placements (one welds, one floats). `mirror=` reflects the part in its OWN frame, before `rotate=`; the build's left/right note says where the sides differ | `block:"placement"` |
| a symmetric section written once | `curve(Tc: (57.7, 0), …, Bc: (11.6, 0), loop=mirror)`: the half closes through its reflection; named keys get twins `G'` | `block:"curve"` |
| a body fuller in the middle across one direction only | `taper(to=profile(…), along=X, across=Y)`; the other direction keeps the outline you drew | `block:"taper"` |
| a body skinned through several stations (a hull, a fuselage, a bonnet) | chain `loft(to=$B, at=…)` lines; each lofts from the current top face | `block:"loft"` |
| the same body with its side line running unbroken through every station, no crease or shading seam at each | `loft(through=($B, $C, $D), heights=(40, 95, 140))`: one height in cm per outline, up from where the loft starts; a chained `loft(to=)` keeps hard stations | `block:"loft"` |
| a line that runs along a body from section to section (a crease, a chine, a keel) | name the key on every section, `G: (55.8, 19.7)@sharp`, and `loft(through=…)`: names pair by name, `@sharp` keeps the line a crease; a feature that starts or stops along the body leaves its name off those sections | `block:"loft"`, `block:"curve"` |
| a long body from blueprint lines (a car front, a hull, a bottle) | name the keys, give each a line from the side and top views, `loft(through=…, heights=…, rails=(G: ($Gz, $Gy), P: (X: $Top)))`, and write a section only where the shape changes; with every key lined, `loft(heights=(30), rails=(…))` carries ONE section the whole way | `block:"loft"` |
| a body shaped by lines you can state as numbers (a car's plan width and roofline, a hull's sheer) | give a rounded slab points (`remesh(4)  relax(1)`: the relax takes the rebuild's small ripples out), then `warp(to=(point.x, point.y * $Plan(point.t.x), lerp(14, $Roof(point.t.x), point.t.z)))` with `Plan` and `Roof` as `profile(…)`s; `$Name(t)` reads a curve at `t` 0..1 | `block:"warp"` |
| any surface moved by a formula of each point: pulled in, swelled, sagged, twisted, leaned in | `warp(by=<cm along the normal>)`, `warp(by=(dx, dy, dz))` or `warp(to=(x, y, z))`, over `point.x/y/z`, `point.normal.x/y/z` and `point.t.x/y/z`; ready-made: `Form = shape('/GraphCodeShape/Presets/Form')` then `$Form.CurveIn(depth=4) [side, smooth=4]`, `Bulge`, `Sag`, `Tumblehome`, `Twist` | `block:"warp"` |
| snow, or anything lying where the sky is open (none under an overhang or a coping's lip, thinning at its rim) | `warp(by=(0, 0, 4 * sky())) [facing=+Z, value=sky(), above=0.4, fade=0.15]`: `sky()` is the share, 0 to 1, of a 60-degree cone about +Z that leaves the shape unblocked, read against the shape as it stood at the line; give the up faces points first (`warp(by=(0, 0, 0)) [facing=+Z, smooth=12]`: `smooth=` is a feature size, so points land about a quarter of it apart, here about 3 cm) | `block:"warp"` |
| a part chosen by a measured value instead of a height (moss in patches, damp where sheltered) | `[value=<formula>, above=<t>]` (or `below=`), the threshold in the FORMULA'S units: `material($Moss) [facing=+Z, value=noise(point.x / 25, point.y / 25, 0), above=0.15]`; a face is chosen by the mean of its corners' values, and `fade=` beside it is a width in those units | `block:"modifiers"` |
| stones, boulders or rubble that rest on each other (a dry wall laid between its faces, a pile in a bin, rocks on a slope), touching and not floating | `add($Boulder, at=$pts, collide=settle, obstacle=($Left, $Right, $Floor), join=none)`: each copy falls along -Z from ITS OWN place (a list of points, a saved set, or `scatter=<n>` over faces), then tips and slides until its centre of mass is over three or more contacts; by each copy's convex hull, deterministic, the 1 % touch rule; the note counts copies, steps, ms and any copy that found no rest. Without `collide=settle`, `at=$Set` keeps every copy at its place | `block:"arrays"` |
| a collision wall used only while things settle (boards, a floor, a bin) | `obstacle=$Form` or `obstacle=($A, $B)` beside `collide=settle`: named entries the copies cannot pass, any faces (open or concave too), in the entry's coordinates, never drawn | `block:"arrays"` |
| a rock or block bounded by geological joint sets (granite in blocks, split stone, a quarried face): big flat faces from a few parallel families, not random planes | `polyhedron(size=(240, 180, 150), joints=((dip=5, azimuth=0, spacing=70%), (dip=85, azimuth=20, spacing=45%), (dip=88, azimuth=110, spacing=60%)), tilt=6, faces=2, seed=17)`: each set is two parallel planes across the box (`dip=` degrees from level, `azimuth=` the way they lean down, `spacing=` a PERCENTAGE of the box's width across them), `tilt=` the degrees each plane leans its own way, `faces=` extra random breaks; every plane in ONE half-space rebuild, no boolean; it stands on its broadest downward face and is fitted to `size=`; `weather()` after it wears the edges | `block:"polyhedron"` |
| wear where the surface turns: edges worn back, hollows kept (or dirt in the hollows) | give the shape points (`[smooth=]`, `remesh`), then `warp(by=-0.8 * clamp(curvature(), 0, 1))`: `curvature()` reads +1 on a 90-degree ridge, -1 in a 90-degree hollow, 0 on a flat face (exactly 2a/pi on a crease of angle a); the same word chooses faces, `[value=curvature(), above=0.5]` | `block:"warp"` |
| a stone with a FRESH split kept crisp while its old faces wear (a split boulder, a quarried face beside weathered ones) | name the split's joint set inside it, `polyhedron(size=(160, 120, 95), joints=((dip=4, spacing=70%), (dip=86, azimuth=15, spacing=60%, save=$Fresh)), seed=17)`, then `crease(4) [border=$Fresh]`, `subdivide(3)`, `warp(by=-1.5 * clamp(curvature(), 0, 1)) [!$Fresh, fade=20]`: the set's faces are an ordinary region (two sets with one name are one); the crease holds the split as a line, the old edges round, the fade pins the split's own points | `block:"polyhedron"` |
| one class of a split stone's edges: where two fresh faces meet, where fresh meets old, or old meets old | an edge word narrowed by the saved region: `[sharp, $Fresh]` (both faces in it), `[border=$Fresh]` (where it stops), `[sharp, !$Fresh]` (neither face in it), on `round`, `crease`, `weather` or `save`; without an edge word `[$Fresh]` and `[!$Fresh]` on an edge verb are both its border | `block:"round"` |
| stones laid in a wall's face: each dropped only within the face, turning only about its normal, packed with less void | `add($Stone, at=$pts, collide=settle, plane=Y, fit=8, obstacle=($Front(h=$height), $Back(h=$height), $Ground), join=none)`: `plane=` names the face by its normal (an axis, or `(0, 1, 0.156)` for a batter), depth and tilt out of the face are kept (two skins = two lines), `fit=` tries turns about the normal and keeps the least void below each stone; an obstacle may be an entry called with its knobs; a stone that starts inside a board is moved out by the smallest move, never lifted over it | `block:"add"` |
| a verb of your own | an `instruction()` holding the lines (a `warp` for anything per-point), kept in a recipe file and imported; the call's bracket reaches the lines inside | `block:"warp"` |
| a joint, crease or bumpy patch smoothed | `relax(<cm>) [region]`; the distance is a cap on how far a point may move | `block:"relax"` |
| a tyre, a wheel or a ring with a tread | `Wheels = shape('/GraphCodeShape/Presets/Wheels')` then `add($Wheels.Tyre, r=, w=, grooves=, depth=)`: the tread is MOVED in by a `warp`, not cut, so it cannot fold or cross where it meets the round surface. A tread cut with `subtract($Tread, repeat=<n>, around=Z, spread=360)` grazes the curve and leaves slivers | `block:"warp"`, `block:"revolve"` |

### Marks, lettering and placement

| You want | Write | Ask |
|---|---|---|
| a shut line, panel gap or groove that follows a path across a surface | `raise(by=-0.6) [top, in=$Foot, along=$Route, project=-Z]`: a brush stroke with a closed footprint and a path | `block:"save"`, `block:"form"` |
| lettering, a badge or a crest from artwork | `outline('/Game/…', id=, width=)` on an imported SVG; SVG right and down become +X and +Y, so view it from +Z (azimuth 90) | `block:"outline"` |
| a part turned the way you meant | `rotate=(roll,pitch,yaw)` is Unreal's FRotator in degrees; positive pitch turns +X toward +Z (so +Z toward −X) | `block:"placement"` |
| trim or glass laid on a curved body at a set stand-off | `conform(to=$Body, gap=<cm>, project=<direction>) [region]`: `gap=` holds it clear of the target (negative sinks it in) and `project=` sends every vertex one way, so a label keeps its shape on a curved side | `block:"form"` |
| a hose, a cable or a pipe along a path | `sweep(along=$Path)` with `$Path` a 3D path (three components per point) | [ASSET-QUALITY.md](ASSET-QUALITY.md) |
| a ROPE along any path you declare (a lashing, a wrap, a binding, a rope over a branch, a span), resting on the real members | in a `plane()`, path words that append guide points in order (`around`, `over`, `under`, `at`, `between`, `tuck`, `to`, or raw `points()`), then `drape(...)`, then Rope's `Line` with `follow(along=)`; see [Ropes: declare the path, drape it](#ropes-declare-the-path-drape-it) | `block:"around"`, `block:"drape"` |

### Colour, UVs and materials

| You want | Write | Ask |
|---|---|---|
| box-projected UVs for a label, a decal or a crate | `uv(box, at=, facing=, up=, scale=, offset=) [region]`; it writes channel 0 only | `block:"surface"` |
| no UV cuts across a painted body | `uv(none)`; see [UVs and tangents](#uvs-and-tangents-uvnone) | `block:"surface"` |
| every tile, plank, stone or quoin of an array its own tint, fixed across rebuilds | after the `add` lines, `colour(random, min=0.7, max=1, seed=3, channel=r) [material=$RoofTile]`: one draw per placed copy, keyed by the copy and `seed=`; the material multiplies its colour by `Vertex Color` R ([MATERIALS.md](MATERIALS.md#vary-each-piece-with-a-shape-vertex-colour)) | `block:"colour"` |
| grime at the base, soot round a chimney, a damp patch, fading over a distance | the area and its fade are a `weightage()` (a `condition=z` band, an `edge=` falloff, `from=`/`to=`, noise, combined with `add`/`multiply`/`intersect`), then `save($Wall)`, `$Grime = select($Wall, weight=$Low)` and `colour(0, channel=g) [$Grime]`; add `per=copy` so each tile takes one value from where it sits | `block:"colour"`, `block:"weightage"` |
| several masks in one mesh, layered | one channel each (`channel=r`, `g`, `b`, `a`); lines run in order and `mix=set\|add\|multiply\|max` with `strength=` says how a later one lands on an earlier one | `block:"colour"` |
| vertex colour that must survive a rebuild | write `colour()` last: after any `remesh` or `join=blend` (they drop it). Inserting a line above an `add` redraws that copy's `random` value. The build line's `colours N` shows it ran | `block:"colour"` |
| a soft blend between two materials on one mesh (lips into skin) | a material id is one integer per triangle, so a slot has no fade. Write a weight with `colour(<value>, channel=) [$Region]`, from a `weightage()`, and let the material read `Vertex Color` | `block:"colour"`, `block:"weightage"` |

### Names, lists, formulas and draws (G27, G28)

For a recipe that repeats, remembers or draws. Every row names the word; the exact grammar is never copied here: ask the block in the last column.

| You want | Write | Ask |
|---|---|---|
| a value one loop pass writes and a later pass or line reads (the course below a stair, the previous plank's end) | an indexed name: `$x[i]` is one body-local number per whole-number index, written by a set and read like any `$name` in that body. No `eq()` selection sums, no `$x1 $x2 $x3...` written out | `block:"values"` |
| an outline built point by point (a crack's stations, a jagged edge) | a points list: `$pts[i] = (x, y)` appends or overwrites an entry, `$pts[$i].y` reads one number, `count($pts)` counts, and `points($pts)` passes it whole. One kind per list and one width per list. A list of `(x, y, z)` is also a set of break points: `fracture(from=$pts)` (2026-11-20) | `block:"values"` |
| a list passed whole into a call, a loft or a recipe | a bare list name as the argument's whole value: `loft(through=$sec, heights=$h)`, `subtract($Scar, c=$cut)`. A recipe parameter whose default is `[]` takes a list | `block:"values"`, `block:"loft"` |
| a field used many times (a wear bell, a hash, a clamp of a knob) | `formula()`: a named formula at file level, read `$Wear(point.x - 40)`. Written once, changed once | `block:"values"` |
| a random number inside a formula or any arithmetic | `random(seed=, min=, max=)` as a term: `$x = 20 + 10 * random(seed=$s + $j, min=0, max=1)`. In arithmetic it needs `seed=`; do not rebuild it from a sine hash | `block:"values"` |
| many copies, each with its own draw | a `loop` whose draw reads the pass: `random(seed=$noise * 1009 + output.index, ...)`; seeds that differ per pass, never one shared seed | `block:"arrays"`, `block:"values"` |
| the first piece of a loop to carry the same `join=` as the rest | write `join=` on the first `add` as well: it is accepted and does nothing there. Do not write the first `add` twice | `block:"joins"` |
| a recipe's knobs passed on to a recipe it calls | K1: `Walkway = mesh($AgedBoard, boards=6) { ... add($AgedBoard, ...) }` takes every knob of `AgedBoard` and hands each one on that the call does not write. Do not copy the knob list | `block:"meshes"` |
| a name read back exactly (a long expression named, then used) | a set stores full precision, so `$a = <expr>` then `$a` builds what `<expr>` inline builds. Name sub-expressions freely | `block:"values"` |

### Damage and broken wood

In progress: the names below are stable, the looks are still being judged. Two kinds, and you choose by what you
already have. A **modifier** (`Damage`) breaks a mesh you already built, and paints nothing. An **out-of-the-box
mesh** (`BrokenTimber`, `Debris`) is a finished piece you place. Every entry is seeded (`noise=`), so one line is a
family.

| You want | Write | Ask |
|---|---|---|
| any wood break or broken-timber mesh, customised: the TOP-LEVEL knobs every `Damage` wood entry and every `BrokenTimber` mesh leads with (round 13) | size: `width` 1-200 cm and `thick` 0.3-100 cm (a `BrokenTimber` mesh also `length` 20-600 cm, the post `height` 30-500 cm); `damage` 0-3 (1: how broken, scaling the break's reach and loose material: `Snap`/`SnapOff` deep, fray, loss, chew; `Crumble` deep, bite; `Split` open, tear; `BeamBreak` deep, fray, split; open defect: a 30 x 30 beam and damage 3 at age 0 can be refused); `age` 0-2 (1: 0 a fresh break with long fine tips, 2 weathered: fine tips halved, chew, crumbs and torn walls 1.5 x, a `BrokenTimber` blank's arrises eased further); `noise` the seed (any number); `budget` 0 game / 1 high / 2 ultra (the break's point spacing at 0.5 / 1 / 1.5 x `detail`; the build line's `[triangles=]` sets the final count). Then placement (`origin=`, `rotation=`; `at_break=`), then the ADVANCED knobs, then the UV frame. Every knob, its unit, range and default is in the KNOBS table above its entry in the preset's header, and EVERY knob is clamped to its range (a value past it builds the range's end, never a broken mesh). At the defaults the meshes are the ones before these knobs existed | `gc_describe preset:"Damage"`, `preset:"BrokenTimber"` |
| your own board or plank snapped across the grain (choose this when the board is already yours) | `Dmg = shape('/GraphCodeShape/Presets/Damage')` then `$Dmg.Snap(width=, thick=, damage=, age=, noise=, budget=, origin=, rotation=)`; also `deep=`, `grain=`, `rough=`, `lean=`, `shelf=`, `tilt=` (cell walls leaning end-on), `sub=` (stepped flanks), `wander=`, `fray=`, `tipfray=`, `chew=`, `thin=` (the bottom torn away toward the break), `peel=` | `gc_search kind:"preset" target:"Snap"` |
| the piece that came off the same break (it fits the board tooth into socket when the arguments match) | `$Dmg.SnapOff(…)` with the same `width=`, `thick=`, `deep=`, `grain=`, `rough=`, `noise=`, `origin=`, `rotation=` as `Snap` | same |
| rotten, dry or brash wood: short, blocky steps and a V bite | `$Dmg.Crumble(width=, thick=, damage=, age=, noise=, budget=, origin=, rotation=)`; also `deep=`, `bite=` | same |
| a split along the grain from an end | `$Dmg.Split(width=, thick=, damage=, age=, noise=, budget=, origin=, rotation=)`; also `open=`, `length=`, `wander=`; `origin` is the mouth on the bottom face, `length` is how far it runs back in cm | same |
| a thick beam or post snapped (one stair step and a split along the grain, not the board's saw-tooth) | `$Dmg.BeamBreak(width=, thick=, damage=, age=, noise=, budget=, origin=, rotation=)`; also `deep=`, `lip=`, `run=`, `delam=`, `split=` | same |
| a ready-made broken plank to place (you want one, not a modifier) | `Timber = shape('/GraphCodeShape/Presets/BrokenTimber', Face = $Oak, Broken = $OakTorn)` then `add($Timber.BrokenPlank(length=, width=, thick=, damage=, age=, noise=, budget=, at_break=))`, then any `Snap` knob (`deep=`, `rough=` ...) | same |
| both halves of one broken board, a little apart | `add($Timber.PlankHalves(…, slide=, swing=))`: the plank's arguments plus how far (cm) and how much (degrees) the far half slid and turned; two parts by design (`gap=`, `turn=` are claimed by `add()`) | same |
| a rotten plank | `add($Timber.RottenPlank(length=, width=, thick=, damage=, age=, noise=, budget=, at_break=))`, then any `Crumble` knob (`deep=`, `bite=` ...) | same |
| a split board | `add($Timber.SplitBoard(length=, width=, thick=, damage=, age=, noise=, budget=, split=))`, then any `Split` knob (`open=` ...) | same |
| a broken beam, lying | `add($Timber.BrokenBeam(length=, width=, thick=, damage=, age=, noise=, budget=, at_break=))` | same |
| a broken fence post, standing | `add($Timber.BrokenPost(height=, width=, thick=, damage=, age=, noise=, budget=, at_break=))`; its grain runs up +Z | same |
| a crate or fence slat, snapped | `add($Timber.BrokenSlat(length=, width=, thick=, damage=, age=, noise=, budget=, at_break=))` | same |
| `at_break` on any `BrokenTimber` entry | the fraction of the length, from the −Y end, where the break falls | same |
| every other look of a break (shelves, spire count and shape, ribbons, losses, chew, peel angle and the groove it tore from, rot steps and crumbs, the bite's steps, split ledges and lips, the horn's fibres, the beam's tread) | each is a knob on the `Damage` instruction, with its unit, range (clamped) and default in the KNOBS table above it; every `BrokenTimber` entry takes the same knobs by the same names and passes them through | `gc_describe preset:"Damage"` |
| fibre bundles that end in blunt torn ends, not a field of small pyramid tips (which reads end-on as gravel) | `torn=0.85` on `Snap`, `SnapOff` or `BeamBreak` (0, the default, keeps the tapered tips); `torntilt=` how far each end tilts, `tornrib=1` one ribbon length per bundle, `tornsep=` how deep the cracks between bundles run (x the spires' reach); `BeamBreak`'s `treadtorn=1` passes `torn` to the beam's ribbed tread. The shop meshes and every `BrokenTimber` board write `torn=0.85, torntilt=0.15, tornrib=1, tornsep=0.15` | `gc_describe preset:"Damage"` |
| end grain that breaks along growth rings and rays, not a grid | `rings=1` and the pith where the log's heart was: `pithx=` (x width off the middle), `pithz=` (x thickness up from the bottom face: `-3` a flat-sawn board, `0.5` a boxed heart), `pithd=` (a per-seed shift, x width); `hold=` how much the board's faces keep their teeth (1), `heart=1` makes the inner rings reach furthest (a post's heart never pits). 0, the default, keeps the grid | same |
| the piece that came off reading like a snapped board end-on (a few long bundles among many short, not a uniform shard field) | `$Dmg.SnapOff(…, longside=1)`, with `torn` above 0 (on `Snap`, 1 gives the long bundles to the +Y piece instead) | same |
| a half's own loss following a steep break's slant, so its high side is not planed into a near-clean cut | `lossfollow=1` on `Snap` or `SnapOff` (0 lays the loss on a near-flat plane) | same |
| a snapped beam's lower step without a row of small chips along its end-on | `$Dmg.BeamBreak(…, lowfray=0.3)`: the lower step's ribbons, x `fray` (default 0.6); `treadfray=` the tread's ribbons, x `ridge` (default 0.35). `BrokenBeam` and the shop mesh write `lowfray=0.3, detail=0.75` | same |
| a rotten or brash bite whose back wall is many planes, not a flat-floored slot | `$Dmg.Crumble(…, bitetilt=1, bitestep=0.5, bitecap=0.1)`: `bitetilt` tilts each block's floor inside the bite by its own draw (0, the default, keeps them level); `RottenPlank` writes these | same |
| a split whose lips fray in layers, its torn walls mapped without stretch | `$Dmg.Split(…, lipfray=0.25, uvfit=1)`: `lipfray` lets the plies' steps and torn ridges reach the board's faces (0, the default: clean lips); `uvfit=1` maps the torn walls on four narrow windows between the width and the thickness, and needs `tear` above 0. `SplitBoard` and the shop mesh write both | same |
| a rotten bite whose floor is torn into fibre stubs with crumbs left in it, not one smooth scoop | `$Dmg.Crumble(…, biterib=0.8, bitecrumb=1)`: `biterib` 0-1 makes every crumb cell inside the bite stop its own share short (0, the default, a smooth floor); `bitecrumb` 1 keeps every crumb in the bite. `RottenPlank` and the shop mesh write both (round 13) | `gc_describe preset:"Damage"` |
| a split whose lips are rows of fibres, not smooth block steps | `$Dmg.Split(…, lipfray=0.25, lipridge=1)`: `lipridge` 0-1 tapers each ply's jog and each torn band along the grain into a sliver pointing at the mouth and narrows the walls' UV windows (0, the default, round 12's blocks); it acts only where `lipfray` is above 0. `SplitBoard` and the shop mesh write it (round 13) | same |
| a thick post or beam whose break reads end-on as layered slabs, like a plank's | `BrokenPost` and `BrokenBeam` (and the shop `BeamBreak`) carry the PASSED plank's break cm for cm (round 13): bundles about 3.8 cm wide in 1.3-1.9 cm bands, a 9 cm reach, the plank's ribbons, slope and shelves. On your own size, keep those centimetres: `grain` = 3.8 / width, `layers` = thickness / 1.3, `deep` = 22.5 / width (Snap/BeamBreak knobs are fractions of the section) | same |
| the fibre normal/AO maps for the `BrokenTimber` meshes (optional: about five minutes of bakes) | build `/GraphCodeShape/Presets/BrokenTimberMaps`: each `SM_BrokenTimber_…Maps` line bakes one mesh from its `…Detail` source; for other arguments write the same two lines with yours (`X = mesh() { add($Timber.BrokenPlank(…)) }`, `XDetail` the same plus `roughen(…, form=ridged) [cut]`) | `block:"build"` |
| an aged or rotten board, uneven in every sense, its end snapped or not (the part every timber kit places) | `Kit = shape('/GraphCodeShape/Presets/TimberKit', Face = $Oak, Broken = $OakTorn, Rope = $Hemp)` then `add($Kit.AgedBoard, width=, thick=, length=, damage=, age=, noise=, budget=, index=)`: width 20 cm (2-60), thick 5 cm (0.5-30), length 200 cm (5-1000), `damage` 0 (0-1: above 0 the +Y end snapped by `JaggedEnd`), `age` 0.5 (0-1: 0 new, 0.5 aged, 1 rotten; every unevenness x (age/0.5)², above 0.55 the board decays: open checks, ridges along the grain, crumbled ends, a sliver off, a missing corner, thinner), `softness` 0 (0-1: 0 crisp, arrises worn 2-6 mm, breakouts, a faceted cup, stepped fibrous ends; toward 1 worn round like driftwood: arrises to 1.75 cm, wavy edges, domed ends; independent of age), `noise` the seed, `budget` 1 (0 Game, 1 High, 2 Ultra), `index` the member's number. About forty advanced knobs (width, thickness and length variation, taper, a waney edge, thickness along, yaw, pitch, roll, lift, cup, bow, crook, twist, ends off square, wear, per-member state, the decay's parts, a sprung end, sag) are listed with unit, range and default in the preset's header; every knob is clamped to its range. The slots are pathless: bind them, or the engine draws its grid material | `gc_describe preset:"TimberKit"` |
| a weathered plank, beam, rope lashing or settled stone for ANY wooden kit (a deck, a house wall, a ladder): the shared wood operations | `Kit = shape('/GraphCodeShape/Presets/TimberKit', Face = $Oak, Broken = $OakTorn, Rope = $Hemp, Stone = $Rock, Iron = $Iron)` then, from its section SHARED WOOD OPERATIONS: `add($Kit.DeckBoard, width=, thick=, length=, cup=, bow=, crook=, twist=, taper=, roll=, pitch=, decay=, checks=, crumb=, gouge=, lips=, hollow=, edgew=, noise=, index=)` (one plank, whole: section, bends, decay, rotted ends, gouges, broken lips, worn hollows, edges wandering WITH their gap, keyed by `sd`/`gl`/`gr`/`nl`); `$Kit.Gouges(...)`, `$Kit.EndRot(...)`, `$Kit.Grooves(...)` the same operations on your own board; `add($Kit.DeckJoist, w=, d=, len=, jdv=, jcheck=, jends=)` an old squared beam (checks, eroded grain, ends off square); `add($Kit.Lash, w=, h=, r=)` a round rope turn round a bundle (slot `Rope`; since TK15 made by the Rope preset, the game budget's flat ring kept as `RingLash`); `add($Kit.Cobble, size=, ar=, tr=, noise=, collide=settle)` a field stone that rests where it lands (slot `Stone`); `$Kit.NailRod` a nail's shank (slot `Iron`); `$Kit.Member(...)`, `$Kit.OldWood(...)` and the wood treatments (next rows); `add($Kit.Pole, r=, len=, taper=, crook=, nodes=, sag=, dv=, checkd=, ero=, foot=, wear=, split=)` a round pole, rung or bamboo culm; `add($Kit.Wrap, R=, r=, sx=)` a round cord turn hugging a pole (`sx` stretches it round two; Rope, ring `RingWrap` at the game budget). WoodRuin's decks are built from these (rebuilt identically, 20 Nov) | `gc_describe preset:"TimberKit"` |
| ONE wooden member of any kind, new to old on one `age` slider, bent the way old members bend: a pole, rung or rail (round), a post or beam (square), a plank, or your own section; a temple pillar is a big round one (owner: "make it bigger and it becomes a pillar") | `Kit = shape('/GraphCodeShape/Presets/TimberKit', Face = $Oak, Broken = $OakEnd)` then `add($Kit.Member, profile=0, size=45, length=400, top=0.88, bulge=0.02, turned=1, age=1, noise=3)` (a temple pillar a bit aged: ~11 long hairline checks, radial end checks, an eased rim): `profile` 0 round, 1 square or rectangular (`width`), 2 plank (flat-sawn, `width` 5 x size by default), 3 your own `section=$Plane`; `size` the thickness (cm; a plank AGES by its `width`), `length` along the grain; `top` a taper, `bulge` an entasis; `age` 0-2 runs `OldWood` (on a Member its checks are CUT: thin V splits subtracted along the grain, crisp at any width and never past the original section; past age 1.3 a member of 8 cm or more loses 2-3 SLABS, shelves split off along a check that taper and tear off obliquely in steps along the grain, their torn ends carrying attached fibre slivers past age 1.5 (a plank on its top face; a SQUARE or rectangular post ages as STRIPS instead: each face split along the grain by checks 3-4 cm apart, strip by strip the outer wood lost to its own depth between a few break lines across the face, the torn ends jagged with slivers, the arrises eaten back first in torn facets), and past 1.6 a member of 14 cm or more rot HOLES long along the grain; every cut end's rim eased; `slabs=`, `holes=`, `ease=` override) and the bends (`bow`, `crook`, `twist`, plank `cup` and `waney`, from age: aged L/400, weathered L/150), `sag` a beam's dip, `pole=1` an unsawn pole (its axis wanders, knots stand out); `turned=1` a turned member (a pillar): straight, no bow, crook or twist; `budget` 0 Game / 1 High. It STANDS along +Z from z = 0, centred; `add(..., rotate=)` lays it down (a rail: `rotate=(0, -90, 0)` runs it along +X, and +X then points down, so `sag` dips). UV: the grain along U on every face, 100 cm a tile; a round is ONE continuous wrap with one seam at `seam` degrees from +X (180, the back, by default; `seam=0` puts a laid rail's seam underneath), each member its own place on the texture. Slots `Face`, `Broken` (the cut ends) | `gc_describe preset:"TimberKit"` |
| ANY wooden shape you built made OLD in its own grain (a carved bracket, a beam you modelled), choosing only the treatments you want | on the shape being built, in this order, each its own instruction: `$Kit.WoodPoints(along=[(0, 0, 1)], size=)` (the points the rest need), `$Kit.Wear(amount=)` (convex parts only, by `curvature()`), `$Kit.Rot(along=, size=, amount=, fh=)` (the foot eaten back), `$Kit.Erosion(along=, size=, amount=, relief=)` (sharp grain crests along the grain and broad hollows; the open side and the foot more), `$Kit.Checks(along=, size=, count=, mouth=, clen=, length=)` (long V checks along the grain, radial to the pith `px`, `py`; on your own shape they are SAMPLED with creased lips; `cut=1` with your section's half sizes `hw=`, `hh=` (-1 round) CUTS them instead, as `Member` does; on a cut end the radial end cracks; `sides=`, `ends=`; `length=` the member's length so the checks ease out at the cut ends and the rim stays whole), `$Kit.Slabs(along=, size=, age=, length=, hw=, hh=)` (shelves split off and rot holes, cut away; a square section ages as strips and torn arrises; `hw`, `hh` a rectangle's half sizes, -1 round), `$Kit.EndCracks(...)` (the ends only), `$Kit.Bend(profile=, size=, length=, bow=, crook=, twist=, cup=, waney=, sag=, wobble=, knots=)` (the boards' own `Age` bends, on a member standing along +Z); or all at once with `$Kit.OldWood(along=[(0, 0, 1)], size=45, age=1, noise=3)`. `along` is a ONE-ENTRY LIST (a bare tuple's components cannot be read). Write `uv(...)` and `material(...)` after. Photo table: `Docs/Research/old-wood-pillars-brackets.md` | `gc_describe preset:"TimberKit"` |
| a ROPE and what rope does: a laid 3-strand rope (or a plain cord) new to old (frayed ends, uneven strands, a broken strand, loose fibres), a straight length, a span sagging between two points (catenary), a hanging end bending onto the ground, a flat coil, turns round a member, a SQUARE lashing of two crossing members, a ROUND lashing of two side by side, a hitch tied off on a post, an overhand knot | `Rp = shape('/GraphCodeShape/Presets/Rope', Rope = $Hemp)` then e.g. `add($Rp.Square, r=, aw=, ah=, bw=, bh=, turns=, frap=, age=, at=)` (member A along X over member B along Y behind it, touching; `sink=` when A sits into B), `add($Rp.Span, span=, rise=, sag=)` or `length=`, `add($Rp.Hang, len=, drop=)`, `add($Rp.Coil, turns=)`, `add($Rp.Wrap, w=, h=, turns=)`, `$Rp.Turn`, `$Rp.Round`, `$Rp.Hitch`, `$Rp.PostHitch`, `$Rp.Overhand`, `$Rp.Line`. Every piece takes the rope knobs `r` (radius), `strands` (0 plain), `lay`, `budget` (0 game, 1 high: lines, spans, coils and knots laid, lashings plain round; 2 ultra: laid everywhere, fibres), `age` 0-2, `fray`, `fuzz`, `lost`, `sides`, `detail`, `noise`, `index`; each in its own frame, place it with `at=`. These pieces' paths are formulas; any other rope (one that rests on what is under it) is a declared path, [Ropes: declare the path, drape it](#ropes-declare-the-path-drape-it) | `gc_describe preset:"Rope"` |
| an old wooden LADDER (pole rails with lashed or through rungs, sawn rails with round or flat rungs), new to decayed, leaning or standing | `Kit = shape('/GraphCodeShape/Presets/TimberKit', Face = $Oak, Broken = $OakTorn, Rope = $Hemp, Iron = $Iron)` then `add($Kit.Ladder, type=, height=, age=, lean=, budget=, noise=)`: `type` 0 pole rails + round rungs lashed on their front with cord, 1 pole rails + rungs through them, 2 sawn rails + round rungs through, 3 sawn rails + flat housed treads (type 3 has not passed the critic yet); `height` 300 (100-800), `age` 1 (0-2: checks, crooked rails, worn rungs; at 2 eroded rails, rotted feet, missing rungs leaving their holes, a cracked sagging rung, slipped lashing), `lean` 0-30 degrees; every construction knob (`width`, `splay`, `pitch`, `rails`, `rungs`, `joint`, `rail_w`, `rail_d`, `rung_d`, `rung_w`, `rung_t`, `over`, `missing`, `crack`, `slack`, `holes`) defaults to -1 = the photographed value for its type (the header lists them). Built from the shared `Pole` (a round pole or rung), `Wrap` (a round cord turn), `DeckJoist` and `DeckBoard`. Shipped lines `SM_Ladder_<PoleLashed|PoleThrough|SawnRound>_<Game|High>` | `gc_describe preset:"TimberKit"` |
| a wooden HOUSE WALL, new to weathered on one slider: board-and-batten, clapboard, shiplap, or East Asian post-and-plank infill | `Kit = shape('/GraphCodeShape/Presets/TimberKit', Face = $Oak, Broken = $OakTorn)` then `add($Kit.HouseWall, type=, size_x=, height=, age=, damage=, softness=, noise=, budget=)`: `type` 0 board-and-batten (vertical boards, battens over the joints, girts), 1 clapboard (lapped courses on studs, a starter strip), 2 shiplap (flush, a rebate strip behind each joint), 3 post-and-plank (sill, square posts at the bay pitch, a head beam, a through rail, vertical or horizontal infill); size 300 x 240 cm (60-600 x 60-500); `age` 1 (0 new and tight, 1 aged: gaps open by `shrink`, cup, bow, nail heads proud, a slight lean and sag; 2 weathered: decayed boards, a rotten foot, split and loose boards, battens off, lost boards broken out with torn stubs at both ends and staggered course to course, one board hanging off a nail); `damage` -1 (0.7 x (age - 1) above age 1); `softness` 0 (1: driftwood-worn arrises AND smoothed breaks); `budget` 1 (0 Game, 1 High). Board width, thickness, gap, lap, batten, girt, stud and post spacing, foot clearance, sill and head height, the rail, and every derived damage knob (missing, rot, split, loose, lift, battoff, nails, proud, lean, sag, decay, cup, bow, crook) are clamped knobs with photo-measured defaults in the section's header (-1 = derived from `type`, `age`, `damage`). Butt joints fall on studs at random and never on the course below's or within 1.5 studs of it. Sixteen shipped states `HouseWall<Batten|Clapboard|Shiplap|Gate><Aged|Weathered><High|Game>` (Game 5,000-6,000 triangles baked from the High) | `gc_describe preset:"TimberKit"` |
| a board's end snapped into a jagged break (a picket, a pale, a slat, a board wall's top, a hole's edge) | `$Kit.JaggedEnd(width=, thick=, base=, damage=, noise=, budget=, style=)` on a board in the Damage frame that runs past `base`: `base` cm, the break's base along the board; `damage` 0.5 (0-1: the teeth reach 0.3 + 1.2 damage x the width); `budget` 1 (0 the outline alone, 20-60 triangles; 1 + splinter jogs and coarse fibre; 2 fine fibre); `style` -1 (drawn) or 0 spikes, 1 stepped, 2 hooked, 3 short, 4 slanted; `tear` 0.8 (0-3) leans each torn face through the thickness. Both tiers cut the same outline, so a game budget keeps the silhouette. It maps the board; paint `material($Broken) [cut]` after | same |
| a weathered plank walkway (good condition, decayed, or broken through) | `add($Kit.Walkway, size_x=, size_y=, damage=, age=, noise=, budget=)`: size_x 300 cm (100-800), size_y 180 cm (80-400), `damage` 0 (0-1: an asymmetric hole with Damage.Snap ends, planks gone, debris on the deck), `age` 0.5 (0-1: 0.5 good condition, 0.9 decayed: every plank decayed, sag, loose, sprung and missing planks, nails proud), `softness` 0 (0-1, as `AgedBoard`'s), `noise`, `budget` 1; slots `Face`, `Broken`, `Rope`; about seventy advanced knobs (beams, sleepers, lashings, planks, the hole's shares, every plank's unevenness) in the header. States `_Aged`, `_Decayed`, `_Broken`, and `WalkwayDebris` for what fell through | same |
| a vertical board wall with snapped jagged tops, or a ragged hole | `add($Kit.BoardWall, size_x=, height=, damage=, age=, noise=, budget=, mode=)`: size_x 200 cm (60-600), height 240 cm (60-500), `damage` 0 (0-1), `mode` 0 snaps the tops (lower and taller teeth with damage; no two neighbours of one family or height) or 1 breaks a ragged hole (larger with damage), `age` 0.5, `softness` 0 (0-1, as `AgedBoard`'s), `noise`, `budget` 1; slots `Face`, `Broken`; the rest in the header. States `_Aged`, `_Snapped`, `_Holed`, and `BoardWallDebris` for what fell out | same |
| one loose shard of wood | `Debris = shape('/GraphCodeShape/Presets/Debris', Face = $Oak, Broken = $OakTorn)` then `add($Debris.Shard(len=, w=, t=, n=, horn=, kink=, twist=, noise=))` | same |
| a thin splinter stick | `add($Debris.Splinter(len=, ar=, kink=, twist=, noise=))` | same |
| a pile of shards, splinters and chips on the ground | `add($Debris.Splinters(count=, sticks=, heap=, chips=, spread=, noise=))` | same |
| a smashed board: one board broken into long pieces, opened apart | `add($Debris.Smashed(w=, len=, t=, pieces=, open=, noise=))`; `open` is how far each piece shrinks about its own centre | same |
| thin flakes scattered round a break | `add($Debris.Chips(count=, spread=, noise=))`; one flake is `$Debris.Chip(len=, ar=, curl=, bow=, noise=)` | same |
| a loose steel bar, a bar end standing out of a break, or a bar bent by angle, radius and droop (debris lane, round 1) | `R = shape('/GraphCodeShape/Presets/Rebar', Steel = $Rebar, SteelCut = $BrightSteel)` then `add($R.Bar, len=, damage=, age=, noise=, budget=)` (len 30 cm, 2-300; `damage` 0 bows and waves it), `$R.BarStub(stick=, damage=, age=, noise=, budget=)` (stick 6 cm, 0.3-60, its origin on the break plane, a hidden root `embed` below; `damage` 0.5 draws the end: sheared, necked or cut square, and the lean) or `$R.BentBar(len=, damage=, age=, noise=, budget=)` (len 30; `damage` 0.5: the bend 10-110 degrees, a second bend past 0.55, droop); `age` 0-1 rust (section loss, pitting on high/ultra); `budget` 0/1/2 = 6/8/12 sides; all grow along +Z; advanced knobs (`d`, `end`, `shear`, `neck`, `lean`, `root`, `angle`, `radius`, `dir`, `bend2`, `droop`, `ribs`), meanings, units and clamps in the file's header | `gc_describe preset:"Rebar"` |
| an angular concrete chunk (fist to head size) or a slab plate, as debris | `CP = shape('/GraphCodeShape/Presets/ConcreteParts', Face = $Cast, Broken = $Aggregate)` then `add($CP.Chunk, size=, damage=, age=, noise=, budget=)` (size 12 cm, 2-80; lies on its cast-skin face) or `$CP.Plate(w=, damage=, age=, noise=, budget=)` (w 30 cm, 4-300; skin both faces or `skins=1`); `damage` 0.5 (0 blocky, 1 sharp and uneven, a plate's sides leaning and top rim chipped), `age` 0 (corners abraded round), `budget` 0/1/2; advanced (`ar`, `tr`, `faces`, `even`, `rough`, `skin`; `d`, `t`, `lean`, `chips`) in the header | `gc_describe preset:"ConcreteParts"` |
| a concrete chunk with steel bars through it, only their ends standing out of its broken faces (the owner's "concrete with steel bar inside") | `DA = shape('/GraphCodeShape/Presets/DebrisAssemblies', Face = $Cast, Broken = $Aggregate, Steel = $Rebar, SteelCut = $BrightSteel)` then `add($DA.ReinforcedChunk, size=, damage=, age=, noise=, budget=, bars=, d=, stick=)`: size 17 cm (6-80), `damage` 0.5 (also the share of ends bent and how far), `bars` 2 (1 a bar, 2 a crossing pair, 3 a second main bar), `d` 1.27 (#4), `stick` 7 cm mean stand-out (each end 0.3-1.4 x, about a fifth stop inside); every bar is two stubs meeting at a hidden point, each end its own fate | `gc_describe preset:"DebrisAssemblies"` |
| a rubble pile of plates, chunks (some reinforced), fines and loose bent bars, sized by class and settled | `RB = shape('/GraphCodeShape/Presets/Rubble', Face = $Cast, Broken = $Aggregate, Steel = $Rebar, SteelCut = $BrightSteel)` then `add($RB.Pile, area=, damage=, age=, noise=, budget=)`: area 140 cm (40-800); the shape of the pile in `count` (0 = derived), `volume` (cm3 the break removed x kept: sizes the biggest pieces), `size` 44, `big` 2 (the biggest class), `grading` 1.5 (each halving of size holds 2^grading times the pieces; `spread=` is reserved by `add()`), `settle` 0.2 (share of a piece sunk into what it lands on), `cluster` 0.45 (the heap's share of `area`), `tail` 2.2 (how far small pieces run out, in heap lengths), `rchunk` 0.33, `bars` 3, `fines` 1, `bed` 1 (a low bed of fines) | `gc_describe preset:"Rubble"` |
| a stone wall module: dry-stone, mortared, ruined (top stones missing along a stepped line) or coursed, with finished end heads, or a 90 degree corner on a quoin pier. Irregular laid rubble: rows of stones of mixed sizes, a few 2-3 x the row's mean (some spanning two courses), polygon outlines, front and back drawn separately (a kit module; builds buried `bury` cm below z = 0 so it stands in any terrain) | `SW = shape('/GraphCodeShape/Presets/StoneWall', Face = $Stone, Joint = $Core)` then `add($SW.Wall, len=, height=, damage=, age=, noise=, budget=)` or `add($SW.Corner, len=, height=, damage=, age=, noise=, budget=)`. TOP-LEVEL knobs, clamped: `len` 100-600 cm (200; a Corner's two arms together), `height` 40-300 cm (120), `damage` 0-1 (0: chipped faceted corners and break faces), `age` 0-2 (1: rounder corners, more in and out), `noise` 1-99999 (the seed; every stone draws `noise * 1009 + index`), `budget` 0 game / 1 high (rounding steps, bulge and facet size; High adds a faceted roughen and a pillowed face). ADVANCED: `mortar` 0-1 (0 dry; 1 joint 1.9 cm, stones rounded 2 cm, face `recess` 0.3-3 cm (1.5) behind the stones, stones in and out only +-1 cm), `ruin` 0-1 (0; from 0.02 the rows are shortened from a drawn end, so whole stones are missing, never sliced; the coping rides only the unbroken run), `ends` 0-2 (2: finished heads of squared stones, so no sliced end shows; 0 to tile modules; a ruin keeps the head on its intact end), `coursed` 0/1 (0 the rows above; 1 the older lattice bands, each broken on its own, one over two), `base` 40-120 cm (60) and `top` 20-base cm (35) the battered section, `stone` 0.6-1.6 (1) stone size, `bury` 0-20 cm (8), `cope` 8-24 cm (14). Slots `Face` (the stones) and `Joint` (core, hearting, mortar). Shipped: `SM_StoneWall_{Dry,Mortar,Ruin}_{Game,High}`, `SM_StoneWall_Coursed_Game`, `SM_StoneWall_Corner_Game`; Game by the recipe about 6,000-9,500 triangles (cap 8,000), High about 30,000-52,000 (cap 48,000). A BOULDER WALL (the owner's gate photo) is settings, never a mode: `rows=0` with `stone`, `round`, `chink`, `uneven`, `fill` (soil in the gaps, slot `Soil`), `cope=0`; `detail=1` (0-1, default 0) makes it the DETAILED boulder wall: chunky field stones pillowed out of the face, PACKED (each bears on the stones below and beside it, joints 1-4 cm, small pinning stones in the triangles where three stones meet and in the vertical joints, soil 14 cm back so it shows only deep in the joints: about 6 % of the face), lumps and dents, some split faces, dark pockets, wandering courses, a ragged top stepping down by up to 1.7 stones, the face bulging, soil in the joints and on the steps; `moss` (0-1, default 0, with `detail=1`) writes moss as a soft vertex-colour mask, G = 1 - moss (Weather's spelling): bind Face/Soil materials that read Vertex Color G. Shipped boulder lines: `SM_StoneWall_Boulder_{Game,High}` and `SM_StoneWall_BoulderDetail_{High,Game}` (`height=250, base=80, top=52, stone=1.5, rows=0, uneven=0.8, fill=0.8, cope=0, ends=0, detail=1`, High `moss=0.45`): High about 325,000 triangles; Game 12,000 with normal, AO and curvature maps (4096) baked from the High on UV1 (UV0 keeps the 100 cm box map for a tiling stone; its material reads the maps with `TexCoord[1]` and greens the joints from the baked AO and curvature, since a 12,000-vertex moss mask draws as triangles) | `gc_describe preset:"StoneWall"` |
| an old pitched roof module: a gable (two slopes of rafters, battens and plain clay tiles or slate, ridge caps, a fascia at each eave, verge boards) or a single slope for a lean-to or a hip's side; sound, old (the ridge dips, the slope bellies, every course waves, a few units stand proud or have slipped) or ruined (a hole showing battens and rafters, a broken eave corner, caps missing). Courses laid from a doubled eave course at the real gauge, every unit lapped on the course below, nothing sunk or floating (a kit module on the eave line z = 0, the ridge at run x sin(pitch)) | `RK = shape('/GraphCodeShape/Presets/RoofKit', Tile = $Clay, Wood = $Oak, Cap = $Clay)` then `add($RK.Roof, len=, run=, pitch=, unit=, age=, damage=, noise=, budget=)` or `add($RK.SlopeRoof, ...)` (the same knobs, one slope, eave at y = 0 rising toward +y). TOP-LEVEL knobs, clamped: `len` 200-600 cm (300, along the ridge), `run` 150-400 cm (250, the SLOPE length of a rafter), `pitch` 25-60 deg (40), `unit` 0 plain tile 265 x 165 / 1 slate 500 x 250 (0), `age` 0-2 (1: 0 sound, 2 very old), `damage` 0-1 (0: the hole (0.6 is about 84 x 66 cm), the broken eave corner and missing caps; a gable's second slope stays whole), `noise` 1-99999 (the seed; every unit draws `noise * 1009 + index`), `budget` 0 game / 1 high (course plates / every unit). ADVANCED: `sag` 0-20 cm (10.5: the ridge dips sag / cos(pitch) at mid-span, 13.7 cm at age 1), `belly` 0-1 (0.4), `slip` 0-2 (1), `wave` 0-2 (1, course waving), `rafter` 20-60 cm (40), `ridge` 0-1 (1, the caps). THE FORM IS SETTINGS (an East Asian roof is values, not a kind): `curve` 0-1 (0, pan arch), `roll` 0/1 (0; 1 = a round cover tile over every pan joint, so each joint is one continuous RIB eave to ridge; High rolls are a chain whose laps sit tight, a solid step, no hollow and no overlap through), `rollw` 0.4-0.8 (0.56, roll 1: a roll's width as a share of the rib spacing), `width` 6-40, `length` 15-60, `thick` 0.5-3, `headlap` 2-24 cm (0 = the unit's own), `sweep` 0-30 cm (0, the eave turns up), `uneven` 0-1 (0, apart from `age`: broad lumps of the tiles, ribs wandering as wholes, a ragged eave row, and every roll its own: strayed, turned about its axis, tail lifted, worn and chipped, a few broken short or missing so the pan shows; a few pans slipped; one loose roll on the crowns and one broken piece in a channel per slope), `ridgeshape` 0-2, `stack` 0-6 (flat-tile courses under the caps), `ends` 0-2 (end blocks; roll 1: sized from the ridge's own height, never larger than the module allows), `overhang` 0-80 cm, `open` 0/1 (rafter tails show, no fascia). The weathered pan-and-roll set, its values measured on the owner's reference photo: `curve=0.22, roll=1, rollw=0.73, width=27, length=43, thick=1.3, headlap=8, sweep=6, overhang=30, open=1, rafter=30, run=220, pitch=25, ridgeshape=1, stack=3, ends=1.9, uneven=1, age=2, sag=4, belly=1`. Slots `Tile` (tiles, slate, plates, pans, rolls, end discs, lips), `Wood` (frame, deck, fascia, beam, verge and barge boards) and `Cap` (ridge bed, caps, stack, end blocks). Game tile: course strips, tile joints come from the roof material (a baked normal map is one build-line option, `maps=(normal, ao), from=$TileOldHigh`, about 1.5 minutes a line); High: every tile. Slipped tiles hang by one nib (turned 5-20 degrees, lower end lifted), clustered. Shipped: `SM_Roof_Tile_{Old_High,Old_Game,Sound_Game,Ruined_Game}`, `SM_Roof_Slate_Old_{Game,High}`, `SM_Roof_Slope_Tile_Old_Game`, `SM_Roof_Pan_Weathered_{High,Game}`, `SM_Roof_Gate_Pan_Weathered_Game` (a short gate-roof gable; Game pan ribs step at every lap, pan joints from the material); Game by the recipe about 3,500-10,000 triangles (cap 8,000; the weathered Game lines 7,436-8,000), High about 75,000-94,000 (cap 140,000) | `gc_describe preset:"RoofKit"` |
| an aged stone stair flight: block treads with staggered joints of varied width, nosings worn and dished down a line of traffic, chips, chipped corners, broken nosings and shallow flakes broken into rough facets (no flat notch), one block always sunk and one tilted, risers unequal; at `damage` 1 blocks cracked through, a front corner broken off and a block broken half away over its dirt bed (a rough-fractured piece stays, the neighbour on the hole's side spalled, `ConcreteParts.Chunk` fragments lying in the hole) | `SK = shape('/GraphCodeShape/Presets/StoneKit', Face = $Stone, Broken = $FreshStone, Joint = $Dirt)` then `add($SK.Stairs, width=, steps=, rise=, run=, damage=, age=, softness=, noise=, budget=)`. Top-level knobs: `width` 60-600 cm (200), `steps` 1-12 (6), `rise` 10-25 cm (16), `run` 20-50 cm (32), `damage` 0-2 (0 aged, 1 broken, 2 heavy; default 0), `age` 0-2 (0 freshly laid, 1 aged, 2 old; default 1), `softness` 0-1 (0 hard and crisp, the default: arrises a few mm to about 1 cm, planar faces, rock-like chips; 1 worn round like river-worn stone; independent of `age`, which keeps the foot wear), `noise` the seed, `budget` 0 game / 1 high / 2 ultra (1). Every advanced knob (joints, wear, chips, settling, weathering) is listed with its unit, range and default in the preset's header comment; a value past its range is clamped. Shipped: `SM_StoneStairs_{Aged,Broken}_{Game,High,Ultra}` | `gc_describe preset:"StoneKit"` |
| a wooden floor-deck module, aged (boards of varied width and gap, staggered butt joints over the joists, each board cupped, bowed, yawed and worn), broken through to a hole or collapsed into a heap (a kit module that swaps in place on a 200 cm grid); or a heavily WEATHERED plank walkway in use (thick planks across old beams, ragged staggered ends, gouged tops, rope-lashed cross timbers, stones lying on it) | `Ruin = shape('/GraphCodeShape/Presets/WoodRuin', Face = $Oak, Broken = $OakTorn, Nail = $Iron, Stone = $Rock, Rope = $Hemp)` then `add($Ruin.Deck, size_x=, size_y=, damage=, age=, noise=, budget=)` (`$Ruin.Wreck` the collapse, `$Ruin.DeckDebris` / `$Ruin.WreckDebris` the matching pile, same knobs). TOP-LEVEL knobs, clamped: `size_x`, `size_y` (100-400 cm, 200), `damage` (0-1.6, 1: the hole is damage x 55 x 35 cm; 0 the intact floor; Wreck 0.4-1.4, its span), `age` (0-2, 1: every imperfection of boards and joists x age; 0 new and true), `noise` (the hole's seed), `budget` (0 game, 1 high, 2 ultra). The floor's seed is `fseed` (shared by every state, so a broken one swaps in); every advanced knob, its unit, range and default, clamped, is in the header's table. The WEATHERED walkway is `add($Ruin.DeckWeathered, ...)`: planks across a walk on three long old stringers running along it (modules placed end to end along X with their own `fseed` make one walkway; a lashed cross timber under most joins, drifting and set back under the planks; a run's first module takes `tiep=1` and its last `tieat=2`, so both ends sit on one); its own knobs, clamped, with the owner's photo as defaults: `size_x` (100-400, 200), `size_y` (100-400, 300), `age` (0-2, 1.6), `board_w` (8-40, 24), `board_t` (1.2-16, 12), `gap` (0.1-2 cm; hairlines: the edges wander WITH their gap, never opening it), `cup` (0-1 cm), `steps` (0-1.5: thickness steps, tilt, ends up or down), `edges` (0-3 cm edge wander), `gouge` (0-4 cm), `end_stagger` (0-20 cm; not `stagger`, which is `add`'s own argument), `stones`, `cluster`, `grit`, `tiep` (share of modules with a support, 0.75), `tieat` (0 start, 1 end, 2 both: a run's last module, 3 drawn), `rope` (0/1), `missing` (share of modules missing a plank), `loose` (share of planks with a sprung end), `budget` (0 game: the beams' checks and grain left to the baked maps, 1 high, 2 ultra), `noise`, `fseed`; past age 1.6 the wood's own damage grows with (age/1.6) squared; underneath it is `Deck` with knobs any `Deck` takes: `gouge` (cm: 2-4 long irregular V gouges a plank, each its own place, wander, length and depth), `lips` (cm: the top edges broken off in runs), `hollow` (cm: a shallow worn hollow), `edgew` (cm: both long edges wandering, each gap with its own wave that both its planks follow), `stones` / `stonesc` / `grit` / `stonesz` (single stones, one cluster and grit, each settled on the planks by G31; slot `Stone`), `ties`, `tie_w`, `tiep`, `tieat`, `rope`, `ropes` (cross timbers under the stringers at a module end, in a share `tiep` of modules, lashed with round rope turns; slot `Rope`), `missp` (the share of modules missing a plank), `tiein`, `tiex` (the supports' ends set back under the planks; their drift in from the join, cm), `jdecay`, `jcheck`, `jends` (the joists and beams old: checks, eroded grain, ends off square, breakouts), `tvar`, `bttaper`, `bpitch`, `plift` (thickness steps, raised and sunk ends); the debris piles settle (G31) with `heap`; a survivor across a hole cracks through (`sthru`, `sease`). Shipped build lines: `SM_WoodDeck_<Intact/Weathered/Broken/Wrecked/Broken_300>` (game, maps baked from `_High`), `_High`, `_Ultra` (not Weathered), and the debris lines | `gc_describe preset:"WoodRuin"` |
| one aged dressed stone (a step, a kerb, a coping, a slab) | `add($SK.AgedBlock, l=, d=, h=, damage=, age=, softness=, noise=, detail=)`: `l` 2-600, `d` 2-200, `h` 2-100 cm (80 x 40 x 16); `damage` 0-2 (0.5 breaks a front corner off, 1.5 also cracks it); `age` 0-2 multiplies its wear (1); `softness` 0-1 (0 hard, the default; 1 worn round); `detail` 1 hero / 0 game; the advanced knobs, clamped, in the header | same |
| the fresh break painted in its own material | `material($Broken) [cut]`: every break face wears `cut`, and the modifiers paint nothing themselves | `block:"surface"` |
| wood grain kept in the UVs | map AFTER the break, each piece along its own length: a boolean throws UVs away, so the `Damage` entries write it for you: the skin box-projected with the grain along +Y, the torn faces split by facing (`uv(plane, …) [cut, facing=±X]` and the rest along the grain, end grain last), 100 cm a tile, `aspect=` the along-grain tile over the across one; if you turn the board with `rotation=`, pass `fibre=` (the grain direction) and `face=` (the wide face's normal) in the mesh's own axes too, and `upright=1` when the grain stands up +Z, because the entry cannot read them out of `rotation=`; `Debris` maps each piece in its own frame and writes `uv(keep)` | `block:"surface"` |
| a reinforced concrete floor-slab module, aged, breached or folded and broken into plates (a kit module on a 200 cm grid) | `BC = shape('/GraphCodeShape/Presets/BrokenConcrete', Face = $Cast, Broken = $Aggregate, Steel = $Rebar)` then `add($BC.SlabBroken, size_x=, size_y=, thick=, damage=, age=, noise=, budget=)`, the same on `$BC.SlabIntact` or `$BC.SlabWrecked` (and `$BC.SlabBrokenDebris`, `$BC.SlabWreckedDebris` with the same knobs for the matching rubble). TOP-LEVEL knobs, clamped: `size_x`, `size_y` (100-400 cm, 200), `thick` (10-30 cm, 15), `damage` (0.3-2, 1: breach radii, bar fates and sag; fold pitch, drop and torn ends), `age` (0-2, the kit's one scale: 0 new, 1 aged, 2 decayed; `$BC.SlabIntact` defaults to 1, `SlabBroken` and `SlabWrecked` to 1.8, decayed all over and then broken; in geometry, the skin out of plane 0.5 cm x age on a field periodic in the module so modules tile, chipped arrises, pop-outs, scaling patches with torn, lobed, stepped outlines and, above 1 at budget 1+, shrinkage-crack grooves, each growing with age; 0 a new cast skin), `softness` (0-1, 0: crisp breaks, the default; 1 worn smooth: rounded arrises and scars, eased rims; independent of `age`, the break faces stay crisp, the debris ignores it), `noise` (the seed), `budget` (0 game, 1 high, 2 ultra). Every advanced knob (bars, cover, breach shape, fold, plates, cracks, torn ends and their crater and scab, `tend_crater`, `tend_scab`, skin) is `Slab`'s, with its unit, range and default in the preset's header table | `gc_describe preset:"BrokenConcrete"` |

The frame for every `Damage` wood entry: the grain runs along +Y, the width along X, the thickness up +Z, the board
lying on z = 0. The board is on the −Y side of the break and what comes off is +Y. `origin=` is the point on the
board's bottom face, mid-width, where the break crosses; for a board centred at the origin with its +Y end at y = 100,
a snap 40 cm from that end is `origin=(0, 60, 0)`. A slanted break is `rotation=(0, 0, angle)`; keep it under about 20
degrees, because past that the break face no longer spans the board. Call these entries with no bracket. Bevel the
board before breaking it, never after.

### Build line: collision, light meshes, checks

| You want | Write | Ask |
|---|---|---|
| simple collision for a prop | `[collision=$Solid]` on the build line; a build without it says so | `block:"build"` |
| a light mesh for mobile, VR or a non-Nanite target that still reads detailed: its normal, AO, curvature or height maps baked from the full-detail recipe | `SM_X5k = $X [triangles=5000, maps=(normal, ao, curvature), resolution=2048]`: the detail is `$X` before the budget, or `from=$XDetail`; the maps land beside the mesh as `T_X5k_Normal` and so on. The line reports the silhouette error, missed rays, how full the UV square is and the normal jump at UV seams. `supersample=` (1-4, default 2; `1` turns it off) bakes at N times the size and averages down, so a baked edge reads as its share of each texel. A material instance's `NormalMap` parameter names the texture once | `block:"build"` |
| a tiling texture (wood grain along a board) AND that mesh's baked maps on one mesh | the same line with `uv_channel=1` (or `auto`): UV0 keeps the author's tiling layout, the maps get their own unique layout in UV1, and the material samples them with `TexCoord[1]`; the line says `on UV1`. Agent-spelled 2026-11-18 | `block:"build"` |
| that light mesh losing only what is shallow: engraving, ticks, small fillets and rivets go into the maps, while anything deeper (bosses, silhouette rings) stays as real geometry | `imprint(below=0.6)` in the body (after the lines it marks), `imprint(below=1.0) [in=$Emblem]` for a region, `imprint(none) [in=$Bolts]` to keep; the light line is `SM_XLite = $X [maps=(normal, ao)]`. A relief line (`raise`, `displace`, `roughen`, `round`, `add`, `subtract`) no deeper than the mark on its faces is left out. The line reports triangles light/full, the outline error (refused above the largest mark) and features flattened/kept. A shallow feature narrower than 2.5 texels at the light build's texel density is kept and reported `narrower than maps`; `narrow=flatten` on the mark overrides. A preset's own marks come with `add` | `block:"imprint"` |
| the shape checked against reference pictures (a blueprint, a side photo) | `Ref = reference(side="Refs/side.png", length=455)` and `[reference=$Ref]` on the build line; each view's note gives both sizes and the worst gaps with where. To LOOK at it the same way, `gc_render ... reference:"Refs/side.png"` writes the picture beside each view | `block:"build"` |
| whether two surfaces will flicker | read `coincident` on the build line; it is absent from a clean one | [ASSET-QUALITY.md](ASSET-QUALITY.md) |
| a mesh that must ship clean (no folds through itself, no self-overlap, no pinched edge) | `[folds=0, crossings=0, nonmanifold=0]` on the build line: the build, dry run included, is refused naming each fault and where it is. Without it the same counts are on the build line and never refuse | `block:"build"` |
| whether a texture will stretch on the shape | read `stretch` (above 1.1) and `uvfolds` on the build line, and the UV note's density against your `scale=`; `[stretch=1.3]` on the build line makes it a contract | `block:"build"` |
| the parts of a placed assembly checked for overlap, floating pieces and open seams, with the contacts you expect named | the editor-Python helper `Tools/Inspection/gc_check_assembly.py`: `gc_check_assembly.scan(allowed_penetrations=[{a, b, max_volume_cm3, reason}], support_roots=, seam_probes=)`. It is a script over the selected actors | [ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md) |

## Crisp lines inside a surface (declared creases)

Smooth surfaces are the default, and that is usually what you want. When a line has to stay crisp (a moulded
shell's parting line, a chamfer that runs out, a brow ridge, a tyre groove), you declare the crease on the
profile that shapes it. You do not paint an edge afterwards.

### The two marks

A stop of a `profile()` can carry a mark:

| Mark | Meaning |
|---|---|
| `@sharp` | A corner. Each side heads straight at its neighbouring stop. |
| `@(dx, dy)` | The slope `dy/dx` at the stop, on both sides. It never makes a corner. |

```gcshape
Side  = profile(0 = 0, 0.2 = 0.15, 0.35 = 0.4@sharp, 0.7 = 0.9, 1 = 1.2)   # a crisp shoulder at 0.35
Shade = profile(0 = 0, 0.6 = 3@(1, 0), 1 = 3.4)                            # level at 0.6: flattens, then rises
Hinge = profile(0 = 0@sharp, 1 = 1)                                        # a corner where the reading leaves the held end
```

Rules worth knowing:

- Only the two segments that meet at a `@sharp` stop change. Every other value is the unmarked profile's.
- `@(dx, dy)` is a line, not an arrow: `(-1, -3)` is the same as `(1, 3)`. Only its direction is read.
  `dx = 0` (straight up) is refused.
- A `@sharp` on the first or last stop, or on a `[linear]` profile, changes no value: those are corners already.
  It declares where the crease is when a clamp holds the argument at the end (see the Leg example below).
- A profile on a percentage, such as `0%@sharp`, reads the mark before the `%`.
- You cannot put `@sharp` and a heading on the same stop, use any other mark (`@round`, `@0.25`), or mark a step
  (two stops at one `t`). The build refuses each with a sentence.

### What a `warp` does with a creased profile

Every reader of the profile gets the corner in its values. A `warp` goes further. Wherever its formula reads a
profile that has a `@sharp` stop, the build:

1. finds the line where the argument reaches the stop (for `$Side(point.t.z)`, the ring at `t.z = 0.35`);
2. cuts that line into the mesh as a row of edges, before the move, so the corner lands exactly on it;
3. marks the row as a hard edge where the moved surface visibly turns. A stretch of the line is marked when it turns
   by about a quarter of a degree or more and the line turns by about half a degree or more somewhere along it.

There is nothing to switch on. Declaring `@sharp` is the opt-in, and a warp that reads no `@sharp` stop builds as
it always did. Two reads of the same profile at the same argument count as one line.

Where the surface barely turns, the cut is made but the edge is not marked, so it shades by `shade:` like the rest
of the surface. The build note says which lines were marked and which were not. That is how a ridge fades out
along its length: the line carries on, and the mark stops where the turn is too small to see.

A marked crease is a hard edge from then on. `shade:` cannot soften it, `round(r) [sharp]` rolls it, and `remesh`
and `subdivide` drop it, so declare creases after any `remesh`.

No line is cut, and the build says so, when:

- the argument reads `point.normal`;
- the argument is a constant;
- the argument touches the stop without crossing it. `abs(u)` at a corner at 0 is the usual case. Write a signed
  argument with the corner inside the profile instead; that is how a V is declared, and it lets the two sides differ;
- the line is narrower than the mesh can resolve. Give the surface more points (a finer `remesh`).

Each line adds about two triangles per crossing, up to a ceiling of 4,000 or half the chosen triangles, whichever
is larger. A warp that would exceed it is refused with a note, and so is a formula that refuses on both sides of a
cut corner (guard it, for example `sqrt(max(0, …))`).

`displace` and `taper` read a creased profile too, but only through its values: they do not cut a line.

### Worked example: a moulded shell with a parting line

Two drafted halves meet at a parting line. `Part` is the line's height along the length. `Wall` drafts each half
away from it, with a corner at 0.5. The argument is signed so it crosses 0.5 exactly where `z` meets `Part`.
No loft can draw a line that loops round a body; this does.

```gcshape
gcshape 1  into:/Game/GraphCodeExamples/SharpWarp  smooth:0.1  shade:40

build {
  SM_Shell = $Shell
}

Part = profile(0 = 1.6, 0.55 = 1.9, 1 = 2.6)
Wall = profile(0 = 0.94, 0.5 = 1@sharp, 1 = 0.94)
Shell = mesh() {
  rect(w=11.5, h=6.2)  round(2.8)  extrude(height=3.8)  round(1.4) [top]  round(0.4) [bottom]  remesh(0.25)
  warp(to=(point.x * $Wall(clamp((point.z - $Part(point.t.x)) / 8 + 0.5, 0, 1)), point.y * $Wall(clamp((point.z - $Part(point.t.x)) / 8 + 0.5, 0, 1)), point.z))
}
```

### Worked example: a chamfer that runs out

`Chamfer` is the chamfer's depth along a table leg. Every point beyond the plane `|x| + |y| = 5 - Chamfer` moves
back onto it, so each chamfer face is flat where the depth is constant. `Hinge` has its corner at its held end:
inside the plane the clamp holds the argument at 0, beyond it the value rises. `round()` cannot draw a chamfer
whose width runs out.

```gcshape
Chamfer = profile(0 = 0, 0.08 = 0, 0.2 = 0.6, 0.8 = 0.6, 0.92 = 0, 1 = 0)
Hinge   = profile(0 = 0@sharp, 1 = 1)
Leg = mesh() {
  rect(w=5.0, h=5.0)  extrude(height=72.0)
  warp(to=(point.x - 2.5 * $Hinge(clamp((abs(point.x) + abs(point.y) - 5 + $Chamfer(point.t.z)) / 5, 0, 1)) * point.x / max(abs(point.x), 0.001), point.y - 2.5 * $Hinge(clamp((abs(point.x) + abs(point.y) - 5 + $Chamfer(point.t.z)) / 5, 0, 1)) * point.y / max(abs(point.y), 0.001), point.z)) [smooth=0.8]
}
```

### Worked example: a brow ridge that fades out

A V that the two sides do not share (an overhang). `Brow` is steep below the crest and eases off above it, with a
corner at the crest. `Lift` fades the ridge to nothing toward the temples; the cut goes on there, unmarked.

```gcshape
BrowLine = profile(0 = 2.2, 0.3 = 3.4, 0.5 = 2.9, 0.7 = 3.4, 1 = 2.2)
Lift     = profile(0 = 0, 0.15 = 0, 0.5 = 1.1, 0.85 = 0, 1 = 0)
Brow     = profile(0 = 0, 0.1 = 0, 0.5 = 1@sharp, 0.8 = 0.15, 1 = 0)
BrowZone = mesh() { rect(w=14.0, h=16.0, at=(7.0, 0.0))  extrude(height=8.0) }
Head = mesh() {
  arc(radius=12.0, from=-90, to=90)  revolve()  remesh(0.35)
  warp(by=$Lift(point.t.y) * $Brow(clamp((point.z - $BrowLine(point.t.y)) / 5 + 0.5, 0, 1))) [in=$BrowZone, fade=1.5]
}
```

Tyre grooves work the same way: give each groove a profile whose every stop is `@sharp`, read it at one argument per
groove, and the build cuts and marks one ring per lip and floor
(`Groove = profile(0 = 0@sharp, 0.5 = 1@sharp, 1 = 0@sharp) [linear]`).

### Creases on curves and outlines

On a `curve()` or `points()` outline the same marks work on the keys: `@sharp` makes a corner and a hard swept
edge, `@(dx, dy)` sets the heading through a key. See the lookup table above and
[SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md#4-keep-a-crisp-edge-inside-a-surface) for choosing between them.

## Lofts that follow your lines

### Keyed loft: names pair across sections

When the sections of a loft name their keys, the loft pairs points by name instead of searching for the best
rotation. Each name becomes a column that runs from section to section. `@sharp` keys give a hard column, soft
keys a smooth one. Sections need not have the same number of points between keys.

```gcshape
Transom = plane() { curve(SheerR: (42, 62)@sharp, (28, 63), ChineR: (14, 56)@sharp, Keel: (4, 0), ChineL: (14, -56)@sharp, (28, -63), SheerL: (42, -62)@sharp, loop=true) }
Mid     = plane() { curve(SheerR: (46, 70)@sharp, (30, 72), (20, 68), ChineR: (12, 60)@sharp, Keel: (0, 0), ChineL: (12, -60)@sharp, (20, -68), (30, -72), SheerL: (46, -70)@sharp, loop=true) }
Bow     = plane() { curve(SheerR: (60, 10)@sharp, ChineR: (30, 6)@sharp, Keel: (14, 0), ChineL: (30, -6)@sharp, SheerL: (60, -10)@sharp, loop=true) }

Hull = mesh() {
  add($Transom)
  loft(through=($Mid, $Bow), heights=(170, 300))
}
Boat = mesh() { add($Hull, rotate=(0, 90, 0)) }
```

The sections are drawn as `(height, across)`, so `rotate=(0, 90, 0)` lays the hull along -X.

Rules:

- If any outline names a key, every outline must (an unnamed section beside named ones is refused).
- A name must appear in the same cyclic order on every outline it is on. A name on only one outline is refused,
  and the message suggests the nearest spelling.
- A feature that starts or stops along the body leaves its name off the sections where it is absent. The loft
  carries it as a soft point between its neighbours.
- Keys written at one point on a section weld, so a fin can run out into a body. A section whose keys all meet at
  one point closes there and takes no cap.
- Outlines must be closed and without holes. Two centre lines, or halves beside full outlines, are refused.

### Lines along the loft: `rails=`

A `rails=` argument gives a named key a line that places it at every height, taken straight from your blueprint's
side and top views. The sections then only describe the shape between the lines.

```gcshape
loft(through=($S1, …), heights=(…), rails=(G: ($Gz, $Gy), P: (X: $PodTop), Kl: $KeelPath))
loft(heights=(30), rails=(…))          # no through=: the start outline is the one section
```

A line is written one of three ways:

- two profiles, `G: ($U, $V)`: the section's first and second coordinate at each height. For a section drawn as
  `(height, across)`, the first is the height;
- one profile and an axis, `P: (X: $U)` or `(Y: $V)`: the other coordinate comes from the sections;
- a 3D path, `K: $Path`: `(u, v, height above the start)`, rising.

A profile is read at `t = height / last height`. If a section's key is off its line, it is moved onto the line
when it is within the larger of `smooth:` and 1 cm (the build note says by how much); beyond that the build is
refused as "the wrong height or the wrong key".

Worked example, a square bottle: one section carried the whole way, with three lines.

```gcshape
Square = plane() { curve((4, 0), (4, 2.4), K: (3.4, 3.4), (2.4, 4), Sc: (0, 4), (-2.4, 4), KB: (-3.4, 3.4), (-4, 2.4), (-4, 0), loop=mirror) }

Face       = profile(0 = 4, .6 = 4, .72 = 3, .8 = 1.4, 1 = 1.4)
Corner     = profile(0 = 3.4, .52 = 3.4, .66 = 2.3, .8 = 1, 1 = 1)
CornerBack = profile(0 = -3.4, .52 = -3.4, .66 = -2.3, .8 = -1, 1 = -1)

Bottle = mesh() {
  add($Square)
  loft(heights=(30), rails=(K: ($Corner, $Corner), Sc: (Y: $Face), KB: ($CornerBack, $Corner)))
}
```

The shoulder line runs round the corners lower than on the faces, which is what makes a bottle read as moulded
rather than turned. A key with no line stays where the start section has it.

A pram dinghy shows several lines at once, with a chine that runs into the keel:

```gcshape
Hull = mesh() {
  add($Transom)
  loft(through=($Mid, $Bow), heights=(120, 240), rails=(Dk: (X: $SheerZ), Sh: ($SheerZ, $Beam), Ch: ($ChineZ, $ChineY), Kl: (X: $Rocker)))
}
```

`rails=` is refused on `loft(to=)`, on a chained loft, on unnamed outlines, for a name no outline writes, for a
name given two lines, and for a line that is flat, closed, or turns back in height. `rails=` lines take no marks.

### Loft quality: what the loft does for you

You do not tune these; they explain what you will see. The first three apply only to a loft with `rails=`; a loft
without `rails=` builds exactly as it always did.

- **Columns follow the true curve.** A named `curve()` outline keeps its curve, and the loft's points are placed on
  that curve rather than on a coarse polyline of it. In a loft with `rails=` the curve is also cut finely enough
  that no piece turns more than about 2 degrees.
- **Soft keys do not become creases.** The loft adds points around a soft key until each facet turns at most about
  8 degrees. A smooth crown or shoulder therefore shades smooth and shows no ridge.
- **Sharp corners cost no extra points.** A `@sharp` key's corner (and a corner of a `points()` outline) is not
  counted as turning, so points do not pile up beside it.
- **Density is bounded.** A facet is never asked to be shorter than about 1.5 times `smooth:`. A span that would
  need more than 255 points is built at 255 and named in the build notes (this ceiling applies to every loft).
  In a loft with `rails=`, the 60,000-triangle cap limits only how far the 8-degree rule may raise the count:
  when holding every facet to 8 degrees would pass it, the loft loosens the limit until it fits, or falls back to
  the distance-only count, and the build note says so ("loosened ... to stay under the loft's cap of 60000
  triangles").

Worked example, a product shell (a toaster or kettle body). The crown `Cr` and shoulder `Sd` are soft; the parting
line `Pl` and the foot `Bk` are `@sharp`, so `Pl` to `Bk` is a straight chamfer that needs only one facet.

```gcshape
S0 = plane() { curve(Tc: (22, 0), Cr: (21.2, 5)@(0, 1), Sd: (16, 9.6), Pl: (7, 10)@sharp, Bk: (0.6, 8)@sharp, Bc: (0, 0), loop=mirror) }
S1 = plane() { curve(Tc: (24, 0), Cr: (23, 5.5)@(0, 1), Sd: (17.5, 10.6), Pl: (7.5, 11)@sharp, Bk: (0.6, 9)@sharp, Bc: (0, 0), loop=mirror) }
S2 = plane() { curve(Tc: (20.6, 0), Cr: (20, 4.5)@(0, 1), Sd: (15, 8.6), Pl: (6.5, 9)@sharp, Bk: (0.6, 7.4)@sharp, Bc: (0, 0), loop=mirror) }

CrownZ = profile(0 = 21.2, .25 = 22.5, .5 = 23, .75 = 22.1, 1 = 20)
PlZ    = profile(0 = 7, .5 = 7.5, 1 = 6.5)
PlY    = profile(0 = 10, .25 = 10.7, .5 = 11, .75 = 10.4, 1 = 9)

Shell = mesh() {
  add($S0)
  loft(through=($S1, $S2), heights=(18, 36), rails=(Cr: (X: $CrownZ), Pl: ($PlZ, $PlY)))
}
```

Without these rules the crown would crease about 24 degrees and the shoulder about 29. With them, each turns under
8 degrees, using about 44 columns instead of 12.

## Ropes: declare the path, drape it

A rope is the path it takes. There is no tie preset: you write the path in a `plane()`, word by word or point by
point, `drape(...)` pulls it onto what it rests on, and Rope's `Line` follows it. Every word appends guide points in the
order written, so words, a `loop` of words and raw points mix freely. The lowest level is the points themselves: a loop
writes `$g[i] = (x, y, z)` and `points($g)` takes the list. Ask each word for its arguments
(`gc_describe kind:"shape" block:"around"`); this section only shows them working together.

| word | appends |
|---|---|
| `around($M, turns=, pitch=, side=)` | turns side by side round a member, or round a bundle (`around($A, $B)`) |
| `over($M)` / `under($M)` | a half turn on the side seen from `up=` (default +Z) / on the side away from it |
| `at($M, angle=)` | one point on the member's surface |
| `between($A, $B, turns=)` | frapping turns between two members, round the turns already there |
| `tuck(len=)` | the end laid under the last ring, held |
| `to((x, y, z), ...)`, or `points(...)` after a word | raw points |

Every point on a member is read from its real section, so a crooked, tapered, oval or square member is followed as
built; `off=` says how many rope radii out the guide stands (1.2; a start laid under later turns takes 0.5). Where on
the member: `t=` (0 to 1 along it) or `at=$Other` (where it crosses Other), then `beside=+1|-1` and `ring=k` (the k-th
rope out past Other's real surface) and `from=` (cm along it). `r=` goes on the first word and later words keep it.
`hold` holds a word's points through the drape; an end a word holds is not also pinned.

A square lashing, rung (A) over rail (B):

```gcshape
Lash = plane(r=0.5) {
  at($B, at=$A, beside=-1, ring=7, angle=60, r=$r, off=0.5, hold)     # the start laid along B, under the turns
  at($B, at=$A, beside=-1, ring=1.5, angle=60, off=0.5, hold)
  loop (repeat=4) {
    over($A, at=$B, beside=1, ring=output.index)
    under($B, at=$A, beside=1, ring=output.index)
    over($A, at=$B, beside=-1, ring=output.index)
    under($B, at=$A, beside=-1, ring=output.index)
  }
  between($A, $B, turns=2)
  tuck(len=4)
  drape(onto=($A, $B), r=$r, taut, self)
}
Tie = mesh() { add($Rp.Line, r=0.5, len=520, sp=1.2, join=none) follow(along=$Lash) }
```

The other common ties, each a few words:

| tie | the path |
|---|---|
| wrap round one pole | two `at($Pole, t=, angle=, off=0.5, hold)` (the start laid along it), `around($Pole, turns=8, side=-1)`, `tuck(len=5)`, `drape(onto=$Pole, r=, taut, self)` |
| round lashing, two poles touching | the start, `around($P1, $P2, turns=7)`, `tuck(len=6)` |
| shear lashing, two poles apart | the start, `around($P1, $P2, turns=6)`, `between($P1, $P2, turns=2)`, `tuck(len=5)` |
| binding a bundle | the start, `around($S1, $S2, $S3, turns=5)`, `tuck(len=4)` |
| over a beam, both ends hanging | `to((-30, 0, 60), r=0.8)  over($Beam)  to((30, 0, 40))  drape(onto=$Beam, r=0.8, length=260, pin=(), friction=0.8)` |
| a span between two points (raw points only) | `points((0, 0, 200), (300, 0, 180))  drape(r=0.6, slack=1.12)` |

Rules that hold: `taut` needs both ends held or pinned; with `self` a run guided inside a later one stays under it, so a
start meant to lie under the turns is guided at `off=0.5` under turns at 1.2; build the `Line` near the drape's length
(its note says it) with stations under 40 degrees of turn round the tightest member (`sp=`), or `shade:40` creases it.

## Welding shells that cross

`add($B, join=hard)` is a union with a crease, and it is the default when you write no `join=`. Two closed shells
that cross (a canopy sunk into a car body, a tube leaning through a slab, a pipe pushed into a hub) can leave
open edges or seams where the boolean could not decide. The build repairs this in two steps, only when the plain
union is damaged:

1. **Re-cut.** It balances the two operands, so the cut triangles are of comparable size, and runs the boolean
   again. It keeps the new result only if it is strictly better.
2. **Weld.** If seams remain, it cuts each shell by the other along the crossing curve, reconciles the points
   where the two cut rims meet, and joins them. The result is closed.

A union that is already clean is never touched, so files that built clean before build exactly as before. The build
note says what happened: the rim loops it seated and the final `open` and `seam` counts. If the weld gives up, the
note says why in one clause and keeps the first result.

You write the same thing you always did. Here a fine tube leans 30 degrees through a coarse slab; the plain union
of the two leaves seams, and the weld makes it one closed shell (the note reports `open 0, seam 0`):

```gcshape
gcshape 1  into:/Game/GraphCodeExamples/Weld  smooth:0.5  shade:40

build {
  SM_Tube = $Tube [pivot=(0, 0, 0)]
}

SlabSec = plane() { rect(w=100.0, h=40.0) }
Slab = mesh() { add($SlabSec) loft(through=($SlabSec, $SlabSec, $SlabSec, $SlabSec, $SlabSec, $SlabSec, $SlabSec, $SlabSec), heights=(25, 50, 75, 100, 125, 150, 175, 200)) }

TubeRing = plane() { circle(radius=15.0, segments=800) }
TubePart = mesh() { add($TubeRing) loft(through=($TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing, $TubeRing), heights=(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120)) }

Tube = mesh() { add($Slab) add($TubePart, at=(3.3, -60.0, 101.7), rotate=(120.0, 0.0, 0.0), join=hard) }
```

Limits: the weld covers `add` only (not `subtract` or `intersect`), a single `add` rather than each copy of a
`repeat=` array, and parts that are not `topology=preserve`. The repair has its own allowance per file; with no
room left it is skipped with a note. It never refuses a file. If you need a hard guarantee, put
`[folds=0, crossings=0, nonmanifold=0]` on the build line.

## The same file builds the same mesh

A `.gcshape` builds the same mesh every time: the same points and triangles in the same order. The engine's voxel
`remesh` and `join=blend` can number their points differently from run to run, so Shape puts the result in a
canonical order straight after them, and its repair passes choose by position rather than by point number. In
practice a rebuild gives you the same crease lines, seams and findings, and saved assets and comparisons between
builds do not drift because of thread timing.

Limit: two coincident copies of one shell cannot be told apart by position and may number differently from build
to build.

UVs, materials and polygroups are carried across a `remesh` and `join=blend`, but vertex colour is dropped. Write
`colour()` after them, as the table says.

## UVs and tangents (`uv(none)`)

A painted body wants `uv(none)`. The automatic UVs a rebuild or a boolean leaves cut islands across smooth panels,
and Unreal draws a faint line at every cut. Keep UVs only on the parts that carry a texture.

A shape with `uv(none)` has no UV set, so Shape writes no tangent frame for it and the engine fills one in when it
imports the mesh. The tangents are therefore the same on every build. If you bake maps from a
`uv(none)` shape, the bake lays out its own UVs and writes tangents for the light mesh.

## A car or a product body, step by step

A vehicle, an appliance or a moulded product uses the spellings above together. This order avoids rework.

1. **Measure first.** Side, top and front outlines from your references ([ASSET-QUALITY.md](ASSET-QUALITY.md)),
   as numbers you can state. Finish one piece, then the rest.
2. **Build the mass.** Either state it as curves (a plan width and a roofline as `profile(…)`s, a rounded slab
   given points with `remesh(4)  relax(1)`, and one `warp(to=…)` that reads them at `point.t.x`; `$Form.Tumblehome`
   leans a cabin in), or skin stations with `loft(through=…)` and lines with `rails=`. Curves give the smoothest
   surface, because there are no stations to ripple between.
3. **Declare the crisp lines** you want inside the surface as `@sharp` stops on the profiles the warp reads, after
   the `remesh`.
4. **Join what should read as one surface** with `join=blend` before you round or cut it, and add pieces that
   touch into one entry before a `round` ([SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md#3-order-the-work-so-the-finish-survives)).
   Where a part crosses the body (a canopy), use `join=hard`; see [Welding shells that cross](#welding-shells-that-cross).
5. **Cut the openings** (doors, windows, lamps, vents) with closed cutters, then `round(…) [cut]` their edges.
6. **Mark the shut lines** as a `raise(by=-…)` brush stroke along each route, rather than a thin box.
7. **Put the glass in** as a thin closed solid with `join=none` and its own material. Do not leave an opening as a hole.
8. **Wheels**: `$Wheels.Tyre` (a revolved section whose tread a `warp` moves in, no boolean); lettering is an SVG `outline`.
9. **Declare the collision** with `[collision=]` on the build line, read `coincident` for flicker, and use `uv(none)`
   on a painted body.
10. **Judge it** from the front, side, back and three-quarter, and read the build's notes before the render.

## A kit is a recipe, not a generator

A kit or family preset (stairs, a deck, a slab, a timber run) is one hand-edited `.gcshape`. Do not write a Python or other script that prints the `.gcshape`, with every course, block and station unrolled into thousands of lines. A generated kit cannot be read, a fix means regenerating and diffing, and the unrolling hides the structure the grammar now states. (The shipped `StoneKit`, `TimberKit`, `WoodRuin` and `BrokenConcrete` were first generated, then rewritten by hand: 16,056 lines became 1,152 for `StoneKit`.)

**When to loop.** A block that repeats with an index (courses, blocks, stations, bars, planks, chips) is a `loop` over indexed names, with the differing values in one list. A value one pass leaves for the next is `$x[i]`; an outline is a points list; a field written twice is a `formula()`. Two copies that differ in a few lines are one `loop (2)` with the differing lines in an `if`. Leave unrolled only what the grammar still cannot index: a name chosen by an index, or a loop that steps a name suffix.

**Keeping draws stable under edits.**
- A seedless `random()` is keyed by the entry name and the file path, plus what its line is (the verb, the part it names, its argument names, or the set's name, its place among identical siblings, and its `if`/`loop` arm and pass). It is not keyed by its line number, so a comment, a blank line or a moved fold no longer re-rolls it.
- The same text built at another address (a copy, a dry run at a scratch path named after the copy) is another file, so every seedless draw re-rolls. To see what an edit changed, build both versions at ONE address.
- Renaming an entry, or merging two entries into one with a flag, DOES re-roll every seedless draw in it. Prefer an explicit `seed=` on every draw that must survive such a change.
- A set's boundaries matter: a name that holds a sub-expression is a different build than the same expression inline whenever a boolean is near-degenerate. Since G28 a set keeps full precision, so naming is safe; in an older file the rounding is the bug.
- Inside a loop, give each pass its own draw with `seed=$noise * 1009 + output.index`; one seed shared by every pass gives every copy the same number.

**Proving a rewrite.** Build the old text and the new one under the SAME package path (separate runs), then compare triangles, vertices, bounds, triangles per slot and area, on every build line and on extra lines at other knob values (other seeds, `age` 0 and 2, the largest size, `damage` high). Two paths means two draws, and a seedless draw then differs with the text identical.

## What to read for your shape

Read for every `.gcshape`:

| Read | For |
|---|---|
| [ASSET-QUALITY.md](ASSET-QUALITY.md) | the bar a shipped model is held to: references first, one finished piece, then expand |
| [ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md) | choosing the method: grammar for structure, SVG for curves, texture for organic detail |
| [SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md) | bevels, crisp edges and protected mating faces |
| [ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md) | parts that touch, sockets, seams, support, z-fighting |

Then, by what the shape is:

| The shape | Read | Shipped example |
|---|---|---|
| a prop a hand holds (a handle, a sword, a tool, a cup, a gun) | [HANDS.md Step 4](HANDS.md#step-4-build-the-prop-to-fit-the-hand-gcshape) (its origin at the grip point in the grip socket's frame, a `socket` per place a finger or palm meets), [The seat is never skipped](HANDS.md#the-seat-is-never-skipped); give it a closed simple collision solid for the part the hand closes on (`[collision=$Solid]` on the build line), which grips read with `surface(part=..., solid=collision)` | `KitchenCup_20261011/Cup.gcshape` (`CupSolid`), `VRHandRig_20261001/Sword.gcshape` |
| a prop that is thrown, dropped or simulated | [MOVE-PHYSICS.md](MOVE-PHYSICS.md): physics reads only simple collision | `KitchenCup_20261011/Cup.gcshape` |
| furniture, trim or buildings | [SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md), [ROADS-AND-BUILDINGS.md](ROADS-AND-BUILDINGS.md) | |
| parts that sit on other surfaces | [ASSET-QUALITY.md §7](ASSET-QUALITY.md#7-parts-must-not-share-a-surface) (the join and seat for each `add`), [ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md); for moss, grout or a material blend where they meet, `rule(condition=contact)` in `gc_describe kind:"shape" block:"weightage"`. ([SURFACE-ALIGNMENT.md](SURFACE-ALIGNMENT.md) is a character's foot on a slope, not a shape.) | |
| organic parts joined to each other | [ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md) | |
| armour or clothing on a body | [GARMENT.md](GARMENT.md) | |

**Socket frame convention.** A Shape `socket(..., facing=..., up=...)` stores its facing on the socket's local +Z
and its up direction on +X. Read the socket's frame, not an assumed Unreal actor-forward axis, when attaching a
prop or making a Move `frame(socket=...)` from it. An explicit `toward=`/`up=` on the Move frame states which of
those local directions the action uses.

## Before you call it done

Read the build's notes (overlaps, folds, open edges, collision pieces), then render the shape from several sides
and, for a held prop, in the hand ([VALIDATION.md](VALIDATION.md)). Three notes are about what a render will not
show you:

- **Left/right.** A shape that is nearly mirror-symmetric (a car, a helmet) is told where its sides differ: how far,
  where, and the mirror point. If the sides should match, build one side and place the other with `mirror=Y` (or `X`).
- **Side lines.** A long, left/right-symmetric shape (a car, a hull, a blade) is told where its lines change the way
  they bend: the top line, a side-view line and three waterlines, read at the scale a person sees. A designed body
  shows a few on its roofline and none on its waterlines; anything else is a ripple, and the note says where.
- **Nanite fallback.** A Nanite mesh is also drawn from a reduced fallback wherever Nanite cannot draw it:
  translucent slots such as glass, ray tracing, platforms without Nanite. When that fallback strays from the built
  shape, the note says by how much and gives the `.gc` settings that keep every triangle.

## Common mistakes

- **Negating `at=` to mirror a part.** It only mirrors a part centred on its own origin. Use `mirror=Y`.
- **Writing `abs(u)` to make a V.** A corner at the argument's zero is only touched, not crossed, so no line is
  cut. Use a signed argument with the `@sharp` stop inside the profile.
- **Declaring a crease, then `remesh`ing.** `remesh` and `subdivide` drop the hard-edge mark. Declare it after.
- **Expecting a mark on a faint turn.** A crease line that never turns by about half a degree is cut but shades by
  `shade:`. The build note lists it. If you need it crisp, make the profile turn more.
- **Limiting a warp with `[in=$Zone]` on a smooth surface.** The zone border can leave a visible shading seam.
  Prefer `[above=]` where the shape allows it, or fade with `fade=`.
- **A rounded slab with too few points before a warp.** The warp can only move the points it has. Give the slab
  points (`remesh(4)  relax(1)`) and a finer `remesh` where lines must be resolved.
- **A chain of `loft(to=)` when you wanted one smooth body.** A chain keeps every station hard. Use
  `loft(through=…)`.
- **Naming a key on one section only.** A keyed loft refuses it. Name it on every section it is on, and leave it
  off the sections where the feature has not started.
- **Giving a key a line it is far from.** A section key more than the larger of `smooth:` and 1 cm off its `rails=`
  line is refused. Check the height and the key name.
- **Cutting a tread with a boolean.** It grazes the curve and leaves slivers. Use the `Wheels` preset, which moves
  the tread in with a `warp`.
- **Relying on automatic UVs for a painted body.** Use `uv(none)` and keep UVs only where a texture lives.
- **Writing `colour()` before a `remesh` or `join=blend`.** They drop it. Write it last.
- **Expecting `join=hard` to weld a surface that is not closed.** An open surface is refused, not quietly appended.

## Reference

| Spelling | Where | What it does |
|---|---|---|
| `P = profile(t = v, …)` | profiles | a curve of numbers over `t` 0..1; `$P(x)` reads it |
| `v@sharp` | a profile stop, a curve key | a corner; on a profile a `warp` that reads it cuts and marks the line |
| `v@(dx, dy)` on a profile stop | profiles | the slope dy/dx through the stop, both sides; only the direction is read; never a corner |
| `(x, y)@(dx, dy)` on a curve key | a curve key | the heading through the key; its direction sets the way it runs and its length (0 to 3) scales the curve's own handle; never a corner |
| `Name: (x, y)` | a curve or points key | names a key so lofts pair it and `rails=` can line it |
| `curve(…, loop=mirror)` | an outline | a half that closes through its reflection; twins named `G'` |
| `loft(to=$B, at=)` | a body | one span from the top face; chains keep every station hard |
| `loft(through=(…), heights=(…))` | a body | one smooth skin through several outlines |
| `loft(…, rails=(K: …))` | a body | lines from the blueprint place each named key at every height |
| `warp(by=)` / `warp(to=)` | a body | moves each point by a formula; cuts declared creases first |
| `add($B, join=hard)` | a join | union with a crease; repaired by re-cut and weld where shells cross |
| `add($B, join=blend, falloff=)` | a join | a smooth melt over the distance |
| `add($B, join=fillet, radius=)` | a join | a constant-radius fillet |
| `add($B, join=none)` | a join | a separate shell |
| `round(r) [region]` | an edge | a curved edge; `segments=1` is a chamfer |
| `uv(none)` | a body | no UV cuts; no tangent frame is written |
| `[folds=0, crossings=0, nonmanifold=0]` | build line | refuse a build with those faults |
| `[collision=$Solid]` | build line | simple collision |

Coming later: a crease with a different heading on each side of a stop, and creases that also shape the surface
where other operations read them. Neither is available yet, so do not write them.
