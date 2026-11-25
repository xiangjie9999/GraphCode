# Author hair with GraphCode Groom

*A GraphCode guide. Read [ASSET-QUALITY.md](ASSET-QUALITY.md) first: hair is a visible asset and
follows the same order of work, references first.*

GraphCode Groom is a separate optional product; confirm it is installed. A `.gcgroom` is a **whole
recipe** that grows and styles a native Unreal Groom. It is not the `.gc` projection of that Groom.
`gc_apply` takes the complete document; ask `gc_apply {"help":true}` for the current call and send it
as a dry run first.

**`gc_describe kind:"groom"` describes both of Groom's files (2026-10-21):** under `recipe`, the
`.gcgroom` grammar (the header, the nine entry kinds and every word each body accepts, with `block:`
for one body: `entries`, `strand`, `region`, `layer`, `feature`, `build`), and beside it the built
Groom's own `.gc` settings blocks. `kind:"hair"`, its name from 2026-10-19, still answers the same. This guide is where it is taught: the proven recipe below, and what each part of
it does. The dry run is the grammar check. Do not invent words from a design note.

## You want → write

Look here before you write a workaround or ask for new grammar. Each row names the spelling in one
line; the last column is where the tool or this guide explains it
(`gc_describe kind:"groom" block:"<entries|strand|region|layer|feature|build>"`). If the tool's help and a row
disagree, the help is right.

