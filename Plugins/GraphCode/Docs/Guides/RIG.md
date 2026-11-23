# Rigs (`.gcrig`): a hand you imported, and an animal from a bare mesh

A `.gcrig` makes a skeleton, its skin weights, a physics asset and an IK rig from a mesh it never modifies. This guide
is the order to write one in, with two worked rigs: Epic's VR hand, kept as imported
(`GraphCodeExamples/VRHandRig_20261001/VRHand.gcrig`), and an elephant from a marketplace mesh with no bones at all. Both
ship as text in [Docs/Examples](../Examples/README.md).
The grammar itself is in the tool: `gc_describe kind:"rig"`, then `kind:"rig"` with one block. How the rig is then
used to hold things is [HANDS.md](HANDS.md); motion is [POSES.md](POSES.md).
For procedural behavior authored on top of this setup, see [Control Rig authoring](CONTROL-RIG.md):
`control_rig()` beside a `skeletal_mesh()` build output creates a native starter with calibrated
inputs. Edit its members in `properties {}` and its logic in `graph {}`. Without that output,
applying the source does not create or refresh a starter. The `.gcrig` header defaults to
`override:true`: it replaces only recorded generated functions, preserving unrelated functions
and authored solve wiring. Header `override:false` preserves an existing output; `[!override]`
on an individual `control(...)` preserves that control's generated function.

Two rules run through all of it:

- **Never type a coordinate you did not measure.** Every point comes from `gc_render mode:"ask"` on the mesh itself,
  one pixel named, one mesh-local answer, written down by hand. A ray measures only the axis it travels, so take as
  many views as there are surfaces (the elephant needed side, nose-on, tail-on and overhead). Mark a point you derived
  rather than measured as derived.
- **Every limit is the real animal's.** A joint's range is what that joint does in life, from straight. When a pose
  later cannot reach something, the answer is never a joint bent past life (see "When a limb cannot reach" below).

## Main declarations, each holding one kind of line

A joint can declare its default animation pose with `rest=(fold=20, side=-5)` inside its
modifiers. State either angle or both. Fold counts from straight; side counts from the native
bind pose. Each needs its matching `range=` or `side=` and must be inside it. The default
applies fold first, then side. A `.gcmove` holds it when nothing moves the joint; swings and
reference poses count from it. Native source animation keeps its recorded pose on the bones
it animates. Parent files inherit each rest angle separately, so `rest=(fold=0)` resets fold
while keeping an inherited side. Omitting `rest=` in a child keeps the parent's values; only a joint
with no inherited or local rest override uses its imported default. Fold zero means straight,
not necessarily the native mesh's curl. A bare joint name in a child clears inherited motion
ranges and rest values and makes the joint rigid; use an explicit legal rest override to keep
a finger movable. Do not freeze all fingers merely to replace the template's rest curls. The mesh's
bind pose and limit frames stay unchanged; for a shared imported skeleton, the Move file
reads the character's child `.gcrig` through `controls:`.
The resulting default must also fit the native IK limits. Move refuses a conflicting default
before writing animations, rather than silently clamping it to a different pose.
On imported joints constrained near a native axis, rest folds use the same effective native
axis as constrained motion. Build notes report its difference from the anatomical direction;
the existing ranges and bind frames remain unchanged.

