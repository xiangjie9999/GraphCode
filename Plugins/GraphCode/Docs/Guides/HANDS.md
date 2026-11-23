# Hands: a fist, a held weapon, and walking with it

Step by step, in the order the work is done. Every recipe here was built and looked at on a rigged
hand; the numbers in them are that hand's and that prop's, so measure yours the same way rather
than copying them. The grammar reference for each instruction is [POSES.md](POSES.md) and the tools'
own help (`gc_describe kind:"move"`, `gc_describe kind:"rig"`); this guide is the order to use them in.

Two rules run through all of it:

- **Never type a number you did not measure or ask the rig for.** A coordinate comes from
  `gc_render mode:"ask"` on the mesh; an angle is about an axis the rig declared (`axis=fold`).
- **Look at the result before believing the build.** A grip can pass every check and still hold the
  gun by its corner. Render close-ups from the palm side, the back of the hand and the prop's far side.

## The four files, and the order to build them

| File | Owns | Example line |
|---|---|---|
| `.gcrig` | the hand: joint ranges and fold axes, finger and thumb chains, pads, the grip socket, hull bodies | `web_grip_r = palm_grip_r [at=$web_r]` |
| `.gcshape` | the prop: its origin at the point the hand takes it by, and its own sockets (trigger, thumb rest, handguard) | `SM_Pistol = $Gun [pivot=Web]` |
| the Blueprint `.gc` | the character mesh with its physics asset, and the prop as a component attached at the hand's socket with **no offset** | `@attach[SocketName="web_grip_r"]` |
| `.gcmove` | the motion: `character:<the Blueprint> part:<its mesh component>`, `ik_rig:`, and `surface(part=<the prop component>)` | `Gun = surface(part=Pistol)` |

Build in that order with `gc_apply` on each file. Changing an upstream file means rebuilding
everything below it: a moved socket moves the prop, and every grip clip closes on where it used to be.

## Step 1: measure, don't guess

1. Render the **mesh asset itself** (the static mesh, or the skeletal mesh in its rest pose), not the
   Blueprint: `mode:"ask"` does not answer on a Blueprint capture.
2. Ask for named pixels: `gc_render mode:"ask" capture:<id> points:[{name, pixel:[x,y], depth:0}]`.
   Each answer is a point in the mesh's own space, with the surface normal and the thickness behind it.
3. An asset render's camera is placed in the preview's centred space, not the mesh's. Ask one pixel
   first and compare it with where you aimed; the difference is the offset for the next camera.
4. To see a surface something else hides (the web between thumb and forefinger, seen from the
   fingertips), use an orthographic camera with `cut:"x=<plane>"`.
5. Write each number into the file with a comment saying how it was measured. The next person (or
   you, after a regression) re-measures instead of trusting it.

Axes: use the words the rig declared, `axis=fold`, `axis=side`, `axis=twist`. A positive fold carries
the joint further the way it already bends, so `by=-20` on a knuckle opens it back. `side` and
`twist` signs are not defined by anatomy: probe one with +40 and -40 in a two-frame clip, look once,
and write what you saw beside the joint in the `.gcrig`.

## Step 2: rig the hand (`.gcrig`)

What a hand needs before any grip will work, from a kept-skin rig of Epic's VR hand:

```gcrig
points {
  thumb_opp_r  = (2.581, 1.993, -1.357)   # a second hinge at the thumb base's own point
  thumb_roll_r = (2.581, 1.993, -1.357)   # and a third: the thumb's roll about its length
  thumb_pad_r  = (5.9, 9.0, -4.4)         # measured pads
  index_pad_r  = (3.5, 15.4, -4.0)
  web_r        = (5.3, 6.72, -1.7)        # the crotch of the thumb-forefinger web, measured
}
contacts {
  ThumbPad = contact(chain=thumb_r, at=$thumb_pad_r)
  IndexPad = contact(chain=index_r, at=$index_pad_r)
}
chains {
  thumb_r = thumb_01_r -> thumb_opp_r -> thumb_roll_r -> thumb_02_r -> thumb_03_r
  index_r = index_01_r -> index_02_r -> index_03_r      # from the knuckle, so a reach bends the finger
}
joints {
  hand_r                                   # the palm's bones stay rigid
  index_01_r  [range=(-20, 90), side=(-15, 15)]
  index_02_r  [range=(0, 110)]
  index_03_r  [range=(0, 80)]
  # ...middle, ring and little the same, without side=
  thumb_01_r   [range=(-50, 25)]
  thumb_opp_r  [range=(-30, 60), closes=down]
  thumb_roll_r [twists=(-10, 45)]
  thumb_02_r   [range=(0, 60)]
  thumb_03_r   [range=(-10, 80)]
}
sockets {
  palm_grip_r = palm_r [on=surface, up=-X]   # +X out of the palm, +Z along a held handle
  web_grip_r  = palm_grip_r [at=$web_r]      # same frame, at the web: where a gun is held
}
build {
  SKM_Hand    = skeletal_mesh()
  IKR_Hand    = ik_rig()
  PHAT_Hand   = physics_asset() [bodies=hull, gives=0.35]
}
```

Why each part is there:

- **Three joints at the thumb's base.** A thumb swings in the palm's plane, swings across the palm,
  and rolls its pad toward the fingers. Without the roll its curl sweeps toward the heel of the palm
  and no angle reaches the fingers. Each extra hinge is a bone at the base's own point.
- **Chains that start at the knuckle.** A contact's limb is everything below its chain's first bone;
  a chain from the root would make every finger part of the thumb.
- **A grip socket in the rig, not typed into the Blueprint.** `on=surface` puts it on the skin and
  measures its +X out of the palm; `up=` says which way a handle runs. A socket on a socket
  (`at=$web_r`) keeps that measured frame and only moves the place.
- **Hull bodies with `gives=0.35`.** Each bone's body is a hull of its own skin, and a held thing may
  press 0.35 cm into the flesh, about what a real grasp compresses. With `gives=` left out the hand is
  rigid and a finger resting on the grip is refused as "inside" it.
