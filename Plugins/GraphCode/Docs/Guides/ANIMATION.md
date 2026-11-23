# Animate a character or an object

For a cinematic, begin with [CINEMATICS.md](CINEMATICS.md): develop the scene's intent, staging,
edited animatic and coordinated performance before finishing individual clips. Use
[CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md) for sequence bindings, timing and delivery.

Reusable forms, independent pose amounts/profiles, and selected-object collision fitting: [pose authoring](POSES.md).
A fist, a held handle, pistol or rifle, keeping the hold in later clips, and walking with a rifle in both hands (planted feet, `[root_motion]` for a game, `repeat=` for a take), step by step: [hands](HANDS.md).
Walking with independently timed breathing, additive references, native masks and action layers:
[animation layers](ANIMATION-LAYERS.md).

Start with the motion the viewer must see: where the body or object starts, what moves it,
what stays in contact, and how it stops. A successful build is not evidence of a convincing
performance. Inspect playback, including approach, contact, fastest motion and release.

Follow [the shared quality bar](ASSET-QUALITY.md#apply-the-quality-bar-to-every-medium): exceed the
minimum motion with purposeful, theme-consistent craft unless simplicity is explicitly requested.
Plan distinctive performance details, then inspect and repair every relevant contact, phase and
transition through repeated playback. A few attractive key poses do not prove the full motion.
For a living character, include the [natural-motion pass](#natural-movement-is-part-of-the-performance)
as part of production authoring, even when the request names only the main action.

**Split 2026-10-20.** This guide passed 800 lines, so its superseded stride-rule bullet moved verbatim to
[ANIMATION-DONE.md](../Historical/ANIMATION-DONE.md) and the step-by-step `grab()` walkthrough to [HANDS.md](HANDS.md#the-grab-route-named-pads-on-a-rig-without-skin-hull-bodies), a pointer
standing where each was. Every heading kept its place.

## Choose the appropriate animation path

| Subject | Authoring and playback |
| --- | --- |
| One rigid prop moving through the world | Native Level Sequence transform tracks or a Blueprint timeline. A static mesh does not need a skeleton. |
| An articulated machine, hinge, piston or creature | A skeleton/IK rig describing its actual joints; `.gcmove` produces a native animation sequence. |
| A character's body | `.gcmove`, imported animation (motion capture, or [measured from video](MOTION-FROM-VIDEO.md)) or retargeted animation; play and combine sequences through an AnimBP, montage or Sequencer. |
| A native MetaHuman body and face | Keep the native meshes and rigs. Animate the resolved body and face separately and synchronize native playback. The body takes `.gcmove` directly through an adapter `.gcrig` ([CHARACTERS.md §7.4](CHARACTERS.md#74-animate-a-metahuman-body-with-gcmove-an-adapter-gcrig)); a retarget is also possible. See [CHARACTERS.md](CHARACTERS.md). |
| A held sword, steering wheel or other prop | Animate the character and prop together through native attachments/playback. A static `grab` contact bake alone does not animate possession or carrying. |

Use tool help for the installed interface, then `gc_describe` for the exact native asset kind or
authoring grammar. Keep `.gcrig`/`.gcmove` files as source; use `gc_apply` to build their outputs.
An `anim_sequence` `.gc` describes supported settings, curves, notifies and markers; it is not a
replacement for the bone-motion source. Native `level_sequence` writes belong to that kind's
grammar. Do not invent a `.gcmove` instruction to move an arbitrary actor.

## Build the runtime performance with native animation tools

**For gameplay animation, plan the runtime composition as well as the individual clips.** Use
Blend Spaces and the other native mechanisms when they fit the requested behavior; a collection
of isolated sequences is not a complete locomotion system. Reuse the project's existing AnimBP,
movement and layering setup. A cinematic may instead need Sequencer. Choose the mechanisms that
serve the result, rather than adding every node to every character.

| Need | Native mechanism and production responsibility |
| --- | --- |
| Continuous changes in locomotion speed or direction | A Blend Space with compatible source cycles; use a single axis when one input is sufficient. Feed actual motion in the intended reference frame, with documented axis units and ranges. |
| Starts, stops, turns, airborne motion or a change of locomotion mode | AnimBP states and transitions with explicit entry, exit and interruption behavior. A continuous blend does not decide when to jump or select a safe stair landing. |
| Walk/run cycles staying in step while blending | Sync Groups and, where appropriate, matching foot-phase Sync Markers; inspect phase changes when the leading animation changes. |
| Breathing, recoil or another independent contribution | An authored additive layer with a correct reference pose and blend weight; use a bone mask where only part of the body should change. Keep breathing independent of locomotion play rate when the performance needs that. |
| Looking/aiming across directions or performing a temporary upper-body action | An Aim Offset or montage slot with the appropriate mask and additive settings; retain the lower-body gait and verify both-hand/prop relationships. |
| Adapting a blended pose to contacts and actual travel | The project's verified foot/hand IK, Control Rig or suitable warping setup, evaluated in the required order after pose contributions that would otherwise break contact. Check the final pose and joint limits. |

GraphCode supports native `kind:blend_space` creation and editing of settings, axes and samples,
including single-axis and Aim Offset classes. Read `gc_describe kind:blend_space` and the relevant
AnimGraph/sequence grammar before writing. This native asset route is separate from `.gcmove`
clip construction; a Move bake does not implicitly generate or connect an AnimBP.

Build compatible samples on the same intended skeleton and coordinate gait phase. Place samples
using their measured motion, inspect intermediate blends and direction wrap, and tune input/weight
smoothing without making controls feel delayed. Inspect blended poses as carefully as the source
clips: interpolation can introduce sliding, clipping or poor weight transfer between good endpoints.
Epic documents [Blend Spaces](https://dev.epicgames.com/documentation/en-us/unreal-engine/blend-spaces-in-unreal-engine?application_version=5.7)
and their intended native playback path.

A one-axis blend space whose samples are looping root-motion clips on one skeleton, when written or dry-run
through GraphCode, carries notes beginning "slide in place:". They first check that each sample sits at its own
clip's speed (within 5%), then say how far each planted foot would slide if a game moved the body at that speed and
played the blend in place, at each sample's own place and the middle between neighbours, and whether the engine
synced by markers or by the share of the loop. "In place" needs the Animation Blueprint's root motion mode Ignore (or
Everything) or the clip's force root lock, not the engine's default. The slide is read on bones, not contact points,
and no asset changes; two-axis spaces and aim offsets get no such note. Example: "at 62.5 cm/s (between A_G1Walk and
A_G1Trot, weights 50% and 50%): the planted feet slide up to 1.9 cm (toe_B_L, over its 0.13 s on the ground)".

Verify marker names, order and group membership on the actual delivered sequences. `clip(source=...)`
copies bone motion and float curves, **not source events or Sync Markers**; author or restore the
required markers/notifies on its output through the supported native asset workflow. Check footstep
events during blends so neither missing nor duplicate events pass unnoticed. See Epic's
[synchronization rules](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-sync-groups-in-unreal-engine?application_version=5.7).

Before delivery, exercise acceleration/deceleration, direction changes, sample boundaries, start/stop,
state interruption and layered actions in the real character. Coordinate stride/play rate with actual
travel and check root-motion ownership. Warping may help with speed or direction coverage when the
installed rig and node support it; it does not supply a missing terrain landing planner. Preserve
the [natural-motion pass](#natural-movement-is-part-of-the-performance) throughout these blends.

### Slopes and stairs: current support

There are separate responsibilities: native movement traverses collision, animation supplies the
gait, and contact/landing adaptation makes the visible body fit the actual terrain. A Blend Space
can blend suitable uphill/downhill or stair clips, but does not sense geometry or choose a foothold.

| Case | Current boundary |
| --- | --- |
| Moving a character over walkable collision | Native Character Movement has slope and step policies. Match its movement mode, collision and travel to the intended course; moving the capsule alone does not establish convincing feet. |
| A walk or a run on a flat floor | Built 2026-10-07: `plant(..., roll=(Heel, Ball, Toe), drive=Root)` keeps each planted foot rolling as drawn with its touching point still, the feet carry the body, and a run keeps its speed in the air. Measured on the example walk: foot within 0.02° of the drawn turn, held point within 0.16 cm. [WALK-AND-RUN.md](WALK-AND-RUN.md). |
| An authored step up and the next step | Demonstrated with declared `ground(z=...)` targets, calibrated sole contacts, authored leg motion and `plant(..., drive=Root)`. Follow [STAIRS.md](STAIRS.md); its fixture values are not a general terrain solution. |
| Reactive runtime ground clearance | `constraints(collision=world, contacts=...)` queries calibrated contact samples and corrects penetration within joint limits. Its `slope=` filters ground normals; current correction lifts along the configured up axis, without solving a desired sole rotation or lowering a hovering foot onto support. |
| A flat walk fitted to stairs at runtime | Measured recipe: capsule on a hidden ramp, foot traces onto the steps, hips held at the touch-down height, back foot settling at rest. Smooth ascent; the back foot floats late in each step. [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md). |
| Automatic walking over changing slopes or arbitrary stairs | Partial. The current mechanisms do not establish a complete terrain-following gait, supported whole-foot landing, persistent stance anchor and clear recovery path coordinated with body progression. Source motion and the final runtime composition still need course-specific acceptance. |

For production, test uphill, downhill, cross-slope, slope breaks, ascent/descent, changed tread/riser
sizes, turns and blocked/missing support as applicable. Check sole support, toe/riser clearance,
knee/hip reach, stance drift, body travel and recovery together. The existing ascending-step example
does not prove descent or automatic adaptation. Do not turn the missing planner into per-stair angle
tables for a request that requires reusable environment adaptation.

Epic's [walkability guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/walkable-slope-in-unreal-engine)
describes the movement policy. Its [Pose Warping documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/pose-warping-in-unreal-engine?application_version=5.7)
explicitly labels Slope Warping as still in development on the project's engine version; do not
present that node's existence as production acceptance. Use a verified native setup for the actual
rig and course, and report the remaining adaptation gaps plainly.

## Inspect and prepare the rig

1. Read the actual body with `gc_read kind:rig`; inspect the mesh's scale, rest pose, bone hierarchy,
   joint axes, chain endpoints and native sockets. A bone's `X` is not necessarily world `X`.
2. Give the joints their intended bend, side and twist ranges. Define these from the character or
   mechanism; do not assign the same human limits to every rig. In a custom `.gcrig`, `range`,
   `side`, `twists` and the bend direction generate native IK settings. Inspect those settings.
3. Ensure every constrained chain has a goal connected to a native solver. Set a useful preferred
   bend for straight fingers, knees or elbows. More iterations cannot repair a wrong bend direction
   or make an unreachable target reachable.
4. Name the matching `ik_rig:` in the Move header even for FK-only clips when its limits should
   constrain the motion. A skeleton's bone names and semantic-axis metadata alone do not supply
   numerical angle limits. Imported rigs need their own calibrated native settings.
5. Test one joint at its lower limit, rest pose and upper limit before composing the performance.
   Check both the bone movement and the visible skin. Bad skin weights require a rig/mesh repair.

The 2026-09-28 implementation applies a supplied full-body IK rig's declared angle limits before
and after solving, including `swing` and bone channels driven by `drive`. Build notes identify
corrected bones and frames. This behavior requires the updated plugin binary. Unconfigured/free
axes remain free; the system cannot infer anatomical limits from a bone name.

Limit projection clips an infeasible rotation to its legal range; in a moving clip a joint asked
on past a stop stays there, never jumping to the other end. It does not ease into the stop: a
held limit then released can make a visible speed change. Check the asked pose and clamp notes,
fix the range/rest mapping or path, and judge playback; slower timing won't fix an unreachable pose.
A limit refusal names the first place a stop is crossed and also lists every stored frame past the stop, as runs
of frames, with their count and the worst excess and the frame it falls on, so one tweak can be judged against
the whole list and not only the first frame.

Add `[readings=(0,last)]` to an animation's build line to inspect its first and final solved keys,
or `[readings]` for every output key. The notes give the IK rig's joint readings in its own words
and signs, chain ends, points/frames, named sockets and achieved plant contacts. Positions are
component-space centimetres; facing is labelled as an `(x,y,z,w)` quaternion. Frames refer to the
final output after timing and refinement. Dry runs report the same solved keys, before compression;
these notes do not measure compressed playback. Requires the updated native plugin (2026-10-13).

**A held thing's direction is its own line, not a chain end** (21 Nov 2026). A chain's `end` line is its end BONE
(`arm_r end` is middle_metacarpal_r and `sword_r end` hand_r on the BlueEdge body). A held part rides its own
carrier (palm_r), and the carrier's own turn (a grip's seat) shows in no chain end: a blade read off a chain end
was 10-12 deg off the blade the build placed. Each `surface(part=)` a bone holds is read on its own:
`held part $Sword (part Sword, on 'palm_r') at (...) facing quaternion(...)`, then
`held part $Sword axes, the part's own, in the character's space: +X (...) +Y (...) +Z (...)` (a sword's blade is
its +Z). Spec: `GraphCode.Move.Readings.HeldPart.BladeFromTheCarrier`. Needs the build after 21 Nov.

For a hand at output frame N, add a named `point(bone=hand_l)` if the exact landmark is not already
reported, select `[readings=(N)]`, and use `gc_apply dry_run:true`. This avoids publishing a test
animation but still solves the full target; it is not a cheap isolated-pose query. Divide a reported
output frame by the solved output fps for seconds, not by the original file's fps after refinement.
A build refused by a body-collision check still reads the asked frames, from the pose it refused:
those notes say `(refused candidate, not built)`, and they describe a rejected pose, not a clip.
Other refusals before the reporting stage (a solve that could not run, a limit refinement that did
not converge) return without readings. For elbow direction, use the [upper-bone aim method](GCMOVE.md#every-gcmove);
an endpoint alone does not choose the bend plane or guarantee continuity at an unreachable target.
If readings or joint/playback warnings were already computed before a later refusal, they are
retained in its explanatory notes (bounded, with a count when more were omitted; refused-candidate
readings carry at most 8 frames, so ask few frames with `[readings=(N)]`).

**Which frame is a pose?** In a clip timed by effort (a `travel` or `accel=`), the poses do not fall on
the frames you wrote: the timing sets the length. The build's timing note says where each pose falls:
"... pose 1 at 0.000 s, pose 2 at 0.627 s, pose 3 at 1.255 s". Multiply a time by the file's fps
(0.627 x 120 = 75) and ask that frame; `last` is always the final frame, so use it rather than count:

    A_Cut = $Cut [readings=(0, 75, last)]

`[as_presets]` (or `[as_presets=(0, last)]`) is read the same way and is read-only too: the key at that frame written again as
preset calls and bone overrides, its other lines kept, the text in the notes ([MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md), part 6).

A dry run prints each as "readings $Cut frame 75 at 0.625000 s" (measured 13 Nov 2026: a 151-frame cut
dry-ran in 5 s). The table of every bone is long and is printed only when the build line asks for it:
take `[readings=...]` off once you have read it.

**How long the editor may spend on a build** is `budget:` on the `gc_apply` call, in seconds; how long
the call waits for the answer is `wait:`. They are two numbers: a wait that ends cancels nothing, and
a budget that runs out stops the job ("Task timed out after N ms; execution has stopped"). Ask a
budget that fits the job (a key with `collision(self)` is 10 to 20 s; a cut with it can pass two
minutes), submit with `wait:0`, and collect with `unreal_info include:["tasks"] task_id:"<id>"`.

## What each implemented instruction does

| Instruction | Use it for | Checks that matter |
| --- | --- | --- |
| `swing(bone, axis=..., by=...)` | A local rotation in degrees; prefer declared `fold`, `side`, `twist` when available | Bone axis/sign, final angle after stacking and inheritance, skin deformation |
| `shift(bone, axis=..., by=...)` | A local translation in centimetres: root travel or an intentional mechanical slide | Axis after preceding rotations, travel distance, root-motion ownership |
| `drive(control, by=...)` | Rig-calibrated articulation or expression, including native float curves | Actual control mapping, combined bone angles, curve playback; a pose control is not a grasp solver |
| `plant(chain, on=$Ground, weight=...)` | Capture and hold the endpoint on a flat ground height. For a walking or running foot use `roll=(Heel, Ball, Toe)`: the foot keeps its drawn turn and its lowest point is held. A plant's IK turns only its own chain: the spine and chest above it are never bent to make a foot reach, and a foot the chain cannot put down falls short and the note says how far. `drive=Root` carries the body from the feet, and a run's body keeps its speed in the air; with `travel()` the body moves at its stated speed and the plants only hold ([WALK-AND-RUN.md](WALK-AND-RUN.md)) | Foot drift, knee direction, weight transfer, activation and release; the foot over a real floor |
| `reach(chain, to=$PointOrFrame, weight=...)` | Put an endpoint at a position; a frame also requests orientation | Position and orientation errors, joint limits, body/arm approach |
| `aim(chain, at=$PointOrFrame, weight=...)` | Turn the chain to look toward a position through its available joint arcs | Direction and residual roll, chain's forward axis, maximum turn |
| `grab(grip, on=$Surface, ...)` | Solve selected contact points onto a static mesh surface | Contact error after limits, digit shape, palm pose, skin clearance and release |
| `fixate(chain, to=$Frame, lock=pose, ...)` | Retain a captured pose relative to a moving target or parent | Reference frame, activation/release, requested channel and final residual |
| `contact(body, on=$Floor, weight=...)` | Acquire normal-only body-surface support during physics baking | Selected native collider, authored physical drive, changing supporting face, acquisition and release |
| `physics(body_or_$Bones, weight=...)` | Let selected bodies yield, then resume following the animation | Default body/joint drives, participation curve, retained limits/collision and recovery |

These motion instructions share optional nested controls. `profile`, `clip`, `bones`, `ground`, `point`,
`frame` and `surface` supply timing, composition and targets. `clip(source=...)` samples an existing
native animation. `release`, `carry` and automatic contact selection remain proposals.
Release a current hold by bringing its weight to zero. Write every `point`/`frame` in the body's own words: a landmark
(`bone=`, `socket=`, another place `target=$X`, or the feet) and distances `at=(up=25, back=5)`, facings `toward=(back,
up=15)` or `toward=lowerarm_r` (an aim takes them too: `aim(bone, toward=(down, left=10))`), measured as `relative=spine_05` (the chest) or `relative=character` faces (a foothold that
stays put); `gc_describe kind:"move" block:"places"`. An unrelated live actor is a different target contract.

For falling, lying on a side/back or changing the supporting body surface, see the
[gravity and body-contact grammar](MOVE-PHYSICS.md). `clip(gravity=true,
collision=$Floor)` bakes native physics against an explicit animation floor; `physics_profile=Name` is an optional override of the character's physical defaults;
`constraints(gravity=true, collision=world)` emits live native Control Rig physics. Pose orientation
and the character's authored Physics Asset drives preserve intent. No Animation Blueprint is
generated. Explicit selected-body contact currently targets the animation floor; its live-world
acquisition binding is still missing.

Angle limits do not define a piston's travel stop or prohibit intentional `shift` animation.
Set mechanical travel in its profile. A root should be moved by one authority during playback:
either the animation's extracted root motion or game logic/Sequencer, not both adding the journey.

### Root travel and visible body height in a game

`[root_motion]` enables native extraction. In the graph, a sequence's root is locked when the Animation
Blueprint's root motion mode is Ignore or Everything, or when the clip's force root lock is on; Unreal then resets the
root transform to its chosen lock pose, including Z. The engine's default mode (montages only) does not lock it, and a
Blend Space alone does not impose that condition. Check the sequence's root settings and the AnimBP's extraction
policy before interpreting a missing jump arc or squat as a solver error.

For an in-place clip whose capsule/game movement owns travel, keep relative body bob, crouch and
pose height on the pelvis rather than on a root that playback removes. Pelvis translation must
also play from Animation: a skeleton's `retargeting { pelvis = Skeleton }` discards those keys;
`reset pelvis` in its retargeting block restores the Animation default. Inspect that skeleton
before editing a shared one. Root-driven plants are not automatically remapped to the pelvis,
and there is no automatic game-clip mode that splits horizontal/vertical motion. Re-solve contacts
after changing the motion owner and check a played frame in the actual game setup.

Every dry run and build plays the clip as the game reads it (a compressed copy, the engine's own pose
path, the body it was solved on, the existing clip's root lock): *in place*, and with `[root_motion]`
also *on a walking capsule* that takes the ground travel and turn, not height. `played height $Clip
... within 0.05 cm` means height and contacts survive; `WARNING played height:` names bone, distance,
frame, planted contacts and cause (the root lock, or a bone's translation retargeting); `WARNING root
motion:` flags root Z off its lock pose. Neither moves a key or a setting. Clips built before
2026-10-21 whose first line was not on the root gave the capsule no root motion: rebuild them.

## Compose movement in stages

Author a readable starting pose, anticipation, main action, follow-through and settle. Distribute
work through the body: a sword swing normally involves the shoulder and torso as well as the hand.
Those choices are authored performance; contact IK does not invent them. For a reach or a grasp, key
poses carry the arm and the reach lands only the last few centimetres; a reach weighted in from the
start hands the route to the solver ([HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md#key-poses-carry-the-arm-ik-lands-the-last-few-centimetres)).

For an automatic starting point, use `swing(arm_R, axis=fold, by=$Arc, follow=auto)` with an actual
IK chain. It allocates turn from chain length and declared joint capacity, letting larger
contributions lead smaller ones. `follow=auto` spreads delays over 12% of source time; `follow=0.2`
chooses 20%. `follow=proximal` keeps `auto`'s shares and spread and orders the delays root first, the
way a strike or a throw moves; the two build the same clip only when every joint's delay already
agrees, not merely when the root leads in both. A scalar becomes a straight rest-to-requested turn in
this mode: the angle is continuous but the speed steps where each joint starts and stops, so give a
profile that rests at both ends for a turn that starts and ends gently. This is a procedural
heuristic, not a balance/muscle simulation or universal human preset. Author the intended path,
anticipation and finish, then refine meaningful moments instead of guessing every joint sample.

Profiles express values over normalized clip time. `frames=48` at `fps:24` is two seconds and
49 stored samples. `swing` values are degrees, `shift` values are centimetres, and hold weights are
fractions from 0 to 1. Use `loop=true` for repeating profiles and phase offsets only where supported.
Use gradual contact weights to avoid a visible snap; do not hide an impossible target with a slow ramp.

Every motion can carry a block of local controls:

```gcmove
swing(arm_R, axis=fold, by=$Arc, follow=auto) {
  at(frame=12, speed=100%)
  at(frame=24, arrive=75%, bone=hand_R, axis=twist, angle=20)
  at(frame=36, speed=200%)
}
```

Frame numbers refer to the original clip. `speed=50%` doubles the time from the previous control
point; `200%` halves it. `arrive=75%` explicitly places a point at 75% of the original duration.
Use one timing form per point. Later arrivals shift, and the longest instruction determines the
output duration. Angle keys move with their owning instruction and refine the final pose after IK;
native joint limits and collision checks remain in force. A lone angle key fades in from the
automatic start pose and fades out toward the end. Endpoint keys stay full at that endpoint.
Use the installed `gc_describe kind:move block:motion_controls` for exact validation and timing.

### Time a motion by effort: `accel=` and `velocity=`

Frame timing says *when* each pose happens. Effort timing says *how hard the body moves*: the
build keeps the motion's path and works out its timing from the joints' limits. A punch at 90%
effort arrives sooner than the same punch at 40%, and neither joint goes past what it was asked.

```gcmove
Raise = profile(0=0, 1=45)
Fast = clip(frames=120) {
  swing(upperarm_r, axis=side, by=$Raise, accel=90%)
}
Slow = clip(frames=120) {
  swing(upperarm_r, axis=side, by=$Raise, accel=40%, velocity=50%)
}
```

- **Where it works:** `swing`, `shift`, `drive` and `aim`. The line moves rest to rest. The joint
  that limits the move (the binding one) reaches the asked acceleration, and every joint stays
  under `velocity=` when it is given.
- **Units:**
  - `90%` is a share of each joint's reference from the rig (see RIG.md, "How fast a joint can
    move");
  - `3000deg/s2` and `600deg/s` are absolute turn rates. On `aim` they are the gaze's turn;
  - `shift` takes only absolute `m/s2` and `m/s` for now.

  A bare number, or a unit of the other kind, is refused with the fix.
- **The rig must declare a `ramp:`,** the quickest a joint builds its acceleration. Without it the
  build stops and names the key. A share also needs the joint's `accel:` (and `velocity:` for
  `velocity=`), or it stops naming the joint.
- **Leave moving, or arrive at a set time:**
  - `at(frame=<last>, velocity=8m/s)` leaves at that speed;
  - `at(frame=<last>, arrive=120%)` fixes the duration;
  - neither may sit before the last frame.
- **Refused by name:**
  - `velocity=` without `accel=`;
  - `speed=` points on the same line (one clock per line);
  - angle points (`bone=, axis=, angle=`) on an effort `shift`, `drive` or `aim` (on a `swing` they
    are timed with the line: see "Pass through a pose without timing it" below);
  - two effort lines on one bone.
- **A travel between poses** takes it as a modifier on the pose reference (2026-10-13): `$Swing [accel=80%]`, and the
  shape of its speed `[pace=(0.25, 1)]` ([POSES.md](POSES.md#travel-between-two-built-poses)).
- **A list of poses timed by effort** (`$Keys [accel=(60%, 60%, 60%)]`: one effort across, rests and passes, momentum, what the
  notes print) is in [MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md#5-timing-a-list-of-poses-by-effort).
- **Not built yet:** `reach` takes no `accel=` yet (it is being built). `grab` and `contact` never take one, and
  neither do `plant`, `fixate`, `balance` and `physics`: a grab or a contact is the last centimetres of a travel between
  key poses, so time that travel (`$Reach [accel=80%]`); for the others, time the line that brings the limb.
- **What the build reports:** a note per line, for example `Fast: swing(upperarm_r) timed by
  effort: 0.807 s at accel 90% (binding upperarm_r: acceleration 90.0%, speed 41%, ramp 0.1 s, 2
  rounds)`. The clip's length follows the solved time.
- **A clip with no `frames=` is timed by its travel** (MOVE-TRAVEL1, 2026-10-20): `Walk = clip() { $Steps [clock=root]
  travel(root, velocity=0.4m/s) }` lasts as long as the travel takes the poses of `Steps`, each reached at distance / speed;
  `travel(hand_r, velocity=8m/s)` does the same for a sword swing ([WALK-AND-RUN.md](WALK-AND-RUN.md) 6a). Every other clip
  still needs `frames=`.
- ~~**A walk's length is its strides' own**~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B2 and B3: the stride rule is removed; the bullet,
  with its earlier 90%-leg form, is moved verbatim to [ANIMATION-DONE.md](../Historical/ANIMATION-DONE.md).)
- **The clip's length and its other lines** (user-ruled 2026-10-14): a clip with a line timed by effort lasts as long as
  its timed motion, and `frames=` is only the scale its other lines are written on. A line with no time of its own (no
  `accel=`, no `speed=`/`arrive=` points: a lean, a head turn, an arm raise, a pose's amount, the clip it starts from)
  is FITTED to that length: it spans the whole clip, however long the solve makes it, so
  `swing(upperarm_l, by=profile(0=0, 1=20))` beside a 0.27 s cut rises over those 0.27 s. A line that states its time
  keeps it, and can make the clip longer (`swing(...) { at(frame=480, arrive=100%) }` holds 480 frames). The note says how
  much the fitted lines were stretched or squeezed: `Cut: timed by effort, it lasts its motion (65 frames, 0.271 s at the
  document's 240 fps), not the 480 written; lines without a time of their own are fitted to it (0.14 of their written
  length)`.
- **A timed line that depends on a fitted one** (it looks at a target a fitted line moves, or reaches from a body a fitted
  line carries) is solved against that line as it plays at the final length: the build solves, refits and solves again,
  searching between two lengths that ask past each other, at most 10 rounds (`gc.Move.LengthRounds`), until the length it
  asks for is the length it was solved on, and says so (`...: timed against the lines fitted to it: its length settled in
  N rounds (fitted->asked frames: ...)`). Settling is about the length; the effort is judged on the built clip.
- **Some of these find no settled length.** A head that looks (timed) at a ball an arm carries, where the arm (untimed) is
  still for 60% of the clip and then drifts: if the look ends before the drift starts it is short, and if the drift starts
  inside the look the look chases it and is long. Fitted to either length, the drift lands on the other side
  (measured 2026-10-14: solved on 184 frames it asks for 361, on 185 it asks for 115), so the search finds no length that settles. The build then says
  its length did NOT settle, builds the round whose length changed least, never certifies it, and names the remedy: give
  the line the timed one reads a time of its own, `at(frame=240, arrive=100%)`, so it keeps its written length and the
  look is solved once against it.
- **To keep a line's written time** in an effort clip, give it its time: `at(frame=<last>, arrive=100%)` keeps the
  written length; `speed=` points keep their pace.
- **A line over its caps is slowed, not refused** (2026-10-13). When the time law cannot settle a joint inside the
  effort (a long ramp, several reversals: a MetaHuman shoulder through six sword poses read 110% speed after 5
  rounds), and the line fixes no duration, the build stretches its whole clock evenly until every joint is
  inside: speed falls by the stretch, acceleration by its square. The note says so: `the effort clock came out over
  its caps after 5 rounds (speed 110.4%, ...); it is slowed by 1.19x to 1.487 s`. Read it as the honest tempo of
  that effort; ask a higher `accel=` if the move must be quicker. A line whose duration is fixed (`arrive=`) is
  still refused when it cannot fit.
- **Under its acceleration is slower, not wrong** (2026-10-13). A move too short for the ramp to build the asked
  acceleration (`too short to reach … at this ramp; peak …`), and a curved path whose correction loop lowered the
  acceleration to stay inside the caps (`the clock stays inside the effort but under the acceleration asked`), both
  build at what they reached and say so. Both used to refuse; the clock they leave is inside every cap.

Effort timing is new in this build. Check a timed clip by playing it, and by the note's binding
joint and shares, before delivering it.

### Pass through a pose without timing it: transition poses

An angle point with no `frame=`, `speed=` or `arrive=` is a transition pose. The motion passes through it, and
its moment comes from the line's effort, so the line needs `accel=`:

```gcmove
swing(upperarm_r, axis=side, by=0, accel=90%) {
  at(frame=0, bone=lowerarm_r, axis=fold, angle=60)
  at(bone=lowerarm_r, axis=fold, angle=120)       # the chamber: no frame
  at(frame=120, bone=lowerarm_r, axis=fold, angle=0)
}
```

- **Order:** poses sit between the timed points written before and after them, in written order.
- **Placement:** each pose's place on the path is its share of the angle travelled (here 60 of 180 degrees:
  frame 40). The build reports where it landed and when the clock reaches it.
- **Name the axis by its job** (`fold`, as the rig declares it) and stay inside the joint's range. An angle past
  the stop is clamped, the build says so, and the clamped path is not the one the effort timed.
- **`angle=` is the joint's own reading, not an amount added.** It counts from rest the way the rig's `range=` does,
  and it REPLACES that turn of the joint, keeping the other two; a swing's `by=` adds to the pose the joint already
  has. So `angle=0` at frame 0 is not "no change": on a MetaHuman upper arm cocked back for a cut it undid the
  arm's whole outward roll. Read the joint's reading in the pose you start from before you key it, and key every
  axis you need of a joint the same way: once its fold was keyed, a shoulder's `side=` on the line's `by=` stopped
  carrying the arm across the body (BlueEdge cut, 2026-10-13).
- **A pass-through pose is not a way to find a pose.** If you do not yet know where the motion ends, build that
  end pose first (step 1 of "Make an action look natural" below).
- **The join:** continuous in acceleration, and it never passes a pose. A joint that reverses at a pose stops
  there for an instant, as a real one must. A joint that keeps its direction passes through at speed.
- **Limits in this build:**
  - `swing` only;
  - one bone and axis per pose;
  - every channel that carries poses needs an angle at frame 0 and the last frame;
  - rest to rest: no `velocity=` exit.
- **Refused by name:**
  - a pose with its own `accel=`;
  - a line with no `accel=`;
  - anchors out of order;
  - the same pose twice (write a hold);
  - poses too close in angle to join in double arithmetic.
- **What is checked:** the joint's TARGET angle, on the installed clock. The bone's composed rotation (the motion
  under it) and the root are not certified yet.

This fragment assumes the rig has a `forearm` bone with a declared fold axis. Replace the paths
and names with the actual asset's declarations:

```gcmove
gcmove 1 skeleton:/Game/Character/SK_Character_Skeleton
         ik_rig:/Game/Character/IKR_Character into:/Game/Character/Animations fps:24
build {
  A_Gesture = $Gesture
}
Elbow = profile(0=0, 0.2=15, 0.55=50, 0.8=20, 1=0)
Gesture = clip(frames=48) {
  swing(forearm, axis=fold, by=$Elbow)
}
```

A child `clip(from=$Parent)` inherits the parent's instructions. Bone rotations/translations
stack in order; a new drive replaces that control, and a new hold replaces the inherited hold on
that chain. A replacement grab replaces the grip's previous selection as a whole. `from` names
another authored clip in the file; it is not a native animation import parameter.

**A walk or a run:** draw the legs in place, plant each foot with `roll=` and let `drive=Root` carry the body;
the full recipe, the rig ranges a gait needs and how to check it over a floor are in [WALK-AND-RUN.md](WALK-AND-RUN.md).

**A crouch, a jump or a landing keeps the feet where they stand.** Prefer `plant()` on the floor with the
rig's sole contacts, so the IK bends the knees as the hips drop. On a rig without sole contacts, work the
leg angles out from the rig's own segment lengths for each frame's hip height, so the ankle stays at its
rest height with the foot flat, and type those. When the depth would need more ankle bend than the joint's
range allows, move the hips back (a `shift` of the root) and lean the trunk forward over them instead, as
a person does; do not push the ankle past its range. Take the body's height curve from physics (push,
free flight at g, landing) rather than easing it by eye: a jump's timing is what the clothing and hair
will answer. Worked example, a standing jump on a GC-rigged human retargeted to a MetaHuman wearing a
simulated skirt (2026-10-06): [CHARACTERS.md §9.2](CHARACTERS.md#92-author-the-body-track) for the
retarget, [GARMENT.md](GARMENT.md#check-simulated-cloth-in-the-fastest-motion-it-will-wear) for the cloth.

## Natural movement is part of the performance

**A production living character needs an intentional natural-motion pass.** Include appropriate
breathing and movement within held poses without waiting for a separate request for polish.
First inspect the source performance: preserve convincing motion already present rather than
adding a second breathing cycle. A conscious freeze, concealed/subpixel movement, a nonliving
subject or a deliberately limited style can justify stillness; record that choice in the brief.
A missing rig control or uninspected clip is a limitation to resolve or report, not an artistic choice.
An isolated solver/contact test may remain still so its evidence stays interpretable; the finished
character must also pass in the complete performance.

### Plan from intent and reference

Describe the character's immediate objective, attention, emotional state, exertion, carried load,
supporting contacts, shot duration and intended camera distance. Identify the action's important
moments and what must remain stable. For example: “An alert character waits with both hands holding
a prop; quiet breathing and a small gaze adjustment remain visible while the grip stays seated.”
This is a performance brief using general controls and constraints, not a new activity-specific verb.

Study moving reference for the subject and state. A moving hold preserves a readable pose with
controlled settling or small changes; it should not drift continually. Plan which body regions
lead, follow and settle instead of giving every joint the same curve or adding random jitter.
See Shawn Kelly's [moving-hold approach](https://www.animationmentor.com/blog/why-all-animators-need-to-master-the-moving-hold/).

| Contribution | Author deliberately | Preserve |
| --- | --- | --- |
| Breathing | Use the rig's supported torso/abdominal articulation or verified deformation controls. Choose timing, size and regional offsets from reference and the current state; a quiet rest and recovery after exertion need different treatment. | Body volume, silhouette, shoulder restraint and contact relationships. Avoid a whole-character scale pulse or automatic root bob. |
| Support and weight shifts | Coordinate pelvis, torso and supporting limbs; transfer support before releasing a loaded foot or hand. | Planted endpoints, plausible support and prop load. Added motion does not establish automatic balance. |
| Anticipation and settling | Shape the preparation, main action, overshoot where appropriate, follow-through and return to a readable hold. | Gameplay timing and the intended pose; avoid slow drift that obscures when an action finishes. |
| Attention and expression | Motivate gaze changes, head adjustments and blinks through attention or acting beats, using the actual face/eye setup. | Readable focus and native face/body ownership. Avoid mechanically timed blinks or a new glance every breath. |
| Hands and held objects | Let free fingers or a relaxed wrist adjust when the acting calls for it; coordinate body, both hands and prop. | Required grip contacts, precision targets and attachment offsets. Constant finger flutter is not a natural grip. |
| Hair, cloth, straps and other flexible parts | Author or simulate lag and recovery appropriate to their material and attachment; distinguish passive response from a creature's intentional tail/ear action. | Collision and the driving performance. Recheck simulations and regenerate affected caches after body timing changes. |

Breathing reference should guide both the curve and the distribution across the body. There is no
universal rate, amplitude, inhale/exhale ratio or mandatory delay between every part. Adapt the
approach to the rig rather than copying another package's scale channels. Jason Martinsen's
[breathing workflow](https://www.animationmentor.com/blog/tutorial-animate-natural-breathing-loops/)
demonstrates reference, staged contributions and loop refinement. For passive secondary motion,
the response should follow its driver and attachment; Drew Adams's
[overlap guidance](https://www.animationmentor.com/blog/follow-through-and-overlapping-action-the-12-basic-principles-of-animation/)
explains that distinction.

### Make an action look natural, step by step

**Worked example (provisional):** [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md) builds a front kick layer by layer with these tools
(timing, weight transfer, the body joining), with the build's refusals and notes read along the way.

What makes a strike, a kick, a throw or a reach read as a person, from most to least important:

1. **How it is timed:** it starts gently, accelerates, arrives and settles. It is not a linear ramp, and it
   does not arrive at the same speed it left.
2. **Lead and lag:** the parts nearer the body's centre start first. Hips, trunk, shoulder, elbow, wrist,
   one after another, is a whip; all of them starting together is a robot.
3. **How much of the body joins:** the trunk turns with the arm, the other arm counters, the head stays
   on target.
4. **Weight:** the body is over the foot that stays down BEFORE the other foot leaves the ground.
5. **Style:** the character's own choices. That is always yours.

Build it in this order, looking at the result after each step:

1. **Key poses first, then constraints and speed in the middle** (user-ruled 2026-10-13; [GCMOVE.md](GCMOVE.md)).
   Build the start, the contact or impact, and the end as poses of their own, one-frame clips in a keys
   `.gcmove`, and render each from the front, the side and above until it is right. A pose that places a hand
   or a held thing is solved once by IK: `reach(chain, from=<its grip>, to=<place>)` for where it is and
   `aim(chain, from=<its axis>, at=<point>)` for which way it points, with the elbow written by swings
   ([HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md#key-poses-carry-the-arm-ik-lands-the-last-few-centimetres)).
   Measure every place you type; a guessed landing sat on the belly and collision pushed the arm out.
   Then the motion reads those poses with `pose(source=)` and travels between them
   ([POSES.md](POSES.md#travel-between-two-built-poses)). Do not look for an end pose by tuning angles inside a
   motion: a sword cut took six builds that way and still ended on the wrong side; its end pose, solved once
   by IK, was right first time ([HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade)).
   A pose the motion only passes through may go on a transition line (above), so the build times it.
2. **Time the main action by effort.** Write `accel=`/`velocity=` on its lines, with fast actions near the
   top of the rig's range and relaxed ones well below it (RIG.md, "How fast a joint can move"). Keep the
   moments that must land on a frame — a foot landing, a hand arriving on a target — as timed poses, and
   put what happens between them on transition lines.
3. **Stagger the chain, proximal first.** Inside one chain, use `follow=proximal` (it orders the delays
   from the chain's root; see "Compose movement in stages"). Across the body, put per-bone `[phase=]` on
   the pose reference: a lead (negative) for the hips and trunk, a lag (positive) for the arm and the
   hand. Lags of a few percent of a short clip between neighbours are enough. Larger values look loose.
   With an `accel=`-timed action, ride its clock with `[clock=]` and lag a line in time with `[phase=]`
   beside it (see `[clock=]` below).
   Read the build's kinematics notes: the peak-speed frames should rise from the centre outward.
4. **Let the body join.** (For a key placed by a `reach`, put `[share=]` on the reach line and let the solver turn the
   trunk, rise the hips and counter with the free arm: [The body joins a reach](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach).
   The group lines below are the hand route, and the override.) Use `[share=]` on group lines that read the action's own
   curve: a little trunk on every joint, a negative share for the counter-arm. The body then moves on the action's own curve in
   CLIP time: changing the curve or the clip's length carries the body with it. When the action is timed by
   `accel=`, give each body line `[clock=<a bone the action moves>]`, so it plays on the action's own clock
   (see the next section).
5. **Move the weight first.** Before a foot lifts, the weight must already be over the foot that stays.
   `balance(…)` with no `weight=` and no `at=` does this itself: the weight starts moving `lead=` before the
   foot lets go (0.3 s unless you say otherwise: `lead=0.5s`, `lead=frame(12)`, or a fraction of the clip),
   is over the middle of the standing foot on the last frame before it, stays there while the foot is up,
   and eases back after it lands. Without that, a plain `balance` cannot cross onto the standing foot in the
   one frame the release leaves, and the build is refused at the release. For your own timing, write
   `weight=$Shift, at=$OverStance` instead (then `lead=` is refused: your shift owns the timing), complete by
   the frame the foot leaves — that is the aim to author. As a starting reference
   for when it begins: measurements put the first shift of pressure under the feet about 0.5 s before
   the heel lifts in an unhurried first step and about 0.3 s at top speed
   ([Farinelli et al. 2021](https://pmc.ncbi.nlm.nih.gov/articles/PMC8543010/), citing the gait-initiation
   literature), and the support leg's pressure moving about 0.3 s before a kicking foot starts, in a
   representative participant ([Moriyama et al. 2024](https://pmc.ncbi.nlm.nih.gov/articles/PMC10876941/)).
   These time the start of the pressure shift, not the moment the whole body arrives over the foot. Tune
   them against your character's body and support. Let the foot's `plant` weight leave only after that.
6. **Anticipate, follow through, settle.** Add a small counter-move before the action (a wind-up, a dip),
   let the end overshoot a little where the material or the effort calls for it, and settle into a
   readable hold. Do not leave the body drifting after the action ends.
7. **Check it like a viewer.** Watch at full speed and at a quarter speed, from the shot's camera and from
   above, with the same lighting each time so the comparison is fair. Then work through:
   - **Contacts:** feet do not slide; a hand stays on what it holds.
   - **Clamps:** the build's "clamped" notes. A joint stopped by its limit is a pose problem, not a style.
   - **Kinematics notes:** peak speed and a candidate velocity step. A step at a seam means two curves
     meet at different speeds.
   - **Reference:** compare against reference footage of a person doing the same action at the same effort.

Common signs it is not there yet:
- every joint follows the same curve at the same time;
- the hand arrives at full speed and stops dead;
- the elbow is still bent when the punch should land, or the knee when the kick should;
- the trunk does not turn with the arm;
- the weight moves only after the foot has already lifted;
- added random jitter is standing in for life.

### The body joins an action: `[share=]` and `[phase=]`

A strike, a throw or a push is done by the whole body: the hips and trunk turn with the limb, the other arm
counters, and the parts nearer the centre move first. Write the body's part as lines that read the ACTION's
curve, so the body moves on that curve in clip time and changing the curve or the clip's length carries the
body with it (with an `accel=`-timed action, add `[clock=]`; see below). **For a key pose placed by a `reach`, the body's
part is not written by hand: `[share=]` on the reach line turns the trunk, rises or sinks the hips and gives the free arm
a counter-placement, and the plants and `balance` carry the weight
([The body joins a reach](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach)). The group lines below are for an action
that has no reach to join (a kick's swing), for what a reach's share does not do (the pelvis), and as overrides.**

```gcmove
Trunk = bones(spine_01, spine_02, spine_03)
Arms  = bones(upperarm_l, upperarm_r)
KickBody = pose() {
  swing($Trunk, axis=fold, by=$Hip) [share=(0.05, spine_01=0.1)] [phase=(0, spine_01=-0.04)]
  swing($Arms,  axis=fold, by=$Hip) [share=(0.25, upperarm_l=-0.2)]   # right arm forward, left arm back
}
Kick = clip(frames=120) {
  swing(thigh_l, axis=fold, by=$Hip)
  $KickBody@100%
}
```

- **`[share=]`, how much:** each bone takes that fraction of the line's `by=`. It is written like `by=`: one value,
  then the bones that differ. A negative share is a counter-move. A bone the share does not name takes 1.
- **`[phase=]`, when, on a curve that does not loop (at most ±0.5 of the clip):**
  - a lag (positive) runs the curve over the rest of the clip and catches up by the end;
  - a lead (negative) finishes that much early and holds;
  - a lag needs a curve that starts at rest, and a lead one that ends at rest, so the joint never starts or stops at
    speed mid-clip;
  - the curve's own stops move with it;
  - on a looping curve `[phase=]` shifts it, as before;
  - on a `[clock=]` line it is in time on the ridden clock instead (below).
- **Fractions add down a chain:** each bone turns in its own space, so three spine joints at 0.05 give about 0.15
  together.
- **Signs follow each bone's own axis**, as the rig declares it.
- **How much the body joins is the pose's amount:** `$KickBody@60%` for a less committed action.
- **Follow an effort-timed action with `[clock=<bone>]`.** Without it a body line reads the curve on clip time,
  while an action timed by `accel=` runs on its own clock, and they come apart. `[clock=thigh_l]` plays the line on
  the derived clock of the earlier `swing` or `shift` with `accel=`/`velocity=` that moves `thigh_l`: the same
  progress at every frame, with the line's own `by=`, `[share=]` and `at(frame=…)` angle points.
  ```gcmove
  Kick = clip(frames=120) {
    swing(thigh_l, axis=fold, by=$Hip, accel=90%)
    swing($Trunk, axis=fold, by=$Hip) [share=(0.05, spine_01=0.1)] [clock=thigh_l]
    swing($Arms,  axis=fold, by=$Hip) [share=(0.25, upperarm_l=-0.2)] [clock=thigh_l]
  }
  ```
  - The clock is a schedule, not a limit: the follower's own speeds are reported against its references, not capped.
  - A `[clock=]` line sits in the clip itself, not in a pose, because a pose's amount is clip time; scale it with
    `[share=]`.
  - The build notes each ride ("swing($Trunk) plays on the clock of line 2").
  - Refused by name:
    - two clocks, or a list;
    - the line's own `accel=`/`velocity=`, or its own `at()` `speed=`, `arrive=`, `accel=` or `velocity=`;
    - an untimed pose (`at()` with an angle and no `frame=`);
    - inside a pose, or on a verb other than `swing` and `shift`;
    - no earlier effort line in the clip moving that bone;
    - a bone timed by a `drive()` or by another `[clock=]` line;
    - a bone that an effort line or a follower of ANOTHER clock already times, in either order. Two followers
      of the same clock may share a bone (2026-10-13): a pelvis that turns and drops, a forearm that bends and
      turns, a wrist that folds and deviates, all on the shoulder's clock.
- **On a `[clock=]` line, `[phase=]` is in time on that clock.** A lag `p` (positive, at most 0.5) holds the joint
  until `p` of the action's duration has passed and plays its whole move in the rest; a lead (negative `p`)
  finishes |p| of the duration early and holds. Write one value, or one value and the bones that differ; a bone an `at(frame=…)` angle
  point turns can be named too.
  ```gcmove
  swing(thigh_l, axis=fold, by=$Hip, accel=90%)
  swing(calf_l, axis=fold, by=$Chamber) [clock=thigh_l] [phase=0.1]   # the knee follows the hip
  ```
  - A lag needs no rest stop on the curve. The build checks how fast the joint would be moving where the lag
    holds it (the curve's slope there times the clock's rate) and refuses it above the rest floor (1 deg/s, or
    0.01 m/s on a shift). An effort clock starts from rest, so a lag normally passes; a speed under the floor is
    allowed and noted at the seam.
  - A lead holds the joint's last value, and an action's clock can land a hair short of its end (2e-14 in the
    build's test). So a led joint's curve must be stored at rest at its end: equal stops from the last one
    before the end on, such as `profile(0=0, .45=60, .9=0, 1=0)`. Any other tail is refused, and the refusal
    reports the difference between the curve's end and where the clock stops. That difference can be tiny, or
    even zero (a tail that moves and comes back is refused too), so the refusal does not prove a visible jump.
    A lead on an action that still moves at its end (`at(frame=…, velocity=…)` there) is refused as well.
  - A lag squeezes the joint's move into less time: its speed rises 1/(1 − p) and its acceleration 1/(1 − p)².
    The build notes both, the seam where the lag ends, and when each angle point is reached with and without
    the phase.
  - How much: in a maximal soccer instep kick the lower leg peaks about 70–77 ms after the thigh
    ([Maastricht study record](https://cris.maastrichtuniversity.nl/en/publications/timing-characteristics-of-body-segments-during-the-maximal-instep/));
    on a 0.3–0.4 s swing with the knee's peak near the middle that is a lag near 0.35–0.45, a rough conversion.
    0.1 is a gentle lag. Not every kick runs from the centre out: a jumping axe kick does not.
  - Refused by name: outside −0.5..0.5, not a number, two phases, a bone the line neither moves nor writes a
    point on, `follow=` on the same line, a looping curve.
- **Limits clamp each follower**, and the build reports it. Plants, grips and reaches are solved after the
  followers, and win.
- **Refused by name:**
  - a share for a bone the line does not move, or one named twice;
  - a per-bone share on a one-bone line (write one value);
  - a share that is not a plain number, or whose product with the curve is not finite;
  - `[share=]` with `follow=`, which divides its turn itself;
  - `[share=]` on `plant`, `aim` and the other goals;
  - ~~`[share=]` on `reach`~~ (SUPERSEDED 2026-10-15, MOVE-ACTION1: on a `reach` the share is the body joining it, not a
    refusal; see [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach));
  - `[share=]` or a one-shot `[phase=]` on a line whose block writes angle points (give those bones a line of their
    own);
  - a one-shot `[phase=]` outside −0.5..0.5, or on a curve moving at the end it holds.

**One pose, its bones in turn.** A pose reference takes `[phase=]` too, per bone:

```gcmove
Cross = pose() {
  swing(spine_01, axis=twist, by=-12)
  swing(spine_03, axis=twist, by=-10)
  swing(upperarm_r, axis=fold, by=60)
  swing(lowerarm_r, axis=fold, by=-110)
}
Punch = clip(frames=60) {
  $Cross@$Drive [phase=(0, spine_01=-0.1, spine_03=-0.05, upperarm_r=0.05, lowerarm_r=0.1)]
}
```

- The trunk turns ahead of the arm and the elbow straightens last, from one `Cross` pose.
- It is the same lead and lag as on a line, applied to the reference's amount curve for each bone that the pose's
  own swings and shifts move.
- Contacts, `drive`, `balance` and angle points stay on the unphased amount.
- Refused by name, beyond the list above:
  - a bone the pose does not move;
  - two `[phase=]` on one reference;
  - a line timed by `accel=` inside a phased pose.
- A `follow=` line's chain is only known at the solve, so names under it are not checked; a name it does not move
  has no effect.
- `Content/GraphCodeExamples/BodyShare_20261010/CrossPhase.gcmove` builds this beside the same pose in lockstep.

### Compose with the actual rig and playback system

1. **Establish the primary performance and contacts.** Inspect the base pose or source clip, its
   root-motion owner, current breathing and native face/physics contributions. Decide which system
   owns each added channel. Resolve a bad base pose or unreachable contact before adding detail.
2. **Calibrate reusable controls on this character.** Use authored rig controls with `drive`, or
   declared bone axes with `swing`/`shift`, for supported contributions. Control names are authored;
   `breath` is not a built-in behavior. Check neutral and maximum poses on the visible skin. A native
   float curve only animates a face or deformation if a verified consumer actually uses that curve.
3. **Author timing and state changes.** Profiles run over normalized clip time, so retiming a clip
   also retimes its breath. Use `loop=true` inside a periodic `profile(...)`, omit a duplicate `1=`
   endpoint, and use `[phase=...]` only with a looping profile where the instruction supports it.
   The build's `[loop]` playback flag alone does not make a profile periodic. One clip drives a
   named control once; an inherited drive with the same name is replaced, not summed. Compose that
   control's intended result into its profile, or use separately calibrated controls for independent
   contributions. Check their combined range. Author overshoot explicitly when wanted.
4. **Include body detail in the contact solve.** Bone `drive` contributions enter FK before
   plant/reach/aim. Put the natural motion in the complete composition and rebuild its constraints
   and collision checks. A contact baked on one pose is not guaranteed after a later native layer
   moves the torso, wrist or pelvis. Where live composition changes the pose, verify the native
   constraint/IK evaluation after the relevant layers, or adjust those layers' influence.
5. **Use native layering when the runtime requires it.** Unreal supports additive poses, bone masks
   and montage slots; inspect the existing AnimGraph and additive reference pose/space, weights,
   curve blending and root ownership. Follow [walk, breathe and act together](ANIMATION-LAYERS.md)
   for the player connections, reference settings and final contact checks.
   `clip(source=...)` currently accepts a non-additive sequence
   on the selected skeleton; it is not a general additive/masked layer import. Do not invent a Move
   `layer()` or `breathe()` instruction, or assume a generated AnimBP. See Epic's
   [blend-node reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprint-blend-nodes-in-unreal-engine?application_version=5.7)
   and [layered-animation example](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-layered-animations-in-unreal-engine?application_version=5.7).

For sustained game idles, plan how breath phase and intensity behave when entering, leaving or
interrupting the state. Avoid restarting the same visible inhale at every action change. Choose
authored variation or stable per-instance variation where repeated characters make synchronization
obvious; do not resample independent noise every frame. A precision hold may suppress visible sway,
while an exerted recovery can increase it and then settle. Make those transitions part of the
performance, using the supported clip or native runtime composition rather than assuming Move
infers fatigue, attention or balance.

### Judge the combined result

Play at the intended speed and camera distance, then inspect close views of contacts and skin.
Compare with the base performance when an added contribution's value is unclear. Check repeated
cycles for pose and velocity continuity at the seam; a matching first/last pose alone can still
jerk. Watch for a floating pelvis, pumping shoulders, sliding feet, rocking props, synchronized
blinks, lost focus and distracting repetition. Keep the amount that serves the acting.

Test the states and transitions the asset will actually use: rest versus exertion, movement versus
held contact, entry/exit, interruption and recovery. In layered playback also check changes in
weight, play rate and relevant LODs; detail disappearing must not change support or cause a pop.
Inspect the saved output with the final face, clothing, hair and props. Record which contributions
are present, which are intentionally restrained, and which remain unverified, with playback evidence.
Successful contact residuals or clean collision checks do not certify a living performance.

## Make a hand grip a sword

**First choice where the hand has skin-hull bodies:** close it with collision forms, seat the prop
by touch and place the thumb by a touch. That route, and the joint-order rules that decide whether
a grip or fist looks right, are in [POSES.md](POSES.md#close-a-hand-grips-and-fists-that-look-right).
The `grab()` route suits named pads on a rig without such bodies.

The step-by-step `grab()` route is in [HANDS.md](HANDS.md#the-grab-route-named-pads-on-a-rig-without-skin-hull-bodies) (moved there 2026-10-20); `fixate` and the body-word places follow.

Fixation is general, not specific to hands: `fixate(wrist_R, lock=rotation)` captures the wrist
chain endpoint's current orientation relative to its native parent. `lock=pose` (default) holds
position and orientation; `lock=position` holds only position. `to=$Frame` captures relative to
another named target, while `relative=component` stays in character component space. Bring weight
to zero to release. Use distinct wrist and digit chains; two constraints cannot own one chain.

For a crouching or turning character, define an aim target on the body in its words,
`point(bone=upperarm_r, at=(right=40, up=10), relative=spine_05)`: it rides the chest at every baked sample, not a frozen
reference-pose position (the build notes what it rides). The same places serve reach, fixate and grab surfaces. Play the resulting sequence with its matching body animation; adapting
to additional live animation layers requires the corresponding runtime IK/AnimBP setup.

## Body collision coverage

Rig builds report native PhysicsAsset coverage and enabled body pairs already overlapping in the
reference pose. Author and inspect colliders for the torso, limbs, hands and fingers that matter
to the motion. A ragdoll with only large limb bodies does not cover individual fingers. Shape
fit and native collision exclusions are part of rig authoring; exclusions should describe intended
joint overlap, not hide an arm passing through the torso.

Move checks final sampled poses and bounded subdivisions between frames against enabled native
spheres, capsules, boxes and convexes, including pure FK clips. An assembled character uses the
selected component's effective PhysicsAsset, including `PhysicsAssetOverride`; a bare skeleton
uses its preview mesh (or the supplied IK rig's preview mesh). A new penetration over 0.05 cm,
or that much worsening of a reference-pose overlap, refuses the bake before writing any output.
The final diagnostic names the clip, frame fraction, seconds, solved fps, both bones and depth. Fix the approach, timing or
collider fit and dry-run again. This rejects an invalid path; it does not automatically plan a
different collision-free reach. Missing PhysicsAssets, absent enabled pairs and unsupported shapes
are reported as unchecked or incomplete coverage. They are not a collision pass.

The refusal's first line is still the first failing pair/sample. Under it, `collision inventory
$Clip (refused candidate, nothing built)` lists every pair past its allowance over the whole clip,
by the same measure and sampling: each pair's deepest depth and where, its first and last failing
frame, and how many samples failed, so the collisions of one clip can be fixed together before the
next dry run. It lists up to 24 pairs and counts the rest; it stops after 30 s and says how far it
got; an interval too fast to sample (over 512 subdivisions) is named, not measured. A clip that
passes takes no inventory. Frame fractions are on the solved output grid, which may be denser than
the authored clip; reported seconds are frame / solved fps, and where the build changed the grid
(a limit refinement doubles it) each frame also names the authored frame at the file's fps. Intermediate contact probes without a sample clock still report
frames alone. The 0.05 cm allowance is already built into this final audit beyond the
rest/compliance allowance, not a user-adjustable graze slider. Contact fitting can use a tighter
numerical tolerance, so its stop depths are not the final audit's threshold. Requested `[readings]`
are given for the refused pose, marked as refused. Collision forms/sliding are available, but do not invent a new
approach path or smooth every graze; [holder changes have additional restrictions](POSES.md#props-that-change-holder).

These are body proxies, not exact deformed skin, clothing, hair, the held prop or environment.
Keep visual checks and native runtime collision/physics where those are needed. Check very fast
motion at an appropriate fps; the bounded audit is not continuous collision detection.

## Hair, clothing and parts that swing

Hair, fur, clothing, cloth that drapes, rigid parts on clothing and swinging bone chains are layers worn on the
body, each moved by its own system: what moves each one, what you author, how to see it moving, wind, `[settle=]`,
live or baked cloth, and what is not built are on their own page, [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md).

## Acceptance before delivery

- Dry-run the complete composition; read correction notes and final contact/orientation residuals.
- Complete the [natural-motion pass](#natural-movement-is-part-of-the-performance): breathing,
  moving holds, attention and secondary response appropriate to the subject and state, or an
  explicit performance reason for restraint. Judge their timing and amplitude in the final shot.
- Play the native output at intended speed and inspect intermediate frames from several views.
- Check maximum bends, thumb/finger opposition, foot contact, self/object intersections, root travel
  and prop attachment. A numerically correct contact can still be a visibly poor grip.
- Check weight zero, independent release, an unreachable target, repeated activation and loop seams.
- Reopen the saved sequence and test the actual AnimBP/montage/Sequencer composition. Include native
  post-process rigs, face, garment and groom where they participate in the delivered character.
- Preserve the source, asset paths and a small recognizable example. State any unverified visual
  or runtime behavior; passing parser tests alone is not animation acceptance.
