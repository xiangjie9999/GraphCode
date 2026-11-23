# Stairs: one step up, then the next

Step by step, in the order the work is done. Built and looked at on a rigged humanoid
(`Content/GraphCodeExamples/StepUp_20261005/`: `StepUp.gcmove`, `Stairs2.gcmove`, and the viewer
Blueprints `BP_StepUp`, `BP_Stairs` with an 18 cm and a 36 cm box). The grammar reference is
[POSES.md](POSES.md), [ANIMATION.md](ANIMATION.md) and `gc_describe kind:"move" block:"constraints"`;
the walk this grows out of is the last section of [HANDS.md](HANDS.md).

**Scope: this demonstrates authored ascent on known tread heights.** The leg angles below are
values chosen for this fixture, not universal human gait measurements. It does not demonstrate
automatic traversal of a changed staircase or descent. For Blend Spaces, movement ownership and
current slope/runtime limits, read [runtime locomotion](ANIMATION.md#slopes-and-stairs-current-support).

## Common runtime collision setup: a ramp for travel, treads for feet

For a character moving through a game, give native movement and foot queries separate collision targets.
On one staircase mesh, use a simple ramp collision shape and detailed complex tread collision with
`Simple And Complex` and keep the capsule's `bTraceComplexOnMove=false`. Native Character Movement
then queries simple collision; Move's current
`constraints(collision=world, contacts=..., channel=...)` ground-contact rig queries complex collision.
This uses Unreal's existing [collision representations](https://dev.epicgames.com/documentation/en-us/unreal-engine/simple-versus-complex-collision-in-unreal-engine?application_version=5.7).
`Use Simple Collision As Complex` makes the foot query see the ramp; `Use Complex Collision As Simple`
makes the movement query see the steps. Neither setting keeps this separation.

In GC Shape, put the collision on the asset's build line:

```gcshape
build {
  SM_Stairs = $Stairs [collision=$Ramp]
}
```

`Stairs` is the visible stepped mesh; `Ramp` is a closed convex mesh entry through the step noses,
in the same coordinates. The ramp is not drawn. Both move together when the output pivot changes,
and rebuilding updates the mesh's own collision, so a separate hidden ramp component is unnecessary.
For several colliders use `[collision=($Ramp, $Landing)]`. Prefer native box, sphere and capsule
shapes where they fit: the existing `Solids` presets provide `Box(w,d,h)`, `Ball(r)` and
`Capsule(r,h)`, with `h` the total capsule height. Shape recognizes these and uses native primitives;
other convex entries become compact hulls. A concave entry must be split into convex entries.
Set `CollisionTraceFlag=CTF_UseSimpleAndComplex` inside the mesh's existing `.gc`
`settings` → `BodySetup = instanced(...)` value. On a placed mesh in an open level,
inspect `gc_render highlights:["collision"]`, then verify simple movement and complex tread traces.

If the ramp and treads are separate components, let the ramp block the character's movement channel
and ignore a dedicated foot trace channel. Let the treads block the foot channel and ignore the
character's movement channel. Declare that channel in project collision settings and pass its name
through `channel=`. A complex trace can still hit the wrong component if both block its channel.
Verify the actual character query settings and both hit surfaces in the running setup.

A spline can guide intended body travel through the host's existing movement system; its progress
and gait must follow actual accepted movement. Keep one owner of body travel. The spline and ramp
do not decide where the feet are supported.

**Current limit:** selecting the treads does not complete foot placement. The current Move rig lifts
penetrating contact samples; it does not lower a hovering foot, align its sole, retain stance or plan
its next landing and swing past a riser. Those remain part of the supported-stairs work. `Floor` below
is an ordinary name for an animation-space `ground()`, not a special world-surface declaration.
Named `surface(mesh=..., at=...)` entries provide authored finite geometry for baking; they do not
bind the running level's collision. Treat collision setup, baked motion and runtime adaptation as
separate acceptance results.

### Feet on the treads in a game

A walk made for flat ground, with the legs fitted to the steps at runtime (capsule on the ramp, each foot
traced onto its step, the hips held at the height taken when a foot touches down, the back foot settling
at rest), is its own guide with a measured example: [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md).

For the baked example below, three rules decide who owns what:

- **The stairs own the heights.** A tread is a `ground(z=)`; its height is the only number the
  stairs contribute. Nothing else about the stairs is typed.
- **The person owns the legs' motion.** What a leg does as a person climbs (the hip and knee lifting
  the foot clear of the tread, the foot coming down flat onto it, then the leg straightening) is typed
  as the swings, measured from the rig's own axes (`axis=fold`), and it is the same for every step.
