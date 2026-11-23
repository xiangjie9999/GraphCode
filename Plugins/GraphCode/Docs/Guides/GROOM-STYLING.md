# Groom styling: research, construction and combing

Read with [GROOM.md](GROOM.md), which owns the complete starting recipe, rendering and binding.

Save the recipe as a `.gcgroom` file under Content. For public apply, use its extensionless Unreal
package as `asset_path` (for example `/Game/Hair/Sweep`) and send the complete recipe as `ir`.
The header's `asset:` names the generated Groom. This preserves addressable recipe provenance
so the native Groom render can load its target mesh after an editor restart.

## Research the actual style before authoring it

**Agents: research the requested hairstyle and how a professional constructs it before writing
the groom.** Read the project's shared research first. Find primary references: a stylist's
demonstration for a hairstyle, and a breed association's standard plus photographs for an animal.
Open the material and examine the method; search snippets and a remembered style name are not
enough. Record source links and relevant video timestamps when actually inspected. Never claim
to have watched footage when only its written tutorial was available.

Write a short construction brief: reference silhouette from front/side/back/top; parting and
growth direction; sections and lengths; which sections join where; over/under crossings; ties,
pins or other support; texture and deliberate loose hair. Separate sourced observations,
measurements on the target, and artistic choices. In OraStudio keep findings in `Docs/Research/`.
If a hidden side has no reference, label the design assumption rather than presenting it as fact.

Build in that physical order. Establish the cut and growth first, brush the loose hair, gather
the required sections, make the feature, then finish free ends. A named hairstyle is not one
direction vector. Inspect the silhouette before adding flyaways; inspect individual crossings
and attachment points before increasing density. Do not use noise to conceal a failed structure.

## Choose the operation by what it selects

| Intention | Operation |
|---|---|
| Hair growing from this scalp/body region departs this way | `flow(..., from=$Region)` |
| Hair currently passing over this region turns this way, wherever it grew | `comb(...)` |
| Hair must gather at an elastic, pin or shaped aperture | A feature's `constraint` and `pull` |
| Three groups must cross as a plait | Three routed sections, e.g. the shipped Braids library |
| Hair beneath a later layer must lie close to the surface | `press(in=$Region, gap=...)` |

**Comb works anywhere on the body, not only on the scalp.** Its region can be drawn on the ears,
the edge of the face, the neck or the shoulders: to lay the side hair over the ears, keep the front
off the face, or drape long hair forward or back off the shoulders, put a comb region there with
the direction the hair should take. This is the Sisir surface-comb approach (source below): the
author places directions on the surface and every hair passing over it follows them. Flow only sets
how each hair leaves its root; past that, hair falls under gravity unless a comb says otherwise.

Comb acts on the current layer. Selection is evaluated at the nearest target-surface point under
the incoming strand, including the actual material slot. It can therefore reach hair passing
over a slot where that layer has no follicles. A subsequent comb reads the changed shape.
Use a region profile to feather a brush boundary. A hard region with high strength deliberately
makes a concentrated bend; reducing strength or fading the footprint gives a softer sweep.

## Comb and lift grammar

These are layer-body excerpts. Declare the named regions on the actual target first.

```gcgroom
comb(toward, from=$Temple, to=$BehindEar, strength=70%)
comb(($Temple, $BehindEar, $Nape), strength=70%)
comb(direction=(0,-1,0), in=$Temple, strength=70%)
comb(toward, from=$Temple, to=$BehindEar, lift=30%, strength=70%)
comb(in=$Crown, lift=60%)
```

