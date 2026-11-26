# Make a pose-based human animation from the presets

Read [GCMOVE.md](GCMOVE.md) first. This guide is the recipe for a strike, a slash, a wave, a reach, a gesture or a reaction of a
human: an action that is a few valid poses and the time between them. It uses the human presets (`Back`, `Head`, `ArmReach`,
`Hand`, `Stance` ...). The language of a preset (values, `none`, `if`, ranges, `max`/`min`, timing a list by effort) is in
[MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md); this guide does not repeat it. A strike with a held thing (a sword) is a
different form: [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md). Every example below is a line from a file that was
built; the file is named. The worked example is the zombie's claw strike, in
`Content/GraphCodeExamples/ParamPoses_20261116/ZombieClaw/`.

## The recipe in ten lines

1. Copy the starter (`Starter_Keys.gcmove`, `Starter.gcmove`) and change its character lines, `into:` and `source=` folder (step 0). Its `presets:` line brings every preset; you paste nothing.
2. Decide TWO poses: the loaded pose and the end of the move. Add a third only where the path between must bend (a contact).
3. Write each pose as one `clip(frames=1)` of preset calls: `$Stance`, `$Back`, `$Head`, `$ArmReach` (one call per side),
   `$Hand`. Every value inside its range.
4. Build the keys for real. Read what the build says it could not reach ("N short"). Lower that value.
5. Render each key from the front, the side, three-quarter and the top, and LOOK. Numbers support the picture; the picture wins.
6. For a bare hand or foot, build the contact key ON the target: halfway blend of the two outer keys plus `reach(...)` of the
   striking point onto the target. Change the outer keys (not the contact) until the striking limb goes one way through it.
7. Write the cut: the built keys as `pose(source=..., from=reference)`, timed `[effort=$effort%, peak=$peak, point=$P,
   fastest_at=$Contact]`, ONE effort, with the clip's own values and ranges.
8. Dry-run the cut. Read the notes: the point's top speed must land on the contact. Then render trails with the speed graph.
9. The graph is the gate: one hump, no spike, head and free hand well under the point's speed, clean feet, the length asked for.
10. Open it in the pose tab, tune by sliders (Apply keeps a trial, Save keeps it for good), Play. Only then show a video.

## Step 0: copy the starter

The human presets ship with the plugin as one library file. Your file brings it in with ONE header line, so your file holds only
your calls and a correction of a preset reaches every file:

```gcmove
presets:/GraphCodeMove/Presets/Human/HumanBody
```

(The `presets:` line needs the Move build published on or after 17 Nov 2026. It takes `presets:(/A,/B)` for several libraries;
your own entry with a library preset's name stands over the library's and the build says so; the build also says when the library
changed since this file's last real build.) The plugin folder `/GraphCodeMove/Presets/Human/` holds `HumanBody.gcmove` (14
presets, 97 ranged values, an empty `build { }`), `Starter_Keys.gcmove`, `Starter.gcmove` and a README. **Copy `Starter_Keys.gcmove`
and `Starter.gcmove` into your own folder.** The starter is a right hook in four keys (guard, wind-up, contact, follow-through)
and the action that plays them. The header of the keys file as shipped:

```gcmove
gcmove 1 character:/Game/GraphCodeExamples/MetaHumanStrike_20261025/BP_StrikeStage part:Body worn:SkeletalMesh
         ik_rig:/Game/GraphCodeExamples/ParamPoses_20261116/IKR_PoseBody
         controls:/Game/GraphCodeExamples/ParamPoses_20261116/PoseBody
         physics_asset:/Game/GraphCodeExamples/MetaHumanBody_20261009/PHAT_ClothedReferenceBody
         presets:/GraphCodeMove/Presets/Human/HumanBody
         into:/Game/HumanStarter fps:30
```

**The lines a person changes, and only these:**

