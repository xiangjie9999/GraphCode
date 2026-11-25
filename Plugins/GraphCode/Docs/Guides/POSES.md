# Reusable poses and collision fitting

This guide describes the implemented authoring path. A `.gcmove` is editable source; apply its
package path with `gc_apply(dry_run=true)` and then `gc_apply` to build ordinary animation assets.
Use `gc_read` for the native character, rig, Physics Asset and resulting animation metadata.

## Compose instructions

```gcmove
Left = pose() {
  swing(left_joint, axis=fold, by=90)
}
Right = pose() {
  swing(right_joint, axis=fold, by=90)
}
Ease = profile(0=0, 0.5=1, 1=1)
Action = clip(frames=48) {
  $Left@40%, $Right@85% [profile=$Ease]
}
```

The names above must resolve on the chosen character. `fold` needs a declared rig axis;
otherwise use that bone's measured local axis. Angles describe the requested form, not a promise
that the joint can reach it. `ik_rig:` supplies the native numerical joint limits. Missing limits
are not inferred from names such as knee or wrist.
Clamping produces a `WARNING joint limit` note naming the bone, largest correction and frame.
The finished tracks are independently checked after composition, fitting, IK and angle overrides,
including finely sampled playback interpolation. An illegal final angle refuses the whole build
before any animation is written. If legal keys drift outside limits during interpolation, the
builder inserts constrained intermediate samples and increases the bake rate, preserving duration
and arrival times. Each sample it adds follows the motion you wrote and is fitted to what the clip
touches, as its keys are: a finger stopped on the one beside it, or a palm held on a surface, stays
there between keys (`WARNING contact between keys` says how far it moved one). The clip is then
checked for collision again, and the refinement reports a warning.
Missing limits and explicitly unbounded axes also warn; collision
acceptance cannot establish a maximum joint angle. Configure limits in the rig to cover those joints.
These are rotation limits; they do not impose mechanical translation stops on `shift()`.


A pose has instructions and optional collision targets; its caller supplies clip duration.
References may nest and may appear beside ordinary instructions. References preserve the
instructions and their constraints until evaluation rather than blending two baked clips.
An overlapping FK channel composes in written order; rotations about different axes do not commute.
Independent amounts do not normalize: 40% of Left and 85% of Right stay 40% and 85%.

These reference forms use the same unitless curve evaluator:

- `$Left@40%` is a constant amount.
- `$Left@$Amount` reads a declared profile whose values are fractions between 0 and 1.
- `$Left@profile(0=0, 1=0.4)` supplies that curve inline.
- `$Left@40% [profile=$Ease]` multiplies the constant amount by the easing profile.

Nested amounts multiply. A reference's curve uses the common source clip clock; an instruction's
own `by=`, `weight=`, phase and nested timing controls remain independent. A constant zero reference
contributes no instructions or collision context. Cycles and invalid references still refuse.
`200%` is a segment speed, not a valid pose amount. Profiles do not take percent-valued stops.

### A pose made in steps, and how a step is entered

```gcmove
Hold = pose(steps) {
  $Open                                   # where it starts, held from frame 0
  contact($Gun.Web, on=web_fold_r)        # then each line in turn
  $Close
  $ThumbOver
}
ThumbOver = pose(from=reference) {
  swing(thumb_02_r, axis=fold, by=90)
}
```

`pose(steps)` is one pose whose lines happen in order, the way a hand opens, takes a thing and closes
on it. Its first line is where it starts, held from frame 0; each later line comes in over an equal
share of the clip it is used in, from where the one before ended. No profile window is written.

A pose is entered one of two ways:

- `pose(from=previous)`, the default: each line on a joint stacks on the lines before it on that
  joint, so a pose says how far to move from where the joint already is. Close from open, seat from
  placed.
- `pose(from=reference)`: the pose's lines are measured from the skeleton's rest. As it comes in,
  every earlier line on the joints it names, and their source motion, gives way, so at full weight
  those joints are their rest turned by its lines. A line after it stacks on it as usual. Holds
  (`reach`, `contact`) are not lines and do not give way.

The difference matters because the joint limits clamp only the sum. A hand opened with a thumb joint
asked to -90 (held straight at its limit) and then curled +90 from the previous pose ends straight;
from the reference pose it ends curled.

**`from=reference` on a `pose(steps)` enters every step from the reference.** In
`pose(steps, from=reference) { $Start $End }`, as `$End` comes in over its step, `$Start` gives way on
`$End`'s bones; at full weight those joints are their rest turned by `$End`'s lines, so a lunge's pelvis
drops once, not twice. Without it the steps stack, each on the one before. `from=reference` on one pose
inside a plain list enters that pose alone.

### A pose with values

Use this when one designed pose is wanted at different amounts (a lean at 20 and at 45) instead of retyped poses that drift apart.
A `name=value` in `pose(...)` or `clip(...)` that is not one of that kind's own settings is a parameter with that default; the
lines read it as `$name`. A default is a number, `true`/`false`, a word (a bone name too) or a `$Place`. A call names the values it
changes, `$Back(lean=45)`, in a body line, a `pose(steps)` list, `clip(from=)` or a build line (`A_Lunge = $Lunge(distance=60)`).

```gcmove
Back = pose(lean=0.0, round=0.0) {
  swing(spine_01, axis=fold, by=$lean * (1 - $round))
  swing(spine_03, axis=fold, by=$lean * $round)
}
Hand = pose(curl=0.0, left=true, right=true) {
  swing(index_02_l, axis=fold, by=$curl * 60 * $left)
  swing(index_02_r, axis=fold, by=$curl * 60 * $right)
}
Zombie = clip(frames=1) {
  $Back(lean=45, round=0.8)
  $Hand(curl=0.6, right=false)
}
```

