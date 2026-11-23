# Holding a sword (or any handle) firmly

*Placed 20 Nov 2026 by the Move lead (owner: "so next time sword holding etc. will be made very fast"). Written from the work records in `Saved/Research/MOVE-PARAM1/`
(GRIP-BY-VALUES, POWER-GRIP-NOTES, BLADE-DIR, CODER-NOTES-16, REVIEW-GRASP-DESIGN, ruling 114). Everything below is grammar the
published build accepted when the PresetSlash keys were built (20 Nov; the three-pose method of section 2A on 21 Nov). What is not shipped says "not available yet".
Route: [GCMOVE.md](GCMOVE.md) -> [HANDS.md](HANDS.md) "Holding on a MetaHuman body" -> this recipe.*

Measured on the BlueEdge MetaHuman sword body, right hand. Copy [Examples/SwordGrip/SwordGrip.gcmove](../Examples/SwordGrip/SwordGrip.gcmove) into your file; the
shipped working example is `Content/GraphCodeExamples/PresetSlash_20261118/PresetSlash_Keys.gcmove`.

## The recipe in one screen (21 Nov 2026: three poses, one by one)

The owner's method (21 Nov: "three poses, one by one, first is hand open, second is four fingers closed, third is thumb on top"),
each pose passed by a blind critic before the next:

```gc
Grip = surface(part=Sword, solid=collision)        # the handle as a collision surface (the build's audit reads it)
# in EVERY key, in this order:
  $Hand(wrist=-30, wrist_side=2, left=false)   # the wrist gives the square look, off its stops
  $SwordSeat     # 1. the SEAT by geometry: turn 22 X, 4 Y, shift 2.124 cm: the open hand touches the handle, nothing inside
  $SwordFist     # 2. the four FINGERS by plain values, each segment touching the handle within the flesh's 3.5 mm
  $SwordThumb    # 3. the THUMB by plain values on top
```

No collision forms and no `collision(self, $Grip)` slide: the build's final audit reports any pass-through and refuses the key.
Keys are one frame (`clip(frames=1)`). The values are in [Examples/SwordGrip/SwordGrip.gcmove](../Examples/SwordGrip/SwordGrip.gcmove);
they are fitted for this handle on this body (section 2A says how to fit another).

## 2A. The three-pose method (shipped 21 Nov)

