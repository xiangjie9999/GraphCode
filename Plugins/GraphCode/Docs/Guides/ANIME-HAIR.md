# Anime hair from card locks

*A GraphCode guide. Read [ASSET-QUALITY.md](ASSET-QUALITY.md) and [GROOM.md](GROOM.md) first; this page adds what is different for cards. The tool's own help is the grammar: `gc_describe kind:"groom"` and `{ "help": true }` on `gc_apply`. If the help and this page disagree, the help is right.*

Card hair is solid locks of hair, each one a swept ribbon of mesh, written by a `.gcgroom` recipe as an ordinary mesh. Use it for anime or toon hair. For realistic hair use strands (the rest of [GROOM.md](GROOM.md)); its advice about `settle`, `lift` and stiffness does not carry over to cards.

Five styles were built from this guide by authors who had never seen the tool: a bob with a cut fringe (Bob), a hime cut (Hime), short spiky hair (Spiky), a side-swept cut with an ahoge (SideSwept) and a centre-parted cut with flicked tips (CentrePart). Recipes and materials: `Content/__MCPTests/GroomCardsStyles/<Style>/` (round 1) and `.../GroomCardsStyles2/<Style>/` (round 2). All five read as the style; none was clean. This page names each style where a number comes from.

## You want -> write

Each row names the word. Ask the tool for its spelling (`gc_describe kind:"groom"`, blocks `entries`, `layer`, `region`, `build`).

| You want | Write | Note |
|---|---|---|
| a lock | a `card()` entry (width, thickness, `section`, `stiffness`) and `card=$X` on a `layer(...)`; `depth=` on the layer | `depth=` is not optional: see the first fault below |
| a row of locks | `count=N` on a band (one path with `widen`); `shift=` slides the row by a share of one spacing, `jitter=` scatters it | `shift=50%` puts one row over the gaps of the row below |
| locks placed one by one | `lock(at=, width=, thickness=, length=, turn=, twist=)` lines in the layer, one per card; `at=` is the share along the band, `at=0%` the first point of `points(...)` | `turn=` is degrees about the scalp normal; positive is anticlockwise seen from outside the head |
| volume, a tuck, or a tip flick | `bend(angle=, axis=across, profile=)`; `axis=out` sweeps sideways | across: negative tucks in, positive lifts. A profile that is 0% until 65% of the length bends the tip only. The profile is the cumulative turn: see "Long hair" |
| a blunt fringe | per-lock `length=` read off the `tips` lines and a width profile that does not reach 0% | "A blunt cut fringe". `cut` on a card is unproven: no recipe uses it |
| a lock that stands (spike, ahoge) | `depth=` 0 to 1, stiffness 4 to 5, length about 1.5 x stiffness, `flow(..., lift=60%)` and a 40 degree up-bend on a late profile | "Locks that stand". The 90-degree start holds for the first cm only. The report's `standing` and `tip_up` are right for these |
| roots on a parting line, a cap patch at the hairline | bands rooted 0.25 cm across the centre line; a short soft layer (stiffness 2, depth 2, length 7) on the hairline | "Recipes" |
| an outward tip flick | a tip-only positive bend, with any mid-length tuck smaller than the flick | "An outward tip flick" |
| the scalp never showing | `cap=$Region` inside `cards(...)`, plus back rows outside the head shape | read `scalp_cards`; what the cap looks like: "Reading the build report" |
| an outline (a STYLE CHOICE, not part of every anime look) | `outline=$M` inside `cards(...)`, `$M` a `material(...)` entry that is an instance of `M_GcToonOutline`; leave `outline=` out for no outline | needs closed sections (section "Rules with numbers"). Match the reference: cel-shaded game hair (two flat tones, Genshin- or Guilty Gear-like) has a thin line; a soft "3D anime render" (smooth shading, a glossy sheen, colour gradients, as in many character renders) has NONE, and an outline there makes it read as a cartoon (an author added one to a soft-render reference on 12 Nov 2026; the owner asked why it was there) |
| the toon look and the colour | `material=` on each layer naming an instance of `M_GcToonHair`; set `Tint` on the instance | section "The look" |
| layers in order | written order is stacking order: nape first, crown last; `stack=` orders cards inside one layer; `cards(T, rise=)` sets the step | `coplanar`, in "Reading the build report" |
| a whorl | a ring band round the whorl (`points(...)` ending near its start, `loop=false`, `count=8` to 14, `shift=50%`), `flow(outward, from=$Ring, weight=, lift=)`, plus the cap | `flow(outward)` radiates from the region's own centre and takes no `to=`. `lock(at=)` is refused on a `circle`; a circle takes `count=1` only. Spiky built three rings this way |
| hair on a MetaHuman | `mesh:` the MetaHuman face mesh; the build writes a skeletal card mesh | section "Wearing it" |

## Why card hair comes out bad, and what fixes it

Every row happened to the authors or was measured on 450 cards (one head, one lock per case; see "A table you can read off"). Most costly first. S is `stiffness`, D is `depth=` in cm, L is the lock's length in cm.

| What you see | Cause | Fix |
|---|---|---|
| Locks stand off the head like hoops | A lock leaves the skin at 90 x (1 - (D + 1) / S) degrees over its first cm (0 if negative), on top, side and back alike. It stands (over 45) when D is under S/2 - 1. Stiffness alone cannot fix it | `depth=` of at least S/2 - 1; D = S leaves it tangent; depth past S - 1 changes nothing. The `standing` line names it |
| Soft locks slid off the crown and hang straight down | `settle` on a card layer: S 2 at 25 cm on top is 1.2 cm off without it and 5.2 cm with it | No `settle` on card layers unless a render shows it helps |
| A tip will not lie down | The tip lies only when L is at least 3 x S (a fit to 15 cases). A 6 cm lock with S 4 or more stands 1.9 to 5.1 cm off at any depth | Soften: short locks need a soft card of their own (S 2) |
| Long locks hang 3 to 5 cm off the skull | A 25 cm lock flowing down hangs 3.5 to 3.8 cm (side) and 4.2 to 5.0 cm (back) off at S 2 or 4, any D: it follows its flow, not the head | Tuck with `bend(axis=across)`, negative, tip only (side -30: 1.7 cm; back -30: 3.0 cm) |
| `flow(lift=)` does nothing, or stands a lock | Up to 10% it moves a tip 0.0 to 0.1 cm; 30% moves 0 to 2.2 cm; 60% moves 1.9 to 11.8 cm and stands it | Use `bend(axis=across)`. On a 25 cm top lock: -30 gives 0.3 cm off, 0 gives 1.0, +30 gives 6.2. A whole-lock -30 hooks the lock back at the head and lengthens it (25 to 36.4 cm); a tip-only bend does not. Hime's crown locks, given a whole-lock -30, curled forward under the chin |
| The top of the head is bare, bangs are thin | Bands rooted at the hairline leave the crown bare | Root the bangs near the crown or parting and run them forward over the top, to about the eyelids |
| An even picket fence of locks | `uniform`: the widest lock is under 1.4 times the narrowest | Widest at least 1.4 times the narrowest. Place the face-framing locks one by one with `lock(...)`. On a row of 3 or 4 cards `random()` cannot reach it: see the report table |
| All locks fall one way | `parallel`: every tip within 10 degrees of the others | `turn=` a few locks; a second flow. `flow(jitter=)` works on `flow(toward)` only and `noise()` is not built; two authors tried both, and `parallel` stayed on six layers |
| A blocky, faceted crown | Short whorl locks. Six long locks with `flow(outward)` lie close (0.6 cm) but as six spokes with bare skin between and a crumpled centre | Root many broad, long top locks in a small looped band round the whorl, like an umbrella, and use the cap |
| Pleated flicker where layers overlap (a ladder look) | Layers written in the wrong order, or neighbours lying in one plane | Write nape first, crown last; read `coplanar` (see "Reading the build report") |
| The helmet look | The hair hugs the skull: read `outside_head` | Arch with a small positive `bend(axis=across)`, add rows. The built recipes arch +10 to +14 on the test head and 12 to 16 at most on the MetaHuman, where 20 made side locks wing out about 4 cm. On 40 cm locks use a rise-and-fall profile |
| A highlight like a headband, a ladder of bands or a patch per lock | The default zigzag, drawn per lock | Soft by default now; set `HighlightV` for the style and the zigzag parameters (see "The look") |
| Bare scalp that you cannot see | Grey test views (`lighting_only`) draw head and hair alike; `lit` is nearly black | The toon look and the cap, or plain `lit` (see "Seeing it"). Five views every round |
| A fringe on the side of the head | The wrong facing. The two heads used so far face +X (the test head, a choice: it is front-back symmetric and has no face) and +Y (the MetaHuman face mesh). The render presets' names do not say: on the MetaHuman `front` shows the profile and azimuth 90 shows the face | Measure the head with `gc_render mode:"ask"` and state which way it faces before writing a band. Points snap to the nearest surface point |
| `count=` or `lock` is refused | They need one path with `widen`. A widened arc, a circle, or a composed region (`intersect`, `add`) is refused | Write the band as `points(...)` and `widen(...)` |

## The method

The first author needed 8 rounds, the second 5 on a MetaHuman head; ten style authors needed 3 to 6 (round 1: 3, 4, 4, 4, 6; round 2: Bob 3, Hime 4, Spiky 5, SideSwept 6, CentrePart 3). Expect to iterate; change one or two things a round.