- `$Back` alone is the defaults. Arithmetic (`+ - * /`, brackets, `sin cos tan asin acos atan atan2 rad deg sqrt pow abs floor ceil min max clamp lerp`, degrees) is read only in a value a parameter was written into; a unit may follow a bracket, `($lean * 2)%`.
- A side is a switch: `true` is 1 and `false` is 0 in arithmetic, so `by=$curl * $right` leaves the right hand alone.
- Names are read as spelled (`$Grip` and `$grip` differ). A parameter its lines never read is refused, so a mistyped setting (`colision=self`) is still refused. Each set of values is one plain pose named as the notes print it: `Back(lean=45, round=0.8)`.
- Limits: a pose named inside another value (`[strike=$Pose]`, `at=$Hold`, another file's `along=File.Entry`) is reached at its defaults only. A clip's line cannot yet play a parametric clip, `frames=$n` is not read, and there are no preset files or imports yet.

### Travel between two built poses

The way to move from one pose to another that you built and checked first (a cocked sword to a landed cut, a
guard to a strike): read both with `pose(source=)`, let the second come in from the reference pose, and give
it an amount curve for the speed.

```gcmove
Start = pose(source=/Game/Anims/A_Key_Cocked) { }                  # step 1's end, or a keys clip
End   = pose(source=/Game/Anims/A_Key_Landed, from=reference) { }  # solved once by IK in a keys file
Strike = profile(0=0, .15=.03, .35=.15, .55=.45, .7=.75, .8=.93, .87=.99, .9=1, 1=1)
Cut = clip(frames=24) {
  $Start
  $End@$Strike              # the start gives way as the end comes in
  plant(leg_l, on=$Floor)
  plant(leg_r, on=$Floor)
}
```

- The joints travel between the two poses in joint space, so the path is the poses' own: a sword cocked over the
  shoulder and landed by the opposite hip rises over the top and cuts down on one diagonal with no key in between.
  Add an in-between pose (a third keys clip) only where that path is wrong.
- Neither pose's held parts need repeating: a `pose(source=)` carries every bone its clip animated, fingers too.
  Laying a grip pose over them again stacks the curl and the fingers go into each other.
- **A one-hand grip source poses the OTHER hand too, and it will be wrong** (measured 2026-10-23, BlueEdge's three
  animations; the user: *"the left hand always look weird"*). A whole-clip `pose(source=)` sets every bone its source has
  a track for, so a fist built for the right hand writes whatever its left hand happened to be doing — in that probe,
  fingers asking 32-50 degrees of HYPEREXTENSION. The clips inherited it for a whole day of renders: on screen a splayed
  claw with the fingertips bent backwards, on the empty hand the eye goes to. **Take only what you meant**:
  `pose(source=/Game/.../A_GripFist, bones=$RightHand)` leaves every other bone untouched, so the free hand keeps the
  skeleton's own rest — a relaxed, slightly curled open hand, which is what an empty hand should be.
  - **The tell is in the build, every time**: `WARNING joint limit: 'index_03_l' clamped by up to 27.547 degrees (its
    fold asked -35.9, past its stop at -8.4)`. A fingertip asking far past a stop is almost never something you wrote;
    it is a pose arriving from somewhere else. Five of those lines sat in the notes from the first build.
  - `bones=` refuses by name a bone its source does not animate, which is how to FIND the right list rather than guess
    it: name what you think, read the refusal, drop that bone, repeat. Here the thumb's opposition and roll hinges and
    all four metacarpals turned out not to be animated at all.
  - Check what else the source was quietly supplying before you narrow it. The same grip was also posing the right
    **arm**; taking only the fingers dropped the sword 29 cm (`upperarm_r` fold 47.99 to 20.32) until the arm was named
    in the group too. Compare a bone you care about in the readings before and after.
- **Give the off limb a job, and choose it yourself** (2026-10-24, the user: *"You always need to find an angle for the arm and hand
  to look natural, it's more like a style preference"*, *"find something to do with left hand"*). ~~A free arm nobody authored is left
  where the shoulder happens to leave it~~ (SUPERSEDED 2026-10-15 for a reach with `[share=]`, MOVE-ACTION1 W5: see "What a share already gives it" below; still true with no share), and an idle limb with no intention reads as awkward however legal its joints are. A resting
  arm is POSED (FK swings), not PLACED, so no bubble and no max-extension rule applies to it -- those govern where a hand REACHES to.
  Which intention it takes is taste, not a solve: build two or three versions of the same key that differ only in that arm and look at
  them side by side. For a swordsman at ready, measured 2026-10-23: the hand at the waist with the elbow back held a whole clip and did
  not compete with the sword; a "counterweight" arm out from the body barely differed from hanging; a forearm up across the chest read
  strongly but crowded the sword arm. Watch the START and END of a clip especially -- that is where a free arm is on screen longest.
  **What a share already gives it (MOVE-ACTION1 W5, built 2026-10-15; read from the code, `GcMoveProgramFreeHand`):** a `reach` with
  `[share=]` already gives the other arm a counter-placement by default: it reaches a point away from the working hand through
  the chest, between waist and chest height, at the reach's weight times the share. It is a default, not your taste: a written
  `reach`, `aim` or other constraint line on that arm overrides it, and dropping the share switches it off. The advice above
  stands for the arm you write ([The body joins a reach](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach)).
- **Three stills or more, each at its own moment:** keep going the same way. Every still after the first is its own
  `pose(source=..., from=reference)` reference with its own curve, flat at 0 until its window opens and flat at 1 after
  it closes, so each takes over from the one before exactly when you say (a strike landing at 0.53 of the cut is
  `$Strike@profile(0=0, 0.37=0, 0.48=0.5, 0.53=1, 1=1)`).
- `pose(steps) { $Start, $End }` does the same travel but splits the clip evenly, so it moves at one speed (the notes
  say "already moving at the clip's start"), unless it is timed by effort (below) or by a travel. **A curve on it
  does not time its steps** (field report GC-03, 2026-10-22): `$Steps@$Curve` weights the whole pose, its first step
  too, so where the curve is 0 the body is at rest and the steps still pass in equal shares. The build says so in a
  note. To place each step, use the per-still form above.
- **A travel can time the poses by a joint's speed instead of by effort** (MOVE-TRAVEL1, 2026-10-20). Build each pose as a
  `pose(source=..., from=reference)` still, list them, and follow the travel's bone with `[clock=]`; the poses are reached at
  distance / speed, and no `frames=` is written:

  ```gcmove
  Cut = pose(steps) { $Guard, $Over, $Finish }      # each a pose(source=) still, its plants written inside it
  Swing = clip() {
    $Cut [clock=hand_r]
    travel(hand_r, velocity=8m/s)                    # the distance is how far hand_r goes along its path between the poses
  }
  ```

  A walk is the same with `travel(root, velocity=0.4m/s)`, the body's distance read off the paws planted in the poses. The
  speed is one fixed speed; `[pace=]` and `[phase=]` on that reference are refused by name. **Unless it builds up**
  (MOVE-TRAVELBUILD1, 2026-10-22; the user on a cut at one speed: *"why so fast? and also, there's no continuation,
  acceleration whatsoever"*): `travel(blade_r, velocity=4m/s, accel=60%)` times the poses by effort, from rest to rest with
  one peak, every joint the poses move inside `accel=` of its reference and the bone never past `velocity=`, which is now
  the most it goes, not its speed; whichever binds sets the time. The note: `follows travel(blade_r, velocity=4m/s,
  accel=60%): from rest to rest in 0.565 s, blade_r going 121.2 cm along its path and 4.00 m/s at its fastest (0.286 s
  in; velocity= is the most it goes)`. `accel=` takes a share or deg/s2 (m/s2 is refused by name) and is refused on
  `travel(root)`. Plants written inside the poses
  hold across runs of them, and do the same when the list is timed by effort (`$Cut [accel=95%]`, 2026-10-22), so one set of
  planted keys serves both timings; a list on no clock keeps its plants as written. The recipe, the notes and the rules:
  [WALK-AND-RUN.md](WALK-AND-RUN.md), section 6a.
- **`path=shortest`: the travelled bone's own shortest path** (MOVE-SHORTPATH1, 2026-10-22; the user: *"travel decides the shortest
  path possible for the specific joint to go from A to B"*). By default (`path=joints`) each joint goes its own way from pose to pose and
  the travelled bone gets their sum: two keys solved alone can hold the forearm turned opposite ways, and a sword is then spun round the
  forearm between them (the 2026-10-22 chest-height cut: the forearm 140 degrees apart in two keys, the blade swung out sideways instead
  of tipping forward). `travel(blade_r, velocity=7.5m/s, path=shortest)` moves the bone itself from each pose to the next on the shortest
  path an arm can take it: swept about the shoulder, one great circle from its place in the one pose to its place in the next, and turned
  with that sweep (any further turn the next pose asks spread evenly), so a blade the keys hold the same way to the sweep and its tip sweep
  arcs about one axis through the shoulder; the IK rig's limb that carries it (the chain whose goal is the bone, or the bone it rides) is
  solved at each step by the keys' own arm solve, ~~the elbow's swivel going evenly from pose to pose~~ (SUPERSEDED 2026-10-22, MOVE-SHORTPATH4:
  an even swivel overshot the shoulder's twist stop between two legal keys; the user: *"if two frames hands are stretched out, the
  intermediate should be too"*) the elbow taking the one way round that keeps the upper arm, forearm and hand in their ranges with the least
  turn of the arm from step to step; the keys keep their own. The note gives each stretch's sweep, the bone's turn beside it (`the bone
  turning N degrees more beside it`: how much the next key's held part disagrees with the last one carried along the sweep; tens of degrees
  is a consistent pair, 100+ spins it in the hand), how far past their ranges the joints went and how close the hand stayed to its place.
  ~~Each stretch is its own: a middle pose makes a corner, so write a cut as its two keys.~~ (SUPERSEDED 2026-10-22, MOVE-SHORTPATH2/3:)
  through exactly three poses the bone goes along the one circle through its three places (`along one arc through its 3 poses`); through
  four or more, one curve joined through them with no corner. A swing still needs only its two keys
  ([HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade)). Refused by name on `travel(root)`.
- **A held blade's cut uses `path=trailing`** (2026-11-15): the shortest path with the sword arm turned at each key, over
  two or three poses; the cut is taught in [A cut with a held blade](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade)
  (the short form). What it does, needs and refuses: ask `gc_describe kind:"move" block:"constraints"`.

**Time the travel by effort** (new in this build, 2026-10-13): put `[accel=]` on the reference, and the build works
out when the poses happen from the joints' limits, as an effort `swing` does. The poses are kept; only their timing
changes.

```gcmove
Swing = pose(steps) { $Start, $End }
Cut = clip(frames=24) {
  $Swing [accel=80%]            # or: $Start, then $End@profile(0=0, 1=1) [accel=80%]
  plant(leg_l, on=$Floor)
  plant(leg_r, on=$Floor)
}
```

- `80%` is a share of each turned joint's rig reference (`velocity:`, `accel:`, `ramp:`); `3000deg/s2` caps each
  joint's turn. The joint that limits the move reaches the asked acceleration, and the clip lasts the solved time. Lines beside it with no time of their own are
  fitted to that time ([ANIMATION.md](ANIMATION.md), "The clip's length and its other lines").
  The note says so: `Cut: pose($Swing) timed by effort: 0.412 s at accel 80% (binding lowerarm_r: ...)`.
  When a head or neck joint binds, a second note says the head is setting the pace: a gaze held on a target while the
  trunk turns under it loads the neck, whose references are the body's lowest (a MetaHuman neck joint 3333 deg/s2, a
  spine joint 7400), so let the head turn part of the way with the chest at the most-turned pose.
- It times every amount at or under the reference: its own `@amount`, a `[profile=]`, and a `pose(steps)`'s steps.
- **Lead and lag, for a whip:** `$Swing [accel=80%, phase=(0, pelvis=-0.06, lowerarm_r=0.05, hand_r=0.08)]`. Each
  named bone plays the travel's clock that much early (a lead, negative) or late (a lag, positive), and all land
  together; the hips start first and the wrist last. A lagged joint covers its path in less time, so it moves faster:
  the build tightens that joint's limits by the same factor before it solves the clock, so as played every joint stays
  inside the effort. A note per bone gives it (`lowerarm_r plays its clock 0.05 of it late: ... speed x1.05,
  acceleration x1.11`). At most 0.5 either way. **One phase map can be shared across a clip's poses** (ruled
  2026-10-22): a lead or lag reaches only the bones a pose swings or shifts itself, so a bone a pose leaves at rest is
  skipped there with a note, and a name that is not a bone of the skeleton is refused as a typo. On a curve with no
  effort, a lag needs the curve at rest at its start and a lead at its end (two equal stops, any distance apart).
  **What a lead does, as measured on 2026-11-12:** it plays the bone's whole path in less time from the travel's START (a lead L plays
  bone(t) = unphased(t / (1 - L))), so it is nothing at the first pose and largest late; it does not start a part earlier inside one
  stretch. With a pelvis lead of 0.14 the sword slash's tip was 28.7 cm off its contact pose's target at the contact instant.
- **A collision form under the effort** (`pose(collision=self)` closes its chain parent first, then child) is timed
  stage by stage: each joint moves from rest to rest in its own stage, and the motion rests for an instant where one
  hands over to the next. The note says `in 2 stages, 0.441 s + 0.644 s, at rest between them`; the binding joint
  still reaches the asked acceleration.
- **Through three or more poses** (`Cut = pose(steps) { $Cock, $OverTheTop, $Land }`, then `$Cut [accel=80%]`): each
  bone is joined through the poses on one smooth path, so a joint that keeps its direction passes the middle pose at
  speed (one arc, not three moves) and a joint that turns back stops there for an instant, as a real one does. The note
  says where each pose is passed: `joined through 3 poses of $Cut (pose 2 at source 0.544, reached at 0.231 s, on
  the travel's clock)`. A bone that also moves another way (a clip-time swing on it, an override, a source clip) is
  not joined; it rests at each middle pose, and the note names it and why.
- **Each arm is driven by its hand** (MOVE-ENDPATH1, user 2026-10-16: *"the drive logic should make a trajectory with the ends
  of the chain, not all twist and folds of all the joints equally"*): in a timed travel, two poses or more, an amount form
  too, a limb's end (the IK rig's goal bone of an elbow or knee chain) is planned first, a turn about the limb's top (the
  shoulder) through the end of every pose, one plane for two poses, and the upper and lower bones follow it, the hand riding
  the forearm. The note says it: `arm_r: its end hand_r is driven on a path, a 145 degree turn about upperarm_r, 47-52 cm out,
  its planned points within 0.3 cm of one plane`. The hand's distance from the shoulder follows the limb's BEND joined through the
  poses, not the distance itself (2026-10-25), so a limb that straightens at a pose -- every strike's contact -- stops there for an
  instant as a real elbow does; joined as a distance it swung through straight with no stop, and the effort clock stretched a 0.665 s
  punch to 2.32 s. Its bones are solved against the trunk where the trunk's own lead puts it; while
  the limb holds before or after its turn, the trunk carries it. A limb whose end a line holds (a plant, a reach, an aim) keeps its joints' travel, and so does
  one whose bones play different leads or lags (`phase=(0, lowerarm_r=0.1)` is a whip): give the arm ONE lag behind the trunk
  to drive it by its hand. Leaving a pose close to the body, a turn about the shoulder can hug it; an in-between pose that
  carries the hand out in front gives the path its shape (BlueEdge's rise, Docs/GC-MOVE-END-PATH-PLAN.md).
- **Shape the speed along the travel: `[pace=]`** (`$Cut [accel=80%, pace=(0.25, 1)]`): each stretch plays at the share of
  the move's fastest speed you write for it. The gather (the first stretch, up to the middle pose) plays at a quarter of
  the cut's speed, and the cut plays the fastest point. The move holds each stretch's speed between a run-up from rest and
  a run-down to rest, so a paced list keeps its pace and no longer has the bell's shape. A pace is relative and written
  0..1, never `%`: 1 is the move's own fastest point, the speed of the whole pose (all the joints the travel turns,
  together), not the rig's top speed, and only the ratio of the values counts (the largest is taken as 1). Write one value
  per stretch between the steps' poses, or `pace=$Profile` over the travel as written (pose k of n at k / (n - 1)). Where
  the pace rises, the joints speed up at the real acceleration, never in a jump.
- **Choosing the numbers.** Think of the ratio between stretches, not of a speed: the largest value counts as 1, so
  `(0.25, 1)` and `(0.125, 0.5)` ask the same. Give 1 to the stretch that should be fastest and each other stretch the
  share of it that it should play. Example, `Paced = clip(frames=24) { $Through [accel=80%, pace=(0.25, 1)] }`
  (`Content/GraphCodeExamples/Kinematics_20261009/PoseEffortPaced.gcmove`; `$Through` is three poses, two stretches). The
  build plays it in 2.982 s: a run-up of 111 ms, the gather at 39.2 deg/s, the cut at 157.8 deg/s (0.249 of it), a
  run-down of 219 ms. The note: `a flat speed would play (0.249, 1.000) of the move's fastest point, the move plays
  (0.25, 1.00)`. The same travel with no pace takes 1.844 s at `accel=80%`: the slow gather costs time, because a paced
  list holds its top speed instead of the bell's single peak. Read the note's `a flat speed would play (...)`: it is what
  the build holds each stretch to, within 5%, and it is your value up to the path's own geometry (0.242 on a path with a
  long gather, 0.277 where one joint stays still). `accel=` sets how fast the top can be. A value under 0.05 is raised to
  0.05 and the stretch plays about 0.11 (`pace=(0, 1)`: 11.6 s). What did not change: a list whose values are all equal, a
  profile that is 1 except at its momentary stops, `pace=Preset.Smooth` and a list with no `pace=` keep the bell and the
  length they had.
