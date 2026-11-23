# Examples

Working recipes, built and looked at in the GraphCode project. Each folder holds the text files an author writes; the
assets they make are produced by `gc_apply` on each file in the order below. Read them beside the guides they come from.

## FaceExpressions: smile, blink and jaw opening

Guide: [FACE-EXPRESSIONS.md](../Guides/FACE-EXPRESSIONS.md). Reusable controls over seven native MetaHuman
Face curves, with the channels and three-second timing of the existing native-tested expression example.

| File | What it is | Use order |
|---|---|---|
| [FaceExpressions.gcrig](FaceExpressions/FaceExpressions.gcrig) | Controls-only adapter: `smile`, `blink`, `jaw_open`; no mesh rebuild | 1: copy to `Content/FaceExpressions/` |
| [Expressions.gcmove](FaceExpressions/Expressions.gcmove) | Two blinks, a held smile and a little jaw opening | 2: set your MetaHuman Blueprint path, dry-run, then apply |

This example requires an assembled native MetaHuman with a functioning Face/RigLogic setup. It builds a Face
animation, not a Body animation or a shared performance player. Other expressions and speech layering require
their own calibrated channels and ownership; see the guide before combining them.

## HandAndPistol: a rigged hand that holds a pistol

Guide: [HANDS.md](../Guides/HANDS.md) (Step 6, and "Holding any weapon") and [RIG.md](../Guides/RIG.md) (Route A).

| File | What it is | Build order |
|---|---|---|
| `VRHand.gcrig` | Epic's VR hand kept as imported: joint ranges, the thumb's two base hinges, `IndexPad` and `IndexTip` contacts, the grip sockets, hull bodies | 1 |
| `Pistol.gcshape` | the pistol with its origin at the web of the hand and its own sockets (`Web`, `Trigger`, `ThumbRest`, `IndexRest`) | 2 |
| `BP_VRHandPistol.gc` | the hand mesh with its physics asset, and the pistol attached at `web_grip_r` with no offset | 3 |
| `VRFist.gcmove` | a fist, the simplest hand clip | 4 |
| `VRPistolGrip.gcmove` | the pistol hold: seated by touch, three fingers wrapped, thumb across then curled, then the fingertip onto the trigger with `collision=none` | 4 |

Source meshes: the hand is `SKM_MannyXR_right` from Unreal's VR Template (`/Game/XRMannequins`), and the pistol is the VR
Template's `/Game/Weapons/Pistol/Meshes/SM_Pistol`. Add the VR Template content to your project, copy the hand mesh and its
skeleton into your own folder (the rig writes sockets and joint axes onto the skeleton it is given), and point the files'
paths at your copies.

## StairsInAGame: a walk fitted to stairs at runtime

Guide: [STAIRS-IN-A-GAME.md](../Guides/STAIRS-IN-A-GAME.md). Unreal's tutorial character walks up a five-step flight
and stops near the top: the capsule rides a hidden ramp, each foot is traced onto its step, the hips take the height of
the foot that touched down, and at rest the back foot settles onto its step. Everything it uses ships with the engine
(`/Engine/Tutorial/SubEditors/TutorialAssets/Character/`: the mesh, `Tutorial_Walk_Fwd`, `Tutorial_Idle`) or is made by
these files, so it builds in any project.

| File | What it is | Build order |
|---|---|---|
| `IKR_StairLegs.gc` | an IK Rig with one Limb solver per leg, goals `FootL`/`FootR` | 1 |
| `ABP_StairLegs.gc` | the Animation Blueprint: foot traces, touch-down hip height, rest settle; walk/idle by speed | 2 |
| `BP_StairWalker.gc` | a Character that walks forward until `StopX`, with the Animation Blueprint | 3 |
| `BP_StairCourse.gc` | five 16 cm × 30 cm steps, a landing, the hidden ramp, and the walker as a child actor | 4 |

Each is one `gc_apply create:true` at `/Game/StairsInAGame/<Name>`; render the course with the guide's step 7.

## WalkAndRun: a rolling walk and a first run

