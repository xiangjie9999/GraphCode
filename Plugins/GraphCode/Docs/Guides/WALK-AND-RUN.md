# Walk and run: feet that roll, a body the feet carry, a run that flies

This guide makes a walk or a run in a `.gcmove` file where:

- **each planted foot rolls** heel → flat → ball → toes, following the drawn motion while the solver holds
  a declared contact point;
- **the feet carry a body that has its own speed:** with `drive=Root` the planted legs say where the body goes, and the
  body gets there carrying its speed and acceleration, as a person does, instead of stopping at every landing;
- **a run flies:** when both feet are off the floor the body follows gravity's arc from takeoff to landing;
- **you can set the body's speed:** `travel(Root, velocity=...)` moves the body forward at that speed, and the planted
  feet hold while it carries them (2026-10-17; it replaced `speed=` beside `drive=Root`);
- **a four-legged walk is key poses the travel times** (2026-10-20): draw the paws' moments as whole-body stills, plant the paws
  that are on the floor inside them, and `travel(root, velocity=...)` times them; see 6a.

It covers the rig the character needs, the clip, the timing, how to check the result, and what to do when it goes
wrong. Built and measured on 2026-10-07; the numbers below are from the fixtures in
[Examples/WalkAndRun](../Examples/README.md#walkandrun-a-rolling-walk-and-a-first-run). GraphCode does not invent a
gait: you draw the motion and declare contact. Inspect the resulting skin and residuals.

**Split 2026-10-20.** This guide passed 800 lines when the key-pose walk (6a) went in, so the superseded stride-and-foothold bullets of the old four-legged walk (6b) moved verbatim to [WALK-AND-RUN-DONE.md](../Historical/WALK-AND-RUN-DONE.md). Nothing was deleted; 6b keeps its number and what still holds.

## What changed, if you used `plant()` before

- **A four-legged walk is whole-body key poses, timed by `travel()` (2026-10-20).** The stride machinery is gone: no cycle made
  from strides, no 90% leg, no footholds, body height, paw paths or parallel-leg pass of GraphCode's own. Read 6a; the old 6b is
  kept, struck.
- **The body carries its speed (2026-10-12).** Before, at every landing the root went wherever kept the drawn foot still,
  so the body nearly stopped and set off again: on the measured run the forward speed fell from 483 to 215 cm/s in one
  frame at each heel strike, and the run read as careful steps. Now the body moves continuously between the footholds
  you wrote, its speed changing no faster than the rig's `body_accel:` allows; the same run dips from 437 to 415 cm/s
  (5%) and carries on. What you wrote is kept exactly: the footholds, when each foot is down, every drawn joint angle.
  The legs absorb the difference by IK and the build says how far (step 5). `blend=` has nothing left to hide. Every
  clip with `drive=Root` builds differently now: rebuild yours and look at it.
- **Use `roll=` for a walking or running foot, not `contact=`.** An ordinary plant keeps the goal bone's turn from the
  landing frame for the whole stance. On a leg chain that ends at the toe, with the contact at the heel, that turn is
  the heel-strike angle (toes up), so the whole foot floats once it flattens: the shipped rifle walk's planted foot
  stood **6.3 cm above the floor** for that reason. `roll=` follows the drawn turn instead.
- **A landing foot lands where the body has carried it.** With `drive=Root`, a foot touching down used to be caught
  one frame behind the body and the body travelled half a step on that frame; the jump `blend=` had to hide at each
  heel strike was 1.79 cm on the walk and is now 0.41 cm (the step-up clip notes 2.8 cm now; a variant of it was
  recorded at 4.9–5.2 cm before, not the same clip). Every clip with `drive=Root` builds a little differently now:
  rebuild yours and look at it.
- **A body in the air keeps its speed, and falls under gravity** (2026-10-12). The horizontal speed is the takeoff's;
  the height follows the project's gravity between the takeoff and landing frames, not the drawn rise curve.

## 1. Give the rig foot contacts that ride their own bones

A foot rolls over three places, so declare three contacts per foot, **back to front**, each on the bone a real foot
carries it on: the heel on the foot bone, the ball (under the joint where the toes bend) on the toe base, the toe tip
on the toes. `bone=` names that bone; it must be a joint of the chain. Measure the points on the skin at rest (sole
height 0 where the foot stands on the floor).

```gcrig
points {
  heel_sole_r = (-7.550, -8.640, 0.000)    # under the heel
  ball_sole_r = (-7.694, 5.362, 0.000)     # under the ball joint
  toe_sole_r  = (-7.694, 13.200, 0.000)    # under the toe joint
}
contacts {
  Heel_R = contact(chain=leg_R, at=$heel_sole_r, bone=foot_r)
  Ball_R = contact(chain=leg_R, at=$ball_sole_r, bone=ball_r)
  Toe_R  = contact(chain=leg_R, at=$toe_sole_r, bone=toe_r)
}
```

`_L` twins are made for you when the rig mirrors. Without `bone=` a contact rides the chain's goal bone (as before),
and every reader except `plant(..., roll=)` refuses a contact on an inner bone, naming it. To try contacts without
rebuilding the rig, put `points { }` and `contacts { }` in a small adapter `.gcrig` and name it on the clip's
`controls:` header line ([RIG.md](RIG.md#rebuild-order)).

**Give the legs and trunk a person's ranges.** A walk and a run need more than a rig built for standing. Measured
needs (cited curves, 2026-10-07) against what RifleMan, the example character, allowed:

| Joint | A walk needs | A run needs | RifleMan allowed | Write in the rig |
|---|---|---|---|---|
| ankle, pointing the toes (plantarflexion) | −17° | −23° to −31° | −10° | `foot_r [..., range=(-50, 85)]` |
| hip, turning in and out | −13° to +6° | ±13° | locked | `thigh_r [..., twists=(-40, 40)]` |
| trunk counter-turn (over the spine) | ±13° | ±21° | locked | `spine_0N [..., twists=(-15, 15)]` each |
| hip extension (leg behind) | −16° | −29° | −24° in the IK rig | allow about −35° |

A range that is too short does not fail the build: the build note prints `WARNING joint limit: 'foot_l' clamped by up
to … degrees` and the foot stops short. Read those lines.

## 2. Draw the legs in place

> **SUPERSEDED for authoring a gait (user-ruled 2026-10-17, on a cat walk drawn this way: *"This looked more and more like
> robot. Did I ask you to hardcode the curve?"*; chosen: key poses; a four-legged walk is written this way in 6a).** A walk is authored as key poses at its events, then the
> motion between them, as [GCMOVE.md](GCMOVE.md) "Poses first, then constraints and speed in the middle" says for every motion:
> not as per-joint `swing` curves over the cycle. This step is kept as the record until the key-pose walk is proven and written
> here (Docs/GC-MOVE-CAT-PLAN.md).

Write the gait the way a treadmill runner moves: **the body stays put and each planted foot slides back** at the
walking speed. `drive=Root` turns that slide into travel (step 3). Draw hips, knees, ankles and toes with `swing` and
looping profiles; the left leg is the right one half a stride later (`[phase=0.5]`), or its own measured curve.

```gcmove
Walk = clip(frames=34) {
  shift(Root, axis=X, by=$Bob)                   # the body's rise and fall (the Root's up axis on this rig)
  swing(thigh_r, axis=fold, by=$Hip)
  swing(thigh_l, axis=fold, by=$Hip) [phase=0.5]
  swing(thigh_r, axis=side, by=1.5)              # a little step width
  swing(thigh_l, axis=side, by=1.5)
  swing(calf_r, axis=fold, by=$Knee)
  swing(calf_l, axis=fold, by=$Knee) [phase=0.5]
  swing(foot_r, axis=fold, by=$Ankle)
  swing(foot_l, axis=fold, by=$Ankle) [phase=0.5]
  swing(ball_r, axis=fold, by=$Toes)
  swing(ball_l, axis=fold, by=$Toes) [phase=0.5]
  ...                                            # the plants, step 3
}
```

**The foot's turn is yours.** The plant follows the turn you draw, so the roll is as good as the curves: how fast the
heel comes down, when it rises, how far the toes bend. Two things to get right:

- **Draw the sole on the floor by the frame its weight rises,** at the place it should stand, already turned as it
  lands (toes up for a heel strike). The plant holds the lowest point of the foot where the drawing has it, projected
  onto the floor.
- **Lift the heel through late stance, not at the last moment.** A real heel starts to rise well before toe-off. The
  shipped rifle walk's ankle curve keeps the foot flat until 50% of the stride and then turns it 25° in two frames;
  `roll=` reproduces that faithfully, and it looks like a snap.

**Take the curves from measured gait, not from memory.** Published joint-angle data (a walk near 1.3 m/s, a run near
3.5–4 m/s) maps onto a rig like this, once the signs are checked by a render on your rig:

| Measured angle | Rig control | Notes |
|---|---|---|
| hip flexion | `thigh` fold | + swings the thigh forward |
| hip adduction | `thigh` side, negated | the rig's + side is outward |
| hip rotation | `thigh` twist | needs `twists=` in the rig; check the sign |
| knee flexion | `calf` fold | |
| ankle dorsiflexion | `foot` fold | pointing the toes is negative |
| toe (MTP) bend | `ball` fold | stance data only in most sets |
| lumbar extension | `spine_01..03` fold, negated, shared between them | the rig's spine folds forward |
| pelvis rise | `shift(Root, axis=<up>)` | centimetres |
| shoulder flexion | `upperarm` fold, after lowering the arm | |
| shoulder adduction | `upperarm` side = 90 + adduction | on a T-pose rig: side 90 brings the arm down |
| elbow flexion | `lowerarm` fold | |

**Check every sign with a one-pose test before mapping a whole curve.** A two-frame clip with, say,
`swing(upperarm_r, axis=side, by=80)` and `swing(upperarm_r, axis=fold, by=40)`, rendered from the front and the side,
showed on RifleMan that side +80 brings the T-pose arm down and fold + then swings it forward.

**Make the curve loop.** Measured curves rarely end where they start. Spread the end-to-start difference linearly
over the stride and drop the duplicate `1.0` key; say in a comment that you did, since it is an authoring change, not a
measurement. Keep the published keys and write `[linear]` after the profile to keep their shape.

**A looping profile needs few stops, if they sit on the curve's turning points.** A body curve that peaks and dips
once a stride is smooth on two to six `profile` stops when each stop is at a peak or a dip (the curve is flat there).
A stop on a slope breaks the acceleration at that stop, and the usual cure, many more stops, is not needed. Put the
stops at the turning points first, then add one only where the curve is still off the take. Measured on the human
walk (probe `HumanWalkStep3KnotProbe.gcmove`): every body cycle came within 0.5 cm and 1.2 degrees of the take.
Ask the `profile` help for the loop and smoothing options.

## 3. Plant the feet with `roll=`, and let them carry the body

```gcmove
Floor = ground(z=0)
OnGroundR = profile(0=1, 0.6=1, 0.62=0, 0.98=0, loop=true)          # heel strike to toe-off
OnGroundL = profile(0=1, 0.1=1, 0.12=0, 0.48=0, 0.5=1, loop=true)   # the same, half a stride later
Walk = clip(frames=34) {
  ...                                                                # the legs, step 2
  plant(leg_R, on=$Floor, roll=(Heel_R, Ball_R, Toe_R), drive=Root, weight=$OnGroundR)
  plant(leg_L, on=$Floor, roll=(Heel_L, Ball_L, Toe_L), drive=Root, weight=$OnGroundL)
}
```

What each part does:

- **`roll=(Heel, Ball, Toe)`** names 2 to 6 contacts of the chain, back to front, in place of `contact=`. While the
  plant weighs, the foot keeps the turn the clip draws, heading included, and **the lowest of the named points is held
  still on the floor**. When another point comes lower, the hold passes to it at the place the two cross, so the heel
  comes down, the foot stands and the heel rises over the ball and toes. These are discrete
  contact targets; sampling and joint limits can leave error. A solved point does not certify the whole sole.
- **The toes below the foot keep their drawn bend.** For a rolling plant the solve puts the leg's IK goal on the
  shallowest bone a roll point rides (the foot), only inside the build; the saved IK rig is unchanged.
- **`drive=Root`** makes the root the body's: the planted legs ask where it goes (one foot alone, two shared), and the
  body travels there **with a speed of its own**. It is solved once over the whole clip: position and velocity continuous
  at every touchdown and takeoff, horizontal acceleration within the rig's `body_accel:`, feet pushing up and never
  pulling down, and never further from a planted foot than the leg reaches. Each foot lands where you drew it, at the
  frame you drew; the body may pass a few centimetres from where the drawn legs alone would have put it, and IK bends
  the legs to make up the difference.
- **`weight=`** is the support timing: 1 while the foot bears weight, 0 in the air. Take it from heel strike and
  toe-off (or from measured force events: support while the vertical force is above about 20 N). The weight rising is
  the touchdown; zero is the release.
- **`blend=`** has nothing to do on a driven root any more: the body never jumps, so the build says *"blend= has nothing to
  do on the continuous body-state track"*. It is still read, and harmless.

`roll=` is refused together with `contact=`, with a sole ring (`contact=(…)`), and on a `surface()`: it plants on a
`ground()` plane. A chain has one IK goal, so a plain plant and a rolling plant cannot take turns on one leg in one
clip; use `roll=` on both.

## 4. A run: let the body fly

A run is a walk with an air phase: both weights are 0 for part of the stride. Nothing extra is written.

```gcmove
Run = clip(frames=42) {                                   # one stride, 0.70 s at 60 fps
  shift(Root, axis=X, by=$Rise)                           # the body's rise and fall, measured
  ...                                                     # legs, trunk and arms, step 2
  swing(thigh_r, axis=side, by=4)                         # step width (see "When it goes wrong")
  swing(thigh_l, axis=side, by=4)
  plant(leg_R, on=$Floor, roll=(Heel_R, Ball_R, Toe_R), drive=Root, weight=$OnR)   # 0 to 0.373
  plant(leg_L, on=$Floor, roll=(Heel_L, Ball_L, Toe_L), drive=Root, weight=$OnL)   # 0.499 to 0.860
}
```

- **While no planted foot weighs, the body is ballistic:** its horizontal speed is constant at the takeoff's, and its
  height is a gravity parabola (the project's gravity, Project Settings > Physics) between the solved takeoff and landing
  heights over the frames your weights leave in the air. Nothing is written for it.
- **Your rise curve shapes the stance, not the flight.** `shift(Root, axis=..., by=$Rise)` is what the body is asked to
  follow while a foot is down; in the air gravity decides. The build names each flight's apex and takeoff speed beside
  what the drawn legs asked (step 5).
- **Use 60 fps** (`fps:60` in the header) for a run: its contacts are short.
- **A run is not a sped-up walk.** Its stance is shorter, its knee and ankle angles are larger and its arms are bent;
  draw it from run data.

## 4b. How hard the body can push, and a pace you ask for

Two references on the `.gcrig` header say what this character's body can do, as `velocity:` and `accel:` do for joints:

```gcrig
gcrig 1 ...  body_speed: 400    # cm/s: its normal full-effort running speed (a measured 3.96 m/s run)
             body_accel: 450    # cm/s^2: how hard its feet can speed the body up or brake it (maximal braking ~445)
```

- **`body_accel:`** bounds how fast the body's horizontal speed may change wherever plants drive the root. Unwritten, it
  is unbounded and the build says so. Rebuild the rig after adding it.
- **`body_speed:`** is what a share in `travel()` is a share of. Unwritten, a share is refused by name; an absolute
  `4m/s` still works.

`travel(Root, velocity=...)` moves the body at a speed, in the clip, once (user-ruled 2026-10-17; `speed=` and a
`drive=Root` beside it were the old form and are refused by name):

```gcmove
Pace = profile(0=0, 0.4=1, 0.8=1, 1=0)        # stops are shares of body_speed: (1 = 100%): start, run, stop
StartRunStop = clip(frames=240) {
  ...
  plant(leg_R, on=$Floor, roll=(Heel_R, Ball_R, Toe_R), weight=$OnR)
  plant(leg_L, on=$Floor, roll=(Heel_L, Ball_L, Toe_L), weight=$OnL)
  travel(Root, velocity=$Pace)                  # or velocity=80%, or velocity=4m/s
}
```

- It moves the top-level root forward (+Y, the character's facing) by the speed integrated over the clip, before the
  legs are solved; the plants then hold each foot where it landed while the body carries the leg over it. Sideways sway
  and the height stay the drawing's. It needs the clip's `ik_rig:`; a second `travel`, or a plant's
  `drive=` beside it is refused by name. `clip(from=)` inherits it, a variant's own `travel` replaces it, `repeat=` and
  `frames=` keep its units. ~~Another bone is refused too.~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B4: `travel(<any bone>,
  velocity=)` is allowed; only the top-level root's travel carries the body, another joint's times the poses that follow it: 6a.)
  A `$profile` speed like `$Pace` is for a clip like this one; a `pose(steps)` that follows the travel needs ONE fixed speed (6a).
- ~~A stride that should keep step with it follows its clock: `$Stride [clock=Root]` plays the stride's poses as the root
  covers the distance, and a plant with no `weight=` holds from the stride's landing to its lift-off.~~ (SUPERSEDED
  2026-10-20, MOVE-TRAVEL1 B3: GraphCode's stride rule is removed. The poses a travel times are whole-body stills with their
  plants written inside them, and the travel times them by distance / speed: 6a.)

**Write a speed that agrees with your strides.** The stride you draw already has a speed: stride length over stride
time (a measured run: 296.5 cm in 0.70 s, 424 cm/s). A speed that contradicts the strides asks the planted legs to
stretch where they cannot, and the plants' notes say how far each foot ended from its hold.

## 5. Build, read the note, and look

`gc_apply` on the `.gcmove`. The note says what happened; read these lines every time:

| The note says | Means | Good looks like |
|---|---|---|
| `plant 'leg_R' rolls on (Heel_R, Ball_R, Toe_R): 5 stance(s); the hold passed Heel_R→Ball_R at frames 3, …` | where each foot handed its weight from one point to the next | heel → ball → toe once per stance; a rocking back and forth near flat is the drawing hovering at flat, and is harmless |
| `at full weight a held point ended at most 0.01 cm from its place` | how far the IK left a held point from where it was put | under 0.2 cm |
| `the lowest point went 0.00 cm under the floor` | the deepest any named point went | under 0.05 cm |
| `plant 'leg_R': lowest actual rolling skin … cm above the floor (… cm under); N vertices with their own bone weights` | the deepest point of the foot's real sole skin, every vertex with its own weights, after the solver seated it on the floor (the named points above can stand on the floor while the heel's skin behind them dips under) | 0 cm under |
| `body state …: accepted by the physical/KKT certificate after the …-iteration budget (…)` | the body's solve reached its iteration count and its answer was then checked and proven optimal | nothing to do: the motion is the solved one |
| `$Run4 body: 8 stance(s), forward speed landing/lowest/take-off in cm/s: frames 42-57 439/415/419 (dip 6%); …` | how much each stance slows the body | a run dips a few percent per step; a large dip means the drawn stride brakes |
| `$Run4 body: 8 flight(s) under gravity 980 cm/s^2: frames 15-21, apex -5.4 cm, take-off 44 cm/s up (the drawn legs asked 66); …` | each air phase, its apex and takeoff | steady from stride to stride |
| `$Run4 body: touchdowns (frame, chain, cm the foot landed ahead of the body): 0 leg_R +33.7, 21 leg_L +33.6, …` | where each foot landed against the body | a run strike lands about 27-36 cm ahead; much more is an overreaching stride |
| `$Run4 body: it ends … cm past the midpoint of its last held footholds, … ; largest fixed-hip reach excess … cm; peak sampled vertical support 2.28 g` | where the body ends against its feet, how close the legs came to their reach, the hardest push | a stop ends near 0 cm; a measured run peaks around 2.5 g |
| ~~`$<clip> travel (line N): the pace was not reached at N sample(s); the largest shortfall is S cm/s at frame F (asked A, attained B): …`~~ | (SUPERSEDED 2026-10-17: `travel()` carries the root at exactly its speed and no build writes this note. Its note is `$<clip>: travel(Root) carries the root N cm forward over the clip (N cm/s on average); the legs are posed and planted on that body`) | the legs reach their holds: read the plants' notes |
| `plant drive: body_accel is absent; horizontal acceleration is unbounded` | the rig states no `body_accel:` | add it to the `.gcrig` header (step 4b) |
| `WARNING joint limit: 'foot_l' clamped by up to … degrees` | the drawn motion asked for more than the rig allows | widen the rig's range to a person's (step 1), or change the curve |

**Then look at it over a floor.** An animation preview has no floor, so a floating foot looks fine there. Put the
character and a floor in a Blueprint (a scaled `/Engine/BasicShapes/Cube` whose top is at z=0) playing the `repeat=`
take, and render a straight side view, orthographic so pixels are centimetres:

```json
{ "asset": "/Game/Your/BP_GaitViewer", "at": 0.333,
  "camera": { "azimuth": 0, "elevation": 0, "projection": "orthographic", "ortho_width": 60,
              "anchor": { "component": "Body", "bone": "foot_r", "radius": 30 } } }
```

At 60 cm across 1024 pixels, one pixel is about 0.06 cm: the gap between sole and floor reads directly. Then render a
side sheet over one stride (`at`, `to`, `count`) to see the roll; over a span a bone anchor is followed, so the foot
stays in the picture and the floor moves under it (`camera_followed`; anchor `{component:"Body"}` alone to hold the
camera still). Check:

- the heel comes down, the foot stands flat, the heel rises over the ball, the toes stay down until toe-off;
- no planted part of the foot slides along the floor;
- the body moves on smoothly at heel strike (no dip or pop) and, in a run, through the air;
- the swinging foot clears the other leg and the floor.

## 6. Make the walk look human

A planted foot is only one part of a walk. The body must continue over that foot while the
other leg recovers. The pelvis, chest and arms move together. A contact solver can hold the
foot accurately while the performance still looks stiff. Author those motions on one clock.

In `swing(calf_r, axis=fold, by=$Knee)`, `Knee` supplies the angle in degrees at each phase.
The `swing` line does not need a second angle parameter. A `plant` alone holds a contact;
it does not invent the recovering leg's knee bend, toe motion or opposing arm swing.

### Establish the phase before drawing the angles

Use one foot's initial contact as phase zero. A stride ends at that same foot's next contact;
it contains two steps. The following clinical timing describes ordinary level walking.
It is a reference, not a universal timing for every speed, age or person.
(Kim, Gallagher and Speckman, 2025, [Biomechanics of Normal Gait](https://now.aapmr.org/biomechanics-normal-gait/).)

| Phase | Stride % | What the viewer should see |
|---|---:|---|
| Initial contact | 0–2 | The heel arrives in front of the body. |
| Loading response | 2–12 | The forefoot lowers and the knee yields as weight transfers. |
| Mid stance | 12–31 | The body passes over the supporting foot. |
| Terminal stance | 31–50 | The heel rises while the body continues forward. |
| Pre-swing | 50–62 | The opposite foot accepts weight; the trailing knee bends and toes release. |
| Initial swing | 62–75 | Knee flexion shortens the recovering leg. |
| Mid swing | 75–87 | The foot passes forward with clearance. |
| Terminal swing | 87–100 | The knee extends for the next contact. |

Do not insert a pause between those phases. Double support is a transfer while moving.
Heel rise starts during terminal stance; it is not a last-frame ankle kick. Clinical angle
landmarks below use the source's convention. Hip angles against vertical are not automatically
the same as hip angles against the moving pelvis.
(Godges, *Normal Gait Mechanics*, after Rancho Los Amigos, 1993, and Perry, 1992,
[clinical handout](https://scal-pt-residencyfellowship.kaiserpermanente.org/wp-content/uploads/2008-Resources/foot-region-docs/03GaitMechanics&DeviationsSummary.pdf).)

| Moment | Hip | Knee | Ankle / toe |
|---|---|---|---|
| Initial contact | About 25° flexion | Extended | Ankle neutral |
| Loading | Still forward | About 15° flexion | About 10° plantarflexion lowers the forefoot |
| Late stance | Maximum 20° extension | Near full extension | 10–20° dorsiflexion |
| Pre-swing | Returning forward | About 40° flexion | About 20° plantarflexion; first MTP about 65° extension |
| Initial swing | Advancing | About 60° flexion | Recover for clearance |
| Terminal swing | About 25° flexion | About 0° | Prepare the next contact |

These landmarks do not define a complete curve. Do not interpolate unrelated peaks and call
the result measured gait. A laboratory take can differ: the cached OpenSim walking example
reaches 72.2° knee flexion at 73% of the contact-normalized cycle, while the clinical reference
above uses about 60°. Preserve the chosen source, its clock and its angle convention.
(OpenSim contributors, 2020, *Moco example3DWalking*, values extracted from the
[pinned example data](https://github.com/opensim-org/opensim-core/tree/9432f35ebde0ad0c619af02d308b151d010eadb2/OpenSim/Examples/Moco/example3DWalking).)

### Put the body and arms on the same clock

Let the pelvis rise and fall as the body passes over each support, and move laterally with
weight transfer. Do not make it a fixed-height block carried by independent legs. Keep pelvis
rotation separate from chest rotation relative to the pelvis. In one walking cohort, preferred
speed pelvis rotation and obliquity ranges were 14.1° and 6.8°. These are total ranges, not
amounts to add in each direction or to every spine joint.
(Pietraszewski, Winiarski and Jaroszczuk, 2012, *Three-dimensional human gait pattern*,
[reference data for normal men](https://actabio.pwr.edu.pl/fcp/aGBUKOQtTKlQhbx08SlkTUARAUWRuHQwFDBoIVURNWHtQFVZpCFghUHcKVj0GQUw/302/public/publikacje/v14-3-2012/42.pdf).)

Each arm normally advances with the opposite leg. Check the visible hand path after lowering
the arms from a T-pose and applying shoulder rotation and elbow flexion together. Copying an
elbow angle alone can leave both hands held forward throughout the walk.
(Collins, Adamczyk and Kuo, 2009, [Dynamic arm swinging in human walking](https://pmc.ncbi.nlm.nih.gov/articles/PMC2817299/).)

The head should not inherit every exaggerated chest nod. Author its intended orientation
through the available neck/head joints; see [HUMAN-GAZE-AND-HEAD.md](HUMAN-GAZE-AND-HEAD.md).
Breathing can use native animation layers, as described in [ANIMATION-LAYERS.md](ANIMATION-LAYERS.md).
Neither extra noise nor breathing repairs a stopped root or an incorrect footfall.

### Choose who moves the body

`drive=Root` derives travel from the drawn supporting legs, and the body carries its own speed
between them (step 3), so a landing no longer stops it. In double support, incompatible leg
curves still ask for different travel: the body takes a smooth course between them and the legs
absorb the rest by IK, which the notes report as a speed dip or a foot landing far ahead. Fix the
drawing; the body state does not coordinate the source gait or choose a human walking speed.
`travel()` moves the body at a stated speed instead (step 4b); its plants hold and never drive.

An alternative is to author the root's forward travel and body motion with `shift`, then use
rolling plants **without** `drive=Root`. The root supplies the journey; IK fits the legs to
the acquired floor contacts. This requires compatible stride length, hip height and reach.
Large corrections, locked knees or missed contacts mean the authored motion needs adjustment.

For a multi-stride take with explicit travel, author its leg/body profiles over all those
strides and give the root one continuous travel ramp over the whole take. `repeat=` restarts
every profile, including the selected clip's own translation instructions. Repeating a travel
ramp can send the root back to its start. Do not treat `repeat=` as a root-motion accumulator.

```gcmove
# Forward4 is an authored travel profile over this entire four-stride take.
# Add the leg/body profiles spanning four strides and rolling plants without drive=Root.
# This rig's local Root Y points backward; check your own root frame.
Walk4 = clip(frames=276) {
  shift(Root, axis=Y, by=$Forward4)
}
```

### Diagnose stiffness in this order

1. Check the source walk on a level floor before terrain correction. Plot successive root
   positions against time. A speed discontinuity already in the source needs a source/drive fix.
2. Check contact events against the leg motion. Rephasing the angles without rephasing support
   weights makes the foot hold during the wrong part of the cycle.
3. Check knee yield, heel rise and toe release. A flat sole followed by a sudden kick reads as
   a rigid lever even with perfect contact. Toe motion must be authored when the source lacks it.
4. Check rest-relative control angles, whole-body motion and rig proportions. `swing(..., by=)`
   is travel from the rig's reference pose, not an absolute anatomical joint angle.
5. Inspect at normal speed and across the loop, with the whole body and a visible floor.
   Check skin clearance during swing as well as stance. Calibrate contacts to the visible
   sole. A heel partly weighted to toe bones moves differently from a rigid heel contact;
   adding toe bend can reveal a skinning problem even when the named points pass.
   More stored frames cannot repair a
   wrong phase, a repeated travel ramp or an unreachable pose.

For terrain, apply [ankle-limited surface alignment](SURFACE-ALIGNMENT.md) to explicitly
selected stance feet. It can leave a heel or edge lifted. It does not improve the source
walk's rhythm, select future footholds or supply body balance.

## 6a. A four-legged walk: whole-body key poses, timed by the travel (2026-10-20; the cat, `Content/__MCPTests/CatMove/Intent/CatWalkPosesIntent.gcmove` and `CatWalkTravel.gcmove`)

> **A looping gait (walk, trot, gallop) is written as 6c since 2026-10-31**: one leg cycle per leg, its poses as rules the build
> solves. 6a stays the way to key a one-off move through whole-body stills, or one joint at a set speed (step 6).

**The goal:** draw the cat at each paw's moment, plant the paws that are on the floor in each drawing, and say how fast the
body goes. The walk is those drawings joined in order, and what lies between them is interpolated (user, 2026-10-19:
*"declare the poses and travel matches them. Basically travel links first pose and last pose and interpolate everything in
the middle"*). GraphCode makes no stride, no foothold, no leg path and no body height of its own. This replaces the stride
rule of 6b, which is struck below. The human walks above are unchanged.

### 1. Draw a key still for each paw's moment

A lateral-sequence walk has four moments, one for each paw landing in turn (left hind, left fore, right hind, right fore);
add keys where the head or tail must move (user, 2026-10-19: *"4 poses, each leg one, even more if head needs to move"*).
Each key is a one-frame clip:

- every paw on the floor is **reached** to its place on the floor, said as a share of the leg (user, 2026-10-19: *"why cant author says how each
  leg plant its feet?"*);
- the paw that curls in the air is reached to a place above the floor, with its folds;
- the **body is written in the key** (step 2).

Draw a paw ahead of its hip or blade in the key where it lands, under the body in the next, behind it in the next, and
curled in the fourth. That drawing is what gives the body its distance (step 5): a paw drawn at the same place in two keys
that it is planted in tells the body nothing.

```gcmove
Floor = ground(z=0)
LandL = point(bone=hip_L, at=(forward=25%), of=leg_B_L, relative=character, on=$Floor)    # the hind paw lands 25% of its leg ahead of the hip
UnderBladeFR = point(bone=scapula_R, relative=character, on=$Floor)                       # the fore paw stands under its shoulder blade
Key1 = clip(from=$Walking) {                  # $Walking is a one-frame clip: the rig as it stands
  shift(pelvis, toward=down, by=3.5)          # the body, step 2: hips low as the left hind takes the weight
  swing(pelvis, axis=fold, by=-6)
  swing(spine, axis=side, by=-3)
  swing(chest, axis=side, by=3)
  swing(neck, axis=fold, by=-6)
  reach(leg_F_L, contact=Pad_F_L, to=$PassFL)  # the left fore curls: passed under its blade, clear of the floor
  swing(wrist_L, axis=fold, by=-30)
  swing(paw_F_L, axis=fold, by=-25.6)
  reach(leg_B_R, contact=Pad_B_R, to=$PushR)   # the right hind is behind, about to lift
  reach(leg_B_L, contact=Pad_B_L, to=$LandL)
  reach(leg_F_R, contact=Pad_F_R, to=$UnderBladeFR)
}
```

The reaches stand each still. The plants come later, in the walk file, because one chain cannot be both reached and planted
in one clip. Places stay shares of the leg on the floor (see the footholds bullets in 6b, which still hold); an author never
researches papers for them (user, 2026-10-19), pick them by eye and look at the render.

### 2. The body is yours, written in each key

GraphCode stands no body for the keys (user, 2026-10-20: *"author says it"*). Each key writes its own dip and rise (the
pelvis `shift` and fold), its sway (`spine` and `chest` `axis=side`, one against the other so the chest keeps its heading)
and the neck against the tip of the pelvis so the head keeps its heading. Between the keys it is interpolated. Write a number
once: needing to tune it again and again to look natural is a fault in GraphCode to report, not an authoring loop (user,
2026-10-20). The body decides what the legs must reach, so a key with its hips too high asks a paw for a place its leg cannot
reach, and the build says how short it fell.

### 3. Read the keys back, plant the paws that are on the floor, and travel

```gcmove
Floor = ground(z=0)
K1 = pose(source=/Game/__MCPTests/CatMove/Intent/A_CatKey1, from=reference) {
  plant(leg_B_R, on=$Floor, roll=(Sole_B_R, Pad_B_R))
  plant(leg_B_L, on=$Floor, roll=(Sole_B_L, Pad_B_L))
  plant(leg_F_R, on=$Floor, roll=(Palm_F_R, Pad_F_R))
}
# K2, K3 and K4 the same way: each plants the paws on the floor in its key
Steps = pose(steps) { $K1, $K2, $K3, $K4, $K1 }
Walk = clip() {
  $Steps [clock=root]
  travel(root, velocity=0.4m/s)
}
```

built with `A_CatWalkTravel = $Walk [loop, root_motion, readings]`. That is the whole walk.

- **`pose(source=..., from=reference)`** is the key's last frame. A whole-clip `pose(source=)` sets every bone its clip has a
  track for, rest included, so a key takes every bone back from the key before it (a paw left at rest in a key is not left
  curled by an earlier one). **A walk measured from a video is keyed the same way, from moments of ONE take:**
  `pose(source=<the take in place>, at=frame(7), from=reference)`, at each foot's landing the take's report names, with the
  feet down then planted inside (MOTION-FROM-VIDEO.md, "Using the clip").
- **Every pose of the list must be a `pose(source=)` picture** of a still: a `pose() { }` written in the file is refused by
  name, because the body's distance is read off the drawn picture.
- **The list ends on `$K1` again.** That closes the loop: the last stretch goes from K4 back to K1.
- **`[clock=root]` and `travel(root, ...)` name the same bone**, exactly (case included). A travel of the root needs the
  document's `ik_rig:`.
- **Write no `frames=`, no `[phase=]` and no `[pace=]`.** The travel times the poses and so sets the clip's length; a
  `[pace=]` or `[phase=]` on this reference is refused by name.
- **One `travel()` per clip, written in the clip** (not in a pose), and no plant beside a root travel takes `drive=`.
- Build `[loop, root_motion]` for a game, whose character movement then takes the travel; without `[root_motion]` for a render.

### 4. What a plant does inside a pose

A run of consecutive poses that plant one chain is **one hold** (user, 2026-10-19: *"Plant is in the clip of the leg itself,
not inside the general walk"*, *"Or it can be inside a pose"*, *"The paw is on the floor until when it curls, it naturally
puts itself to the next pose"*):

- it takes hold at the run's **first** pose, holds through its **last**, and lets go across the stretch into the next pose,
  where the paw goes to the place that pose draws it, interpolated like everything between poses;
- on the cat each paw is planted in three keys (landing, under the body, behind) and curls in the fourth;
- a list that ends on the pose it starts with joins the run across the seam;
- where the lines of a run differ, the line in the run's **last** pose is kept, and of two plants of one chain in one pose the
  later one: **order settles every conflict, never a refusal** (user, 2026-10-19: *"order overwrite any conflicts"*);
- a plant written in a clip with no `pose(steps)` that follows a travel (a human walk's, with `weight=`) is untouched.

### 5. How the body's distance and the times come out

- **The body's distance over a stretch is not written.** It is how far the paw planted at BOTH ends of the stretch is drawn
  apart, measured forward of the root in the two keys: that paw stays on its print while each key draws it at another place
  under the body, so the body went by the difference. Where several paws are planted at both ends, the plant written
  **last in the later key decides** (user, 2026-10-19: *"Order is important. Follow the last line"*), and the other paws stay
  on their prints with their legs making up the difference. The cat's file writes the paw under the body last in every key:
  it landed in the key before and now carries the weight, while the paw behind is letting go, so its leg is the one that
  gives. On the cat's keys the four stretches come to about 6.6, 9.7, 6.6 and 9.3 cm.
- **The paw is measured at the point of its `roll=` that landed first**, the same point in every key of its run. This is a
  reading of the user's *"depend on what landed first?"* (2026-10-19), to be confirmed.
- **`shift(root, ...)` is not needed** (user, 2026-10-19: *"I assumed we don't have to declare shift root"*) **and is not read**:
  `travel()` carries the root already, so a `shift(root)` in the keys would move it twice. A stretch that no plant holds at both
  ends is refused by name; a `shift(root)` as the fallback is not built.
- **The time.** Pose k+1 is reached at distance / speed after pose k, from 0, and the clip lasts their sum, the closing
  stretch of a loop included. A stretch whose deciding paw is drawn no further back in the later key is refused (the body
  would go back or nowhere).
- **The speed is one fixed speed, starting at full speed** (user, 2026-10-19: *"implement fixed speed"*, *"start speed =
  speed"*). Write `0.4m/s`, or `80%` of the rig's `body_speed:`; a `velocity=` that varies over the clip is refused by name.
- **In a `[loop]` the cycle fills whole frames exactly,** round(T x fps) of them, so its last frame flows into its first. The
  speed is rounded to fit (it changes by under half a frame's worth) and the build says so; without `[loop]` the length rounds
  up to whole frames.

### 6. Any joint: a sword swing

`travel(<any bone>, velocity=)` takes the joint's speed. The poses are timed by how far that joint goes between them, and
nothing is carried: only the top-level root's travel moves the body. The distance is measured **along the joint's
interpolated path** between the poses, not the straight chord, and its clock goes along that path at the speed.

```gcmove
Floor = ground(z=0.85)
Guard  = pose(source=/Game/__MCPTests/BlueEdgeProbe/A_Intent_K0Guard, from=reference) {
  plant(stand_r, on=$Floor)
  plant(stand_l, on=$Floor)
}
# Over (A_Intent_K2Cock) and Finish (A_Intent_K3Land) the same way
Cut = pose(steps) { $Guard, $Over, $Finish }
Swing = clip() {
  $Cut [clock=hand_r]
  travel(hand_r, velocity=8m/s)
}
```

Both feet are planted in every key, so each foot is one hold through all three. On `SwordSwingTravel.gcmove` (60 fps, no loop)
the hand's path is about 122 cm (its chords would give 104), taking 0.153 s at 8 m/s, which is 10 frames (measured 2026-10-20). Between the solved
samples the played hand can pass the speed where the path turns back (9.75 m/s for the 8 asked, last measured 2026-10-20);
read the trails graph (`graph:true`) on the hand.

### 7. The paw and the floor (every reach and plant on a leg with a paw)

- **A reach or a plant moves only the leg above the paw** (user, 2026-10-20: *"make it optional, allow authoring to
  override"*). On a chain longer than four bones (the cat's blade, shoulder, elbow, wrist, paw, toe; hip, stifle, hock,
  paw, toe) the held point is put on its place by the first three bones. The paw is not folded to reach: the bones below
  the 4th keep their drawn turns, and on a ground the 4th bone turns only as far as standing flat needs.
  - On a ground (a plant, or a reach to a place standing on its ground) the paw stands **flat**, the pad on its place: its
    contact points (the plant's `roll=` points, the reach's chain contacts) are brought level on the ground, keeping the
    drawn heading. A paw behind, rolled onto its toes by the walk's plant, stays rolled.
  - In the air (a curl) it keeps its drawn wrist and paw folds.
  - An author's frame on the reach (`from=` or a facing) overrides the facing.
  - A person's leg and arm end at their 4th bone and are unchanged.
- **Nothing passes through the floor** (MOVE-TRAVEL1 B7; user-ruled 2026-10-12: the engine absorbs physics). The points of a
  plant's `roll=`, and for a reach to a place on a ground the chain's rig contacts, are kept on or above that ground at every
  sample, held or not: a paw swinging in to land is checked too. A point under it is lifted the least way by its own leg. A
  plain plant with no `roll=` holds its one point on the floor already.

### 8. What the build says

| The note | Means | Good looks like |
|---|---|---|
| `plant(leg_B_R) is one hold over poses 3-5 and on through the seam to pose 1 of $Steps (pose 1's line N kept), taking hold at pose 3, let go across the stretch into pose 2` | one run of a paw, and which line was kept | each paw one run of three keys |
| `<clip>: pose(Steps) follows travel(root) at its fixed X cm/s, each stretch as far as the paw planted at both ends and written last in the later pose is drawn apart: pose 1 to 2: D cm by plant(leg) at its <contact> ...; S s in all` | which paw decided each stretch, how far, and each stretch's time | the paw you meant, and a distance near what you drew |
| `<clip> [loop]: its cycle fills its N frames exactly (S s), so the last frame flows into the first: travel(root) goes X cm/s instead of the Y written` | the speed rounded to whole frames | within a few percent of what you wrote |
| `<clip>: pose(Cut) follows travel(hand_r) along hand_r's path, at its one speed V cm/s the whole way: pose 1 to 2: D cm (its chord C), T s` | a non-root travel's stretches along the path | the path longer than the chord where the arm sweeps |
| `line N: plant(leg) went under its floor at S sample(s), ...; its own leg turned it back onto the floor at K` | the floor seat | K equal to S; WARNING where it is less |
| `line N: reach(leg) moves only the leg above its paw, the paw standing flat (...)` | the paw rule, with how far a stop kept the paw off flat | no WARNING; a paw within 5 degrees of flat |
| `WARNING ... the paw stands up to X degrees off flat (frame f), held by ...` | a stop of the leg keeps the paw tilted | the note names the stop: change the place, or the key's body |

Then look: render each key and the walk's frame at the same pose moment side by side (the stretch note gives the moments), and
the trails of the four toes and the root (`highlights:[{type:"trails", of:[toe_F_L, toe_F_R, toe_B_L, toe_B_R, root]}]`) from the
side, top and front.

### 9. Not built yet

- **A speed curve, and a build-up under the rig's acceleration caps.** Today the speed is one fixed speed from the first
  frame. The user's later words (*"the speed is the upper limit, not really fixed"*, *"because things need build up"*) are
  for an optional speed curve afterwards, and a start speed if needed; neither is built.
- **In progress (user, 2026-10-20: *"lift early when appropriate frame, not necessarily fully stretched"*):** from its lift key
  a planted paw rolls onto its toes and its weight is capped by that key's sweep (Claude's reading of the ruling, to confirm).
  Not built; until then a paw is held to its let-go across the stretch into its curl.

## 6c. A gait written as rules: each leg's cycle, its joints' angles and its footholds (MOVE-GAIT1, 2026-10-31; `Content/__MCPTests/CatMove/Intent/CatGait.gcmove`, `Rig/Elephant/ElephantGait.gcmove`)

**The dividing line** (user, 2026-10-31: *"anything that DECIDES the motion belongs in the file and the build"*): write what a walking
animal does, measured or chosen, and let the build find the folds. No angle converted by hand, no picture drawn by a solver outside
the tool. A gait is then only when each leg's cycle starts and how much of it the paw is down (Hildebrand 1965).

1. **One cycle per leg, its poses at its events.** `StepBL = pose(steps) { $LandBL, $MidBL, $LiftBL [at=0.64], $SwingBL, $LandBL }`,
   played as `$StepBL [phase=0]` with `$StepFL [phase=0.25]`, `$StepBR [phase=0.5]`, `$StepFR [phase=0.75]` (the walk's lateral
   sequence); the trot is the same poses at `0, 0.5, 0.5, 0` and `[at=0.45]`. Each pose is `pose(from=reference)`.
2. **Each joint at its measured angle, and how far it may give:** `swing(stifle_L, axis=fold, to=67, give=8)`. `to=` is the fold
   the joint ends at as the rig counts it (0 straight; a knee of 113 degrees is 67). It is read on the rig's declared rest; a joint
   that closes the other way reads negative (the cat's shoulder, `to=-65`). Write `give=` for each measured joint, a girdle (hip,
   blade) with `by=` and no give (free, it sets where the paw lands), and `swing(hip_L, axis=side, by=0)` so the paw stands under it.
3. **Each paw on its foothold:** `reach(leg_B_L, contact=(Sole_B_L, Pad_B_L), to=$AtLandL, flat=1)` stands the paw there (the pair's
   front contact on the place; with `flat=1` the paw stands within a centimetre of flat on the LOWER of its two pads, as a plant rolls it, never
   under the floor, and a paw in the air may tilt toes-down; without it both pads at the place's height, which on the cat's asymmetric right
   forepaw opened the wrist 34 degrees past its give). Measure the foothold from the
   girdle where the moving body has it: `AtLandL = point(bone=hip_L, at=(forward=10.4), relative=moving, on=$Floor)`, or as a share
   of the leg (`at=(forward=45%), of=leg_B_L`). A pad animal measures from its pad: `point(contact=Sole_B_L, at=(forward=80),
   relative=character, on=$Floor)` (the elephant).
4. **The stance:** a `plant(leg_B_L, on=$Floor, roll=(Sole_B_L, Pad_B_L))` beside the reach in each pose the paw stands in. Its
   mid-stance and lift-off either name their footholds (the walk: under the hip, 9.2 cm behind it) or say **where it landed**:
   `reach(leg_B_L, contact=Pad_B_L, to=planted)`, the landing's pads carried back by the travel since, at the height of its landing's lower pad
   (the gallop). It is refused by name in the landing pose itself, without that leg's plant, outside a cycle, on a reach that is no rule, and
   under `velocity=sweep`. A plant lets go across the whole stretch into the next pose, so keep that stretch short: a pose just after
   lift-off with the paw in the air (the cat's `$Push*`, 0.04 of the cycle later), or the swing pose early (the elephant's at 0.76 since
   2026-11-01: at 0.84 its plants faded across 0.16 of the cycle and dragged each swinging foot 23-35 cm toward its print, 11-18 at 0.76;
   a push pose reaching the elephant's toe pitched its foot heel-up and flicked it 14 cm over its swing's height). And a foot that lifts
   off its toe ROLLS onto it: `plant(leg_B_L, on=$Floor, roll=(Heel_B_L, Sole_B_L, Toe_B_L))` (the elephant's since 2026-11-01). Held flat on
   one `contact=` to its let-go, each elephant hind foot lost its print over the last third of its stance and dragged 6.6 cm, the straight leg
   unable to reach back to a flat foot 80 cm behind; rolling, every foot holds its print within 0.01 cm.
5. **The speed:** `travel(root, velocity=sweep)` is the speed the drawn footholds imply (each planted leg's sweep over its stance
   time, the slowest; the note gives each). With `to=planted` state the speed instead (`velocity=2.6m/s`): the stance then follows it.
6. **A gait nobody measured joint by joint (the gallop):** each joint at the walk's measured angle where its reference (the critic-passed
   picture gallop) stands within the give of it, and at that reference's own angle where it does not (the hind knee yields to 97 at mid-stance,
   the foreleg lands and lifts straighter); the walk's own footholds, since a cat lands its toe as far ahead of its hip at every speed (Harnie 2022);
   and room to open where a gallop goes further than a walk: `give=25` in mid-swing (the knee ranges further, Wisleder 1990) and at lift-off
   (a push-off stretched behind, Hildebrand 1977), each reason written beside the line. The foreleg's mid-swing is the walk's elbow (91) with
   `give=25` and a reach that carries the pad 8% of the leg up under the blade: at the picture gallop's elbow (78) its toes brushed the floor
   through early swing, as the pictures' did, and lifting them asked the elbow to 101-104, past any give drawn from 78.
   **Where a real cat says otherwise, the photograph wins** (2026-11-04, Muybridge 1887 plate 716, `Docs/Research/cat-walking-joint-curves.md` in
   the GraphCode development checkout): a galloping cat's leading forepaw lands under its nose, far ahead of where a walking one lands (Harnie's equal footholds
   are measured only to 1 m/s), so the gallop's forefoot place is `point(bone=scapula_L, at=(forward=17.4), relative=moving, on=$Floor)` (the
   head's joint ahead of the blade plus the face's front ahead of the head, both measured on the rig), not the walk's 10.4.
7. **A reach in flight before each landing**, at about 0.9 of the leg's cycle: the paw a little further ahead than it lands and a few
   centimetres up (`GReachF*`: 3.5 cm past the nose, 4 up, the elbow and wrist near straight; `GReachB*`: 2 cm ahead of the landing, 2.5 up, as the
   walk's `ReachB*`). Reach it with the PAD ALONE (`contact=Pad_F_L`), as a swing's pass does: a paw in the air need not be flat, and held flat on
   palm and pad 4 cm up a forepaw ended 1-2 cm short at every distance tried. Start a reach's free shoulder near where the leg will end (`to=-48`,
   where the other side solved): from the landing's -65 the solve stopped in a bent elbow 3 cm short. Without the hind's reach, read at the 120 fps
   the gallop is stored at, each hind paw skimmed 0.5-1 cm over the floor through the last sixth of its swing and lifted again before landing, which
   reads as a late, paired landing; a 30 fps read hides it between samples. Hold the swing pass late enough (`[at=0.7]`) that the paw does not sink
   on its way to a far reach.
8. **The tail, in the rig's words:** on a rig whose tail folds as its nod (each tail band wrapping the tail, `closes=back`; RIG.md), write its
   carriage from the rig's rest (`pose(from=reference)`: + fold lays it back toward level), a sideways swing once a stride (the dog's, Wada et al.
   1993; `Docs/Research/cat-tail-in-locomotion.md`) and an up-down follow of the body's bob, each joint phased a little after the one before
   (`$TailBob [phase=(0.95, tail_02=0.99, tail_03=0.03, tail_04=0.07)]`). In a gallop, phase its lift to the gather and make it larger than the
   pelvis's own pitch, or the pelvis carries it the other way (it drooped in the gather until its base lifted 34). Size every tail amount against a
   photograph: under about ten degrees of change through the stride both critics read "frozen".

**What the build does:** each rule reach is solved into its own pose's lines on the gait's body at the moment its cycle reaches
that pose (bobbing, tipped, flexing as it plays), so one set of leg poses serves every gait it plays in. Each joint moves as
little as its give lets, the rig's ranges hold, and the note lists each joint's turn, the fold it ends at and how far the paw
ended from its place, with `WARNING` past a give or a missed place. The cycle's join carries each planted leg's girdle through
its landing and lift-off at its stance's speed. Read the result with `Tools/GaitCheck` (its README): `snaps`, `footfalls`, `angles`.

**Measured (2026-11-01, against the same gaits from pictures, now `CatGaitPictures.gcmove`, `ElephantGaitPictures.gcmove`):** the
walk, trot, gallop and elephant pass the same specs. Worst turn per frame: the walk 34.9 deg (36.8 from pictures), the trot 36.7 (33.0),
the gallop 37.6 at the right hip just after its push-off (33.1), its knees 31.4 and 30.0 (33.4, 31.6) and its hock 32.4 in the swing's
deeper tuck (~~2026-10-31: walk 37.4, trot 38.2~~); its forepaws lowest in stance on their pads (toe joints 0.56 and 0.77 cm up), where from
the pictures they dipped to 0.07 and -0.36 cm in early swing; the sweep 43.8 and 81.1 cm/s; every `to=planted` pad within 0.05 cm of where it landed,
carried back 12.1 cm at mid-stance and 24.3 at lift-off at 2.6 m/s. The elephant: a 157.6 cm stride at 0.89 m/s (160, 0.90), its worst
turn 11.0 deg/frame (16.3). ~~Still past their give in the notes: the walk's and trot's landing knee (12-16 degrees) and their in-air reach
just before it (25-40)~~: their footholds and measured angles do not meet on this sculpt's legs at any one body height (measured 2026-11-01:
the hips 2 cm lower put the landing knee within its give and every other hind pose 8-19 past theirs; the whole body lower pressed the chest
into the right elbow), and the footholds are the robust numbers, so the landing's knee and ankle and the trot's lift-off knee give 25, the
in-air reach's knee and ankle 45, the foreleg's landing elbow 25 and its reach's elbow 35 and wrist 25, each reason written over the poses.
~~Still past give~~ (all FIXED 2026-11-01, no note left): the walking sculpt's right foreleg is the lifting one (blade 1.2 cm forward and shoulder
0.8 higher than the left's mirror, bones 1.08 longer below), so it has angles of its own, mid-stance elbow to=81 (it folds 87, the left 80) and
landing wrist 34 and toes 25 (flat 0.87-0.90 cm, the left 0.86-0.89; ~~1.25-1.28 past flat=1~~); the trot's hind reach, near straight on both
sides, is a pose of its own (knee and ankle 30); the gallop's leading right fore lifts at 0.27 (~~0.28: its shoulder 0.7 past 25, its blade's curve
past its stop~~). A green spec says nothing of these, so `AGaitIsOneLegCycleTimedByItsPhase` now fails any `PAST ITS GIVE` note: read them.

**Not yet:** `to=planted` under `velocity=sweep` (each needs the other: refused by name); a cycle that rides a clock; a foothold on a
moving landmark's socket; ~~the cat's right forepaw drawn as its left, a SCULPT fix~~ (FIXED 2026-11-01 in the sculpt: measured on
`SM_BritishShorthairCatSquare`, the walking cat's right forepaw is modelled lifting, its lowest skin 0.66 cm up against the left's 0, its toes'
front 2.10 cm up against 0.98, while each paw's joints sit centred in their skin, so through stance its toe pads stood 1.20 cm over its palm
against the left's 0.47. `BritishShorthairCatSquare.gcshape` now turns the skin below the right wrist 20 degrees toes-down about its toe joint
with one `warp()`, the rig's right palm, pad and toe-top points carry the same turn and its bones stay, so its pads sit under its bones as the
left's do: 0.43 cm in the walk, 0.45 in the trot, its right forefoot's places measured 1.2 cm further back from its blade, which the
lifting foreleg carries that far forward of the left's mirror (the gallop keeps its picture-measured 10.4 on the right). Turned with the skin the bones would have carried the pads back, every `to=` being read on
bones; ~~0.45 cm lower as well~~, that lengthened the leg and folded its elbow further); ~~the cat's left elbow likewise (its IK rig's Full Body IK limits hinge it on Y and lock Z, the right elbow the other way round,
so its fold is read on a locked axis, "range 0 to 0", and the final audit clamps it 6-10 degrees in every gait: the gallop's left elbow tops
out at 92 in swing, the right at 102)~~ (FIXED 2026-11-01 in Move: the IK rig was right; a rest-twisted joint's declared fold was snapped to
the rig's axis in the twisted frame instead of on the bind, and a twisting fold on Y with its Z locked was read past 90 as a side on the
locked axis; the gallop's elbows now reach 102.5 and 102.6, research note cat-elbow-rest-twist-axis.md in the GraphCode development checkout).

## 6b. ~~A four-legged walk, written as intent~~ (2026-10-19; superseded by 6a on 2026-10-20)

Moved verbatim to [WALK-AND-RUN-DONE-2026-11.md](../Historical/WALK-AND-RUN-DONE-2026-11.md) and [WALK-AND-RUN-DONE.md](../Historical/WALK-AND-RUN-DONE.md).

## 7. Use it

- **In a game,** build the one-stride clip with `[loop, root_motion]`: the character's movement takes the travel the
  feet made. For walk/run blending, sync groups and play rate, see
  [ANIMATION.md](ANIMATION.md#build-the-runtime-performance-with-native-animation-tools).
- **For a render or a sequence,** the `drive=Root` example can use `clip(from=$Walk, repeat=4)` without
  `[root_motion]`: it walks off with its travel in the root bone. For explicitly authored root travel,
  use the full-take profile arrangement in section 6 instead.
- **Holding something while walking** works as before: lay the held pose over the walk (`clip(from=$Walk) { $Gripped }`,
  [HANDS.md](HANDS.md#step-9-a-rifle-in-both-hands-then-walk-with-it)).

## When it goes wrong

| You see | Why | Do this |
|---|---|---|
| The planted foot stands above the floor | an ordinary `plant(..., contact=…)` keeps the landing turn of the goal bone; a contact far from that bone hangs off it | use `roll=` with contacts on their own bones |
| `plant takes contact= or roll=, not both` | both on one line | keep `roll=` |
| `plant roll= takes 2 to 6 rig contacts, back to front` | one contact, more than six, or a missing `( )` | `roll=(Heel_R, Ball_R, Toe_R)` |
| `contact 'Ball_L' belongs to chain 'leg_L', not 'leg_R'` | a left contact on the right plant | use the plant's own side |
| `contact 'Heel_R' rides 'foot_r', not the goal bone 'toe_r' …: only plant(..., roll=) reads a contact on an inner bone` | a `bone=` contact used by `reach`, `grab`, `contact()` or a plain plant | give that reader a contact on the goal bone |
| `self-collision in 'Run' at frame 9.4: 'ball_l' intersects 'calf_r' by 0.40 cm` | the swinging foot passes too close to the standing leg | widen the step: `swing(thigh_X, axis=side, by=2..4)` on both thighs |
| The heel snaps up at toe-off | the drawn ankle turns late and fast | lift the heel through late stance in the curves |
| The body dips at heel strike (the stance note shows a large dip) | the drawn stride brakes, or the two planted feet ask for different places (the legs disagree in double support) | check the stride: both feet should slide back at the same speed in the drawing |
| A flight carries a different speed each stride | one leg pushes harder than the other in the drawing | expected for measured data; average the legs if you want them equal |
| `travel(Root, velocity=80%) asks a share of this character's body_speed:, and its rig states none` | a share with no `body_speed:` on the rig | add `body_speed:` to the `.gcrig` header and rebuild the rig, or write `velocity=4m/s` |
| `travel() takes velocity=, ... not speed=` | the old spelling | write `velocity=` |
| `travel(Root, ...) moves the body, and plant(..., drive=Root) moves it again from the feet` | a plant still drives the root | remove `drive=` from the plants: `travel()` moves the body |
| `travel(Root, ...) moves the body in the solve, and this document names no ik_rig:` | no `ik_rig:` on the `.gcmove` header | name the character's IK rig in the header |
| `from pose N to pose N+1 of $Steps no plant holds a paw in both poses, so nothing says how far the body goes` | the walk's keys plant no paw in both ends of a stretch | plant the paw that carries the body in both keys (6a step 5) |
| `... so the body would go back` (or `nowhere`): `travel() carries it forward, so the later picture draws that paw further back` | the deciding paw is drawn no further back in the later key | draw it behind in the later key, or write another paw last in that key |
| `pose N of $Steps is no pose(source=) picture` | a pose in the list is written in the file | build it as a still and read it with `pose(source=..., from=reference)` |
| `a pose(steps) on this travel is timed at one fixed speed, and velocity= here varies over the clip` | a `$profile` speed on a travel that times poses | write one speed (`0.4m/s`) |
| `[clock=root]: nothing in this clip moves root on a clock` / `[pace=...]: this travel reaches its poses at its one fixed speed` | a `[clock=]` that names another bone than the travel's, or a `[pace=]` / `[phase=]` on the reference | name the travel's bone; take `[pace=]` off |
| `'Walk' writes no frames=: a clip is timed by its travel() only through a pose(steps) that rides it` | no `frames=` and no `$Steps [clock=root]` | add the reference, or write `frames=` |
| `plant drive body state was not certified: … (equality … cm, inequality … cm, stationarity …, complementarity …)` | the body's best motion could not be proven correct. A negative `inequality` is how far it still breaks a leg-reach or acceleration limit: almost always the pace asks the body somewhere the planted feet cannot carry it | make the footholds and the pace agree (step 4b): plant the feet where the pace puts the body, or ask a pace the drawn stride really covers |
| `plant 'leg_L': rolling skin seating skipped at … sample(s), frames …; worst measured skin … cm under floor; incoming plants retained. …` | on those frames the held point and the rig's joint limits cannot lift the foot's skin out of the floor, so the frames keep the skin under it (a clip without `balance(...)` still builds; with `balance(...)` over that leg it refuses) | draw the foot flatter at touchdown (less toe-up at heel strike), or move the plant's held point a little further back on the heel |
| `rolling skin not bound: …; seating skipped` | the foot's sole could not be measured on this mesh, so its skin is not checked or seated | check that the rig's foot contacts sit on the mesh's sole; `balance(...)` over that leg needs it |
| A held point ends up to about 1 cm off, and a little under the floor, on the last full-weight frame of a push-off (`a held point ended at most 0.96 cm …`, and a `'thigh_X' clamped … past its stop` warning for that leg) | the drawn leg asks more hip extension than the rig allows, and the solver does not turn the pelvis to make up the distance, because the pelvis is shared by both legs and the trunk | draw a little less hip extension at take-off, or end the plant a frame earlier |

## What it does not do yet

- **No gait is made for you.** You draw the legs (or map measured curves); the solver keeps the feet and moves the body.
- **A travel's speed is one fixed speed.** A speed curve and a build-up under the rig's acceleration caps are not built (6a step 9).
- **Flat floors for the baked rolling plant.** `roll=` plants on `ground()` planes.
  [SURFACE-ALIGNMENT.md](SURFACE-ALIGNMENT.md) adds a separate bounded runtime correction for declared
  stance contacts. Stance selection, future footholds and body coordination remain authored.
- **The heading of a planted foot follows the drawing.** If the drawn foot turns a degree during stance, the planted
  foot turns about its held point too (under 0.5 cm at the toes on the example). Holding the heading was tried and
  removed: on a rig whose legs cannot twist it cost the foot's pitch.
- **The body's speed is carried within one clip, not across clips.** `repeat=` continues it; `clip(from=$Run)` starts
  a new clip at rest, and the `[loop]` note says how far the end state is from the start (speed and position) without
  forcing the seam.
- **The root stands in for the whole body.** The carried speed is the root's; arms and trunk follow their drawing.
  Balance reads that speed in a run ([MOVE-BALANCE.md](MOVE-BALANCE.md), "Balance while moving").
- **The note checks the stored frames; playback interpolates between them.** On the 30 fps example walk every stored
  frame is within 0.05 cm of the floor, but between two of them the ball dipped 0.17 cm under it during late stance.
  Build at 60 fps (`fps:60`) where that matters, and judge contact in a render, not only in the note.
- **The note's "lowest point" covers planted feet only.** A swinging foot is your curve and is not checked: on the
  measured test run the landing heel passed 3.2 cm under the floor on the last frame before contact while the note
  read 0.00. Look at the frames just before each landing in a side render.
- **The measured full-body walk and run are not accepted examples yet.** They include mapped pelvis,
  trunk, leg and arm curves. Comparing planted motion with its drawn twin measures the solver's
  changes, not fidelity to the source person. Neutral frames, proportions, phase continuity,
  toe motion and the visible performance must also be checked.
- **A knee may lock straight at heel strike.** When the rig's proportions differ from the person the curves came
  from, the heel can arrive a little above the floor; the solver straightens the knee to reach it and the note reports
  `'calf_…' clamped … past its stop at 0.0`. A few degrees is normal; more means the drawn hip height is too high for
  your rig's legs (lower the root curve, or scale it to your leg length).
- **A reported stop can use a different zero.** Rig ranges are measured from straight; the
  solver reports travel from the reference pose. Runner's measured rest bend is -5.933165°,
  so its -30° anatomical hip bound becomes -24.066835° from rest. This difference alone is
  not a propagation defect. Check [RIG.md](RIG.md#reading-what-the-build-says-about-joints)
  and the source-to-reference mapping before widening limits.
- **Left and right twist are not always mirrored on a rig.** On the test rig, `twist` + turns the right thigh and upper
  arm in but the left ones out. Check a new rig's signs with a one-frame swing per side before mapping curves onto it;
  a wrong sign shows up as a foot turned into the other foot (the self-collision check refuses it) or a hand into the
  thigh.
- **A landing now waits for the body's travel (since 2026-10-07), and the body carries its speed (since 2026-10-12).**
  Any older clip that drives its root with `drive=Root` builds differently from before. Rebuild such clips and look at
  them.
- **The shipped rifle walk has not been switched to `roll=` yet** (its carry pose fails a hand self-collision check at
  frame 0, unrelated to walking).