1. Describe the style in words: parting or whorl, bangs, side locks, back length. Say which checklist rows it waives (a hime cut: blunt even tips).
2. Measure the head: render the head alone, then `gc_render mode:"ask"` on that render's `capture` (hairline loop, crown, ears, eyes, nape; see "Seeing it"). Write down which way it faces and that in the front view +Y is on the viewer's left. The test head's numbers are in "The test head".
3. **Root every lock at the middle of the scalp: the crown and the parting.** Plan 5 to 9 masses (the back, each side, the face-framing locks, the bangs, a flicked layer) and root each one at the crown or along the parting, never on a band low on the sides or the back: from there each lock lies over the skull and the head's shape shows through, as in a drawn or modelled anime head. Locks rooted on bands round the head leave it straight and stack outward, and the style reads as a cage standing off the head (measured on a long cut: a median 3.5 cm off the skull rooted on 17 bands, 1.4 cm rooted at the crown and parting; "Rooted at the middle of the scalp" below). Choose stiffness and depth from the table below, and the recipe for the style from "Recipes".
4. Write the masses nearest the face as `lock(...)` lines, deliberately uneven: one thick lock beside two or three thin ones, uneven gaps, one or two turned the other way. Write the back as counted rows with `shift=50%` between rows. **One instruction per line**, in every `card()`, `region()` and `layer()` body: `width(4)  thickness(0.3)` on one line, or two `lock(...)` side by side, is refused ("an instruction ends at ')'; put the next instruction on its own line"). Every block on this page is written that way.
5. Order layers so the one that should lie on top is written later: the back first, then the sides, the face-framing locks, the bangs last. Put the cap in the first build so scalp cannot mislead you.
6. Dry run: `gc_apply` with `asset_path:"/Game/<path>/<Recipe>"` (no extension, the `.gcgroom` saved beside it in `Content/`) and `dry_run:true`; use `create:true` the first time. It is the grammar check and it prints the whole report (3 to 4 s, as long as a build): read it ("Reading the build report").
7. Build once. Only then make the viewing Blueprint: it cannot be created before the card mesh exists ("no StaticMesh exists at '.../SM_<Name>Cards'"). Render the five views. Set `length=` per lock from the `tips` lines (root and tip heights are how you read a hem). Change one or two things. Repeat.
8. The look last: tint, light direction, highlight height.

## Working in the editor

- Submit every `gc_apply` build with `wait:0`, keep the task id, and read that task's result with `unreal_info include:["tasks"]` and that `task_id`. The task list is shared by every author on the editor and carries no asset path; without your id you open someone else's result. `wait:0` may answer "STILL RUNNING ... Do NOT resubmit" even for a 150 to 270 ms material create; the task is complete on the next read. `gc_render` takes no `wait` (and no `view`): call it directly; renders took 0.4 to 0.6 s. Builds took 3 to 4 s alone and waited 5 s to 3 minutes in the queue.
- A build or dry-run answer is 20 to 25 thousand characters (a `tips`, `lifted` and `clamped` line per card; `unreal_info` `lines:` does not apply to tasks) and has no totals-only form: read the layer lines and the totals first, and the `tips` lines only for the layer you are fixing.
- A submit that answers "fetch failed ... completion_unknown" has no task id and does not say if it ran. Check before you resubmit: `gc_read` the asset or look at the mesh's saved time. Once the build had run and once it had not. A rebuild in place is safe to repeat; for a Blueprint or material create, look first. "An operation is servicing cancellation/status only. Retry other tools after it finishes." comes after a build or while another author's job runs; it cleared in 10 to 20 seconds in four logs (once about a minute). Retry after that.
- A card-only recipe writes the card mesh named in `cards(...)` (`SM_<Name>Cards`) and no Groom asset: the build says "no Groom was written; GR_<Name> is left as it was". Keep the `asset:` path in the header and do not look for it.

## A table you can read off

Measured on the test head with diamond locks, width 3 cm, no `settle`, `lift=0%`; top locks flow forward over the crown, side and back locks straight down. Tip height off the skin in cm. Other heads were not measured, and **nothing was measured above 25 cm**: for 40 to 50 cm locks the only numbers are Hime's build (see "Long hair").

| Lock | Use | Result | Outside it |
|---|---|---|---|
| 6 cm | S 2, D 2 | 0.4 cm off at every place | S 4 or more: 1.9 to 5.1 cm off at 13 to 48 degrees, at any depth |
| 12 cm | S 2, D 2, or S 4, D 3 | 0.4 to 0.6 cm at every place. S 6, D 3 also lies on side and back (0.6 to 0.8) | S 8 or 10: 3 to 8 cm off, a visor on top. S 6 on top: 2.1 cm off |
| 25 cm, top | S 2 to 6, D 2 to 3 | 0.8 to 1.2 cm | S 10: 12.2 cm off |
| 25 cm, side and back | any S 2 to 6 | hangs 3.5 to 5.1 cm: tuck with a tip-only negative `bend(axis=across)` | S 10 hangs 4.9 to 9.2 cm |

The rule behind it: S at most L/3, and D at least S/2 - 1. Width does not change how a lock lies (36 cards at 1.5 and 5 cm matched 3 cm to 0.01 cm): choose width for coverage. Layers do not interact, and a build repeats exactly.

## The test head

`SM_GroomHeadHuman` is an egg with no face and no features: "the front is +X" is a convention, and the mesh is symmetric front to back. Authors measured it with `gc_render mode:"ask"`: semi-axes X 11.5 (Hime read 11.4), Y 7.5, Z 14.4; centre z 15.5; extent z 1.05 to 29.9; crown about (0.3, 0, 29.95); ears at Y +-7.4; neck bulge below z 5. Hime wrote "semi-axis Z 15, centre 15.47" in its recipe comments and the example at the end of this page says 15 and 15.45; Bob measured 14.4 and 15.5 from the extents, SideSwept "about 14.5" and 15.45. Use 14.4 and 15.5. The mesh has no eyes or chin: authors took eyes at z 18 and a chin at z 7. A perspective render's silhouette pixels answer about 2 cm inside the extents: take extents from entry and exit points or an orthographic camera. In the front view (camera at +X looking toward -X) **+Y is on the viewer's left** and -Y on the right (the top view has the front at the bottom of the image): an author who took +Y for the right sent a sweep the wrong way for two rounds.

## Recipes

Spellings and numbers are from the recipes that worked (`Content/__MCPTests/GroomCardsStyles/<Style>/<Style>.gcgroom` and `.../GroomCardsStyles2/<Style>/`). Each is one place to start, not the only way. The style recipes there were rooted on bands round the head, before the rule in step 3: take their numbers, not their roots.

**Rooted at the middle of the scalp (measured 12 Nov 2026 on a long layered cut, `Content/__MCPTests/GroomCardsPinkV3/PinkV3.gcgroom`, sheet `Saved/GroomCards/PinkV2/pinkv2-sheet.png`).** Every lock is rooted at the crown or along the centre parting and leaves lying down (`depth=3`), stiffness 3. The same cut rooted on 17 bands round the head stood a median 3.5 cm off the skull (`outside_head`) and read as a cage; rooted this way it lies 1.4 cm off, `solidity_top` 0.84, and the skull and face show through. The masses, in written order (the later lies on top):

- the back: an arc of roots behind the whorl, the locks fanned out from the arc's own middle with `flow(outward, from=$CrownArc)`;
- each side: one row of roots along the parting, from the front hairline to the crown, `flow(direction=(-0.15,1,-0.35))` on the +Y side and `(-0.15,-1,-0.35)` on the other (sideways and a little back);
- a cheek-length layer from the same rows, flicked out at the ends: length 22 to 25 cm and `bend(angle=38, axis=across, profile=$Sweep)` with `Sweep = profile(0=0%,0.62=0%,0.92=100%,1=100%)`, so it flares at the jaw (with the flick starting at 45 % of an 18 cm lock it flared at eye level, like wings);
- two face-framing locks each side from the front of the parting, `flow(direction=(0.5,0.6,-1))` and its mirror;
- the bangs last: roots on an arc just behind the front hairline, laid forward, 10 to 12.5 cm so they end at the brows (12 to 16 cm covered the face).

Vary the widths with `width(random(min=, max=), profile=)` in each `card()`. A lock's end hangs straight down once it leaves the head at this stiffness (`plumb` about 0.46). For more body see the next recipe, but add it a little at a time and aim the launch down: rooted on bands, `stiffness(12, rod)` with face-framing locks launched sideways (y 0.5, lift 16 %) stood out like an umbrella. Rooted at the crown and parting, `stiffness(6, rod)` on the long back, side and framing locks kept the top on the head (1.41 to 1.49 cm) and gave the ends body (`plumb` 0.46 to 0.03), but on a head with no body the ends spread into an A-line and the back fanned 34 degrees backward like a broom (`PinkV3-r4.gcgroom` beside the recipe): judge the amount on a head with shoulders.

**SUPERSEDED 12 Nov 2026 20:35 - DO NOT FOLLOW AS A RECIPE: it lifts the hair off the head.** `rise=` is added once per LAYER written later, so 14 layers at `rise=0.5` stacked the last ones 4 to 7.25 cm off the skull (the bangs 7.25 cm; the owner: "the hair isnt on the head anymore"). Every row as its own layer multiplies it. What still holds: roots at least a lock's width apart in a row gave 0 crossings, and narrow diamond locks read as locks; how anime hair is actually built around the head is being studied (Docs/GC-GROOM-CARD-PLAN.md, state 20:35). The text as written: **Solid locks that never pass through each other (measured 12 Nov 2026, `Content/__MCPTests/GroomCardsPinkR/PinkR.gcgroom`; Docs/Research/anime-lock-shape-and-intersections.md).** Locks 2 to 2.6 cm wide (the reference's long locks measure 1.4 to 2 cm on a 15 cm head), `section(diamond)` at `thickness(30%)`, a width and thickness profile narrow at the root, widest a third of the way down and sharp at the tip (`profile(0=65%,0.35=100%,0.75=85%,0.93=35%,1=0%)`). Lay each mass as two or three rows from the same roots (the crown arc, each side of the parting), each row's `count=` small enough that its roots are at least a lock's width apart, the later rows `shift=33%` and `shift=67%`, and `cards(..., rise=0.5)`: crossings went from 73 to 0 and stayed 0 with a third row. Thick locks rooted 1 to 2 cm apart are what cross (93 crossings), not thickness itself. With only two rows the head shows between the locks (scalp cover 72 %); three rows gave 81 %.

**Long locks with body, not limp (measured 12 Nov 2026 on a long layered cut, `Content/__MCPTests/GroomCardsSpring/`, sheet `Saved/GroomCards/Spring/sheet-S0_S2_S5_S6.png`).** A lock follows its flow for about one stiffness length and then hangs toward straight down, so a 30 to 40 cm lock at stiffness 4 hangs plumb from just below the head: the limp look. Raising `stiffness` alone (8, 12, 16) only flares the hair into a stiff bell that still drops plumb. What gave body: stiffness 12 on the long cards with `depth=` about half of it (6), and an outward bend carried over the WHOLE length, so the arc does not stop where the flow does:

```gcgroom
Arc = profile(0=0%,1=100%) [linear]
SheetCard = card() {
  ...
  stiffness(12)
}
# on each long back and side layer (depth=6), instead of tip-only bends:
  bend(angle=20, axis=across, profile=$Arc)
```

(16 degrees on the face-framing locks.) The locks leave the head, bow outward and hold an arc. A profile that rises, eases back and rises again (`profile(0=0%,0.3=100%,0.65=25%,1=70%)`) reads looser, closer to an S, but as two bends rather than a smooth S. Not yet possible: a flick at the tip of a long lock, and an arc that holds without tuning the bend to each lock's length (both are tool work in progress).

**Pointed bangs that follow the forehead (measured 12 Nov 2026, `Content/__MCPTests/GroomCardsMassCheck/V5/GR_V5.gcgroom`, sheet `Saved/GroomCards/MassCheck/bangs-sheet.png`).** Bangs rooted on a band near the crown (about 4 cm above the hairline) leave along the crown's upward normal and stand off the forehead as separate spikes, with a strip of cap showing between them; changing only the flow's direction did not help. What worked: root the band low, just above the hairline, and add a `comb` over a forehead region with `lift=0%`, then a tip-only inward bend. The bangs become one curtain lying on the forehead with pointed tips ending at the brow, the cap strip is gone, and crossings fell from 8 to 5 on the same style. On the egg test head (front is +X; see "The test head"):

```gcgroom
TipOnly = profile(0=0%,0.7=0%,1=100%) [linear]
BangBandLow = region() {
  points((7.5,-5.0,25.2),(8.6,0,25.0),(7.5,5.0,25.2),loop=false)
  widen(4)
}
Forehead = region() {
  points((8.6,-5,23),(9.6,0,21),(8.6,5,23),loop=false)
  widen(6)
}
Bangs = layer(region=$BangBandLow, card=$BangCard, length=15, seed=209, material=$Hair, depth=3) {
  lock(at=5%, width=1.8, length=12.5, turn=18)
  lock(at=26%, width=1.4, length=16, turn=6)
  lock(at=50%, width=2.4, length=15.5, turn=-1)
  lock(at=73%, width=1.5, length=16, turn=-8)
  lock(at=95%, width=1.8, length=12.5, turn=-18)
  flow(direction=(1,0,-0.45), from=$BangBandLow, weight=100%, lift=12%)
  comb(direction=(1,0,-0.8), in=$Forehead, lift=0%)
  bend(angle=-35, axis=across, profile=$TipOnly)
}
```

(The measured recipe had nine locks, widths 1.4 to 4.8 cm, five shown here.) Mix wide and narrow locks for the pointed look. Watch the layer behind the bangs: once they lie low, a lock that used to hide behind them (a forward flick) can show sticking straight out in the side view; turn or shorten it. Whether the -35 tip bend reads as a curl toward the face was not measured.

