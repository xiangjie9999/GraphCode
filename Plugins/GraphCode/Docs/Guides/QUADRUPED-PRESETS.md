# Four-legged pose presets (QuadrupedBody)

`Plugins/GraphCodeMove/Content/Presets/Quadruped/QuadrupedBody.gcmove` is GraphCode Move's library of pose presets for a
four-legged animal: `$Stance`, `$Hips`, `$Back`, `$Head`, `$Tail`, the hind leg (`$Thigh`, `$Shin`, `$HindPaw`, `$HindAt`,
`$HindReach`) and the fore leg (`$UpperArm`, `$Forearm`, `$ForePaw`, `$ForeAt`, `$ForeReach`). Each one poses one body part from a
few values, and each value has a range. It is the human library's language on four legs: read
[MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md) first.

**No animal ships with it.** The presets were written and measured on one test cat (a digitigrade sculpt; its skin cannot ship),
and no second body has been tested. Every number of that body is a `rig_` value whose default is the test cat's. To use the
library you bring your own quadruped, rename the bones, measure and set its `rig_` values, and teach the picture reader on it.

## Using it

Put this one line in the header of your own `.gcmove` and call the presets in your clips. Nothing is pasted:

    presets:/GraphCodeMove/Presets/Quadruped/QuadrupedBody

Your file keeps its own `skeleton:`, `ik_rig:` and `into:` lines. On the test cat, a sit, a pounce crouch and a trot contact
were written like this:

```gcmove
KSit = clip(frames=1) {
  $Stance(drop=0.7, shift_forward=-3, lean=40, round=0.3, heel_bl=0, heel_br=0, forward_fl=-0.3, forward_fr=-0.3)
  $Head(level=1)
  $Tail(raise=65, azimuth=120, curl=1)
}
KCrouch = clip(frames=1) {
  $Stance(drop=0.45, lean=92, round=-0.2, forward_fl=0.25, forward_fr=0.25, blade_fl=0.8, blade_fr=0.8, forward_bl=0.15, forward_br=0.15, heel_bl=35, heel_br=35)
  $Head(level=1, jut=0.5, look_at=$Prey)
  $Tail(raise=85, curl=-0.2)
}
KTrotContact = clip(frames=1) {
  $Stance(drop=0.18, lean=90, forward_br=0.67, forward_fl=0.16, bl=false, fr=false)
  $HindReach(raise=30, azimuth=180, reach=0.95, right=false)
  $HindPaw(point=30, right=false)
  $ForeReach(raise=25, azimuth=180, reach=0.9, left=false)
  $ForePaw(point=50, left=false)
  $Head(level=1)
  $Tail(raise=140)
}
```

`$Prey` is your own place, for example `Prey = point(at=(forward=60, up=3))`. A one-sided pose is the same calls with one side
switched off: `$Stance(fr=false)` lifts the figure's right fore paw and leaves it to `$ForeReach(..., left=false)` and `$ForePaw`.

## Building and dry-running a clip with the library

Write the `.gcmove` (header with `skeleton:`, `ik_rig:`, `presets:` and `into:`, a `build { A_Name = $Clip [readings=(0)] }`
block, a `point(bone=...)` for each joint you want to read, then the clips). Dry-run it by its asset path, no extension, with
`unreal_gc_apply`, the editor running, nothing written:

    unreal_gc_apply asset_path:/Game/MyAnimal/MyPoses dry_run:true

It answers "Would build N animations" or refuses by name (the line, the value, its range). Drop `dry_run` to build. The verb's own
help is `{"help": true}`.

