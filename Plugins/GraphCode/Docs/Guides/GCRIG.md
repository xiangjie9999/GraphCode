# Before you write a `.gcrig`

Read this page, then every guide it lists for what your rig must do, then the nearest shipped rig,
**before the first line of the file**. Then ask the tool for each block you will use
(`gc_describe kind:"rig" block:"<block>"`). Write in your plan which of them you read.

A rig is what every later clip stands on. A bone, a contact, a socket or a range the rig lacks does not
make the step that needs it optional: add it here. "The skeleton has no such bone" is a reason to write
a rig line, never a reason to skip a step in a `.gcmove` ([HANDS.md](HANDS.md#the-seat-is-never-skipped)).
If the rig builder refuses a line the method needs, that is a tool fault to fix and record, not a
detour to take.

## You want → write

Look here before you write a workaround or ask for new grammar. Each row names the spelling in one
line; the last column is where the tool or a guide explains it (`gc_describe kind:"rig" block:"<block>"`).
If the tool's help and a row disagree, the help is right.

| You want | Write | Ask or read |
|---|---|---|
| a joint that stops at a real range | `joints { knee [turns_in=$knee_back, range=(-5, 145), closes=back] }`: every limit is a (min, max) pair in degrees and zero is straight | `block:"joints"`; [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) |
| a forearm that folds at the elbow and rolls along its length | `twists_in=$Region` beside `turns_in=`; the fold stays in its short band, the roll goes along the forearm | `block:"joints"` |
| a twist range that widens as the arm rises | `upperarm_r [twists_raised=(-90, 73.8)]` beside `twists=`; `twists=` is the bone hanging, `twists_raised=` the bone level; a `.gcmove` reads it at the pose's elevation | `block:"joints"`; [RIG.md](RIG.md#a-twist-range-that-follows-the-bones-elevation-twists_raised) |
| a joint to move faster or slower than the default | `velocity:`, `accel:` and `ramp:` on the joint line | [RIG.md](RIG.md#how-fast-a-joint-can-move-velocity-accel-and-ramp) |
| a bone for a held thing to ride, where the hand has none | a socket with `bone=`: a skinless carrier bone the clip seats the prop with | `block:"sockets"` |
| a second grip point that reuses the first one's frame | `web_grip_r = palm_grip_r [at=$web_r]` | `block:"sockets"` |
| IK that places or points a held thing | a `chains` line ending at the bone that carries it | `block:"chains"`; the table below |
| feet placed by IK without turning the hips | foot chains that start at the thigh, not the pelvis | the table below |
| a head that looks at a target | `neck = neck_01 -> neck_02 -> head [goal=$head]`, then `aim` in the clip | `block:"chains"` |
| a place on a hand or foot a clip can plant, reach or grab with | `points`, `contacts` and `grips` | `block:"points"`, `block:"contacts"`, `block:"grips"` |
| a foot that rolls heel, ball, toe | a contact on the foot joint (`Heel_R = contact(chain=leg_R, at=$sole_r, bone=foot_r)`), read by `plant(…, roll=)` | `block:"contacts"`; [WALK-AND-RUN.md](WALK-AND-RUN.md) |
| creases, folds and bulges where a joint bends | a `skin { knee_back [crease=…, folds=…] }` line | `block:"skin"` |
| a shape the skin takes without bones (a breath, a swell, a puff), played by a clip | `morphs { Breathe = morph() { displace(by=0.4, along=out) [in=$flank_L] } }`: Shape's `displace` on this rig's regions, several lines adding; each morph is a control of its name, so a clip writes `drive(Breathe, by=…)` | `block:"morphs"`; the build note's vertices-moved and largest-move figures |
| a shape that comes in as a joint folds (a biceps, a crease), in every clip, blend and IK pose | `Biceps_L = morph(follows=elbow_L.fold, range=(30, 140)) { … }` in `morphs { }`: weight 0 at the first angle and 1 at the second, set while the game runs by the generated post-process Control Rig; no clip drives it; a `_L` morph derives its `_R` twin | `block:"morphs"`; the build note's symmetry figure |
| a MetaHuman smile, blink or jaw without rebuilding the face | a controls-only adapter and `drive(…)` in the clip | [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md) |
| collision bodies that grips and self-collision read | `physics_asset() [bodies=hull, gives=…]` | [RIG.md](RIG.md) |
| two bodies of that physics asset to ignore each other at runtime | `collision_pairs` on the `physics_asset`, not a per-body instance setting | `gc_describe kind:"physics_asset" block:"collision_pairs"` |
| a joint's inherited rest put back to straight | `rest=(fold=0)` in the child: each rest angle is inherited separately, so it resets fold and keeps an inherited side. There is no reset word; a zero value is the reset. A bare joint name in a child clears inherited ranges and rest values and makes the joint rigid | `block:"joints"`; [RIG.md](RIG.md) |

## Every `.gcrig`

| Read | For |
|---|---|
| [RIG.md](RIG.md) | the blocks, Route A (a mesh that already has bones) and Route B (a bare mesh), the rebuild order, reading the joint notes |
| [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) | ranges from studies, and how a range is read against a joint's rest |
| [RIG.md: how fast a joint can move](RIG.md#how-fast-a-joint-can-move-velocity-accel-and-ramp) | `velocity:`, `accel:`, `ramp:`, the references every effort-timed clip reads |

## What the rig is for, and what to read for it

| The rig | Read | Shipped example |
|---|---|---|
| named MetaHuman face expressions without rebuilding the face | [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md): controls-only adapter over native Face curves | [FaceExpressions.gcrig](../Examples/FaceExpressions/FaceExpressions.gcrig) |
| a MetaHuman | [METAHUMAN-RIG.md](METAHUMAN-RIG.md): inherit the shared 84-joint map in a small per-character adapter; [CHARACTERS.md §7.2](CHARACTERS.md#72-native-metahuman-assets-are-a-preservation-boundary) preserves native assets | [Examples/MetaHumanBody.gcrig](Examples/MetaHumanBody.gcrig), also installed in this project at `Content/GraphCode/Rig/MetaHumanBody.gcrig` |
| another character you did not build | [RIG.md](RIG.md) Route A; [CHARACTERS.md](CHARACTERS.md) section 7: read its actual hierarchy and preserve the source assets | `VRHandRig_20261001/VRHand.gcrig` |
| a hand that holds things | [HANDS.md](HANDS.md) Step 2 (rig the hand), [The seat is never skipped](HANDS.md#the-seat-is-never-skipped) | `VRHandRig_20261001/VRHand.gcrig` |
| a hand with NO bone the held thing rides (`palm_r` on Epic's VR hand) | the `sockets` block's `bone=`: `palm_grip_r = hand_r [at=..., toward=..., up=..., bone=palm_r]`, a skinless carrier bone the clip seats the prop with | `VRHandRig_20261001/VRHand.gcrig` (its `palm_r` is Epic's) |
| a held thing that a clip places or points by IK (a sword's landing, a tool on its work) | a `chains` line ending at the bone that CARRIES the thing's bone: `sword_r = clavicle_r -> upperarm_r -> lowerarm_r -> hand_r` when the thing rides `palm_r` (a MetaHuman's `arm_r` ends at `middle_metacarpal_r`, which does not carry it; ending at the skinless carrier lets IK spin the thing); [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade) | `BlueEdge_20261013/MetaHumanSwordRig.gcrig` |
| a held blade whose line and edge a clip states (a cut's start, its landing) | a bone of the blade's own on the bone the thing rides, +X along the blade, +Z its edge: `blade_grip_r = palm_r [at=(0, 0, 0), toward=Z, up=-X, bone=blade_r]`; every pose then reaches `frame(bone=blade_r, toward=+X, up=+Z)` to one frame saying the place, the line and the edge; [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade) | `BlueEdge_20261013/MetaHumanSwordRig.gcrig` |
| a held thing carried by the arm, not dragged by IK (a sword wrist bent to point the blade) | keep the wrist's measured range (the MetaHuman template's `hand_r [range=(-60, 60), side=(-30, 30)]`, [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md#wrist)) and let the ARM point the blade: find the hand's place with the bubble at full reach, reach only the hand, no `aim` through the wrist (a +-15 clamp used until 2026-10-22 had no source and barred a level blade at chest height with a long arm, which a real wrist holds); [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade) | `BlueEdge_20261013/MetaHumanSwordRig.gcrig` |
| feet placed on footholds by IK while the hips are posed by swings (a lunge's keys) | foot chains that start at the THIGH, so a reach does not own the pelvis: `stand_l = thigh_l -> calf_l -> foot_l -> ball_l`, and for a foot on its ball the ankle too, `heel_l = thigh_l -> calf_l -> foot_l` (reach both: the ball's frame and the ankle's point). A reach turns only its own chain and holds everything above it; `leg_l` starts at the pelvis, so reaching the foot turned the hips. A MetaHuman inherits both from the shared template (2026-10-14) | `Examples/MetaHumanBody.gcrig` |
| a head that looks at something | a chain of the neck and head, `neck = neck_01 -> neck_02 -> head [goal=$head]` (from `spine_05` it also moves both shoulders), aimed from a frame on the face (`aim` turns the chain's last bone toward the target by that bone's +X unless `from=` says otherwise); a MetaHuman inherits it, and its face is the head bone's +Y ([METAHUMAN-RIG.md](METAHUMAN-RIG.md)) | `Examples/MetaHumanBody.gcrig`, `KitchenCup_20261011/KitchenCupBody.gcrig` |
| a thumb or finger that cannot reach where a real one does | [HANDS.md](HANDS.md) "When the hand cannot reach"; a second hinge at a joint ([RIG.md](RIG.md) Route A) | `VRHand.gcrig` `thumb_opp_r`, `thumb_roll_r` |
| feet that stand, walk or balance | [WALK-AND-RUN.md](WALK-AND-RUN.md) (rolling-foot contacts), [MOVE-BALANCE.md](MOVE-BALANCE.md) (measured sole rings that balance reads) | `Balance_20261009/BalanceContacts.gcrig` |
| touch or contact points (pads, tips, soles, the web) | the `contacts` and `points` blocks; [POSES.md](POSES.md) | `VRHand.gcrig` `contacts { }` |
| collision bodies for self-collision and grips | [RIG.md](RIG.md) `physics_asset() [bodies=hull, gives=...]` | `VRHand.gcrig` |
| a native Control Rig | [CONTROL-RIG.md](CONTROL-RIG.md) | |
| an animal or a bare mesh | [RIG.md](RIG.md) Route B | [ElephantRig](../Examples/README.md) |

## Before you call it done

Read the build's notes (joint axes, limits, bodies, what was written where), then pose it in one short
`.gcmove` and look at it, as [RIG.md](RIG.md) says.