- **Nobody owns the body's path.** `plant(..., drive=Root)` takes the body's travel, forward and up,
  from whichever feet are planted. **The knee's extra bend is never typed:** it is what a leg must do to
  hold a foot 18 cm up while the hips are still low, and the IK does it.

## 1. The rig you need

A humanoid `.gcrig` with leg chains from the pelvis to the toe, a sole contact per foot, fold axes and
ranges on hip, knee, ankle and ball, and hull bodies (RifleMan.gcrig has them: `leg_R`, `Sole_R`,
`thigh_r [closes=forward, range=(-30, 120)]`, `calf_r [closes=back, range=(-5, 150)]`). Without a sole
contact a plant holds the ankle, and the foot sinks a sole's depth into the tread.

## 2. One step up

```
Floor = ground(z=0)
Step = ground(z=18)
# The right leg: lifted and swung forward over the first 30 percent, then brought down onto the tread,
# flat, by 40 percent; then it straightens as the body rises over it. The left: standing until 65
# percent, letting go over 0.15 s, then lifting: knee folded and toes up before the hip brings it forward.
HipR = profile(0=0, 0.3=48, 0.4=39, 0.45=39, 0.75=15, 1=0)
KneeR = profile(0=0, 0.15=75, 0.25=75, 0.4=63, 0.45=62, 1=5)
AnkleR = profile(0=0, 0.15=20, 0.3=22, 0.4=24.5, 0.45=24, 1=0)
HipL = profile(0=0, 0.7=-10, 0.8=-8, 0.9=13, 1=25)
KneeL = profile(0=0, 0.7=10, 0.8=20, 0.9=72, 1=90)
AnkleL = profile(0=0, 0.6=0, 0.75=-20, 0.9=8, 1=28)
ToesL = profile(0=0, 0.6=0, 0.7=25, 0.85=0, 1=0)
OnStepR = profile(0=0, 0.4=0, 0.45=1, 1=1)
OnFloorL = profile(0=1, 0.65=1, 0.8=0, 1=0)
SideL = profile(0=1.5, 0.7=1.5, 0.85=5, 1=5)
StepUp = clip(frames=30) {
  swing(thigh_r, axis=fold, by=$HipR)
  swing(calf_r, axis=fold, by=$KneeR)
  swing(foot_r, axis=fold, by=$AnkleR)
  swing(thigh_l, axis=fold, by=$HipL)
  swing(calf_l, axis=fold, by=$KneeL)
  swing(foot_l, axis=fold, by=$AnkleL)
  swing(ball_l, axis=fold, by=$ToesL)
  swing(thigh_r, axis=side, by=1.5)
  swing(thigh_l, axis=side, by=$SideL)
  plant(leg_R, on=$Step, contact=Sole_R, drive=Root, weight=$OnStepR)
  plant(leg_L, on=$Floor, contact=Sole_L, drive=Root, weight=$OnFloorL)
}
```

What each part does:

- `plant(leg_R, on=$Step, …, weight=$OnStepR)`: when the weight rises (40-45%), the plant takes the
  sole where the swings put it, projects it onto `z=18`, and holds it there until the weight is zero.
- **Land the sole ON the tread, flat, by the frame its weight rises: its height, its place and its
  tilt are all the swing's.** A foot the plant catches above the tread is pulled down to it, and the
  body comes down with it by its share of the planted feet: with the swing held at its peak (hip 50°,
  knee 70°) through the landing, this foot was caught 6.6 cm up and the body sank 2.8 cm as it landed.
  The plant holds the height only, so WHERE the foot lands is the swing's too: opening the knee alone
  to lower the foot puts it under the knee, about 9 cm further forward, and the toes ended 18 cm past
  a 28 cm box. Here the hip comes down to 39° and the knee to 63° so the heel lands just inside the box's
  front edge and the toes inside its back edge, 0.8 cm from the tread's height. And the plant keeps the
  foot's tilt from that frame for the whole stance, so the ankle gives back what the hip and knee
  change (24.5° here; flat when hip − knee + ankle stays what it was). The side view of the viewer
  Blueprint, with the box, at the landing frame shows all three. Early in the swing the knee folds
  to 75° and the ankle lifts the toes (20°) by 15%, so the rising toe clears the box's nose.
