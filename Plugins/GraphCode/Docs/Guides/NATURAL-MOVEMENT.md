# Author a natural movement: a worked front kick

[Back to animation](ANIMATION.md)

This guide shows how to make a movement look like a person did it, using a `.gcmove` front kick built from
nothing, one layer at a time. It explains why each layer is there, gives starting values, and says what to
read in the build's notes and what to look for in a render. [ANIMATION.md](ANIMATION.md) is the reference for
every verb used here; this page is about using them together.

The worked example is `Content/GraphCodeExamples/NaturalKick_20261011/NaturalKick.gcmove`. It is a
**provisional** example, not a finished natural kick: it shows the method and builds, and it still has two known
rough moments, at the foot's release (about 0.52 s) and at its landing (about 1.34 s), explained in step 5
("Watch the hand-offs"). It builds three clips, so each layer can be compared with the one before:

| Clip | Adds | What changes on screen |
|---|---|---|
| `A_NaturalKick_1_Leg` | key poses, timing, the foot letting go and landing | the leg kicks, but the body stays centred over both feet while it stands on one; nothing holds its weight over the standing foot |
| `A_NaturalKick_2_Weight` | `balance(…)` with its automatic lead | the hips slide over the standing foot before the kicking foot lifts, and back after it lands |
| `A_NaturalKick_3_Body` | the trunk and arms joining the kick | the trunk counter-leans against the raised leg, the arms counter-swing, the parts slightly out of step |

## What makes a movement look natural

These are authoring cues drawn from studies of people doing such actions, not laws every movement obeys. Each
has a tool.

| People usually… | So in GraphCode… |
|---|---|
| take real time: a kick's snap lasts about a tenth of a second, a weight shift a few tenths | write the curves in **seconds** from reference, at a frame rate that can show them (60 fps for fast actions) |
| move the weight over the standing foot **before** lifting the other | `balance(…)` with no `weight=` and no `at=`: it starts the shift `lead=` before the release (0.3 s by default) |
| in fast strikes and kicks, move from the centre out: hip before knee, trunk before arm (not every technique does) | stagger the curves: start the distal joint a little later, or use `[phase=]` (a lead or lag) |
| move the whole body, not one limb | lines that read the action's own curve with `[share=]`, wrapped in a pose whose amount says how much the body joins |
| let a foot leave the floor and take weight back gradually | `plant(…, weight=$Curve)`: let go quickly, take weight back over 0.15–0.2 s |
| prepare, follow through and settle | a small counter-move first, a recoil after, a held end pose |
| stay within what measured people reach | read the build's kinematics notes against measured human references for the same activity |

## Before you start

- **A rig with contacts and a physics asset.** `plant` needs measured sole points (`controls:`) and `balance`
  needs the physics asset's masses (`physics_asset:`). The example uses the Balance rig in
  `Content/GraphCodeExamples/Balance_20261009`.
- **Choose the clip length in seconds, then the frame rate.** The kick is 2.0 s at `fps:60` (120 frames). At
  30 fps a 0.1 s snap is three frames.
- **Write times in seconds first, then divide by the clip's length** to get profile fractions. For a 2.0 s clip,
  0.5 s is `.25`. Keep the seconds in a comment: you will tune them.

## Step 1: block the action in time

Write the key poses as profile stops and place them in time from reference. A front kick has a lift, a chamber
(knee up, shin folded), a snap out, a short hold, a recoil and a set-down.
The snap ends at FULL EXTENSION: the knee straight at contact, the foot's target where the straight leg reaches (user-ruled
2026-10-25: a strike is fully extended; [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md)).

**Leave rest and arrive at rest, and space the stops EVENLY to do it** (measured 2026-10-23, BlueEdge's guard and shove;
`Content/__MCPTests/BubbleKeyAnims_20261023/AnimClips.gcmove`). A `profile()` is piecewise LINEAR, so its slope is the
body's speed and every change of slope is an acceleration. Two things follow, and both cost a build to learn:

- **A first segment with a real slope is a jerk on frame 1.** A shove that went `0=0, 0.03=0.06` left rest at a slope of
  2.0 and then trebled twice in 0.1 s; the clip's own note read `already moving at the clip's start: thigh_l 626 deg/s`
  and its thigh ran at **788% of its 25000 deg/s2 reference**. Easing the opening took that to 148 deg/s and 312%.
- **Stops that CROWD toward a flat are a second-derivative cliff, however smooth the numbers look.** Crowding them to kill
  the last of that start jerk (`... 0.227=0.95, 0.233=1`, into a flat hold) did fix the start — 54 deg/s — but put a corner
  exactly at the two junctions: the shoving hand went from 7194 cm/s2 with **0 velocity steps** to **36352 cm/s2 with steps
  at both of them**. It fixed the leg by breaking the arm.

The shape that beat both, on every measure, is a raised cosine on EVEN stops — `amount = (1 - cos(pi * s)) / 2` sampled
every tenth of each beat. It has zero slope at both ends by construction and no crowding anywhere: thigh 312%, hand
3406 cm/s2, **0 velocity steps** on the hand and the blade. Keep the beats you blocked; only the shape inside them changes.

**A curve cannot buy a beat that is too short.** Across all three shapes the thigh's peak acceleration stopped at ~312-442%
of reference and would go no lower: a 25 degree lean onto planted feet in 0.35 s needs that acceleration whatever the curve
does. When the number will not come down, the beat is the constraint — lengthen it, ask for less travel, or accept it
knowingly (the note says "a reference is this character's 100%, not a limit"). **And do not re-cut a curve that already
measures clean**: the same session reshaped a reach that was already at 72% of reference with 0 steps, and had to put it back.

Read the result on the graph, not by eye: [VALIDATION.md](VALIDATION.md#judge-a-clip-on-its-trails-and-its-graph).

```gcmove
# 2.0 s clip at 60 fps. The left foot lets go at 0.48-0.52 s, the knee chambers by 0.74 s, snaps out by
# 0.86 s, recoils by 1.02 s, and is set down by 1.32 s; the plant takes weight back over 1.34-1.50 s.
LeftPlant=profile(0=1,.24=1,.26=0,.67=0,.75=1,1=1)
FreeLeg=profile(0=0,.26=0,.27=1,.74=1,.75=0,1=0) [linear]
Hip=profile(0=0,.255=0,.30=18,.36=70,.43=88,.46=88,.51=82,.58=40,.64=8,.66=0,1=0)
Knee=profile(0=0,.285=0,.37=95,.43=10,.46=10,.51=80,.58=40,.64=10,.66=0,1=0)
Open=profile(0=0,.265=0,.29=6,.64=6,.67=0,1=0)
Kicking=pose() {
 swing(thigh_l,axis=side,by=$Open)
 swing(thigh_l,axis=fold,by=$Hip)
 swing(calf_l,axis=fold,by=$Knee)
}
Leg=clip(source=/Game/GraphCodeExamples/Balance_20261009/A_Guard,frames=120) {
 plant(support_L,on=$Floor,contact=(S_l_0,…,S_l_14),weight=$LeftPlant)
 plant(support_R,on=$Floor,contact=(S_r_0,…,S_r_14))
 $Kicking@100% [profile=$FreeLeg]
}
```

What each part is for:

- **Each stop is a key pose; two equal stops are a hold.** The curve between stops is smooth and never
  overshoots, so a stop at 88 is the highest the hip goes.
- **The hip leads the knee.** The knee's curve starts 0.03 of the clip (60 ms) after the hip's. Started
  together, the shin folded back while the thigh was still low, and the foot swung into the standing foot: the
  build refused it ("self-collision … 'ball_l' intersects 'ball_r' by 0.41 cm").
- **Hip and knee come back down together.** It is tempting to lag the whole knee curve with `[phase=]` on the
  pose reference. Here that delayed the knee on the way down too, so the foot swung behind into the standing
  leg. Put the knee's late start in its own curve, and let both reach 0 at the same time.
- **The snap is measured, not guessed.** With the snap over 0.08 s the knee's peak acceleration read 54,500
  deg/s², past the 40,000 deg/s² RIG.md suggests for a human knee. Over 0.12 s it
  reads about 30,000, at about 1,000 deg/s (leg-only clip, 60 fps; at 0.1 s the same knee read 38,000 at 60 fps
  but 76,000 when sampled at 120 fps, so leave a margin).
- **The contacts carry the timing of the feet.** `LeftPlant` lets the foot go over 0.04 s and takes its weight
  back over 0.16 s; `FreeLeg` switches the kicking pose on only while the foot is free. Land with the leg already
  straight: a plant that takes the foot while the knee is still bending fights the pose.
- **Keep the free foot clear of the standing one.** This guard stance puts the feet close together. With a small
  hip opening (`Open`, 6°) while the foot is free, the build passes its self-collision check; without it, the
  swinging foot touched the standing foot at lift-off and landing.
- **Write the opening before the fold.** Lines on one bone stack: each turns the bone as the lines above left
  it. Written after a large fold, a side turn is read by the joint limits mostly as twist, about
  atan(tan side × sin fold), and this rig, like many, locks the thigh's twist. Written after the fold, `Open` was
  clamped by 6° (the whole opening); written first, by 1.35° (most likely from the guard pose the clip starts
  in; not isolated). The build says so in a `WARNING joint limit: … its twist asked …` note.

Build it and look from the front: the leg kicks, but the hips stay centred between the feet while the body stands
on one foot. Nothing holds the weight over the standing foot yet.

## Step 2: move the weight first

```gcmove
Hips=bones(pelvis)
Weight=clip(from=$Leg) {
 balance(pelvis,over=(support_L,support_R),bones=$Hips,margin=.5,max=20)
}
```

With no `weight=` and no `at=`, `balance` times the shift from the plants. The build says what it did:

```text
balance lead: (support_L) lets go at frame 29; the weight eases onto (support_R) from frame 10,
there at frame 28, held to frame 90 (18 frames, 0.300 s)
```

The weight starts 0.3 s before the release, is over the middle of the right foot when the left lets go, stays
there while the leg kicks, and eases back over the same span after the left foot takes weight again. 0.3 s is a
starting reference, not a measurement of the body's mass: the pressure under the feet (centre of pressure) starts
moving about 0.3 s before a fast kick or step and 0.5 s before an unhurried step, and the centre of mass follows
it. `lead=` sets when the centre of mass starts moving; tune it by eye. Change it with `lead=0.5s`,
`lead=frame(12)` or a fraction of the clip.

**Choose `bones=` as a person would move.** `balance` finds the smallest change it can make with the joints you
permit, and the smallest change is not always the natural one:

| `bones=` | What happened in the example |
|---|---|
| the whole spine (`pelvis, spine_01, spine_02, spine_03`) | it tipped the trunk sideways, the head about 25° off vertical |
| none | it could not reach the standing foot ("final COM is 4.17 cm outside safe support"): with both feet planted, the hips cannot slide sideways unless the pelvis may turn |
| `pelvis, spine_01` | the head still tilted about 15° through single support |
| `pelvis` (the example) | the hips slide over the standing foot and the trunk stays upright, a few degrees of lean toward the standing foot: what a person does |

Always look at a weight shift from the front.

## Step 3: let the body join

A kick is done by the whole body. Write the trunk and arms as lines that read the kick's own `$Hip` curve, so
they move with it and retiming the kick carries them along:

```gcmove
Trunk=bones(spine_01,spine_02,spine_03)
Arms=bones(upperarm_l,upperarm_r)
KickBody=pose() {
 swing($Trunk,axis=fold,by=$Hip) [share=(-0.05,spine_01=-0.07)] [phase=(-0.02,spine_01=-0.03)]
 swing($Arms,axis=fold,by=$Hip) [share=(0.2,upperarm_l=-0.15)] [phase=0.02]
}
Body=clip(from=$Weight) {
 $KickBody@100%
}
```

- **`[share=]` is how much.** Each spine joint leans back 5% of the hip's angle (the lowest 7%); at the hip's 88°
  that is 6.2° + 4.4° + 4.4°, about 15° over the trunk, the counter-lean a kicker makes against the raised leg. The right arm swings
  forward with 20% and the left back with 15%.
  **The amounts in this example are not a measured kick** (checked 2026-11-10): no source gives a 15° backward lean or an
  arm swing for a front kick. The one trunk measurement is about 15° of flexion with its direction unstated; coaches say the
  trunk is held steady and only reads as leaning back because the hips drive forward, and that the arms stay in the guard.
  Keep the method (`[share=]` and `[phase=]`), take the amounts from the action's own sources, and keep a front kick's lean
  to a few degrees. Sources: `Docs/Research/martial-arts-kick-duration-and-peak-speed.md`, sections 10-12.
- **`[phase=]` is when.** A lead of 0.02–0.03 runs the trunk's copy of the curve over the first 97–98% of the
  clip: the trunk follows the same curve as the hip, a little ahead at every point, by up to 0.02–0.03 of the
  clip (40–60 ms) near the end. Its start moves earlier too, by p·r: `$Hip` stays at rest until 0.255, so the
  trunk leaves rest only 0.005–0.008 of the clip (10–15 ms) before the hip, and both stay still for the first
  quarter. A lag of 0.02 holds the arms' copy for the first 40 ms and lets them catch up by the end; since the
  curve is at rest until 0.255, the arms visibly start p·(1 − r), about 0.015 of the clip (30 ms), after the hip. The offset is not the same
  at every point: an event at fraction r moves by p·r for a lead and p·(1 − r) for a lag. The parts end up
  slightly out of step, centre first. A lag needs a curve at rest at its start and a lead one at rest at its end;
  `$Hip` rests at both.
- **The pose's amount is how committed the body is.** `$KickBody@60%` is a lazier kick with the same timing.

## Step 4: anticipate, follow through, settle

In this kick the weight shift is the anticipation, the recoil and the arms' lag are the follow-through, and the
clip ends on a held guard pose. To go further: add a small counter-move before the lift (a slight dip or a lean
toward the standing foot as the weight goes over), and let the arms overshoot their guard a little on the way
back. Do not leave the body drifting after the action ends.

## Step 5: check it like a viewer

**Read every refusal as advice.** In this example the build refused, in turn:

| The build said | Meaning | The fix |
|---|---|---|
| `self-collision … 'ball_l' intersects 'ball_r'` | the free foot passed through the standing foot | lift the thigh before the knee folds; keep a small hip opening while the foot is low |
| `self-collision … 'foot_l' intersects 'calf_r'` (chamber, recoil) | the shin folded back into the standing leg | start the knee later; keep the thigh up while the knee recoils |
| `balance not attained … final COM is … outside safe support` | the weight could not get over the standing foot | permit the pelvis to turn (`bones=`) |
| `final support not attained … facet_touch` or `below_plane` at the landing | the foot landed while the leg was still bending, or too fast | straighten the leg before the plant takes it; land over at least 0.15 s |

**Read the notes.** These matter for a kick:

- `balance lead: … lets go at frame 29 … there at frame 28`: the weight is over the standing foot before the
  release.
- `final static balance and attained sole support audited at every key and quarter interval`: every frame is
  statically balanced, its centre of mass over the supporting sole. This is a static check: it says nothing about
  momentum, speed or how the body gets from one frame to the next, which is what the next two items and the
  renders are for.
- The balanced clips come out at a finer rate than the header (here 120 fps for `fps:60`): `balance` refines its
  samples and keeps the duration. The note indices for balance use the source rate.
- The kinematics notes, against measured human motion (the example's knee figures are from the leg-only clip,
  sampled at 60 fps; a finer sampling can read a sharp turn higher, so compare like with like):

  | Measured in the example (knee joint, `calf_l` relative to the thigh) | Reference |
  |---|---|
  | 1,035 deg/s at the snap | about 950 deg/s knee extension in expert taekwondo and karate roundhouse kicks (Gavagan & Sayers 2017) |
  | 29,951 deg/s² | RIG.md's suggested human knee reference, 40,000 deg/s² (this example's rig declares no references) |

  Soccer studies report the SHANK segment's own rotation (1,600–1,970 deg/s; about 28,500–33,600 deg/s² before
  ball contact). That is the lower leg turning in space, thigh motion included, not the knee joint's angle: a
  different quantity, so do not compare the knee figures above with it.

- `WARNING joint limit: … clamped`: a joint was stopped by a limit. Usually the pose asks past the rig: change
  the curve or widen the range rather than leave a clamp. If it names a twist you never asked for, look at the
  order of the lines on that bone: a side turn written after a big fold reads as twist (step 1). This example
  still reports a 1.35° clamp on the thigh's twist, most likely from the guard pose it starts in.
- `a candidate velocity step at frame N … the step is not proven`: look at those frames before acting. `balance`
  corrects each frame on its own, and a one-frame correction of a fraction of a degree can read as a large
  acceleration.

**Look at it.** Render the clip from the side and from the front, orthographic, with the same exposure each
time, at full speed and at a quarter speed. Check that the feet do not slide, the head stays up during the
weight shift, nothing passes through anything, and the landing is quiet.

**Watch the hand-offs: the release and the landing.** `balance` moves the centre of mass about 7.6 cm over the
standing foot (the notes' COM positions), the pelvis correction reading about 5–6 cm, so the free foot, hanging
from the shifted hips, sits inside its planted spot. A person opens the hip a few degrees as the weight goes
over, so the lifted foot stays where it was.

- **On your rig, open the hip while the foot is free,** by about atan(hip shift / hip height), a straight-leg
  estimate to start from: 4–5° for a 6–7 cm shift at a hip about 90 cm up; tune it against the render. Use `swing(thigh_l, axis=side, …)`, rising as the foot lets go and back to 0
  before it lands.
- **The opening does not smooth the hand-offs in this example.** With it written first (step 1), the hand-offs
  stay rough, so the opening alone is not their cause. The working diagnosis is `balance` changing its solution
  as the contacts change; that is not yet isolated (it needs the leg, weight and body clips compared at one
  frame rate, raw and as played):
  - at the release, about 0.52 s, the thigh reads about 80,700 deg/s² (measured at 120 fps), the same with the
    opening clamped or working;
  - at the landing, about 1.34 s, `balance` changes its solution between two frames and the foot jumps onto its
    spot in one frame (the toe reads 11.7–14.9 m/s and 1,400–1,800 m/s² there in the notes; with the opening
    clamped the jump was about 10 cm, judged from the renders).

  Both are reported for fixing (`balance`'s continuity at a contact change). Permitting `spine_01` too turned
  the landing into a shorter settle over a few frames (about 6 cm by eye, measured before the opening was
  fixed), but tilts the head about 15°: the posture was the better choice here.
- **Check those frames on every balanced action.** Render a short span at full frame rate around each release
  and landing, and read the kinematics notes there. A planted foot's jitter is in its `*_joint` rows, not its
  `*_place` rows, and the there-and-back spike is its signature:
  [VALIDATION.md](VALIDATION.md#plants-and-landings-read-the-turn-not-the-place).

## When the action is timed by effort: `[clock=]`

The kick is timed by hand because its foot lets go and lands mid-clip, and plant weights run on clip time. For an
action whose contacts do not change during the move (a punch from a planted stance, a throw, a reach), time the
action by effort and let the body ride its clock. (This snippet is a pattern and is NOT built in this example: the
Balance rig declares no `ramp:`, `accel:` or `velocity:`, so `accel=90%` would be refused there. The mechanism
itself is tested natively by `GraphCode.Move.Program.ClockFollow`.)

```gcmove
Punch = profile(0=0, 1=70)
Cross = clip(frames=60) {
 swing(upperarm_r, axis=fold, by=$Punch, accel=90%)
 swing($Trunk, axis=twist, by=$Punch) [share=-0.15] [clock=upperarm_r]
 swing(upperarm_l, axis=fold, by=$Punch) [share=-0.3] [clock=upperarm_r]
}
```

- `accel=90%` times the punch from the joints' limits: the rig must declare its joints' `ramp:`, `accel:` and
  `velocity:` (see RIG.md, "How fast a joint can move").
- `[clock=upperarm_r]` plays each body line on the punch's own clock: the same progress at every frame. Without
  it the body would run on clip time and come apart from the punch.
- A `[clock=]` line sits in the clip, not in a pose; scale it with `[share=]`. Its full rules are in
  [ANIMATION.md](ANIMATION.md), "The body joins an action".

## One clock for the whole body: a run-up into a kick

A kick from a standing guard is timed by the kicking leg. A kick from a run is timed by the **footfalls**: the stride
sets when the support foot lands, the support foot's landing sets when the ball can be struck, and the trunk, arms
and balance follow that one schedule. Synchronising the whole body means writing every part against the same
events, in seconds, and letting the body carry the speed in between.

**1. Write the events first, as times.** From reference (`Docs/Research/run-up-kick-kinematics.md` in this project;
the numbers below are its cited values), for a right-footed instep kick:

| Event | When |
|---|---|
| kicking foot's last toe-off | the run's own, at the end of the approach |
| support (left) foot lands | at **47%** of toe-off-to-contact |
| ball contact | about **0.25 s** after that toe-off, about **0.13 s** after the support foot lands |
| support foot pushes off | about **0.33 s** after it landed |

Place each profile's stops at those clip fractions. Every channel (each leg, the pelvis, the spine, each arm, the
plant weights, the pace) is one profile over the whole clip, so changing one time moves everything written against it.

**2. The footholds are the skeleton of the schedule.** `weight=` on each plant says who stands when; write the
support foot's planted spot **relative to the target**: its heel about 33 cm behind the ball centre and about 10 cm to
the side (measured). The last stride is long (about 1.55 m) and the plant reaches ahead of the body.

**3. Ask the pace the footholds can keep** ([WALK-AND-RUN.md](WALK-AND-RUN.md), step 4b): the stride's own speed through
the approach, slowing over the last stride, and about 1.5 m/s lost on the support foot. A pace that contradicts the
footholds makes the legs stretch where they cannot, and the notes say where.

**4. The strike rides the hip's clock.** The thigh leads, the shank whips out about 70 ms behind
(`swing(calf_r, ...) [clock=thigh_r] [phase=...]`); the knee folds to about 103° in the backswing and opens to about 7°
at contact. The kicking leg is off the floor, so it rides the body: **check contact in world space**, at the contact
frame, with the ball in the scene, because the carried body can pass a few centimetres from where the drawn root was.

**5. The trunk joins, and stays over the hips.** The pelvis turns the kicking hip toward the ball (about 21°) and may
tilt for reach; the spine cancels the tilt so the chest stays over the hips, and comes slightly forward in the
follow-through. A pelvis tilt carried by the whole trunk reads as leaning back.

**6. Balance reads the speed.** `balance(pelvis, over=(leg_L, leg_R), bones=$Posture, margin=.5, max=20)` keeps the
extrapolated centre of mass over the support foot or the next foothold ([MOVE-BALANCE.md](MOVE-BALANCE.md), "Balance
while moving"). It does not brake the run. When it reports large shortfalls at the plant, move the plant, not balance.

**7. Check the synchronisation.** Read the build's notes: each touchdown's distance ahead of the body, the dip at the
support stance, any pace shortfall. Then render a fixed side camera at normal speed and a slow sheet around contact:
the support foot lands beside the ball, the kicking foot meets it within a frame of the written contact, and the body
carries on past the ball, losing speed but never moving backward.

The run-up kick in this project (`Content/__MCPTests/Gait_20261007/Runner/RunKick.gcmove`, written by a generator from
measured strides) is a test fixture for this method, not yet an accepted example.

## Sources for the reference numbers

- Weight shift before a step or kick: Farinelli et al. 2021 (https://pmc.ncbi.nlm.nih.gov/articles/PMC8543010/),
  Moriyama et al. 2024 (https://pmc.ncbi.nlm.nih.gov/articles/PMC10876941/).
- Proximal-to-distal order in kicks, and the thigh slowing as the shank whips out: Estevan et al. 2015
  (https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4633266/), Sørensen et al. 1996
  (https://pubmed.ncbi.nlm.nih.gov/8981287/).
- Knee speeds in kicks: Gavagan & Sayers 2017 (roundhouse kicks, eight experts per art, 500 Hz motion capture);
  soccer shank speeds from Dörge et al. 2002 and Meamarbashi 2010, and shank accelerations before ball contact
  from inertial-sensor measurements of the instep kick.
- The suggested human joint references (the 40,000 deg/s² knee figure): RIG.md, "How fast a joint can move"; the
  example's Balance rig declares none.
- Run-up kick timing, plant placement and the speed lost on the support foot: Augustus et al. 2021, Langhout et al.,
  Ruscello et al. 2020, Hay's support-foot rule, collected with their exact figures in this project's
  `Docs/Research/run-up-kick-kinematics.md`. Balance while moving: Hof 2005 (extrapolated centre of mass), Pratt et al.
  2006 (capture point), Raibert 1986 (running foot placement).