- **Or name a preset: `pace=Preset.Smooth`** (`Preset.Linear`, `Preset.Smooth`, `Preset.EaseIn`, `Preset.EaseOut`), a speed
  curve the build knows. `Smooth` is the speed of a minimum-jerk move, a bell that leaves and arrives slowly with the least
  change of acceleration, the one to reach for when a motion looks jerky; `EaseIn` is its rising half then full speed,
  `EaseOut` full speed then its falling half, `Linear` one speed throughout. `Smooth` keeps the bell, as an even list does; the other three ask a shape and are timed like any pace, held between a run-up from rest and a run-down to rest, so `Linear` is one flat speed and the clip's length changes with it. ~~**On a walking leg** (a stride that follows
  `[clock=root]` and plants its paw) the stance is the paw's, carried back by the body, so the preset shapes the SWING
  alone, from lift-off to landing: `$StrideL [clock=root, pace=Preset.Smooth]` leaves the floor and lands at the
  stance's own speed and swings in one bell between, where a list of shares ramps at each pose.~~ (SUPERSEDED 2026-10-20,
  MOVE-TRAVEL1 B3: the stride rule is removed, and a `[pace=]` on a reference that follows a travel is refused by name; the
  travel times such poses by distance / speed, [WALK-AND-RUN.md](WALK-AND-RUN.md) 6a.) **A pace shapes speed
  ALONG the path, so it cannot slow where the path turns:** a swing that passes in-between poses where the leg changes
  what it does (the knee folds, then the thigh swings forward) is rushed through them at the bell's peak (measured on
  the cat walk, 2026-10-18: knee 12,858 and ankle 14,447 deg/s2 against 8,313 and 10,449 with a profile slow at
  landing and lift-off). There, keep a profile; read the trails graph (`graph:`) before and after.
- ~~**A walking leg with no `pace=` is timed by its joints' acceleration**~~ (2026-10-18; SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B3:
  the least-effort swing is removed with the stride rule; the rest of this bullet is history): its swing plays at the least share
  of the joints' `accel=` references (the `.gcrig`'s) that fits it into its part of the cycle, leaving at the lift-off rate
  and landing at the contact rate, each joint's whole acceleration bounded, its path's turning included, so it slows where
  the swing turns at an in-between pose. The note gives the share and the joint that binds it. Write a `pace=` only to
  shape it otherwise.