The shared [MetaHuman body template](Examples/MetaHumanBody.gcrig) now supplies a relaxed
standing animation rest: shoulder side 25 degrees from its A-pose, elbow fold 10 degrees,
and different gentle curls for each finger joint. Finger and thumb MCP/IP starting angles
come from [Lee et al. 2008, table 3](https://www.jstage.jst.go.jp/article/jergo/44spl/0/44spl_0_436/_pdf)
(neutral forearm, shoulder flexion zero); shoulder/elbow settings are authored choices
checked on two bodies. Trunk, legs, wrists and thumb base retain the body's native stance.
An empty `Default = clip(frames=24) {}` with `controls:` naming a child of that template
holds these defaults. Rebuild a clip to pick up changed defaults; an existing native source
retains its recorded bone motion.

For a new MetaHuman, follow the [per-character adapter quick start](METAHUMAN-RIG.md): inherit this shared file, name the fitted body mesh and build its IK rig. Joint ranges and defaults are already supplied.

The template explicitly declares all 84 MetaHuman body/placement joints: root/pelvis, limbs, fingers,
palm metacarpals and toes. Ring/pinky metacarpals cup the palm with positive `fold`; index/middle stay
rigid. The ball carries shared toe-off (negative `fold`) and toe curl (positive); proximal toe carriers
stay rigid so MTP motion is not doubled, and distal toes curl with positive `fold`. Face and corrective
helpers remain outside these authored controls. The template comments identify measured ranges and
modelling allowances. **Ball migration, 2026-10-13:** its explicit central toe chain replaces the old
averaged branch direction; remove or recalibrate child ball ranges based on the old +98–100° rest reading.

| Block | Holds | Example |
|---|---|---|
| header | the mesh, where to build, orientation, mirroring, influences and starter regeneration policy | `gcrig 1  mesh:/Game/.../SKM_MannyXR_right  into:/Game/...  mirror:false  override:true` |
| `points { }` | every coordinate, and nowhere else | `index_pad_r = (3.5, 15.4, -4.0)` |
| `chains { }` | who hangs off whom | `index_r = index_01_r -> index_02_r -> index_03_r` |
| `joints { }` | what each bone can do | `index_02_r [range=(0, 110)]` |
| `controls { }` | named calibrated inputs; `[!override]` preserves an existing starter control function | `Fold = control(index_02_r.fold=(0, 90))` |
| `regions { }` | where skin bends and how far it reaches | `stifle_back_L = region($stifle_pt_L, long=110, wide=45%)` |
| `skin { }` | how each region behaves | `stifle_back_L [crease=0.9, folds=1, bulge=0.35]` |
| `sockets { }` | what hangs off a bone | `palm_grip_r = palm_r [on=surface, up=-X]` |

Plus `contacts { }` (the pads a motion touches with), and an optional `build { }` naming the assets to make. A key in
another block's place is refused, naming the block it belongs to.

## Route A: a mesh that already has bones (the VR hand)

1. **Read it back first:** `gc_read kind:"rig" source:"live"` on the skeletal mesh gives a starting declaration with
   its bone hierarchy and orientations.
2. **Keep what the import has.** The VR hand keeps its skin and every bone; the file states only what it adds: limits,
   contact pads, sockets, one extra hinge at the thumb's base. Work on copies of a template's mesh and skeleton, because
   the build writes sockets and joint axes onto the skeleton it is given.
3. **Give each joint its real range, in degrees from straight.** Fingers: knuckle `(-20, 90)`, middle joint `(0, 110)`,
   tip `(0, 80)`; a forefinger's knuckle also swings sideways, `side=(-15, 15)` (a real one about 20° each way). The
   metacarpals are a bare name on a line, which means rigid.
4. **A saddle joint is two hinges.** One bone cannot hold the thumb's base: `thumb_01_r` keeps its in-plane swing and
   `thumb_opp_r`, at the same place, closes across the palm (`closes=down`); `thumb_roll_r [twists=(-10, 45)]` is the
   roll.
5. **Declare what touches: contacts.** A pad is a patch of the finger's own skin near a point:
   ```gcrig
   points {
     index_pad_r = (3.5, 15.4, -4.0)     # 1 cm along the last segment from index_03_r, 0.8 cm toward the palm
     index_tip_r = (3.6, 15.85, -4.6)    # 0.8 cm further along: the fingertip
   }
   contacts {
     IndexPad = contact(chain=index_r, at=$index_pad_r) [radius=1.25, cone=35]
     IndexTip = contact(chain=index_r, at=$index_tip_r) [radius=1.0, cone=35]
   }
   ```
   Declare more than one touching area on a finger that works things. When the pad cannot reach a trigger in a full
   grip, the tip may ([HANDS.md](HANDS.md), "Holding any weapon").

   **A contact may ride an inner bone of its chain:** `Heel_R = contact(chain=leg_R, at=$heel_sole_r, bone=foot_r)`.
   Unset, a contact rides the chain's goal bone (its last joint). Only a rolling plant (`plant(..., roll=)`) reads a
   contact on an inner bone; `reach`, `grab`, `contact()` and a plain plant refuse it by name. Feet for walking and
   running: [WALK-AND-RUN.md](WALK-AND-RUN.md) step 1, including the ankle, hip and trunk ranges a gait needs.
6. **Sockets for what the hand holds.** `palm_grip_r = palm_r [on=surface, up=-X]` (on the nearest skin, its frame
   measured); `web_grip_r = palm_grip_r [at=$web_r]` puts a pistol's grip point in the web of the hand.
7. **Bodies that stop each other.** `PHAT_VRHand = physics_asset() [bodies=hull, gives=0.35]`: each bone's body is its
   own skin, and a held thing may press 0.35 cm into it, as real flesh compresses about 3 mm round what it holds. A
   soft region can give more (`skin { web_fold_r [gives=1.0] }`), and since 2026-10-21 it gives where skin meets skin
   too: a fingertip pressing a thumb's thenar pad is in only as deep as it is past what both fleshes give beyond 0.35.

## Route B: a mesh with no bones (the elephant)

1. **Face it.** `faces:+Y` because the marketplace mesh came in sideways (trunk at +Y). Every `closes=` word is read
   against it, and it derives the mirror plane: write the `_L` side and the `_R` side is implied.
2. **Place every joint by measurement,** in the mesh's own space and units. The elephant is 2.1× life size; the format
   works in what the asset hands you. A spine point is the measured back surface less the depth of muscle over the
   vertebrae, and is marked derived.
3. **Chains: parentage, one line each.** The chain's name is also its IK chain's. `[dynamic=]` marks a chain
   meant to sway on its own; today it is read and inherited but reaches no asset, so the trunk below does not
   swing yet: write it on the joints, where it reaches the physics asset ([How freely a joint's body moves](#how-freely-a-joints-body-moves-dynamic);
   see [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md)):
   ```gcrig
   spine   = root -> pelvis -> spine_01 -> spine_02 -> spine_03 -> chest -> neck_01 -> neck_02 -> skull
   trunk   = skull -> trunk_01 -> trunk_02 -> trunk_03 -> trunk_04 -> trunk_05 -> trunk_06 -> trunk_07 -> trunk_08 -> trunk_09 -> trunk_10  [dynamic=0.3]
   leg_B_L = pelvis -> hip_L -> stifle_L -> hock_L -> foot_B_L
   ```
4. **Joints: most of an animal is rigid.** `pelvis`, `chest`, columnar feet, tusks: a bare name. A bone that bends
   names the region it bends in, and its range: `spine_01 [turns_in=$back_lumbar, range=(-12, 12), side=(-8, 8)]`.
   Several joints may share one region (both neck bones in `neck_band`, both ear bones in `ear_fan_L`). Twist only
   where the animal wrings (`twists=` adds helper bones; a trunk twisting at every joint would spawn thirty).
   **The region a joint turns in sets the axis it folds about** (built 2026-10-19): the fold is square to the bone and to
   the side the region sits on, so where you put the region's point decides how the joint bends, for a joint that is
   only a little bent at rest too. Put the point on the skin where the joint closes, measured as the skin hit straight out
   from the joint: a wrist's on the side its paw points to (the cat's left paw points 44 degrees out, and a point straight
   ahead of the band's line folded its carpus 33 degrees off the paw), a spine's straight above the joint on the back's
   midline (three points 3 cm to one side folded the cat's back 25 degrees off, and every sideways sway tipped the chest).
   **One point says three things: where along `along=` the band's skin blends (it is centred where the point lies along that
   line), its side (the point's offset square to the line), and so the fold.** On a neck that rises, a point straight ahead of the
   joint lies far along the line: the cat's neck and head points sit 8.1 and 4.5 cm along, so its neck band covers the head and its
   folds lie 33 and 76 degrees off a nod (the head "folds" mostly by rolling). Measured square to the line instead (tried 2026-10-20
   and 2026-10-27), the folds nod but the neck's sided band, which reaches to any depth on its side, takes the upper arm's skin
   behind the throat, so the cat keeps its old points until a band's two jobs are separated (GC-MOVE-CAT-PLAN.md, W42). Measure
   both before you settle a neck.
   The build notes each joint that folds this way and how far that is from the plane its bones bend in. A full band
   (`wide=` unset) has no side, so its joint folds in its bones' plane and needs `closes=`. `closes=` still beats the region.
   **So leave `closes=` off a joint whose region you measured** (2026-10-21, the Bigfoot hand): `closes=` is a world
   direction, and on a hand rolled 18 degrees `closes=down` folded every finger 18 degrees off its palm, toward the thumb
   and into its neighbours. **Where the bones' bend cannot say which way the joint closes** (a spread finger's knuckle bends
   sideways at rest, more than 60 degrees from the region's axis) it closes toward the region's side; nearer, the bend decides.
   **A helper from `twists=` rides the bone that ends at its joint** (a hip's on the thigh, never on the pelvis it
   branches off) **and takes no skin by nearness**: its skin is its bone's, shared along that bone. (Built 2026-10-19: hung
   on the pelvis, the cat's hip helpers held the skin over its thighs and its legs swung inside a still body; given skin
   by nearness at the hip joint, they held the back above the hips and the back stepped as the legs moved.)
5. **Regions: where skin bends.** A band round a limb takes a percentage of the way round (`wide=45%`); a blob with no
   ring round it takes centimetres (`belly = region($belly_pt, long=260, wide=170)`). Orient a band along its limb:
   `[along=($hip_L, $hock_L), tilt=10]`. Measure a region that wraps far round its own line (the ear flap needed 259 cm
   of arc before it stopped tearing at 40° of flap). **A full band holds its own limb's skin** (2026-10-19 evening): it takes
   only skin nearer its own bones (the limb it turns, the bone that limb turns from, and helpers riding them) than any
   other bone, so a full band round a limb buried in its body (a cat's thigh inside its flank) no longer selects a slab
   through the whole body (it did, up to the head, and re-laid 57,000 triangles). ~~Keep such a band sided (`wide=`) until
   the tool can bound a band to its own limb~~. Check the flank's weights with the build's `DIAG_<Mesh>_Weights` overlay
   (`gc_render ... overlay:`) before you animate.
   **Rev 5 — BUILT, MEASURED, REVERTED 2026-10-20; not in force.** A band's family is still its joint, its parent
   BONE and everything below. Rev 5 changed nothing visible on the cat (chest wall 0.14 of the humerus's motion, was 0.13),
   and the foreleg folding into the chest is the motion: the shoulder folds -84 degrees with the arm flat under the chest
   (GC-RIG-BIND-OWNERSHIP-PLAN.md "Rev 5"). If an arm sinks into the body, check the pose before the skin.
   ~~**A band hands over from the whole part its joint turns from** (2026-10-20): its family is the joint, everything below
   it, and every bone that moves exactly as its parent, through bones that never bend. A blade that hangs from a rigid
   attachment on the ribcage (`mount_F_L = chest -> scapula_border_L -> scapula_L`) therefore turns from the chest: the
   chest skin under the blade's band blends into the blade and the upper arm as the bind blends it, and keeps its chest
   share. That band's size is how far the trunk hands over to the limb: measure it over the trunk skin the limb moves
   against (a cat's chest wall, where its elbow slides back), as every region is measured.~~ **Do not widen a limb's root band
   over the trunk to make the trunk follow the limb** (tried on the cat 2026-10-20 with Rev 5 on, and removed;
   GC-MOVE-CAT-PLAN.md "the blend"). The take-back only rescales what the bind gave, so the chest wall followed the humerus
   0.21 (was 0.14). Meanwhile the arm's bones spread back along the belly line and notched it up to 5 cm behind the elbow.
   Skin under no band of the limb
   still keeps only the bones it meets at a joint, so a far bone (a foreleg on the face) never reaches it.
6. **Skin: how each region behaves.** One hard fold with flesh rolling out: `[crease=0.9, folds=1, bulge=0.35,
   keep_volume=0.5]`. Broad soft bends state no `folds=` (it is a floor on the ring count; dropping it on three spine
   bands cut 9% of the mesh with no visible change).
7. **Sockets** for what rides the animal: `saddle = spine_02 [on=surface]`, `howdah = spine_01 [at=(0, 0, 110)]`.

## How a rebuilt rig keeps separate skin pieces on their limb

A `turns_in=$Region` band named by one joint also determines which whole connected mesh pieces beyond that band
belong to the joint and its descendants. A qualifying shoe shell or toe piece then takes its skin weights only from
that bone family, so moving the other leg does not pull it away. This is inferred when rebuilding skin; write the
band and joint as usual. The old bare `owns_skin` flag is refused and should be removed from older recipes.

A piece cut through by the band keeps the original bind's blend, and a band shared by several joints makes no
ownership claim. A connected body therefore does not become rigid limb by limb. Uncertain, oversized or conflicting
claims, and pieces without weight in the claiming family, can be left out; read the build's ownership and skip notes.
Nested ancestor/descendant claims compose. Imported meshes that keep their existing skin bypass this reweighting.

Coincident vertices split for materials, UVs or normals belong to the same connected skin at the asset's float
position precision. Before binding or cutting, unique matching boundary edges are joined in the working mesh,
so a ring cannot subdivide just one side of a material seam. UV and normal overlays and material IDs stay split;
nearby positions are not welded. The topology receipt counts joined pairs and ambiguous/unmergeable groups
left separate. After all weight passes, remaining coincident copies receive identical weights. The build's
`skin seam audit` reports how many coincident pairs it checked, how many still differ, their largest per-bone
weight difference, and how many differences it corrected. These counts describe the working skin before the
asset writer creates render-section copies. Source assets are unchanged. Rebuild existing rigs to apply this repair.

`diagnostic()` colours the `_Weights` mesh by the strongest bone on every material slot. Black means no positive
weight. Grey is not a weight value: check the display material. Older builds assigned the vertex-colour material
only to slot 0, so other sections appeared grey despite carrying valid weights. Rebuild the diagnostic to repair it.

Rebuilding a generated rig updates its skeleton in place, so existing clips retain their skeleton reference.
Rebake clips when changed joint intent or rest proportions require new motion; rebuilding does not orphan them.

Rebuild the rig before inspecting every affected clip. Look at ankles, fingers, neck and deep folds as well as the
corrected piece: a compact foot alone does not establish good deformation everywhere. The shipped elephant text uses
straight-zero ranges and `influences:12`, matching its repaired native build; deep-fold playback still shows leg/hip
skin fins and slits, so that extreme deformation is unaccepted. For your own mesh, measure the rest bend and respond
to any required influence-cap message, then judge the actual skin in motion.

## How fast a joint can move: `velocity:`, `accel:` and `ramp:`

A `.gcmove` line timed by effort (`accel=90%`, ANIMATION.md, "Time a motion by effort") reads
these from the rig, so an animation never has to state them.
- **In the header, for every joint:**
  - `velocity:` is the character's normal full-effort turn rate, in deg/s;
  - `accel:` is how fast that rate can change, in deg/s²;
  - `ramp:` is the quickest a joint builds to full acceleration, in seconds.
  Each is a plain number above 0, with the unit implied by the key: `velocity: 600`, never
  `600deg/s`.
- **On one joint, overriding the header:** `[velocity=1500]`, `[accel=40000]`, `[ramp=0.035]`.
  `parent:` and the `_R` twin carry them as they carry `range=`.
- **They are references, not maximums.** A measured motion past one is reported, not refused.
- **Nothing is defaulted.** A joint with no reference is unknown: a `.gcmove` asking a share of
  it is refused, naming the joint and the key.

```gcrig
gcrig 1  velocity:600  accel:8000  ramp:0.05
joints {
  calf_r [range=(-1.6, 141.9), accel=40000, ramp=0.035]
  thigh_r [range=(-35, 133.8), accel=25000, ramp=0.035]
}
```

The example values are human-scale starting points from published motion studies: knee about
40,000 and hip about 25,000 deg/s², with ramps near 0.035 s. A whipped joint in a skilled strike
reaches them; a joint moved by its own muscles alone reaches about 15-50%. Set them for your
character.

The shipped [MetaHuman body template](Examples/MetaHumanBody.gcrig) declares the following
references per joint (research revised 2026-10-10). Copying it and naming it as `parent:` carries
these into a character's rig. A `.gcmove` naming that child's `controls:` reads the current
inherited references when building its clip. Rebuild timed clips to use changed values;
existing baked clips retain their keys until rebuilt.

| Joint | Velocity (deg/s) | Accel (deg/s²) | Ramp (s) |
|---|---:|---:|---:|
| Shoulder fold/side | 1100 | 25000 | 0.05 |
| Elbow flexion/extension | 2200 | 40000 | 0.025 |
| Wrist | 2500 | 150000 | 0.015 |
| Finger MCP/PIP | 1000 | 26000 | 0.04 |
| Finger DIP | 600 | unknown | unknown |
| Hip | 850 | 25000 | 0.035 |
| Knee | 1600 | 40000 | 0.035 |
| Ankle | 1000 | 40000 | 0.025 |
| Each of five spine joints | 190 | 7400 | 0.04 |
| Each of three neck/head joints | 266.67 | 3333.33 | 0.08 |

These are starting references, with rounded cited speeds and mostly extrapolated acceleration
and ramp values. The revised elbow and knee accelerations use published throw/kick estimates;
hip acceleration also uses scaled project gait measurements. The revised elbow, hip and knee
ramps use muscle-model measurements. Some recent citations were available only as search
summaries, so these values have not been calibrated on the template body. Whole-trunk and
whole-neck speed/acceleration are divided across their serial joints.

Clavicles and thumbs have no matching reference row; toes and corrective/twist helpers receive
no invented default. The current format carries one reference set per joint. It therefore uses
the shoulder's bending reference and the elbow's flexion/extension reference: the much faster
throwing-twist references (shoulder 7000/500000/0.01; forearm 2000/150000/0.015) require separate
axis references before they can be used accurately for percentage timing.

As starting effort bands, everyday reaching uses about `accel=1%` to `9%`, walking `3%` to
`17%`, and running near 4 m/s `22%` to `53%`. Skilled strikes can use `50%` to `100%` through
the whip of parent segments. A joint's own muscles alone usually reach `15%` to `50%` of these
references. Inspect the resulting clip and adjust for your character and action.

## How freely a joint's body moves: `dynamic=`

`dynamic=<0..1>` says how freely a part's physics body moves when a clip simulates it: 0, only
animation moves it; 1, it swings free; between, a spring holds it toward the clip's pose, weaker as the
number rises. It acts in a `.gcmove` clip that simulates physics (`gravity=true`), never in plain
playback; a Control Rig Physics setup made by hand reads the same drive but pulls toward the middle of
the joint's range unless it follows the animation. On a joint it sets that joint's body; on a chain it
reaches no joint yet: write it on the joints.

```gcrig
build {
  PHAT_Hand = physics_asset()
}
joints {
  hand_r [turns_in=$wrist_band_r, range=(-60, 60), side=(-30, 30), dynamic=0.75]
}
```

- It reaches the rig's own `physics_asset()` and nothing else: a rig with no such build line notes that
  its declarations reach no asset. A hand-made physics asset a clip names is not changed by it.
- Unset is 1, free, which is what every rig-built asset already was. Absent `jiggle=` is 0 for the same
  reason from the other side: each default is what happens today.
- The scale is not linear, and its numbers are starting values until the calibration table lands
  (MOVE-RELAX1 S1): below about the middle a joint barely leaves its pose. The build notes what it wrote
  for each declared joint; how far a joint actually swings is measured in the clip's bake.
- The clip's `physics($Hand, weight=…)` releases a share of what the joint declares; at 100% only the
  body's damping is left. Write `dynamic=` for what a joint is, `weight=` for what one clip does with it.

## A twist range that follows the bone's elevation: `twists_raised=`

A person turns the upper arm out further with the arm raised than with it hanging (outward 55-74 hanging, about 90 at the arm
level, active, in clinical studies: `Docs/Research/shoulder-rotation-by-arm-elevation.md` in the OraStudio project). `twists=` is one pair
for every arm position; `twists_raised=` is the same roll's pair with the bone LEVEL, 90 degrees up from the trunk's down:

```gcrig
joints {
  upperarm_r [twists_raised=(-90, 73.8)]   # beside an inherited twists=(-73.8, 73.8); alone in a file it is refused
}
build {
  IKR_MyRaised = ik_rig()
}
```

- **Each stop moves in a straight line with elevation**, from the `twists=` end at 0 degrees (bone hanging) to the `twists_raised=` end
  at 90 (bone level); above 90 the raised pair stays. Elevation is the bone's line against the trunk's down (the joint's grandparent, an
  arm's `spine_05`), so a leaning body or the arm's own twist does not change it.
- **It must contain `twists=`** at both ends, needs `twists=` on the merged joint (write it beside an inherited `twists=`, or write both)
  and a ball joint (`range=` and `side=`); each is refused by name, with what to write.
- **Inheritance:** a child may write the key alone and keeps the rest of the parent's line. A child that restates `twists=` without it
  DROPS an inherited `twists_raised=` (the build notes it); a bare line or `orientation=` clears it.
- **A mirror twin takes the same two numbers, NOT swapped.** A `_L` line derives its `_R` twin with `(-90, 73.8)` as written, though the
  twin's outward end may be the other one. A mirrored arm needs its own sign check; write each side's line yourself.
- **Outward means the arm turned away from the body** (with the elbow bent, the forearm swings out to the side, the thumb pointing away).
  Which end of the pair is outward is the bone's own sign. **Only the MetaHuman template's RIGHT upper arm is verified**: there it is the
  MINUS end (its long axis is the bone's -X), so the outward end that grows is the low one. **No left line ships**: the template's
  `upperarm_l` sign is unchecked, so do not copy the right line to the left by guess.
- **A wrong sign is NOT refused.** A pair that grows the wrong end still contains `twists=`, so it builds and silently widens the INWARD
  end instead. Only a sign checked on the body (a short probe clip) tells you which end to grow.
- **Tested only on the upper arm's roll.** No other joint has been checked with it.
- **What it reaches today:** the IK rig keeps `twists=` as its own limit, so every native reader is unchanged, and carries the raised
  pair beside it as metadata (read back with `gc_read kind:"rig"` on the IK rig). **A file with the key and no `build { ... ik_rig() }`
  records it nowhere**; its build note says the pair is carried nowhere. **A `.gcmove` reads it**: for a joint that carries the key, Move
  takes the twist range at the pose's own elevation, not the hanging pair, in its IK placement and clamps, limb placement, balance's trials,
  collision's fit and slide, and the cut-building side (the key rule, the free-wrist search, the trailing travel's margins, a key near a stop,
  and the stop a refusal names, which says "at elevation e"). Measured on the test rig: an arm about level keeps an outward turn of 89 where
  the plain rig keeps 73.8; hanging keeps 75.7; an arm forward and down (elevation about 36) stops at 80.2. **A rig without the key builds
  and moves exactly as before**: a rig opts in per joint.

  Worked example, to copy for a right upper arm: `upperarm_r [twists_raised=(-90, 73.8)]` beside an inherited `twists=(-73.8, 73.8)`.
  The MINUS end is the outward one for the MetaHuman right arm, so it grows from -73.8 (hanging) to -90 (level); the plus end stays.
  The 90 is the active outward turn with the arm level, from two goniometer studies (`Docs/Research/shoulder-rotation-by-arm-elevation.md`).

## Reading what the build says about joints

- **A build note's angles are measured from the rest pose, the rig's are from straight.** The VR hand's forefinger
  middle joint rests bent 14.9°, so its `range=(0, 110)` appears in a motion's note as *"fold at 95° (-15 to 95)"*.
  Check the rest bend before concluding a limit is too tight.
- **"At a limit then: '<joint>' ..."** at the end of a reach note means that joint is at its stop: the anatomy is
  what stops the motion.
- **"joint '<j>' is turned N degrees about its own length so it folds about its bone's own Y axis"** (since 2026-10-17):
  a bone built from points is framed by its aim, and where the world's up would have left its fold more than 15° off
  every axis of the bone (a leg pointing straight down, a body standing diagonally), the build turns it about its length
  so the fold IS one of its axes. Then `swing(axis=fold)`, the IK rig's per-axis limits and the physics asset all mean the
  same turn. Nothing to do; a rig built before this keeps its old frames until rebuilt, and its clips need rebuilding then.
- **A band centred on a point on a limb's SIDE folds the limb sideways.** A band's `wide=` runs round the limb and is the
  fold axis, so a part-wide band (`wide=45%`) round a point on a tail's flank made the tail's `fold` swing it sideways and
  its `side` tip it back and down at once: no line could lift it (the cat, 2026-11-04). Centre a part-wide band on the
  side the joint closes toward (a knee's back, the toes' top), or for a tube that bends every way (a tail, a trunk) wrap it,
  `wide=100%`, and say which way it closes, `closes=back`. Check with a few-frame probe clip (one joint, one axis, 20
  degrees a frame) read off its readings before writing motion. Changing a band re-rolls its bones on the rebuild, so clips
  BUILT on the old frames read as a twist on the joint's locked axis: rebuild them, or stop reading those bones from them.

## When a limb cannot reach

Climb this ladder, one rung at a time, and look at the whole limb after each change:
1. Is the limit the real animal's? Raise it only up to life.
2. Change the touching area: declare another contact (the fingertip, the side of the finger, the heel of the hand).
3. Change the grip or placement in the motion (`drive=`, `slide=`, the seat).
4. Change the point on the thing touched (a socket in its `.gcshape`), only where a real limb would touch.
5. Accept a near touch and say how near; never bend past life.

## Rebuild order

`.gcrig` → the skeletal mesh, skeleton, physics asset and IK rig → every `.gcmove` that uses them. A contact or a limit
lives on the skeleton, so adding one means rebuilding the rig (`gc_apply` on the `.gcrig`), then the clips. To try a
contact without rebuilding, a small adapter `.gcrig` with only `points { }` and `contacts { }` can be named by a
clip's `controls:` header line; move it into the real rig once it is right.