**THE ROOT CAUSE it fixes.** The old seat (+20 about X, +8 about Y, written in every key) turned the handle INSIDE the open hand: with
every hand value at zero the handle was 12 mm through the thumb base, and the 20 Nov keys (built with a probe's shift of -0.5 cm)
also put it 8.5 mm into the heel and 5.8 mm into the middle side. Every collision stop fought on 20 Nov (the thumb frozen at its
base, start-overlap refusals, fingers stopping on the thumb) followed from that. Measure the open hand against the handle FIRST.

**1. Seat by geometry, not by builds.** Read the body's reference-pose skin (each vertex with its dominant bone) and the handle's
collision solid (for BlueEdge a 3.3 x 2.64 cm rounded box, its axis 1.075 cm out along the socket's +X), and place the handle so the
open hand touches it with nothing inside: `Saved/Research/MOVE-PARAM1/seatfix/seatopt.py`. Use the MESH's own reference pose, not the
skeleton's (they differ by ~12 cm at this hand). The shipped seat: 22 about palm_r X (the handle along the finger-root line), 4 about Y
(the pommel end into the heel), then `shift(palm_r, axis=(-0.0789,0.9193,0.3856), by=2.124)` (cm, exact; in the turned frame). Open
hand, mm of gap: heel 0.2, index root 0.0, pinky root 0.2, middle metacarpal 0.9, ring root 2.1, thenar 2.3, index metacarpal 2.4,
ring metacarpal 3.2, middle root 5.0. The shift plays because palm_r's retargeting is `Animation` on SK_BlueEdgeBody_Skeleton
(21 Nov; the 29 shipped clips that carried a stale palm_r translation were rewritten to the reference first, proven by scans).

**The square look comes from the wrist, not from turning the handle in the palm.** Handle to forearm at the cock key: 71.9 deg with
`$Hand(wrist=-30, wrist_side=2)` (the hand's side reads 29.8 against its 30 stop). `wrist_side` is toward the little finger and LOWERS
the angle on this body (5 / 15 / 25 gave 53 / 45 / 37 deg at seat 0).

**2. The four fingers by plain values.** Calibrate each joint's axis once (one clip with every finger joint folded 20 and each
knuckle turned 5 sideways; fold = the bone's -Z, side = +Y), pose the skin on those axes and fit each finger's knuckle, middle and
tip so each segment touches the handle 0.5-2.5 mm into the skin (`seatfix/fingerfit4.py`). The published 30 mm-handle angles
(Shimawaki 2019) put every finger 13 mm through a handle seated at the roots and the build refused them. Shipped, as written
(knuckle / middle / tip; side): index -2 / 54 / 13, side -6; middle 15 / 43 / 20.5; ring 15 / 33 / 33.8, side +4;
pinky 0 / 20 / 10, side +8 (+ = toward the thumb, closing the spread). The rig reads them higher (its rest curl): index
6.4 / 67.5 / 26.1, middle 23.8 / 53.1 / 33.2, ring 21.7 / 42.5 / 45.5, pinky 4.6 / 32.1 / 25.6. Contact, mm (- = into the skin):
index -2.3 / +2.9 / -2.4, middle -0.6 / -0.5 / -1.1, ring -0.9 / -2.2 / -0.7, pinky -2.4 / -1.2 / -2.0.

**3. The thumb by plain values** (`seatfix/thumbfit3.py`): thumb_01_r fold -30, twist 17 (clamped 1.9 at its 17 stop),
thumb_opp_r 47.5, thumb_roll_r twist 16, thumb_02_r fold -10, thumb_03_r fold 5. Its middle segment and tip rest on the handle
(-0.3 / -0.5 mm) and the tip on the middle fingertip; the build's own hull check passes it (opposition 45 put the tip 5.5 mm into
the middle fingertip and was refused). The capsule model reads about 4 mm shallower than the build's hull check: the build is the proof.

**Thumb ranges against anatomy** (Barakat, Field & Taylor 2013, Hand (N Y) 8(2):179-182, 100 adults, active): IP flexion 88 +- 2.3
= the rig's thumb_03 stop (88); MCP flexion 60 +- 5.5 = thumb_02's 60 (the build prints it as 40: its native frame is offset 20);
CMC palmar/radial abduction 61.2 / 62.9 against thumb_01's side +-20, tighter, but widening it changed nothing (the fit chose side 0
at +-20 and at +-62.9). thumb_opp_r WORKS -12.7 to 77 on this body ("asked -30.0, past its stop at -12.7 (range -13 to 77)"),
not the declared -30 to 60.

**Result in PresetSlash (21 Nov):** the cut 158 frames, 1.31 s (was 137 frames, 1.14 s), "Nothing passes through anything"; the
blade line moved 3-7 deg from the 20 Nov cut. Pictures: `Saved/Pictures/PresetSlash/blade-dir/THREEPOSE_COCK_HAND.png`,
`THREEPOSE_CUT_beside_shipped.png`, `POSE1v3_HAND_OPEN.png`, `POSE2_FINGERS.png`, `POSE3_THUMB.png`.

## 2B. The 20 Nov method (SUPERSEDED 21 Nov by 2A; kept for the record)

### The 20 Nov recipe (SUPERSEDED)

```gc
Grip = surface(part=Sword, solid=collision)        # the handle as a collision surface
# in EVERY key, in this order, nothing between them:
  $SwordSeat(turn=20)        # swing(palm_r,axis=X,by=20): the handle square across the fist, same in every key
  $SwordOpen                 # fingers pre-curled: knuckles -30, middles +30, tips +25, thumb out of the way
  $SwordFist                 # pose(collision=$Grip): knuckles +100, middles +110, tips +95 (asked past the handle)
  $SwordThumb                # a PLAIN pose: thumb_opp_r 40, thumb_roll_r +30, curls 40 / 70 (wraps across the fingers)
  collision(self, $Grip)     # the clip's LAST line
```

Keys are one frame (`clip(frames=1)`). Once the rig has its lines: paste, name the part, build.

## 1. The rig lines a held prop needs (once per body)

Without these, either the cut is refused or the thumb cannot oppose, or a cut with the square seat takes three times too long.
In `MetaHumanSwordRig.gcrig`, `joints { }`:

```gc
thumb_opp_r  [range=(-30, 60), closes=in, velocity=1000, accel=26000, ramp=0.04]   # works -12.7 to 77 on this body (2A)
thumb_roll_r [twists=(-45, 45), velocity=1000, accel=26000, ramp=0.04]
palm_r       [velocity=2500, accel=150000, ramp=0.015]
upperarm_r   [... velocity=1100, accel=25000, ramp=0.025, twist_velocity=7000, twist_accel=500000 ...]
lowerarm_r   [twist_velocity=2000, twist_accel=150000]
```

- `palm_r` is the skinless carrier the sword rides (a socket with `bone=palm_r` on hand_r, HANDS.md "Holding on a MetaHuman
  body"). Give it **the hand's own references** (hand_r: 2500 / 150000 / 0.015). With none, an effort-timed cut refuses with
  "accel=100% is a share of palm_r's reference acceleration, and the rig declares none for it". No range: it is not a joint a pose bends.
- `thumb_opp_r` and `thumb_roll_r` stand at thumb_01_r's place (`points { }` plus `chains { thumb_r = thumb_01_r -> thumb_opp_r ->
  thumb_roll_r -> thumb_02_r -> thumb_03_r }`). They take **thumb_01_r's references** (1000 / 26000 / 0.04); with none the cut
  was refused the same way (thumb_opp_r's reference acceleration). The ranges are the VR hand's sourced ones.
- Any other carrier or hinge you add that a clip turns needs the same three references copied from the nearest real joint.
- `twist_velocity=` / `twist_accel=` give the arm's turn about its own length a speed and acceleration of its own (sourced:
  `Docs/Research/joint-speed-and-acceleration-references.md` section 4; the citation sits on the rig's lines). A square handle makes a
  cut turn the forearm and upper arm about their length fast; timed against the swing figures the same cut took 2.79 s, with
  them 1.14 s. A line that states only these two keys keeps every other key the parent gave the joint.

### (SUPERSEDED 21 Nov) 2. The seat: one turn of palm_r, identical in every key

`$SwordSeat` is `swing(palm_r,axis=X,by=20)`, a turn about the palm's normal. **+20 is the shipped seat** (owner, 20 Nov: "The grip
is much better now"): as in a kendo or iaido grip the handle crosses the fist near a right angle to the forearm (70.7 deg off the
hand axis) and all four fingers close square round it. The earlier seat, -12, laid the handle on the line from the heel on the
little-finger side to the index base (38.6 deg; with the hand open the handle crossed the palm 50.7 deg from the finger axis), so it
ran along the forearm: use it only for a grip that should look like that. Rules that cost a day each:

- **Write the same seat in every key.** A seat that differs per key (seat by touch, a collision-seated carrier) moves the sword
  between keys and a cut built on them is refused ("not a held leaf"). Since the carrier fix (published 20 Nov), `collision()`
  never moves a prop's carrier: palm_r stops where the pose put it (0.0001 deg between keys).
- **A seat by shift never plays on this skeleton** (palm_r translation retargeting `Skeleton`). A turn is all you can write.
  (SUPERSEDED 21 Nov: palm_r is `Animation` now and a shift plays exactly, `by=` in cm; spec ShiftMovesByCentimetres.)
- **What the turn does to the blade:** it turns the sword in the hand, about 1 deg of blade per deg of seat, and it changes the
  arm's path in a cut built with `path=trailing`. Measured on the keys planned for seat -12, with the swing references only
  (no twist references), cut time by seat:

| seat | handle vs hand axis | blade line moved | pinky | cut |
|---|---|---|---|---|
| -12 (before 20 Nov) | 38.7 deg | 0 | on the handle | 0.88 s |
| -4 | 46.7 | 8 deg | on the handle | 3.84 s |
| +4 | 54.7 | 16 | into the palm | 2.18 s |
| +12 | 62.7 | 24 | on the handle | 3.65 s |
| +20 (shipped) | 70.7 | 32 | on the handle | 3.11 s; 1.14 s with the twist references and the chest at -26 |
| +30 | 80.7 | 42 | on the handle | 2.80 s |

  The shape of the cut survives any seat (one clean fan); its TIME does not, because `path=trailing` re-poses the upper arm's
  and forearm's twist and, timed against the swing figures, the effort clock stretches the cut 2.5 to 4.4 times. The rig's twist
  references (section 1) time that twist as the arm really turns: PresetSlash at +20 is 1.14 s ("The right forearm's turn at
  0.57 s sets that time"), with the Slash key's chest turned -26 (was -16) so the trunk carries part of the turn.
  **Choose the seat before you plan the cut's keys, and leave it.**
- Below -12 (e.g. -20) the carrier slides even in one frame (palm_r stopped 2.44 deg short): reject.

### (SUPERSEDED 21 Nov) 3. The open pose before the fist

`$SwordOpen`: knuckles (`index_01_r .. pinky_01_r`) fold -30, middle joints (`_02_`) fold **+30**, end joints (`_03_`) fold **+25**,
thumb pulled out of the way (`thumb_01_r` fold 50, `thumb_02_r` -8, `thumb_03_r` -12).

Why the pre-curl: the fist is asked as one pose and the collision fit stops each joint when its segment first touches. A straight
finger swung about its knuckle meets the handle with its middle or tip, which freezes the whole finger and leaves the tip flexed
little. Bending the middles and tips FIRST (HANDS.md "curl the middle and end joints first") lets the knuckle close until the
palm side meets the handle. Before: pip -25 / dip -15 (fingers read 11/55/15, 44/63/6, 52/63/1, 41/60/15 as knuckle/middle/tip);
now +30 / +25.

Sweep of the pre-curl (Cock, one frame): +45/+35 pinky into the palm; +70/+50 knuckles open and palm_r stopped; +58/+40 palm_r
stopped; +38/+30 pinky into the palm; **+30/+25 all four fingers on the handle** (shipped). If your handle is thinner or thicker,
sweep these two numbers and read the finger angles; they are the only finger values that depend on the prop.

### (SUPERSEDED 21 Nov) 4. The fist, `collision(self, $Grip)` last, one-frame keys

`$SwordFist = pose(collision=$Grip) { swing($MCP,axis=fold,by=100) swing($PIP,axis=fold,by=110) swing($DIP,axis=fold,by=95) }`.
It asks the hand **past the handle**; the collision form puts each joint where it was asked and slides it out of what it is in,
so every finger stops on the handle's surface. The stop note reads "collision form stopped '<bone>' where it touched; the rest of
its asked move was refused ... N cm inside": N is the depth of the REST of the asked move, which is what asking past the
object is for. Judge the grip by the render, not by that number.

- `collision(self, $Grip)` is the clip's last line. `self` keeps the fingers off each other (the ring/little finger overlap that
  the final audit refuses otherwise); `$Grip` is the handle.
- **Keep keys to one frame.** A staged (12-frame) closing makes `collision()` slide for minutes (110 to 167 s a key), the
  thumb ends different in every key, and before the carrier fix it moved palm_r, so the cut's held-leaf check refused. One-frame keys built in seconds.
- Do not make the fingers plain poses and rely on `collision()` alone for the fist with no collision form: the asked-past plain
  poses left the middle finger frozen against the ring finger short of the handle (2026-10-13).

### (SUPERSEDED 21 Nov) 5. The thumb across the fingers

`$SwordThumb = pose() { thumb_opp_r fold 40, thumb_roll_r twist +30, thumb_01_r twist -17 / fold -80, thumb_02_r fold 40, thumb_03_r fold 70 }`.

- **A plain pose, not a collision form, at the square seat** (20 Nov). At +20 the handle overlaps `thumb_01_r` (1.87 cm), so as a
  collision form the thumb stopped on the handle's side at once (opposition 3.2 of 40, roll 6.8 of 30) and lay along the handle.
  As plain values it takes them in full and wraps over the front of the handle, its tip on the index finger as in the iaido photo;
  `collision(self, $Grip)`, the clip's last line, slides it out of whatever it is in. Six value sets were swept (opposition 30 to 60,
  roll 30 to 50, base fold -50 to -100): 60 opposition sticks the thumb out sideways; these values sit closest to the photo.

- `thumb_opp_r` 40 brings the thumb toward the palm. **`thumb_roll_r +30` is the inward sign on this skeleton**: with opp 40 and
  roll **-30** the thumb sticks out sideways (opp 40/50/60 all do); opp -30 + roll -30 curls in but less across. +30 lies across the front
  of the fingers.
- Curl 40 / 70 (before 63 / 80): the thumb's form stops on the fingers (`thumb_03_r` on middle_03 / index_03), not inside the handle,
  and no start-inside warning remains. Read-backs identical in all three keys (thumb_opp 40, roll 30, thumb_02 40, thumb_03 38.0).
- Result at the square seat: the thumb wraps across the front of the handle with its tip on the index finger.

## 6. What NOT to do

- **No per-key seat, no collision-seated carrier.** It breaks the cut ("not a held leaf"). Same `$SwordSeat` in every key.
- **No staged / multi-frame fist** (section 4).
- **No reach to the thumb target** (`reach(thumb_r, contact=ThumbPad_R, to=index/middle joint)`): the thumb chain is 9.2 cm and the
  targets were 9.5 to 10.9 cm away, so it ends 2.6 to 3.6 cm short with the thumb lying in the palm. Values, not a reach.
- **No closure solver.** A grasp closure solver was designed and coded (close the chain together, stop proximal joints at the first
  touch), reviewed (REVIEW-GRASP-DESIGN.md: it kept the stop rule that makes a fingertip soft) and **put on hold by the owner
  on 20 Nov at 02:45** ("if your rule fail, can't you just use my rule?"); its renders were worse than the accepted grip. Ruling 114
  in `Docs/GC-MOVE-PARAMETRIC-POSES-PLAN.md`. The rule that stands: keep the accepted grip and change the finger **values** only.
  The solver is **not available yet** (code on scratch, out of the tree).
- **Do not turn the open pose up to "firmer".** +45/+35 and beyond put the pinky into the palm or open the knuckles.

## 7. Measured results

Fingers as knuckle / middle / tip, degrees:

| | accepted grip | seat -12 (before 20 Nov) | **seat +20 (shipped)** |
|---|---|---|---|
| index | 11 / 55 / 15.5 | -8.3 / 56.7 / 64.5 | 54.0 / 52.7 / 38.1 |
| middle | 44 / 63 / 6 | 48.7 / 55.5 / 45.1 | 62.6 / 47.5 / 37.7 |
| ring | 52 / 63 / 1 | 56.0 / 44.7 / 44.2 | 50.9 / 44.1 / 36.7 |
| pinky | 41 / 60 / 15 | 42.9 / 47.7 / 47.5 | 35.0 / 42.1 / 40.6 |

At +20 every finger closes at the knuckle as well as the middle and tip (the index knuckle no longer stops at -8 on the handle), all
stop on `$Grip`, identical in Cock / Slash / Spent. The thumb's form stops on the handle early at this seat (thumb_opp_r 3.2,
thumb_roll_r 6.8 of the asked 40 / 30): it lies along the handle's side. The cut: **137 frames, 1.14 s**, "Nothing passes through
anything". At -12 it was 106 frames, 0.88 s ("the right upper arm's turn at 0.18 s sets that time").

Pictures (`Saved/Pictures/PresetSlash/grip-by-values/`): `SHIPPED_THUMB_beside_photo_and_before.png` (iaido photo, before, now),
`FINAL_beside_photo_and_accepted.png` (finger values), `THUMB_OPP_beside_photo_and_NEW.png` and `OPP_quick.png` (the roll's sign),
`SEATm4_beside_photo_and_NEW.png`; `Saved/Pictures/PresetSlash/blade-dir/SEAT30_beside_photo_and_shipped.png` (square seat),
`SEAT20_cut_beside_shipped.png`, `BLADEPOINTS_three_ways.png`, and the shipped square grip beside the iaido photo and the
seat -12 grip and cut: `SQUARE_GRIP_SHIPPED_before_after.png`. Check from four sides (HANDS.md): palm side, back, far side, fingertips.

## 8. Open gaps (not shipped)

- **The thumb pad on the index middle segment (21 Nov, measured):** with the handle at the finger roots the index's first segment lies
  over the handle top, so the thumb tip stops on it, 2.7 cm short of the index middle segment where the iaido photo has it. Not the
  thumb's stops (fitted with the sourced ranges, a free opposition and thumb_02 to its real stop: 2.70-2.73 cm) and not this seat
  (moved 0.5 / 1.0 cm toward the palm centre: 2.71 / 2.58 cm, and at 1.0 cm the handle no longer touches the index side). Open.
- The items below are from 20 Nov; the palm contact and the thumb-at-its-base items are answered by 2A.

- **The square handle: SHIPPED 20 Nov** (seat +20, the rig's twist references, the Slash chest at -26; the cut 1.14 s against
  0.88 s before, the owner's choice). `$BladePoints` (a blade-direction preset, only in the scratch `HumanBodyBlade.gcmove`) can put
  the blade back on the old line but bends the wrist 30 to 44 deg: not shipped.
- **The thumb at the square seat: SHIPPED 20 Nov** as a plain pose (section 5): it wraps across, tip on the index finger.
- **The palm against the handle** (owner 20 Nov: "close the grip a little, so the palm touches the handle"): the palm meets the handle
  at the index base, the little-finger heel stands off. A seat SHIFT does not play on this skeleton (below) and a metacarpal cup lifts
  the ring and pinky off the handle: **not available yet**; a contact seat needs palm_r's translation to play.
- **The thumb deep across the fist**: its across stops at -7 deg (the handle overlaps its base at rest); it lies over the fingertips.
  Nearer reach targets were worse by eye.
- **The index knuckle** read -8 (stopped by the handle) at seat -12; at +20 it closes to 54.
- **A lower seat toward the wrist** needs palm_r's translation to play (retargeting `Animation`: a skeleton asset change, the owner's call).
- **Left hand**, and props other than the BlueEdge sword: not measured. A different prop on its own socket needs its own seat fit.

## 9. Checklist for the next prop (21 Nov: the three-pose method)

1. Rig as section 1; palm_r's translation must play (retargeting `Animation`).
2. Pose 1: measure the open hand against the handle's collision solid; place the handle touching (0-2 mm), nothing inside (2A.1).
   The square look from the wrist.
3. Pose 2: calibrate the finger axes, fit the four fingers by values (2A.2); build; the audit must accept it.
4. Pose 3: the thumb by values (2A.3); the build's hull check is the proof of no pass-through.
5. One pose at a time, each judged (render palm side, thumb side, heel) before the next. Then plan the cut.

### The 20 Nov checklist (SUPERSEDED)

1. Rig has `palm_r` (carrier) and `thumb_opp_r` / `thumb_roll_r`, all with references (section 1).
2. `Grip = surface(part=<yours>, solid=collision)`; paste `SwordGrip.gcmove`'s presets.
3. Fit the seat once with points on `palm_r` (square across the fist, about 70 deg off the hand axis, for a two-handed sword
   grip); same turn in every key. Give the arm its twist references (section 1) before you time a cut.
4. Per key: `$SwordSeat $SwordOpen $SwordFist $SwordThumb`, then `collision(self, $Grip)` last, one frame.
5. Build, read the finger angles and notes, render palm side / back / far side / tips; sweep `$SwordOpen`'s pip/dip if a finger misses.
6. Only then plan the cut.