- **A soft web, for a pistol.** The web between thumb and forefinger is about 2 cm of muscle, and a
  pistol's backstrap is pressed deep into it. Name that skin as a region and let it give more than the
  palm:

  ```gcrig
  regions {
    web_fold_r = region($web_r, long=8, wide=3) [along=($thumb_opp_r, $web_r)]   # thumb base out through the web
  }
  skin {
    web_fold_r [gives=1.0]          # half that muscle at the web's edge, fading to the hand's 0.35
  }
  ```

  With only the palm's 0.35 cm the palm and the thumb's bulge held the backstrap 3 cm off the web, and
  from there the trigger was out of the forefinger's reach. The region is also the place a clip puts
  the gun on (Step 6), and it follows the skin as the thumb opens.

On a whole body, root each arm's chain at the upper arm and give the collarbone its own chain:
`collar_R = spine_03 -> clavicle_r -> upperarm_r`, `arm_R = upperarm_r -> lowerarm_r -> hand_r`.
With the clavicle inside the arm chain, the full-body IK left reaches across the body 33 to 43 cm short.

A rebuilt rig with no palm bone states the grip socket on the hand bone at a measured point, in the
frame a held prop expects. The build aims each bone's +X at its children and keeps +Z nearest up, so
for a right hand in a palms-down T-pose out of the palm is the hand's -Z. The handle does not run
square across the hand: a fist holds it from the heel of the palm to the web, about 24 degrees toward
the fingers. Measure both points on the palm's skin and let the handle run between them:

```gcrig
points {
  web_r  = (-83.3, 2.8, 151.4)    # the web's crotch, on the palm's skin
  heel_r = (-79.9, -4.7, 151.4)   # the palm's heel on the little finger's side, by the wrist
}
sockets {
  web_grip_r = hand_r [at=$web_r, toward=-Z, up=($heel_r, $web_r)]   # +X out of the palm, +Z up the handle
}
```

Square to the hand (`up=-Y`), a rifle's barrel lay along the fingers, so a butt in the shoulder with
the muzzle down needed the hand pointed 42 degrees down and the arm ended 17 cm short.

## Step 3: make a fist

Ask each joint for nearly its limit and let the hand's own bodies stop it. Fingers first, then the
thumb in four moves, each its own pose so the order holds:

```gcmove
MCP = bones(index_01_r, middle_01_r, ring_01_r, pinky_01_r)
PIP = bones(index_02_r, middle_02_r, ring_02_r, pinky_02_r)
DIP = bones(index_03_r, middle_03_r, ring_03_r, pinky_03_r)
Fingers = pose(collision=self) {
  swing($MCP, axis=fold, by=55)
  swing($PIP, axis=fold, by=85)
  swing($DIP, axis=fold, by=75)
}
Out    = pose(collision=self) { swing(thumb_opp_r, axis=fold, by=58) }      # out in front of the palm
Roll   = pose(collision=self) { swing(thumb_roll_r, axis=twist, by=40) }    # pad toward the fingers
Curl   = pose(collision=self) {
  swing(thumb_02_r, axis=fold, by=50)
  swing(thumb_03_r, axis=fold, by=70)
}
Across = pose(collision=self) { swing(thumb_01_r, axis=fold, by=-50) }      # swung across LAST
Fist = clip(frames=48) {
  $Fingers@profile(0=0, 0.45=1, 1=1)
  $Out@profile(0=0, 0.45=0, 0.55=1, 1=1)
  $Roll@profile(0=0, 0.5=0, 0.6=1, 1=1)
  $Curl@profile(0=0, 0.58=0, 0.7=1, 1=1)
  $Across@profile(0=0, 0.7=0, 0.92=1, 1=1)
}
```