**A blunt cut fringe (Bob; Hime).** `cut` on a card is unproven (no recipe uses it, and three authors avoided it); get the straight line from per-lock lengths. Read each lock's tip height from its `tips` line, then change `length=` by the height error and rebuild. Bob's fringe: 11 locks, tips z 20.5 to 21.7, a straight cut just above the eyes. The card is `width(4, profile=$Blunt)`, `thickness(0.3)`, `section(crescent, bow=25%)`, `stiffness(4)`; the band is `points((0.5,-5.5,28),(0.5,5.5,28),loop=false)` then `widen(4)`; it runs from -Y (the viewer's right) to +Y, so `at=4%` is on the viewer's right.

```gcgroom
Blunt = profile(0=100%,0.85=100%,1=92%) [linear]
Fringe = layer(region=$FringeBand, card=$FringeCard, length=15, seed=309, material=$Hair, depth=3) {
  lock(at=4%,  width=3.6, length=9.5,  turn=30)
  lock(at=13%, width=5.0, length=10.5, turn=24)
  lock(at=22%, width=3.8, length=11.5, turn=16)
  lock(at=31%, width=5.4, length=12.8, turn=9)
  lock(at=40%, width=2.8, length=14.4, turn=3)
  lock(at=50%, width=5.2, length=15.6)
  lock(at=59%, width=3.4, length=14.6, turn=-3)
  lock(at=68%, width=5.4, length=12.8, turn=-9)
  lock(at=77%, width=3.0, length=11.5, turn=-16)
  lock(at=87%, width=5.0, length=10.5, turn=-24)
  lock(at=96%, width=3.6, length=9.5,  turn=-30)
  flow(direction=(1,0,-0.2), from=$FringeBand, weight=100%, lift=8%)
}
```

Hime's fringe is the same idea with 13 locks, widths 1.5 to 3.8 cm, lengths 10.8 to 16.1 and a width profile that stays at 96% to 100%: all tips within 0.45 cm of z 19.8 after two corrections from the `tips` lines (a first build had tips z 12.9 to 18.1). Forehead showed between Hime's locks; a second layer of seven wide (4.0 to 5.8 cm) locks under it, lengths 11.2 to 15, closed it. Bob's first TopBase (long locks behind the fringe) stood forward above it as a second plank; five short soft locks (length 7.5 to 8, stiffness 2, depth 2) fixed it.

The ends: Bob (width 100% to 92%, thickness 0.3 flat) and Hime (both flat) both read as planks, and a blunt fringe cannot meet row 12's "taper". Tapering the thickness to 0 at the tip while the widths vary more is the fix to try; **no recipe has used it, so it is not measured**. Vary widths as both did (1.5 to 5.4 cm): row 4 of "Judge it" passed.

**Fan the fringe, aim a lock (Bob; SideSwept).** A fringe band with `flow(direction=(1,0,-0.2))`: the outer locks follow the head's side surface and end at the temples (Bob round 1, tips z 15.9 at |y| 7.7). Bob fixed it with the `turn=` values above (positive at the -Y end, negative at the +Y end, about 30 at the outermost) and lengths of 9.5 at the ends against 15.6 in the middle. Hime copied the sign pattern and it held.

On the flat crown `turn=` did not steer: SideSwept's Wing turns of -6, 4, 8 then -20, 6, 26 changed the tips' spread from 4.9 to 5.3 degrees; why Bob's `turn=` steered and these did not is not measured. A mostly sideways `flow(direction=(0.2,1,-0.5))` on locks rooted at x 1.3 to 4 ran straight down the face (tip (13.3, 1.7, 17.1)), while a lock rooted at x -1 with the same flow went 11 cm sideways. To aim a lock at a place use `flow(toward, from=$ForeBand, to=$ForeTarget, mapping=midpoint, weight=100%, lift=12%)` and a small projected circle (`ForeTarget = region()` with `circle(radius=0.8)` and `project(at=(9.0,0,25.5))` on their own lines; `scalp_cards` 98.4% to 99.4%); `midpoint` moves one aim vector (band midpoint to target) to every lock, so the locks run parallel and do not converge (a lock rooted at y -1.87 ended at y -4.1 with the target at y +6.6).

**An even hem (Hime; Bob).** `length=` is the length along the path, not the hang. Hime's crown row, eight locks at one `length=` of 44.5, ended at z -7.3 in the middle (the path crosses the flat top) and -15.3 at the ends. Set a length per lock from the `tips` line: Hime's crown 44.2, 46.5, 48.5, 51, 51, 48.5, 46.5, 44.2 (longest in the middle). Bob's upper back row of 11: a counted row had its centre tips at z 13 to 14 against 8 to 10 at the ends; lock lines of 21.0, 22.0, 23.5, 24.9, 26.0, 26.2, 25.2, 23.4, 22, 21.6 and 20.3 cm (25 to 27 cm at the centre, 21 at the ends) gave one hem at z 8.6 to 10.6. A row of 19 cm from the crown reached only z 17. The same row splayed at the centre of the back because the flow follows the narrowing head; `turn=` +8 to +14 on one half and -8 to -14 on the other fixed it.

**Roots on a parting line (CentrePart).** Two bands whose roots lie exactly on the centre line (y +-0.05) left a strip of cap: each layer rises 0.15 cm and the cap shows between them. Cross the line by 0.25 cm: the layer that flows toward +Y is rooted at y -0.25, the one that flows toward -Y at y +0.25 (no cap at the parting in the top view). The +Y half's band:

```gcgroom
RTopBand = region() {
  points((4.0,-0.25,28.8),(-5.0,-0.25,29.3),loop=false)
  widen(3)
}
```

Its flow is `flow(direction=(0,1,-0.5), from=$RTopBand, weight=100%, lift=4%)`; the mirror band uses y +0.25 and `(0,-1,-0.5)`. Seven locks of widths 2.6 to 5.2 and lengths 19 to 22, `turn=` of 2 to 4 alternating.

**A cap patch at the hairline (SideSwept).** A smooth flat patch with a straight lower edge on the forehead is the cap: no card reaches it (`scalp_cap_only` 2.6% to 3.4%, and it looked like a slab). Four wide locks under the fringe did not reach it; a short soft layer rooted ON the hairline closed it (`scalp_cards` 98.3% at the end, one small hole left at the part). Card: width 3.6, thickness 0.3, both with `LockTaper`, crescent, stiffness 2; band `points((7.0,-4.2,27.0),(7.0,-0.4,27.3),loop=false)`, `widen(3)`. Write it before the fringe layers:

```gcgroom
ForeFill = layer(region=$ForeBand, card=$ForeCard, length=7, seed=316, material=$Hair, depth=2) {
  lock(at=10%, width=4.0, length=7.5)
  lock(at=40%, width=3.0, length=6.5)
  lock(at=68%, width=4.4, length=7)
  lock(at=94%, width=3.2, length=6.5)
  flow(direction=(1,-0.1,-1), from=$ForeBand, weight=100%, lift=3%)
}
```

**Locks that stand: spikes and an ahoge (Spiky; SideSwept).** The fault rows treat standing as a defect; for spikes use them as the rule, with one limit. A lock stands over 45 degrees when D is under S/2 - 1; the first cm leaves at 90 x (1 - (D + 1) / S): S 5 D 0 is 72, S 5 D 1 is 54, S 4 D 0 is 67.5, and Spiky measured 54 to 72. **That angle holds for the first centimetre only**: after it the lock follows its flow. Spiky's first crown, `lift=0%` and a 15 degree tip bend, built flat umbrella spikes (a root at z 28.75, its tip at z 28.66 pointing z -0.30) although `standing` and `tip_up` both fired. What stood them (tips z 31 to 34): `lift=60%` on the flow and a 40 degree up-bend on a late profile, stiffness 5, depth 0, lengths 6.6 to 9.0 cm. The ring `RingB` is nine points round the crown at z 28.9 (`points((5,0,28.9),(3.5,2.7,28.9),(0,3.8,28.9),(-3.5,2.7,28.9),(-5,0,28.9),(-3.5,-2.7,28.9),(0,-3.8,28.9),(3.5,-2.7,28.9),(5,-0.3,28.9),loop=false)`, `widen(3)`); the recipe has ten locks, three shown:

```gcgroom
SpikeTaper = profile(0=100%,0.45=80%,1=0%) [linear]
UpFlick = profile(0=0%,0.35=15%,1=100%) [linear]
SpikeCard = card() {
  width(3.0, profile=$SpikeTaper)
  thickness(0.4, profile=$SpikeTaper)
  section(diamond)
  stiffness(5)
}
Crown = layer(region=$RingB, card=$SpikeCard, length=7.5, seed=306, material=$Hair, depth=0) {
  lock(at=3%, width=3.5, length=7.0)
  lock(at=43%, width=3.8, length=7.2)
  lock(at=73%, width=4.9, length=8.6)
  flow(outward, from=$RingB, weight=100%, lift=60%)
  bend(angle=40, axis=across, profile=$UpFlick)
}
```

Spiky's inner rings use the same two lines: a ring of 8 at z 29.8 (lift 60%, bend 35), a ring of 1.2 (5 locks, 3.4 to 4.4 cm, lift 80%, bend 30), the upper back (lift 60%, bend 30); the lower back rows lift 0% to 45% with no bend. The report then read `standing=88`, `tip_up=88` (96 cards) and `scalp_cards=72.9%`: by design, see the report table. It reads as spikes from every side; the back reads as six rows of similar wedges (row 6 failed), a pinecone.

An ahoge is one thin stiff lock: on a flat crown `flow(direction=(0,0,1))` has no tangent to project, so SideSwept aimed it forward, lifted 70%, and curled it. `Point = profile(0=100%,0.5=95%,0.85=45%,1=0%) [linear]`; the band is `points((-0.6,0.2,29.8),(-0.6,1.4,29.8),loop=false)` then `widen(1.2)`:

```gcgroom
AhogeCurl = profile(0=0%,0.35=0%,1=100%) [linear]
AhogeCard = card() {
  width(0.7, profile=$Point)
  thickness(0.3, profile=$Point)
  section(diamond)
  stiffness(12)
}
Ahoge = layer(region=$AhogeBand, card=$AhogeCard, length=9, seed=313, material=$Hair, depth=0) {
  lock(at=50%, width=0.7, length=9)
  flow(direction=(1,0.2,0), from=$AhogeBand, weight=100%, lift=70%)
  bend(angle=-100, axis=across, profile=$AhogeCurl)
}
```

The bend sign went against the help's reading ("positive lifts away from the head"): with the front at +X, **-100 curled the tip forward (toward +X) and +50 curled it backward**. SideSwept's round-1 note says the +50 ahoge was "a straight spike, no curl" and its fault list says it curled backward; the two notes differ. Stiffness 12 and length 9 are one recipe's numbers; the Spiky numbers above are another's. `standing=1` for the ahoge is expected.

**Side rows that lie (Spiky).** A ring rooted at z 25 with stiff spikes (S 5 and 4, depth 0 to 0.5) left as horizontal wings in the front view. The row became a soft card (stiffness 3), depth 1, length 5.5 to 7.5, and the wings were gone: first-cm angle by the formula is 30 degrees (S 3, D 1) against 72 (S 5, D 0). Bob's and SideSwept's side rows are stiffness 4, depth 3 with a tip tuck of -28 to -40.

**An outward tip flick (CentrePart).** A flick is a tip-only positive bend. Alone it worked weakly: flicks of 22 to 32 degrees (profile 0% until 70%) and of 16 to 18 looked alike in the top view, which was a spiky star, and the hair hung 3 to 4 cm off the head. Adding a mid-length tuck to lie the hair down **cancelled the flick**: tuck -18 then flick +40 left no tip pointing outward (`tip_up=0`). Bends add their turns along the lock, and the tuck's profile holds at 100% to the tip, so the net turn there is the sum. CentrePart's final, on every layer (arch +6, tuck -8, flick +50, S 4 or 5, depth 2 to 4) is `Arch = profile(0=0%,0.4=100%,1=100%)`, `TuckMid = profile(0=0%,0.3=0%,0.65=100%,1=100%)`, `Flick2 = profile(0=0%,0.68=0%,1=100%)` and, in each layer, `bend(angle=6, axis=across, profile=$Arch)`, `bend(angle=-8, axis=across, profile=$TuckMid)`, `bend(angle=50, axis=across, profile=$Flick2)`. Lengths: lower back 10 to 12.5, mid 13 to 16, upper 15 to 18, sides 15, top locks 18. Tips hooked outward in the front and back views. `tip_up=51` of 59 cards, all of them the flicked tips: intended. The report's own advice for `tip_up` (a negative bend) is the opposite of this style. From the top the flicked tips make a spiky star rim.

**Long hair (Hime).** A bend's `profile` is the **cumulative turn**, not the bend's shape: `Arch` (0% to 100% by 40%, held) leans the whole lock, and on a 40 cm lock that flared the hair to nearly twice the head's width. A profile that rises and falls bends only its start. Written as `Bump = profile(0=0%,0.12=100%,0.3=100%,0.5=0%)`, `TuckIn = profile(0=0%,0.65=0%,1=100%)`, `bend(angle=8, axis=across, profile=$Bump)`, `bend(angle=-8, axis=across, profile=$TuckIn)`. With `Bump` (arch 7 to 10) and a tip tuck of -8 to -10, the back became one straight curtain with no flare; the hair stood about 3 cm wider than the head each side (top view). Lengths for tips near z -15 (45 cm under the crown), bands at z 10, 18, 26: nape row 25, mid row 33.5, upper row 42.2, crown 44.2 to 51. A second Hime with bands at z 22, 26.5 and 28 used 37.5, 42.7 and 44.3 to 51.2; side locks 17.9 to 19.8 for a chin end at z 7. Stiffness 4, depth 3. No lock longer than 25 cm was measured in the lock study; these are build results, not predictions.

**No working recipe yet: sideburns, and long hair over the crown.** Sideburns (Spiky): locks of 9 to 10.4 cm with `flow(direction=(0.3,0.5,-1))` built `parallel` (tips spread 0.0) and hung as vertical planks, worse than the shorter ones; three locks of 5.6 to 7 cm per side, stiffness 4, depth 0.5, `flow(direction=(0.7,0.9,-0.5))` and `lift=30%` still sit detached on the temple, roots on bare skin at the hairline. Long hair over the crown (Hime): crown cards fall radially, tips at y +-8.8 against a head 7.5 wide, whatever the flow: `(-1,0,-1.2)` and `(-1,0,-0.3)` gave the same tip y; the top view is a faceted flower of wide cards sliding off the dome.

## Rules with numbers

- **Counts.** Back: three or four stacked rows, the outer shape outside the head (a drawing rule, from VRoid). 25 to 130 cards a head (VRoid Hub models); 5 to 9 masses of 3 to 5 locks (a drawing rule). About 4k triangles is lean and 50k is the top; the build warns above 50k. The built recipes: 69 cards and 11.2k triangles; 70 cards and 11.2k plus a 2.3k cap. The five styles: 53 to 80 cards, 7.5k to 12.2k triangles (the cap is 1,138 of them).
- **Widths.** Mix: a few large locks, several thin; no source gives sizes relative to a head. The built recipes use bangs 1.5 to 4.4 cm and back sheets 2.2 to 5.7 cm on a head 15 cm wide (authored starting values, not sourced).
- **Tips and bangs.** Tips pointed (width profile reaching 0%), blunt only for a cut fringe; the built locks taper to 0% or 10%. Bangs running forward to about the eyelids, tapered, overlapping (a drawing rule, from VRoid); root them just above the hairline and comb them along the forehead (Recipes, "Pointed bangs that follow the forehead"): rooted near the crown they stand off the forehead as a fountain (measured 12 Nov 2026). CentrePart's bangs ended at the cheek (z 15) not the eyelids (z 18); Spiky's stopped at z 22 to 25, above the brows.
- **Sections and detail.** Small `angle=` on the front (5 on the fringe), large on the back (12), as VRoid spends it. Closed (`diamond` by default, `triangle`, `crescent`, `round`, `ridge`) for anything outlined; `flat` only for cheap hidden back rows. Diamond and crescent look almost the same under the toon shading.
- **Round or flat, and anything between.** A lock is a ring of points at each station along its curve, joined by quads, with the station's width across and its thickness through (`width(...)` and `thickness(...)`, each with an optional `profile=` along the lock, so both vary from root to tip). How round it reads is `section(...)` and the thickness against the width: `section(round)` is an ellipse (8 sides near, 6 and 4 at lower detail levels): with `thickness` equal to the width it is a round tube (a spike, a braid strand, an ahoge), at a third of the width a flattened oval, at a tenth a ribbon with rounded edges. `section(crescent, bow=)` cups the lock across its width (bow is the edge drop in widths); `diamond` and `ridge` put a crease down the middle; `flat` is a single-sided strip. **The simple way to choose roundness is a percentage: `thickness(<n>%)` is that share of the lock's width at every point along it** (its width times the width profile there), so `thickness(100%)` stays a round tube and `thickness(10%)` a ribbon however the width tapers, and anything between (`thickness(35%)`, an oval) keeps its shape to the tip; `random(min=20%, max=40%)` draws it per lock, and a `profile=` on it still multiplies (a lock that flattens toward its tip). The share is above 0 % and at most 200 %. `thickness(<cm>)` is the other form, centimetres through the lock whatever its width: on a tapering lock give it the same `profile=` as `width` if the shape should stay as round toward the tip (otherwise the tip ends thicker than it is wide). A `lock(..., thickness=)` line is always centimetres, for that one lock. Rounder locks read as more 3D and cost more triangles; most anime locks are flat-ish ovals or cupped (VRoid's are diamond or triangle sections).

## Reading the build report

A build or dry run answers SHORT, totals first (since GraphCode 0.1.8; about 3 to 4 thousand characters, where it used to be 20 to 35 thousand): two `totals` lines (size and cover, then every count), one line per layer, then one line per kind of problem naming at most 5 cards, worst first, with "and N more" and the fix. The last line, `report:`, names a text file under `Saved/GraphCode/GroomCardReports/` holding the WHOLE report: every card's `tips` line, every crossing, clamped and lifted card, the dry run's plan lines and each total in full. Read the answer first; open the file for a card the answer does not name. Each build overwrites its mesh's file.

| Line | Means | Change |
|---|---|---|
| `layer X: N cards, widths .. cm (median ..), lengths .., tips spread D degrees` | The layer's size, and its flags: `uniform`, `parallel`, `clamped`, `lifted`, `stacked`, `crossings`, `tip_up`, `standing` | Fix by the flag, named below |
| `uniform (the widest .. is 1.31 times the narrowest ..)` | Widest under 1.4 times narrowest. On a row of 2 or 3 cards (SideSwept 1.12 and 1.15) or of 6 to 10 with `random()` over a 1.5 range (Spiky 1.36 to 1.38; Hime's 4.2 to 6.0 gave 1.25 to 1.30) the flag fires although `random()` is the cure the guide gave | Widths from `lock(...)` lines (Bob's Upper row, 3.4 to 5.2 cm, passed), or a wide `random()` range (Bob's Mid row, 2.8 to 5.4 on 9 cards, passed). Ranges of 1.4 to 1.5 on 6 to 10 cards still fired. On a row of 3 or fewer, leave it |
| `parallel (every tip within 10 degrees ...)` | One direction | `turn=` a lock, a second flow. Curtains that fall on purpose (bangs, side locks) trip it (tips within 7.7 to 9.4 degrees) |
| `crossings=` and `touching=` | Pairs of cards through each other by more than 0.2 cm, and by less; a `crossing A card i through B card j` line follows each | For a crossing arch the upper one higher with `bend(axis=across)`, `turn=` it, or reorder with `stack=`. The five styles built 22, 28, 2, 24 and 19; the 2 was 80 short stiff cards; Hime's were 0.2 to 0.33 cm deep |
| `coplanar=` (cm2) | Card surface within 0.03 cm of another card's or the cap's: where it flickers. ShortCut 514, MetaHuman copy 286; the five styles 91 to 890 | `cards(T, rise=<cm>)` wider (SideSwept used `rise=0.25`), or vary `depth=` between neighbouring layers; it falls, it does not reach 0 |
| `scalp_cards=`, `scalp_cap_only=` | Share of the cap region with a card over it, and with only the cap. Without a cap the line reads `scalp_uncovered`; it also names the largest bare patch and the layers nearest it | Widen cards, add a row. See below |
| `outside_head` | Median and least gap between the hair's outer surface and the skin (1.97 cm and 2.55 cm built; the five styles 1.39 to 2.37 median). A tight gap is the helmet look | Arch with `bend`; add rows |
| `tip_up=` | Tips more than 1.5 cm off the skin pointing away from it, with each card's height and angle. Falling with gravity does not count as pointing away: a tip hanging down in air is counted as `hanging=` in the totals instead | Softer, more `depth=`, or a tip-only negative `bend(axis=across)`, **unless the locks flick or stand on purpose** (below) |
| `standing=` | Cards leaving the skin over 45 degrees in the first cm | `depth=` about half the stiffness, **unless the locks stand on purpose** |
| `clamped=` | Cards narrowed where a bend is tighter than half their width | Loosen the bend or narrow the card there |
| `lifted=`, `buried=` | Cards with stations moved out of the head; cards still inside after it | Arching with `bend(axis=across)` cut one author's lifted stations from 90 to 30; `buried` should be 0 |
| `long=` | Cards that leave the target's neighbourhood: they will meet a body the target lacks | Shorten, or give a whole-body target |
| `tips <card>, lock line N` (in the report file) | Root, tip, pointing, length and the nearest other tip | Edit that `lock` line; the tip's z is how you check a hem |

- **`standing` and `tip_up` are right for spikes, an ahoge and flicked tips.** No build stood locks by accident: they were non-zero only where the style stands or flicks (Spiky 36 and 44, SideSwept `standing` 1, the ahoge, and `tip_up` 1, a fringe lock; CentrePart 0 and 51). Read them as a count of intended locks and look at the picture for the rest. A style cannot mark a layer as meant to stand; the flags stay on.
- **Hair past a headless head.** Hair that hangs below the test head's mesh reads `long=` (Hime, round 2: `long=49`, `tip_up=0`). Hair that ends above the head's lowest point but hangs in the air beside it reads `hanging=` since GraphCode 0.1.8: SideSwept's shoulder-length back and sides gave `tip_up=28` before and give `tip_up=0 hanging=28` now (tips 4.7 to 6.6 cm off the skin, pointing 0.92 to 0.99 straight down); CentrePart's 65 outward flicks still read `tip_up`. `hanging` is information, not a defect. A card pointing back at the face (a hooked tip) shows only in `tips spread` and the tip direction.
- **What a good `scalp_cards` is.** The only sourced figure is the example's 99.6% with a cap; the five styles read 93.5%, 93.8%, 67.2%, 99.4% and 95.5% in round 1 and 99.5%, 100%, 72.9%, 98.3% and 98.0% in round 2; no threshold was measured. A spiky style cannot reach the others' figures: stiff standing spikes leave the skin under them bare, so the cap shows (Spiky 72.9% with 96 cards). It is an area share over the whole `Scalp` region, steep sides included, so a patch on the forehead is small in the number and large in the picture: SideSwept at `scalp_cap_only=1.1%` showed a pale slab on the forehead in the front view, and 0.6% at the end. Look at the forehead, the nape and behind the ears in the renders as well.
- **What the cap looks like.** The cap is the under-surface the hair is built over: a smooth, flat patch with a straight or curved edge and **no outline**. In round 2 it showed in the hair's own colour, flat tan under gold hair (Spiky, `scalp_cap_only` 27.1%) and flat pink under pink (SideSwept, 1.7% to 3.4%), no longer as a pale band. A cap you can see is a place a card should cover ("A cap patch at the hairline" in "Recipes").

## The look

Two materials ship with the plugin, `M_GcToonHair` and `M_GcToonOutline`, and the build makes them in `/Game/GraphCode/Groom/` the first time a recipe names them. Never edit them. Make an instance of each; the recipe's sidecars (`MI_ShortCutHair.gc`, `MI_ShortCutOutline.gc` beside `ShortCut.gcgroom`) are the pattern. It refuses inline text, and it refuses a file path as `asset_path` (`C:/.../MI_X.gc` and `Content/.../MI_X.gc` both answer "GraphCode cannot place a sidecar ... package is outside the roots GraphCode projects (/Game and /GraphCode/)"). What worked for all five: write the `.gc` at its sidecar place, `Content/<path>/<Name>.gc` (the asset's path with `Content/` for `/Game/`), then `gc_apply asset_path:"/Game/<path>/<Name>" create:true` (the package path, no extension). The same holds for the Blueprint and for the recipe (`/Game/<path>/<Recipe>`). To change a parameter afterwards, two ways: edit the sidecar and `gc_apply asset_path:"/Game/<path>/<Name>"` without `create` (Hime and CentrePart: applied cleanly; Bob found the sidecar stale, because the tool rewrites it with new `sig`, `bin` and `seal` after a create, so read it again first), or `gc_apply asset_path:"/Game/<path>/<Name>" block:"parameters" base_sig:<the sig the last answer gave> ir:"parameters { ... }"` (Bob). A bare header, without `sig`, `bin` and `seal`, is accepted and the tool rewrites the file. **Put one empty line between the `parent:` line and `parameters {`**: without it the tool answers "Unrecognized GraphCode header line 'parameters {' ... missing the BLANK LINE" (found checking this guide). A `dry_run:true` with `create:true` on the package path answers "can be created as a material_instance and 1 block(s) would be written":

The Hime instance, as authored: `gc 1  kind:material_instance  asset:<path>`, `parent: /Game/GraphCode/Groom/M_GcToonHair`, and `parameters { LightDirection: linear_color = (R=0.55,G=-0.35,B=0.75,A=0.0)  Tint: linear_color = (R=0.06,G=0.03,B=0.1,A=1.0)  UseFixedLight: float = 1.0 }` on separate lines; the tool rewrites the file with its own header words and blocks. Parameters on `M_GcToonHair`, with defaults from its recipe:

| Parameter | Default | What it does |
|---|---|---|
| `Tint` | (0.85, 0.8, 0.75) | The lit tone. A **linear** colour: it reads brighter than the number (Hime's (0.06, 0.03, 0.10) rendered as a clear mid purple, not violet-black) |
| `ShadeTint` | (0.6, 0.55, 0.75) | A **multiplier** on the lit tone, not a colour: the shade is `Tint x ShadeTint`. A dark value (0.01, 0.012, 0.045) blackened the whole shade side. Used: (0.5, 0.5, 0.75) with a dark navy Tint, (0.16, 0.30, 0.38) with a pale teal one |
| `UseFixedLight`, `LightDirection` | 0; (-0.4, -0.6, 0.7) | 0 follows the scene's sun. Set 1 with (0.55, -0.35, 0.75) from the front and above on the +X-facing head, (0.4, 0.55, 0.75) on the +Y-facing one. Bob used (0.35, -0.35, 0.85) and found the shade side dark |
| `Threshold`, `ShadowBias`, `EdgeSoftness` | 0.15, 0.5, 0.02 | Where the two tones split and how sharp the edge is. Also: `CardVariation` 0.16, `LockJitter` 0.12 (per-lock brightness and split), `EdgeDarken` 0.18 (width 0.3), `RootDarken` 0.15 (span 0.25), `Shade2Offset` 0.5, `Shade2Strength` 0.25, `Shade2Soft` 0.05 (a deeper shade step), `ProxyBlend` 0.7, `UseProxyNormal` 1, `Paint` (white texture, multiplied by `Tint`); `Debug` 0 (1 colours by layer, 2 by card, 3 the highlight height, 4 the per-lock random, 5 and 6 UV0 U and V) |
| `HighlightV`, `HighlightWidth`, `HighlightStrength`, `HighlightSoft` | 0.3, 0.05, 0.3, 1.0 | Height of the highlight (relative to the hair's extent: a deep fringe needs its own; `Debug` 3 draws it as lines), its width, brightness and softness |
| `HighlightZig`, `HighlightZigAmp`, `HighlightJitter` | 1.0, 0.15, 0.6 | The zigzag along each lock and the per-lock offset: at these values one soft, broken ring across the locks; the old 1.2 and 2.5 made a patch per lock. `HighlightWobble` 0.6, `HighlightLitRange` 0.4, `ViewShift` 0.12 also act on it |

- **Colour.** `Tint` on the hair instance; `OutlineColor` on the outline instance. Values the five authors used: navy (0.03, 0.04, 0.10) with `ShadeTint` (0.5, 0.5, 0.75); violet (0.06, 0.03, 0.10); gold (0.75, 0.5, 0.12); pink (0.80, 0.30, 0.40); silver-teal (0.45, 0.70, 0.72) with `ShadeTint` (0.16, 0.30, 0.38). A tint dark enough to be black in the lit tone makes the overlap order unreadable (Hime row 3), and with the fixed light in front the back of a dark hair is nearly black (Bob's (0.03, 0.04, 0.10) read mid navy in front): rows 3 and 10 cannot be judged from the back at that tint.
- **Highlight.** One soft mark per lock at its own height, on the lit tone. Round 1's Bob passed row 9 with `HighlightV` 0.45, `HighlightStrength` 0.2, `HighlightZigAmp` 0.3, `HighlightJitter` 4.0, and those numbers did not transfer: they depend on the layers' heights. On Bob's round-2 recipe they gave one stripe across the fringe, `HighlightV` 0.6 with Jitter 8 gave blobs per lock and Jitter 6 sat between; Hime at `HighlightV` 0.45 to 0.72 had blobs low on the side hair; CentrePart with Bob's values had a faint soft band, one across both curtains. The defaults before GraphCode 0.1.7 gave a ladder of bands in the top view, blobs per lock or a hard band on the cap. Raising `HighlightJitter` never fixed row 9; lowering it did, and the values two bullets down are the defaults since 0.1.7. The MetaHuman instance sets `HighlightV` 0.7.
- **Outline.** `M_GcToonOutline` defaults: `OutlineWidth` 0.5, `DepthPush` 4.0, `DistanceScale` 0.01, `TipFloor` 1.0, `FadeSpan` 0.15. `DepthPush` was 2.5 until the evening of 12 Nov 2026: at 2.5 a layered cut drew dark A-shaped triangles where crossing locks overlap the bangs and a line between most bang locks; at 4 those go, a few lines stay where a lock clearly overlaps another, the silhouette is unchanged, and bang tips lying at the brow keep their line against the skin (two A/Bs, `Saved/GroomCards/OutlineAB/` and `OutlineAB2/`). At 6 and above every inner line goes and the hair reads flat again. Set 2.5 on the outline instance for more inner lines. `FadeSpan` is the share of each lock's length, at its end, over which the outline fades to nothing. Without it (0, the look before GraphCode 0.1.8) a blunt fringe reads as stacked planks: a dark frame round every square cut end and a black slit in every gap between locks, which is the outline's hull showing through (with the outline off the slits vanish; measured 12 Nov 2026, `Saved/GroomCards/PlankAB/`). At 0.15 pointed tips and spikes keep the line up both sides and lose only the closing stroke at the very point; set 0 on the outline instance if a style needs closed tips. Where locks lie at one depth, two neighbours can merge and the outline doubles; vary depth. At the defaults before 0.1.7 (`OutlineWidth` 0.3, `DepthPush` 0.6, `TipFloor` 0.4) all five failed row 10 (lines between most locks, doubled where layers stack); the next bullet's values fixed it. `Debug` on the hair material colours by layer or card to find the pair.
- **The defaults since GraphCode 0.1.7, and why (measured on two built styles, one light and one dark). On an older build, set them on your two instances.** A project's copy of either material made by an older build is laid down again from the new recipe the next time a build uses it. Outline instance: `DepthPush` 2.5, `OutlineWidth` 0.5, `TipFloor` 1.0. Each lock's hull is pushed 2.5 cm away from the camera, so it hides behind the neighbouring lock and only the rim and the edges of big masses keep a line; at the old 0.6 the layers' own steps (up to 2.6 cm on a fringe) are larger than the push and every lock shows a line. The cost: tips lying within about 2 cm of the face lose their line (1.2 keeps more of them and leaves some lines between back layers; 5 leaves the rim only). Hair instance: `HighlightJitter` 0.6, `HighlightZigAmp` 0.15, `HighlightWobble` 0.6, `HighlightSoft` 1.0: one soft, broken ring across the locks, where the old 2.5 and 1.2 gave a patch per lock. Then choose `HighlightV` for the mass the ring sits on (the share down the hair's front extent, from the top): 0.3 put it on a bob's fringe (with `HighlightWidth` 0.07, `HighlightStrength` 0.45), 0.45 on a centre-parted cut (width 0.05, strength 0.3); 0.6 on the bob fell to cheek level and showed only side patches. No one height fits every style.
- **Strips or structure: the light decides first (measured 12 Nov 2026 on the long cut rooted at the middle of the scalp, close-up `Saved/GroomCards/PinkV2/lit2-closeup.png`).** `M_GcToonHair` is unlit: no lock receives a shadow or ambient occlusion, so neighbouring locks melt into one pale sheet and read as strips. The same geometry on a plain lit material that takes the painted tile (any material with a `Paint` texture parameter is accepted: `Content/__MCPTests/GroomCardsPinkLit/M_PinkLitHair.gc`, default lit, roughness 0.45) shows each lock lit on one side and dark on the other, and dark where it tucks under the next. Second, the lock's own section: ribbons 0.3 cm thick on 3 to 6 cm stay flat even lit; narrower locks (2 to 3.6 cm) at `thickness(38%)` with `section(diamond)` read as separate solid locks. Do not make wide locks thick: `thickness(45%)` on 4 to 6 cm locks made 2 to 2.7 cm tubes that shoved each other up into columns at the crown (`standing` 65 to 81 degrees, `lifted` 3 cm).
- **Lit locks: `M_GcLitHair` (since the release after 0.1.7).** The soft "3D anime figure" look, with no outline and no painted strand lines: name `/Game/GraphCode/Groom/M_GcLitHair` (or an instance of it) as the layer's `material=` and keep `normals=auto` on `cards(...)` (the material reads the build's smooth head-shaped normal from the mesh's tangent). Set `RootColor` and `TipColor` (the colour runs from the root colour at `GradientStart` 0.1 of each lock's length to the tip colour at `GradientEnd` 0.7; both start as the toon material's neutral tone); `ProxyBlend` 0.6 (how far each lock's shading leans toward the one smooth shape: 0 reads as separate tubes, near 1 as one blob); `EdgeTone` 0.55 (a lock darkens toward both edges, which draws the seams); `CardVariation` 0.2 (each lock a little lighter or darker); `Fill` 0.25 (a soft light in the shadows); `Roughness` 0.35, `Specular` 0.5. Painted tiles still show on it (`Paint`). The pink used on the long cut: `RootColor` (1.0, 0.55, 0.68), `TipColor` (1.0, 0.04, 0.3).
- **Static meshes.** Use `outline=` on `cards(...)`. A component overlay set in a Blueprint is ignored in the static-mesh preview.
- **A soft "3D anime render" look instead of cel (measured 12 Nov 2026 on a long pink layered cut, `Content/__MCPTests/GroomCardsSoftShade/MI_SoftHair.gc`, sheet `Saved/GroomCards/SoftShade/softshade-sheet.png`).** No `outline=`; colour from a painted tile with `Tint` white (a pale root to a saturated tip); on the hair instance: `EdgeSoftness` 0.5, `Threshold` 0.25, `ShadowBias` 0.3, `ShadeTint` (0.88, 0.64, 0.80) (a pale warm shade, not a dark one), `Shade2Strength` 0.1 with `Shade2Offset` 0.9 and `Shade2Soft` 0.4, `LockJitter` 0.05, `EdgeDarken` 0.1, `CardVariation` 0.08, `RootDarken` 0, `HighlightStrength` 0.45, `HighlightV` 0.34, `HighlightWidth` 0.14, `HighlightSoft` 1.0, `HighlightZigAmp` 0.03, `HighlightJitter` 0.15, `HighlightWobble` 0.3, `HighlightLitRange` 1.2, `ProxyBlend` 0.15, `UseFixedLight` 1. That removes the hard two-tone split and gives a soft glow on the crown and fringe. The band alone cannot give a glossy streak running ALONG each lock (pushing `HighlightStrength` toward 0.9 washes pale hair to white). **Hair sheen (in Groom builds published after 12 Nov 2026 16:00; off by default, so nothing changes until you set it):** `SheenStrength` 0.4, `SheenExponent` 220 (60 hides the small facets tight bends show at high values), `SheenShift` -0.4 (moves the gloss up onto the crown and bangs), `SheenStreak` 0.6 (breaks it into streaks along the lock), `SheenTint` (1, 0.82, 0.90), and a second, hair-coloured lobe down the long locks: `Sheen2Strength` 0.3, `Sheen2Shift` 0.1. It moves with the camera, unlike the band. `Debug` 7 shows the strand direction and 8 the first lobe alone. Measured on the pink soft build (`Content/__MCPTests/GroomCardsSheen/`, `Saved/GroomCards/Sheen/sheen-sheet.png`, row E). Wrap lighting and a rim were tried and paled the whole mass, so they are not offered. Fine white strand lines still come from painted tiles, not from the sheen.

## Seeing it

Build the recipe once, make the viewing Blueprint (below), then render the five views in one `gc_render` call. `gc_render` takes no `wait` and no `view`; `help:"camera"` names the views (`front`, `rear`, `left`, `right`, `top`, `isometric`) and the orbit form, and `views:[{name, camera:{azimuth, elevation, zoom}}]` works (the angles go inside `camera`; an `azimuth` beside `name` is refused), with `output:{lifetime, path}`. This call (Hime's, from its manifest) wrote five pictures and a `manifest.json`:

```json
{
  "asset": "/Game/__MCPTests/GroomCardsStyles2/Hime/BP_HimeView",
  "view_mode": "lit",
  "views": [
    { "name": "front", "width": 1024, "height": 1024, "camera": { "azimuth": 0, "elevation": 5, "zoom": 0 } },
    { "name": "threeq", "width": 1024, "height": 1024, "camera": { "azimuth": 40, "elevation": 12, "zoom": 0 } },
    { "name": "side", "width": 1024, "height": 1024, "camera": { "azimuth": 90, "elevation": 3, "zoom": 0 } },
    { "name": "back", "width": 1024, "height": 1024, "camera": { "azimuth": 180, "elevation": 5, "zoom": 0 } },
    { "name": "top", "width": 1024, "height": 1024, "camera": { "azimuth": 0, "elevation": 89, "zoom": 0 } }
  ],
  "output": { "lifetime": "persistent", "path": "Saved/GroomCards/Styles2/Hime/final" }
}
```

| View | Azimuth / elevation | Notes |
|---|---|---|
| front | 0 / 5 | the camera is at +X; +Y is on the viewer's left. Spiky used elevation 10 |
| three-quarter | 40 / 12 | Spiky used 45 / 15 |
| side | 90 / 3 | the camera is at +Y. SideSwept used 0, Spiky 5 |
| back | 180 / 5 | SideSwept used 8, Spiky 10 |
| top | 0 / 89 | screen-up is -X: the front is at the bottom of the image |

- **Zoom and framing.** `zoom` 0 is the fit; `zoom:1` is not "1x" and crops hard (warning `subject_cropped`). Hime's call frames automatically; Bob added `anchor:{component:"Cards"}` inside `camera` to frame the hair, and that worked too. A point anchor rendered black. Saving: A call without `output` is transient; the PNGs are in `Saved/GraphCode/renders/.transient/<capture>/`. A persistent `output` once answered "fetch failed" (Bob, round 1) and the same call without `output` worked.
- **Light and brightness.** Plain `lit` is enough once `UseFixedLight` is 1. `gc_render` has no exposure parameter. Brightness comes from `Tint`: SideSwept's pink (0.80, 0.30, 0.40) rendered plain `lit` read as a pale pink.
- **`mode:"ask"`** needs a `capture` from an earlier render (the `capture` field of its answer); give no `asset`. It is refused on a Blueprint capture (`subject_unsupported ... requires a static_mesh, skeletal_mesh or metahuman capture`): to ask about the hair, render `SM_<Name>Cards` alone and ask that capture. A perspective capture's silhouette pixels answer about 2 cm inside the extents.

The viewing Blueprint (the Hime author's `BP_HimeView`; created on the first try). `gc_read` of the shipped `BP_ShortCutView` fails on its saved sidecar ("Unrecognized GraphCode header field 'seal'"); `source:"live"` reads it. Create it the way the material instances are made ("The look"; a `dry_run` of it before the first build is refused with "no StaticMesh exists at '.../SM_<Name>Cards'", so build first): write the text at `Content/__MCPTests/GroomCardsStyles2/Hime/BP_HimeView.gc`, then `gc_apply asset_path:"/Game/__MCPTests/GroomCardsStyles2/Hime/BP_HimeView"` with `create:true`. It is the saved Blueprint shortened to its header, parent and components.

```gc
gc 1  kind:blueprint  asset:/Game/__MCPTests/GroomCardsStyles2/Hime/BP_HimeView
parent: /Script/Engine.Actor

components {
  Root = SceneComponent()
  {
    Head = StaticMeshComponent(StaticMesh=obj'/Game/__MCPTests/GroomVisual/SM_GroomHeadHuman.SM_GroomHeadHuman')
    Cards = StaticMeshComponent(StaticMesh=obj'/Game/__MCPTests/GroomCardsStyles2/Hime/SM_HimeCards.SM_HimeCards')
  }
}
```

## Judge it

Judge five views: front, three-quarter, side, back, top. Answer yes or no. Rows 9, 10 and 11 (highlight, outline between locks, crossings) failed on every style in both rounds, and no recipe setting fixed them: they come from the tool's look and stacking, which is being worked on. Judge them, but do not spend rounds trying to clear them. A blunt fringe fails the "taper" in row 12 by design; waive it. A style may waive a row on purpose; say so up front (a hime cut waives tip variety, the mass count and direction variety). A green build is not approval; the picture is. Since then rows 9 and 10 have a fix in settings: the outline and highlight values in "The look". Row 11 has none yet: a larger `rise=` on `cards(...)` cuts crossings (0.25 to 0.5 took one bob from 17 to 8) but lifts the fringe and thickens the silhouette, and past 0.5 it opens the scalp.

1. Does the silhouette sit outside the head shape, not a helmet?
2. Is there no skin between locks at the parting, hairline, crown or nape?
3. Is the back more than one layer, with a visible overlap order?
4. Are there at least three clearly different widths near the face?
5. Are tips pointed (or deliberately blunt) and not all one shape?
6. Do 5 to 9 masses read, not dozens of equal wedges?
7. Do most locks share a flow, with a few outliers?
8. Do locks radiate from one whorl or parting line (top view)?
9. Is the highlight a soft broken ring, not one hard stripe and not a patch per lock?
10. Are shadow edges clean with no speckle, and (if the style has an outline at all: see "an outline" in the first table) the outline unbroken at the rim and not between every lock?
11. Does no lock pass through another or through the head, and does every root end under another lock or in the scalp?
12. Do the bangs start at the crown, taper, overlap and reach about the eyelids?
13. Is the triangle total inside the plan?

## Wearing it

On a static head the card mesh is built in the head's own space: place it at the same transform. To look at it, use the viewing Blueprint above.

A skeletal character or a MetaHuman gets a skeletal card mesh on the target's skeleton, held by its own Skeletal Mesh Component that follows the body's pose (`SetLeaderPoseComponent` in the construction script). It is not the MetaHuman Hair slot. The component property is `SkinnedAsset=`. Examples: `/Game/__MCPTests/GroomCards/Probe/BP_CardProbe_OnHuman`, `BP_CardProbe_OnMetaHuman` (body, leader pose), and `/Game/__MCPTests/GroomCardsMetaHuman/BP_MediumView` (the face mesh with its cards as a child component). When you render a Blueprint, frame it with `anchor {component:"Cards"}`.

**Painted tiles.** `paint=<size>` on `cards(...)` writes tile templates and a `PROMPT.md` beside the recipe. An image model paints the tiles in greys, and the material's `Tint` colours them; rebuild to wear them. None of the five styles was painted.

Not built or not proven: swinging hair (bone chains); collision with a body the target does not include (`long=` reports it); tails that grow from a tie; hair following a moving MetaHuman head (not yet tested); feathers; `cut` on a card; any style on a MetaHuman or seen moving (the five were shape-only on the test head).

## A complete example

A short anime cut on the test head `SM_GroomHeadHuman`, with the toon look, an outline and a cap. Its coordinates are that head's (face toward +X, ears at Y = +-7.4): measure your own.

```gcgroom
gcgroom 1 mesh:/Game/__MCPTests/GroomVisual/SM_GroomHeadHuman asset:/Game/__MCPTests/GroomCards/GR_ShortCut gen:1 tolerance:0.02

# Head frame (measured, mesh-local): an ellipsoid, semi-axes X 11.5, Y 7.5, Z 15, centre z 15.45. Face toward +X (a choice: the mesh is
# symmetric front to back), ears at Y = +-7.4, eyes about z 18, crown about (0.5, 0, 29.9). Points snap to the nearest surface point.
# CF4 copy of GroomCardsAuthoring/ShortCut.gcgroom: the toon look, its outline and a cap over the scalp; the hairstyle is unchanged.
Hair = material('/Game/__MCPTests/GroomCards/MI_ShortCutHair')
Line = material('/Game/__MCPTests/GroomCards/MI_ShortCutOutline')
LockTaper = profile(0=100%,0.6=90%,0.9=40%,1=0%) [linear]
FlatTaper = profile(0=100%,0.8=80%,1=10%) [linear]
TipFlick = profile(0=0%,0.7=20%,1=100%) [linear]
Arch = profile(0=0%,0.4=100%,1=100%) [linear]
TuckIn = profile(0=0%,0.65=0%,1=100%) [linear]

BangLock = card() {
  width(1.6, profile=$LockTaper)
  thickness(0.35, profile=$LockTaper)
  section(diamond)
  twist(random(min=-12, max=12), profile=$TipFlick)
  skew(random(min=40%, max=60%))
  stiffness(6)
}
SideLockCard = card() {
  width(2.2, profile=$LockTaper)
  thickness(0.4, profile=$LockTaper)
  section(triangle)
  stiffness(6)
}
TopBaseCard = card() {
  width(4.4, profile=$FlatTaper)
  thickness(0.3)
  section(crescent, bow=30%)
  stiffness(6)
}
WhorlCard = card() {
  width(3.2, profile=$LockTaper)
  thickness(0.3)
  section(crescent, bow=30%)
  stiffness(2)
}
UpperSheet = card() {
  width(random(min=3.4, max=5.4), profile=$FlatTaper)
  thickness(0.3)
  section(crescent, bow=25%)
  stiffness(6)
}
MidSheet = card() {
  width(random(min=2.8, max=4.8), profile=$FlatTaper)
  thickness(0.3)
  section(crescent, bow=25%)
  stiffness(6)
}
SideSheet = card() {
  width(random(min=2.6, max=4.2), profile=$FlatTaper)
  thickness(0.3)
  section(crescent, bow=25%)
  stiffness(6)
}
BackSheet = card() {
  width(random(min=2.2, max=3.8), profile=$FlatTaper)
  thickness(0.3)
  section(crescent, bow=25%)
  stiffness(6)
}

# Bands: one path each, locks stand along its centre line.
BangBand = region() {
  points((0.5,-5.5,28),(0.5,5.5,28),loop=false)
  widen(4)
}
RTempleBand = region() {
  points((6.5,3.0,26),(4.5,5.5,24),loop=false)
  widen(3)
}
LTempleBand = region() {
  points((6.5,-3.0,26),(4.5,-5.5,24),loop=false)
  widen(3)
}
RSideBand = region() {
  points((4,4.2,26.6),(-6,3.2,26.5),loop=false)
  widen(4)
}
LSideBand = region() {
  points((4,-4.2,26.6),(-6,-3.2,26.5),loop=false)
  widen(4)
}
UpperBackBand = region() {
  points((-1,-4.4,28),(-1,4.4,28),loop=false)
  widen(4)
}
CrownRing = region() {
  points((2.8,0,29.6),(1.5,2.2,29.6),(-0.9,2.2,29.7),(-2.2,0,29.8),(-0.9,-2.2,29.7),(1.5,-2.2,29.6),(2.8,-0.1,29.6),loop=false)
  widen(3)
}
CrownMid = region() {
  circle(radius=0.8)
  project(at=(0.3,0,29.9))
}
TopBand = region() {
  points((-0.5,-4.8,27.6),(-0.5,4.8,27.6),loop=false)
  widen(4)
}
MidBackBand = region() {
  points((-4.0,-4.5,26.5),(-7.0,-2.6,26.5),(-8.0,0,26.5),(-7.0,2.6,26.5),(-4.0,4.5,26.5),loop=false)
  widen(4)
}
LowerBackBand = region() {
  points((-5.2,-5.8,22),(-9.0,-3.4,22),(-10.3,0,22),(-9.0,3.4,22),(-5.2,5.8,22),loop=false)
  widen(4)
}

# The scalp as a closed loop on the hairline: front z 24 (+X), over the ears (Y +-7) about z 20, nape z 9.5 (-X); crown midpoint.
Scalp = region() {
  points((9.4,0,24),(7.5,4.2,23.5),(3.5,6.7,22),(-1.5,7.0,20),(-5,6.5,16),(-8.5,4.5,11),(-10.5,0,9.5),(-8.5,-4.5,11),(-5,-6.5,16),(-1.5,-7.0,20),(3.5,-6.7,22),(7.5,-4.2,23.5),loop=true)
  midpoint((0,0,30.4))
}

LowerBack = layer(region=$LowerBackBand, card=$BackSheet, length=16.5, seed=104, material=$Hair, depth=3) {
  lock(at=4%, width=3.1, length=15, turn=6)
  lock(at=15%, width=4.9, length=17.5, turn=-4)
  lock(at=26%, width=2.6, length=16, turn=3)
  lock(at=36%, width=4.2, length=19, turn=-8)
  lock(at=49%, width=5.7, length=14.5, turn=5)
  lock(at=60%, width=2.9, length=18, turn=-6)
  lock(at=72%, width=4.7, length=16, turn=10)
  lock(at=84%, width=3.4, length=17.5, turn=-5)
  lock(at=95%, width=5.1, length=15.5, turn=4)
  flow(direction=(-0.3,0,-1), from=$LowerBackBand, weight=100%, lift=0%)
  bend(angle=-30, axis=across, profile=$TuckIn)
}
MidBack = layer(region=$MidBackBand, card=$MidSheet, count=9, jitter=12%, length=random(min=15.5, max=18.5), seed=112, material=$Hair, depth=3) {
  flow(direction=(-0.4,0,-1), from=$MidBackBand, weight=100%, lift=4%)
  bend(angle=10, axis=across, profile=$Arch)
  bend(angle=-18, axis=across, profile=$TuckIn)
}
UpperBack = layer(region=$UpperBackBand, card=$UpperSheet, count=11, shift=50%, jitter=10%, length=random(min=19, max=22), seed=105, material=$Hair, depth=3) {
  flow(direction=(-1,0,-1.2), from=$UpperBackBand, weight=100%, lift=8%)
  bend(angle=14, axis=across, profile=$Arch)
  bend(angle=-25, axis=across, profile=$TuckIn)
}
RSide = layer(region=$RSideBand, card=$SideSheet, length=14, seed=106, material=$Hair, depth=3) {
  lock(at=8%, width=2.4, length=13)
  lock(at=28%, width=5.0, length=14.5, turn=6)
  lock(at=45%, width=3.0, length=11.5, turn=-8)
  lock(at=64%, width=4.4, length=15.5, turn=2)
  lock(at=86%, width=2.2, length=13.5, turn=9)
  flow(direction=(-0.1,1,-1.2), from=$RSideBand, weight=100%, lift=6%)
  bend(angle=10, axis=across, profile=$Arch)
  bend(angle=-20, axis=across, profile=$TuckIn)
}
LSide = layer(region=$LSideBand, card=$SideSheet, length=14, seed=107, material=$Hair, depth=3) {
  lock(at=8%, width=2.6, length=13.5, turn=-3)
  lock(at=28%, width=4.6, length=14, turn=-6)
  lock(at=45%, width=2.2, length=12, turn=8)
  lock(at=64%, width=5.0, length=15, turn=-2)
  lock(at=86%, width=3.4, length=13, turn=-9)
  flow(direction=(-0.1,-1,-1.2), from=$LSideBand, weight=100%, lift=6%)
  bend(angle=10, axis=across, profile=$Arch)
  bend(angle=-20, axis=across, profile=$TuckIn)
}
RTemple = layer(region=$RTempleBand, card=$SideLockCard, length=16, seed=108, material=$Hair, depth=3) {
  lock(at=12%, width=3.0, length=15.5, turn=4)
  lock(at=50%, width=1.4, length=17, turn=-2)
  lock(at=88%, width=2.2, length=14, turn=-8)
  flow(direction=(-0.1,0.3,-1), from=$RTempleBand, weight=100%, lift=8%)
  bend(angle=18, axis=out, profile=$TipFlick)
}
LTemple = layer(region=$LTempleBand, card=$SideLockCard, length=16, seed=109, material=$Hair, depth=3) {
  lock(at=12%, width=1.5, length=15, turn=-4)
  lock(at=50%, width=3.2, length=16.5, turn=2)
  lock(at=88%, width=2.6, length=14.5, turn=8)
  flow(direction=(-0.1,-0.3,-1), from=$LTempleBand, weight=100%, lift=8%)
  bend(angle=-14, axis=out, profile=$TipFlick)
}
# Base under the bangs: broad cupped cards from just behind the whorl, so the gaps between the bang locks show hair, not scalp.
TopBase = layer(region=$TopBand, card=$TopBaseCard, length=15, seed=110, material=$Hair, depth=3) {
  lock(at=6%, width=4.2, length=14, turn=20)
  lock(at=18%, width=4.8, length=15, turn=12)
  lock(at=33%, width=3.8, length=16, turn=4)
  lock(at=50%, width=5.0, length=14.5)
  lock(at=67%, width=4.0, length=15.5, turn=-4)
  lock(at=82%, width=4.6, length=14, turn=-12)
  lock(at=94%, width=4.2, length=15, turn=-20)
  flow(direction=(1,0,-0.3), from=$TopBand, weight=100%, lift=9%)
  bend(angle=10, axis=across, profile=$Arch)
  bend(angle=-22, axis=across, profile=$TuckIn)
}
# The whorl: short cupped locks rooted on a ring round the crown, aimed at its centre and swirled one way, covering the crown.
Whorl = layer(region=$CrownRing, card=$WhorlCard, length=6, seed=111, material=$Hair, depth=1) {
  lock(at=5%, width=3.4, length=6.5, turn=38)
  lock(at=21%, width=3.0, length=5.5, turn=34)
  lock(at=38%, width=3.6, length=6.5, turn=40)
  lock(at=55%, width=2.8, length=6, turn=36)
  lock(at=71%, width=3.4, length=5.5, turn=38)
  lock(at=88%, width=3.0, length=6.5, turn=42)
  flow(toward, from=$CrownRing, to=$CrownMid, mapping=midpoint, weight=100%, lift=12%)
}
# Bangs written last: they lie on top. Eleven locks of uneven width, gaps and length, the outer ones fanned inward over the forehead.
Bangs = layer(region=$BangBand, card=$BangLock, length=18, seed=101, material=$Hair, depth=3) {
  lock(at=5%, width=1.8, length=16, turn=22)
  lock(at=15%, width=3.4, length=18, turn=16)
  lock(at=22%, width=2.2, length=19, turn=10)
  lock(at=33%, width=4.4, thickness=0.45, length=17.5, turn=5)
  lock(at=40%, width=1.5, length=19.5, turn=2)
  lock(at=51%, width=3.6, length=18, turn=-3, twist=-20)
  lock(at=58%, width=2.4, length=17, turn=-4)
  lock(at=70%, width=4.0, length=16.5, turn=-7)
  lock(at=76%, width=1.9, length=18, turn=-10)
  lock(at=87%, width=3.2, length=17, turn=-15)
  lock(at=95%, width=1.7, length=15.5, turn=-22)
  flow(direction=(1,0,-0.3), from=$BangBand, weight=100%, lift=14%)
}

build {
  LowerBack = $LowerBack [cards=Hair, angle=12]
  MidBack = $MidBack [cards=Hair, angle=12]
  UpperBack = $UpperBack [cards=Hair, angle=12]
  RSide = $RSide [cards=Hair, angle=10]
  LSide = $LSide [cards=Hair, angle=10]
  RTemple = $RTemple [cards=Hair]
  LTemple = $LTemple [cards=Hair]
  TopBase = $TopBase [cards=Hair, angle=8]
  Whorl = $Whorl [cards=Hair, angle=8]
  Bangs = $Bangs [cards=Hair, angle=5]
  SM_ShortCutCards = cards(Hair, angle=8, lods=1, normals=auto, outline=$Line, cap=$Scalp)
}
```

Its report said:

- 69 cards, 11,220 triangles, nine layers plus the cap; `scalp_cards=99.6%`, the largest bare patch 0.1%.
- `standing=0` and `tip_up=0`: no lock hoops or flies off; `buried=0`.
- `outside_head`: median 1.97 cm, never under 0.02 cm: not a helmet.
- `uniform` on UpperBack, TopBase and Whorl (widest 1.29 to 1.32 times narrowest); `parallel` on the side and temple layers (tips within 10 degrees).
- `crossings=36`, `coplanar=514 cm2`: the flicker that stacking by `rise` reduces but does not remove. Its Whorl is short locks on a ring and only covers the crown; for a crown that reads well, root long locks round the whorl (see the faults table).