- ~~**Each joint of a walking leg keeps its own time in the swing**~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B3: the per-joint
  swing retime is removed with the stride rule; the rest of this bullet is history) (2026-10-18 night; user: "Why back leg still feels so
  robotic", then "fine tune in natural intent so next author can easily make any similar motions following intents. Not
  hard cold numbers"). Every joint passes each pose of the swing when the leg does, as keys in an animation are shared;
  between them each joint moves at its own pace: timed alone, at the least share of its own `accel=` reference that plays
  the swing in its time, then re-timed (a monotone cubic, no overshoot) to cross each pose with the leg and to leave and
  land at the stance's speeds. So a joint turning back (the knee most folded at mid-swing) slows only itself and the hip
  carries on through it. On one shared timing the whole leg slowed wherever one joint turned: it moved pose, slow, pose.
  Timed wholly alone, without the poses, the cat's shoulder blade swung forward before its elbow had folded and the
  forepaw kicked out level: the poses say how the joints go together. Write the swing with the keys the animal has
  (lift-off, the leg passing under the body, landing): a pose that only holds some joints still while another moves (the
  cat's old early swing) slides the leg forward in one bent shape. The note lists each joint's share; a joint that cannot
  play its time alone keeps the shared timing and is named.
- **The run-up and the run-down are planned for:** the move starts from rest and ends at rest, and the rise to the held
  speed and the fall from it last as long as the effort needs; the note gives them (`with a run-up of 111 ms and a
  run-down of 219 ms from rest`). The build keeps a timing only if every joint stays inside the effort AND every stretch
  plays within 5% of its flat value. The note says so (`it plays its pace`), or names the stretch that is off (`it plays
  over its pace in stretch N (X against Y)`, or `under`); read it.
- **What is checked:** the `it plays its pace` note judges the solved timing at its dense samples on the travel's own
  clock; the tests judge the clip's raw keys, each key interval's average turn against the flat value the note prints
  (within 10%). The game plays the
  COMPRESSED clip, whose decoded rotations sit within 0.0001 degree of the keys, so it plays the same pace (measured on
  the five pace clips of the tests, a slow 31 deg/s gather included).
- **Reading a played rotation yourself:** normalize each decoded rotation before measuring an angle. A compressed clip
  decodes float rotations whose length is off 1 by up to ~3e-7, and `FQuat::AngularDistance` does not normalize, so an
  unnormalized rotation reads ~0.06 degree against itself: over one 4 ms key, a phantom 15 deg/s.
- **The level** comes from `accel=` beside it (the time falls out) or, without it, from the clip's `frames=`: the shaped
  motion then fills exactly that time, and a note says when that needs more push than the rig's references.
- **Give the cut the keys it needs:** a clip plays its pace from key to key, so a short clip keeps the shape, but too
  few keys round the cut's peak. Measured on one case: a 2 s motion fitted into 24 frames at 240 fps (0.1 s, 25 keys)
  puts the cut in about five keys and plays its peak 7% under the planned one, the gather unchanged; other clips round
  by their own amount. Give `frames=` the length the motion needs for a crisp peak. To read a pace off the
  keys, take the turn over ONE key interval: a difference across two keys averages a peak a few keys wide with its
  flank.
- **A momentary stop:** a profile that touches 0 at one point (`Dip = profile(0=1, 0.4=1, 0.5=0, 0.6=1, 1=1)`) makes
  the whole travel rest there for an instant, as a real move between two beats does. A 0 over a whole stretch
  (`pace=(0, 1)`, or two 0 stops in a row) is raised to 0.05 of the fastest point and the note says so (`goes under 5% of
  the move's fastest point; it plays no slower than that there`); the stretch then plays about 0.11 of the fastest point
  and the clip is much longer (`pace=(0, 1)` at `accel=80%`: 11.6 s, against 1.8 s with no pace).
- The notes give what each stretch played and the fastest point it is a share of: `pace (0.25, 1) played (0.25, 1.00)
  of the move's fastest point, asked (0.25, 1.00)` and `its pace is the shape of its speed: a flat speed would play
  (0.249, 1.000) of the move's fastest point, the move plays (0.25, 1.00), with a run-up of 111 ms and a run-down of 219
  ms from rest, and it plays its pace`. When a faster timing was inside the caps but did not play the pace, a third says
  so: `a faster timing of 2.727 s is inside the caps and is not taken, because it plays (0.403, 0.342, 0.944, 1.000) where
  a flat speed plays (0.249, 0.211, 0.827, 1.000); it takes 3.052 s`. A collision form timed in stages takes a pace too
  (one note per stage).
- **With lead and lag** (`[accel=80%, pace=(0.25, 1), phase=(0, hand_r=0.08)]`): the pace shapes the travel's own
  clock, and each led or lagged bone plays that clock early or late, so the whole pose's speed is the pace spread by
  the phases, and a momentary stop is passed by each bone at its own time (the hips pause first, the hand last). The
  notes say so.
- **Refused by name:** `[velocity=]` alone; a lead or lag past 0.5; a `[phase=]` on a pose inside a timed one (each
  bone takes one lead or lag for its whole travel: write the phases on the timed reference); an effort inside a pose
  that is already timed; `m/s` (a pose names no chain end); a joint that another timed line
  already times; a pace in `%`; a pace list whose count is not the travel's stretches; a pace list on a reference
  that is not a `pose(steps)`; a pace that is 0 everywhere; a pace inside a pose that is already timed; a pace whose sharp
  change no timing inside the effort can play (`after N rounds no timing stays inside the effort and plays its pace`: ease
  the change over at least 0.04 of the travel as written). A sharp change that can be played, but not at the full effort,
  is built and timed at less (`it is timed at N% of the effort asked`). A pace on a whole cycle's seam, or on a pose
  followed by `travel(accel=)`, is not held yet: the note says `which a held top does not time yet, so the move keeps the
  bell the effort gives`.
- Add `collision(self)` once the path is right. Over a whole blended travel it is slow (a 0.3 s cut held the
  editor 306 s); a clip without it is still audited at the end and refused at the first overlap, which says
  where.

### A strike: name the point and the pose it lands on

A strike makes one point, a fist or a blade's tip, fastest at one pose of a timed travel: the contact (MOVE-STRIKEAT1,
2026-11-12). Declare the point, name it and the contact on the travel, then read the notes:

```gcmove
MTip = point(bone=blade_r, at=(95, 0, 0))           # the point, declared the usual way; at= is in the bone's frame
Cut2 = pose(steps) { $Ready, $Cock, $Contact2, $Finish2, $Ready }
Slash2 = clip(frames=600) {
  $Cut2 [accel=66%, point=$MTip, strike=$Contact2]   # then read the notes the build prints
}
```

On the sword slash this line, with no pace written (it keeps its small hand `phase=` map), plays a 2.02 s clip in which
the tip is fastest 0.016 s after the contact, at 8.9 m/s, with the sword arm at 100% of its reach.

- **`point=$Place` names the point.** It takes a place of the file declared as `point(bone=..., at=(x, y, z))`. Alone it
  changes no timing: it adds notes that say when and how fast the point is fastest, its speed at each pose, and how far
  its limb (found from the IK rig, `arm_r (upperarm_r to hand_r)` on the slash) is extended then.
- **With `point=`, a `pace=` shapes that point's speed, not the whole pose's:** `pace=(0.25, 1)` moves the point at a
  quarter of its fastest speed in the gather. The travel never runs through its poses faster than at the pace's top, so
  a pose where the point barely moves does not slow the whole clip. A pace that asks no shape (`Preset.Smooth`) still
  changes the bell with `point=`: the effort is spread along the point's path.
- **`strike=$Pose` names the contact, and the tool writes the pace.** It needs `point=`, and a pose of the steps that is
  not the first or the last. The point's pace at the poses is then 0.3 through the gather, 0.3 at the pose before the
  strike, 1 at the strike pose, 0.5 at the pose after it and 0.5 to the end; the run-up and run-down to rest are the
  effort's own. These numbers are chosen (a recovery at half the strike's top), not measured. **A `pace=` written beside
  it wins:** `strike=$SHit names the contact; [pace=(0.25, 1, 0.45, 0.2)] is written, so strike= writes none`.

**The poses a strike needs** (learned on the sword slash, 2026-11-12):
- **The contact is the limb at its fullest reach, in the middle of the body's turn** (on the slash: chest square to the
  front, arm straight). Placed earlier, the blade was fastest with the elbow bent 58 degrees and the tip at knee height,
  and the arm came straight 0.24 s later with the tip under 3 m/s. A contact where the point travels little costs time:
  on a test arm whose fist moved 1.5 cm per 1% of the travel there and up to 5.1 elsewhere, the clip ran 3.3 times longer.
- **The pose after the contact goes further the same way, for every big joint.** One clock carries every joint, so a
  joint that turns back at the contact stops the whole travel there: with the sword arm's shoulder and elbow turning
  back, the tip made 21.4 m/s before the contact, 4.0 m/s at it and 11.9 m/s after it.
- **Give the free limb a job** ("Give the off limb a job" above): on the slash the left arm swings back and down as a
  counterweight. It obeys the same rule: no reading turns back at the contact.
- **Eyes on the target cost nothing now.** A head that holds its direction while the trunk turns under it no longer
  sets the travel's clock; nothing is written for it. Measured: a held head 0.711 s, the same turn with the neck still
  0.711 s, the old charge 0.981 s with neck_01 binding; the slash went from 3.58 s, with the head binding, to 2.02 s.

**Reading the notes,** and what to change:
- **Fastest at:** `$Fist, as timed, is fastest at 0.915 s (1.18 m/s) while arm_R (upperarm_r to hand_r) is at 91.0% of
  its reach; it is longest at 1.214 s, where $Fist moves 0.66 m/s: the point is fastest before its limb extends`. Good
  reads `at 100.0% of its longest reach in the next 0.3 s` with no last clause. If not, move the contact to full reach.
  "As timed" is read on the pose clocks: a hold solved later (an IK hold on the arm) is not in it.