**Reading the bones back.** `[readings=(0)]` on a build line prints, for frame 0, each joint's angles and each `point` as
`$PName at (x, y, z)` in cm (z up, forward +Y; the figure's right is -X). **The notes are long**: the six-pose test below printed
about 465 notes and 500,000 characters, more than a tool's output limit, so save the result to a file and filter it for your
point names (`$PFL`, `$PHead`) and for `short`. The "N short" notes (an aim or a stop that fell short) come first and carry no
clip name: tell the clip by the bone's side and the size. Every one-frame clip also prints "barely moves ... plays as a still
pose": that is expected for a pose file.

## Sides, in one place

`right` and `left` (and `fl`, `fr`, `bl`, `br`) are **the figure's own sides**; on the test cat the figure's right is -X and its
`_L` bones. Two switches look alike and mean opposite things:

- **A sided preset's `right=false` means "do not pose the right leg", so it poses the LEFT.** `$ForeReach(raise=50, reach=0.6,
  right=false)` poses the figure's left fore leg (`paw_F_R`, +X) and `$ForePaw(point=30, right=false)` its paw.
- **`Stance`'s `fr=false` means "the figure's right fore paw is not planted", that is lifted.** `$Stance(fl=false)` lifts the
  figure's left fore paw (`paw_F_R`) and leaves it to the sided presets.

So a stalk with only the left fore paw lifted pairs `Stance(fl=false)` with `ForeReach(..., right=false)`: the left is the one
lifted in `Stance` and the one posed (right switched off) in the reach. Pair them the same way for every leg.

## Worked examples (a first-time author's blind test)

Six poses, written from this guide only, all built in one dry run (one refusal, below). Floor = paw-joint height at its pad
clearance: hind 2.444 / 2.342, fore 3.324 / 2.982 cm. Paw joints are read with `point(bone=paw_F_L)` and the like.

```gcmove
BStalk = clip(frames=1) {       # only the figure's LEFT fore paw lifted
  $Stance(drop=0.3, lean=82, round=0, fl=false)
  $ForeReach(raise=50, azimuth=0, reach=0.6, right=false)
  $ForePaw(point=30, right=false)
  $Head(nod=-10)
  $Tail(raise=100, azimuth=180)
}
BLaunch = clip(frames=1) {      # pounce launch: hind legs pushed back, fore legs thrown forward and up
  $Stance(drop=0.1, lean=80, forward_bl=min, forward_br=min, fl=false, fr=false)
  $ForeReach(raise=70, azimuth=0, reach=0.9)
  $ForePaw(point=20)
  $Head(nod=-15)
  $Tail(raise=120)
}
BLand = clip(frames=1) {        # landing: fore paws down, hind legs still in the air behind
  $Stance(drop=0.3, lean=70, round=0.2, bl=false, br=false)
  $HindReach(raise=50, azimuth=170, reach=0.7)
  $HindPaw(point=20)
  $Head(level=1)
  $Tail(raise=90, azimuth=180)
}
BStretch = clip(frames=1) {     # fore legs forward, rear up
  $Stance(drop=0.1, lean=92, round=-0.2, forward_fl=0.25, forward_fr=0.25)
  $Head(nod=-15)
  $Tail(raise=150)
}
BLook = clip(frames=1) {        # look over the shoulder, to the figure's right
  $Stance
  $Head(turn=18)
  $Tail(raise=120, azimuth=180)
}
```

| Pose | What it read back | Limit |
|---|---|---|
| Stalk | The lifted paw is the figure's left (`paw_F_R`, x +9.07): z 9.30, y 27.8 (forward, up). Planted fore z 4.30; hind pads z 2.38 / 2.25 (within 0.1 of clearance). | The planted fore paw stands 0.97 cm over its clearance, just inside 1 cm (its wrist reads about 25-29 short). |
| Launch | Both fore paws up and ahead (z 17.7 / 17.6, y 35 / 37); hind pads z 2.51 / 2.30, pushed 2.5-3 cm behind the rest footprint. | The build warns `aim(wrist_R) asks side -38.2, past its stop at -15.0`: the wrists read 38-40 short, so the fore legs are not exactly the line asked (a wrist stop). |
| Land | Fore pads z 3.06 / 3.03 (-0.26 / +0.05 of clearance); hind paws lifted behind (z 6.4 / 6.9, y -24). | None found. |
| Stretch | Fore paws moved forward (y 26.6 / 27.3 against 18.4 / 18.6) and the head down 8.5 cm (z 24.5 against 33); hind pads z 2.52 / 2.30. | **The fore pads stand 1.6-1.7 cm over clearance (z 4.94 / 4.67): not yet explained** (the crouch's 4 cm wrist stop does not cover it). **The rear barely rises**: the pelvis is 0.6 cm above the look pose's, and the ranges say why it cannot rise far: `drop` stops at -0.072 (the hind legs straight) and `lean` at 92-96 (not measured as the cause in this test), so this stretch reads as chest down, hardly as rear up. |
| Look | Rest stand, all four pads within 0.1 of clearance; neck side 20.8, head fold -8.0; the head joint moved 2.5 cm toward -X against the stretch: turned to the figure's right. | `Head`'s `level` and `look_at` read short (the neck and head twist is locked); use `turn` and `nod`. |

The README's own sit built too: the fore pads within 0.4 cm, **the hind paws 3.3 cm under the floor** (the hock's 159 stop, see
below), and `$Head(level=1)` reads "18.9 short".

**The one refusal.** `$Stance(drop=0.1, lean=92, round=-0.3, ...)` was refused: "lean=92 ... is outside its range ...
lean_max=90.34 (formula ... came to 90.34 with round=-0.3 ...)". The note offers `lean_max=92` in the call; that writes past a
real pelvis stop. Do what the file's head says instead: **a lean at its end needs `round` written beside it**, and the pelvis's
forward stop ends `lean` at 92.24 with `round=-0.2`, so write `round=-0.2`.

## The language it shares with the human library

- **A value not written is `none`**: its lines are not written. A default that is a formula follows the values it reads.
- **Ranges**: every value an author writes carries `x_min` and `x_max` beside it. A number outside is refused by name;
  `x=max` and `x=min` mean the end; a call may widen an end.
- **Two angles for a long bone**: `raise` 0 straight down, 90 level, 180 up; `azimuth` 0 ahead, 90 out to the limb's own side,
  180 behind, below 0 across. Absolute in the figure's frame.
- **Sides**: a sided preset takes `left=true, right=true`; a side written `false` writes none of its lines, so uneven sides are
  two calls. **Right is the figure's right, the build's own word: -X** (the character's directions are left +X, forward +Y,
  up +Z). On the test cat the `_L` bones stand on the figure's RIGHT. Midline values (`shift_right`, `side`, `turn`,
  `curl_side`) are positive to the figure's right. `Stance`'s suffixes are `fl`, `fr`, `bl`, `br` (fore/back, left/right).
- **Scales**: an angle read off a picture is degrees; "how much of what this part can do" is -1..1 with 0 the rest; a distance is
  a fraction of the body's own size (a leg's length, a girdle's width), except `Hips`' shifts, in cm as the human `Hips`.
- **Layers**: first-layer presets own one part; second-layer presets (`HindReach`, `ForeReach`, `Stance`) call only the first.
  Entries marked "helper ... not listed" carry arithmetic for the preset above them.
- **Known limits, as the human library's**: an effort on an aimed preset is timed wrongly (time a list of built keys); two poses
  in one list may not aim the same bone.

## What each preset says

The comment above each preset in the file lists its values, their ranges, what `none` means, its `rig_` values and the worked
values built on the test cat. In short:

| Preset | Says | Values (range on the test cat) |
|---|---|---|
| `Hips` | the pelvis lowered, moved, turned (no `hike`: the pelvis's twist is locked) | `drop` of the mean hind leg (-0.072..0.893), `shift_right`, `shift_forward` cm, `turn` degrees (-12..12) |
| `Back` | the hinge at the hips and the spine's shape. **Everyday "arched back" (a Halloween cat) is `round>0`.** | `lean` degrees from vertical of the hip-middle-to-neck line (level 90; the ends are formulas of `round`), `round` -1..0.75 (not written: follows `lean`), `side` degrees |
| `Head` | the face against the chest | `turn` (-18.6..22.2), `nod` (-41.5..50.1), `jut` -1..1 (the head carried forward and down, the face held), `level` 0..1, `look_at` a place |
| `Tail` | the line from the root to the tip, and its curve | `raise` (30..180), `azimuth` (180 straight behind, below 180 to the figure's right; 90..270), `curl` -1..1 (1 the tip hooked forward over the back), `curl_side` -1..1 |
| `Thigh`, `Shin` | where the thigh and the crus point, for a leg off the ground | `raise`, `azimuth`. The crus rests pointing back: write `Shin`'s `azimuth` near 180 |
| `HindPaw`, `ForePaw` | a free paw: the metapodial and toes | `point` degrees (+ the paw folded back under the leg), `toes_up` degrees |
| `UpperArm`, `Forearm` | where the upper arm and forearm point; the blade with the upper arm | `raise` (0..100; 0..120), `azimuth`; `blade` -1..1 (1 the scapula at its back stop; not written, follows the upper arm) |
| `HindAt`, `ForeAt` | a PLANTED paw, the leg worked up from the paw joint | written by `Stance` (the places are hidden from the Pose tab); an author writes `heel` (0..90: the metapodial's angle from the floor; not written, that leg's rest) and the fore leg's `blade` |
| `HindReach`, `ForeReach` | a lifted leg along a line from the girdle joint (hip; shoulder) to the hock or wrist | `raise`, `azimuth`, `reach` (share of the two long bones, 1 straight), `swivel` degrees round the line (0 the joint's natural side), `blade` (fore) |
| `Stance` | the trunk ONCE and the four paws on the floor | `drop`, `shift_*`, `turn` (to `Hips`); `lean`, `round`, `side` (to `Back`); `forward_*` and `out_*` (where each paw joint stands against its rest footprint, of the leg's length / the girdle's width); `heel_*`, `blade_fl/fr`; `fl/fr/bl/br` (`false` lifts that leg) |

`Stance` works out where each hip joint and blade pivot stands from its own trunk values and hands each planted leg its two
places: the author says only where each paw stands on the floor and never works out a pivot. Not written, `Stance` is the rest
stand (all four pads on the floor); `drop` not written follows the switches (with both hind paws lifted the front stays up and
the rear swings about it). **Each planted leg's reach share is printed in the notes, and a leg out of reach is refused by
name.** `forward_br=min` is the full push (the leg straight under the trunk). **Do not call `Back` beside `Stance`:** `Back` is
swings, so a second call adds to the spine silently.

Postures are values, not presets: there is no `Sit`, `Crouch`, `Pounce` or `Bow`.

## Fitting your own quadruped

The file's head has the whole list; this is the order.

1. **Copy the library into your project** (for example `Content/MyAnimal/Presets/MyAnimalBody.gcmove`) and point your header's
   `presets:` at the copy (`presets:/Game/MyAnimal/Presets/MyAnimalBody`). Passing a `rig_` value in a call reaches only that
   preset's own lines: `Stance` does not pass `HindAt`'s or `ForeAt`'s `rig_` values down, so a whole other body sets its numbers
   as the defaults in its copy.
2. **The bone block.** The block at the top of the file maps each part to the test cat's bones by figure side, and the frames
   under it are renamed with it: trunk `pelvis`, `spine`, `chest`; `neck`, `head`; `tail_01..04`; fore leg `scapula_*`,
   `shoulder_*`, `elbow_*`, `wrist_*`, `paw_F_*`; hind leg `hip_*`, `stifle_*`, `hock_*`, `paw_B_*`, `toe_B_*`. On the test cat
   the `_L` bones are the figure's RIGHT, so every `$right` branch writes `_L` bones and every `rig_*_r` holds an `_L` number.
   Rename throughout the copy; if your skeleton's left bones are on the figure's left, swap the suffixes as you rename.
3. **Measure the `rig_` values with `[readings=(0)]`.** Make a scratch file on your skeleton and IK rig with a point on each joint
   and an empty clip, and dry-run it:

   ```gcmove
   build {
     A_Rest = $Rest [readings=(0)]
   }
   PHipR = point(bone=hip_L)
   PStifleR = point(bone=stifle_L)
   Rest = clip(frames=1) {
   }
   ```

   The notes print every joint's angles and each point as `$PHipR at (x, y, z)` in component cm, at the BUILT rest (the rig's
   `rest=` folds, not the sculpt's bind points). From these come the bone lengths (one leg-length rule: the sum of a leg's bones
   from its root to the paw joint), the rest places of the hips, blade pivots and paw joints, the metapodials' rest angle from
   the floor (each leg's `heel` default), the trunk's rest line (`rig_rest_tilt`) and the trunk segments. A clip of one swing of
   20 per joint gives each joint's sign (the test cat's chest folds the other way from its spine; the library's head lists the test cat's signs).
   A value's end is where the build stops it: write the value at the end, read what the build says, and set `x_min` / `x_max`.
4. **The pad clearance.** "The floor" in these presets is the PADS, and at the built rest pads usually float. Plant each leg on
   the floor at full weight and read how far each paw joint drops:

   ```gcmove
   Floor = ground(z=0)
   Pads = clip(frames=1) {
     plant(leg_F_L, on=$Floor, roll=(Palm_F_L, Pad_F_L))
     plant(leg_B_L, on=$Floor, roll=(Sole_B_L, Pad_B_L))
   }
   ```

   (your rig's own leg chains and contacts). The paw joint's height with its pads planted is `HindAt`'s `rig_paw_z_r/_l` and
   `Stance`'s `rig_pad_*`. Then solve `Stance`'s rest stand (`rig_rest_lean`, `rig_rest_drop`) so `$Stance` alone stands all four
   pads on the floor; on the test cat it read within 0.05 cm.
5. **Head and Tail's axes.** Their joint axes, tail segments and rest face are `rig_` values now, written by
   `Saved/Research/MOVE-PARAM1/work-quad-headtail/gen3.py` from the built rest's readings. On another body, rerun it on your
   readings before trusting `Head` and `Tail`.
6. **Check it.** Dry-run the three clips above on your animal and read the paw joints, heels and reach shares in the notes.
7. **Teach the reader.** The picture route reads stills of your animal into joints, and its reader is taught per character
   (`picture_route.py teach`: renders of your animal, about an hour on one CUDA graphics card;
   [MOTION-FROM-VIDEO.md](MOTION-FROM-VIDEO.md)). The Pose tab's Train window is designed, not built.

## Rig stops and gaps found on the test cat

The build names every stop it meets; nothing floats silently. The file's head lists them in full. The main ones:

- **Aims on twist-locked joints wait on an engine fix.** An aim turns the neck, head and tail (their twist is locked in the IK
  rig) only 1-3 degrees. So `Head` says `turn`, `nod` and `jut` as swings; its `level` and `look_at` are written in the human
  form and read short until that fix. `Tail` is swings only.
- **The hock's flexion stop** (159 on the test cat) ends a sit's `heel` 0 and a crouch's `heel` 35 well short: the sit's hind
  paws sit 3.3 cm under the floor. A paw far ahead meets the hip's side stop. A planted fore leg at rest is nearly straight, so a
  paw moved ahead needs the trunk lowered (its reach share says so). A crouch's fore paws stand 4 cm high at the wrist's side stop.
- **A lean at its end needs `round` beside it** (`$Back(lean=max, round=0)`): the end depends on `round`, and with `round` left
  to follow `lean` the build refuses it as a circle.
- **The trunk**: no `hike` (the pelvis's twist is locked); `round` ends at 0.75 (the body meets itself past it); the pelvis's
  forward stop ends `lean` at 92 for a crouch; `round`'s default follows `lean` by a fit with one judged point, not cat data.
- **The tail**: tail_01's side stop holds the azimuth; a tail wrapped round the side by `curl_side` alone is past every side
  stop; `curl=1` reaches the same line.

## What is not built

- **The open mouth**: the test rig has no jaw bone; no tongue, eyes, ears or claws either.
- **A paw touching a place** (another paw, the mouth, a prey point): there is no `PawOn` and no landmarks; a lick is two
  directions until a pose needs a contact.
- **Aims on twist-locked joints** (above): waits on an engine fix.
- **A shipped animal**: none. A second body has not been tested, and the defaults fit the test cat only.
- **`[as_presets]` and the Pose tab's "Convert to presets"** read the human library only. The Pose tab itself shows this
  library's ranged values; `Stance`'s `fl`/`fr`/`bl`/`br` carry no range, so they are written in the text, not ticked.
- **The Train window** (above). The cat's gaits are not presets: a walk, trot or gallop is a gait file of footholds, and this
  library is for poses (a sit, a crouch, the keys of an action).