Guide: [WALK-AND-RUN.md](../Guides/WALK-AND-RUN.md). The rifle walker's legs with each planted foot rolling heel →
ball → toe (`plant(..., roll=)`), the feet carrying the body (`drive=Root`), and a probe whose body flies between
steps. Rebuilt and measured 2026-10-12, with the body carrying its own speed: on the walk and the probe a held point
ends within 0.01 cm of its place and nothing goes under the floor; the body's forward speed dips 0-9% per step instead
of stopping at each landing; the probe's body flies on a gravity arc (apex 10-11.5 cm) at its take-off speed. The
rifle walker's rig states no `body_accel:`, so the notes say its acceleration is unbounded. The run uses the same
850 rig; its earlier 450 measurements are not claimed for this refresh.

| File | What it is | Build order |
|---|---|---|
| `FootRoll.gcrig` | an adapter: heel, ball and toe contacts per foot, each on its own bone (`bone=`), for a rig that has none | 1 |
| `RollWalk.gcmove` | the walk: `A_RollWalk` (game loop, `[root_motion]`), `A_RollWalk4` (four strides for a render), `A_RollWalkFK` (the legs as drawn, no plants, to compare) | 2 |
| `FlightProbe.gcmove` | the walk's legs with each foot down 35% of a stride, so the body flies twice a stride: checks the carried speed | 2 |
| `BP_GaitViewerRoll.gc` | the walker over a floor (top at z=0), playing `A_RollWalk4`: render it orthographic from the side | 3 |
| `Runner.gcrig` | the walker's rig with a person's ankle, hip and trunk ranges and the foot contacts built in, for running; its header states the body's references, `body_speed:400 body_accel:850` | 1 (for the run) |
| `Run.gcmove` | a measured human run (3.96 m/s, Hamner, Seth and Delp 2010; credit in [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md)): `A_Run` (one stride, game loop), `A_Run4` (four strides whose body carries its speed through every landing and flight), `A_RunFK` (the legs as drawn) | 2 (after `Runner.gcrig`) |
| `RunKick.gcmove` | a run-up into a right-footed instep kick and its recovery (about 3.2 s): `A_RunKick` (`[root_motion]`) and `A_RunKickWorld` (the same without root motion, so a preview shows the body travel) | 2 (after `Runner.gcrig`) |
| `RunKick-BuiltContent.zip` | the run and the kick rebuilt with body_accel:850 (native13), with the runner and two viewers, to watch without building: unzip into your project folder and read its `HOW-TO.txt` | — |

**The run-up kick (native13, improved example).** Rebuilt from the original source with `body_accel:850`: the rig
apply took 251.72 s, RunKick 106.47 s and Run 58.20 s. In the final 50-frame window, runtime total 3D speed at the
end fell from 95.14 to 4.80 cm/s. RunKick stores 386 keys at 120 Hz; playback was measured at 480 Hz. Run and
RunKick were rendered over a floor. The final landing is accepted as an improvement, with contact defects below;
balanced kick variants are not included. This is not a whole-motion certificate.

The final 50-frame skin check found no floor penetration: minimum right-sole height was 0.02485 cm in source
tracks and 0.02508 cm in runtime playback; left was 0.01403/0.01483 cm. Maximum runtime held-point movement in
the floor plane was 0.05231 cm left and 0.01724 cm right. The strict held-point check still reports 37 misses
(previously 42), so this is not a full contact pass. Remaining whole-clip joint and compression limits are
not closed by these landing measurements.

Seven earlier problem windows were also sampled. Their worst runtime sole dip is 0.385 cm (previously 0.461 cm),
but recovery got worse: right frame 310.5 dips 0.367 cm (previously 0.167 cm), and left frame 303.5 dips 0.131 cm
in a previously clear window. These defects remain. The earlier lower-than-drawn crouch and free kicking leg
stopping short have not been closed by this check. Ordinary Run was rebuilt and rendered with the same rig;
there is no fresh whole-run numerical contact certificate.