- **At each pose:** `$MTip, as timed, at its poses: $Ready 0.000 s 0.01 m/s; $Cock 0.409 s 0.72 m/s; $Contact2 0.980 s
  8.05 m/s; $Finish2 1.310 s 0.73 m/s; ...`. The contact should carry the largest number.
- **The strike's pace:** `strike=$SHit: pose 3 of 5 at source 0.637; $Fist's pace at the poses (0.3, 0.3, 1, 0.5, 0.5)
  (gather 0.3, cock 0.3, follow 0.5, recovery 0.5, chosen: a recovery at half the strike's top), ...; write pace= to
  change it`.
- **The cap:** `the travel never runs through its poses faster than at the pace's top ($SHit, source 0.637, where it
  moves 1.5 cm per 1%), and the cap held over 11% of the travel`. When the top's cm per 1% is far under the note's
  largest (here 1.5 against 5.1), the point travels little at the contact: author the poses either side so it travels
  through it.
- **A stop:** a verdict comes only when a joint turns back at the strike pose, or the point moves there under 0.8 of
  its fastest: `strike=$SHit is a stop: upperarm_r side turns back at $SHit (-0.0, 50.0, 20.0) (at $SCock, $SHit,
  $SBack); $Fist cannot stay fast through the pose, since one clock carries every joint: author $SBack further the same
  way`. The numbers are the joint's readings at the three poses named. Do what it says.
- **Stops at:** a joint that only arrives and stays (an elbow reaching straight) is listed with no verdict: `lowerarm_r
  fold stops at $SHit (110.0, 20.0, 20.0) (at $SCock, $SHit, $SStay); $Fist, as timed, moves 0.24 m/s there, 97% of its
  fastest (0.24 m/s)`. Nothing to change while the share is high (the slash's tip: 94%).
- **The head:** `the head holds its direction while the body turns under it, so the travel is timed by head_held, ...`
  says the held head applied. `the head turns with the neck here, so the neck is charged as before: ...` says it was
  weighed and the neck binds as it did: turn the head against the trunk so it holds, or let it follow less (a third of
  the turn passed, 0.711 s against 0.801 s). `gc.Move.HeldHead 0` turns it off everywhere.

**Refused by name:** `strike=` with no point (`a strike shapes the speed of a named point; write [point=$Tip] beside
it`); a pose not in the steps (`$Nope is not a pose of $Blow's steps`); the first or last pose (`a strike has a gather
before it and a follow-through after it`); a pose passed twice (`a strike names a pose the steps pass once`); a word
written twice (`one travel has one contact`, `one travel measures one point`); either word on a pose inside a timed one
(`a strike is placed on the clock of the reference that names it`); a point with no clock (`needs a clock to measure:
give it accel= or pace=`); a name that is no place, or a frame (`$Palm is a frame; point= takes a point(bone=...)`); a
point no bone of the clock carries (`no bone from it up to the root turns on $Swing's clock`); a point that does not
move (`it has no speed for the pace to shape`) or barely moves at the strike pose (`the point barely moves at the strike
pose; author the poses either side of $THit so it travels through it`); a whole cycle (`a point is not measured on a
whole cycle`); a travel that follows `clock=` (`is not measured by a point`); a pace with a momentary stop, or a
collision form, under a point (`is not shaped on a point yet`).

**Not built yet:** the leads and lags are still yours, by hand with `phase=`. One top speed serves the whole travel, so
a recovery asked fast can hold the top down, and the point's speed at the contact with it; this is being worked on. The
point's speed that shapes the pace is read from the poses alone (clip-time lines and leads and lags left out); the "as
timed" notes give the result. `phase=` cannot make the body go first inside the cut (see "Lead and lag" above); a stagger between poses is planned.

## Describe a form, then fit it

```gcmove
Blade = surface(part=Dagger)
FingerForm = pose(collision=(self, $Blade)) {
  swing($Knuckles, axis=fold, by=90)
  swing($MiddleJoints, axis=fold, by=100)
  swing($EndJoints, axis=fold, by=80)
}
```

The groups and their axes are character data; these are not reserved anatomical names. A thumb
form can be a separate pose, referenced with a later amount curve. Omitting it leaves it untouched.
There is no built-in fist, thumbs-up, close or crouch activity.

The solver first resolves the composed intent, explicit angles and supplied rig limits. It then advances its participating joints
as a coordinated form first, then independently, sweeps each advance, and shortens a blocked
advance to first contact. Touching neighbors can therefore move together. Other
joints can continue. Later frames start from the preceding fitted joints, but their goals still
come from the authored form and amount; weights do not accumulate. Each candidate also checks
interpolation from the preceding output frame. Notes identify constrained samples without
rewriting the requested percentages.
Collision fitting uses that solved placement, so an aim can orient the weapon before fingers
close around it. Residuals measure the fitted result. All output frames, joint limits and
intervening motion are checked before publication; unresolved penetration refuses the output.

`collision=self` selects enabled native Physics Asset pairs. `$Blade` selects that declared mesh's
source triangles. Existing body shapes and exclusions remain authoritative. Where participating
deform bones lack bodies, native skinning can supply temporary convex approximations; diagnostics
identify the inferred bones. Newly inferred shapes exclude their immediate parent at the shared
joint, following native articulated-body defaults; siblings and nonadjacent links still collide.
Existing authored pairs stay unchanged. This does not edit the source Physics Asset. Unsupported geometry,
missing coverage and reference overlaps must be inspected; a coarse collider is not exact skin.

A selected collision context stays active for the whole clip if its pose is used anywhere, so
fading the amount cannot turn off the final swept check. This is deliberately conservative.
The starting pose must already be legal. One case is handled for you: a joint resting on what it touched,
pushed a hair past the allowance by bones its form does not move (a reaching arm solved again), opens
back up to 4 degrees to start clear, and the build note says how far. A collision pose advances toward a form and stops; it does
not plan a detour around an obstacle, find a globally optimal grip, or solve balance.

### Stop, or slide: `collision()` after the poses

```gcmove
ThumbOut = pose() { swing(thumb_opp_r, axis=fold, by=80) }
Grip = clip(frames=48) {
  $Seat@$SeatIn                  # pose(collision=$Gun): stops where it touches
  $ThumbOut@$ThumbOuts           # a plain pose: posed as asked
  reach(thumb_r, contact=ThumbPad, to=$ThumbRest, weight=$ThumbLays)
  collision(self, $Gun)          # then kept clear by sliding
}
```

`collision(...)` is a line of a clip, after the poses it corrects. Its targets are checked over the
whole clip like any collision target, and every bone the clip turns that is in no
`pose(collision=)` is posed as asked, then moved out of what it overlaps by the least turn of its
joints, within their limits. Each sample starts where the one before ended, so where going straight
on would take a part inside something it slides along it toward where it was asked, instead of
stopping where it first touched. At the first frame there is nothing to slide from, so a bone that
starts inside what it touches is turned out by the smallest single-joint turn that clears it, and the
note says which. A bone in a collision pose, or one the clip shifts (a seat), still
stops. The build's note names what slid and how far short anything was left. This is a local slide,
not path planning: a part centred on an obstacle has no side to go round by, and stays where it met it.

### Let the reach slide the held thing: `drive=`

```gcmove
Gun = surface(part=Pistol)
TriggerFace = point(socket=$Gun.Trigger)
# in the clip:
reach(index_r, contact=IndexPad, to=$TriggerFace, weight=$Press, drive=palm_r)
```

A reach without `[share=]` moves only its chain (with a share the body joins it, [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach)), so a target its joints cannot bend toward stays out of reach however the
fit goes. `drive=` names the bone the held part rides (the one `surface(part=)` on it); the build slides
that bone along the part's own length, its +Z, by the least amount that brings the target within the
chain. Where the part is attached is the start and nothing is typed, so a smaller hand rides lower on
the same grip. It is worked out once, before the collision fit, where the reach weighs most, and is the
part's place for the whole clip: the seat and the fingers close on it there. The note says how far and
which way it slid. When a reach still falls short, its note says which way (sideways out of the plane its
first joint folds in, along the chain, or across it) and which joints sat at a limit.

`slide=<cm>` states the distance instead of searching for it, signed: `+` toward the part's working
end, `-` away. Use it when the searched slide costs another finger (on the template pistol the 0.83 cm
that brought the trigger to the pad also put the guard on the middle finger). `collision=none` on a
reach leaves its whole chain out of the part's collision for that line and only reports the depth; it
is how a trigger finger goes on after the hold is made. `pose(source=<a built clip>, bones=$Group)`
takes only the named bones from that clip's last frame, so a finger placed well once is reused.

## Hold an object while moving

