# Poses with values, ranges and timing a list of poses by effort

Read [GCMOVE.md](GCMOVE.md) first. This guide is for the author who wants one designed pose used at many amounts, who wants
a preset library for the body, or who wants to time a list of poses by effort. It covers four things:

1. a `pose()` or `clip()` that takes values, and how it is called;
2. a value that is not written (`none`), a default worked out from other values, and `if`;
3. a range for a value, and the words `max` and `min`;
4. timing a list of poses by effort: one effort across, rests and passes, and what the notes say.

> To make a pose-based human action (a strike, a slash, a wave, a reach) from the shipped library, follow
> [HUMAN-PRESET-ANIMATION.md](HUMAN-PRESET-ANIMATION.md): two or three valid poses from the presets, looked at, then one effort
> across, the graph as the gate, tuned in the pose tab. The library is used by the `presets:` header
> (`presets:/GraphCodeMove/Presets/Human/HumanBody`), not by pasting copies of its presets into your file.

> A four-legged animal has its own library in this language (`presets:/GraphCodeMove/Presets/Quadruped/QuadrupedBody`, no animal shipped): [QUADRUPED-PRESETS.md](QUADRUPED-PRESETS.md).

Every example below was built (dry run) on the product's build. Every refusal is quoted in the build's own words, without the
`line N:` the build puts first (your file's line numbers will differ). The short form of the first part is also in
[POSES.md](POSES.md#a-pose-with-values).

The examples in parts 1 to 3 sit under this header. Use your own skeleton, rig and bones; every number in them is only an amount
that stays inside this skeleton's joint stops:

```gcmove
gcmove 1 skeleton:/Game/GraphCodeExamples/Balance_20261009/SK_Balance_Skeleton ik_rig:/Game/GraphCodeExamples/Balance_20261009/IKR_Balance into:/Game/__MCPTests/Param1/Docs fps:30
build {
  A_Doc_Bent = $Bent
  A_Doc_Plain = $Plain
}
```

A clip of one frame (`clip(frames=1)`) is a still pose. The build says "barely moves" about it: that is expected.

## 1. A pose or clip with values

A `name=value` in `pose(...)` or `clip(...)` that is not one of that kind's own settings is a value of yours with that default.
The lines of the entry read it as `$name`. A call names the values it changes, by name:

```gcmove
Back = pose(lean=0.0, round=0.0) {
  swing(spine_01, axis=fold, by=$lean * (1 - $round))
  swing(spine_03, axis=fold, by=$lean * $round)
}
Bent = clip(frames=1) {
  $Back(lean=20, round=0.4)
}
Plain = clip(frames=1) {
  $Back
}
```

- `$Back` alone is the defaults. `$Back(lean=20)` changes only `lean`.
- A default is a number, `true` or `false`, a word (a bone name too), a `$Place`, or `none` (part 2).
- Arithmetic (`+ - * /`, brackets, `sin cos tan asin acos atan atan2 rad deg sqrt pow abs floor ceil min max clamp lerp`,
  angles in degrees) is read only in a value that a parameter was written into. A plain `by=20` stays a plain number.
- A call may sit on a body line, in a `pose(steps)` list, in `clip(from=)` and on a build line (`A_Lean = $Back(lean=30)`).
  It takes `@amount` and `[modifiers]` as any pose does, and those may read values too: `$Hand(curl=$grip) @$share%`.
- Names are read as spelled: `$Lean` and `$lean` are two names.
- The build notes name each use with its values and what it worked out, for example `$Back(lean=20) (played on line 23): round not
  written, follows lean: 0.222222222`.

Refused by name, with the way out in the sentence:

- A declared value that no line reads, so a mistyped setting is still caught. `Fist = pose(grip=none) { swing(spine_01,
  axis=fold, by=5) }` gives: `pose() has no 'grip'. It takes steps, from=previous or from=reference, collision= and source=
  ... Nor is 'grip' a parameter of Fist: a name=value beside the settings is a parameter only where the pose's lines read it as
  $grip, and none of them does`.
- A call that names a value the entry does not declare: `$Back(leen=3)` gives `Back declares no parameter 'leen'. Its parameters
  are lean=10 (line 5)`.
- A value with no name: `$Back(4)` gives `'4' is not name=value; a use passes its values by name, $Back(lean=...)`.
- An empty value (`lean=`), a call with values on a pose that declares none, a value given twice, and values
  given to a group, a profile, a place or a wind.
- A division by zero, or a word an expression cannot read, at the defaults or at a use.

A pose can be a value: `at=$Hold`, `fastest_at=$Pose` and `$FastestAt(poses=$Keys, at=$Contact1K, ...)` name poses, and a clip's line can
play a parametric clip with the pose list passed in (part 5). `frames=$n` is not read.

## 2. `none`, formula defaults and `if`

Three states for a value: written; not written, so its default; and `none`, which means "not asked".

```gcmove
Arm = pose(raise=none, elbow=none) {
  if ($raise != none) {
    swing(upperarm_r, axis=side, by=$raise)
  }
  if ($elbow != none) {
    swing(lowerarm_r, axis=fold, by=$elbow)
  }
}
Back = pose(lean=none, round=$lean / 90) {
  if ($lean != none) {
    swing(spine_01, axis=fold, by=$lean * (1 - $round))
    swing(spine_03, axis=fold, by=$lean * $round)
  }
}
Both = clip(frames=1) {
  $Back(lean=20)
  $Arm(raise=40)
}
Nothing = clip(frames=1) {
  $Back
  $Arm
}
Straight = clip(frames=1) {
  $Back(lean=20, round=0)
}
```

- **`none`** is a default or a call value, written as the lowercase word `none`. A pose whose values are all `none` and whose
  lines are guarded writes nothing: `$Nothing` above "moves no bone, so every frame is the rest pose". That is how a part leaves a
  joint alone instead of pinning it to a number of one skeleton.
- **A default that is a formula** of the entry's other values (`round=$lean / 90`) is worked out ONCE, where it is declared. It
  follows the values it reads: `$Back(lean=20)` gives `round` 0.222, and `round=0` written on purpose gives a straight back
  (`$Straight`). When a value the formula reads is `none`, the formula's value is `none` too.
- **`if (...) { lines } else { lines }`** is read as in a `.gcshape`. The branch not taken is as if never written, and the notes
  say which lines were left out: `$Arm (at its defaults): if ($raise != none) is false: line 10 not written`.
- **A switch** is a bare value: `if ($left) { ... }`. `true` is 1 and `false` is 0 in arithmetic. A sided preset writes each side
  under its own `if`, so uneven sides are two calls:

```gcmove
Hand = pose(curl=0.5, left=true, right=true) {
  if ($left) {
    swing(index_02_l, axis=fold, by=$curl * 60)
  }
  if ($right) {
    swing(index_02_r, axis=fold, by=$curl * 60)
  }
}
Uneven = clip(frames=1) {
  $Hand(curl=1, right=false)
}
```

- **`else if`**, **`&&`**, **`||`** and brackets are read in a condition. Comparisons are `== != < <= > >=`; `== none` and
  `!= none` ask whether a value is written.

```gcmove
Bend = pose(amount=30) {
  if ($amount > 60) {
    swing(spine_01, axis=fold, by=9)
  } else if ($amount > 30) {
    swing(spine_01, axis=fold, by=6)
  } else {
    swing(spine_01, axis=fold, by=3)
  }
}
Reach = pose(raise=none, elbow=none) {
  if ($raise != none && $elbow != none) {
    swing(upperarm_r, axis=side, by=$raise)
    swing(lowerarm_r, axis=fold, by=$elbow)
  } else if ($raise != none || $elbow != none) {
    swing(upperarm_r, axis=side, by=10)
  }
}
```

  The notes for `$Bend(amount=45)` read: `if ($amount > 60) is false: line 17 not written; if ($amount > 30) is true: line 21 not
  written` (the first branch is skipped because it is false, the `else` because the `else if` was true).
- A `none` passes down a call: `$Arm(raise=$r)` with `$r` unset leaves `raise` unset in the called pose, so a second-layer preset
  can leave a fact open.
- A condition reads this entry's values, the file's entries, numbers and words.

Refused by name, checked when the file is read (the preset is checked at its own defaults, before anyone calls it):

- A line that reads a value that is not written, without a guard. `Fist = pose(grip=none) { swing(spine_01, axis=fold,
  by=$grip) }` gives: `in Fist (at its defaults), '$grip' reads grip, which is not written (none) there: guard the line with if
  ($grip != none) { ... }, or give grip a default`. A line is never silently dropped: guard it, or give a default. The same
  sentence names a use (`in Lean(by=none) (played on line 8)`) when only a call's `none` reaches the read.
- A default that reads itself, or defaults that read each other in a circle: `the defaults of a, b on P = pose(...) read each
  other in a circle (a -> b -> a), so none of them has a value to start from: give one of them a value of its own`.
- A condition that names something unknown: `'$y > 0': $y is not a parameter here, nor an entry of this file, and a condition
  reads only the entry's parameters, the file's entries, numbers and words`.
- A lone `=` in a condition: `'$x = 3' has a single '=': a condition compares with == (`$x == 3`), and = gives a value only in a
  call`.
- Brackets that do not close in order: `the condition does not balance its brackets: each (, [ and { closes with its own ), ] and
  } in order, and the condition ends at the ) that closes its first (`.
- An `if` with no braces after it: `if (...) is followed by { } holding its lines: 'if ($left) { aim(...) }'`.
- An empty condition: `'if ()' asks nothing. A conditional takes a comparison: 'if ($lean > 30) { ... }', or a switch: 'if
  ($left) { ... }'`.
- An `else` with no `if` before it: `'else' follows the } of an if (...) { ... } block, and here there is none before it`.
- `collision()` inside an `if`: `collision() is a step of the whole clip: write it outside the if`.
- A single `&` or `|` in a condition is refused too (by the specs).

All of a condition's words are checked when the file is read, in a branch taken or not and in a preset nothing calls yet, so a
typo cannot hide.

## 3. Ranges and the end words

Beside a value `x`, two ordinary values `x_min` and `x_max` are its range. They are not new syntax: each is a number, `none`, or
a formula of the entry's other values, written once by the preset's author and overridable in a call like any value.

```gcmove
Back = pose(lean=10, lean_min=-20, lean_max=60) {
  swing(spine_01, axis=fold, by=$lean * 0.1)
}
Round = pose(lean=10, round=0.3, lean_max=90 - 100 * $round) {
  swing(spine_01, axis=fold, by=$lean * 0.1)
  swing(spine_03, axis=fold, by=$round)
}
Far = clip(frames=1) {
  $Back(lean=max)
}
Wider = clip(frames=1) {
  $Back(lean=75, lean_max=80)
}
Formula = clip(frames=1) {
  $Round(lean=max, round=0.5)
}
```

- A number outside the range is refused. `$Back(lean=75)` gives: `lean=75 (the call wrote it) is outside its range for this use,
  lean_min=-20 to lean_max=60: write a value inside it, or, if this body really goes further, give a wider lean_max in the call
  ($Back(lean=75, lean_max=75))`. `$Wider` shows that second way out.
- `lean=max` and `lean=min` mean the end, for that use. The note says so: `$Back(lean=max) (played on line 15): lean=max: 60`.
  The end word is the whole value, lowercase, and works only where the entry declares that end. An end may be a formula: for
  `$Round(lean=max, round=0.5)` the note reads `lean=max: 40; lean_max not written, follows round: 40`.
- A ranged value comes to a number, `none`, or the bare word `max` or `min`, and nothing else. A default outside its own range
  is refused at the declaration.
- An end that is `none` is no end: the other end still holds.
- `x_min` and `x_max` need not be read by any line. But they only count beside a declared `x` of the same entry (names as
  spelled).

Refused by name:

- `$Back(lean=max)` where the entry declares no `lean_max`: `Back declares no lean_max, so lean=max has no meaning here: write a
  number, or declare lean_max`.
- A word: `lean=far` gives `'far' is not a number, and lean has a range: its value is a number, none, or the bare word max or
  min`.
- Arithmetic on an end: `lean=max - 5` gives `an end word stands alone as the whole value (lean=max, lean=min); arithmetic on an
  end is not read: write the number, or declare another value to work it out from`.
- A default past its own range: `a=70 (its default) is outside its range at its defaults, at most a_max=60: write a default
  inside it, or, if this body really goes further, declare a wider a_max`.
- A unit: `lean=50%` gives `a value with a range takes no unit yet: write lean=50`.
- A minimum above the maximum for a use: `$Back(lean_min=70)` gives `lean_min=70 is above lean_max=60 for this use, so no value of
  lean lies inside its range: lower the one or raise the other`.
- The end word in capitals (`lean=Max`) is refused: write it lowercase.

## 4. Presets in layers

A preset library keeps one small preset for each part of the body, then presets that call those, then whole stances, then
actions. Each layer only calls the one below it:

| Layer | What it is | Example |
|---|---|---|
| Part (first layer) | one body part, optional facts, each default `none` or a formula | `Back(lean, round, side, twist)`, `Head`, `UpperArm`, `Hand` |
| Second layer | calls parts, adds a rule across them | `ArmReach(raise, azimuth, reach, swivel)` calls `UpperArm` and `Forearm`; `Stance` calls `Hips` and `FootAt` |
| A stance | a clip of one frame that calls parts with values | `ZombieStance` |
| An action | stances in order, timed by one effort across (part 5) | the zombie's claw slash |

A small second-layer preset, built and dry-run (it reads two parts and one value with a range):

```gcmove
Menace = pose(menace=0.5, menace_min=0, menace_max=1) {
  $Back(lean=20 * $menace)
  $Arm(raise=20 + 40 * $menace, elbow=10 + 20 * $menace)
}
Stalk = clip(frames=1) {
  $Menace(menace=0.3)
}
Lunge = clip(frames=1) {
  $Menace(menace=max)
}
```

(`Back` and `Arm` are the two presets of part 2.) The notes print `$Back(lean=6)` and `$Back(lean=20)`: each distinct set of
values is one plain pose, named as the notes print it.

### The library, and how to copy from it today

`Content/GraphCodeExamples/ParamPoses_20261116/Body.gcmove` and `Hand.gcmove` are such a library on the MetaHuman stage
character, with the presets' own rig (`IKR_PoseBody`, `PoseBody`):

- `Body.gcmove`: `Back`, `Head`, `UpperArm`, `Forearm`, `ArmReach`, `Hang`, `Hips`, `FootAt`, `Stance`, `Thigh`, `Shin`,
  `LegReach`, `Foot`, and the whole-body clips `Rest` and `ZombieStance`.
- `Hand.gcmove`: `Hand`, the fingers and the thumb.
- Above each preset a comment lists its values, their ranges, what `none` means, and worked values that were built.
- Entries marked "helper ... not listed" carry arithmetic for the preset above them: copy them with it.
- A value named `rig_...` is a number of that skeleton (a bone length, a rest direction) with its default. On another skeleton,
  give the call another value.

There is no way yet for one file to use another file's presets. A file carries its own copy. To use the library today:

1. Start your `.gcmove` with the header of `Body.gcmove` (character, `ik_rig`, `controls`, `physics_asset`, `into`, `fps`): the
   presets were measured on that rig.
2. Copy the entries you need, with their helpers, from `Body.gcmove` and `Hand.gcmove` into your file below the `build { }`
   block. Copy the whole entry, comments included: the comment is the preset's description.
3. Write your clips under them and list them in `build { }`. A file made this way (the header of `Body.gcmove`, every entry
   from its `Face` frame down to `Foot`, and one clip `Mine = clip(frames=1) { $Back(lean=30)  $Head(turn=-20, nod=10)
   $ArmReach(raise=70, azimuth=10, reach=0.9, swivel=30, left=false) }`) was dry-run and builds.

Every file that does this holds its own copy, so a fix to a preset in the library does not reach a file that copied it.

The zombie's whole action is an example of this: `Content/GraphCodeExamples/ParamPoses_20261116/ZombieClaw/` holds the keys
(`ZombieClawV4_Keys.gcmove`: five stances, each a clip of the part presets) and the cut (`ZombieClawV4.gcmove`: the five
stances read back as poses and timed in order). The keys must be built first, because the cut reads them as built clips.

## 5. Timing a list of poses by effort

An effort tells the build how hard the body works. `effort=(...)` on a list of poses gives one effort per stretch, a stretch
being the move from one pose to the next. A list of `n` poses has `n - 1` stretches. The older words (`accel=`, `strike=`,
`Preset.Attack`, `Preset.Smooth`, `Preset.Linear`) still build; this part writes the new ones.

### The normal way: one effort across

**Write the same number on every stretch.** The build then turns distance into time: a long travel takes longer than a short
one, and the action reads as one body at one level of exertion. This is how an action is timed unless you have a reason not to.

```gcmove
Fist = point(bone=hand_r, at=(10, 0, 0))
A0 = pose() {
  swing(upperarm_r, axis=fold, by=10)
  swing(lowerarm_r, axis=fold, by=5)
}
A1 = pose(from=reference) {
  swing(upperarm_r, axis=fold, by=35)
  swing(lowerarm_r, axis=fold, by=5)
}
A2 = pose(from=reference) {
  swing(upperarm_r, axis=fold, by=60)
  swing(lowerarm_r, axis=fold, by=5)
}
Back = pose(from=reference) {
  swing(upperarm_r, axis=fold, by=10)
  swing(lowerarm_r, axis=fold, by=5)
}
Four = pose(steps) { $A0, $A1, $A2, $Back }
Cut = clip(frames=240) {
  $Four [effort=(60%, 60%, 60%)]
}
```

(These ran under the MetaHuman header of the library, `fps:240`. A share needs the rig to declare `accel:` for the joint; on a
rig that does not, the build says `accel=40% is a share of upperarm_r's reference acceleration, and the rig declares none for
it. Declare `accel:` in the .gcrig header (or [accel=] on upperarm_r), or state an absolute amount such as accel=3000deg/s2`.)

The build made three stages, one per stretch, `0.098 s + 0.146 s + 0.199 s`, all at 60%. The last stretch turns the arm
furthest, so it takes longest. Write `through=($A1)` to pass `$A1` without stopping:

```gcmove
Cut = clip(frames=240) {
  $Four [effort=(60%, 60%, 60%), through=($A1)]
}
```

That made two stages, `$A0 to $A2 at 60% 0.235 s` and `$A2 to $Back at 60% 0.200 s`.

What the list form gives you even at one effort, over a single `effort=60%`:

- **Declared rests.** Every in-between pose is a rest, so which poses the body stops at is written in the file, not worked out
  from angles. A small change to a pose cannot flip the timing.
- **One rise and one fall per stage**, joined at rest, with no level stretches and no one-sample notches at the poses.
- **Soft ends:** each stage starts and ends with speed and acceleration at zero.
- **Notes that name each stage** (what bound it and how long it took), so you can tune by reading them (below).

**A cut with a held blade** (`$Keys [clock=blade_r]` and `travel(blade_r, ..., path=trailing)`, HUMAN-THROW-AND-STRIKE.md) is
timed by its travel, so its one effort is written on the travel, `travel(blade_r, velocity=12m/s, effort=100%, path=trailing)`
(`effort=` is `accel=`'s plain spelling there too; both on one travel is refused). The accepted slash lasts 1.784 s at 100% and
1.261 s at 200%. The `$Keys [clock=blade_r]` line takes no timing word of its own: `effort=` there is refused with the travel's
spelling to write, and `pace=` beside the clock is refused.

### A value of yours in the list

One number can drive every stretch, which is exactly one effort across. These build (dry run), each on a clip or pose that
declares `e`:

```gcmove
Cut = clip(frames=240, e=60.0) {
  $Four [effort=($e%, $e%, $e%)]
}
Slow = pose(e=60.0) {
  $Four [effort=(($e)%, ($e)%, ($e)%)]
}
```

Both gave the same three stages as `(60%, 60%, 60%)`. `$Slow(e=40)` gave stages at 40%. `effort=($e)%` on a plain travel builds too.
What does not build (the build's sentences below still say `accel=` in their advice; read it as `effort=`):

- `effort=($e, $e, $e)` with a bare number: `accel= needs a unit: 90%, 3000deg/s2 or 8m/s2`. Write the `%` after each value.
- An absolute rate from a value, `($e)deg/s2` or `$e deg/s2`: refused, `'($e)deg/s2' has 'deg/s2' after the expression`. A
  unit after a value is read only as `%`; write absolute rates as numbers.
- A name that is not declared on that clip or pose: `accel= takes 90%, 3000deg/s2 or 8m/s2, not '$e1%'`.

### The rules

- **One value per stretch.** The values are all shares (`60%`) or all absolute (`8000deg/s2`). A list of one value on two poses
  is the same as the single `effort=40%` (both took 0.120 s).
- **Every in-between pose is a REST.** The body arrives there and leaves from zero speed.
- **Two poses are not rests:** the fastest pose (it is passed at speed by its meaning), written `fastest_at=` (the split strike law below) or `peak_at=` / `strike=` (the one-bell law, as every file before 19 Nov times), and any pose named in `through=($Pose)`. `through=` names an in-between pose, once,
  not the fastest one.
- **A stage** is the run of stretches between two rests. A stage with a passed pose holds more than one stretch.
- **Stretches that share a stage may differ in effort**: the stage eases from one to the next. Measured 18 Nov on the claw's version 4
  keys, `effort=(60%, 150%, 100%)` with `fastest_at=$Contact2K` built `$Windup1K to $Follow3K at 150% easing to 100% 0.390 s`. (Until
  then this was refused; that refusal is gone.) Keep them close: the warning below still holds.
- **Beside a list:** `pace=` takes a speed curve per stretch, or one word for every stretch: `Preset.Even`, `Preset.FastStop` or
  `Preset.FastStart` (under `peak_at=` the stretches of one stage write one curve: a stage has one top, and the first written off Even places it; under `fastest_at=` each side of the hit takes its own).
  `Preset.Flat` and a pace written as numbers are refused there, and so is `phase=`. `velocity=` is one cap for every stage (the notes
  say when it bound a stage). `stagger=` and `point=` keep their meaning in the stage that holds the `fastest_at=` pose. `point=`
  without `fastest_at=` only measures.
- **One length per stretch:** `seconds=(0.40, 0.15, 0.30)`, below.
- The clip lasts as long as its stages; `frames=` is only the scale of its other lines.

### Different efforts on neighbouring stretches

Between two STOPS the build does not blend one effort into the next: each stage is timed alone and the stages are joined at
rest, so a slow stretch beside a fast one reads as two different speeds, not as one body speeding up. For example, the same
list with `effort=(50%, 100%, 25%)` builds as three stages of 0.108 s, 0.114 s and 0.478 s, each a full rise and fall at its own
level. If an action looks like "multiple speeds", check that first: write one number on every stretch and judge it.

Through a **Fastest here** pose written `fastest_at=` the two sides are joined at speed instead, by the split strike law below, so
different efforts (or lengths, or curves) either side of the hit read as one body speeding up into it and slowing after it.

### The split strike law: the hit is the fastest moment

**It is opt-in by the word.** `fastest_at=` (what the Pose tab's "Fastest here" writes) turns it on; `peak_at=` and `strike=` keep the one-bell law exactly,
so every file written before it times as it did. Both words name the fastest pose; only `fastest_at=` splits its stage. At one effort either side and Even
curves the split law is the one-bell law within a frame, and a curve moved off Even meets no jump.

A stage that passes its `fastest_at=` pose is split there into two sides joined at ONE top speed of the `point=`: the in-side
speeds up from the stop before it to the top, reaching it exactly at the pose; the out-side slows from that same speed to the
next stop. There is no jump at the pose, and nothing is faster in the stage. Each side has its own length and its own curve:

- **Its length:** written with `seconds=(...)` (each stretch's own entry), or, timed by effort, what its effort gives (a side's
  time goes as one over the square root of its effort; with one effort and Even curves this is the one-bell law within a frame).
- **Its curve:** the side's `pace=` word (Even, FastStop, FastStart, or a number): on the in-side FastStop builds slowly and
  FastStart bursts away; on the out-side FastStop brakes hard and FastStart settles slowly. The build bends the two shapes
  together as far as needed for one top speed with those lengths.
- **The top speed** is whatever the two lengths and the point's way before and after the pose give. The notes say it: `strike=$C:
  its sides as timed: $A to $C 0.150 s; $C to $B 0.300 s; $Point moves V m/s at $C (its stage's fastest V m/s)`, and the split as
  built (`splits at $C into two sides joined there at one top speed ...`).
- **Timed by lengths, the seconds win:** with `seconds=(...)` the stage is timed by its own time map built from its two sides, so the
  hit lands at its declared second exactly and the stage ends at its own; each side's curve shapes that side, and the build says the
  effort the lengths need (`its stage is timed by its lengths ... so it needs about N% effort`). When the lengths cannot share one
  top speed at the hit (the way before it over its length against the way after it over its length), a side's shape bends to keep
  one speed, to an exponent of 64 at most (the accepted TP_W2's roughly 12 times between its sides is met inside it). Past that the
  seconds still win: each side plays its own curve over its own length, so the motion fills the seconds, the speed drops at the hit,
  and the build says it: `WARNING strike=$Slash: the seconds win ... the speed drops at $Slash by N% of its top. Slash → Spent: one
  top speed needs about ... s here (asked ...), or Cock → Slash about ..., or a pose further along the path`.
  An arm slash at `(0.40, 0.15, 0.30)` reaches Slash at 0.55 s with one top speed; at `(0.40, 0.04, 1.50)` it reaches it at 0.44 s and
  the end at 1.94 s, with that WARNING.
- **Timed by effort, what it cannot play is said:** a stage too short or too long for its effort range gives the effort it needs, as
  `seconds=` always has.

### Momentum

A stage is an even rise and fall to a full stop. A list made only of rests reads as stop, go, stop: the same arm swing with
every pose a rest (`$Start, $Target, $Over, $Settle` at 60%, no `through=`) built as three stages, `0.124 s + 0.112 s +
0.141 s`, stopping at `$Target` and again at `$Over`. What carries an attack through is a pose the body PASSES (`through=`)
and a pose beyond the target that the swing arrives at, an overshoot. Name the target in `through=`, and let the rest be the
overshoot:

```gcmove
Swing = pose(steps) { $Start, $Target, $Over, $Settle }
Cut = clip(frames=240) {
  $Swing [effort=(60%, 60%, 60%), through=($Target)]
}
```

(`Start` 10, `Target` 50, `Over` 65, `Settle` 40 degrees of `upperarm_r` fold.) The build made two stages: `$Start to $Over at 60%
0.253 s`, passing `$Target`, and `$Over to $Settle at 60% 0.141 s`. The arm does not stop at the target. Keep the weight inside
the feet at each of these poses ([MOVE-BALANCE.md](MOVE-BALANCE.md)): a swing past the target is only believable if the body
could hold it.

### The speed curve, the length and the library preset

All three build today (`SB_Words.gcmove`, `SB_Seconds.gcmove`, `FA_Preset.gcmove`; the claw's keys, 0.315 s at 270%):

```gcmove
$Keys [effort=270%, pace=Preset.FastStop, point=$MClaw, fastest_at=$Contact1K]
$Keys [seconds=0.45, pace=Preset.FastStop, point=$MClaw, fastest_at=$Contact1K]
A_Claw = $FastestAt(poses=$Keys, at=$Contact1K, point=$MClaw, effort=270)
```

- `pace=Preset.Even` (top in the middle), `Preset.FastStop` (top late, hard stop), `Preset.FastStart` (top early, long settle),
  `Preset.Flat` (alone, no contact). Even and FastStop are peak 0.5 and 2/3; `peak=` writes any number.
- `seconds=` times by length: the first line lasted 0.315 s, the second 0.450 s and its note said `seconds=0.45: needs 129.2% effort`. Write one of the two.
- `FastestAt` is in the library (`presets:/GraphCodeMove/Presets/Human/HumanBody`): one effort across, the point fastest at `at=`, a stop curve
  (`stop=`). It gave the same timing as the first line. A refusal inside it (`at=` the last pose) names the library's line, not yours.
- A curve per stage, measured 18 Nov on the claw's version 4 keys (`effort=(60%, 150%, 150%)`, fastest at the contact; stage 1 Stance0K to
  Windup1K, stage 2 through the contact): on a stage between two stops the curve plays as named (stage 1 at 60%: Even 0.360 s, FastStart
  0.362 s, FastStop 0.341 s) and changes no other stage. (Measured before the split strike law, which now times that stage: a curve on the stage holding the fastest pose was held near Even, FastStop 0.403 to 0.395 s, and FastStart moved the top off the pose, 0.045 s before the contact.)

### One length per stretch

`seconds=(0.40, 0.15, 0.30)` gives each stretch its own length, as `effort=(...)` gives each its own effort (one value per stretch; the
travel lasts their sum). Each stage is timed alone, so the build finds each stage's effort for its own length and says it: `seconds=(...):
one length per stretch, each stage's effort found for its own length: $GuardK to $CockK needs N% effort; ...`, then when each pose is
reached against when it was asked. A stage that passes its Fastest here pose is timed by the split strike law (above), that pose reached when its lengths say; a stage passing a pose named in `through=`, with no curve written for it, finds the curve that brings that pose in when asked. A WARNING names a pose reached more than one frame off, and why (a written curve places it; or the speed law cannot play
those lengths with those poses). With no effort written the list is timed from 100% on every stretch, its stops declared as an effort
list's; one bare effort beside it is refused. `pace=(...)` beside it still gives each stage its curve.

### In the pose tab

The Timing group has, in this order: "Timed by" (effort or seconds); a row per stage named by its poses ("Stance → Windup", "Windup → Follow, through Contact"; a `fastest_at=` pose splits its stage into two rows, one per side), its effort slider with the seconds the last build gave it beside it ("0.360 s", "bound by speed" when more
effort would not shorten it), and under it that stage's speed curve ("speed": Even, FastStop, FastStart; Custom turns the curves into
numbers); timed by seconds, a row per stretch named "Guard → Cock" with its seconds slider and its from-to times ("0.00-0.40 s"), and
each stage's effort as the result the build found. Then each in-between pose has one choice, **Stops / Passes / Fastest here**, shown on
its scrubber label too: Passes writes it in `through=(...)`, Fastest here writes `fastest_at=$Pose` (one pose at most; it needs
`point=`), Stops takes it out of both. A literal list first needs **Make it adjustable**, which declares one effort and one curve per
stage without changing the timing. Each tip says what the value does to the motion first and, in grey, where it is saved.

**Opening a file loads its saved animation; it builds nothing.** The tab reads the file's header and build lines, puts the saved
clip on the body, and shows the timing results and the scrubber labels its last build stored on the clip. When the saved clip is
not what the file builds now (the file, a `presets:` library it reads, the skeleton, or a face's `body:` clip changed since its
build), the tab says "The saved animation is older than the file" (or "... is not built yet") and offers **Build now**, which
builds the file as it is saved; the file is not changed. Until then the old clip stands, with no labels. A file the build would
refuse still opens: its refusal shows at the first drag, Apply or Build now.

### What the notes print

For a list the notes print the list as written with each in-between pose a rest or passed (and, with `point=`, how far the point
turns there and how far it moves per 1% of the travel), then one line per stage. For the first example:

```text
3 stages, each one bell from rest to rest at its own effort: $A0 to $A1 at 60% 0.098 s, binding upperarm_r: acceleration 60.0%,
speed 43% as played, its acceleration binds; $A1 to $A2 at 60% 0.146 s, binding upperarm_r: acceleration 60.0%, speed 42% as
played, its acceleration binds; $A2 to $Back at 60% 0.199 s, binding upperarm_r: acceleration 60.0%, speed 60% as played, its
acceleration binds
```

- "its acceleration binds" means the stage plays at the effort you asked.
- "its speed binds (the top speed its effort gives)" means the move is long or slow enough that the joint reached its speed
  cap first. **A higher effort will not shorten that stage.** The note also says how much of the acceleration was played.
  With `e=40` the last stretch read `acceleration 26.7%, speed 40% as played, its speed binds (the top speed its effort gives)`.
- With `fastest_at=` and `point=`, a WARNING is printed when the point's top speed misses the contact by more than 0.03 s.

Refused by name (each was built to see the sentence):

- A count that does not match: `$Line3 [accel=(50%)] gives 1 value(s), and $Line3 passes 3 poses, so it has 2 stretch(es): one
  value per stretch`.
- (Superseded 18 Nov: different efforts in one stage now build, eased; see the rules above.)
- A pace that is no peak beside a list (`Preset.Flat`, or numbers): `an effort per stretch times each stretch, and a pace shapes one
  effort's speed over the travel; write one of them (beside a list a peak word plays: Preset.Even (Preset.Smooth), Preset.FastStop
  (Preset.Attack) or Preset.FastStart; Preset.Flat (Preset.Linear) is no peak)`.
- `fastest_at=` beside `peak_at=` (or `strike=`) on one line: `fastest_at= is the plain spelling of peak_at= and strike=, one word`.
- A length per stretch that does not match: `[seconds=(...)] gives N value(s), and $Keys passes M poses, so it has M-1 stretch(es):
  one length per stretch`; beside one bare effort: `a length per stretch finds each stage's effort, and one bare effort is one level
  for the whole travel`.
- `phase=` beside a list: `a [phase=] lead or lag is not built yet beside a list of efforts; write one accel= with the phase, or
  the list without it`.
- `through=` on the last pose: `$A2 is the last pose, where the travel ends at rest; through= names an in-between pose`.
- `through=` with no list: `through= names the poses an effort per stretch passes at speed, and $Line3 has none: write
  accel=(...) with one effort per stretch beside it`.
- Mixed units: `'3000deg/s2' is in another unit than '50%'; one list is written in one unit`.
- By the specs: `through=` naming the first pose, a pose twice, a pose that is not a step, or the strike; a strike with no
  `point=`; and a passed pose where the point turns back more than 90 degrees (take it out of `through=`).

### Reading a finished action: the zombie's claw slash

This is an example of how to READ the stage notes, not of how to write an action (the file is in the older words; it is written
here in the new ones). Its efforts differ from stage to stage, which
today reads as different speeds (see the warning above). `ZombieClaw/ZombieClawV4.gcmove` times six poses (stance, wind-up,
contact, follow-through, hang, stance) with five efforts and one point on the middle fingertip:

```gcmove
MClaw = point(bone=middle_03_r)
Keys = pose(steps) { $Stance0K, $Windup1K, $Contact2K, $Follow3K, $Hang4K, $Stance5K }
ClawSlash = clip(frames=300) {
  $Keys [effort=(25%, 100%, 100%, 35%, 22%), point=$MClaw, fastest_at=$Contact2K, stagger=(spine=0.2,arm_r=-0.2)]
  plant(leg_l, on=$Floor)
  plant(leg_r, on=$Floor)
}
```

The contact is the strike, so it is passed, and the two stretches either side of it share one stage: both are written 100%.
The build made four stages from the five stretches:

| Stage | From to | Effort | Time | Bound by |
|---|---|---|---|---|
| 1 | stance to wind-up | 25% | 0.864 s | neck_01, speed |
| 2 | wind-up to follow-through, through the contact | 100% | 0.687 s | upperarm_l, acceleration 99.8% |
| 3 | follow-through to hang | 35% | 0.358 s | neck_01, acceleration 35.0% |
| 4 | hang to stance | 22% | 1.043 s | head, speed |

Read it so: stages 2 and 3 are bound by acceleration, so they play at the effort written; stages 1 and 4 are bound by speed, so
raising their efforts would not shorten them. The whole clip is 2.952 s. The notes read `$MClaw, as timed, moves 5.75 m/s
there, 100% of its fastest (5.75 m/s)` at the contact, and no WARNING is printed.

Things to read for a strike:

- **Read the WARNING first.** The cut `ZombieClaw.gcmove`, with the efforts `(25%, 60%, 60%, 28%)` over five poses (no hang),
  prints: `WARNING strike=$Contact2K: under the effort per stretch $MClaw is fastest at 1.091 s (6.54 m/s), 0.098 s before the
  contact at 1.189 s, where it moves 4.48 m/s`. It says how early the top is and how fast the point moves at the contact, and
  the strike's verdict adds `author $Follow3K further the same way`: the claw is slowing at the contact because the
  follow-through pose is too near it, a fault of the poses and not of the efforts.
- Lowering the effort of the stage that holds the strike moves the top FURTHER from the contact, so do not use a lower effort
  to bring the top to the contact. The same five poses with `effort=(25%, 100%, 100%, 28%)` and with `(25%, 40%, 40%, 28%)`
  gave: at 100%, 2.095 s, the claw's top 8.98 m/s at 0.996 s, `0.075 s before the contact`; at 40%, 2.681 s, top 4.36 m/s,
  `0.148 s before the contact`. Both print the WARNING. Move a pose instead.
- A note about the floor of the contact's bell (`asked a share under the law's floor at 106 of its 1201 points ... held at 0.05
  there`) is information: the build held a small share up so the claw is still fastest at the contact.
- A pose a joint turns back at (`pelvis fold turns back at $Contact2K ...`) is named in the strike's verdict. Re-author that
  joint's value at the pose so it keeps going the same way.

The claw is also two files today. The keys (`ZombieClawV4_Keys.gcmove`) build five clips, one per stance, and the cut reads them
back with `pose(source=/Game/.../A_ZCV4_Contact, from=reference)`. The keys must be built first: a cut whose source clip is not
built is refused (`$K0: source=/Game/__MCPTests/Param1/Docs/A_NoSuchKey must name a native AnimSequence with animation data`).

## 6. A pose back to preset values, and bone overrides

A key you already have (built from reaches and plants, a frame of an animation, a key of direct lines) can be written again as
preset calls with values (ruling 106, slice 1). What the presets cannot say is written as a **bone override**.

**A bone override** sets one bone at its readings:

    swing(upperarm_r, at=(fold=61.338, side=-59.161, twist=-37.463))

- Each axis named ends at its value as `[readings]` reads it: the rig's word and sign, counted from the bone's rest. The
  slider's ends in the Pose tab are the rig's stops on that reading. Name one, two or three of `fold`, `side`, `twist`; an axis
  not named keeps what the other lines give the bone.
- It REPLACES what the pose's lines give that bone: a preset's swings, a preset's aim (`$ArmReach`'s upper arm), any swing. Line
  order does not matter for it, unlike every other swing. The bones below it go with it. The build says so: `the bone override
  swing(upperarm_r, at=(...)) replaces what aim(upperarm_r) on line N gives upperarm_r: the override wins over the aim`.
- A plant, a reach or a grab still holds. It solves after the override and wins on its own chain, and the build says how far it
  moved the bone: `plant(stand_r) on line N moved the bone override of calf_r by 6.0 degrees on its fold`.
- A plain `swing(<bone>, axis=..., by=)` ADDS to what a preset gives the bone (measured: `$Back(lean=30)` writes spine_01 fold
  7.907; a swing of 10 on it reads 17.907). On a bone an aim points it does nothing, and the build now says that too.
- The older `swing(<bone>, axis=fold, to=67)` is a different thing and is unchanged: a drawn fold (the angle between the bones,
  0 straight), added to the other lines and free to a reach (the gaits use it).
- Refused by name: a word the rig does not declare on that bone, a value on an axis the IK rig locks, a header with no
  `ik_rig:`, `by=`, `to=`, `axis=`, `give=`, `follow=`, a group, a modifier.

**The conversion.** Add `[as_presets]` (frame 0) or `[as_presets=(0, last)]` to a build line and dry-run it. It is read-only:
the notes give the converted key, and you write it with `gc_apply`. In the Pose tab, **Convert to presets** does the same for
the chosen pose: the key's text is rewritten, shown on the body, Undo takes it back, Apply builds it, Save writes it. The button is
shown only on a key with something to convert: a `source=` frame, or a `swing`, `shift` or `aim` that is not a bone override. A
key made only of preset calls has none, and shows no button.
**Upload pictures...** in the same tab does it toward a pose read off one or two pictures (the character's joint reader, then the
rig fitted to them: MOTION-FROM-VIDEO.md, "In the editor") instead of the pose as built.

- Only the lines that set the pose are replaced: preset calls, `swing`, `shift`, `aim`. Everything else stays as written, where
  it stands: the declaration (`source=`, `frames=`), `plant`, `balance`, `reach` and its `[share=]`, `collision`, comments. The
  notes list what was kept and what was replaced.
- The presets, in this order: `Stance` when both feet stand (the key plants both, or both are within 1 cm of its floor,
  its `ground(z=)`), with `Back` on top of it; otherwise `Hips`, then `FootAt` for a foot the key plants or stands, `LegReach`
  and `Foot` for a free one; `Back`; `Head` after `Back` (it counts from the chest); `ArmReach` per arm; `Hand` per hand.
  (Superseded 18 Nov: "`Hips` always, `Stance` never written".)
- **What the kept holds move is fitted to where they start, so they act once** (19 Nov, CODER-NOTES-17). A plant, a reach
  (with its `[share=]`), a grab and a `balance` solve on the pose they are handed: a converted key fitted to the key AS BUILT
  handed them a pose they had already moved, and they moved it again (the slash's cock: the share turned a trunk the presets
  had already turned, and the arm took another elbow, 30 degrees off). So the bones a kept hold moves (its own limb, and every
  bone above that limb or above a kept balance's pelvis: the trunk a share turns, the pelvis) are fitted to the key's own pose
  lines on its base pose with no hold and no `aim()` (an aim is solved with the holds, and a limb's elbow is chosen from the
  pose before any aim); every other bone to the key as built. Where those bones are still off (more than 0.02 degrees), plain
  swings (`swing(spine_04, axis=twist, by=0.17)`) are written under one comment, `# small corrections so this pose matches the
  original exactly; delete them to let the balance settle on its own`: they add to the presets' turn where the holds start and are
  not preset sliders. They matter because a `balance(at=)` that is not reached stops where its start leads it: a tenth of a degree
  on the spine moved the slash's hips 7 cm. A kept reach whose upper arm the converted key would roll more than 2 degrees from the
  source's gets `upper_roll=` the source's roll before its collision fit (`reach(sword3_r, to=$SlashHand, upper_roll=-22.9) [share=0.9]`):
  the key's `collision()` fit moves the arm after the reach and `balance` hold it, so the converted arm starts the fit where the
  source's did, and where it still lands more than a quarter of a degree off, the word is moved by that once. (The slash's arm
  is a twister elbow: its roll IS its swivel, held by the elbow going round, in `balance`'s trials too.)
  The check is then made as built, against the key as built: a trunk bone that a replaced aim also turned (a look past the
  neck's room) gets its override there, and the place each held limb hangs from (the collarbone, the hip) counts as an end,
  within 1 cm. Measured: the accepted slash's three keys build back the same pose (GraphCode.Move.ToPresets.SameSlash): every
  joint within 2 degrees except a held forearm, within 3 (the collision fit slides it by an amount its start decides; the
  hand, the sword and the ends are in their bounds), and the cut through them within 5 % of its time and of its blade's path
  (GraphCode.Move.ToPresets.SameCut1-5: 1.783 s against 1.792 s, 444.4 cm against 446.4 cm).
- Known limit: a key takes up to 90 s to convert (the slash's, 52 to 81 s on a busy machine): each check builds the whole key,
  its collision set up again every time. The note `time ... s` says where it went.
- A foot a kept plant holds: with the pose its plant starts from known (above), its heel, turnout and knee are fitted to that
  pose too, so the plant stands the converted foot from the sole it stood the source's on. (18 Nov, superseded 19 Nov: only its
  place, because a heel fitted to the built, stretched frame left no sole to stand on.) Otherwise only its place
  (`forward`, `out`); the plant decides the sole. Where the
  plant cannot stand the foot as the presets pose it (a stretched front leg pins the foot at its stops, and its sole has no
  facet to stand on: `no_sole`), it stands it on the sole of the key's **base pose** instead: the `source=` frame, or else
  the reference pose, taken before the preset calls and the key's own lines, with that pose's toe turn, at the place the
  presets give. The build says so: `plant 'stand_r' on ...: no facet as posed (...), stood on the base pose's facet (the
  source frame: vertices ..., ... cm^2) with its toe turn; the place is the posed one`. This holds for every plant, not
  only a converted key, and only where the plant would refuse: a key that builds today builds the same. A base pose with no
  standing facet either still refuses, naming both.
- Each preset's values are fitted with the library's own arithmetic: every try is the file's text solved as written.
  `Stance` starts from its feet in closed form (each ankle's `forward`, `out`, `turnout`, `heel` and `knee_out`, and the
  weight, from FootAt's and Stance's own arithmetic), so a lunge is found, not a nearby straight-legged stand.
- **A key on a `source=` frame whose pelvis is not at its rest** gets that offset in its Stance call:
  `$Stance(..., rig_base_px=-3.47, rig_base_py=-16.53, rig_base_pz=-11.91, rig_base_turn=-19.8, rig_base_hike=0.3)`. Hips adds
  to the pelvis the frame already has, while each FootAt works its leg out from the rest pelvis plus what Stance passes it;
  the `rig_base_` values (cm, degrees; the base pose's pelvis off its rest, read on the hip joints) are passed to FootAt beside
  them, so the legs are worked out from the pelvis as it stands; Hips' shift goes along the base pelvis's turned frame, so
  Stance tells FootAt that shift turned by `rig_base_turn`. They are facts of the frame, not sliders; 0 on the reference pose.
  Where Hips is written instead of Stance, the same offset goes into each FootAt's `px`..`turn`.
- Each key's conversion says where its time went (a `time` note by stage and by kind of work).
- A value the pose does not fix (moving it changes nothing) is written `none`, or left to its default. A value that ends at its
  range end is said.
- A bone override is written only for a joint still more than 2 degrees off, and only on the axes the rig leaves free. Joints
  between 0.5 and 2 degrees off are left to the presets, so their values stay adjustable; the notes count them.
- The converted key is then solved once as written, its plants and reaches in. The notes give its worst joint and its worst
  chain end against the key as built.
- **A library gap** is a value a preset is missing: one bone and one reading overridden with the same sign in two or more keys
  of a file, moving a chain end by more than 1 cm. The notes name it (`as_presets: library gaps ...`).
- Each key's conversion says how long it took. Keys of one file are converted in order, each starting from the one before.

## 7. The face

The face has presets of its own, by region (`$Mouth`, `$Eyes`, `$Brows`, `$Cheeks`), in the same
form, for a part:Face file: `presets:` and `controls:` `/GraphCodeMove/Presets/Human/HumanFace`. The
face's keys can land on the body's poses as the body's build times them (`body:` in the header,
`[lands_at=$Pose]` on a step). How, and the Pose tab's face: [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md),
"Face presets by region, and a face timed by the body".

## What is not built yet

- A smooth change from one effort to the next. Today each stage is timed alone and joined at rest, so different efforts on
  neighbouring stretches read as different speeds. Keep them close.
- Different efforts for the stretches inside one stage (refused, see part 5). Write them the same, or make the pose between
  them a rest.
- A rest in which the parts do not all stop in the same frame (every part of the body stops together at a rest today).
- Trailing of a loose part (a tail, a cloth, hair) on a list of built keys: a `follow=` line beside the list builds, but it
  takes that part out of the travel, so it does not trail.
- A part starting before another by a word under a list: `[phase=]` is refused beside a list. `stagger=` still works inside the
  strike's stage.
- An effort on an aimed preset (one that works by `aim(...)`, as every arm and leg said by direction does). Do not use one. It
  is not refused, but it is timed by the collar bones alone, a few degrees, so the time is wrong. Time the move as a built key
  (a pose of joint turns) instead, or time the list of poses.
- Two poses in one list that aim the same bone (two inline `$ArmReach` keys; ruling 112): each such key is solved alone, as a
  one-frame still of its own lines with the `collision()` of the clip that plays the list (its plant, reach and balance lines left
  out), and enters the list as that pose from the reference, as a key built by hand and read with `pose(source=..., from=reference)`
  does; its plant, reach and balance lines, and those it brings through a pose it calls, stay live in the list. The build says it
  per key (`$CockK: a key of $Keys ... solved alone ... with collision(self, $Sword), as the clip that plays it`). Two aims on one
  bone inside ONE key are still refused (`'upperarm_r' is already held by the aim() on line N`). The preset slash as one file:
  `PresetSlash_One.gcmove`, the same motion as part 5's two files.
  A still is kept and used again while nothing that decides it changed (the key's text, the clip's collision targets, the presets,
  rig, skeleton, meshes and files it names, the Move build); an unsaved edit to any of them solves it afresh, and the build says
  why. Limits: two clips that play the list with different `collision()` are refused (write the keys as built clips); a key solved
  so plays as that pose wherever the file uses it.
- A palm the whole arm cannot face. `palm_raise`/`palm_azimuth` say where the palm faces in the room; the forearm turns about
  its length only to its stop (+-90.6 degrees on the MetaHuman). On a straight arm the shoulder turns the whole arm for the rest; on a
  bent arm it cannot without moving the forearm, so the palm stops short and the build says by how much (`'lowerarm_r' is turned N
  short of its up=`). Past what the arm can turn at all, the forearm takes the nearer stop, so two keys that ask the same far palm
  from different arms can land on opposite stops: read that note, and ask a palm the arm can face (measured on the preset slash's
  Spent key: palm_azimuth -100 to -60 held the forearm at one stop, -40 to 60 at the other).
- `[share=]` on a list of poses: a pose reference with an effort list does not take it (the build answers `a pose reference takes
  one optional [profile=$Curve] multiplier, one optional [phase=] lead or lag, an effort [accel=80%] ...`).
- An absolute rate from one of your values inside the list (`($e)deg/s2`): write absolute rates as numbers.
- One-file actions: a part, a stance and an action in one file that uses a library file's presets, or keys and cut in one file.
  Today every file holds its own copy of the presets (part 4), and an action built from keys is two files (part 5).
- An effort on one line inside a pose (`swing()` in a pose takes no `effort=` yet), and efforts on a step of a list other than
  through the list above.
- Names: a pose named inside another value is reached at its defaults only (part 1).
- A conversion (part 6) from pictures, where an unseen roll should be `none` though the fit could set it: today the input is
  a whole pose, and only a value whose change moves nothing is written `none`.
- The as-built blade path of converted slash keys under the cut that reads them (ruling 106 slice 1, Done when (3)): waits on
  ruling 104's fix.
- A `Stance` that leaves a foot a kept plant holds unaimed (the plant owning its turn). Today `Stance` aims every foot, and a
  plant that cannot stand the foot as aimed stands it on the base pose's sole (part 6).