- **A foot that rolls (2026-10-07):** `plant(..., roll=(Heel_R, Ball_R, Toe_R))` in place of `contact=Sole_R` keeps the
  turn the clip draws instead of the landing frame's, and holds the lowest of the named points; see
  [WALK-AND-RUN.md](WALK-AND-RUN.md). It plants on `ground()` planes, so a tread works, but these stair clips have not
  been rebuilt or measured with it yet. Since the same day a landing foot also lands where the body has carried it,
  which changes every `drive=Root` clip a little: the step-up's largest landing jump is now 2.8 cm. Rebuild and look.
- **The trailing foot lifts before it swings.** A `ground()` is a plane: nothing in the solve knows
  where a riser's nose is, so whether a swinging toe clears it is the swing's to say. Swung forward as
  it let go (hip 30°, knee 60° at the end), the left toe went 7.3 cm into the box's nose and ended 3.9 cm
  inside the next riser, where the next clip starts. Here the hip stays back until the plant has let
  go (0.8), then the knee folds (72° by 0.9, 90° at the end) and the ankle lifts the toes (28°): the toe
  rises 23 cm while moving 3 cm back, crosses the nose 18 cm above it, and ends 18 cm behind the next
  riser. Folding the knee while the plant still weighs drags the body after the foot (worst change of
  speed 0.87 instead of 0.65 cm per frame²): fold it once the weight is 0.
- `drive=Root` on both plants: while the left foot is planted on the floor and the right on the tread,
  the root moves so that neither planted sole slides. As the right leg straightens from 45% on, the
  body rises over the planted foot; nothing types the rise.
- **The stance carries the body over the foot.** The planted leg's hip opens from 37° to 15° by the
  time the other foot lets go (75%) and to 0° at the end, so the body moves forward over the tread the
  whole way. With the hip ending at 12°, the body slid back 8 cm after the push-off: while both feet are
  planted the root follows what both legs ask, and here they asked for places 8.7 cm apart, so when the
  left let go the root moved to the right leg's place.
- `drive=Root` fades the one-frame jolts by itself over 0.2 s (its `blend=` default, since 2026-10-06): the
  root follows the weighted mean of what the planted feet ask, and that mean moves at once when a weight
  does. The blend fades each such jump over the seconds given; a foot that is lifting is judged by where it
  was the frame before it began to leave, so its own lift never drops the body. Letting the left foot go
  over 0.15 s (0.65→0.8) instead of 0.05 s spreads the hand-off too. Measured on this clip's root (change of
  speed per frame, worst / rms; cm): vertical 1.49 / 0.46 without the blend → 0.62 / 0.25 (0.65 once
  the trailing leg folds, above), forward 1.03 /
  0.35, no frame moves the body backward, and no frame drops it more than 0.03 cm. (Written with the swing
  held at its peak and the left let go in 0.05 s, the same step measured 2.80 / 0.79 → 0.68 / 0.29, forward
  1.31, and a 1.1 cm backward frame.) Write `blend=` only to change the time; every driving plant of a clip
  takes the same one.
- `SideL`: the lifting leg steps a little out. At 1.5° on both thighs the build refused the clip
  ("'calf_l' intersects 'calf_r' by 0.40 cm"): the lifting shin passed the standing one too close.
- 30 frames at 30 fps builds in a few seconds and stores at 30 fps.