Use a native hand/bone/socket attachment as the weapon's transform authority. With `character:`
and `part:` selecting the body, `surface(part=Dagger)` reads the existing static mesh component,
its mesh, and its attachment ancestry. It rejects placement overrides that would disagree with
the visible prop. Move does not create the prop or execute Blueprint construction scripts.

Establish the grip before fitting fingers. A bone name only gives the bone's frame; it does not
locate a handle inside the palm. Where a `.gcrig` owns the hand, author the grip socket in its
`sockets { }` block (`palm_grip_r = palm_r [on=surface, up=-X]`, or `[at=$point, toward=, up=]` at a
measured point; `gc_describe kind:rig block:sockets`); otherwise author a mesh-owned socket on the
skeletal mesh (`gc_describe kind:skeletal_mesh block:sockets`). Calibrate its position and
orientation against the actual palm and handle geometry, build the prop with its origin at the
point the hand takes it by (`.gcshape` `[pivot=<socket>]`), and attach the weapon there with zero
extra offsets. The whole route, step by step from the rig to the held weapon and a walk with it: [HANDS.md](HANDS.md).
Use a project-owned mesh when the source is shared engine content. A socket name by itself does
not prove contact: inspect palm, thumb and every finger in close-ups from several directions.

Rebuild the pose after changing that native attachment, because the collision surface has moved.
Check the open form, completed grip, and motion extremes in `gc_render`, followed by playback.
Collision fitting limits closure at contact; it does not relocate a misplaced handle, guarantee
that every digit touches, or prove that the resulting grasp could support the object.

For an independently moving static prop, animate its native transform in a `level_sequence`
(`gc_describe kind:level_sequence`). Pose references currently address the selected skeleton;
an articulated object can use the same pose mechanism once its joints have been rigged.

Independent targets can instead use:

```gcmove
Blade = surface(mesh=/Game/Props/SM_Dagger, socket=GripSocket, at=(0,0,0))
BodyTarget = point(bone=spine, at=(30,0,20))
```

Here `at=` is a position in the declared attachment frame, evaluated as the body moves. Without an
attachment, it is in the selected component's space. Timing uses nested `at(frame=..., arrive=...)`
controls. Ask `gc_describe` for the installed Move grammar and its supported controls.

Compose the body and arm performance, fit the finger forms, and add IK where there is a spatial
goal. Position and orientation constraints can share a chain if they do not claim the same channel.
Two independent owners of the same IK goal currently refuse; there is no automatic soft-goal
compromise. A general `fixate(..., lock=rotation)` can retain the wrist relative to its parent.
Avoid having the weapon follow the hand while the same hand position follows that weapon.

Aim a mounted object's declared frame when its axis differs from the bone's forward direction:

```gcmove
BladeAxis = frame(socket=Grip_R, toward=Z, up=X)
# Inside the motion, with a declared body-relative HighTarget:
# aim(arm_R, from=$BladeAxis, at=$HighTarget)
```

`from=` must belong to the chain endpoint or its native socket. It supplies the origin and facing;
`at=` remains the target. Omitting `from=` keeps the ordinary bone +X convention. This lets the
same rig aim a blade, tool or other attachment using its authored mount, within the joint limits.
The source origin is refreshed during the solve, including offsets which move as the mount turns.

Validate the result in playback, from more than one angle and with a hand close-up. Verify the
intended posture, joint folding direction, stable attachment, contact, and the timing of the
upper arm, forearm and wrist. Collision acceptance alone does not establish natural motion.
The native compressed animation is also sampled after saving. A `WARNING joint limit` naming
compression means runtime playback deviated from the verified source tracks; review the native
compression settings and the rendered result before accepting it.

## Props that change holder

For a pickup, handover or set-down, declare the Blueprint's static mesh component as
`surface(part=Prop)` and use `attach($Prop, to=<body socket or bone>, at=<time>)` and
`detach($Prop, at=<time>, onto=$Surface)` inside the clip. `onto=` is optional. Ask
`gc_describe kind:"move" block:"objects"` for the exact installed contracts before composing it.
The prop keeps its world transform at the change and follows its new holder from the next frame.
Reach to its declared socket first; changing holder does not bring the prop into the hand for you.

**Fade the reach, never the pose. A `from=reference` pose displaces multiplicatively, so cross-fading it
against the reach it replaces leaks a quarter of whatever is underneath.** The two are not symmetric,
and the guide already says so in both directions: *"Holds (`reach`, `contact`) are not lines and do not
give way"* — a reach weight must be brought down by hand — while a `from=reference` pose makes earlier
lines *"give way as it comes in"* on its own. Weight the pose in **while the reach is still at full**,
hold it at 1, and only then fade the reach; the pose never comes back down, because the layer beneath it
is displaced rather than blended. Write them as mirrored complements and the result is
`t(1−t)·S + (1−t)²·H + t·B`, a `w(1−w)` residue of the base pose peaking at **25% at the crossover** —
measured 2026-10-24 as both arms being tugged a quarter of the way back toward a counter-top grip in the
middle of each handover, worth a 0.35 s wobble and a large share of 126 velocity-step candidates.
Ordering: receiving reach arrives → hold pose comes in under it → reach fades → `attach`.

**Do not fade a carrier pose out as its reach rises.** A carrier exists so the limb solve *starts* near
the answer, and that is as true late in the reach as early. Fade it and the reach's input becomes
whatever is underneath — typically an arm at rest — which leaks straight through at `1 − reach weight`.
The signature is arithmetic and unmistakable: at a reach weight of 0.839 the clip came out **7.2 cm**
short with the hand 45 cm away at rest, and 0.161 × 45 = 7.2. **A residual that equals (1 − weight) ×
(distance to the pose underneath) is not something a solve can improve** — it will reproduce it exactly.
The working reference holds its carrier at `1` for the whole clip.

**A hand-off pose needs `from=reference`, or it stacks instead of replacing — and it will look like the
bug you were trying to fix.** A pose referenced with an `@curve` enters `from=previous` by default, so
it ADDS to whatever the clip already set, exactly like the `swing()` lines you replaced it with.
Measured 2026-10-24, with all five hand-offs written without it: a release pose built from a rest clip
ADDED the rest hand's 12–15° of finger curl to fingers a grip source had already closed, so the hand
did not open — *it closed further*; a withdrawal added +2/−4/+8 to a raised arm and lowered it by two
degrees; a thumb-to-rest added +8/+10/+5 to a thumb already wrapped round the prop and drove it 2.40 cm
into the shirt. The grammar is explicit — *"pose(from=reference) measures its lines from the skeleton's
rest, so as it comes in every earlier line on the bones it names (and their source motion) gives way"*.
`from=reference` combines with `bones=`. **The tell that a channel is stacking is a joint asking far
past a stop for a value nothing in your file wrote** (`hand_r` asking −63.6° against a −49.3° stop):
that is two channels summing, not a number you can re-tune.

**Angles copied from a working file only mean what that file meant by them if they enter from the same
place.** `from=previous` measures a pose's lines from whatever the clip has already set, so the same
`swing(thumb_01_l, axis=fold, by=50)` is a different thumb depending on what is underneath it.
Measured 2026-10-24: a thumb spread lifted verbatim from a working grip stacked its +50 on a rest pose
that a different line had already set, and the build clamped `thumb_01_l` 11.9° past its 20° side stop.
The fix is not to re-tune the 50 — it is `from=reference`, so the number means from rest, which is what
the source file meant. Keep `from=previous` where a step is deliberately built on the one before it (a
spread then a wrap that nets back to about rest is a two-step sequence and only works stacked).

**A whole-clip `pose(source=)` poses bones you did not mean, and the other hand is the usual victim.**
Without `bones=` it *"SETS EVERY BONE ITS SOURCE HAS A TRACK FOR … a bone the source holds at rest is
written too (by=0)"*. So a one-hand grip source also poses the idle hand, and a clip that then brings
that hand in starts with the two hands inside each other — which a collision form cannot start from,
because frame 0 must be legal. Either give the source a `bones=` group, or take the other side back
with a rest pose carrying `from=reference`. The tell is the same one: a warning naming a bone your file
never touches.