These files use the rifle walker of this project (`/Game/GraphCodeExamples/RifleWalk_20261003`: `BP_RifleWalker`,
`IKR_RifleMan`, `SM_RifleManBody`, a rebuilt copy of Unreal's tutorial mannequin). In another project, point the paths
at your own character, its IK rig and its mesh, and measure the sole points on your mesh. `Run.gcmove` is written by a
generator from the measured take; its header lists every curve's source joint and mapping.

## ElephantRig: a creature from a mesh with no bones

Guide: [RIG.md](../Guides/RIG.md) (Route B). `Elephant.gcrig` shows a whole animal: measured points, a spine, trunk, ears,
tail and four legs as chains, rigid and bending joints with the regions they turn in, skin behaviour per region, and
sockets for what rides it. The mesh it was measured on is a marketplace asset and does not ship; use it as the pattern and
measure your own animal.

Updated 2026-10-07 for ranges measured from straight and a 12-influence cap, matching the current native fixture's
successful rebuild. Its deep-fold playback still shows leg/hip skin fins and slits; this is an authoring example,
with that extreme deformation unaccepted. Inspect and repair the extreme poses on your own animal before using it
as a production character.

## SkirtFabrics: the same simulated skirt in silk and in denim

Guide: [GARMENT.md](../Guides/GARMENT.md) ("How an `[as=cloth]` garment moves is its fabric's numbers") and
[ANIMATION.md](../Guides/ANIMATION.md#hair-clothing-and-parts-that-swing). `SkirtFabrics.gcgarment` builds one A-line skirt
twice, `[as=cloth]` and held from the hips with `hang(from=$Hips)`, differing only in `fabric()`: silk at 80 g/m² and a 1.5 cm
bending length, denim at 400 g/m² and 4.5/4 cm. Worn under the same clip they hang and swing visibly differently: the silk in
many narrow folds close to the legs, the denim in broad folds with its hem standing further out. It was measured on a
MetaHuman (`mesh:`) and the project's own denim and ivory materials; point `mesh:` at your character and `material()` at
yours, and re-measure the rings on your body with `gc_render mode:"ask"`.

## TShirt: make a T-shirt, step by step

Guide: [GARMENT.md](../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../Guides/GARMENT-PIECES-AND-EDGES.md).
[TShirt/README.md](TShirt/README.md) builds a crew-neck cotton-jersey tee up in the order a maker thinks (fabrics, rings,
armholes and sleeves, a rib neckband cut short, hems as turned cloth with coverstitch rows, pressed armhole seams), quoting the
lines and why, with the pictures rendered on the published 16 Nov build as the expected result. Beside it: `TShirtArmsDown`,
`TShirtOversizedArmsDown` (two cuts on the arms-down body) and `TeeSleeveHem` (a sleeve hem turned as real cloth).

## GarmentBodies: the two bodies the garment examples are cut to

A customer does not have the project's test bodies, so they ship as Shape files. `TPoseBody.gcshape` makes
`/Game/GraphCodeExamples/Garment/SM_TPoseBody` (about 180 cm, arms out along X); `ArmsDownBody.gcshape` makes
`SM_ArmsDownBody` (arms 20 degrees from the sides). Build the body first, then the garment. Every ring in the garment
examples is measured on these bodies; on another body, re-measure them.

## Common garments: worked garment files

Each folder holds the `.gcgarment`, the parts file it references and a short README. **Not rendered on the published 16 Nov
build** except the T-shirt; `ButtonShirt` and `Dress` may fail to build on that build (under investigation). Hoodie, ZipJacket and
LongCoat carry pictures rendered 16 Nov on an earlier build.

| Folder | Garment |
|---|---|
| [ButtonShirt](ButtonShirt/README.md) | button-up shirt |
| [CargoTrousers](CargoTrousers/README.md) | cargo trousers |
| [Dress](Dress/README.md) | dress |
| [Gambeson](Gambeson/README.md) | quilted gambeson |
| [HoodedCloak](HoodedCloak/README.md) | hooded cloak (needs a MetaHuman body) |
| [Hoodie](Hoodie/README.md) | hoodie |
| [Jeans](Jeans/README.md) | jeans |
| [Joggers](Joggers/README.md) | joggers |
| [LeatherArmour](LeatherArmour/README.md) | leather armour |
| [LinenTunic](LinenTunic/README.md) | linen tunic |
| [LongCoat](LongCoat/README.md) | long coat |
| [Robe](Robe/README.md) | robe |
| [Scarf](Scarf/README.md) | scarf |
| [Shorts](Shorts/README.md) | shorts |
| [SuitTrousers](SuitTrousers/README.md) | suit trousers |
| [Sweater](Sweater/README.md) | sweater |
| [TacticalVest](TacticalVest/README.md) | tactical vest |
| [TankTop](TankTop/README.md) | tank top |
| [ZipJacket](ZipJacket/README.md) | zip jacket |
