# Feathers: stylised solids and realistic flat cards

*A GraphCode guide. Read [ANIME-HAIR.md](ANIME-HAIR.md) first ("Working in the editor", "Seeing it", the material table in "The look", the painted tiles paragraph); this page adds only what differs for feathers. The tool's own help is the grammar: `gc_describe kind:"groom"` and `{ "help": true }` on `gc_apply`. If the help and this page disagree, the help is right. The help lines for `stiffness(rigid)` and `solid()` were published on 12 Nov 2026 at 16:58. The realistic feather below is in the Groom build published on 12 Nov 2026 at 20:10 (earlier parts 18:17). The help lines for the row word `turn($R, ...)` ship with a later publish, so the help may not list it yet.*

Two kinds of feather, both in rows on a simple arm mesh. **Stylised:** `solid()` rigid pieces, the toon material, an edge line on each feather (the first half of this page). **Realistic:** flat `card()` sheets that wear a painted cut-out of a whole feather through the shipped lit material `M_GcCardMasked`, with a normal map the tool derives from the tile (section "Realistic feathers").

**Not available yet** (each was checked against the plan on 12 Nov 2026):

- No folding and no animation of a wing. No body feather patches.
- An unpainted cut-out tile of a rigid entry draws a grey feather the build generates (a quill, a rounded vane, the shaft at the entry's `skew` on the tip side, barbs at 35 degrees); it takes the instance's `Tint`, and a painted tile replaces it. Published 12 Nov 20:33 and tested, but NOT yet seen in a render, so how a wing of generated feathers looks is unverified.
- Rows on a curved base. Each rigid root takes its own triangle's face normal, so on a curved arm neighbours can part along their length. How a row should share its lay direction on a curved base is not ruled yet and not built. Only a flat slab has been built.
- The realistic wing has only been seen with test-generated leaf tiles, never with painted feather tiles.
- Its underside reads very dark in the preview render (one light from above, no sky light). It has not been judged in a daylit level.

**Which verb.** Three entry verbs, one per shape. `strand()` is a line. `solid()` is a closed swept volume: anime hair locks and the stylised feathers. `card()` is a flat cut-out sheet: the realistic feather. A recipe in the old spelling (`card()` with a closed section, `card=$X` on the layer) still builds the same, and the answer adds one `spelling:` line naming the successor.

Three stylised wings were built by authors who had never used the tool, on 12 Nov 2026 (`Content/__MCPTests/GroomFeathers/`): `AngelWing` with hair words, `AngelWingRigid` with the new word, `AngelWing2` with look changes. The last is the recipe below: 24 feathers, 5,928 triangles, 6 builds. The realistic fixtures are `FlatFeatherRow` (9 cards) and `SpreadWing` (89 cards).

## You want -> write (stylised)

| You want | Write | Note |
|---|---|---|
| a feather that lies flat and straight | `stiffness(rigid)` in the `solid()`; `flow(direction=(0,-1,0), from=$Band, weight=100%, lift=3%)` in the layer | no `depth=`, no holding `bend` |
| the fan toward the tip | per-lock `turn=`: Flight 0, 4, 10, 19, 28, 40, 54, 68, 82; Middle 0 to 78; Covert 0 to 76 | 0 at the shoulder, rising faster toward the tip. `tips spread` read 82.0, 78.0, 76.0 degrees (rigid wing). Without lock lists: the row word `turn` below |
| which feather lies on top in a row | `stack=against` on the `layer(...)` | the feather nearer the path's first point (the shoulder) is on top. 48 overlaps sampled, the shoulder side on top in all |
| the leaf shape | `Oval = profile(0=35%,0.3=72%,0.6=100%,0.8=88%,0.92=56%,0.98=20%,1=0%)`, `width(8.0, profile=$Oval)` | no `[...]` modifier on it |
| a slight sweep | `Sweep = profile(0=0%,0.45=0%,1=100%)`; `bend(angle=8, axis=out, profile=$Sweep)` after `flow` (6 for Middle and Covert); `skew(38%)` on the solid | the long feathers bow sideways from 45% of their length. For `skew` see the next section |
| colour, lift and tones | `flow(..., lift=3%)`, 5%, 8% for Flight, Middle, Covert; feather instance `CardVariation 0.35`, `EdgeDarken 0.35`, `HighlightStrength 0` | lift 0% lies in the surface, 100% stands on its normal; some feathers read slightly dirty grey |
| an edge line on every feather | `outline=$Line` in `cards(...)`; outline instance `DepthPush 0.05`, `OutlineWidth 0.4` | 0 looked the same as 0.05; 0.3 lost a few short pieces |
| the arm covered | `OverrideMaterials=(...MI_Wing2Feather...)` on the base component of the viewing Blueprint | colour only: the arm is still a straight bar |

## How a rigid feather behaves (solid or flat)

- It starts at its root on the surface and runs straight. It does not follow the target after the root, does not sag, and goes on past the target's edge. A straight blade measured 0.0000 cm off its chord (one fixture).
- Its direction is the layer's `flow` direction laid into the surface's tangent plane, turned by the lock's `turn=` (about the surface normal at the root, anticlockwise seen from outside), then raised toward the normal by `flow(lift=)`.
- **`skew` on a rigid row** (a `count=` row or lock lines): under 50% it narrows the side of every piece that faces the path's LAST point (the wing tip side), whatever `turn=` says. This is the published build (20:10), checked by a spec on the swept mesh. Before 20:10 it narrowed the shoulder side. `AngelWing2` below was built before and has not been rebuilt. Soft hair solids are untouched. On a flat cut-out card the visible shaft is wherever the tile paints it.
- `bend`, `twist` and `skew` act on top, as on any solid (`bend(axis=across)` on a rigid feather was not tried). `depth=` on a rigid layer is refused: the error says "remove depth=".
- A row shingles by itself: each feather rolls about its own length by the least angle (at most 15 degrees) that lets its overlapping edge clear its neighbour. Flat sheets shingle too (a row of nine, `against` and `along`, passed its spec). `stack=` says which neighbour is on top: `against` the one nearer the path's first point, `along` the one nearer its last, `centre` the middle one.
- A later layer lies on an earlier one by the real thickness where they overlap: write the layers that must be underneath first.
- A rigid root takes its own source triangle's face normal (since 20:10), then the distance field's gradient, then the blended vertex normal; its shading normal is that normal rolled with the feather. A rigid piece that no flow owns takes the nearest flow and the report names the cards; a rigid layer with no `flow` is refused.
- Painted tiles (`paint=<size>` on `cards(...)`): a rigid `solid()` gets a feather prompt in greys that `Tint` colours; a flat `card()` with `paint(alpha)` gets the cut-out prompt below. Each piece wears its whole tile, unmirrored. No painted feather tile has been rendered on a stylised wing.

## The base

A wing grows on a mesh; a recipe with no mesh is refused. The authors' slab is `Content/__MCPTests/GroomFeathers/AngelWing/WingBase.gcshape` (`rect(w=55.0, h=11.0)`, `round(5.0)`, `extrude(height=2.5)`, `round(0.8)`; make it as [GCSHAPE.md](GCSHAPE.md) says). Measure which way it lies: 55.0 x 11.0 x 2.5 cm; length is X (-27.5 at the shoulder to +27.5), width Y (+-5.5), up is +Z (top face at z 1.25). From above (camera azimuth 0, elevation 89) the shoulder is at the top of the picture and feathers point to screen right (-Y). Write roots on the top face (`z 1.25`).

- **The header is one line.** `into:` on its own line gave "line 2: 'into' is not an entry".
- **The rounded end is the one place a root leaned.** The plan measured 0.86 degrees on this slab before the first root fix and 0.000 after it. Tertials rooted 2.4 cm from the rounded end still leaned 1.9 degrees until the 20:10 face-normal rule (fault 9). The stylised wings here predate both and are not re-measured.

## The stylised wing, step by step

1. Make the base, then the two material instances (ANIME-HAIR.md, "The look"). Whether instances can be made before the first build was not recorded.
2. Three layers, **flight row first and coverts last**: Flight 9 feathers, 30 to 45 cm long, 7 to 9 cm wide; Middle 8, 18 to 22 cm, 5.5 to 6.5 cm; Covert 7, 9 to 12 cm, 4 to 4.5 cm. Each row is a band (a path from the shoulder end to the tip end, `widen(6)`) and one `lock(at=, width=, length=, turn=)` per feather.
3. Dry run (`dry_run:true`), build with `create:true`, then the viewing Blueprint (it cannot exist before the feather mesh). Render above (azimuth 0, elevation 89), below (0, -89), three-quarter (-50, 35), along the arm (0, 8).

Which neighbour is on top, seen from above (`Docs/Research/feather-card-construction-practice.md`): primaries, secondaries and greater and lesser coverts have the inner feather on top, `stack=against`. Median coverts are outer on top in many species (`stack=along`, untried). Lesser over median over greater, all over the flight bases: write the flight row first. A tail half has the centre pair on top (`stack=centre`, no tail built).

## The stylised recipe (AngelWing2, as written)

`Content/__MCPTests/GroomFeathers/AngelWing2/AngelWing2.gcgroom`. This is its Flight row only, to keep the page short: Middle and Covert are the same pattern (a `solid()` of width 6.0 and 4.5, a band at y -1 and 1.5, the lock numbers in the file, `lift=` 5% and 8%, `bend(angle=6, ...)`); the full file has all three. Its three comment lines are left out. Change the three asset paths to your own.

```gcgroom
gcgroom 1 mesh:/Game/__MCPTests/GroomFeathers/AngelWing/SM_WingBase asset:/Game/__MCPTests/GroomFeathers/AngelWing2/GR_AngelWing2 gen:1 tolerance:0.02
Feather = material('/Game/__MCPTests/GroomFeathers/AngelWing2/MI_Wing2Feather')
Line = material('/Game/__MCPTests/GroomFeathers/AngelWing2/MI_Wing2Outline')
Oval = profile(0=35%,0.3=72%,0.6=100%,0.8=88%,0.92=56%,0.98=20%,1=0%)
Sweep = profile(0=0%,0.45=0%,1=100%)
Thin = profile(0=100%,0.9=100%,1=40%) [linear]
FlightCard = solid() {
  width(8.0, profile=$Oval)
  thickness(0.2, profile=$Thin)
  section(crescent, bow=6%)
  skew(38%)
  stiffness(rigid)
}
FlightBand = region() {
  points((-23,-3,1.25),(25,-3,1.25),loop=false)
  widen(6)
}
Flight = layer(region=$FlightBand, solid=$FlightCard, length=40, seed=11, material=$Feather, stack=against) {
  lock(at=0%,   width=7.0, length=30.0, turn=0)
  lock(at=12%,  width=7.5, length=32.5, turn=4)
  lock(at=25%,  width=8.0, length=35.5, turn=10)
  lock(at=37%,  width=8.0, length=39.0, turn=19)
  lock(at=50%,  width=8.5, length=43.0, turn=28)
  lock(at=62%,  width=9.0, length=45.0, turn=40)
  lock(at=75%,  width=9.0, length=45.0, turn=54)
  lock(at=87%,  width=8.5, length=42.0, turn=68)
  lock(at=100%, width=7.5, length=38.0, turn=82)
  flow(direction=(0,-1,0), from=$FlightBand, weight=100%, lift=3%)
  bend(angle=8, axis=out, profile=$Sweep)
}
build {
  Flight = $Flight [cards=Wing, angle=5]
  SM_AngelWing2Cards = cards(Wing, angle=5, lods=1, normals=auto, outline=$Line)
}
```

The file on disk is still in the old spelling (`card()`, `card=$...`) and builds the same. Instances (make them as ANIME-HAIR.md "The look" says):

- `MI_Wing2Feather`, parent `/Game/GraphCode/Groom/M_GcToonHair`: `Tint` (0.92, 0.94, 1.0), `ShadeTint` (0.62, 0.76, 1.0), `LightDirection` (0.3, -0.2, 0.9), `UseFixedLight` 1, `UseProxyNormal 0`, `CardVariation` 0.35, `EdgeDarken` 0.35, `HighlightStrength` 0, `Debug` 0.
- `MI_Wing2Outline`, parent `/Game/GraphCode/Groom/M_GcToonOutline`: `OutlineColor` (0.2, 0.3, 0.5), `OutlineWidth` 0.4, `DepthPush` 0.05.
- The viewing Blueprint holds `Base` (the base mesh, `OverrideMaterials` = `MI_Wing2Feather`) and `Cards` (`SM_AngelWing2Cards`) under a `SceneComponent` root.

## Realistic feathers: flat cut-out cards

A realistic feather is a flat sheet that wears a painted picture of the whole feather. Seen on `FlatFeatherRow` and `SpreadWing`, on the 12 Nov 2026 builds, with test-generated leaf tiles.

**The entry** is the `Primary = card() { ... }` of the recipe below: `width(...)`, `thickness(0.1)` (required by `card()`, unused on a flat sheet), `section(flat)`, optional `skew(...)`, `stiffness(rigid)`, `paint(alpha, note='...')`. Name it on the layer with `card=$Primary`, write `paint=1024` in `cards(...)`, roots on the top face of the base. A flat card takes no outline: with `outline=` written the answer says `outline <Mesh>: slot X holds flat card() sheets ...` and drops the outline hull for that slot.

**The material.** A slot is masked when every layer in it is `section(flat)` with `paint(alpha)`. The build then parents the slot's painted instance to `M_GcCardMasked` (made from the shipped recipe the first time) unless `material=` already is that material or an instance of it. The build's answer says so, in this line (quoted from the tool's source; the saved report files do not hold build notes):

```
paint <Mesh>: slot <Slot> holds only flat card() sheets with paint(alpha), so its painted instance is parented to <M_GcCardMasked path> (lit, two-sided, cut out where the magenta was, with the normal atlas), not to <the material= you wrote>; set Tint, BackTint, NormalStrength, Transmission, Roughness or AlphaClip on that instance, or write material= naming an instance of <M_GcCardMasked path>
```

The toon material's settings are not carried over. Parameters of `/Game/GraphCode/Groom/M_GcCardMasked` (Masked, two-sided, Two Sided Foliage):

| Parameter | Default | What it does |
|---|---|---|
| `Paint`, `PaintNormal` | white square, flat normal | the painted atlas (unpainted draws as `Tint`) and the normal atlas derived from its brightness |
| `Tint` | white | a realistic tile is painted in its own colours |
| `BackTint` | white | multiplied on the back face |
| `AlphaClip` | 0.33 | cut-out threshold, under a half so thin tips survive |
| `NormalStrength` | 1.0 | 0 is flat, 1 is the atlas |
| `Transmission` | 0.35 | share of colour a vane passes when backlit |
| `Roughness` | 0.65 | matte with a soft sheen |

The normal map reads brightness as height: dark gaps between barbs are grooves, bars are ridges (the atlas measured bars as ridges; a render cannot tell ridge from groove). Brightness 0 to 1 is 12 texels of height, so on against off differs in about 60,000 pixels on `SpreadWing`.

**Painting the cut-out.** `paint=` writes `Paint/<CardsName>/PROMPT.md`, `template_<Kind>.png` and `overview.png` beside the recipe. Open `Paint/Wing/PROMPT.md` (the `SpreadWing` one is a worked example): it says how to paint and gives one prompt per kind. In short:

- The WHOLE feather, its outline included, on flat pure magenta (#FF00FF) that fills everything outside it, in its own natural colours (`Tint` stays white). No magenta or pink inside, no shadow, no glow, no soft edge.
- The quill touches the top edge at the entry's `skew` share from the left (50% with no `skew`). One straight pale shaft runs from there to the tip at the bottom edge. The vane is widest about a third down, at about 90% of the width. Only the quill touches an edge.
- Keep dark gaps between barbs, since they become grooves. Save as `painted_<Kind>.png` in that folder, rebuild, and read the `paint` lines: each tile says painted or placeholder.
- A tile is rejected, and stays a placeholder, when it is the untouched template or the magenta covers under 5% or over 95%.
- Sizes: at `paint=1024` the `SpreadWing` primary tile is 144 x 976 pixels in the atlas and the covert tile 96 x 144. At `paint=512` the `FlatFeatherRow` tips showed slight stair-steps. Nothing more is measured.

## A fan and a row in one line each

A `count=` row replaces lock lists. Row words read a region that carries `profile(..., domain=along)`, so a number follows the row. For a band's path the weight is the profile at the root's share along the written points (first point 0, last point 1), on rigid layers.

- `turn($Region, value=<degrees>)`: each card turns by value times the profile weight. A lock line's own `turn=` wins. Refused as a percentage and on a strand layer.
- `length($Region, value=|scale=)`: refused after the layer's first `flow(...)`. The refusal text: "length(...) is a population instruction and must precede flow/styling instructions: write it, length($Region, ...) included, before the layer's first flow(...); the card row words width, thickness, twist, skew and turn($Region, ...) may stand anywhere in the body".
- `width($Region, scale=)`: on a 0 to 100% profile, `scale=50%` runs from full width at the first card to half at the last (the plan's spec: 1 - s/2 for scale 50%).
- A lock line's own value always wins over a row word.

The two layers of `SpreadWing` that use them, exactly as written:

```gc
Primaries = layer(region=$PrimaryBand, card=$Primary, count=10, length=30, seed=21, material=$Feather, stack=against) {
  length($PrimaryLength, value=45)
  flow(direction=(0,-1,0), from=$PrimaryBand, weight=100%, lift=0%)
  turn($PrimaryFan, value=80)
}
Secondaries = layer(region=$SecondaryBand, card=$Secondary, count=14, length=22, seed=22, material=$Feather, stack=against) {
  length($SecondaryRamp, value=30)
  flow(direction=(0,-1,0), from=$SecondaryBand, weight=100%, lift=0%)
  turn($SecondaryRamp, value=6)
}
```

## The realistic recipe (SpreadWing, as written)

`Content/__MCPTests/GroomFeathers/SpreadWing/SpreadWing.gcgroom`, on the base above. Its comment lines (they cite internal plans) are left out. Change the asset paths to your own, and make the base first. Six rows: primaries on the tip half, secondaries on the shoulder half, tertials at the shoulder, then three covert rows each nearer the leading edge (+Y).

```gcgroom
gcgroom 1 mesh:/Game/__MCPTests/GroomFeathers/AngelWing/SM_WingBase asset:/Game/__MCPTests/GroomFeathers/SpreadWing/GR_SpreadWing gen:1 tolerance:0.02
Feather = material('/Game/GraphCode/Groom/M_GcCardMasked')
Ramp = profile(0=0%, 1=100%) [linear]
Peak = profile(0=0%, 0.6=100%, 0.8=100%, 1=70%) [linear]
Primary = card() {
  width(6.0)
  thickness(0.1)
  section(flat)
  skew(38%)
  stiffness(rigid)
  paint(alpha, note='a dark brown eagle primary with a pale shaft and faint bars')
}
Secondary = card() {
  width(5.5)
  thickness(0.1)
  section(flat)
  skew(38%)
  stiffness(rigid)
  paint(alpha, note='a dark brown eagle secondary, broad and rounded at the tip')
}
Tertial = card() {
  width(5.0)
  thickness(0.1)
  section(flat)
  stiffness(rigid)
  paint(alpha, note='a brown tertial, soft and rounded')
}
Covert = card() {
  width(4.0)
  thickness(0.1)
  section(flat)
  stiffness(rigid)
  paint(alpha, note='a brown covert feather with a lighter edge')
}
PrimaryBand = region() {
  points((4,-3,1.25),(25,-3,1.25),loop=false)
  widen(6)
}
PrimaryFan = region() {
  points((4,-3,1.25),(25,-3,1.25),loop=false)
  widen(6)
  profile($Ramp, domain=along)
}
PrimaryLength = region() {
  points((4,-3,1.25),(25,-3,1.25),loop=false)
  widen(6)
  profile($Peak, domain=along)
}
SecondaryBand = region() {
  points((-19,-3,1.25),(2,-3,1.25),loop=false)
  widen(6)
}
SecondaryRamp = region() {
  points((-19,-3,1.25),(2,-3,1.25),loop=false)
  widen(6)
  profile($Ramp, domain=along)
}
TertialBand = region() {
  points((-26,-3,1.25),(-20.5,-3,1.25),loop=false)
  widen(6)
}
GreaterBand = region() {
  points((-25,-0.5,1.25),(25,-0.5,1.25),loop=false)
  widen(6)
}
GreaterFan = region() {
  points((-25,-0.5,1.25),(25,-0.5,1.25),loop=false)
  widen(6)
  profile($Ramp, domain=along)
}
MedianBand = region() {
  points((-24,1.5,1.25),(24,1.5,1.25),loop=false)
  widen(6)
}
LesserBand = region() {
  points((-23,3.5,1.25),(23,3.5,1.25),loop=false)
  widen(6)
}
Primaries = layer(region=$PrimaryBand, card=$Primary, count=10, length=30, seed=21, material=$Feather, stack=against) {
  length($PrimaryLength, value=45)
  flow(direction=(0,-1,0), from=$PrimaryBand, weight=100%, lift=0%)
  turn($PrimaryFan, value=80)
}
Secondaries = layer(region=$SecondaryBand, card=$Secondary, count=14, length=22, seed=22, material=$Feather, stack=against) {
  length($SecondaryRamp, value=30)
  flow(direction=(0,-1,0), from=$SecondaryBand, weight=100%, lift=0%)
  turn($SecondaryRamp, value=6)
}
Tertials = layer(region=$TertialBand, card=$Tertial, count=3, length=18, seed=23, material=$Feather, stack=against) {
  flow(direction=(0,-1,0), from=$TertialBand, weight=100%, lift=0%)
}
Greater = layer(region=$GreaterBand, card=$Covert, count=24, length=9, seed=24, material=$Feather, stack=against) {
  flow(direction=(0,-1,0), from=$GreaterBand, weight=100%, lift=0%)
  turn($GreaterFan, value=50)
}
Median = layer(region=$MedianBand, card=$Covert, count=20, length=6, seed=25, material=$Feather, stack=against) {
  flow(direction=(0,-1,0), from=$MedianBand, weight=100%, lift=0%)
  width($MedianBand, scale=85%)
}
Lesser = layer(region=$LesserBand, card=$Covert, count=18, length=4, seed=26, material=$Feather, stack=against) {
  flow(direction=(0,-1,0), from=$LesserBand, weight=100%, lift=0%)
  width($LesserBand, scale=70%)
}
build {
  Primaries = $Primaries [cards=Wing, angle=5]
  Secondaries = $Secondaries [cards=Wing, angle=5]
  Tertials = $Tertials [cards=Wing, angle=5]
  Greater = $Greater [cards=Wing, angle=5]
  Median = $Median [cards=Wing, angle=5]
  Lesser = $Lesser [cards=Wing, angle=5]
  SM_SpreadWingCards = cards(Wing, angle=5, lods=1, normals=auto, paint=1024)
}
```

The build's report for it (20:11): `cards SM_SpreadWingCards LOD 0: cards=89 triangles=468`, `crossings=0 touching=0`, `rigid Primaries: shingle 1.50 degrees ... highest 0.16 cm above the surface at its roots, crossings 0`, Primaries `tips spread 72.0 degrees`, lengths 31.25 to 45.00 cm.

## Reading the report

An all-rigid target adds one line per layer, `rigid <Layer>: shingle <degrees>, highest <cm> above the surface at its roots, crossings <n>`, and this line (quoted from the rigid wing's report):

```
not a head: every card layer is rigid (blades such as feathers), so the measures of hair on a head are not taken
rigid Flight: shingle 8.15 degrees (...), highest 1.42 cm above the surface at its roots, crossings 0
```

- A good wing: `crossings=0 touching=0` and a small `highest`: 2.3 to 2.7 cm on the stylised wing, 0.07 to 0.33 cm on `SpreadWing`. `coplanar` is large for flat sheets 0.02 cm apart: 732.90 cm2 on `FlatFeatherRow` (four stills showed no z-fighting) and 3,060.50 cm2 on `SpreadWing` (flicker not judged). Neither was seen moving.
- `scalp`, `outside_head`, `cap`, `uniform`, `parallel`, `long`, `standing` and `tip_up` are not printed. If they are, a layer is not rigid.
- The `tips` lines in the report file (`Saved/GraphCode/GroomCardReports/`) give each piece's root and tip. Tip z 0.6 to 1.3 over a root at 1.25 is flat; tips at z -24 to -39 were hanging feathers.
- `rigid <Layer>: N pieces no flow owned took the nearest flow (cards a, b)` (for example `rigid Row: 2 pieces no flow owned took the nearest flow (cards 1, 9)`) names the pieces the tool placed for you.
- A capped row prints `rigid <Layer>: capped: card A stays X cm short of clearing card B under it at 15 degrees: their roots' surface planes differ by D degrees, ...` (form from the plan, not a real report line).

## Judge it

**Stylised wing**, the ten rows of the first wing's brief, from the four views. No row-by-row verdict was written for AngelWing2; this is its log, report and the lead's reading of the pictures. 1 reads as a wing from above: yes, plain leaf-shaped feathers. 2 overlap one way: yes (48 sampled overlaps). 3 rows in order, no bare root stubs: yes, `crossings=0`. 4 lengths smooth along the flight row: yes by the written 30 to 45 to 38 cm. 5 fan opens toward the tip: yes by `turn=`, spread 82 degrees. 6 nothing through a neighbour or the base: yes. 7 tips rounded or pointed: pointed leaves yes. 8 outline without doubling: an edge line on every feather; doubling not recorded. 9 from below no black patch: arm white above and pale blue below; holes not recorded. 10 base covered: partly, still a straight bar with a rounded end. Expect: no shaft or vane detail (flat white and grey); pointed tips, because a rounder tip (`0.92=66%`, `0.98=32%`) came out faceted; every feather sweeps by one rule; the arm is a slab.

**Realistic wing** (`SpreadWing`, built and rendered once on the 20:10 build, 12 Nov 2026, with test tiles):

| What | Seen | Verdict |
|---|---|---|
| Layout | primaries fan (`tips spread` 72.0 degrees in the report), secondaries near parallel, three shorter covert rows; the longest primaries are second and third from the tip; nothing stands, hangs or crosses | the rows, the fan word and the row words work |
| Reads as a wing | a spiky fan in one flat brown: the test tiles are thin spears with a white line, every kind one colour | NOT yet a bird wing: the look rests on painted tiles |
| Gaps | black see-through triangles between fanned primaries just past the covert edge at the wrist; a grey slab between the short lesser coverts at the leading edge | fault: spear tiles leave the roots open; broad rounded tiles and longer coverts are the expected cure, not yet seen |
| `skew` | every shaft is centred | expected: the shaft is where the tile paints it, and the test tiles paint it in the middle |
| Cut-out edge | no pink in the wing; fewer and fainter dots on the overlap seams of the flat row | mostly fixed; a faint remainder |
| Normal map | bars and shaft read as relief in `detail_lighting` | works |
| Underside | readable, very dark | not judged (preview light) |

## Faults the authors hit, most costly first

1. **Hair pieces wrap and hang.** A soft piece leaving the base's edge follows the surface over it and hangs (tips 24 to 39 cm down). Fix: `stiffness(rigid)`.
2. **`depth=` under a thin base** put roots below the 2.5 cm slab and a curtain of feathers hung 25 cm beneath, with no report line (hair-word wing). Fix: no `depth=` on a rigid layer.
3. **Inner edge lines vanish.** The hair outline default `DepthPush` hides lines between pieces lying a fraction of a millimetre apart. Fix: 0.05 on the outline instance.
4. **`[smooth]` on a profile is refused:** "profile modifier '[smooth]' is not supported; use [linear] or omit it for smooth interpolation". Omit the modifier.
5. **`section(ridge, bow=10%)`** gave cross-cracks at the roots and no readable shaft under the toon light. The crack came from `lift=0%` with a `bend` on a rigid piece, now fixed (a spec passes; not looked at in a render). Fix for the shaft: `section(crescent, bow=6%)`; a 25% bow dropped an 8 cm card's edges 2 cm.
6. **Uneven tones.** With the proxy normal on, the hair-word wing's flight feathers drew in the shade tone and its middle feathers white. Every wing so far set `UseProxyNormal 0`.
7. **A rigid piece no flow owned stood up** along the surface normal (the two end cards of `FlatFeatherRow`, 18 cm high). Now it takes the nearest flow and the report says `rigid <Layer>: N pieces no flow owned took the nearest flow (cards a, b)`. A rigid layer with no flow is refused: "layer '<L>' is rigid (stiffness(rigid)) and writes no flow: a rigid layer needs a flow(direction=...) or flow(outward, ...) to say which way its pieces lie; without one each would stand along the surface normal".
8. **`length($R, ...)` after `flow(...)` is refused.** Write it before the layer's first `flow` (text above).
9. **Three tertials near the base's rounded end would not shingle:** one root leaned 1.9 degrees, so the blade dipped 0.6 cm over 18 cm and the roll ran to the 15 degree cap. Fixed: a rigid root takes its triangle's face normal. A capped row now names its pair (form above). On a curved base this can return (not available yet).
10. **A short row squeezed into too little room** (3 cards 1.17 cm apart, 5 cm wide) hit the 15 degree cap. The report's `capped` word names the pair and says when they overlap too far for a roll to part them. Fix, unverified: space the cards wider or make them narrower.