| You want | Write | Ask or read |
|---|---|---|
| hair that lies down instead of standing out | `depth=0.5` to `1` on the `layer(...)` | the first table below |
| a zone whose edge fades into its neighbour | `falloff(<cm>)` in the region: a cross-fade that width, centred on the edge. Two neighbouring zones that both fade add to 100% on their shared edge | `block:"region"` |
| a band whose weight fades toward its edges | `profile($P, domain=across)` in the region | `block:"region"` |
| hair that has already left the root turned over an ear or off the face | `comb(toward, from=, to=)` | [GROOM-STYLING.md](GROOM-STYLING.md#comb-and-lift-grammar) |
| a groom that survives a MetaHuman assembly and fades with distance | `Game = lods(strands, strands(50%))` and `Hair = $Coat [lods=Game]` in the `build { }` block; see "Levels of detail" below | `gc_describe kind:"groom"` (the optimisation path) |
| to know how much scalp shows, or whether locks weave | read the build note: `scalp_visible=` per layer, `lock_crossings=` | the first table below |
| a braid | import `/GraphCodeGroom/Presets/Braids` with `groom(...)` and call `$Braids.Plait(in=$Region)` inside a layer | `block:"feature"`; [GROOM-STYLING.md](GROOM-STYLING.md) |
| a plait that finds its own route | `plait(...)` with no points and no `from=`/`to=`, then a constraint after it that pins its far end; without that pin it is refused and says so | `block:"feature"` |
| thick short fur, plush or velvet | `[shells=Coat]` on the build line and a `shells(...)` mesh line | "Thick short fur as shells" below |
| anime or toon hair: solid locks, a toon look, an outline | a `card()` fibre, `[cards=Hair]` on the build line and a `cards(...)` mesh line | [ANIME-HAIR.md](ANIME-HAIR.md): cards lie by different rules from strands |
| a wing of feathers, stylised or realistic | `stiffness(rigid)` in a `solid()` (stylised) or in a flat `card()` with `paint(alpha)` (realistic), rows of layers on an arm mesh | [FEATHERS.md](FEATHERS.md): rigid pieces lie by different rules from hair |
| hair seen moving before the game | `gc_render kind:"scene"` on the Groom with `animation:` and `simulate:true` | [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md) |
| a hair strand cap (Unreal stops at 255 points a hair) | `optimise(points=240, in=$Scalp)` last in the layer | the first table below |
| brows | the brows example below: a thin region per zone, a flow and a comb for each | "Human eyebrows example" below |
| hair gathered into a tie, a bun or a half-up knot | a feature's `Gather = constraint(at=, gap=, in=$Scalp, pull=, clamp=)` and `spline(at=$Gather)` from the region; the half-up recipe is in the styling guide | `block:"feature"`; [GROOM-STYLING.md](GROOM-STYLING.md) |
| a few wandering hairs on a style | `Stray = stray($Strands, amount=3%)` inside a feature, then `cut($Stray, gap=2)` to trim them short | the strays paragraph below the first tables |

## Why hair comes out bad, and what fixes it

Every row below happened on real heads and was fixed. Check this table before tuning anything.

| What you see | Cause | Fix |
|---|---|---|
| Hair stands straight out like quills: a "porcupine", a nest, a mane, a radiating sphere | The layer has no `depth=`, so every hair leaves along the skin's normal | `depth=0.5` to `1` on the layer: the follicle starts under the skin, so the hair leaves already turned along its flow. Stiffness cannot fix this: stiffness 4 made a nest and 16 made a mane. |
| Stray hairs flying everywhere; a frizzy, messy mass | No clumps, no settle, lifts too large, stiffness too high | Start from the proven coat below: two clump passes, a short settle, lifts of 0–15%, stiffness 2.5–3.5 with a profile that softens toward the tips |
| A bald oval, or hair parting in every direction round one spot | A flow's direction aims straight into the head somewhere inside its own region (its *pole*) | Each region's direction must not point into the head anywhere in that region. Split the scalp into regions that each fall one way. |
| A region's flow seems to do nothing | Flows are ordered: a later flow takes follicles from earlier ones. A whole-scalp flow written last takes everything. | Write the general flow first, then the specific ones |
| Hundreds of loose hairs left under the coat | A feature (a braid, a gather) called on a region that already has a fade profile applies the weight twice | Give the feature an unweighted twin of the region for its `in=` |
| Hair launched metres off the head, or through the ears | Open mesh edges or thin flaps (fixed in the engine on 2026-09-25) | If the build reports a lift in metres on a new mesh, stop and report it; do not tune the recipe around it |
| Looks right in a close-up of the crown, wrong from the front | Judged from one camera | Always judge from the front, three-quarter, side, back and top |
| The build note says `optimise_automatic=N` (or, rarely, refuses a curve with too many points) | Unreal caps a hair at 255 points; the build simplified those hairs to the cap itself | `optimise(points=240, in=$Scalp)` as the last geometry step, to spend the budget where you choose |
| A lock refuses "holds nothing ... N of them are already held by '$OtherLock'" | Two features' `in=` regions overlap once laid on the head, so both take the same hair | Draw the regions apart; a region drawn in the air lands nearer the middle of a round head |
| You need to know how much scalp shows, or whether locks weave across each other | — | Read the build note: `scalp_visible=` per layer (share of its roots a viewer would see skin beside) and `build: lock_crossings=` (pairs of locks, in any layers, that swap sides down the fall) |
| A plait refuses: "this plait's spine bends to a radius of ... tighter than the ... the braid reaches either side of it" | The route turns more sharply than the braid is wide, so the inside of the bend would fold the figure over | Open the bend, or author a narrower `across=`; the refusal gives the radius and the distance along the run |
| A plait refuses: "laid from the pin it leaves to the pin that holds its far end" | A plait written with no points and no `from=`/`to=` is laid between its pins, and nothing after it pins its far end | Give it a run (points, or `from=`/`to=`), or pin its end with a constraint |

## Plan the hairstyle before writing it

1. **References.** Front, side, back and top views; the parting; the length in each zone; the volume;
   the hairline; any gathered features (a braid, a tail, a bun). For a character wearing a helmet or
   hood, decide whether the hair is hidden, compressed, or only the part that shows.
2. **Zones.** Split the scalp into regions, each falling one way: the two halves either side of the
   parting, the band along the front hairline, the back of the head. Write down which way each falls.
3. **Measure the scalp on the actual head.** Render the head and use the render tool's hit query
   (`gc_render mode:"ask"`) to get points on the skin: about 14 round the hairline from the forehead,
   over each ear to the nape, plus a crown point inside the loop. Write them once, as regions.
4. **Start from the proven coat.** Change the regions and flows first. Leave the fibre, clumps and
   settle alone until the hair lies on the head.
5. **Dry run, build, render** all five views, then change one thing at a time.

## The proven recipe

Before adapting this recipe, follow [hairstyle research and construction](GROOM-STYLING.md).
Agents must research the requested hairstyle or coat and how it is made, then map its sections,
gathers, crossings and support into Groom. That guide includes comb/lift grammar, a plait example
and British Shorthair guidance. A successful build is followed by rendered inspection.

This style was built and seen lying correctly on a MetaHuman head. It is a centre parting: each half
falls to its side, the front band sweeps sideways and back so the face is clear, and the back of the
head falls straight down. About 28,000 hairs, 24–30 cm, built in about 50 seconds.

`mesh:` names the **MetaHuman character**. The hair grows on the face the MetaHuman editor builds,
and the build saves that face as `<binding>_Head` and makes it the binding's source. Any MetaHuman
that wears the binding gets the roots at the same place on its own scalp.

**The coordinates are that head's. Measure your own.**

```gc
gcgroom 1 mesh:/Game/Characters/Hero/MH_Hero asset:/Game/Characters/Hero/Hair/GR_Hero_Hair binding:/Game/Characters/Hero/Hair/GB_Hero_Hair gen:1 tolerance:0.02

Hair = material('/Game/Characters/Hero/Hair/M_Hair')
Taper = profile(0=100%,0.7=100%,0.9=45%,1=5%) [linear]
Flex = profile(0=100%,0.2=100%,0.6=70%,1=55%) [linear]
SoftWave = profile(0=0%,0.25=0%,0.55=70%,1=100%) [linear]
Tips = profile(0=0%,0.6=0%,1=35%) [linear]
Fibre = strand() {
  width(random(min=0.006,max=0.009),profile=$Taper)
  stiffness(random(min=2.5,max=3.5),profile=$Flex)
  wave(random(min=2.2,max=3.2),around=random(min=-90,max=90),profile=$SoftWave)
}
# The hairline: fourteen measured points, forehead over each ear to the nape. The midpoint (a crown
# point inside the loop) says which side of the loop is the scalp.
Scalp = region() {
  points((0.03,9.63,168.89),(3.56,8.90,168.15),(5.96,7.25,165.32),(6.49,6.68,162.0),(7.2,1.77,163.72),(5.37,-1.95,157.59),(3.23,-3.12,154.2),(-0.04,-4.42,153.82),(-3.3,-3.04,154.21),(-5.45,-1.9,157.6),(-7.2,1.77,163.72),(-6.41,6.79,162.02),(-5.88,7.41,165.35),(-3.49,8.95,168.16),loop=true)
  midpoint((0,1.4,171.3))
}
# Each half of the scalp, split down the middle from the hairline to the nape.
Right = region() {
  points((0.03,9.63,168.89),(3.56,8.90,168.15),(5.96,7.25,165.32),(6.49,6.68,162.0),(7.2,1.77,163.72),(5.37,-1.95,157.59),(3.23,-3.12,154.2),(-0.04,-4.42,153.82),(-0.035,-6.9,160.75),(-0.036,-6.18,168.74),(0,1.4,171.5),loop=true)
  midpoint((4.3,1.4,169))
}
Left = region() {
  points((0.03,9.63,168.89),(0,1.4,171.5),(-0.036,-6.18,168.74),(-0.035,-6.9,160.75),(-0.04,-4.42,153.82),(-3.3,-3.04,154.21),(-5.45,-1.9,157.6),(-7.2,1.77,163.72),(-6.41,6.79,162.02),(-5.88,7.41,165.35),(-3.49,8.95,168.16),loop=true)
  midpoint((-4.3,1.4,169))
}
# The back of the head, from behind each ear up to the crown: it falls straight down, so the
# parting stops at the crown.
Back = region() {
  points((5.37,-1.95,157.59),(4.43,-3.49,169.85),(-4.43,-3.49,169.85),(-5.45,-1.9,157.6),(-3.3,-3.04,154.21),(-0.04,-4.42,153.82),(3.23,-3.12,154.2),loop=true)
  midpoint((-0.035,-6.9,160.75))
}
# The front of the hairline, temple to temple and 4 cm back: swept sideways and back, clear of the eyes.
Front = region() {
  points((-5.88,7.41,165.35),(-3.49,8.95,168.16),(0.03,9.63,168.89),(3.56,8.90,168.15),(5.96,7.25,165.32),loop=false)
  widen(4)
  intersect($Scalp)
}
# The parting, weighted from the middle: full strength along the part, fading to nothing 7 cm down
# each side, so hair near the part combs sideways and hair further down falls, with no seam.
FromPart = profile(0=100%,0.3=100%,1=0%) [linear]
Parting = region() {
  points((0.03,9.63,168.89),(0,1.4,171.5),(-0.036,-6.18,168.74),loop=false)
  widen(14)
  profile($FromPart,domain=across)
}
TopRight = region() {
  add($Parting)
  intersect($Right)
}
TopLeft = region() {
  add($Parting)
  intersect($Left)
}
FrontRight = region() {
  add($Front)
  intersect($Right)
}
FrontLeft = region() {
  add($Front)
  intersect($Left)
}
Coat = layer(region=$Scalp,strand=$Fibre,density=90,length=random(min=24,max=30),depth=1,distribution=jittered,jitter=55%,seed=73,material=$Hair) {
  flow(direction=(0.15,-0.4,-1),from=$Right,weight=100%,lift=5%)
  flow(direction=(-0.15,-0.4,-1),from=$Left,weight=100%,lift=5%)
  flow(direction=(1,-0.35,-0.45),from=$TopRight,weight=100%,lift=0%)
  flow(direction=(-1,-0.35,-0.45),from=$TopLeft,weight=100%,lift=0%)
  flow(direction=(0,-0.3,-1),from=$Back,weight=100%,lift=4%)
  flow(direction=(1,-0.8,0.1),from=$FrontRight,weight=100%,lift=15%)
  flow(direction=(-1,-0.8,0.1),from=$FrontLeft,weight=100%,lift=15%)
  style(in=$Scalp) {
    clump(radius=0.5,strength=45%,profile=$Tips)
  }
  settle(time=0.1,gravity=100%,damping=85%,in=$Scalp)
  optimise(points=240,in=$Scalp)
}
build {
  Hair = $Coat
}
```

Name the recipe file apart from the Groom it builds (for example `Hero_Hair.gcgroom` building
`GR_Hero_Hair`). A recipe named exactly like its Groom is routed to the Groom's own `.gc`.

## What each part does

- **Header.** `mesh:` is the head, or a MetaHuman character; `asset:` is the output Groom; `binding:`
  is required on a skeletal target. `tolerance:` is the curve accuracy in centimetres.
- **`strand()`** is the fibre. `width` tapers along a profile. `stiffness` is how far a hair travels
  before it can turn 90°; the `Flex` profile softens it toward the tips. `wave` adds a soft wave that
  starts partway down.
- **`region()`** is an area of scalp: a loop of measured `points` with a `midpoint` inside, or a line
  `widen`ed into a band. `profile(..., domain=across)` fades a band's weight toward its edges.
  `add`, `intersect` and `exclude` combine regions.
- **`layer(...)`** grows one population: `density` (hairs per cm²), `length` (a range),
  `depth` (the follicle under the skin, so hair lies down), `distribution=jittered` with `jitter=`,
  a `seed`, and the `material`. Density and length come before any geometry step.
- **`flow(direction=, from=, weight=, lift=)`** sets which way the hair in a region goes. The
  direction is projected onto the scalp at each root. `lift` is how much it rises off the head before
  following the direction: keep it small (0–15%). Write general flows before specific ones.
- **`comb(...)`** turns the hair that passes over a region, wherever it grew. The region can be
  anywhere on the head or body: over an ear, along the face, on the shoulders. Use it for where the
  hair should lie once it has left its root (side hair over the ears, the front off the face, long
  hair draped off the shoulders); flow only decides how it leaves the root. Grammar, strength, lift
  and blended controls: [GROOM-STYLING.md](GROOM-STYLING.md#comb-and-lift-grammar).
- **`style(in=) { clump(...) }`** gathers hairs into locks. A second, smaller clump pass breaks the
  tips into finer locks. `GR_AcrossExp`, the longer coat this came from, used two passes:
  `clump(radius=1.8, strength=70%, profile=$Locks)` then `clump(radius=0.5, strength=45%, profile=$Tips)`.
- **`settle(time=0.1, gravity=100%, damping=85%, in=)`** lets the hair fall briefly under gravity and
  rest on the head. Short is right; a long settle loses the style. A layer's hair rests on the hair of the
  layers built before it; hair within one layer does not rest on itself (measured 2026-10-21).
- **`optimise(points=, in=)`** spends each hair's point budget where it turns, under Unreal's cap.
- **`build { }`** names the hair groups inside the one Groom asset.

## Build and judge

1. Dry run the whole document; read every refusal and note.
2. Build. Read the built Groom and its Binding. Each build measures how much memory it took and the next
   build of that Groom waits until that much, plus 2 GB, is free; the refusal says how much is needed and
   free, and the dry run's `memory:` note says it ahead of time.
3. Render the front, three-quarter, side, back and top, with the hair material under the game's
   lighting. Check the roots, parting, coverage, direction, clumping, the silhouette past the head,
   and anything passing through the ears, neck or clothing.
4. Put the binding on the character: the character's `.gc` `outfit { Hair = <the binding> }`. Render
   the dressed character, then run an animation: hair follows its simulated guides at runtime. To see
   it moving before that, `gc_render kind:"scene"` on the Groom with `animation:` (a clip for its
   target's skeleton) and `simulate:true`; [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md)
   has the order for a character with hair and clothing in motion.
5. Save, reload and look again.

A green build or a contact number is not approval. The picture is.

## Braids, gathers and deliberate strays

For a braid, import the shipped `/GraphCodeGroom/Presets/Braids` **as a library** inside a complete
recipe. It has no mesh, asset or `build` of its own. Fit its columns and ties to the measured scalp.
Braids, ties, bands and presses are features (`feature()`, `plait`, `pull`, `press`, `constraint`)
and are advanced. The dry run is their grammar check, and every one must be judged in the render. A
braid that compiles but looks like a rope has not met the requirement.

Natural strays are authored, not accidents. `Stray = stray($Strands, amount=3%)` inside a feature
picks a few hairs that leave the braid partway, and `cut($Stray, gap=2)` trims them short. Frizz is
hair that stops partway along its route and hair a later feature did not select, never hair launched
off the head.

## Human eyebrows example

`Content/__MCPTests/GroomMetaHuman/MH_Brows.gcgroom` (Unreal path `/Game/__MCPTests/GroomMetaHuman/MH_Brows`)
grows both brows on a MetaHuman character, so any MetaHuman can wear its binding `GB_MH_Brows`. It is a
GraphCode development-project fixture. Its research, with sources, is `Docs/Research/human-eyebrow-construction.md`.

- **Place the brow by the face, then measure it.** The head sits above the inner eye corner, the arch on
  the line from the nose wing through the pupil, the tail on the line from the nose wing through the
  outer corner; the arch's centre about 2 cm above the pupil. The outline is measured on the head with
  `gc_render mode:"ask"`: upper edge, lower edge, the tail's tip. About 5 cm long, 0.6 cm tall,
  tapering to 0.2 cm at the tail.
- **One outlined region per brow, its zones as bands.** A brow is thinner than a midpoint can reliably sit
  inside, so the head, upper half, lower half and tail are `points(...)` lines `widen`ed and
  `intersect`ed with the brow, which need no midpoint. A thin zone with its own loop and midpoint is
  refused when its midpoint lands on the boundary.
- **The hair follows the triple pattern.** The head's hairs point up; the body's upper half down and
  out and its lower half up and out, so they converge gently along the middle (a slope of about 0.3; at
  0.55 the brow read as a cross-hatch); the tail out and slightly down. General flow first, the head last.
- **Flows launch the hair; combs lay it along the skin.** Each zone's flow aims at a target well past
  it: `flow(toward, from=$Tail, to=$PastTheTail, mapping=midpoint)`, `flow(toward, from=$Head,
  to=$UpTheForehead, mapping=midpoint)` (about 1.2 cm past the tail's tip, about 5 cm up the forehead,
  or the hairs meet in a sheaf; a destination may lie outside the growth region, its own surface then
  gives its centre). A flow only decides how a hair leaves its root: short stiff brow hair then runs
  straight and stands off wherever the skin curves away (the brow ridge at the head, the temple at the
  tail). So give every zone a `comb(toward, from=$Pass, to=$Target)` with its flow's target: it turns
  the hair along the skin toward it. A comb steers only over its footprint, so draw footprints where
  the hair passes, not where it grows: the brow's middle line `widen`ed past both edges, the tail's
  line out to its tip, a line from the head up the forehead. Measured on MH_RigProbe: median hair 0.33
  of its length off the skin without the combs, 0.13 with them (5-10 degrees is 0.09-0.17).
- **It lies almost flat.** A follicle turns its hair under the skin on the hair's bend radius
  (stiffness x 2/pi), so `depth` must reach the stiffest hair's: `depth=0.85` for
  `stiffness(random(min=0.9, max=1.3))` (at 0.7 the stiffer half surfaced 13-16 degrees off). Lift 1-2%.
  Hairs 0.6-1.1 cm, shorter at the head (`length($Head, scale=0.6)`),
  the tail shorter and sparser (`density($Tail, scale=0.7)`). Set the density from the build note's
  `area=` for the sourced 200-240 hairs a brow: on MH_RigProbe each brow measures 2.09 cm2, so 110
  hairs/cm2 gives ~230. Clump very lightly (radius 0.05, 10%); a scalp's clump drags short brow hair
  into wisps.
- **Judge it close.** At a head-and-shoulders view a light brow reads as a soft band; check the pattern
  in a close-up of one brow, its flatness from the side and the tails from above (they must follow the
  face's outline, not stand past it). Colour is the material's choice. In GraphCode's development project the spec
  `GraphCode.Groom.Look.HumanBrows` builds the example and draws those views under `Saved/GroomBrows/`.
- **A brow from a photo.** Trace its upper and lower edges against the photo's pupils, in units of the
  distance between them, and place them by the character's own pupils (its eye joints) at that scale;
  the lines, targets and footprints follow from the outline as above. `MH_BrowsStraight.gcgroom` is a
  straight, thick, dark brow made this way from a reference photo, with the character's own brow
  material (`GraphCode.Groom.Look.StraightBrows`, which also draws it framed as the photo was).

## British shorthair example: coat, whiskers and eyes

GraphCode's British shorthair is a worked example of **Shape + Groom + materials**. The coat and
whiskers are `.gcgroom` recipes; the eyes are material assets fitted through `.gcshape`. Use this
composition as a reference for an animal, measuring its own skin and eye geometry first.

The local example is under `Content/__MCPTests/GroomCat/BritishShorthairBare/` (Unreal path
`/Game/__MCPTests/GroomCat/BritishShorthairBare/`). These are GraphCode development-project fixtures, not a
shipped preset or assets guaranteed to exist in a new project.

| Read this example | What it demonstrates |
|---|---|
| `BritishShorthairSections.gcshape` and `BritishShorthairSmooth.gcshape` | Prepare the imported skin, remove source whisker geometry, and give anatomy named material slots for Groom regions. Protect eyes, nose, mouth and paw pads from fur. |
| `BritishSculptedCoat.gcgroom` | Compose down, awn and guard fibres into four output groups, with two independent half-density undercoat populations. |
| `BritishSculptedUnderLibrary.gcgroom`, `BritishSculptedOuterLibrary.gcgroom`, `BritishSculptedGuardLibrary.gcgroom` | Slot-based zones, softened length transitions, regional flow/lift and small clumps; the outer libraries also carry the local eye-clearance refinements. |
| `BritishFineWhiskers.gcgroom` | Separate upper/lower muzzle and brow populations, measured root bands, taper and directional fans. |
| `BritishFelineFace.gcshape`, `M_BritishFelineEyes`, `MI_BritishFelineLeft`, `MI_BritishFelineRight` | Separate eye projections and per-eye iris orientation, with a vertical oval pupil and a round outer iris. |
| `BP_BritishShorthairGroom` | The assembled appearance, including component material and width overrides. Judge this as well as the individual Groom. |

For example, this excerpt from `BritishSculptedCoat.gcgroom` declares the soft support fibre and
imports one population from the sectional library. It is **part of that recipe**, not a standalone
document: `$Taper`, `$UnderWave` and `$UnderMat` are declared earlier, and the full `build` also emits
`UndercoatB`, `AwnHair` and `GuardHair`.

```gcgroom
Down = strand() {
  width(random(min=0.0045,max=0.00675),profile=$Taper)
  stiffness(random(min=3.0,max=4.0))
  wave(random(min=0.20,max=0.30),around=random(min=-180,max=180),profile=$UnderWave)
}
UnderA = groom('/Game/__MCPTests/GroomCat/BritishShorthairBare/BritishSculptedUnderLibrary',Fibre=$Down,Hair=$UnderMat,Density=73.125,Seed=3101)
build {
  UndercoatA = $UnderA.CoatLayer
}
```

The reusable lessons are:

- **Give each population a job.** Undercoat supplies fullness; awn and guard hair supply the surface
  direction and fine variation. Shape the cheeks, chin and ruff through regional length and lift.
  Larger clumps made this cat look spiky; finer clumps and partially released tips retained plushness.
- **Separate coverage from transitions.** Keep a hard growth exclusion over bare anatomy while
  softening neighbouring length controls. A faded zone must not expose long body-default hair at
  the eyelid or muzzle. Clear pupil-crossing fibres with local flow and eye-rim length controls.
- **Judge the assembled materials.** The direct Groom preview uses a neutral support surface.
  It cannot establish the final skin/fur colour match or Blueprint overrides. Check root-to-tip,
  per-strand and anatomical colour variation on the complete cat.
- **Treat eyes as their own optical/material problem.** The latest recorded face uses
  `M_BritishFelineEyes`, reusing MetaHuman `MF_customIris` and matching structure/normal maps.
  Its pupil remap moves pigment, detail sampling and the pupil mask together while holding the
  outer iris fixed. Left/right instances vary iris orientation without rotating the vertical
  aperture. Keep smooth corneal reflections separate from iris detail and painted catchlights.
  This example depends on the installed MetaHuman material functions and maps.
- **Compare one change at a time.** Keep the approved coat beside a new candidate, use the same
  front, quarter, side and muzzle cameras, and wait for shader and Groom warmup before judging.
  The recorded result is static look development, not proof of animated binding, runtime cost or
  final user acceptance. The older `GrayCat` coat is a separate baseline on a different mesh.

In GraphCode's development repository, the fur evidence and remaining differences are in
`Docs/GC-GROOM-BRITISH-BLENDER-LOOKDEV.md`; the current eye/face asset map and superseded eye
experiments are in `Docs/Design/Assets/BritishShorthair/FACE-MATERIALS.md`. Read those records before
rebuilding the example; do not copy an older experiment merely because its filename says "final".

## Thick short fur as shells

Dense short hair that stays near the skin (an undercoat, velvet, plush, a rug's pile) can be drawn as
**shells**: a stack of copies of the skin, each showing the cross-sections of the fibres at its
height. Opacity costs nothing per fibre, so a coat far too dense for strands is cheap. Hair that
leaves the skin (a braid, a ponytail, long guard hair, whiskers) cannot be sliced and stays strands.

The layer is written exactly as before; only its build line changes:

```gcgroom
build {
  Guard = $Guard
  Under = $Under [shells=Coat]
  SM_CoatShells = shells(Coat, count=24, resolution=1024, material=$Wool)
}
Wool = material('/Game/Rugs/MI_Wool')
```

- `[shells=Coat]` draws that layer into target `Coat` **instead of** as strands. `Guard` above stays
  strands, so one coat can be both.
- `SM_CoatShells = shells(Coat, ...)` makes the mesh, beside `asset:`, with its fur maps
  (`_Fur`, `_Lean`) and material instances (`_Material`, and `_FinMaterial` for the fins). Only the
  target is required: `count` 2–64 (default 16), `resolution` a power of two (default 1024),
  `fins=true|false` (default on: strips along the outline so it does not step), `lods=` (default:
  the target mesh's own LOD count, halving the shells each level). A skeletal target makes a skinned
  mesh on its own skeleton; put it on a Skeletal Mesh Component that follows the body.
- Every word of the layer still counts: region, density, length, flow, clumps and settle all reach
  the shells, because the maps are baked from the finished hair. The maps use the mesh's own UV0;
  overlapping UVs are reported, and so are fibres that lean across a UV seam (`leans_across_seams`),
  which keep their own triangle's direction past it.
- **Any unwrap works; a bad one is named.** Where UV0 gives a part more or less texture per cm, the
  shells still carry the layer's density and fibre width in cm. Two numbers in the build note say where
  your UVs cannot: `uv_short` is the share of the skin given so little UV per cm that it holds fewer
  fibres than asked, and `uv_fine` the share given so much that a fibre is wider than its cell and is
  clipped square. Both
  near 0% is a clean unwrap; fix a large one in the UVs, not in the recipe. Stretch that differs
  between U and V (a sphere's latitude-longitude unwrap) is corrected in the material, so fibres stay
  round.

**Colour.** A hair material cannot draw a mesh, so shells use GraphCode's `M_GcFurShell` (made in
`/Game/GraphCode/Groom/` on first use). Its look is `RootColour`, `TipColour`, `RootShade` (how dark
the roots go) and `FurRoughness`. Set them one of two ways:

1. **Your own instance (recommended).** Make a Material Instance of `M_GcFurShell`, set the colours,
   and name it with `material=`. Rebuilds keep it; several shell meshes can share it.
2. **Directly on the generated `<Name>_Material`.** A rebuild rewrites only the maps and
   measurements it bakes, so your colours survive. But a colour set here hides any `material=` you
   add later, until you clear it.

Do not edit `M_GcFurShell` itself: a plugin update re-applies it. The fins follow either way. The
strands in the same coat still use their layer's `material=`, so match the two by eye.

**Judging it.** The first render after a build can show Unreal's grey checker while the shell
material compiles, and the next a soft, uncombed pile while the new maps stream in; judge the one
after. Up close each fibre is drawn at the layer's `width`, so a pile that looks thin at a few
centimetres wants a wider `width(...)`, not more shells. A strand Groom standing over shells casts Unreal's hair shadow onto them as blotches coarser
than a fibre; if the pile looks dirty, try the Groom component's `Cast Shadow` off.

## Current limits

- ~~The `.gcgroom` grammar is not in `gc_describe`; this guide and the dry run are the route.~~
  Superseded 2026-10-19: ~~`gc_describe kind:"hair"`~~ — and 2026-10-21: `gc_describe kind:"groom"`.
- `gc_render` on a Groom no recipe built (a stock MetaHuman groom) draws it on the head of the Groom
  Binding made for it; render the Binding itself to choose which head.
- Groom cards (flat strips baked from strand layers) are built only through a `lods(...)` path, never on
  their own (see "Levels of detail" below); the path's `mesh()` rung is not built yet and a real build
  refuses it by name. Solid card locks for anime hair are a different thing and do build on their own: a
  `card()` fibre and a top-level `cards(...)` mesh line, [ANIME-HAIR.md](ANIME-HAIR.md).
- `distribution=relaxed` is parsed but refused; use `jittered`.
- Unreal owns interpolation and simulation. The recipe owns the styling **and the level-of-detail chain**
  (the next section); a groom with no chain has only what Unreal gave it, and that can cost you the groom.

## Levels of detail

A groom with no level-of-detail chain can vanish where you did not expect. Epic's MetaHuman assembly
decides per groom whether it will ever be seen: it asks for the highest LOD at which the groom is visible
(`GetMaxVisibleLOD`, in Epic's `MetaHumanGroomEditorPipeline.cpp`) and drops the groom from a Production
build when that is lower than the lowest face LOD the build keeps. A groom visible only at LOD0 is
therefore dropped from an assembly whose face starts at LOD1; the field report met this as a groom dropped
from the assembled character.

Write the chain in the recipe, so the groom is visible at LOD1 and beyond:

```text
build {
  Hair = $Coat [lods=Game]
  Game = lods(strands, strands(50%))
}
```

- `strands` is the groom as built; `strands(50%)` is Unreal's own curve decimation. Cards and a mesh
  follow in the same line when you want them (`cards(angle=10)`, `mesh()`); rungs run strands, then
  cards, then mesh, and each takes over at half the screen share of the one before.
- Every build line that names `Game` shares the one chain. A path makes the Groom's LOD mode Manual and owns
  that group's `lods` and `cards`, so a rebuild writes the path's settings, not the Groom's; a group without a
  path keeps what was set by hand.
- The tool's help holds the whole rule, including card widths, atlases and what a mesh rung needs:
  `gc_describe kind:"groom"` (the optimisation path). Do not copy the rung arguments from this page.
- Check it the way the assembly will: build, assemble the character, and look for the hair. A
  `lods(...)` line that builds is not proof the assembled MetaHuman kept the groom.
- `pipeline=optimized` strips face LODs that `pipeline=cinematic` keeps ([GCHUMAN.md](GCHUMAN.md)), so a
  groom that survived a cinematic assembly can still go in an optimised one.
