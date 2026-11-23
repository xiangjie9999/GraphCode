# Author and fit a Garment

*A GraphCode guide. Read it when clothing is the current task; [`SETUP.md`](../../SETUP.md)
routes here.*

GraphCode Garment turns a body-relative clothing recipe into Unreal assets. Its source is a
`.gcgarment` document. Its results can be a static mesh, a skinned mesh on the source body's
skeleton, or a MetaHuman outfit with the wardrobe data needed to wear it. The recipe owns the
garment's construction intent; the generated mesh or outfit is a result to inspect, not a substitute
for that recipe.

This guide explains decisions and workflow. It deliberately does not copy the program grammar.
Start every authoring session with `gc_describe` for `kind:"garment"`, then ask `gc_apply` and
`gc_render` for their installed help. Those surfaces are the authority for current arguments,
supported operations and refusals.

## Where the detail lives

This guide is the entry guide. Sewn-on pieces, declared edge finishes and press direction are in
[GARMENT-PIECES-AND-EDGES.md](GARMENT-PIECES-AND-EDGES.md), with the status of each word (several were
added after the 10 Nov build). Armour is [below](#armour-is-a-garment); simulated cloth is under
[Fit, skinning and collision](#fit-skinning-and-collision-are-different-claims). A wrap skirt, its waistband, ties, hem
and darts are under [A skirt: wrap, band, ties, hem](#a-skirt-wrap-band-ties-hem); where a simulated skirt is held, under
[Fit, skinning and collision](#fit-skinning-and-collision-are-different-claims). The game cloth's solver words
on `hang()`, a hem held up while the garment hangs, and curtains are in
[GARMENT-HANG-AND-CURTAINS.md](GARMENT-HANG-AND-CURTAINS.md).

## You want → write

Each row names the spelling in one line; the last column is where it is explained. If the installed
description (`gc_describe kind:"garment"`) and a row disagree, the description is right. Rows marked
*added* are after the 10 Nov build; the build published on 17 Nov has all of them.

| You want | Write | Ask or read |
|---|---|---|
| a plain T-shirt, start to finish, with the file, the body and the pictures (the other common garments: hoodie, jeans, coat, jacket, trousers and more) | copy and build the shipped example | [Examples/TShirt](../Examples/TShirt/README.md); the rest in [Examples](../Examples/README.md#common-garments-worked-garment-files) |
| a pocket as its own piece of cloth (*added 13 Nov*) | `PocketR = patch($EdgeR1, $EdgeR2, $EdgeR3, $EdgeR4, $EdgeR5, open=$EdgeR1, turn=1cm, hem=1.27cm)` after drawing its five sides as `line()`s | [pieces and edges](GARMENT-PIECES-AND-EDGES.md#a-piece-of-cloth-sewn-on-patch) |
| a belt loop or a label cut raw (*added 15 Nov*) | `Loop1 = patch($L1Out, $L1Top, $L1In, $L1Foot, turn=0, ply)`, `layers=3` for a strip folded in three, then `stitch` rows on its sides and `tack($L1Top@0.5, length=0.95cm)` | [belt loop](GARMENT-PIECES-AND-EDGES.md#a-belt-loop-a-raw-strip-folded-to-plies) |
| clear space under a sewn-on piece (*coded 16 Nov*) | add `gap=0.1cm` to the `ply` patch | [gap](GARMENT-PIECES-AND-EDGES.md#clear-space-under-the-piece-gap) |
| a faced edge (*added 16 Nov; meshed as a turned edge until the mesher takes it*) | `rim($Low, faced(6cm, $Lining))` with `Lining = fabric(...)` declared | [edge kinds](GARMENT-PIECES-AND-EDGES.md#declare-the-kind-of-the-edge) |
| a hem turned twice with a narrow first turn (*added 16 Nov; `first=` is read, not yet carried*) | `rim($Low, turned(2cm, twice, first=6mm))` | [edge kinds](GARMENT-PIECES-AND-EDGES.md#declare-the-kind-of-the-edge) |
| one edge finished differently from the rest (*added 16 Nov*) | `construct(edge=turned(2.54cm, twice))` for the garment, then `rim($Low, raw)` for that edge | [edge kinds](GARMENT-PIECES-AND-EDGES.md#declare-the-kind-of-the-edge) |
| a seam allowance pressed to one side | `press($Armhole_L, -0.635cm)` | the seam row of the table below |
| a facing that follows the topstitching | `press($FlyL, -3.195cm, follow=($J1, ..., $J7), margin=0.68cm)` | [press direction](GARMENT-PIECES-AND-EDGES.md#which-way-a-pressed-seam-stands-plain-and-follow) |
| a pocket facing laid beneath the cloth (*experimental; no successful cut yet*) | `patch(..., turn=0, ply, under, gap=0.1cm, window=(...))` | [under](GARMENT-PIECES-AND-EDGES.md#a-piece-laid-beneath-the-cloth-under-and-window-experimental) |
| a woven fabric's look (*experimental, unrendered*) | `weave=twill(3/1, z), gauge=(26, 16)` on the `fabric()` line | [woven looks](GARMENT-PIECES-AND-EDGES.md#experimental-unrendered-woven-fabric-looks) |
| a wrap skirt (no wrap word yet) | the skirt cut open down a drawn line, `cut($Wrap)`; or a lap riding it, `Overlap = extend($Lap, 14)` off `Lap = line($Waist@0.06, $KneeHem@0.16)` | [a wrap skirt, today](#a-wrap-skirt-today) |
| a waistband that shows as a band | `band($Waist, width=4.13cm, fabric=$Twill, cut=100%)`; a turned `rim()` is the garment's own cloth turned inside and does not show | [a band shows](#a-band-shows-a-turned-edge-does-not) |
| a waistband that runs on into ties (no word yet) | a sash garment of its own, cut open, its two sides `extend`ed into ties and tied in a declared knot | [ties, today](#a-waistband-that-runs-on-into-ties-today) |
| a knot (square or half; no bow yet) | `Knot = constraint(at=, facing=)`, then `guide($EndA, $Knot@wrap, place)` and `guide($EndB, $Knot@wrap, place)` | [knots, today](#knots-today) |
| a curtain hung from a pole on hooks, with a pressed crease if wanted (*added 19 Nov*) | `Pole = track((x, y, z), (x, y, z))`, `Left = panel(width=250, drop=230, fabric=$Linen) { heading(hooks, on=$Pole, every=12.5cm) settle(time=3, slide=2) }`; built as a static mesh against a room mesh | [a curtain](GARMENT-HANG-AND-CURTAINS.md#a-curtain) |
| a narrow hem turned twice | `rim($Hem, turned(0.635cm, twice))` (*added 16 Nov*); older spelling `rim($Hem, turned=0.635cm, layers=3)` | [narrow hem](#a-narrow-hem-and-a-top-that-narrows) |
| a skirt's top shaped in to the waist | darts: `DartFROther = cut($DartFR)`, then `sew($DartFR, $DartFROther)` | [darts](#a-narrow-hem-and-a-top-that-narrows) |
| where a simulated skirt is held | `hang(from=$Hips)`, the widest round of the seat | [where to hold the cloth](#fit-skinning-and-collision-are-different-claims) |
| the game cloth's solver, said in the recipe (*added 19 Nov*) | `hang(from=$Hips, damping=5%, local_damping=30%, iterations=12, substeps=4, ccd, self=0.6cm, repair)`; `spacing=1.25cm` for finer simulated points | [solver words](GARMENT-HANG-AND-CURTAINS.md#hangs-solver-words-the-game-cloth-said-in-the-recipe) |
| a skirt or any garment fitted on a MetaHuman | `mesh:` naming the MetaHuman character asset, not its assembled body mesh | [step 3](#3-inspect-and-measure-the-real-body) |
| a tee pulled up a little, with creases: a hem lifted and held there while it hangs (*added 19 Nov; not yet looked at by the owner*) | `HemUp = ring((0, 10.2, 94.5), level) { expand(0.26) }`, then `settle() { guide($Hem, $HemUp) }`: the whole hem is taken to the ring and held, its spare cloth folding on its own; held all round, so it gathers into a band, and a few-point lift is not shipped | [the recipe](GARMENT-HANG-AND-CURTAINS.md#a-tee-pulled-up-a-little-with-creases) |
| a ring standing off the body, or a level ring | points typed on the skin, then `expand(1.5)` or `expand((0.93, 2.54))`; for a level ring every point at one height | [step 3](#3-inspect-and-measure-the-real-body) |
| a simulated skirt's collision, solver and material settings | nothing to write: the build sets it | [what the asset is written with](#fit-skinning-and-collision-are-different-claims) |

<!-- Sources: the same constructs, copied from the parser specs and example garments cited beside each example in GARMENT-PIECES-AND-EDGES.md. -->
## Place Garment in the character pipeline

Keep each authoring concern with its real owner:

```text
character presentation
├─ person and identity ................ .gchuman, or an existing body asset
├─ garment construction ............... .gcgarment
│  ├─ rings, silhouette and openings
│  ├─ fit, looseness, clearance and thickness
│  ├─ cloth shaping and garment-to-garment composition
│  └─ generated static, skinned or outfit result
├─ rigid or modelled garment parts .... .gcshape, referenced by the garment
├─ armour ............................. .gcgarment layers + .gcshape plates and rivets together
├─ hair and brows ...................... Groom or the character's own sources
├─ body and facial motion .............. Rig and Move
└─ what a MetaHuman is wearing ......... the character asset's .gc outfit block
```

The divisions matter:

- `.gchuman` makes or identifies the person. It does not own a wardrobe design.
- `.gcgarment` makes clothing against a named body. It does not change that body.
- A character's `.gc` chooses what the character wears. It does not own the rings, cuts or folds
  that made the clothing.
- `.gcshape` owns buckles, buttons, badges, spikes, pads and other modelled parts. Garment may place
  a Shape entry and pass it values, but should not copy the part's geometry into the garment source.
- `.gcmove` owns motion. A garment must be tested against that motion, but the garment recipe does
  not own the animation.

See [the character guide](CHARACTERS.md) when clothing must work with body motion, facial animation,
hair or MetaHuman assembly. See [the content guide](CONTENT.md) when choosing between authored,
existing and imported source content.

### Garment, mesh and character are not synonyms

| Thing | Role |
|---|---|
| Body mesh | The surface and, when present, skeleton the clothing is authored against. It is an input and is not modified. |
| `.gcgarment` | The revisable construction source: body-relative rings, pieces, fit and build choices. |
| Built garment mesh | A generated Unreal result. It can be rendered and inspected, but its vertices cannot reconstruct the recipe that made them. |
| Imported clothing mesh | Existing fixed content. Importing or fitting it does not turn it into a parametric `.gcgarment` design. Keep its original source and licence. |
| `.gchuman` | A person's identity and body-generation intent. It does not own the wardrobe. |
| MetaHuman character | A character asset that can wear generated wardrobe items and supply a refittable outfit body. |

Use an existing or imported clothing mesh when the fixed design is what the project wants. Use
Garment when the AI must own and revise the construction: longer sleeves, another neckline, a wider
hem or a different fit. Do not promise that GraphCode can recover those decisions from arbitrary
triangle geometry.

## Keep a clean, layered construction source

Characters and models should normally be made in parts. Preserve a clean base body and head, then
keep hair, brows, clothing and accessories as separate sources. Here, **bare** means uncluttered for
construction. It does not prescribe nudity or the final presentation.

Fit a garment against that clean body because it gives every later stage an honest reference:

- skin weights come from the body under the cloth;
- covered-body masks can be derived without another outfit already hiding the surface;
- overlap between a shirt, trousers, coat, belt and accessories can be allowed explicitly;
- collision and clearance are measured against the body rather than an unknown earlier layer;
- each layer keeps independent materials, detail levels and simulation policy;
- the layered sources survive even if a final character is baked or merged for delivery.

Do not use a previously assembled MetaHuman mesh as the only fitting source when its current outfit
has already removed covered body geometry. Use the complete construction body. The dressed character
is still essential later, but as a presentation and motion test.

Preserve all layered sources before any final bake. A merged delivery mesh cannot recover the body
surface, garment recipe, hidden-body decision or independent material and simulation boundaries that
were collapsed into it.

## Choose the result for the job

The output kind is a deformation and reuse decision, not a quality ladder.

| Result | Use it for | What it does not do |
|---|---|---|
| Static mesh | Shape, silhouette and still-pose proofs; rigid clothing or a fixed prop | Follow a skeleton or behave like fabric. |
| Skinned mesh | Clothing for the source body that should follow its skeleton | Refit itself to unrelated bodies or collide like simulated cloth. |
| MetaHuman outfit `[as=outfit]` | A wardrobe item that Epic's outfit pipeline can refit to another rigged MetaHuman | Become simulated merely because the result is a Chaos cloth asset. |
| Simulated cloth `[as=cloth]` | Clothing that drapes, swings and collides through Chaos; on a MetaHuman, a simulated wardrobe outfit | Work without a declared held region, the ChaosClothAsset plugin and the body's Physics Asset. |

A GraphCode MetaHuman outfit built `[as=outfit]` is **render-only clothing**: a skinned render mesh
plus the source body needed by Epic's refit path. It has no simulation mesh. An `[as=cloth]` build
adds a simulation mesh of the garment's middle, held where `hang(from=$Ring)` says. On a MetaHuman,
that build also goes through the wardrobe path. The engine generates the simulation pattern.

The MetaHuman build also derives the wardrobe item and covered-body maps. The character then wears
that item through its own `outfit` block. Keep that last step separate from building the garment: one
creates reusable clothing; the other chooses a character's current presentation.

## A safe authoring workflow

### 1. State the clothing job

Record the body, reference pose, clothing type, expected outer and inner layers, required openings,
camera distance and target result. Decide whether the garment is a still proof, skinned clothing for
one body, a refittable MetaHuman outfit, or simulated cloth. Do not add simulation just because the
project has Chaos.

For production work, choose one representative garment and one representative character first. A
T-shirt, skirt or pair of trousers exposes fit, openings, skinning, masks and materials more usefully
than a library of uninspected outputs.

### 2. Discover the installed surface

Read the current Garment description and the apply/render help before writing. A `.gcgarment` is a
whole upstream program, not a `.gc` projection: it has no asset signature, binary signature or
coverage declaration. Apply the whole document through the supported program route. Do not invent a
verb from an older design note, and do not turn a refusal into hand-written syntax.

The current installed description covers body-derived rings and copied floating rings; profiles;
multi-ring garments; fit, clearance and thickness; cuts, additions and joins; construction and
relaxing; cloth extension and sewing; guided and tightened free cloth; gathered, pleated and eased
fullness; optional Shape parts; and static, skinned and MetaHuman outfit results. Ask its narrower
branches for the exact current form.

### 3. Inspect and measure the real body

Render the actual source body in its authoring pose. Use the render tool's supported hit-query mode
to obtain surface points for necklines, cuffs, armholes, waist, hem, face opening and other landmarks.
Write those measured points into the recipe. Example coordinates from a fixture belong to that
fixture and are not a fit for another body.

Confirm scale, axis, pose, complete body coverage and skeleton before building clothing. On a
MetaHuman, use the complete merged head/body source used for fitting, not only the visible assembled
geometry. A body with hidden torso faces may still look correct while being unusable as the skin and
collision reference.

**Fitting on a MetaHuman.** An assembled MetaHuman's body mesh has no skin under the clothes it wore
when it was assembled: the outfit's body mask takes that geometry out (on one test character,
nothing above about 130 cm but the arms). A ring measured there finds nothing, and the build refuses it as
"has a point off the body" or "found no closed ring round the body through its point". Name the
MetaHuman character asset itself as `mesh:` instead, as the shipped SkirtFabrics example does
(its header names a test character there; write the path of your own character asset): it is read as its head and body merged, at
rest, detail level 0, welded, with small pieces such as eyes and teeth left out, and the build's note
says what it kept. That read came out as one closed skin on the
test character; a character that wears clothes now has not been measured again. Read the note and every ring
line before trusting it, and where the skin is still missing, fit on a whole body (the source body or a
clean copy) and keep the dressed character for the presentation test. Skin a new garment uncovers, such
as legs under a shorter skirt, should come back through the character (assemble it and look): the build writes the wardrobe item and
its body mask; wear the item in the character's `outfit { }` in place of what it wore and assemble
again. Clothes the character already wears are not measured against: crossings are counted on the
garment's own cloth and against the body, and `over=` names only garments of the same file. Measuring
against clothes a character already wears is not built.

**Check every ring before you build cloth between rings.** Most badly folded, pinched or twisted
cloth starts with a bad ring. The dry run prints one line per ring; read each one:

- **Its facing.** A ring round the torso or a leg should read `counts clockwise looking down`. One
  that reads `looking (0.48, 0.04, -0.87)` is tilted about 29°. A ring found through one or two
  points is the shortest flat cut through them, and near the armpits that cut runs over a shoulder.
  Measured 2026-09-28: a one-point and a two-point (front and back) chest ring near the armpits both
  tilted. The same chest ring drawn through four points (front, side, back, side) read level.
- **Its length.** Compare it with the body at that height. A chest ring 20 cm longer than expected
  has wandered over a shoulder or into an armpit.
- **`@0` on both of a garment's rings at the same place round the body.** The grammar is explicit:
  "a twist is declared by where the two first points sit, never corrected". A waist ring starting at
  the front and a chest ring starting at the side makes cloth that spirals a quarter turn.
- **Measured points, not guesses.** Take every point from the render tool's hit query on the actual
  body. A point off the skin makes a ring that is not where you think it is.

**Where a ring lies on the skin.** Every typed point is moved onto its nearest skin, and a point more
than 1 cm from the skin is refused ("has a point off the body"). No typed point floats, so never type a
stand-off: put the point on the skin and stand the ring off with `expand(cm)`, which pushes each point at least
that far off its nearest skin (a stretch that would cross a hollow is drawn straight across it instead), or `expand((up, side))`, two amounts blended round the ring (`up` toward its
`@0` and across from it, `side` square to that). A ring through three or more points is a smooth curve
through them, put onto the skin wherever the skin bulges past the curve; at the default `smooth=100%`
it bridges a hollow straight across, and below 100% it is drawn toward the skin and follows it. A ring
through one or two points is the shortest flat cut of the body through them, its hull at 100%. Turned
to the shortest cut, a two-point ring can slope with a sloping waist. For a level waist today, type four
or more points at one height (front, side, back, side) and read the ring's line: the curve through
them keeps that height except where the skin bulges past it and where each point was moved onto its
nearest skin.

```gcgarment
# Five points at one height round the chest, then stood off the skin by two amounts.
# Source: Plugins/GraphCode/Docs/Examples/ButtonShirt/ButtonShirt.gcgarment, lines 15-17.
ChestOnSkin = ring((-1.592, 12.672, 123.640), (0.000, 12.746, 123.640), (16.475, 1.350, 123.640), (0.000, -10.382, 123.640), (-16.475, 1.350, 123.640)) {
  expand((0.93, 2.54))
}
```

**Then read what the build says about the cloth.** Since 2026-09-28 the build checks the rings of each
garment against each other and measures the cloth it lays, in a dry run too:

- A note when two rings a garment is laid between face more than 20° apart, and when their `@0`s sit
  more than 45° apart round the body. Both are either meant or the fold the build is about to make.
- Each build line ends `folds N (M through), crossings C, inward I`. `folds` counts triangles facing
  against their neighbours; `M through` are the ones that pass through the cloth, which are real folds,
  and the rest are creases. `crossings` counts triangles passing through another part of the garment:
  the cloth through itself, or a thick garment's outside wall through its inside. `inward` counts cloth
  facing the body it lies over. A note under the line says where each lies, largest cluster first.
- The "laid flat in UV0" note says how evenly the cloth is laid in its UVs, as a range of its true size.

What the tilted plate taught, measured 2026-09-28: a cuirass built from the 29° chest ring to a level
waist rendered covered in black-and-white speckle, with its top rising over one shoulder. **The speckle
was the checker texture, not two surfaces fighting**: rendered `lighting_only` the plate was clean, and
`unlit` showed the same speckle. The cloth had folded, and the UV flattening laid the folded piece at
many times its size, so the checker tiled into noise. The flattening now falls back to measuring a
piece whose solve folds or runs uneven. The fold itself is real geometry: the cloth under the high side
laps over the cloth below it. That is what `folds` and `crossings` are for; fix the ring.

Then build a static result and render it lit, from all sides and at a grazing angle, before adding
anything else. `gc_apply save:false` leaves a garment build's assets dirty in the editor rather than
saved. The exception is a line that goes into a MetaHuman's wardrobe: the wardrobe writer saves as it
writes, so that is refused by name before anything is built.

### 4. Build the silhouette before details

Start with the fewest rings that express the garment. Check the neckline, openings, waist, hem and
overall outline on the body. Add middle rings only where the silhouette needs another controlled
station, such as a shirt that must hang from the bust rather than follow the fold below it.

Keep these decisions separate:

- **fit**: how closely the cloth follows the body between rings;
- **clearance**: how far it stands off the body;
- **thickness**: whether the result is a sheet or a visible solid shell;
- **fullness**: how much cloth is fitted around an edge and whether it gathers, pleats or eases;
- **width**: the outline held by the rings themselves.

More cloth does not automatically make a wider skirt. It makes deeper or more numerous folds inside
the silhouette the rings allow. Widen the outline deliberately when the design calls for a flare.

Build and render a simple static result first. This isolates construction and surface problems from
skinning, wardrobe and animation problems.

**Shape a top from a real pattern, not by eye** (learned on the common T-shirt, 1 Nov 2026: a blind
critic's 5/10 became 7.5+ from the rings alone). Measure the body's girths at the chest, waist,
hip line and hem height by slicing its mesh, take ease and lengths from a published pattern's own
defaults (FreeSewing's code is open), and write every
number's source beside it:

- **Neckline at the base of the neck.** Through the shoulder-neck points, where the shoulder slope
  meets the neck column, and dropping to the block's back and front neckline depths below them, round
  as a drafted neckline is (a quarter ellipse each side: give it points between). A ring on the neck
  column reads as a mock neck, and cloth gathers into it. *Corrected 1 Nov (the T-shirt's second
  pass):* a block's front depth assumes a forward-leaning neck; on an upright one (SM_TPoseBody) it
  dropped the front to a 57 cm boat neck. Measure the base of the neck column on the body and put the
  ring there; check its length against the pattern's neck girth (Brian's neck × 1.05 came within 1 %).
- **A chest ring at the pattern's chest ease**, as a middle ring just below the armholes (a middle
  ring may not cross a cut armhole), expanded by the ease over 2π. Without it the cloth bridges from
  neck to hem and hangs as a tent. Ramp the section above it from 0% to 100% (`loose@1=$Profile`)
  and span the section below at 100%: a jump from 0% to 100% at the ring shows as a ledge there.
  The armhole's underarm point stands straight above the chest ring's built side, as a tee's side seam
  runs down from it; a side let out past it lays the body inside the sleeve, and the build says so in a
  note ("laid through each other").
- **The hem at the pattern's length and hip ease**, with points at the front, back and both sides:
  a front-and-back ring takes the shortest path over the hips and comes out slanted.
- **A sleeve at the pattern's biceps ease**; a hem ring several centimetres off all round flares.

**How the cloth is made.** Every garment is laid out in 3D as one paper-thin sheet: `construct()`
shapes it on the body, and the pieces you write are regions of that sheet, sewn where you sew.
Thickness is a number stored on each piece and grown into a solid only at the end. The hang then
rests the cloth at that shape's own lengths, each piece laid flat (a flat piece is kept only where
it agrees with the built shape within about 3%), so it settles into the shape you built; with no
`settle()` written it hangs for 3 seconds; `settle(off)` keeps it as laid and does not hang it (no
seconds of Chaos, and its note says the cloth was not hung). A hang that comes apart - its edges more
than twice their laid length at the 99th of every 100 - is refused, never saved; a knot's settle stops
after two minutes, or when the task is cancelled, and says its 120 s budget of frames ran out. A garment cut smaller than the body (`loose=` below 0%)
is laid at its 0% shape and hung with its cloth shorter across the grain, at the knit's own
stiffness for a stretch fabric. Read the build's notes after every build.

~~*Before 20 Oct 2026 (the drafted route, superseded):* `construct()` works the garment out flat: its
pieces are cut from the body's measures, joined at their seams while still flat, and the 3D is made
once, at the end of the build, when the joined cloth is laid round the body and hung under gravity.
A garment it cannot draft yet is built the earlier way, its cloth laid in 3D first, and the build's
notes name the step that sent it there (`'Skirt' is made the old way ...: construct() does not draft
'match() with pleat()' flat yet`). Read the notes after every build: a drafted garment's say how it
was cut, laid and hung — its stretch, how near the skin it came, any fold finer than the solver can
part — and which of your words shaped only the earlier way's look. A drafted hang takes from about 20
seconds for a skirt to a minute or more for a shirt with sleeves.~~

*Superseded 20 Oct 2026 for gathers: the next paragraph is the drafted route's. A gathered skirt is
now laid in 3D like every garment and is being re-checked there.*
A gathered skirt drafts: `match($Waist, 180%, gather)` cuts each row as full as it says (fading
where `falloff=` says), lays the extra as folds at the hang's own spacing, and holds the waist at
the folds' valleys. Its waistband, `Band = extend($Waist, 3.5)` below the `match`, is cut straight
the waist's length round, sewn to those valleys, and the skirt hangs from it; where the body widens
above the waist the band rests on it (up to 1 cm in, then the build refuses and says to cut it
narrower or start the skirt lower). Very full gathers (about 250 %) can still leave a few faces
over the stretch limit at the waist; the build names them. Grow it in its own fabric,
`extend($Waist, 3.5, fabric=$Fused)`, where it is interlined.

### 5. Compose cloth and hard parts

Add sleeves, legs, bands or other garment pieces only after their shared boundaries are clear. Open
or extend cloth only through operations the installed help marks as implemented. Use guide operations
to say where a free edge goes and pull operations to remove slack; neither creates missing cloth.

Keep hard parts in Shape. Place a buckle, badge or row of spikes from its `.gcshape` entry and verify
its mounting face, orientation, gap and deformation. Armour is the largest use of this combination:
see [Armour is a garment](#armour-is-a-garment). Shape is optional for Garment, so a cloth-only
recipe should still build when Shape is absent, while a recipe that requests a Shape part should
refuse clearly.

Treat layers as geometry, not draw order. Leave explicit room where a shirt overlaps trousers, a
coat sits above a shirt, or a strap passes under a buckle. Hidden-body masks stop covered skin from
drawing; they do not create physical separation between two garments.

### 6. Choose deformation after the rest fit works

Once the static result is right, build a skinned result if it must follow the source skeleton. Check
the transferred weights at shoulders, armpits, elbows, hips, crotch and knees. Rigid parts need their
own intended stiffness rather than blindly bending like the cloth below them.

Build a MetaHuman outfit only when the source is a MetaHuman and refitting is part of the job. Verify
it first on the source body, then on at least one materially different compatible body. Compare the
refit with the source-body control: an error already present on the source is not evidence that the
refit introduced it.

Finally, put the generated wardrobe item on the character through the character's `.gc`. That is the
point to test the whole presentation with hair, face, other clothing and the actual animation set.

### 7. Rebuild from the source and keep the evidence

Change the `.gcgarment`, rebuild, inspect the generated result and save it. A generated asset carries
provenance back to its source and body, but its reflected `.gc` does not replace the garment recipe.
Keep the recipe and every referenced Shape source in version control with the project.

Reload the saved outputs in a fresh editor state before calling the work durable. See
[validation](VALIDATION.md) for the wider persistence, performance and package gates.

## Armour is a garment

Owner's ruling, 2026-09-28: making armour needs Garment. An AI asked for a warrior's armour built
loose Shape meshes and placed them by hand. They did not fit the body, did not follow the skeleton,
and the AI did not know what to plan first.

Armour is built with **Garment and Shape together**:

- **Garment owns everything that wraps the body.** It fits each layer to the body, stacks the layers
  with clearance, skins them to the skeleton, and gives the hard parts a place to sit.
- **Shape owns everything hard.** Plates, cops, buckles, rivets, studs and edge rolls are `.gcshape`
  parts. The garment imports them and seats each one on its cloth. The part's geometry stays in its
  own file; the garment only says where it goes and passes it values.

### Plan the layers from the skin out

```text
body (a MetaHuman or custom body, clean, in its authoring pose)
└─ under-layer ........ shirt, gambeson, trousers: garments with fabric thickness
   └─ mail ............ a garment wearing a mail material
      └─ plate ........ a thick, closely fitted garment shell (cuirass, faulds), or
      │                 Shape plates seated on rings (pauldrons, couters, vambraces, poleyns)
      │    └─ rivets, rolled edges, studs: Shape parts in rows along rings
      └─ straps and belts: garments cut open and extended, through Shape buckles
         └─ tabard, surcoat, cape: garments over the lot
```

For each piece, decide which of three constructions it is:

| Piece | Construction |
|---|---|
| Something that wraps the body and bends with it: gambeson, mail, leather, tabard | A `.gcgarment` between two rings, with `fabric=` or `thickness=`. Skinned: it bends with the skin under it. |
| A shell that follows the body closely: cuirass, back plate, faulds | A garment with `loose=0%` and a plate `thickness=`, its rings expanded to clear everything under it. |
| A rigid piece that must not bend: pauldron, couter, poleyn, gorget plate, helmet parts | A `.gcshape` part placed with `add($Parts.Entry, at=$Ring@u, stiffness=100%)`. On a skinned build it moves as one piece, and the cloth under it moves with it. |
| A row of the same hard part: rivets, studs, scale plates, lames | `add($Parts.Entry, along=$Ring, repeat=N)`. N copies evenly round a ring. |
| A strap through a buckle | A garment cut open (`cut($Front)`), `extend`ed into a tail, `guide`d through the buckle's socket, then `pull`ed snug. |
| A real seam down a garment: a skirt's side seams, a coat's back seam | Cut the cloth open down a line (`Back = cut($Side)`), then sew the two sides (`sew($Side, $Back)`). The garment is whole again, and `press($Side, 1cm)` and `stitch($Side, offset=0.6cm)` treat that line as a seam. |
| A facing cut to follow its topstitching: the jeans' fly facing curving in with the J to the foot | `press($FlyL, -3.195cm, follow=($J1, $J2, $J3, $J4, $J5, $J6, $J7), margin=0.68cm)`, the stitched rows drawn above it as ordinary two-place lines. The rows, the margin, the refusals and which way the stack hangs (a `follow=` press is a facing, hanging inward) are in [the press section of the pieces-and-edges guide](GARMENT-PIECES-AND-EDGES.md#which-way-a-pressed-seam-stands-plain-and-follow). |
| A pressed crease down a trouser leg or a sleeve | Draw the line on the cloth and press it with no width: `CreaseR = line($Top_R@0.75, $Ankle_R@0.75)` then `press($CreaseR)`, above `settle()`, in the garment that hangs. The fabric says how a pressed crease holds: `crease=` on `fabric()`, the FAST press test's angle (the weft strip's for a crease down the warp). The hang rests the line folded 180 less it, a ridge out from the body; the note says how far it folded as hung. A seam is pressed with its width, never as a crease. |
| A lapel, a turned-up cuff, a pocket flap turned back | Grow the strip past its edge with `extend`, then turn it back with `fold`. The build lays the fold as a start pose over the cloth it grew from. A turned-up cuff on a sleeve (cloth grown from a whole ring) is held by its own hoop, and the hang makes its turn; every other fold - a lapel, a collar, a pocket flap - stays that start pose, moved by the hang only within about a centimetre, until what holds it is modelled. Left unpressed, its turn rests flat and a cuff's turn stays near where it was laid, held by its own hoop; a lapel left unpressed stands open, because no canvas or roll-line tape holds it (not modelled yet). Pressed with `press` (no width) below the fold, its turn is steam-set: its hinges rest at 180 less its fabric's measured `crease=`, so state that on the fabric - a pressed fold in a fabric without it is refused by name. A tailored lapel is a soft roll, never creased, so a pressed lapel is a set crease, not that roll. A faced fold is one ply in the hang, and its visible edge and the roundness of its turn are mesh-finishing rules, not the hung cloth. A collar on a stand (`fold(..., stand=)`) is never pressed: its hinges rest at the turn it is laid at, a drawn rest and not a measured set. Ask the installed help for the exact spelling. |
| A collar, cuff, placket or waistband that stands stiffer than the shirt | `Collar = extend($Neck, $Depth, fabric=$Fused)` with `Fused = fabric(...)` stating the interlined laminate as one measured cloth (weight, thickness, bending length, stretch, shear, each marked measured / derived / author's); `fold($Collar, stand=)` reads it and doubles it as a faced collar. Naming the cloth's own fabric changes nothing. |
| A knit neckband or rib band on an edge | `band($Neck, width=2cm, fabric=$Rib, cut=75%)`. A rib neckband is cut 70-80 % of the neckline (three sewing guides; 75 % for a 75-100 % stretch rib) and sewn on stretched. Hung, it lies along the body's own slope at the neckline and hugs it: measured 14 Nov on the Common tee at 75 %, the seam unpuckered (+0.4 %), the band within 1 degree of the skin's slope front and back and 6 at the sides, its free edge 16 % over its rest, nothing in the skin. Cut at 95 % (a knit binding's figure) the 2 cm band stood up 21 degrees steeper than the cloth. A waistband that ends at a fly takes `ply` beside `ends=$Fly, over=$FlyL, lap=3.2cm` (refused without them): its UNDER end runs `lap=` past the opening as its own doubled cloth beneath the over end, which stops flush at the fly's edge, the two plies 2 x the fabric's `thickness=` apart mid to mid (14 Nov, slice B1; without a button it rests by its seam and by contact). `button=1.28cm` with `ply` places the button along the lap from the opening (your number; Charlie's pattern puts it at 0.4 of the lap, refused outside the lap or without `ply`): `settle()` ties the over end to the under end there, mid-width, at their stack - one place held, the rest of the lap by its seam and contact. A lapped fly takes the same word, `sew($Fly, $FlyL, over=$FlyL, lap=1.2cm, ply)` (refused without `over=` and `lap=`): the fronts are not welded down the fly - the over front's fold is a free edge lying on the under front's underlap, the stack apart (half the fold's plies plus half the underlap's one, each the fabric's `thickness=`), the crotch seam sewn below the fly's foot; only the underlap's root holds it (no zip, bar tack or J topstitch yet), so hung it may gape (14 Nov, slice C1). |
| A garment's own hem turned as real cloth (a tee's coverstitched hem) | Per edge, below `construct()`: `rim($Cuff_L, turned(2.54cm), ply)`, the same words as `patch(..., hem=(d, once), ply)`. Added 15 Nov 2026; parsed, and it builds as the relief of two layers until the edge slices land. Needs a fabric with `crease=` and a hem turned once; refusals and the example fixture are in [the edge section of the pieces-and-edges guide](GARMENT-PIECES-AND-EDGES.md#a-hem-turned-as-real-cloth-ply-on-an-edge). The stitching is the file's own `stitch` rows. |
| A piece cut RAW and sewn flat (a jeans label), or a strip folded to plies (a belt loop) | `Loop1 = patch($L1Out, $L1Top, $L1In, $L1Foot, turn=0, ply)`, or `layers=3` for three plies, then `stitch` rows on its sides and `tack` across its ends. Added 15 Nov 2026; the words (`turn=0`, `layers=`, `ease=`, `gap=`), the example and every refusal are in [the belt loop part of the pieces-and-edges guide](GARMENT-PIECES-AND-EDGES.md#a-belt-loop-a-raw-strip-folded-to-plies). |
| A bar tack (the fly's base, a pocket corner, a belt loop's end) | `tack($J5@1, length=0.95cm)`, after the row's `stitch($J5)`: a place written as any place on a line or ring (`$Line@u`, `$Line@32cm`), centred there along the row (on past its end along its last span), and its declared length (bar tacks are 6.4-9.5 mm commonly). Built at `settle()` as needle ties through EVERY layer under each hole, at the row's `every=`, each pair at its derived stack, its note giving holes, layers and ties. It moves no cloth. Refused by name: no `length=`, a length of zero, another setting, and a place that is not `$Line@u`. The row must be stitched (its holes come from the resolved row), and today it is tied only on a garment with declared plies. |
| A panel let into an opening | `cut($Left, $Right)` takes the cloth between two lines out; `Fill = extend($Left, cm)` grows new cloth across the opening from one edge, a little more than the opening is wide; `sew($Fill, $Right)` joins its end to the other. Where the two edges differ in length — a slanting hem, a shaped panel — write `sew($Fill, $Left, match=ease)`: end to end, the longer eased onto the shorter. Grow the new cloth from the longer edge, so the longer of the two sewn edges is the loose one. Sewing the two edges of an opening straight together is refused where a ring holds the cloth: taking a garment in is a smaller ring. |
| Two tails tied: a knotted shirt front, a sash | Two tails `extend`ed from edges, two `guide`s through the same points with `@over` on the one on top at each (a half knot is three, taking turns), then `pull($A, $B)` both; `settle(time=1)` after it runs Chaos at build time to author the hanging pose. Runtime simulation is selected separately by `[as=cloth]`. Ask the installed help for the rest, and inspect whether the knot actually cinches and retains its crossings. |
| A sash or robe tie that reads as a knot | Declare it: `Knot = constraint(at=, facing=)`, then `guide($EndA, $Knot@wrap, place)` and `guide($EndB, $Knot@wrap, place)` — a square knot, laid by folds; `$Knot` bare on one end, a half knot. The ends come to it from either side, a cut wide enough apart for each to go round the other. |

Layers are geometry, not draw order. Each outer layer's rings must stand further off the body than
the inner layer's clearance plus its thickness. Hidden-body masks do not separate two garments.

### A verified example: gambeson, cuirass, pauldron and rivets

This was dry-run on a T-pose test body on 2026-09-28. The points were measured on that body with the
render tool's hit query. **Measure your own body; never reuse these numbers.**

The Shape library holds the hard parts. Each part is modelled at the origin with its **mounting face
at z=0**. The garment seats that face on its cloth, with +Z pointing out of the body, +Y up the body
and X along the ring.

```gcshape
gcshape 1  into:/Game/Warrior/Armour/Parts  smooth:0.02

# Hard armour parts for garments to place. Mounting face at z=0, +Z out of the body.
build { }

S = shape('/GraphCodeShape/Presets/Solids')

PauldronRadius = 9      # the cap's reach round the shoulder
PauldronRise = 5        # how far its crown stands off the shoulder
Plate = 0.25            # steel thickness
RollRadius = 0.4        # the rolled edge round the rim
RivetHead = 0.45

# The rolled edge: a ring of round bar lying on the rim.
Roll = mesh() {
  circle(radius=$RollRadius)
  revolve(offset=$PauldronRadius)
}

# A shoulder plate turned from its profile: rim, falling shoulder, crown. `capped=false` keeps the
# turned curve an open surface, and `shell` gives it thickness; the default `revolve()` would close it
# to the axis and `shell` would hollow a solid. A finished pauldron is several overlapping lames;
# build and review this entry against the reference.
Pauldron = mesh() {
  curve(($PauldronRadius, 0), ($PauldronRadius * 0.8, $PauldronRise * 0.5), ($PauldronRadius * 0.4, $PauldronRise * 0.9), (0, $PauldronRise))
  revolve(capped=false)
  shell($Plate)
  add($Roll, at=(0, 0, $Plate / 2))
}

# A rivet as it shows on the outside: a domed head. The shank is inside the plate and never seen.
Rivet = mesh() {
  add($S.Dome, r=$RivetHead, h=$RivetHead)
}
```

The garment fits the layers and places the parts:

```gcgarment
gcgarment 1  mesh:/Game/Warrior/SM_Body  into:/Game/Warrior/Armour

build {
  SM_Gambeson = $Gambeson
  SM_Cuirass = $Cuirass
}

Quilted = fabric(thickness=1.2)

Neck = ring((0.095, 6.051, 149.589), (5.779, 2.123, 150.916), (-0.095, -6.038, 150.916), (-5.589, 2.494, 150.916)) {
  expand(1)
}
Hips = ring((0, 9.8, 86.4), (0, -11.4, 87.4)) {
  expand(2)
}
Armhole_L = ring((24.158, 4.407, 141.063)) {
  expand(2)
}
Armhole_R = ring((-24.158, 4.407, 141.063)) {
  expand(2)
}
Wrist_L = ring((68, 3.0, 139.5)) {
  expand(1.5)
}
Wrist_R = ring((-68, 3.0, 139.5)) {
  expand(1.5)
}
# The plate stands clear of the quilting: its rings sit further off the body than the gambeson's
# clearance plus its 1.2 cm wall. Four points keep the chest ring level near the armpits.
Chest = ring((0.106, 12.638, 133.627), (14.968, 6.784, 134.474), (0, -10.554, 134.347), (-14.742, 7.020, 134.440)) {
  expand(3.5)
}
Waist = ring((0, 9.8, 105), (0, -9.8, 105)) {
  expand(3.5)
}
RivetLine = ring($Chest) {        # a copy of the chest ring, 1.5 cm down the body
  shift((1.5, 0, 0))
}
Shoulder_L = ring((32.341, 4.145, 140.638))

Armour = shape('/Game/Warrior/Armour/Parts/ArmourParts')

Sleeve_L = garment($Armhole_L, $Wrist_L, loose=80%, fabric=$Quilted)
Sleeve_R = garment($Armhole_R, $Wrist_R, loose=80%, fabric=$Quilted)

Gambeson = garment($Neck, $Hips, loose=60%, fabric=$Quilted) {
  cut($Armhole_L)
  add($Sleeve_L)
  cut($Armhole_R)
  add($Sleeve_R)
  add($Armour.Pauldron, at=$Shoulder_L@0, stiffness=100%)
  construct()
  relax(1)
}

Cuirass = garment($Chest, $Waist, loose=0%, thickness=0.25) {
  add($Armour.Rivet, along=$RivetLine, repeat=24)
}
```

What the dry run reported, and what it teaches:

- Every ring on the torso read `looking down`, the chest ring 125.6 cm round. The first try, with
  one or two chest points, tilted 29°: see the ring checks in step 3.
- `would place $Armour.Pauldron at $Shoulder_L@0 ... on SM_Gambeson` and
  `would place $Armour.Rivet x24 along $RivetLine ... on SM_Cuirass`: the parts found their cloth.
- Parts are never sunk into cloth. A negative `gap=` on a part was overridden: each rivet was
  "lifted ... so no part of it lies in the cloth". A part sits on the outermost cloth under it.
  *(That dry run predates 2026-09-28's hold: the cloth under a part is now pressed onto it instead —
  [below](#the-cloth-under-a-plate-goes-with-it). A negative `gap=` still counts as 0.)*
- The copied `RivetLine` "floats off the body": that is expected for a copied ring, and the rivets
  still seat on the cuirass under it.
- The pauldron was first a solid turned cap. Rebuilt later that day as the plate above, its own build
  line names no problem: one part, 18.8 x 18.8 x 5.5 cm, `volume 107` cm3 facing outward. The
  garment dry run was not repeated against it.

This is a working method, not finished armour. The pauldron here is one plate with a rolled edge,
standing in for a set of overlapping lames. The whole set still has to be built to the reference, posed, rendered and judged, as
[ASSET-QUALITY.md](ASSET-QUALITY.md) requires.

### Parts across the whole garment: Shape's array words

A part's `add()` takes every word Shape's `add()` takes to make copies, meaning what Shape's grammar
says it means, with the garment's cloth as the surface and its rings as paths. Only one of `at=`,
`along=`, `scatter=` and `onto=` says where. Every copy is seated on the outermost cloth where it lands
and lies flush on it, unless `orient=keep`.

```gcgarment
Brigandine = garment($Chest, $Waist, loose=0%, thickness=0.6) {
  add($Hardware.Rivet, repeat=(7, 5), spread=(70%, 80%), onto=-Y)   # rows cast at the front
  add($Hardware.Rivet, repeat=(7, 5), spread=(70%, 80%), onto=+Y)   # and at the back
}

Studded = garment($Chest, $Waist, loose=0%, thickness=0.4) {
  add($Hardware.Stud, scatter=60, apart=3.0, seed=4)                 # all over, none closer than 3 cm
  add($Buttons.Badge, at=$Placket@0, repeat=5, step=(0, 0, -4.5))    # five buttons down the front
}
```

- `scatter=` draws over the whole garment's outside, by area. `onto=` casts a grid at it along a
  direction — `onto=-Y` travels toward -Y, so it lands on the front — and needs no `[ ]`: the garment
  chooses its outside.
- `repeat=` with `step=` or `spread=` after an `at=` grows a lattice from that one place, in the body's
  frame, each copy seated where it lands. A copy that lands off the cloth is left out and the build says so.
- `along=$Ring, repeat=N, spread=30%` fills 30% of the ring, centred on its `@0`.
- `jitter=(move=, turn=, scale=)` and `seed=` work on all of them. A misused word is refused in
  Shape's own words, naming the part.
- Copies take no name, and one `add()` places at most 1000.

Checked on a brigandine fixture:
70 rivets in five rows and seven columns, front and back, and 60 studs over the front, back and
sides, all resting on the cloth.

### Plates that take the shape of the cloth under them

A rigid plate seated at one point stands off a torso that curves away from it. A part that should
lie on the cloth says so in its own file: it declares a `Host` parameter and conforms to it. The
garment hands it the outside of the cloth under each copy, in the part's own frame, and builds it
once for each copy. By itself the part lies on its default, here a flat sheet at z=0, so it still
builds and previews as a flat plate.

```gcshape
Flat = mesh() {
  rect(w=200.0, h=200.0)
  fill()
}
Splint = mesh(Host = $Flat) {
  rect(w=4.5, h=18.0)
  extrude(height=0.3)
  conform(to=$Host, project=-Z) [bottom, carry=!bottom, smooth=3.0]
}
```

```gcgarment
Belly = ring((0, 11.2, 119.5), (0, -10.2, 119.5)) {   # level: a front and a back point
  expand(1.5)
}
Splinted = garment($Chest, $Waist, loose=0%, thickness=0.8) {
  add($Hardware.Splint, along=$Belly, repeat=16)
}
```

- `project=-Z` lands the back straight down onto the cloth. `carry=!bottom` moves the rest of the
  plate with its back, so it keeps its thickness. `[smooth=]` sets how finely the back is cut first, so
  it can follow the curve.
- The garment never writes `Host=`; one written there is refused. `gap=` still lifts the fitted plate.
- The build says how the copies fitted, once for each thing said, including any `[smooth=]` owed.
- Hang a row that should stand flat from a ring that reads `flat` in the build's ring report, such as
  one drawn through a front and a back point (flat is not level: that ring is the shortest cut through
  its two points, so it slopes with a sloping body). A chest ring drawn through four points follows the body
  between them and dips under the arms, and a copy of it keeps the dip: splints hung from one
  measured 4.6 cm apart in height.

Checked on a splinted-garment fixture: sixteen splints, each back lying on the cloth (no
point further off than the plate's 0.3 cm), none sunk, each bent 0.4 to 1.1 cm to follow the torso,
their middles level within 0.2 cm.

### The cloth under a plate goes with it

Owner's ruling, 2026-09-28: *automatic*. Every part holds the cloth under it, as stiffly as its
`stiffness=` says. Fabric riveted to a plate goes where the plate goes, and the fabric between plates
stays free:

- **At rest, the cloth under the part takes its shape.** Where the part's back lies over the cloth, the
  cloth is pressed onto it: in where the part dips into the cloth, out where it stands off. A flat plate
  on a round belly has the padding under it pressed flat to meet it, where before the plate touched at
  one point and stood off at its sides. The part itself does not move. A fitted splint already matches
  its cloth, so nothing moves.
- **In motion, that cloth moves with the part.** On a skinned build the cloth under a part takes the
  part's own weights, so when the body bends under a plate the two go together instead of parting, or
  the padding coming up through the plate.
- **Both ease back across a border**, measured along the cloth. In motion it is `hold=` wide, 3 cm
  unless written, so a rivet grips only the cloth round it and the fabric between plates stays free.
  At rest, a part with no `hold=` written widens its own border until the cloth bends no tighter than
  its fabric does easing back from how far the part pressed it: `√(6 · bend · lift)`, never under
  3 cm. `bend` is the fabric's `bend=` (the geometric mean of along and across), or 3 cm where the
  fabric states none. A flat plate standing 1.76 cm off a round belly eases back over 5.6 cm; a
  rivet, which presses almost nothing, over 3 cm. A stiffer fabric bridges further.
- **A written `hold=` is both borders**, at rest and in motion, never widened. `hold=0` holds
  nothing, and the part is lifted clear of the cloth as it was before.

```gcgarment
Armour = shape('/Game/Warrior/Armour/Parts/ArmourParts')

Vest = garment($Chest, $Waist, loose=0%, thickness=1.0) {
  add($Armour.Plate, at=$Belly@0)                # holds the quilting under it; the rest border is derived
  add($Armour.Plate, at=$Back@0, hold=6cm)       # 6 cm at rest and in motion
  add($Hardware.Badge, at=$Arm_L@0, hold=0)      # holds nothing: lifted clear, as before
}
```

- The cloth is never pressed into the skin. What the skin stops lifts the part instead, and the build
  says by how much.
- A part at `stiffness=` below 100% presses the cloth part of the way and is lifted the rest; at 0% it
  holds nothing. A part a `guide()` or `pull()` laid cloth against, or a part that bends, presses no
  cloth but still holds it in motion.
- The build line says, for each add, how many points of cloth it holds, how far it pressed them in and
  out, and how many it eases back across its border — and, where the rest border was widened, how
  wide, from what lift and what bending length.

Checked on a plackart fixture (at rest) and on a plate on a MetaHuman's upper arm, the arm turned at the shoulder.

### What Garment and Shape cannot do for armour yet

- **A plate is fitted to the cloth under it, not to a region of the body** (`Host=$Shoulder_L` is still a proposal), and only a copy resting on cloth is fitted: a
  part on an accessory or at another part's point keeps its own shape.
- **Skinning alone has no secondary cloth motion.** Tabards, capes and mail skirts built
  `[as=skeletal]` or `[as=outfit]` follow bones. Use `[as=cloth]` with `hang()` for simulated cloth,
  and validate each combination of cloth and attached armour in motion.
- **Thin curved plates in Shape have one working route**, the pauldron's above: an open profile,
  `revolve(capped=false)`, then `shell`. The others have measured traps: see
  [ASSET-QUALITY.md](ASSET-QUALITY.md#choose-the-operation-for-the-form). Read every part's build
  line; a clean one names no problem, and `volume` must be positive.
- **A single part is placed on a ring, or on the open edge of a cut or extension.** Copies can go
  anywhere over the cloth (above), but naming one place on it (`$Garment@(u, v)`) and choosing part of
  the cloth for copies (a `[ ]` on a garment's `add()`) are not built.

### Test armour in motion

Build `[as=skeletal]` once the static fit is right. Pose the character with the arms raised, forward
and crossed, the torso bent and twisted, and a walk, run and attack. Rigid plates must not cut into
each other or the layers under them at the shoulders, elbows, hips and knees. That is where plates
are split into lames, and where you leave the clearance. Record which extreme poses you accept.

## A skirt: wrap, band, ties, hem

What a wrap skirt with a tied band needs, as the current build has it, and where it stops.

### A wrap skirt, today

There is no wrap word yet. Two routes build one, both test fixtures on the T-pose body (29 Oct):

- **The skirt cut open down a slanting line**, both edges finished. A cut down a drawn line through a
  middle ring is built: the garment is laid as one piece for it. The two edges meet; nothing laps.

```gcgarment
# From the test fixture WrapSkirt, written for a T-pose test body; the numbers are that body's, take yours from gc_render.
Skirt = garment($Waist, $Hips, $KneeHem, loose=100%, fabric=$Cotton) {
  construct()
  Wrap = line($Waist@0.06, $KneeHem@0.16)   # the wrap's front edge, slanting from the waist to the hem
  cut($Wrap)
  rim($Wrap, turned=1cm)
}
```

- **A lap grown from a line drawn on the cloth**, riding the skirt as a patch pocket does, its far edge
  free. The shipped [LongCoat](../Examples/LongCoat/LongCoat.gcgarment) grows its double-breasted lap
  the same way, its width a profile: `Lap = extend($LapRoot, $LapWidth)`.

```gcgarment
# From the test fixture OverlapWrapSkirt (WrapSkirt's rings and garment line), written for a T-pose test body; take your numbers from gc_render.
  Lap = line($Waist@0.06, $KneeHem@0.16)   # where the wrap's panel is sewn on, slanting from the waist to the hem
  Overlap = extend($Lap, 14)                # the panel, lapped over the skirt and riding it, its far edge free
  rim($Overlap, turned=1cm)
```

**Which way the lap grows.** Off a line drawn down the cloth it grows round the garment the way its
rings count; off a line drawn round it, up toward the first ring. No word chooses the other way. The way
the rings count comes from the header's `reference:` (a point; unwritten, `(0, 0, 0)`): the skin is
walked from it, and each ring is turned by the side it reaches. Moving `reference:` to a ring's other
side turns that ring's count, and with it every `$Ring@u` place on it, every strip grown round, and
which side of a cut is `$Line` (the side whose cloth runs the way the rings count). Set it before you
write places, and read each ring's line in the dry run.

**"Laid as one piece."** A lap off a drawn line needs the garment laid as one piece. The refusal's
"(cut at rings on the body, as a top at its armholes)" is an example, not the condition: a garment cut
at rings is laid as one piece already, and one cut at no ring and not joined is laid again as one piece
by itself, as both skirts above are.

**Limits of this version, plainly:**

- The lap is laid 0.10 cm over the skirt at its foot and sides (the hang's contact either side and a
  hair), bowing off between them to about 0.6 cm at its free edge, and drawn two cloths thick. This
  lap takes no `gap=`; only the cloth grown `over` the cut does ([below](#a-wrap-skirt-grown-over-the-cut)). The build's note gives the number.
- In the build's own hang the lap rides the skirt: none of its points is simulated.
- In an `[as=cloth]` asset the lap is a piece of its own and nothing ties it to the skirt. On the held side
  of the ring the cloth is held from (`hang(from=)`) it is held as the skirt is; past it, it is free. The asset meets no two layers that start closer
  than its contact (see [what the asset is written with](#fit-skinning-and-collision-are-different-claims)),
  so the lap and the skirt may not collide with each other, and a lap built this way can flicker when
  the character walks.
- Cloth grown from a cut's side grows straight on, without looking at the body: an over-front grown so
  stands off flat and runs below the hem (OverlapWrapSkirt's own history). Two free fronts, one lying
  over the other and parting below the waist, are built the other way: with `extend(..., over)`.

### A wrap skirt grown over the cut

*Added 19 Nov.* Cut the skirt open down the front and grow the over-front from one side of the cut with the
bare word `over`: `Over = extend($Wrap, $Overlap, over)`. It lies over the cloth past the cut, welded at its
root, free below the waist. A waistband that follows it is `band($Waist, width=3.2cm, fabric=$Cotton, cut=100%,
ends=$Wrap)`, with no `over=` or `lap=`. Spellings, the one example and every refusal are in
[GARMENT-PIECES-AND-EDGES.md](GARMENT-PIECES-AND-EDGES.md#cloth-laid-over-a-cut-extend-over-and-a-band-that-follows-it).

### A band shows; a turned edge does not

A `band()` is cloth of its own, in its own fabric and thickness, on the garment's edge: it is cloth laid
over the edge, not the garment's own cloth turned in. When its fabric states a stretch across (`stretch=(along, across)`), the band is a
piece grown past the edge, cut shorter by that stretch and sewn on (the rib band in the table
[above](#plan-the-layers-from-the-skin-out)); with no stretch across, or with a pair of widths, it is drawn
onto the finished cloth over the edge. A turned or faced edge, `rim($Waist, turned(3cm))` or
`rim($Waist, turned=3cm)`, is the garment's own cloth turned to the inside: its plies are drawn on the
inside face and the outside does not move, so from outside it reads as the plain top of the garment.
For a waistband that shows, write `band()`:

```gcgarment
# A woven waistband. Source: Plugins/GraphCode/Docs/Examples/CargoTrousers/CargoTrousers.gcgarment, line 221.
  band($Waist, width=4.13cm, fabric=$Twill, cut=100%)
```

### A waistband that runs on into ties, today

There is no word yet for a band whose ends are ties. Build the band as a garment of its own, a sash:
two rings at its top and foot, cut open at the front, each side of the cut `extend`ed into a tie, the
two tied in a declared knot.

```gcgarment
# From the test fixture RobeTie, written for a T-pose test body; the numbers are that body's, take yours from gc_render.
Sash = garment($WaistLow, $WaistHigh, loose=0%, thickness=0.3, fabric=$Fleece) {   # a terry sash, 3 mm
  construct()
  Front = line($WaistLow@0+5cm, $WaistHigh@0+5cm)
  Back = line($WaistLow@0-5cm, $WaistHigh@0-5cm)
  cut($Front, $Back)                     # open 10 cm at the front, where the ends come round to the knot
  EndA = extend($Front, 45)
  EndB = extend($Back, 45)

  Knot = constraint(at=(0, 12, 103), facing=(0, 1, 0))   # where it is tied, and which way is out of the body
  guide($EndA, $Knot@wrap, (-10, 13, 62))   # A goes once round B there, then hangs toward its place
  guide($EndB, $Knot@wrap, (10, 13, 62))    # and B once round A: a square knot
  pull($EndA, $EndB)
}
```

A sash that should lie over the skirt can say so on its garment line with `over=`, which names
garments of the same file (anything else is refused), as a coat names the shirt under it (a sash and a
skirt in one file have not been built together yet):
`CoatBody = garment($CoatNeck, $CoatHips, loose=$Hang, fabric=$Wool, over=$Shirt) {`
(from a test fixture of a coat over a shirt). The sash is not sewn to the skirt.

A waistband that overlaps and buttons in front is already a band:
`band($Waist, width=4cm, fabric=$Denim, ends=$Fly, over=$FlyL, lap=3.2cm)`, with `ply` and `button=` as in the row
"A knit neckband or rib band on an edge" [above](#plan-the-layers-from-the-skin-out); only ends running on as ties
have no word.

**Which way the ties come into the knot** (the rule a declared knot refuses by):

- Each end grows from an edge that runs within about 45 degrees of the knot's up, as the sides of a cut
  down the band do, so it comes into the knot from the side. An end grown from an edge lying across the
  up (the waist's own edge, a band's top) would come in from above or below, and is refused:
  "grows from an edge lying across '$Knot''s up".
- The two ends grow on opposite sides of the knot: "grow on the same side of '$Knot'; each end comes to
  a knot from its own side" is the refusal.
- The knot's up is `up=` on the constraint, else the world's up made square to `facing=`.
- Ends that hang down into a knot are tied the other way: two guides through the same points (the
  "Two tails tied" row [above](#plan-the-layers-from-the-skin-out)).

### Knots, today

Built: the declared knot, `@wrap` on both ends a square knot and on one a half knot, laid by folds; and
the guided knot, two guides through the same points with `@over`, then `pull`. There is no bow: no word
for one, and none is promised yet. A `settle()` after a knot hangs only the knot's two ends, never the
garment, for its `time=` (at most 10 seconds of hang). That settle has a limit of its own (*added
13 Nov*): two minutes of the editor's clock, checked at every frame together with the task's cancel.
Past it the build is refused: "the knot did not settle in Chaos: its budget of 120 s of frames ran out".
A knot that keeps running out can be kept as laid by its folds with `settle(off)` (*added 13 Nov*),
which hangs nothing.

### A narrow hem, and a top that narrows

A shirt's or blouse's narrow hem, a quarter inch turned twice, three plies: `rim($Hem, turned(0.635cm,
twice))` (*added 16 Nov*; [edge kinds](GARMENT-PIECES-AND-EDGES.md#declare-the-kind-of-the-edge)). In
the 10 Nov build write `rim($Hem, turned=0.635cm, layers=3)`; for every edge of the garment, as the
shipped LinenTunic and ButtonShirt examples do, `construct(rim=(turned=0.635cm, layers=3))`.

On rings the cloth is already taken in between them: a waist ring smaller than the hip ring narrows the
skirt. To shape it as a tailor does, cut darts and sew them shut; a dart is `cut` and `sew`, no new word:

```gcgarment
# One of four darts, its press toward the centre. From the test fixture DartedSkirt, written for a T-pose test body; take your numbers from gc_render.
  FrontRight = line($Waist@0.12, $Hem@0.12)          # a line of cloth down through each dart, waist to hem
  DartFR = line($FrontRight@0.17, $Waist@0.12)       # each dart from its tip up to the waist: 9 cm in front, 13 at the back
  DartFROther = cut($DartFR)
  sew($DartFR, $DartFROther)
  press($DartFR, -0.8cm)                             # pressed toward the centre
```

## Fit, skinning and collision are different claims

Garment construction keeps its generated cloth outside the source body in the authored result and
can account for the garment's other cloth and placed hard parts during supported guide and pull
operations. That is an authoring-time geometric guarantee. It is not runtime cloth collision.

Skinning transfers bone influence from the body. It makes clothing follow animation predictably,
but it does not make a hem swing, stop a skirt at the legs, or prevent a sleeve from crossing an
elbow in an extreme pose. Hidden-body maps can conceal covered skin, but they must not be used to
hide a bad silhouette, missing opening or exposed area that should remain visible.

Use this decision tree:

```text
Does the clothing need to move?
├─ no
│  └─ static result, or a rigid Shape part
└─ yes
   ├─ following bones is enough
   │  └─ skinned mesh or render-only MetaHuman outfit
   └─ it must drape, sway or collide as fabric
      ├─ author a hanging pose during the build: settle(time=...)
      └─ simulate during animation: hang(from=$Ring) and build [as=cloth]
```

Garment has both build-time Chaos settling and runtime cloth authoring. `settle(time=...)` changes
the authored pose: it settles existing tied tails, or the whole garment when there is no knot.
`hang(from=$Ring)` declares the held side for `[as=cloth]`; put it below `construct()`. This runtime
build requires the optional ChaosClothAsset plugin and a Physics Asset on the body, and creates a
simulated outfit when the body is a MetaHuman. `[as=outfit]` remains render-only.

**Where to hold the cloth.** `hang(from=$Ring)` holds every point of the cloth on the garment's first
ring's side of that ring's flat plane, or within how far that ring bows off its flat plane, to its place on the body, and
lets the rest hang; the build's own `settle()` holds the same points (with no `hang()` written, it holds
the first ring). The held cloth goes wherever the body goes; the free cloth falls from the held edge. So
the held edge is where the hang begins, and it belongs where the body itself carries the cloth: on a
skirt, the widest round of the seat and hips, as the shipped SkirtFabrics example holds from `$Hips`.
Held from a ring higher up, over the belly or the small of the back, the free cloth starts on a body that
still slopes outward under it. Held lower, more of the cloth moves with the body instead of hanging. The
ring you hold from may be any ring of the file, not only one the cloth passes through: hold from a copy
of the widest ring (`Hold = ring($Hips)`) rather than adding a middle ring for it.

**How cloth hangs and moves is its fabric's numbers, in a fabric test's own units.** Write a fabric
as its test report prints it — SiroFAST's units where FAST measures it — and copy measured values
from the research notes the fabric help names rather than guessing: `weight=` (g/m²), `stretch=`
(E100, the % a strip lengthens under 100 gf/cm, along and across), `shear=` (shear rigidity G, N/m),
`stiffness=` (the overhang-test bending length, cm, along and across) and `friction=` (kinetic). A
garment that hangs with `settle()` is refused while its fabric leaves one of these out. The hang
turns each into Chaos's own units — springs along, across and on the bias, bending along and across
the grain — and the build's note on the line says every value and where it came from. Limits said in
the note rather than hidden: on the 2.5 cm simulation the cloth bends about a fifth shorter than the
fabric (the mesh cannot hold a tighter bend); an edge off the grain carries part of the grain's
spring, so the cloth shears somewhat stiffer than its G; and a MetaHuman `[as=cloth]` takes stretch and bending at
their overall value every way and no shear, because the engine lays that mesh out in its own frame,
not the fabric's grain; on any other body the asset does not take them at all (see "What an `[as=cloth]`
asset is written with"). The same skirt in silk and in denim ships in
[Examples/SkirtFabrics](../Examples/README.md#skirtfabrics-the-same-simulated-skirt-in-silk-and-in-denim).

**What an `[as=cloth]` asset is written with.** These are the build's values; several have a `hang()` word
that replaces them ([solver words](GARMENT-HANG-AND-CURTAINS.md#hangs-solver-words-the-game-cloth-said-in-the-recipe)).
Self-collision is on, each point's radius the larger of half the cloth's thickness and a fifth
of the simulation's spacing, 2.5 cm unless `hang(spacing=)` says otherwise (so 0.5 cm for thin cloth), its friction the fabric's. Body collision
is the larger of half the cloth's thickness and the hold ring's `expand()` (the smaller, where it gives two). The solver runs 8 iterations
a 60 Hz step (at most 16) and 2 substeps. Chaos never meets a point and a triangle that lie closer than
their two radii summed (1.0 cm at 0.5 cm each) in the pose the body is in, so two layers laid closer than that,
a lap on a skirt or a turned-back fold, can pass through each other in the asset. On a body that is not a
MetaHuman the fabric's bending and stretch do not reach the asset today: it keeps the engine's own,
though the build's notes list the fabric's; the MetaHuman outfit route passes them. And a cloth asset
built before keeps the materials on its slots: a slot holding anything but the engine's default material
is kept on every rebuild, the plain fabric an earlier build wrote included, so a stitched or knit look
built later may not show until you set the slots by hand. To try a setting without rebuilding, use
the test Blueprint [below](#check-simulated-cloth-in-the-fastest-motion-it-will-wear).

These routes still need verified material lengths, seams, fabric settings, held regions, body and
self-contact, and runtime evidence. A successful build or settle is not a promise that every knot,
cape, layer or animation is valid. Inspect the reported failures and the rendered motion.

### Check simulated cloth in the fastest motion it will wear

Live or baked is decided per clip on the `.gcmove` build line (`[bake]`); a baked clip is played by a
Blueprint you write from [ANIMATION-HAIR-AND-CLOTH.md "Play a baked clip"](ANIMATION-HAIR-AND-CLOTH.md#play-a-baked-clip). Either way,
check the cloth in the motion that pushes it hardest — a jump, a landing, a spin — not only at rest.

- **Render the motion as a sequence from a fixed camera** (`simulate:true`, `at`/`to`/`count`, the
  camera anchored on the body so it does not follow the jump). A skirt in a standing jump, seen correct
  (2026-10-06): it hangs long and narrow under the push, springs out at take-off, keeps its shape in
  the air, opens into a bell as the body falls, drops past its rest length on landing and swings back;
  in a crouch it lies on the thighs with the knees coming out under the hem.
- **Judge whether a limb went through the cloth from a section, not from a low view.** Cloth lying on
  raised thighs, seen from low in front, shows its inside and the thighs below it, and reads exactly
  like thighs that came through. Cut through the limb and look from the side with an orthographic
  camera: `cut:"x=<the thigh's x>"`, `camera:{anchor:{component:"Body", bone:"pelvis", radius:60},
  azimuth:<a side view>, projection:"orthographic"}`. Everything between the camera and the plane is
  removed, so the camera belongs on the side you want cut away; a picture that comes back uncut or
  empty is the other side, so turn the azimuth by 180. The cloth on top of the thigh is right; the cloth
  behind it is a pass-through.
- **Chaos's inertia default is fine for this.** The cloth feels 75% of the body's own motion by default
  (`LinearVelocityScale`, `AngularVelocityScale` 0.75). Measured on a skirt in a 36 cm jump, 100% changed
  its length by about 1 cm at most. Leave it unless your own measurement says otherwise.
- **Try a cloth setting without rebuilding** by setting it at BeginPlay in a small test Blueprint:
  `ChaosClothComponent:GetClothOutfitInteractor` (a call with an execution pin), then
  `ChaosClothAssetInteractor:SetFloatPropertyValue` / `SetVectorPropertyValue` / `SetIntPropertyValue`
  with the property's name (`LinearVelocityScale`, `UseCCD`, ...). It changes only a property the cloth
  asset already carries. Render it beside the unchanged one at the same moments.
- **Standing up from a deep crouch, a skirt's front lies between the thighs for a moment** and reads like
  culottes, then falls free as the legs straighten: seen 2026-10-06, caught at 1.8 s and an ordinary
  A-line again by 2.2 s, 0.35 s after the body stood upright. That is fabric draped across the thighs
  falling back, as a real skirt does. Judge it over the half second after the character stands; cloth
  still caught after that is a fault.

For an `[as=cloth]` result, disabling simulation does not turn a cloth
asset back into a plain skinned mesh. It can remove the CPU simulation tick, while cloth render data,
shader work and memory remain. A genuinely free non-cloth tier needs an exported result with no cloth
data, followed by visual and motion validation. Measure the real target scenario before assigning a
performance number.

## Materials, UVs and visible stitching

Current generated cloth has a useful UV0 layout, including authored fullness, and Shape parts keep
their own UVs. Inspect scale, orientation, seams and every material slot on the generated asset. A
checker material is a useful diagnostic, but it is not production fabric.

Fabric/material and decorative-stitch authoring are fast-moving technical surfaces. Ask the
installed Garment description before every task. If it describes either one, follow that help and
verify the built result; if it does not, do not invent words from a design note or an older example.
The stable lesson is that UVs and generated geometry are only the carrier—the visible fabric,
surface response and seam treatment still need their own render proof.

A `fabric()` line can say how its cloth looks, in the words its spec uses: `knit=jersey` or
`knit=rib1x1` (a woven fabric's experimental words are in [the pieces-and-edges guide](GARMENT-PIECES-AND-EDGES.md#experimental-unrendered-woven-fabric-looks)), always with `gauge=(wales, courses)` a centimetre (a woven's ends
and picks), and optionally `colour=#RRGGBB` (a swatch's sRGB):
`Jersey = fabric(weight=144.9, thickness=0.68mm, ..., knit=jersey, gauge=(15, 19.4))`. The build
draws that construction into a tiling normal map at those counts (a 2 cm tile, `<asset>_Look_<Fabric>_N`;
a jersey's inside gets its purl back, `_Back_N`) and sets it, with the colour, on the instance each
slot of that fabric wears. A knit with no gauge, or a gauge with no knit, is refused: the loops would
have no size. Write the gauge from a source, never by eye. A fabric that says none of these words
looks exactly as before: the plain grey fabric. Heathered yarn, prints and an albedo are not drawn yet.

Treat appearance as a separate deliverable:

- preserve fabric sources and licences;
- verify texture density and weave scale on the body, not only in a thumbnail;
- inspect normal and roughness response under directional light;
- check both faces, rims and openings on a thick garment;
- prove any manual material assignment survives a recipe rebuild before calling it durable.

The [content guide](CONTENT.md) covers import, provenance and material-source checks.

## Verify the garment as a garment

A successful apply proves only that an asset was built. Use several passes.

### Source and ownership

- The `.gcgarment` and every referenced `.gcshape` are saved and tracked.
- The body path and reference pose are recorded.
- The generated result reports the expected source and body provenance.
- Rebuilding from the same sources is deterministic, and a source change marks the result stale.

### Rest fit

- Inspect front, back, both sides, above and below, plus close-ups at openings and overlaps.
- Check silhouette, clearance, thickness, normals, UVs, material slots and every opening.
- Look specifically at neck, armpits, crotch, waist overlap, cuffs and hem.
- Render the body and garment together, and isolate each when the source of an intersection is
  unclear.

### Motion

- Test neutral idle, walk, run and the project's representative range of motion.
- Check ordinary poses first, then the known extremes. Record any accepted extreme-pose allowance;
  do not silently generalize it to normal movement.
- Exercise body and facial animation together when the garment reaches the neck, head or accessories.
- Check every supported detail level, because simplified geometry and reduced bones can introduce a
  failure that the closest view does not have.

### Character presentation

- Verify the wardrobe item on the source MetaHuman and, for an outfit, on another compatible body.
- Inspect covered-body masks in both preview and the assembled result. Confirm that skin intended to
  stay visible was not removed.
- Test hair, brows, other garments and accessories at the same time. A clean shirt alone does not
  prove a coat, belt or hairstyle leaves it unobstructed.

### Render evidence

Use lit and diagnostic views for different questions. An unlit black pixel can be a real dark
surface, not a hole; compare the same camera in a lit view before reporting missing geometry. A
preview render also does not run every game-time behavior, so use runtime evidence for animation,
dynamic materials or collision claims.

### Persistence, performance and release

- Reopen the saved assets and repeat the relevant fit and motion check.
- Measure the target character count, detail level, platform and camera distance.
- Test the packaged candidate if the claim is about the shipped game.
- Keep failures and not-run checks explicit. Use the full [validation guide](VALIDATION.md).

## Know the present boundary

The current public Garment surface includes body-relative construction, skinned clothing,
build-time settling and runtime Chaos cloth. It is not yet a complete digital tailoring suite.

```text
currently supported through installed help
├─ body-relative rings, profiles and multi-ring garment surfaces
├─ fit, clearance, thickness, openings, added pieces and joined trouser forms
├─ cloth extension, sewing, relaxing, guiding and tightening
├─ gathered, pleated and eased fullness on supported simple garments
├─ optional modelled Shape parts
├─ UV0 on generated cloth
├─ build-time Chaos settle() and runtime [as=cloth] with hang() (optional plugin)
└─ static, skinned, render-only outfits and simulated cloth/outfit results

added after the 10 Nov build (see GARMENT-PIECES-AND-EDGES.md: not yet rendered)
└─ sewn-on pieces (patch), declared edge kinds, press direction

not a current promise
├─ arbitrary knots, frictional locking or universally collision-free cloth
├─ any fabric or stitch form not described by the installed Garment help
├─ every crop, slanted cut, ruffle, stretch or pleat variant
├─ every guide, pull or fullness operation on already-combined/open cloth
└─ automatic correctness through every animation, body, layer and detail level
```

Treat a named refusal as a boundary, not an invitation to approximate it with a nearby operation.
Check current help before repeating this list: the installed product may have advanced since the
guide was written. If the capability is absent, leave the requirement explicit and report the gap.

## Leave a cold-start handoff

For each finished or paused garment, record:

- `.gcgarment` path and generated asset paths;
- source body, skeleton or MetaHuman, plus the authoring pose;
- static, skinned, render-only outfit or simulated cloth result and why it was chosen;
- intended layers and overlap order;
- fit, clearance, thickness and fullness intent in plain language;
- referenced Shape parts and their sources;
- material sources and unresolved appearance work;
- hidden-body-mask expectations;
- simulation decision: none, build-time settle, runtime `[as=cloth]`, external work, or future requirement;
- tested animations, bodies, detail levels and camera views;
- saved render/runtime evidence and exact known failures.

That record lets the next AI continue from the authored source rather than reverse-engineering a
generated mesh or mistaking a render-only outfit for simulated cloth.