What goes wrong otherwise: swung across first, while it still lies in the palm's plane, the thumb
meets the forefinger's side and hangs down it. For a thumb tucked inside the fist, do the thumb's four
poses first with its base lifted only about 15 degrees, then the fingers ([POSES.md](POSES.md#close-a-hand-grips-and-fists-that-look-right)).

**A hand rigged from a bare mesh (Route B), measured on the Bigfoot's left hand, 2026-10-21:**

- **Fold each finger about its crease, not a world word.** Give every finger joint a sided `turns_in=` band at the
  palm-side crease, measured straight out along the palm's normal, and no `closes=`. `closes=down` is world-down: on a
  hand rolled 18 degrees every finger folded 18 degrees off its palm, toward the thumb and into its neighbours, and no
  clip timing could undo it ([RIG.md](RIG.md), Route B step 4).
- **The thumb's base is three hinges here too** (`thumb_01` across, `thumb_opp` out, `thumb_roll` roll, at one point);
  a skin made here takes them since 2026-10-21. One joint with fold, side and twist stopped short across.
- **Probe each hinge's sign on the hinge itself, with no collision** (a clip of plain poses added one at a time: out, then
  rolled, then across, then the knuckle curled). The Bigfoot roll kept the sign its single-joint thumb had, the wrong way
  round on the hinge: every curl then turned the thumb away from the fingers, and no order or angle of the other moves fixed
  it. A roll that brings the pad toward the fingers curls the thumb in toward them; one that turns it away curls it outward.
- **A range counts from straight, and a thumb's base rests off straight.** Ranges worked out about the sculpt's rest (25 back
  to the forefinger, 10 beyond) need the rest bend the build reports added to the pair: the Bigfoot base rested 12 degrees
  across, so `(-35, 38)` stopped the across at 23 ([HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md)).
- **A `contact()` on a chain moves only what is below its first bone.** The across is the base's first hinge (`thumb_01`, on
  Epic's VR hand too), so a thumb touch from that chain moves out, roll and curl but never swings the thumb across: put the
  across in first by a typed pose. On the Bigfoot hand such a touch also ended 2-5 cm short (the plan, "STATE 22 Oct ~00:40").
- **When the middle and end segments outreach the palm's depth** (long fingers), one pose closing knuckle, middle and end
  joints in turn lays a straight tip on the palm before the end joint moves (end joints 8-16 degrees). Give each row its
  own pose with overlapping weights, the end joint starting a little after the middle one:

```gcmove
Knuckles = pose(collision=self) { swing($MCP, axis=fold, by=75) }
Middles  = pose(collision=self) { swing($PIP, axis=fold, by=85) }
Tips     = pose(collision=self) { swing($DIP, axis=fold, by=80) }
# in the clip: $Knuckles@profile(0=0, .5=1, 1=1)  $Middles@profile(0=0, .1=0, .7=1, 1=1)  $Tips@profile(0=0, .2=0, .75=1, 1=1)
```

  Poses on one weight are one form again and close parent first; their weights must differ.

## Step 4: build the prop to fit the hand (`.gcshape`)

1. **Measure the prop** (Step 1): which way its barrel and its top point, which side the right palm
   presses, how far its grip leans, and the exact points a hand uses: the backstrap's pocket under
   the frame where the web goes, the trigger's face, where the thumb rests, the handguard.
2. **Name those points as sockets in the prop's own coordinates**, and make its origin the one the
   hand takes it by. `[pivot=Web]` moves the origin to that socket's position (not its rotation) and
   carries every socket with it.
3. **Never turn a gun in its `.gcshape`.** Build it in its own frame: bore along the forearm, top
   toward the thumb. Hung on the web socket that way, its grip rakes down the palm by its own angle, as
   a real one does. A turn typed into the shape to stand the grip upright like a sword's handle
   (`add($Gun, rotate=(-21, 0, 0))`) laid the grip across the knuckles, pointed the bore 21 degrees
   off the forearm and put the trigger 3 cm above the forefinger. If the gun has to point somewhere
   else in the hand, that is a pose in the `.gcmove`, never the prop's shape.

```gcshape
build {
  SM_Pistol = $Gun [pivot=Web]
}
Gun = mesh() {
  add(mesh('/Game/Weapons/Pistol/Meshes/SM_Pistol'))
  socket(Web,       at=(0, -0.5, 5.6),  facing=-Y)   # backstrap pocket under the beavertail, measured
  socket(Trigger,   at=(0, 4.45, 4.7),  facing=+Y)   # trigger's front face
  socket(ThumbRest, at=(2.1, 4.0, 7.4), facing=+X)   # frame's left side above the guard
  socket(IndexRest, at=(-2.0, 8.0, 6.9), facing=-X)  # frame's right side, finger off the trigger
}
```

4. **Attach it in the Blueprint** at the hand's socket with no offset:
   `Pistol = StaticMeshComponent(StaticMesh=obj'/Game/.../SM_Pistol')` and
   `@attach[SocketName="web_grip_r"]`. The prop's place is the socket's; its size is its own, so a
   thicker grip sits further out by itself.

A grip socket on a bone the motion can move (Epic's VR hand has `palm_r`) lets a clip seat the prop
by touch (Step 5).

~~A hand with no such bone cannot move the prop against the palm, so put the prop's origin on the
surface the palm meets instead: the grip's flank at the backstrap's height, measured, rather than the
backstrap's centre line.~~ **SUPERSEDED 2026-10-13 (user: "ensure and guarantee no more shortcut
ways").** That fallback is the shortcut that skips the seat. Two agents took it for the MetaHuman sword
(`BlueEdge_20261013`, 2026-10-13): the sword rode `palm_grip_r` from the reference pose, where the resting
thumb already lay 1.3 cm inside the handle, and the user called the grip "so weak" (thumb straight off the handle,
middle finger stopped on the ring finger). ~~Every finger stopped 1.2-1.3 cm inside the handle~~ CORRECTED the same
day: that figure is the depth of the step the fit TRIED and refused (the "collision form stopped" note), not where the
finger ended; the fit then bisects back to the touch. Read that note as "the rest of the asked move would have gone
this deep", never as the final depth. **A hand with no carrier bone
is not a reason to drop the seat.** Use one of the two routes in "The seat is never skipped" below.

## The seat is never skipped

Every hold starts with the held thing OUTSIDE the hand and ends with it touching. There are exactly two
ways to do that, and nothing else counts:

1. **The prop moves into the hand:** a carrier bone the clip can shift (`palm_r` on the VR hand; Step 5's
   `Away` then `Seat`). If the skeleton has no carrier bone, add one in the character's `.gcrig`: a
   socket with its own skinless bone, `palm_grip_r = hand_r [at=..., toward=..., up=..., bone=palm_r]`
   ([RIG.md](RIG.md), [GCRIG.md](GCRIG.md)). It is never replaced by the fallback struck out above.
2. **The hand moves onto the prop**, as a person picks a thing up: the prop stands FREE in a grip stage
   (a part on the body, no socket), and `reach(arm_r, from=$Palm, to=$Grasp)` lands the open palm on its
   surface (`Grasp` 0.1-0.2 cm out along the prop's grip socket). The MetaHuman's route: no new bone,
   because the arm is the carrier. Precedent: `KitchenCup_20261011/KitchenCupKeys.gcmove` (`A_CupKeyGrip`).
   A clip that plays the hold afterwards reads only the finger bones from that key
   (`pose(source=<key>, bones=<the hand's finger and thumb bones>)`) with the prop attached at the same
   palm place.

**Never** attach the prop to the palm socket in the clip that CLOSES the fingers on it: at the reference
pose it already lies in the resting hand.

**The build note is the gate, not the picture.** A grip is wrong, whatever it looks like, when its note
says either of these:

| The note says | What it means | What to do |
|---|---|---|
| `collision surface '$X' rests deeper than that inside the skin in the reference pose (...)` | The prop starts inside the hand; every finger may now end that deep in it | Stop. Take route 1 or 2 above. Never accept the build |
| `collision form stopped '<finger>' where it touched; the rest of its asked move was refused (... '$X' is N cm inside the skin ...). That depth is where the asked pose would have gone, not where it stopped` | The finger stopped ON the prop; N is how far past it the pose asked, which is what asking past is for (["Reading the stop note"](#holding-on-a-metahuman-body-what-the-vr-hand-has-that-a-body-lacks-2026-10-13)) | Nothing, if the final audit passes. Judge the hold from renders |

The note does not give a finger's final depth (wording before 2026-10-13 did not say so, and two agents misread it).
The final audit refuses a finger left inside; look at the hold from the palm side, the back of the hand and from
below, before saying a grip is done.

## Holding any weapon: the order, and what to do when the hand cannot reach

These come from building the pistol and the rifle on a real hand, with an author judging every picture. They apply
to a knife, a tool or a controller as much as to a gun.

**The order.**
1. **The weapon into the hand by touch.** Start it off the palm and bring it in with a collision pose until it meets
   the hand (`Seat`, Step 6). Never put it on a point of skin that moves with a finger or the thumb. This step is
   NEVER optional: a hand with no carrier bone takes the other route in "The seat is never skipped" above.
2. **The fingers that hold it,** each asked past the weapon and stopped where it touches. In the note, a finger
   stopped along its own segments is wrapped; one stopped only at its tip is poking.
3. **The thumb: across first, then curled.**
4. **Only then the finger that works it** (the trigger finger, a button thumb): a `reach` with `collision=none`,
   rising after the hold is made. Declare where it goes; do not wait for a solver to find a way in.
5. **`collision(self, $Weapon)` at the end of the clip** pushes out whatever a plain pose left overlapping, frame by
   frame. Poses are declared; collision only cleans up and reports.

**When the hand cannot reach** (the note ends *"... could not be put where the file asked ... At a limit then:
'<joint>' fold at N°"*), the joint is at its stop: the anatomy is the limit, not the solver. Climb this ladder one
rung at a time, and look at every finger after each change:

1. **Check the limit is a human one.** The note's angles are measured from the rest pose, which is often already
   bent: the template forefinger's middle joint rests at 14.9°, so its "95°" is 110° from straight, the top of a real
   finger. Raise a limit only up to what a real joint does (`.gcrig` `joints { }`, see [RIG.md](RIG.md)); never past.
2. **Change the touching area.** Pad to fingertip, or to the middle of the finger: declare a second contact on the
   rig beside the pad (`IndexTip = contact(chain=index_r, at=$index_tip_r)`) and reach with it.
3. **Change the grip.** Slide the weapon in the hand with `drive=<the bone it rides>`, and state the distance with
   `slide=<cm>` when the searched one costs another finger. On the template pistol, the 0.83 cm that brought the
   trigger to the pad also put the trigger guard on the middle finger's base. Every rung moves something else.
4. **Change the point on the weapon.** A lower or nearer point of the same surface (a new socket in the `.gcshape`),
   only where a real finger would press.
5. **Accept a near touch, and say how near.** A natural hold with the finger 0.8 cm off the trigger is better than a
   finger bent past what a hand can do. The report is the record.

**Reusing a finger that worked.** `pose(source=/Game/.../A_Built, bones=$Index)` takes only the named bones from a
built clip's last frame, so a finger placed well in one build is reused in another without typing its angles.

**Working this way.** One change per build, the same cameras before and after, every finger looked at after each
change, because fixing one finger moves another. When something that worked stops working, rebuild the earlier
version's own text first and compare: that tells a changed recipe from a changed engine.

## Step 5: hold a handle (a sword, a hammer)

The hand opens, the handle comes in until it touches the palm, the fingers close until they touch,
and the thumb is brought onto the fingers by a touch, not by angles.

```gcmove
Blade = surface(part=Hammer)
Held  = profile(0=1, 1=1)
Open = pose() {                                  # fingers straight, thumb forward off the palm
  swing($MCP, axis=fold, by=-18)
  swing(thumb_opp_r, axis=fold, by=40)
  swing(thumb_02_r, axis=fold, by=-14)
  swing(thumb_03_r, axis=fold, by=-10)
}
Away = pose() { shift(palm_r, axis=(0.09, 0.97, -0.23), by=2) }                    # the socket's +X, in palm_r's frame
Seat = pose(collision=$Blade) { shift(palm_r, axis=(0.09, 0.97, -0.23), by=-3) }   # asked past; stops on touch
Fingers = pose(collision=(self, $Blade)) {
  swing($MCP, axis=fold, by=73)
  swing($PIP, axis=fold, by=85)
  swing($DIP, axis=fold, by=75)
}
ThumbRest = bones(index_02_r, index_03_r, middle_02_r, middle_03_r)
Grip = clip(frames=48) {
  $Open@$Held
  $Away@$Held
  $Seat@$Held
  $Fingers@profile(0=0, 0.55=1, 1=1)
  contact(thumb_r, on=$ThumbRest, weight=profile(0=0, 0.6=0, 0.95=1, 1=1))
}
```

The shift's axis is the grip socket's own +X (out of the palm) written in the frame of `palm_r`, the
bone it rides; on this hand it is (0.09, 0.97, -0.23). It is the palm's measured normal, not a guess.
The same lines held a thin sword handle and a thick hammer handle with nothing changed but the
Blueprint and the part's name: that is the test that the socket and the pivot are right.

## Step 6: hold a pistol the way a person does

A person opens the hand wide between thumb and forefinger, puts the gun's backstrap deep in that web, lays the
trigger finger along the frame, wraps the other three fingers under the guard, wraps the thumb round the other
side, and only then puts the trigger finger on the trigger. Write it in exactly that order: the whole hold first,
as ONE pose made of steps, and the trigger last, as its own line. The sample is
`GraphCodeExamples/VRHandRig_20261001/VRPistolGrip.gcmove`, shipped as text in [Docs/Examples/HandAndPistol](../Examples/HandAndPistol).

```gcmove
Gun         = surface(part=Pistol)
TriggerFace = point(socket=$Gun.Trigger)
WrapMCP = bones(middle_01_r, ring_01_r, pinky_01_r)
WrapPIP = bones(middle_02_r, ring_02_r, pinky_02_r)
WrapDIP = bones(middle_03_r, ring_03_r, pinky_03_r)
Open = pose() {                                     # 1. the gun starts off the palm; the hand open,
  shift(palm_r, axis=(0.09,0.97,-0.23), by=5)       #    fingers back, thumb spread away and forward
  swing($MCP, axis=fold, by=-18)
  swing($PIP, axis=fold, by=-10)
  swing($DIP, axis=fold, by=-10)
  swing(thumb_01_r, axis=fold, by=30)
  swing(thumb_opp_r, axis=fold, by=45)
  swing(thumb_02_r, axis=fold, by=-90)
  swing(thumb_03_r, axis=fold, by=-90)
}
Seat = pose(collision=$Gun) {                       # 2. the gun comes into the palm until it touches
  shift(palm_r, axis=(0.09,0.97,-0.23), by=-6)
}
IndexAlong = pose(collision=(self, $Gun)) {         # 3. the trigger finger lies on the frame
  swing(index_01_r, axis=fold, by=120)
}
Close = pose(collision=(self, $Gun)) {              # 4. middle, ring, little: each stops where it touches
  swing($WrapMCP, axis=fold, by=73)
  swing($WrapPIP, axis=fold, by=85)
  swing($WrapDIP, axis=fold, by=75)
}
ThumbAcross = pose(from=reference) {                # 5. the thumb swung across the frame, STRAIGHT
  swing(thumb_opp_r, axis=fold, by=80)
  swing(thumb_roll_r, axis=twist, by=35)
  swing(thumb_01_r, axis=fold, by=-40)
  swing(thumb_02_r, axis=fold, by=0)
  swing(thumb_03_r, axis=fold, by=0)
}
ThumbCurl = pose() {                                # 6. then curled onto the far side
  swing(thumb_02_r, axis=fold, by=90)
  swing(thumb_03_r, axis=fold, by=90)
}
Hold = pose(steps) {
  $Open
  $Seat
  $IndexAlong
  $Close
  $ThumbAcross
  $ThumbCurl
}
Last = profile(0=0, 0.7=0, 1=1)
Trigger = clip(frames=48) {
  $Hold                                                                            # the whole hold first
  reach(index_r, contact=IndexTip, to=$TriggerFace, collision=none, weight=$Last)  # 7. then the trigger
  collision(self, $Gun)                             # anything a pose left overlapping slides clear
}
```

How each step works, and why it is written that way:

- **`pose(steps)`.** The first line is where the pose starts, held from frame 0; every later line comes in over an
  equal share of the clip, in written order, from where the one before ended.
- **The seat is a touch: the gun comes in until it meets the hand.** `Open` shifts the bone the gun rides
  (`palm_r`) 5 cm out of the palm, and `Seat`, a collision pose, brings it back until the gun touches. The axis is
  this hand's own "out of the palm", measured once. Do not seat on a region of skin (`contact($Gun.Web,
  on=web_fold_r)`): that region's nearest point here rides the thumb, so it moved as the thumb opened and closed and
  left the gun 1 cm low, where the little finger could only poke the grip with its tip (2026-10-06).
- **The trigger finger lies along the frame before the other fingers close,** so the gun is gripped with it out of
  the way, as a shooter's is.
- **A collision pose closes a finger knuckle first,** then its middle joint, then its tip. On this grip that wraps
  the three fingers. Read the build note: each finger's segments are stopped in their OWN skin (`'pinky_01_r'`,
  `'pinky_02_r'`, `'pinky_03_r'` each just inside `$Gun`). If only a finger's last segment is stopped, it is poking
  the grip with a straight finger: curl its middle and end joints first in a step of their own (`CurlWrap` below),
  or move the grip so the fingers have room.
- **The thumb swings across straight, then curls.** Swung and curled in one step, the curled tip meets the frame's
  side before the thumb gets round. `ThumbAcross` is `from=reference`, so it says where the joints END rather than
  how far they move from `Open`'s spread; its 0s straighten the outer joints for the swing.
- **The trigger comes last, collision-free.** `collision=none` leaves the whole trigger finger out of the gun's
  collision for this line: the gun does not stop it, and the build REPORTS how deep it goes in, at which frame and at
  the end (*"bones '$Gun' does not stop go THROUGH it ..."*); it never refuses for it. The weight rises only after the
  hold is made, so the finger moves from its place on the frame.
- **The fingertip, not the pad.** In this full wrap the trigger sits close to the forefinger's knuckle. With the
  middle joint already at a real finger's 110°, the pad ends 1.4 cm short and the tip 0.8 cm short: the rig declares
  an `IndexTip` contact beside `IndexPad` for exactly this (the ladder in "Holding any weapon").

```gcmove
CurlWrap = pose(collision=(self, $Gun)) {           # only when a finger's tip pokes: middle and end first
  swing($WrapPIP, axis=fold, by=60)
  swing($WrapDIP, axis=fold, by=40)
}
```

Look at the result from three sides before believing it: the palm side, the back of the hand, and the gun's thumb
side (give the camera `exposure`, it is in shadow). A trigger reached while a middle finger rides up into the guard
is not a grip.

## Step 7: grab a rifle in the natural order

Put the weapon in the hand, close the four fingers, then the thumb: sideways across first, then
folded in. Every closing is a collision pose, so the gun stops each joint where it touches.

```gcmove
Gun = surface(part=Rifle)
Spread = pose() {                                # fingers back, thumb spread clear of the palm
  swing($MCP, axis=fold, by=-18)
  swing($PIP, axis=fold, by=-10)
  swing($DIP, axis=fold, by=-10)
  swing(thumb_01_r, axis=fold, by=25)
  swing(thumb_opp_r, axis=fold, by=-30)
  swing(thumb_02_r, axis=fold, by=-14)
  swing(thumb_03_r, axis=fold, by=-10)
}
FourFingers = pose(collision=(self, $Gun)) {
  swing($MCP, axis=fold, by=91)
  swing($PIP, axis=fold, by=95)
  swing($DIP, axis=fold, by=85)
}
ThumbSideways = pose(collision=(self, $Gun)) { swing(thumb_opp_r, axis=fold, by=85) }
ThumbFoldsIn = pose(collision=(self, $Gun)) {
  swing(thumb_01_r, axis=fold, by=-61)
  swing(thumb_02_r, axis=fold, by=62)
  swing(thumb_03_r, axis=fold, by=79)
}
Grab = clip(frames=48) {
  $Spread@$Held
  $Away@$Held                                    # as in Step 5: start clear, seat by touch
  $Seat@profile(0=0, 0.25=1, 1=1)
  $FourFingers@profile(0=0, 0.3=0, 0.6=1, 1=1)
  $ThumbSideways@profile(0=0, 0.6=0, 0.75=1, 1=1)
  $ThumbFoldsIn@profile(0=0, 0.75=0, 0.95=1, 1=1)
  collision(self, $Gun)
}
```

The other order fails visibly: folded at its base first while still turned back, the thumb sweeps over
the top of the forefinger and lies there.

## Step 8: keep the hold in every later clip

Do not solve the grip again in each clip. Build the grab once, then start any clip that holds the
weapon from the grab's last frame:

```gcmove
build { A_RifleHold = $Hold }
Grabbed = pose(source=/Game/Weapons/A_RifleGrab) { }
Hold = clip(frames=48) {
  $Grabbed
}
```

`pose(source=)` takes a built, non-additive AnimSequence on the same skeleton, and it must not be an
output of the same file: the grab has its own `.gcmove` and is built first. Each bone becomes one
shift and one swing from rest, so at full amount the pose is that frame exactly. Rendered side by
side, the hold's first frame and the grab's last differed by at most 3 pixels.

## Step 9: a rifle in both hands, then walk with it

Three files, each built from the last one's end (`RifleWalk_20261003`, built and looked at 2026-10-04):
**Ready** places the rifle and the support palm with the hands open, **Grab** closes the fingers, and
**Walk** walks with the grab's last frame. Ready takes about 20 s to build, Grab about 100 s, Walk
about 75 s for both its clips.

**Ready: say where the rifle goes, not the hand.**

```gcmove
Gun = surface(part=Rifle)
ButtFrame  = frame(socket=$Gun.Butt, toward=-Z, up=X)     # the stock's plate: +X down the barrel, +Z its top
LowReady   = frame(at=(-16.5, 8.2, 150), toward=(0.551, 0.702, -0.451), up=(0.278, 0.355, 0.893))
GuardFrame = frame(socket=$Gun.PalmL, toward=(-0.707, 0.707, 0), up=-Z)   # the left palm up under the handguard
Ready = clip(frames=2) {
  $ArmsDown@$Held                                 # where the solve starts
  $Open@$Held                                     # fingers back, thumbs spread, so they arrive clear
  reach(arm_R, from=$ButtFrame, to=$LowReady)     # the butt in the shoulder pocket, the barrel down and across
  reach(arm_L, contact=Palm_L, to=$GuardFrame)
  collision(self, $Gun)
}
```

- `at=` is the shoulder pocket measured on the skin; `toward=` is the barrel (+X the character's
  left, +Y ahead): 27 degrees down and 38 across, a patrol carry. **Across, because the support hand
  holds the handguard's back, about 49 cm down the rifle from the butt.** Pointed 15 degrees across,
  that stood 68 to 73 cm from the left shoulder, past a 61 cm arm, and the left hand ended 18 cm short.
- The grip socket must run heel to web (Step 2). Square to the hand, the arm could not bring the butt
  to the shoulder at all.
- If an arm comes back short, the solve swings its elbow round the line from shoulder to hand by
  itself and the build notes the turn. There is no seed pose to tune by hand.

**Grab: only the fingers move.** Start from Ready's last frame and close each hand as in Step 7: four
fingers, then the thumb across, then folded in, the right hand first.

```gcmove
Ready = pose(source=/Game/GraphCodeExamples/RifleWalk_20261003/A_RifleReady) { }
FourR = pose(collision=(self,$Gun)) { swing($KnucklesR, axis=fold, by=110) ... }
Grab = clip(frames=24) { $Ready@$Held  $FourR@$RightCloses  ...  $ThumbFoldL@$LeftThumbFolds }
```

**Walk: the steps are the feet's.**

```gcmove
build {
  A_WalkerRifle  = $Carry [loop, root_motion]   # the game's: its character's movement takes the travel
  A_WalkerRifle4 = $Carry4                      # a take: four strides, the travel kept in the root bone
}
Floor = ground(z=0)
OnGroundR = profile(0=1, 0.6=1, 0.62=0, 0.98=0, loop=true)          # heel strike to toe-off
OnGroundL = profile(0=1, 0.1=1, 0.12=0, 0.48=0, 0.5=1, loop=true)   # the same, half a stride later
Walk = clip(frames=34) {
  swing(thigh_r, axis=fold, by=$Hip)
  swing(thigh_l, axis=fold, by=$Hip) [phase=0.5]
  ...                                            # knee, ankle and ball the same way
  plant(leg_R, on=$Floor, contact=Sole_R, drive=Root, weight=$OnGroundR)
  plant(leg_L, on=$Floor, contact=Sole_L, drive=Root, weight=$OnGroundL)
}
Gripped = pose(source=/Game/GraphCodeExamples/RifleWalk_20261003/A_RifleGrab) { }
Carry  = clip(from=$Walk) { $Gripped }
Carry4 = clip(from=$Carry, repeat=4) { }
```

- `plant(..., drive=Root)` holds each foot where it landed while it bears weight and moves the body
  over it, so a standing foot never slides and the stride length is whatever the legs make.
- **Measured 2026-10-07: with `contact=Sole_R` this walk's planted foot stands 6.3 cm above the floor.** An ordinary
  plant keeps the goal bone's (the toe's) turn from the heel-strike frame, and the heel contact hangs 22 cm off it.
  Plant a walking foot with `roll=(Heel_R, Ball_R, Toe_R)` and contacts on their own bones instead: the same legs then
  stand on the floor and roll heel to toe ([WALK-AND-RUN.md](WALK-AND-RUN.md), example `RollWalk.gcmove`). This shipped
  walk is not switched yet.
- **In a game, use the looping clip with `[root_motion]`.** The engine then takes the root's travel
  for the character's movement (an animation blueprint set to root motion from everything, or a
  montage), so the capsule moves exactly as far as the feet stepped. Without `[root_motion]` a
  looping clip snaps back to its start every cycle.
- **For a render or a sequence, use the `repeat=` take without `[root_motion]`.** It keeps the travel
  in the root bone, so it walks off wherever it plays; with `[root_motion]` the root is locked for a
  movement component and a preview walks in place.
- The arms hang off the chest and the rifle off the right hand, so the hold rides the walk with no
  solve of its own.

## The `grab()` route: named pads on a rig without skin-hull bodies

Steps 3 and 5 close a hand with collision forms and need a hand with skin-hull bodies. For a rig without them, `grab` closes named pad
contacts on a fixed surface: this is that route, step by step (the first grip proof, a static target). It was written in
[ANIMATION.md](ANIMATION.md) as "Make a hand grip a sword" and moved here, word for word, on 2026-10-20 (a DOC-STRUCTURE split of
that guide). Where the hand has skin-hull bodies, use Steps 3 and 5 and
[POSES.md](POSES.md#close-a-hand-grips-and-fists-that-look-right) instead.

1. Position the wrist and arm near the hilt with the palm facing appropriately. Use the rig's actual
   hand dimensions. Keep the blade away from the selected grasp surface.
2. Declare independent digit chains and contact points on the usable pads. Include the thumb or
   another opposing member where the intended grasp needs one. Do not use a chain ending only at
   the wrist and expect it to curl fingers. A tip-bone origin may be inside the skin; a named pad
   offset is usually a better measurement.
3. Put the full set of candidates in a named grip and mark compulsory contacts as required. Select
   `contacts=all` or an explicit subset. `contacts=auto` remains unimplemented. The common base is
   derived from all candidates, so releasing one digit should not change what counts as the palm.
4. Reference actual hilt geometry with `surface(mesh=..., at=..., rotate=..., scale=...)` in the
   skeletal component's space. The current query uses the whole referenced mesh. For a sword whose
   nearest surface is the guard or blade, supply a separate real hilt mesh as the contact target;
   there is no current surface-region selector.
5. Run `grab` with a smooth closing profile. With swing/aim and no positional body goals, it keeps the common base and outside
   bones at the authored FK/aim pose, bends selected chains within declared limits, and lets tip
   orientation follow the articulation. Inspect the actual pad error and finger shape, not only
   the wrist's distance to the prop.
6. Stage closure when the performance needs it: define separate thumb and finger grips with
   independent profiles. Either may lead; there is no universal thumb-first rule. The leading
   group can hold while the other closes. Use separate grips because a second instruction for
   the same grip replaces it rather than scheduling a second stage.
7. Inspect thumb opposition, every knuckle, fingertips, palm clearance and skin penetration at
   intermediate frames. The current point-contact solve does not prevent an intermediate finger
   segment or the palm from intersecting the object, and does not certify physical grip strength.

Combine the arm's swing and aim with the digit grip in one clip. To carry the sword, put the hilt
surface on `bone=hand_R` or a native `socket=` and express its transform in that attachment's local
frame. The captured contacts then move with the hand. Attach the visible sword to the same frame
and offset during native playback; the contact bake itself creates no runtime attachment.

For a grip already declared with named contacts:

```gcmove
Hilt = surface(mesh=/Game/Props/SM_SwordHilt, at=(20,0,0))
Close = profile(0=0, 0.35=1, 0.75=1, 1=0)
GripStudy = clip(frames=48) {
  grab(sword_hand, on=$Hilt, contacts=all, weight=$Close)
}
```

The position above is illustrative, not a recommended wrist offset. Measure it on the actual rig.
If the target cannot be reached, correct the approach, contact choice or rig. Read the reported
residual; do not widen anatomical limits merely to turn a contact metric green.

For staged closure, the rig might declare `thumb_hold = grip(thumb)` and
`finger_hold = grip(index, middle, ring, little)` in `grips { }`. These names must match actual
independent chains whose starts include the intended palm base. Prefer named pad contacts for a
production rig. Then the Move fragment is:

```gcmove
FingersClose = profile(0=0,0.2=0,0.45=1,0.8=1,1=0)
ThumbClose = profile(0=0,0.4=0,0.6=1,0.8=1,1=0)
StagedGrip = clip(frames=72) {
  grab(finger_hold,on=$Hilt,weight=$FingersClose)
  grab(thumb_hold,on=$Hilt,weight=$ThumbClose)
}
```

Swap the profiles to lead with the thumb. A staged timing curve does not create thumb opposition
or a collision-safe path; those still depend on the rig, approach and collision coverage.

**A sword swing needs native prop attachment and playback composition.** This `surface` remains
fixed in component space. Leaving its grab active while moving the hand away asks the fingers to
stay behind at that surface. For carrying, give the surface the hand's bone/socket attachment and
local transform, or layer a baked closed grip over the body motion with native animation tools.
Attach the visible prop at the matching socket and coordinate detach/release at the intended frame.
Verify its position and rotation with the hilt visible. The grab bake does not perform those runtime
attachment events.

## When the build refuses: what it means and what to do

| The build says | What it means | Do this |
|---|---|---|
| `form fitting needs a legal starting pose: '$Gun' is 0.61 cm inside the skin of 'index_01_l'` | The first frame already overlaps; a collision pose can only stop motion, not undo a start inside | Open the hand first (`fold` negative), start the prop away and seat it, or bring the hand in with a reach weight that rises from 0 |
| `self-collision in 'Grab' at frame 0: 'spine_01' intersects 'upperarm_l_twist_02'` | A `collision=(self, ...)` pose checks every body pair of the whole character, not just the hand | Start the arms clear of the torso; a clip line that only the prop should stop is `collision($Gun)` |
| `... (allowed 0.000000 cm, their overlap at rest)` between hulls standing well off the skin | Before the plugin update of 2026-10-04 a rolled arm's twist helpers counted as rigid with it, so the arm's skin check fell back to its hulls | Update the plugin; the pair is then checked on the skin |
| `form fitting needs a legal starting pose: '$Gun' is 0.73 cm inside the skin of 'upperarm_r_twist_01'` on a clip that starts from another's last frame | Before the plugin update of 2026-10-04 the checks saw twist helpers unrolled, while the clip bakes their roll, so the earlier clip passed and the later one did not | Update the plugin and rebuild the earlier clip |
| `'foot_r' intersects 'toe_r' ... their overlap at rest` in a walk | A bare toe (no region, no limit) was checked against the foot across the ball | Update the plugin and rebuild the `.gcrig`: a bare end bone now rides its parent |
| A socket edited in the `.gcrig` changes nothing in the clips | Before 2026-10-04 each rebuild added its sockets beside the old ones, and the engine used the first | Update the plugin and rebuild the `.gcrig` once: it now replaces them |
| `reach 'arm_L': ... it ends 33 cm from its place` with joints at their limits | The place is out of that chain's reach | Move the socket or the start pose; on a body, root the arm chain at the upper arm (Step 2) |
| No error, but a finger lies flat along the grip, bent only at its knuckle (`collision form stopped '<finger>_02_r'` near frame 0) | A collision pose closes a finger knuckle first, and on a thick grip the straight finger lands flat before its middle joint can curl | Curl the middle and end joints first, in a step of their own, then close the knuckle (Step 6, "A collision pose closes a finger knuckle first") |
| `WARNING joint limit ... clamped` | A pose asked past the joint's range | Expected when asking past an object; otherwise the sign or axis is wrong: probe it (Step 1) |
| Nothing, but a later step seems to do nothing (a thumb told to curl stays straight) | Lines on a joint stack, and the limits clamp only the sum: an earlier step's -90 asked past the limit cancels a later +90 | Write the later step `pose(from=reference)`, so it is measured from rest (Step 6) |
| A collision clip takes minutes | Collision against a detailed prop is checked at every sample | Try a change on a copy with `frames=6`, then build the real one |
| A reach ends short with *"At a limit then: '<joint>' fold at N°"* | The joint is at its stop: the hand's anatomy, not the solver | The ladder in "Holding any weapon": check the limit is human, then change the touching area, then the grip, then the point on the weapon |
| A finger's stop notes name only its last segment (`'<finger>_03_r'` inside `$Gun`) | The knuckle closed first and the straight finger pokes the grip with its tip | Curl its middle and end joints first (`CurlWrap`, Step 6), or give the grip room |
| After `drive=` reaches a trigger, the middle finger curls up beside the guard | The slide moved the hand up the grip, and the guard now sits on the middle finger's base | State a smaller slide with `slide=`, or change the touching area instead (the ladder) |
| A reach with `collision=none` reports *"go THROUGH it"* | A report, not a refusal: that finger is not stopped by the part | Read the depth and frame; if it matters, move the place, the grip, or the steps that bring the finger in |

## Check it

Render the grip from four sides at close range: the palm side, the back of the hand, the prop's far
side (where a fingertip can pass through a guard unnoticed) and from the fingertips. Then play the
clip and watch the order: fingers before thumb, thumb sideways before it folds. A pose that reads
right in one view and wrong in another is wrong.

## Holding on a MetaHuman body: what the VR hand has that a body lacks (2026-10-13)

The hammer, sword and pistol holds above were built on Epic's VR hand, whose `.gcrig` was made for holding. A
MetaHuman body is not, and the same lines give a weak grip on it until its rig gets the same four things
(the BlueEdge sword, `GraphCodeExamples/BlueEdge_20261013`, BUILD.md has the record):

| The VR hand has | A MetaHuman body | Status |
|---|---|---|
| a carrier bone the held thing rides (Epic's `palm_r`), so the clip seats it by touch | none; the thing rode the hand from the rest pose, 1.3 cm inside the resting thumb | **fixed:** a socket's own skinless bone on a kept skin (`palm_grip_r = hand_r [..., bone=palm_r]`), on a /Game copy of the body and its skeleton |
| two extra thumb hinges at its base (`thumb_opp_r`, `thumb_roll_r`), so the thumb opposes and its contact lands (0.001 cm) | `thumb_01` folds 44 degrees off its own axes, the IK rig approximates that limit as +-20, and the thumb cannot come across a slim handle (contact stopped 1.32 cm short) | **owed:** the same second hinges in the body's `.gcrig` (a second hinge on a kept skin is built) |
| finger bodies fitted to its own skin (`physics_asset() [bodies=hull, gives=0.35]`) and measured pads | finger collision inferred on the fly from skin weights; 23 neighbour pairs already overlap at rest, so fingers block each other and the final audit refuses their overlap (pinky/ring 0.40 cm) | **owed:** a fitted hull physics asset for the body's fingers |

**What holds on this body today (2026-10-13, `BlueEdge_20261013/MetaHumanGripFirm.gcmove`, built in 18 s):** the hammer's
open and seat (`Away` then `Seat` on `palm_r`), then the four fingers and the thumb as PLAIN poses asked to a full fist over
the clip, and `collision(self, $Blade)` last. The collision line puts each joint where it was asked and slides it out of
what it is in: onto the handle's surface, and the middle finger past the ring finger. With the fingers as COLLISION poses
instead (`pose(collision=(self, $Blade))`), the middle finger froze where its tip first met the ring finger's, short of the
handle (the user: "why the middle finger doesn't curl fully"). This is "a fist first, then the sword in it" (the user)
with `collision()` cleaning up, HANDS.md's "Holding any weapon" step 5. Look at it with the arm raised
(`GripShow.gcmove`): hanging at the thigh, half the cameras land inside the body.

~~**A GraphCode gap, not a recipe choice:** a clip fits ALL its collision poses against ONE set, the union of every
pose's `collision=` and every `collision()` line. So "the fingers stop on the handle, then the thumb stops on the
fingers" cannot be written in one clip: the thumb's `self` stops the fingers on each other. Until each pose keeps its
own set, write it as two clips, the second starting from the first's last frame with `pose(source=)`.~~
**Measured 2026-10-25: not a gap a per-pose set closes.** A clip still fits all its collision poses against the union of
their targets, but fingers whose pose leaves out `self` pass into each other as they curl, and the final audit refuses it
on both hands (the VR hand's middle and index tips 0.41 cm, 0.35 allowed; this body's ring and little finger 0.40), so the
two-clip way above fails at its first clip too. Close the fingers and then the thumb as plain poses timed one after the
other and end with `collision(self, $Blade)`, as above: it slides each finger onto the handle and off its neighbour
(Docs/GC-MOVE-COLLSET-PLAN.md has the record).

**Reading the stop note (corrected 2026-10-13; the note itself now says so):** `collision form stopped '<bone>' where it
touched; the rest of its asked move was refused (frame N: ... '$Part' is X cm inside the skin of '<bone>' ...)` gives the depth of the REST of the asked move, the step the fit tried first and refused
(`GraphCodeMoveFormFit.cpp`: the full remaining form is tried, its failure kept as the reason, then bisected back to
clear). The finger itself stopped at the touch, within the give. A 1.3 cm figure there means "asked 1.3 cm past the
handle", which is what asking past the object is for. Judge the final hold from the renders and the final audit.