`toward` aims along the shortest path over the skin to the destination region's midpoint, so a
destination around a curve (a brow's head toward the temple) is reached the way the skin goes,
not by a line through the head flattened at the hair (which on a brow ridge turns into "up").
`flow(toward, ...)` heads the same way.
A chain follows successive named regions; it requires at least two, without repeats. Vectors
use the target mesh's local coordinates. `from`/`in` identify the passing-hair footprint, not roots.
`style(in=$Temple) { comb(lift=30%) }` supplies the same selection to its inner comb.

| Parameter | Meaning |
|---|---|
| `strength` | Default 100%. Amount each reached portion turns, multiplied by region weight. 50% turns all reached hair partway; it does not choose half the population. |
| `lift` | Default 0% for directional comb. Blend the heading toward the local outward skin normal. Lift alone retains the current heading at 0%; 100% aims along the normal. It does not specify a height. |
| `reach` | Optional nonnegative distance in centimetres from the target surface. Omit to use the whole above-surface footprint; set it to avoid catching distant overhanging locks. |
| `mix` | Default `replace`: an ordered stroke on the current shape. Consecutive `mix=blend` controls contribute to one direction field on the same incoming shape. |

There is no separate `lift()` verb and no symbolic `direction=up` in this grammar.
`direction=(0,0,1)` means mesh-local +Z: on a cheek that is sideways along the face, whereas
`lift=100%` aims away from the cheek. Use `flow(..., lift=...)` for the follicle departure;
comb preserves the root launch and any prefix already fixed through a feature landmark.
It turns free hair beyond that prefix, so it cannot relocate a braid's pins or lift a tied base.

Write root controls (`flow`, `curl`) before comb. An early comb runs before features/clumps;
a comb written after those runs in the late stream with cuts, presses and settles. Do not put
another feature after a late comb. Consecutive combs are ordered. Later settling can change the
styled result. The turn respects bend length and preserves material length; it is not a new
length control or a guarantee that the final silhouette matches the reference.
## Natural variation and blended surface controls

Use a layer-local composition to vary comb response. The region sets the footprint; the
composition sets influence within it. Start with modest smooth noise so neighbouring hairs
vary together; independent strand noise is useful for smaller irregularities.

```gcgroom
Brush = composition($Temple, weight=65%, edge=65%,
                    noise=15%, noise_mode=smooth, noise_scale=2, seed=42)
comb(toward, from=$Brush, to=$BehindEar)
```

Here the influence stays within 50–80%; smooth noise need not reach both limits. `noise_scale`
is centimetres in the region's retained surface frame. `noise_mode=strand` instead gives each
hair a stable seeded variation. Rebuilding with the same inputs repeats the result; there is no
time-dependent flutter. `noise=0%` removes the current composition's noise. An omitted seed
starts from the layer seed. `strength` on comb multiplies this influence once.

Composition uses the same ordered snapshots and booleans as feature composition: changing
`composition($Brush, noise=0%)` affects subsequent combs; earlier combs retain their field.
Overrides stay inside the layer. Composed weights replace the region's soft selection weights:
author `weight`, `edge`, `radius`, `axis` and `falloff` for the intended fade. Noise-only on a
previously uncomposed source starts from uniform influence. Noise never extends coverage.
Comb clamps negative influence to zero; it does not turn hair backwards. `from`/`in` accept
compositions, while destinations and chain anchors remain ordinary surface regions.

The creator's [Sisir Surface Comb demonstration](https://www.youtube.com/watch?v=3TDV4pnClrc&t=169s)
uses multiple surface vectors with individual intensity and falloff (2:49–3:45). GC adapts that
idea to passing hair with explicit blended controls:

```gcgroom
Left = composition($Passing, weight=10%, edge=75%, axis=along)
Right = composition($Passing, weight=75%, edge=10%, axis=along,
                    noise=10%, noise_scale=2, seed=91)
comb(direction=(1,0,0), in=$Left, mix=blend)
comb(toward, from=$Right, to=$Nape, lift=10%, mix=blend)
```

All consecutive blend controls read the same incoming strands. Each contributes its normalized
direction times strength and influence. GC aims along their sum and uses the sum's magnitude,
capped at 100%, as the turn response. Equal opposing controls cancel; one control behaves like
an ordinary comb. This is GC's defined blend, not a claim about Sisir's unpublished formula.
Each control keeps its own lift and reach. A different geometry operation or an ordinary comb
ends the group; composition declarations merely update subsequent control snapshots.

Sisir also offers optional guide-length changes (3:46–4:22). GC keeps length in `length(...)`
and root departure in `flow(...)`. Comb shapes existing free portions and preserves their length.
It writes a rest shape that remains until another operation or runtime simulation changes it;
use an explicit later `settle(...)` when the style should relax under gravity.

### What makes the result look natural

Treat noise as finishing variation after the silhouette, parting, sections and direction changes
work without it. Prefer smooth noise for coherent differences between neighbouring locks. Add
per-strand variation sparingly: a large independent change on every hair can produce a fuzzy,
crossing fan rather than a groomed lock. As an artistic starting experiment, try 5–15 percentage
points of variation and compare against zero; this range is not a measured universal preset.
Choose the spatial scale for the size of the intended locks and the actual target, not its name.

Feather the composed influence where a brush starts and stops, and give the strand enough bend
length for the turn. A uniform composition can replace a previously feathered region weight
with an abrupt boundary. Additive noise can also leave nonzero influence near an edge: inspect
the result rather than assuming `edge=0%` alone guarantees a zero noisy boundary. Blend nearby
directions for a broad transition, and reserve strong concentrated turns for intentional styling.
Correct exposed gaps through the authored coverage and section arrangement; random turns do
not supply missing hairs. Judge length variation, taper, clump size and loose ends against the
style reference. For British Shorthair fur, preserve the supporting coat and rounded silhouette;
for a braid, preserve the readable sections and crossings before adding loose hairs.

**Visual evidence, 2026-10-07:** the five passing-hair test variants were inspected from back and
quarter views. Smooth noise breaks up uniform sweeps coherently, per-strand noise visibly fans
individual turns, and joint controls give a gradual change of heading across the selected patch.
The proxy still has sparse coverage, exposed skin and concentrated bends at brush boundaries.
These captures demonstrate the controls and improved variation; they do not establish a finished
natural hairstyle, realistic cat coat, or collision-free result. In OraStudio the comparison is
`Saved/GroomComb/NoiseComparison.html`, with recipes in `Content/__MCPTests/GroomComb/`.
Agents must render and assess the complete intended style separately before calling it natural.

## Example: prepare and construct a three-section plait

Sam Villa's [braiding education](https://www.samvilla.com/blogs/hair-tutorials/braids-upstyles-guide)
starts with controlled, approximately equal sections and an alternating crossing sequence.
Feeding new sections along the scalp is an additional construction step. A gathered free plait
does not become a French braid merely by changing its name.

This excerpt adapts OraStudio's `Content/__MCPTests/GroomVisual/PresetBraid.gcgroom`.
Declare `$Braided`, `$LeftCol`, `$MidCol`, `$RightCol`, `$Crown`, `$Nape`, `$Fibre` and `$Hair`
first. The columns must be disjoint and carry comparable hair amounts, not merely equal areas.
The numbers below fit that fixture head in centimetres; remeasure them for another head.

```gcgroom
Braids = groom('/GraphCodeGroom/Presets/Braids',
  LeftColumn=$LeftCol, MidColumn=$MidCol, RightColumn=$RightCol,
  TieX=0, TieY=9.8, TieZ=15, TieRadius=1.5,
  StartX=0, StartY=9.8, StartZ=14, EndX=0, EndY=8.8, EndZ=-34,
  UpX=1, UpY=0, UpZ=0, Across=1.6, Through=1.04, Pitch=7.5)
Coat = layer(region=$Braided, strand=$Fibre, density=55, length=90,
             distribution=random, seed=141, material=$Hair) {
  flow(direction=(0,0.62,-0.78), from=$Crown, lift=15%)
  comb(toward, from=$Crown, to=$Nape, strength=50%)
  $Braids.Tie(in=$Braided)
  $Braids.Plait(in=$Braided)
}
build {
  Hairstyle = $Coat
}
```

The comb prepares the incoming loose hair; the tie gathers it and the three plait routes make
the crossings. The inserted comb is an adaptation, not a claim that this exact braid has been
visually approved. The shipped library is a reusable feature library without its own mesh/build.
Keep enough strand length for travel from scalp to tie, all crossings, and the free end. Inspect
the whole tie aperture against the skull. Check crossing order, ribbon breadth, taper and the
braid's facing from the side as well as behind. A round twisted cord is a failed plait silhouette.

For an updo, research its support too. Sam Villa's
[donut braided updo](https://www.samvilla.com/blogs/hair-tutorials/pulled-donut-braided-updo)
gathers and secures a ponytail before distributing hair around a support and braiding it.
Our translation is to model those gathers, support and pins explicitly; combing alone cannot
provide the hold. Inspect the final feature with its support visible and again in the full style.

## Example: British Shorthair plush coat

[TICA's breed description](https://tica.org/breed/british-shorthair/) identifies a short, very dense,
plush coat with a crisp feel, a compact body and rounded cheeks. Translate that reference into
short supporting fibre, fine surface direction and a restrained broken silhouette. Those are
our art decisions, not density or length measurements supplied by the breed standard.

Start from the four-population `BritishSculptedCoat.gcgroom` described in
[GROOM.md](GROOM.md#british-shorthair-example-coat-whiskers-and-eyes). Its two undercoat populations
provide support; awn and guard groups describe the surface. Apply a comb inside the appropriate
outer layer before clumping, not to an imported output handle after `build`.
For example, after declaring measured `$Cheek`, `$BehindJaw` and `$Shoulder` regions:

```gcgroom
# Inside the outer-coat layer, after its root flow:
comb(toward, from=$Cheek, to=$BehindJaw, strength=25%, lift=35%, reach=3)
comb(in=$Shoulder, lift=25%, strength=30%, reach=3)
```

These values are starting experiments, not an approved cat preset. Keep the undercoat's fullness;
brush only surface hair that needs to turn. Do not flatten the whole coat or create large spiky
clumps to suggest plushness. Use shorter/finer hair around lids and muzzle, strict growth exclusions
on bare anatomy, and a separate tapered whisker population. Inspect cheek roundness, neck/body
transition, paws, ears, eye clearance and the complete cat with its real materials.

## Required visual review

Render baseline and candidate with identical cameras, lighting and materials. Use front, side,
back, top and a close oblique view of the changed region. For comb, compare fixed root departure,
the passing section and the unaffected control area. For lift, inspect height from the side.
Check bends, scalp penetration, sparse holes, taper and feature anchors; revise and render again.
Then view the dressed character/animal and, if animation is part of the deliverable, play it.
Record actual capture paths and outstanding defects. Parser success, a green test and a diagnostic
count are evidence about implementation; none substitutes for looking at the hair.