- the character lines, made per character and not shipped: `character:` (your MetaHuman Blueprint) and `part:` (its body
  component), `ik_rig:` (an IK Rig made over your body's mesh), `controls:` (your body's `.gcrig`: each joint's limits and axes,
  read on YOUR mesh), `physics_asset:` (your body's Physics Asset; the build checks limbs do not pass through each other with it).
  The lines above are this project's own, so a file in this project can keep them;
- `into:` (the folder the built keys go to; an absolute package path);
- in `Starter.gcmove`, the folder in its five `source=` lines (`pose(source=/Game/HumanStarter/A_Starter_Windup, from=reference)`),
  and the same character lines and `into:`.

Keep the `presets:` line as it is. Not shipped: the IK rig, the controls, the physics asset, the character, the worked whole-body
poses of `Body.gcmove`, the examples. The presets use a MetaHuman body's bone names; their `rig_` numbers were measured on the
MetaHuman ClothedReference body (another body passes its own, `$Back(lean=20, rig_rest_tilt=-4.1)`).

An action is TWO files: a keys file (the poses, one clip each) and a cut file (the poses read back as built clips, timed). The
reason: one clip cannot hold two aims on one bone in turn. Name them `<Name>_Keys.gcmove` and `<Name>.gcmove`: the tab finds the
action beside a keys file by that name. Build the keys first; the cut reads them as built assets. A strike needs a key after its
contact, so the fist travels THROUGH the contact: the starter's follow-through key is there for that.

## Step 1: choose the poses

- **An action is two or three valid poses and the time between them.** Start with two: the loaded pose and the end of the blow.
  Add a third only where the path between must bend (the contact). Add the stance in and out after.
  *Why:* six keys with an effort each was over-building; the cut that read as "one blow" was three poses.
- **Keep the wind-up short.** At one effort the build turns distance into time, so a big wind-up becomes the fastest thing in the
  clip (`A_ZC_Claw3P_Full`, stance to wind-up, claw 23.57 m/s: "a violent wind-up at the slash's effort").
- **Keep the free hand and the head near where they were.** A part that moves a lot between keys binds the clock or peaks late
  (`upperarm_l` bound a stage at a 58 degree blend). In the claw the free arm stays at 25/20, 27.5/30, 30/110 (raise/azimuth).

The claw: pose A the loaded claw (`KWindup`), pose B the end of the rake low across the body (`KFollow`), pose C the contact (step 4).

## Step 2: write a pose from presets

Degrees count from the figure: raise 0 straight down, 90 level, 180 up; azimuth 0 ahead, 90 out to the limb's own side, 180
behind, below 0 across; tipped forward, to the right and turned to the right are positive. A sided preset takes `left=true,
right=true`; a side written `false` writes nothing, so uneven sides are two calls. The comment above every preset says what each
value means and its range (`HumanBody.gcmove`; the table at the end lists them). Pose A of the claw, as built
(`ZombieClawX_Keys.gcmove`):

```gcmove
KWindup = clip(frames=1) {
  $Stance(forward_r=0.0307, forward_l=-0.0893, out_r=0.15, out_l=0.15, turnout_r=10, turnout_l=10, drop=0.05, weight_side=0.45, weight_fore=0.5, hike=6, lean=10, turn=15)
  $Back(lean=10, round=0.3, side=4, twist=30)
  $Head(jut=0.6, nod=-5, turn=-30)
  $ArmReach(raise=25, azimuth=20, reach=0.85, swivel=30, right=false)
  $ArmReach(raise=140, azimuth=110, reach=0.5, swivel=60, left=false)
  $Hand(knuckle=-0.3, tip=0.4, spread=1, thumb=1, thumb_across=0, thumb_tip=0.5, wrist=-20, left=false)
  $Hand(curl=0.25, fan=0.6, right=false)
}
```

- **Call order:** `Stance` first (feet and weight), then `Back`, `Head`, the arms, the hands. Write the SAME `Stance(...)` feet
  values in every key, or the feet slide between keys.
- **Write only what you mean.** A value not written is not asked (`none`), so the part keeps its rest. Do not write a number to
  "fill in".
- **The arm is by where the hand is:** `raise`, `azimuth` say the line from the shoulder to the wrist; `reach` (0.26 to 1) how far
  along it; `swivel` swings the elbow round that line (a bent arm near the trunk needs `swivel` 30 or more).
- **The shoulder is set on `ArmReach` itself:** `shoulder_forward` (below 0 draws it back and opens the chest, measured 4.0 cm back
  at -1; above 0 rolls it forward, 6.6 cm at 1) and `shrug` (1 raises it 12.0 cm, below 0 presses it down), each -1 to 1. Not
  written, they follow the arm's raise. They act when `raise` or `azimuth` is written.
- **Weight inside the feet:** if the build's balance reading says outside, raise `weight_fore` (toward the balls), do not move the
  hips back. The claw's wind-up went from outside by 0.70 cm to inside by 1.89 cm at `weight_fore=1.0`
  (`ZombieClaw/ZombieClaw_Meter.gcmove` measures a key: `balance(pelvis, over=(leg_l, leg_r), max=0)`).
- **A hand shape:** the example file `Hand.gcmove` (not shipped) has worked shapes (`Rest`, `One`, `Point`, `Claw`, `Relaxed`, `Limp`, `Cocked`...). A hand that
  reads as a fist at a distance needs its fingers apart: the claw is `knuckle=-0.3, tip=0.4, spread=1, thumb=1, thumb_across=0`.

Build the keys (a keys file, one build line per key, `[readings=(0)]` asks for the solved readings):

```gcmove
build {
  A_ZCX_Windup = $KWindup [readings=(0)]
  A_ZCX_Contact = $KContact [readings=(0)]
  A_ZCX_Follow = $KFollow [readings=(0)]
}
```

A dry run is `unreal_gc_apply` with `asset_path` the file as an asset path and `dry_run:true`; the keys are built for real
(`dry_run:false`) before the cut is built. A note such as `'lowerarm_r' could not be turned to look where the file asked - 38.7
short` means the value is past what the body can do (the wind-up's raise 140 reached 109): write the number it read, or change a
neighbour (the swivel), then build again. A value past its range is refused with the way out in the sentence.

## Step 3: look at every pose

A pose is VALID only when its numbers AND its pictures agree. Render each key at `at:0`, four views (these are the real requests;
use the character blueprint of your header):

```json
{"name":"unreal_gc_render","arguments":{"asset":"/Game/GraphCodeExamples/ZombieAttack_20261116/BP_ZombieView",
 "animation":"/Game/GraphCodeExamples/ParamPoses_20261116/ZombieClaw/A_ZCX_Contact",
 "views":[{"name":"C_front","camera":{"azimuth":90,"elevation":8,"zoom":-0.3,"anchor":{"component":"Body"}},"at":0},
          {"name":"C_side","camera":{"azimuth":0,"elevation":5,"zoom":-0.3,"anchor":{"component":"Body"}},"at":0},
          {"name":"C_front34","camera":{"azimuth":45,"elevation":10,"zoom":-0.3,"anchor":{"component":"Body"}},"at":0},
          {"name":"C_top","camera":{"azimuth":90,"elevation":85,"zoom":-0.3,"anchor":{"component":"Body"}},"at":0}],
 "output":{"lifetime":"persistent","path":"Saved/Pictures/C","overwrite":"replace"}}}
```

Look for, in each view: the hand clear of the head and the body (the top view shows depth; a hand that lines up behind the head
in the three-quarter view is fine at 37 cm and not at 10); the feet flat on the floor; the weight over the feet; the face where it
should look; a limb not through the body; and that a stranger would call it "a claw raised to strike", not a bow.
Real rejections: a wind-up that kept the zombie's 50 to 60 degree fold read as a bow with an arm out; a follow-through with the
hand on the groin and the face to the floor; a contact with `raise=75` put the hand over the face from the front (lowered). The
readings support the picture (claw tip 37 cm from the head bone, the elbow's bend, the weight inside); where they disagree, the
picture wins. Have a second pair of eyes (the lead, a critic) look at the four views before anything is timed.

## Step 4: the contact key by construction (a bare hand or foot)

A held thing (a sword) cuts THROUGH a target with a trailing travel. A bare hand, foot or finger cannot (the travel needs a held
leaf with no joint limits): it strikes ON a contact pose. Build the contact key on the target, from the two outer keys:

```gcmove
Floor = ground(z=0.85)
CTarget = point(at=(forward=60, up=118))
MTipR = point(bone=middle_03_r)
KContact = clip(frames=1) {
  $Stance(forward_r=0.0307, forward_l=-0.0893, out_r=0.15, out_l=0.15, turnout_r=10, turnout_l=10, drop=0.05, weight_side=0.45, weight_fore=0.575, hike=4, lean=26, turn=-5)
  $Back(lean=26, round=0.65, side=0, twist=-1.8)
  $Head(jut=0.6, nod=-20, turn=-2.5)
  $ArmReach(raise=27.5, azimuth=30, reach=0.925, swivel=30, right=false)
  $ArmReach(raise=87.5, azimuth=25, reach=0.7, swivel=80, left=false)
  $Hand(knuckle=-0.2, tip=0.6, spread=1, thumb=1, thumb_across=0, thumb_tip=0.5, wrist=0, left=false)
  $Hand(curl=0.25, fan=0.6, right=false)
  reach(limb_r, from=$MTipR, to=$CTarget) [share=0.2]
}
```

- **Halfway blend:** each value of the contact is the middle of the same value in A and B (lean 10 and 42 give 26; round 0.3 and 1
  give 0.65; twist 30 and -33.6 give -1.8). Then `reach(...)` puts the striking point (`middle_03_r`) on the target, which is a
  place in the body's words (60 cm ahead, 118 cm up: the victim's chest). `[share=0.2]` is how far the trunk joins the reach
  (`[share=0]` is refused: above 0 and at most 1).
- **Do not plant the contact key's feet.** The contact's own `plant`s slid the legs (the ankles ended about 15 cm from their keys); the cut plants both feet.
- **The striking limb's joints must go ONE WAY through the contact.** A joint that goes past the contact's value and comes back
  makes the contact a stop. Read the build's line (the cut's dry run prints it):
  `strike=$Contact1K is a stop: upperarm_r side turns back at $Contact1K (-38.6, -35.8, -56.5)`. The three numbers are that joint's
  angle at the pose before, at the contact, and at the pose after: -35.8 lies outside -38.6 and -56.5, so it turns back. **Change
  the outer keys, not the contact.** A few degrees (2.8 on the shoulder's side, 6 on the forearm twist) did not hurt the graph.
- **The reach picks the arm's roll itself:** `swing(..., axis=twist)` lines after `reach(...)` change nothing. To order the roll
  A, C, B, change the outer keys: pose B uses `palm_raise=90, palm_azimuth=-90`, `swivel=50`, `raise=45, azimuth=-95` for the
  forearm; a wind-up swivel alone cannot order it. The follow-through key, as built:

```gcmove
  $ArmReach(raise=45, azimuth=-95, reach=0.95, swivel=50, palm_raise=90, palm_azimuth=-90, left=false)
```

## Step 5: time it

The cut reads the built keys back and times them with one effort. As built (`SB_Words.gcmove`, 0.315 s, the claw's top 0.002 s
after the contact):

```gcmove
Windup0K = pose(source=/Game/GraphCodeExamples/ParamPoses_20261116/ZombieClaw/A_ZCX_Windup, from=reference) { }
Contact1K = pose(source=/Game/GraphCodeExamples/ParamPoses_20261116/ZombieClaw/A_ZCX_Contact, from=reference) { }
Follow2K = pose(source=/Game/GraphCodeExamples/ParamPoses_20261116/ZombieClaw/A_ZCX_Follow, from=reference) { }
Keys = pose(steps) { $Windup0K, $Contact1K, $Follow2K }
ClawSlash = clip(frames=300) {
  $Keys [effort=270%, pace=Preset.FastStop, point=$MClaw, fastest_at=$Contact1K]
  plant(leg_l, on=$Floor)
  plant(leg_r, on=$Floor)
}
```

with `MClaw = point(bone=middle_03_r)` and `Floor = ground(z=0.85)` above it. `frames=` here is only a scale; the clip lasts as long
as the effort says. The older words (`accel=`, `strike=`, `Preset.Attack`, `Preset.Smooth`, `Preset.Linear`) still build; write the new ones.

- **Effort is how hard; the seconds are a result.** The build turns distance into time. Twice the effort makes the stage about
  0.7 times as long. A stage "bound by speed" does not shorten with a higher effort.
- **One effort across a move; different efforts only across a rest** (wind-up, hit, recovery). Neighbouring efforts read as
  "multiple speeds"; the strike pose and any `through=` pose are passed, so their two stretches make one stage (written with
  different efforts, the stage eases from one to the other: keep them close).
- **A list flows by default (23 Nov 2026):** every in-between pose is passed, each joint stopping only where it turns back; write
  `stops=($Pose)` only where the whole body truly stops (a hold, a reversal of the whole body, the jab's fist turning back).
- **`fastest_at=` names the contact, `point=` the striking point.** The build shapes the stage so the point is fastest at the contact.
  `fastest_at=` splits the stage at the contact (each side its own length and curve, one top speed there); `peak_at=` and `strike=` name the contact on the one-bell law, as files before 19 Nov time. A pose is a value, so `fastest_at=$Contact1K` can be a
  clip value too (below).
- **`pace=` is the speed curve:** `Preset.Even` (top in the middle), `Preset.FastStop` (top late, a hard stop: the claw's
  choice, peak 2/3), `Preset.FastStart` (top early, a long settle), `Preset.Flat` (alone, no contact). Not `Preset.EaseIn/EaseOut`.
- **`seconds=` times by length instead of effort:** `$Keys [seconds=0.45, pace=Preset.FastStop, point=$MClaw, fastest_at=$Contact1K]`
  (`SB_Seconds.gcmove`) lasted 0.450 s, and its note said `seconds=0.45: needs 129.2% effort`. Write one of `effort=` or `seconds=`.
- **The library times it for you:** with the `presets:` line, `A_Claw = $FastestAt(poses=$Keys, at=$Contact1K, point=$MClaw,
  effort=270)` (`FA_Preset.gcmove`) gave the same 0.315 s as the line above. It writes only those words; `stop=` takes the curve.
- **`peak=` is the curve as a number** (`Even` 0.5, `FastStop` 0.667, `FastStart` 0.333): the time before and after the contact. Toward 0.74 a longer build
  and a harder stop; toward 0.26 a snap and a long settle. The claw, 270% (measured with the older `accel=270%, strike=` words): `peak` 0.5: 0.304 s, 12.30 m/s; 0.6: 0.310 s, 12.27 m/s, fall 0.100 s;
  0.7: 0.318 s, fall 0.092 s but the left hand reaches 51% of the claw's top (over the gate); 0.74 the same fall, 53%. Keep 0.6.
  The top stays on the contact for every peak. Any number builds; the stage holds it to what the shape allows and says so.
  **A peak belongs to a STAGE, not a stretch.** Beside an effort list it is written one value per stretch, but a stage (the
  stretches between two rests; a passed pose starts none) has ONE top, placed by the first of its stretches written off 0.5.
  A four-pose cross with K1 and K4 passed, `[effort=(100%), peak=(0.5, 0.8, 0.5)]`, leaned the whole stage K0..K6 (held at
  0.75) and left the fist's top where it was (`S39_FourPeak`, 26 Nov 2026). No word places a top inside one passed stretch.
- **A hard stop comes from a short follow-through pose, not from the speed shape.** A pose just past the contact (22 cm for the
  claw: key `KHitStop`, the contact's values blended 0.35 toward B, with its own `reach(... to=$HTarget) [share=0.2]`) stops the
  hand soon after the contact. As built (`ZombieClawClawStrikeStop.gcmove`, 0.317 s, 9.68 m/s, 92% at the contact, fall 0.050 s):

```gcmove
Keys = pose(steps) { $Windup0K, $Contact1K, $HitStop2K }
ClawSlash = clip(frames=300) {
  $Keys [accel=(900%), peak=0.74, point=$MClaw, strike=$Contact1K]
```

  The stop from the top speed to rest took 0.108 s with `peak` 0.5 and 0.092 s at 0.7; the hit-stop pose took 0.050 s. The cost: a
  22 cm stop costs top speed and needs a much higher effort (900 against 270), and the graph puts a red mark on the feet at the
  stop (0.04 cm of movement). **End the action on the hit-stop pose.** A slow settle added after it as a further stage pulled the
  top off the contact on today's engine.
- **Declare the timing as the clip's own values, with ranges**, so the pose tab shows sliders
  (`ZombieClawClawStrikeParam.gcmove`, accepted, in the older words; called with `effort=350` it gave 0.275 s, 13.95 m/s):

```gcmove
ClawSlash = clip(frames=300, effort=270, effort_min=100, effort_max=600, peak=0.6, peak_min=0.26, peak_max=0.74) {
  $Keys [effort=$effort%, peak=$peak, point=$MClaw, fastest_at=$Contact1K]
  plant(leg_l, on=$Floor)
  plant(leg_r, on=$Floor)
}
```

  A value out of range is refused with the way out (`effort=700 ... give a wider effort_max in the call ((effort=700,
  effort_max=700))`).
- **Do not:** time a list of inline preset keys (two keys aiming one arm in one list are refused, and an `$ArmReach`'s upper-arm roll for its forearm is not in the timing: time built keys); use `pace=` or
  `phase=` beside a list; write unequal efforts in one stage; fake a late peak with unequal efforts; pass a whole-body contact
  pose with `through=` for a strike (two bells and a stop).

## Step 6: the graph is the gate

Dry-run the cut (about 5 to 8 s), read the notes, then render the trails with the speed graph (the real request; `of` names the
bones whose speed is drawn):

```json
{"name":"unreal_gc_render","arguments":{"asset":"/Game/GraphCodeExamples/ZombieAttack_20261116/BP_ZombieView",
 "animation":"/Game/GraphCodeExamples/ParamPoses_20261116/ZombieClaw/A_ZC_ClawStrike",
 "views":[{"name":"Strike_trail_side","camera":{"azimuth":0,"elevation":5,"zoom":-0.45,"anchor":{"component":"Body"}},"at":0.304,
   "highlights":[{"type":"trails","of":["hand_r","lowerarm_r","head","pelvis","hand_l","foot_l","foot_r"],"graph":["place","centre"]}]}],
 "output":{"lifetime":"persistent","path":"Saved/Pictures/trails-Strike","overwrite":"replace"}}}
```

(`at` is the clip's length; add views at azimuth 90, elevation 85 for the top and 125 for the three-quarter.) The graph is the
`..._side-graph.png` beside the picture. Read the build notes first:

- `... $MClaw, as timed, is fastest at 0.191 s (12.30 m/s) ...` must be the contact's time. `WARNING strike=...: fastest ... 0.041 s
  before the contact` (the notes still say `strike=` and `accel=` in their advice: read them as `fastest_at=` and `effort=`) says the top is early: move a pose, do not lower the effort (that moves the top further away).
- `strike=... is a stop: ... turns back at ...` : step 4. A refusal inside a library preset (`$FastestAt` with `at=` a rest) names the library's line, not yours.

Then read the graph. **No video goes to anyone until all of these hold:**

| Check | Pass |
|---|---|
| The striking point's speed | ONE hump; no dip deeper than 20% |
| Acceleration | no spike over 1.5 times its neighbours (a wanted hit-stop is a smooth fall, not a spike) |
| Head and the free hand | each under half the point's top (claw: head 35%, left hand 47%) |
| Feet | no red marks; they moved under 1 cm |
| Length | the length asked for |

The video is real time, fixed camera, with the strips and the graph beside it.

## Step 7: tune in the pose tab

Open it from **Window > Level Editor > GraphCode Pose** (or the console `GraphCode.Pose.Open <package> [pose]`). It is a tab
with the panel (sidebar) on the left and its own viewport with a copy of the file's character on the right.

- **The file field** at the top has a search (type any words of the file's name or folder) and a folder scope; it lists the
  project's `.gcmove` files, newest first. The strip under it has the file's clips as chips. A keys file shows its pose chips and,
  beside them, a **Timing** chip (the timing of the action built from those keys). An action file opens with a **Timing** group.
- **Rows.** Each called preset is a panel inside its parent; a row is a tick, a name, a slider, a number. A value you wrote shows
  ticked. A child's value follows its parent until you tick **override** on that row. A row without a range is a plain number
  box. Dragging a pose value poses the body only, when you let go ("Let go to pose"). A refused value is set back; it never
  leaves the tab stuck (Undo or Discard also bring it back).
- **Rig** box (top left): every bone faint, joints at a stop orange, the dragged value's bones bright.
- **Copy and paste.** Right-click a part's header: Copy, Paste onto, Copy to pose. Right-click a row's name for one value. A paste
  makes the target part EQUAL to the source, ticks included (one undo step).
- **Pose values: Apply and Save.** **Apply** builds the keys and then the action built from them (about 20 s on the claw) as a
  TRIAL; Play then plays the trial ("Playing the trial of ... with your changes"). **Save** writes your ticked values at the
  clip's own call and builds the same two things into the real files. A Save after an Apply builds again (the trial is built from a
  copy of the file with other paths, so it cannot be moved into place). **Discard** drops the trial.
- **Play**, pause, and the frame scrubber under the viewport ("0.42 s frame 50 / 277"); they show the trial while there is one.
  Values stay editable while paused.
- **Timing values.** A clip that declares `effort`, `peak` ... with ranges (step 5) shows them as sliders in the Timing group.
  **Changing a timing value builds nothing**: the state line reads "Changed, not saved ... Apply builds the action". **Apply**
  builds a trial of the action only (6 to 16 s measured; the keys are not rebuilt) and the results appear under the sliders:
  "Length", "Whole move 0.264 s at 400%", "Top speed 14.84 m/s at 0.190 s", "Top speed lands 0.003 s after the contact". Play
  and the scrubber show the trial; **Discard** drops it. **Save** writes the values at the build line (the owner's own saved
  line is `A_ZC_ClawSlash = $ClawSlash(hit=200, windup=30)`) and rebuilds the action.
- **Timing rows, one line each:** a row per stage named by its poses ("Stance → Windup"), its effort slider with the stage's seconds
  beside it, then its speed curve (`Even`, `FastStop`, `FastStart`; `Custom` turns the curves into numbers); each in-between pose has one
  choice, Stops / Passes / Fastest here (also on its scrubber label); "timed by" switches between effort and seconds, and timed by seconds
  each stretch has its own seconds slider with its from-to times ("Guard → Cock 0.00-0.40 s") and each stage's effort is the result the
  build found. Hover a row: what it does first, where it is saved in grey (MOVE-PARAMETRIC-POSES.md section 5).
- **Make it adjustable** rewrites a literal timing line as the clip's own values without changing its timing: on the claw action
  its fixed efforts became `windup`, `hit` and `recovery` (one effort across is a single `effort`; a lone peak is `peak`).
- No sliders? The file holds an unranged pasted copy of the presets, or the clip declares no `_min`/`_max`: use the `presets:`
  line and declare the ranges. A file on the `presets:` line gets its sliders from the library: the starter's `KWindup` shows
  `Stance` rows with ranges (`forward_r` -1..1, `out_r` -0.5..1.5, `turnout_r` -35..60), the unwritten ones at their defaults.
- Judge the tuned action with step 6 again (the graph), not by the sliders.

## What goes wrong, and what to change

| Symptom (the owner's words) | Cause | What to change |
|---|---|---|
| "hits his own head" | the hand passes through or near the head: a wind-up hand behind the head, a contact `raise` too high (75 put it over the face), a follow-through arm carried past the temple | change the poses (`ArmReach` raise, azimuth, swivel), not the timing; hand 15 cm or more from the head bone at every frame; look at the frames between keys too |
| "too slow and doesn't have momentum" | every stage is a rise and a stop at rest, and the efforts were low (25 to 100) | two or three poses; ONE higher effort (the claw strike 270); the contact is passed, not rested at |
| "looked like multiple speed" | different efforts on neighbouring stretches (40, then 250, then 35) | write the same effort on every stretch; efforts differ only across a rest |
| "a loose part (the other hand, the head) moves before the rest" | `parts=own` is written (each part then runs its own clock), or the keys ask that part to travel much further than the others | take `parts=own` out (the published engine puts the whole body on one clock); make that part's values closer between the two keys |
| "the animation became so slow" (after Apply in the tab) | a rebuilt action played slower than the file asks: an engine change, not your values (stages came out 0.342/1.618/0.788 s against 0.753/0.757/0.788 s) | dry-run the saved action and compare the length with before; do not Save while it differs |
| "very fast end stop" (wanted) | the speed shape alone barely stops harder: `peak=` took the claw's stop from 0.108 s to 0.092 s | a HIT-STOP POSE 22 cm past the contact gave 0.050 s (`A_ZC_ClawStrikeStop`, `[accel=(900%), peak=0.74, point=$MClaw, strike=$Contact1K]` in the older words, 0.317 s) at the cost of a lower top speed and a much higher effort. End the action on the hit-stop pose: a slow settle as a further stage pulled the top off the contact |
| "can't change back" | a refused value left the tab stuck (fixed 17 Nov: set the number back, Undo or Discard) | set the value back; if the tab still sticks, report it |
| no sliders in the tab | the file holds an unranged pasted copy of the presets, or the clip declares no `_min`/`_max` | `presets:` header; declare `effort`, `effort_min`, `effort_max` on the clip |
| `... is outside its range ...` | a number outside the preset's range | write a value inside it, or widen the end in the call: `$Back(lean=75, lean_max=75)` |
| `'lowerarm_r' could not be turned ... N short` | the value asks more than the shoulder or elbow stop allows | write the number the build read; change a neighbour (`swivel`) |
| `self-collision in ... intersects ... by 1.03 cm` | a limb is inside another (a hand through the other arm) | move the arm out (azimuth, swivel), look at the picture |
| `strike=... is a stop: ... turns back at ...` (`fastest_at=`) | the contact is not between the outer keys for that joint | change the outer keys (step 4) |
| `WARNING strike=...: fastest ... before the contact` (`fastest_at=`) | the follow-through is too near, or the wind-up is the fastest thing | author the follow-through further the same way; shorten the wind-up; do not lower the effort |
| `... share one stage ... write them the same effort` | two efforts inside one stage (the strike and `through=` poses are passed) | write them the same, or let a pose between them be a rest |
| the effort does not shorten the stage | `its speed binds (the top speed its effort gives)` | change the poses (a shorter move), not the effort |
| `the timing could not keep upperarm_r inside the effort after 4 rounds` | a three-stretch list at 900% with a settle (`effort=(%, %, %)`) | time two stretches (A, C, hit-stop) with one value `effort=900%` |
| a call "at its defaults" does not move the part | the preset's value is `none` unless written | write the value |
| the centre of mass outside the feet | the lean puts it ahead of the soles | raise `weight_fore`; do not move the hips back ([MOVE-BALANCE.md](MOVE-BALANCE.md)) |

## Reference: the presets

Generated from the shipped library, `Plugins/GraphCodeMove/Content/Presets/Human/HumanBody.gcmove` (a script reads each preset's values
and `x_min`/`x_max`; "formula" means the end follows another value; `Back`'s `lean` grows with the hunch). Defaults, worked
values and what `none` means are in the comment above each preset in the file. A value `rig_...` is a number of the skeleton:
leave it. A sided preset also takes `left` and `right`.

| Preset | Layer | What it sets | Values and their range |
|---|---|---|---|
| `Back` | first | the hinge at the hips (the pelvis's forward tilt) and the spine's shape (spine_01..05: fold, ... | lean [formula..formula], round [-1..1], lumbar [-1..1], thoracic [-1..1], side [formula..formula], chest_tilt [formula..formula], twist [formula..formula] |
| `Head` | first | neck_01, neck_02, head on fold, side, twist, and the look | turn [-75.5..75.5], nod [-84.4..55.7], tilt [-50.3..50.3], level [0..1], jut [-1..1], look_at |
| `Hips` | first | the pelvis only: lowered, moved, turned and hiked | drop [-0.169..0.5], shift_right [-54..54], shift_forward [-129.7..127.9], hike [-30..30], turn [-60..60] |
| `Stance` | second | the first line of a standing pose | forward_r [-1..1], forward_l [-1..1], out_r [-0.5..1.5], out_l [-0.5..1.5], turnout_r [-35..60], turnout_l [-35..60], knee_out_r [-35..25], knee_out_l [-35..25], heel_r [-25..53.5], heel_l [-25..53.5], knees [0..120], drop [-0.08..0.5], weight_side [-1..1], weight_fore [-1..1], hike [-30..30], turn [-60..60], lean [-58.2..180] |
| `UpperArm` (sided) | first | the collar bone (clavicle fold and side) and where the upper arm points (upperarm aim) | raise [0..175], azimuth [-180..180], shrug [-1..1], shoulder_forward [-1..1] |
| `Forearm` (sided) | first | where the forearm points and where its palm faces (lowerarm aim; its turn about its length ... | raise [0..180], azimuth [-180..180], palm_raise [0..180], palm_azimuth [-180..180] |
| `ArmReach` (sided) | second | the arm along a line from the shoulder and how far out | raise [0..180], azimuth [-180..180], reach [0.26..1], swivel [-60..150], palm_raise [0..180], palm_azimuth [-180..180], shrug [-1..1], shoulder_forward [-1..1] |
| `Hang` (sided) | second | the arms hanging at the rig's rest directions in the figure's frame, so a bow or a slump says ... | shrug [-1..1], shoulder_forward [-1..1] |
| `Hand` (sided) | first | The wrist, four fingers and the thumb of one or both hands, POSED | curl [-2..1], knuckle [-2..1], tip [-2..1], index [-2..1], index_tip [-2..1], middle [-2..1], middle_tip [-2..1], ring [-2..1], ring_tip [-2..1], little [-2..1], little_tip [-2..1], fan [-1..1], spread [-1..1], index_side [-15..11], middle_side [-8..15], ring_side [-15..15], little_side [-15..15], thumb [-1..1], thumb_across [0..1], thumb_tip [-1..1], wrist [-71..83], wrist_side [-16.8..38.5] |
| `FootAt` (sided) | first | a planted foot, POSED (no plant, reach or balance): thigh, calf, foot and ball of that leg, ... | forward [-1..1], out [-0.5..1.5], turnout [-35..60], knee_out [-35..25], heel [-25..53.5], px [-54..54], py [-129.7..127.9], pz [-43.1..14.5], hike [-30..30], turn [-60..60] |
| `Thigh` (sided) | first | where the thigh (hip to knee) points, for a leg OFF the ground (an alternative per side to ... | raise [0..180], azimuth [-180..180], turnout [-53.5..53.5] |
| `Shin` (sided) | first | where the shin (knee to ankle) points, for a leg OFF the ground (an alternative per side to ... | raise [0..180], azimuth [-180..180] |
| `LegReach` (sided) | second | a free leg along a line from the hip joint and how far out | raise [0..180], azimuth [-180..180], reach [0.4..1], swivel [-30..30], turnout [-53.5..53.5] |
| `Foot` (sided) | first | the FREE foot's ankle and toes (foot fold and twist, ball fold), as offsets from where the leg ... | point [-39..61.8], toes_up [-35..65.6], ankle_roll [-27.7..27.7] |

The worked whole-body poses (`Rest`, `ZombieStance`) and the hand shapes (`Rest`, `Claw`, `Relaxed`, `Limp`, `Cocked`...) are not in the
shipped library; they are in `Content/GraphCodeExamples/ParamPoses_20261116/Body.gcmove` and `Hand.gcmove`, as examples to read.

## Checklist: tick all before saying the animation is done

- [ ] Started from the starter: the `presets:` line is kept; the character lines, `into:` and the `source=` folder are yours; no pasted copy of the library.
- [ ] Two or three poses; each a `clip(frames=1)` of preset calls; every value inside its range.
- [ ] The keys are built for real; every "N short" note is answered.
- [ ] Each pose was rendered from the front, the side, three-quarter and the top, and LOOKED at: hand clear of the head and the
      body, feet planted, weight inside, the face where it should look. The picture agrees with the numbers.
- [ ] A bare-limb strike: the contact key is built on the target; no striking joint turns back (or only a few degrees and the
      graph is clean).
- [ ] The cut times ONE effort across, `point=` and `fastest_at=` set, written as the clip's own values with ranges.
- [ ] The notes: the point is fastest on the contact (no WARNING); no "is a stop".
- [ ] The graph: one hump; no spike over 1.5 times; head and free hand under half the point's top; no red marks on the feet; the
      length asked for.
- [ ] Tuned in the pose tab; Apply built a trial, Save kept it, and the saved action was dry-run to the same length.
- [ ] The video is real time, fixed camera, with the graph beside it.