**An undeclared axis is locked, not free.** A bone the `.gcrig` declares no limits for has range (0,0)
on the axes it does not declare — the build says so on every one (*"its undeclared side asked 0.0, past
its stop at -0.0 (range -0 to -0)"*). Giving such a bone its own FK owner can leave the collision fit
with a bone it must move out of an overlap and no joint it may turn: *"form fitting exhausted 256 joint
passes before convergence"*. Leave those bones to the limb chain that already fits them. **For a bone
with no declared limits, do not author a 1° `axis=(x,y,z)` nudge just to make `bones=` accept it** —
that is authoring around a diagnostic, and the nudge is clamped to nothing anyway. Declare a real limit
in the rig, or leave the bone out of the group. (The nudge is only legitimate on a bone the rig *does*
give an axis, where it genuinely just creates a track.)

**A withdrawal must be a `pose(source=…, bones=…)`, never a `swing()`.** `swing(by=)` COMPOSES on
whatever the clip has already set, and a clip whose `pose(source=)` carries no `@curve` sets every
bone at weight 1 for its whole length. So `swing(upperarm_r, axis=fold, by=15)` written to bring an
arm down instead ADDS flexion to an arm that is already raised and holding something — it cannot
lower it at any value or either sign. Measured 2026-10-24: a release authored that way rendered with
both forearms up at chest height, the two hands merged and the prop engulfed between them, and two
rounds of re-guessing the numbers (and the sign) only moved it sideways. Only `pose(source=)` SETS.
That needs a source whose arm is genuinely at rest; if none exists, build one — a one-frame clip is
seconds to solve (`KitchenCupStand.gcmove` → `A_StandRest`).

**A hand-off group is the chain the rig declares, intersected with what the source animates.** Both
halves bite. A limb chain can own a bone that does not sound like an arm (`limb_r = upperarm_r ->
lowerarm_r -> hand_r -> middle_metacarpal_r`), and a palm socket a prop hangs off may ride exactly
that bone. But `bones=` is refused for a bone its source leaves alone — *"bones= names
'middle_metacarpal_r', which A_PassKeyBoth does not animate"* — and that refusal is information: a
source that never moved a bone leaves it where the base pose put it, which is where the clip has it
too, so naming it would change nothing. Read the `.gcrig` for the chain, and let the refusal tell you
which of those bones the source actually carries. A one-frame rest clip only writes tracks for bones
it was asked to move, so author every bone a hand-off will name. Where a bone has no declared limits,
drop it from the group rather than nudging it — see the rule below.

**The receiving chain's reach must be fully handed over BEFORE the prop changes hands.** A reach whose
target is a frame on the prop becomes degenerate the instant that prop rides the reaching hand — the
constraint is satisfied by any pose at all, and the solve wanders. The engine refuses this for the
chain that LOSES the prop but cannot for the chain that GAINS it: that reach is legal, merely
meaningless. Fading it across the attach is not enough. Measured 2026-10-24: attach at 0.50 with the
reach fading 0.50–0.56 asked the hand for a point 30 cm to the side and finished 43.7 cm short. Ease
the reach to zero against the measured hold pose, let the hold take the hand, and put the `attach` a
beat later — the hand does not move, it is simply held by a pose instead of a constraint.

**The chain that LOSES the prop must finish before the handover, and it needs somewhere to be handed
off to.** A reach whose `from=` is a frame on the prop is carrying it, and the build refuses that
reach from weighing once another hand holds it, by name: *"reach(limb_r, from=...) weighs at frame
46, where the prop its from= place is on is held by hand_l, not by the chain's end bone 'hand_r':
let its weight rise only after the attach that puts the prop in that hand"*. So ease that reach's
weight to zero before the attach. But a reach falling to zero drops its chain back to whatever FK is
underneath, which in a pickup clip is the grip pose down at the counter — so weight in an ordinary
`pose()` holding the arm where the reach had it BEFORE the reach fades, and let the withdrawal start
from that. Order: receiving hand lands → losing arm handed from IK to FK → `attach` to the new
holder → losing hand opens → losing arm away (measured 2026-10-24, `KitchenCupPass.gcmove`).

**A key cannot start the next clip where a prop is involved.** `pose(source=<clip>)` carries the
BONES and nothing else, so a clip that starts from a key where the body holds something has the body
in place and the prop back at its stage rest. The build says so plainly — *"attach($Cup,
to=palm_grip_l) at frame 12: nothing of palm_grip_l or what it carries is within 5 cm of $Cup"*, and
the first hand started 3.4 cm off it. Author a pickup-and-handover as ONE timeline that attaches at
frame 0 while the hand is still at the prop, with the holder changing part way through; that is why
every held-prop file here starts from its grip pose.

Use an ordinary `reach()` whose target or `from=` is on the prop to follow it across the change,
and keep successive reaches on one chain from weighing at the same time. `collision()` sliding works
beside a prop that changes holder, with or without an IK rig: the fit and every check move the prop
onto its new holder at the change, so the fingers stop on or slide along the cup where the hand holds
it. A reach whose target or `from=` is on that prop is not pulled toward its place by the slide; its
solve's answer stands. A `drive=` slide of another held part works in such a clip. Still refused by
name: a `drive=` slide of the prop that changes holder itself, a reach into one of its holes, and
effort-timed reaches on it. Dry-run the whole clip to check the actual combination rather than
rebuilding a series of partial guesses.

For native game playback, the build emits `GC_Holder_<Prop>` curves and attachment/detachment
notifies; the game must handle them with native AttachToComponent/DetachFromComponent using
KeepWorldTransform. The baked prop motion does not install a runtime attachment handler in the
AnimBP. Read the build's holder list and event names, then verify pickup, transfer and release.
Shape socket facing is local +Z, up is +X; see [GCSHAPE.md](GCSHAPE.md#what-to-read-for-your-shape) before
assuming a prop socket uses actor-forward axes.

**A holder curve is a ladder, and each rung of the game's tick must exclude the rungs above it.**
The curve is 0 for the scene and k for the k-th holder the clip names, so a two-handed pass reads
0 -> 1 -> 2 and never comes back down. Write the tick as the curve ALONE choosing the rung, with the
"have I already done this?" test nested INSIDE that rung. Folding the two into one `AND` is what
breaks it: `curve > 1.5 AND Holder != 2`, with `curve > 0.5 AND Holder != 1` in its else, looks
right and is not, because the first test also fails once the state has reached 2 — and then the
else runs with curve = 2, its unbounded `> 0.5` matches, and the prop goes straight back to the
first hand. Measured 2026-10-24 on `BP_PassPlayback`: `h=2.0 H=1` and `h=2.0 H=2` alternated 34
times each over the last 68 ticks, the cup was re-attached to the right hand on every other frame,
and it rode that hand down out of shot — which reads on screen as the prop VANISHING at the
handover, not as a flicker, because a `KeepWorld` re-attach leaves no pop to see.
**Moving a collision in TIME only moves which bone it hits, when the obstacle is there for the whole
window.** Measured 2026-10-24 on the cup pass. The left thumb spread into the right hand and blocked
all 72 elbow candidates at frame 26 (`thumb_03_l` into `thumb_03_r`, 0.98 cm against a 0.35 cm give).
The spread did start before the giving hand began to open, so it read as an ordering fault and the
curve was moved later. The frame-26 pair cleared exactly as predicted -- and the build was then
REFUSED on `thumb_03_l` into `thumb_01_r` at frames 35-41: the same two hands, a different bone.
The giving hand held the prop from frame 0 to the handover and was not clear until well after it, so
there was no moment in the window for the arriving thumb to open into. A third timing, after the
giving arm was entirely clear, BUILT -- and still blocked every elbow candidate on the same pair at a
frame where the spread curve reads ZERO. **The resting thumb collided on its own.** That is the test
worth copying: push the moving part's curve to zero over the frame that fails, and if the contact
survives, the pose was never the cause and no timing will help. Before re-timing a beat to dodge a
contact, ask how long the thing it hits is PRESENT: if that is the whole window, re-timing buys
nothing and can cost a refusal where there was a warning. Re-time for ORDER (who lets go before who
closes, which is real and worth fixing); change the GEOMETRY for occupancy.

**A prop that disappears at a handover is a playback bug until the tick is instrumented, not a clip
bug.** The clip, the curve, the socket and the solve were all correct here and were each suspected
first; three guesses at the tick were wrong. One `PrintString` of the curve and the state, once per
tick, through a `gc_render ... simulate:true`, settled it in a single render. Reach for that print
before the second guess: a holder curve read wrongly cannot be seen in the frames, only in the log.

## Close a hand: grips and fists that look right

Measured on a rigged hand with skin-hull bodies (a sword, a hammer, an empty fist with the thumb
outside and with it inside). Every rule here came from a result that passed its numbers and still
looked wrong, so look at the result before trusting the build report.

**Ask past the object; collision decides.** Ask each joint for nearly its limit and let collision
stop it where it touches. Asking for the angle you expect it to reach leaves knuckles half open.

**End a `grab` on a prop with `collision($Prop)` — the grab alone does not keep fingers out of it.**
`grab(..., contacts=auto)` asks every joint to the end of its fold range, and a `solid=collision`
prop is ONE CLOSED CONVEX, so the asked pose goes straight through a hollow one. The line after it
is what pushes them back out: with it, each finger reports *"collision form stopped '<bone>' where it
touched"* and the final audit refuses a finger left inside; without it, a left hand closing over a
cup's rim finished with the cup 0.74–1.71 cm inside the index, middle and ring skin. A grab with no
collision line looks like a hollow-prop modelling problem and is not one (measured 2026-10-24; the
working grip in `KitchenCupDrinkKeys.gcmove` has always ended with it).

**Two hands cannot close on a small prop at the same height.** On a 7 cm cup each hand wraps some
130° from its palm, so a second grip beside the first puts its fingers on the first hand's. Four
different frames on the cup's far side were each refused with *"the body blocked every elbow at all
36 tried"*, and when a rim grip above the first hand did solve, it cost 2.3 cm and 15.2° of reach
error with `thumb_03_l` 0.50 cm inside `thumb_03_r`. Author the handover the way a person does it:
the second hand arrives and steadies, the first lets go, and only then does the second close.

**The order of a limb's own joints decides where it ends.** Each joint stops on whatever it meets
first, so a joint that moves too early meets the wrong thing. Inside one pose a chain closes parent
first (knuckle, then middle, then tip). To change the order, split the joints into poses with their
own weights, in the order a real limb uses:

- **Fingers of a fist:** the default knuckle-first, or all three joints at once, gives the tight
  shape. Curling the outer joints first and the knuckles last leaves a looser fist with palm showing.
- **Thumb outside a fist:** after the fingers close, bring it out in front of the palm, roll its pad
  toward the fingers, curl it, and swing it across LAST. Swung across first, while it still lies in
  the palm's plane, it meets the forefinger's side and hangs down it.
- **Thumb inside a fist:** before the fingers, lift its base only a little (about 15°), roll, curl,
  then swing across. A base lifted far stands proud of the palm and props the forefinger open; a
  straight thumb swung across lies under the knuckles and stops the fingers closing.

**A thumb has to roll.** Opposition turns the thumb's pad toward the fingers: about 17° of roll on
its own and 45–60° inward as part of opposition. Without a roll at its base the thumb's curl sweeps
toward the heel of the palm and no angle reaches the fingers. On an imported hand add a hinge that
only twists at the thumb's base (`.gcrig`: a bone at the base's own point, `[twists=(-10, 45)]`, no
`closes=`; `gc_describe kind:rig block:joints`), then turn it with `axis=twist`. Probe the sign with
+40 and −40 before choosing the range.