Seen (side view over `BP_StepUp`'s box, 2026-10-06): at 0.4 s the right foot is down flat on the box,
heel at its front edge, knee bent about 63° and the hips still low; it stays there while the body rises
over it, and by 1.0 s the body stands upright on the step with the left leg folded behind, its toe
above the box and behind where the next riser stands.

## 3. The next step: the next clip, or the same one

Two ways on. This example's `Stairs2.gcmove` is the next clip, started from the previous final frame
(below). A whole flight can also be ONE clip (section 3b): one leg may hold several plants in turn.
The next clip carries the root's place across the join; one clip has no join at all. Here the new
tread is 18 cm higher:

```
Start = pose(source=/Game/GraphCodeExamples/StepUp_20261005/A_StepUp) { }
Step = ground(z=18)
Step2 = ground(z=36)
HipL = profile(0=0, 0.3=23, 0.4=8.4, 0.45=8.4, 0.75=-10, 1=-25)   # the left hip ended StepUp at 25
KneeL = profile(0=0, 0.25=-15, 0.4=-27, 0.45=-28, 1=-85)          # ... and the knee at 90
AnkleL = profile(0=0, 0.3=-6, 0.4=1, 0.45=0.5, 1=-28)             # ... and the ankle at 28
HipR = profile(0=0, 0.7=-10, 0.8=-8, 0.9=13, 1=25)               # the right ended StepUp at 0
KneeR = profile(0=0, 0.7=5, 0.8=15, 0.9=67, 1=85)                # ... and its knee at 5
…
OnStep2L = profile(0=0, 0.4=0, 0.45=1, 1=1)
OnStepR = profile(0=1, 0.65=1, 0.8=0, 1=0)
StepUp2 = clip(frames=30) {
  $Start
  swing(thigh_l, axis=fold, by=$HipL)
  …
  plant(leg_L, on=$Step2, contact=Sole_L, drive=Root, weight=$OnStep2L)
  plant(leg_R, on=$Step, contact=Sole_R, drive=Root, weight=$OnStepR)
}
```

- `pose(source=)` must name a built asset, so the first step is built by its own file first
  (`StepUp.gcmove`, then `Stairs2.gcmove`). It carries the root's place too, so the second clip starts
  standing on the first tread.
- **Every swing in the second clip is added to that start.** A profile's value is the change from where
  the first clip left the joint: the left hip was 25, rises to 48 to clear the tread (`0.3=23`) and comes
  down to 33.4 as the foot lands (`0.45=8.4`, lower than the first step's 39, because the body starts
  this step further over its tread); it ends at 0, so `1=-25`. The knee, folded to 90 at the start,
  opens to 75 and lands at 63 (`0.4=-27`). The standing right foot's plant
  starts at weight 1 on `$Step`, where the first clip left it. The second step is the first with the legs
  swapped, each profile measured from where the first left that joint.
- **A clip's last frame is the next clip's first, so check it against the NEXT riser.** On one box the
  first clip's end looked right; over two boxes its trailing toe stood 3.9 cm inside the second riser
  and the second clip opened with it there. Render each clip over the treads that follow it too.
- A third step is the same file again with the heights moved up one tread and the legs swapped.

Seen (side view over `BP_Stairs`'s boxes, 2026-10-06): the clip opens on the first box with the left
leg folded behind, its toe passes the 36 cm box's nose at least 4.3 cm above it, comes down flat on that
box by 0.4 s, heel at its front edge, and the body is up on it by 1.0 s.

## 3b. A flight in one clip

Plant a foot on the floor and then on its tread with two `plant()` lines on the same leg. Their weights
take turns: one reaches 0 before the other rises, and the frames where both are 0 are that leg's swing.
Two steps up 16 cm risers, each foot landed flat as in section 2 (the swings are omitted here):

```
Floor = ground(z=0)
Tread1 = ground(z=16)
Tread2 = ground(z=32)
OnFloorR = profile(0=1, 0.017=1, 0.033=0, 1=0)
OnTread1R = profile(0=0, 0.2=0, 0.225=1, 0.825=1, 0.9=0, 1=0)
OnFloorL = profile(0=1, 0.325=1, 0.4=0, 1=0)
OnTread2L = profile(0=0, 0.7=0, 0.725=1, 1=1)
Flight = clip(frames=60) {
  …
  plant(leg_R, on=$Floor, contact=Sole_R, drive=Root, weight=$OnFloorR)
  plant(leg_R, on=$Tread1, contact=Sole_R, drive=Root, weight=$OnTread1R)
  plant(leg_L, on=$Floor, contact=Sole_L, drive=Root, weight=$OnFloorL)
  plant(leg_L, on=$Tread2, contact=Sole_L, drive=Root, weight=$OnTread2L)
}
```

- Each plant takes its place from where the swing brought the foot when its own weight rises, and
  forgets it at 0. A chain still has one IK goal, so two plants of one leg weighing on the same frame
  are refused, naming both lines and that frame.
- The body has one root path for the whole flight, rising 33 cm. Measured (2026-10-06), with section 2's
  landing and trailing fold at twice its speed: each foot caught within 2.5 cm of its tread, the root's
  worst vertical change of speed 0.59 cm per frame², its lowest one-frame change −0.17 cm; the two
  landings 33 cm apart; every swinging toe crosses its riser's nose at least 8 cm above it. With the
  swing held at its peak through the landings instead, the feet were caught 8.6 and 9.1 cm up and the
  worst was 0.96.

## 4. For a game, for a render

- `[root_motion]` on the build line hands the travel to the character movement; without it the root
  bone carries it and a preview or sequence climbs off where it plays (HANDS.md, "Carry4").
- Render each clip over a viewer Blueprint with boxes at the treads (`BP_StepUp.gc`: a cube scaled
  `(0.8, 0.28, 0.18)` at `Z=9` is an 18 cm riser, 28 cm deep). The side view at 0.45 s shows whether the
  sole is on the tread or in the air beside it.

## 5. What each refusal means

| Refusal | Why | What to do |
|---|---|---|
| `'calf_l' intersects 'calf_r' by 0.40 cm` | the lifting shin passes the standing one with the feet too close | step the lifting leg out a little (`axis=side`, 3-5°) while it passes |
| `self-collision check … needs more than 512 subdivisions; 'ball_l' travels up to 25.68 cm` | a plant released while the leg's own pose is far from where the foot was held: the foot snaps there. Seen with the rise typed on the root and no `drive=`. | let `drive=Root` move the body so the legs agree with the held feet; release a plant over several frames |
| `'leg_L' is already held by the plant() on line N` | two hold declarations other than plants compete for the same chain (`reach`, `grab`, `aim`, `contact`) | one such line per chain per clip; plants may take turns (section 3b) |
| `the plants on lines A and B both hold 'leg_L' at frame N` | two plants of one leg weigh on the same frame; a chain has one goal | let one weight reach 0 before the other rises |
| `the plants on lines A and B drive one root with different blends` | driving plants of one clip, including ones inherited through `clip(from=)`, fade the root over different times | give them the same `blend=` (unset is 0.2 s) |
| `form fitting needs a legal starting pose` | a foot starts inside a floor it collides with | start the swing above the tread, or plant before colliding |

## 6. What the grammar does not do yet (2026-10-05)

- **It does not find the tread.** The plant projects the foot onto the height you typed, wherever the
  swings put it: a foot short of the tread's edge is held in mid-air at that height, and nothing says
  where the edge is. A `plant()` on a `surface()` (the staircase mesh), with the sole's footprint
  landing on one tread or shifting to the nearest tread that fits, is the next item
  (`Docs/GC-MOVE-ARM-LADDER-PLAN.md` §8, §9.12). Until then the riser heights are typed, one `ground()`
  per tread, and a staircase with other risers is an edit of the heights.
- **It does not know a riser's nose.** A `ground()` is a plane, so a swinging toe may pass through the
  front of a tread and nothing refuses it; the swing must lift the foot before it carries it forward
  (section 2). Look at the side view over the boxes at every frame of each swing.
- **The plant's note says nothing about where it held the foot**, nor how far above its tread it
  caught it. Check the side view at each landing frame: the sole should already be on the tread, flat.
- **At runtime** a baked clip does not adapt to a level's stairs. `constraints(collision=world, …)`
  emits a native rig that traces calibrated contact samples and corrects penetration
  ([MOVE-PHYSICS.md](MOVE-PHYSICS.md)). Clearing those samples does not establish a supported landing:
  the recorded integration can lift a support foot when its footprint straddles a riser. The next
  landing, swing clearance and body progression still need coordinated planning, recorded in
  `Docs/GC-MOVE-ENVIRONMENT-ADAPTATION.md`. A Blend Space supplies/blends motion; it does not close
  those terrain-planning gaps.