**Seat a held object by touch.** Attach it at a palm socket with no offset. Start it clear (shift
the bone it rides out of the hand) and let a collision form bring it in until it touches; a thick
handle and a thin one each stop where they touch. Rig bodies built as hulls of the skin with
`physics_asset() [bodies=hull, gives=0.35]` measure that touch on the skin, letting the flesh give
about as much as a real grasp does.

**Place a thumb on a grip by a touch, not by angles.** `contact(thumb_chain, on=$FingerBones,
weight=$Touch)` with no pad named brings any part of the thumb's skin onto any part of theirs. A
contact holds the spot it chose when its weight began to rise, so let typed poses on the same limb
finish first; overlapping them leaves it short.

**Read the stop notes.** `collision form stopped 'X' (frame N: ... 'A' intersects 'B' ...)` names
what each joint met; lines marked `before its contacts` are the form's own pass, before any contact
moved the limb. The depth quoted is the step that was refused; the kept pose is within the allowance.

```gcmove
# Thumb outside: fingers first, then the thumb out, rolled, curled, and swung across last.
Fingers = pose(collision=self) {
  swing($Knuckles, axis=fold, by=55)
  swing($MiddleJoints, axis=fold, by=85)
  swing($EndJoints, axis=fold, by=75)
}
Out = pose(collision=self) {
  swing(thumb_opposition, axis=fold, by=58)
}
Roll = pose(collision=self) {
  swing(thumb_roll, axis=twist, by=40)
}
Curl = pose(collision=self) {
  swing(thumb_middle, axis=fold, by=50)
  swing(thumb_tip, axis=fold, by=70)
}
Across = pose(collision=self) {
  swing(thumb_base, axis=fold, by=-50)
}
Fist = clip(frames=48) {
  $Fingers@profile(0=0, 0.45=1, 1=1)
  $Out@profile(0=0, 0.45=0, 0.55=1, 1=1)
  $Roll@profile(0=0, 0.5=0, 0.6=1, 1=1)
  $Curl@profile(0=0, 0.58=0, 0.7=1, 1=1)
  $Across@profile(0=0, 0.7=0, 0.92=1, 1=1)
}
```

For the thumb inside, reverse the halves: the thumb's four poses first (its base `by=15`), then
`$Fingers`. For a held handle, put a seat pose (`pose(collision=$Handle) { shift(weapon_bone, ...) }`)
before the fingers, use `collision=(self, $Handle)` on the fingers, and end with the thumb's touch.
Bone names, groups, axes and signs above are one hand's; read yours from the rig.

## Hold a pistol: seat by touch, wrap, thumb across, then the trigger

A pistol is held the way a person takes one: the hand opens wide between thumb and forefinger, the
backstrap comes deep into that web, the trigger finger lies along the frame, the other three fingers
wrap under the guard, the thumb swings across and curls round the other side, and only then does the
trigger finger go onto the trigger. The whole hold is one `pose(steps)`; the trigger is its own line
after it. The full recipe, built and looked at, is [HANDS.md, Step 6](HANDS.md#step-6-hold-a-pistol-the-way-a-person-does),
and the files ship in [Examples/HandAndPistol](../Examples/README.md).

1. **The gun names its points, and is never turned in its file.** In its `.gcshape`: `socket(Web, ...)`
   in the backstrap's pocket under the beavertail, `socket(Trigger, ...)` on the trigger's face; built
   `[pivot=Web]` in its own frame (bore along the forearm, top toward the thumb). An angle in the hand
   is a pose, not a turn in the shape.
2. **The hand names its touching areas.** In its `.gcrig`: a chain per finger from its knuckle and
   more than one contact on the finger that works the gun (`IndexPad` and `IndexTip`), and the socket
   the gun hangs on ([RIG.md](RIG.md)).
3. **The clip is the hold, the trigger reach, and `collision(self, $Gun)`.**

```gcmove
Hold = pose(steps) {
  $Open           # the gun 5 cm off the palm, fingers back, thumb spread
  $Seat           # pose(collision=$Gun): the gun comes in until it touches
  $IndexAlong     # pose(collision=(self,$Gun)): the trigger finger on the frame
  $Close          # pose(collision=(self,$Gun)): middle, ring, little, each stops where it touches
  $ThumbAcross    # pose(from=reference): the thumb swung across, straight
  $ThumbCurl      # then curled onto the far side
}
Trigger = clip(frames=48) {
  $Hold
  reach(index_r, contact=IndexTip, to=$TriggerFace, collision=none, weight=$Last)
  collision(self, $Gun)
}
```

What to know about it:

- **Seat by touch, never on a region of skin.** `contact($Gun.Web, on=web_fold_r)` put the gun on the
  region's nearest point, which on this hand rides the thumb; the gun moved as the thumb opened and
  closed, sat 1 cm low, and the little finger could only poke the grip with its tip.
- **The trigger reach is collision-free and comes last.** `collision=none` leaves the whole trigger
  finger out of the gun for that line: nothing stops or re-solves it, and the build reports how deep
  it goes in (*"bones '$Gun' does not stop go THROUGH it"*) instead of refusing. `Last =
  profile(0=0, 0.7=0, 1=1)` keeps it on the frame until the hold is made.
- **Read where the finger ended:** `reach 'index_r': ... it ends 0.82 cm from its place`, and the
  joints at a limit. A short reach in a full wrap is the anatomy: climb the ladder in [HANDS.md,
  "Holding any weapon"](HANDS.md) (touching area, then grip, then the point on the gun), never a joint
  past life.
- A grip clip against a detailed gun mesh takes from 25 s to a few minutes, and a slow clip says where
  its time went (`$Trigger took 24 s to solve: ...`). Try a change on a copy with `frames=6` first.

## Floors and travel

Declare a floor once and reference it from both collision forms and contact constraints:

```gcmove
Floor = ground(z=0)
Form = pose(collision=(self, $Floor)) {
  swing(hip, axis=fold, by=$Stroke)
  plant(leg, on=$Floor, contact=Sole, drive=root, weight=$Stance)
}
```

The bone and chain names come from the rig; `Stroke` and `Stance` are authored profiles.
`Sole` is an existing rig contact on that chain. `contact=` holds the actual pad/sole point;
without it the endpoint bone origin is held. Declare the sole once in the rig instead of moving
the floor up to the ankle. The contact follows endpoint rotation and scale during the solve.
Floor collision tests all covered native body shapes, including shoulders and back during a roll,
against the solid half-space below that component-space height. It checks motion between keys
and allows no initial floor penetration. It does not place a visible floor in the level.

`drive=root` is optional and names the actual top-level root bone. Active planted contacts derive
its translation from the leg form; an airborne leg does not drive travel. Several contacts share
a weighted solve. A single active driving contact determines the translation fully; its weight
still controls the subsequent IK goal. Use a stance profile that actually reaches zero when the
foot lifts. Accumulated travel persists through release, and replant captures a new contact.
Ordinary `plant()` without `drive=` holds a foot under separately authored body movement.

Add `[root_motion]` to the build output if the game should consume the resulting root track.
Runtime movement must also use Unreal's root motion path. Collision and root translation do not
choose a gait, create balance, or invent a roll's rotation and changing contact points. Render
against a stationary visible floor and inspect planted-paw drift, body clearance and release.
