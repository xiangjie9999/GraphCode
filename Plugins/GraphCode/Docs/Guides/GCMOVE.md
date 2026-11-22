# The animation index: before you write a `.gcmove`

The guides are layered, and this page is the animation layer's index: [README.md](README.md) sends every
animation task here; this page names the guides for each situation; those guides (ANIMATION, HANDS,
MOVE-BALANCE, NATURAL-MOVEMENT, [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md), ...) hold the methods; and
HUMAN-MOVEMENT.md is itself the index of the per-activity human guides below it. Rigs have their own
index, [GCRIG.md](GCRIG.md), and shapes theirs, [GCSHAPE.md](GCSHAPE.md).

Read this page, then every guide it lists for what your clip does, then the nearest shipped example,
**before the first line of the file**. Then ask the tool for each block you will use
(`gc_describe kind:"move" block:"<block>"`). Write in your plan which of them you read.

The guides hold methods that were paid for with failed builds. The most expensive mistakes in this
product came from writing a clip from memory and skipping a step a guide already states, then spending
hours tuning around what the step would have prevented. When a step cannot be done with what you have
(a bone, a contact, a range the rig lacks), do not skip it and do not take a fallback: fix the rig or
the tool, as [GCRIG.md](GCRIG.md) says, and record it.

## You want → write

Look here before you write a workaround or ask for new grammar: several "Move cannot do this" reports
were a spelling that already existed. Each row names the spelling in one line; the last column is where
the tool or a guide explains it (`gc_describe kind:"move" block:"<block>"`). If the tool's help and a row
disagree, the help is right.

| You want | Write | Ask or read |
|---|---|---|
| a planted foot or hand to let go smoothly, not snap off the floor | `plant(<chain>, on=$Floor, weight=$Profile)`; the weight is a profile (0 to 1), and one that falls to 0 over the frames you choose is the release ramp | `block:"constraints"`; [ANIMATION.md](ANIMATION.md) ("bringing its weight to zero") |
| a hand to arrive on a place and stay there | `reach(<chain>, to=$Frame, weight=…)` with the place in the body's words, `point(at=(forward=…, up=…))` | `block:"places"`; [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md) |
| the weight to move over a place, or onto the staying foot before the other lifts | `balance(…, at=)`; with neither `weight=` nor `at=` it shifts the weight `lead=` seconds ahead of the lift | `block:"balance"`; [MOVE-BALANCE.md](MOVE-BALANCE.md) |
| a walk, swing or kick timed by how fast a joint travels | `travel(<bone>, velocity=…)` over key poses, plants written inside them | [WALK-AND-RUN.md](WALK-AND-RUN.md) section 6a; `block:"poses"` |
| a grip carried from one clip into the next | `pose(source=/Game/…/A_Grab)`: that built clip's last frame, so the next clip starts holding | `block:"poses"`; [HANDS.md](HANDS.md) |
| a stride, a swing or any clip played several times | `clip(from=$Walk, repeat=4) { }` | `block:"clips"` |
| one bone of a pose set whatever the presets or an aim give it (a bone override), or a key written again as preset values | `swing(upperarm_r, at=(fold=61.3, side=-59.2, twist=-37.5))` replaces the bone's readings (a plant or a reach still holds on its chain); `[as_presets]` on a build line converts the key (read-only notes); the Pose tab's **Convert to presets** | [MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md) part 6 |
| a pose or clip with values that can be changed where it is used (one lean, used at 20 and at 45) | declare each value in the parentheses, `Back = pose(lean=0.0) { swing(spine_01, axis=fold, by=$lean) }`, read it as `$lean`, and call `$Back(lean=45)`; `$Back` alone is the defaults | `block:"poses"`; [POSES.md](POSES.md#a-pose-with-values) |
| a held prop let go into physics | `detach($Prop, at=) [physics]` | `block:"objects"` |
| a smile, a blink or an open jaw | the FaceExpressions adapter and `drive(smile, by=$Smile)`; there is no expression verb | [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md) |
| a shape the skin takes without bones (a breath, a throat puff) over the clip | the rig makes it in `morphs { }` and every morph is a control of its own name, so `drive(Breathe, by=$Breath)` with no adapter; a curve-only control takes weights below 0 and above 1 | `gc_describe kind:"rig" block:"morphs"`; [GCRIG.md](GCRIG.md) |
| to see whether a motion is smooth or jerky | the build's kinematics note, then `gc_render` with a `trails` highlight and `graph:true` | [VALIDATION.md](VALIDATION.md#judge-a-clip-on-its-trails-and-its-graph) |
| two physics bodies (a wheel and its chassis, a held thing and a hand) to ignore each other at runtime | `collision_pairs { A\|B = disabled }` on the `physics_asset`, or the body setup's collision response. Do not rely on `DefaultInstance.CollisionEnabled` on a skeletal body: Unreal takes a skeletal body's response from its body setup and the component | `gc_describe kind:"physics_asset" block:"collision_pairs"`; [MOVE-PHYSICS.md](MOVE-PHYSICS.md) |
| a chain kept clear of a surface for the whole clip (clearance above a sill, a table, the floor) | a clip line `collision(self, $Surface)` after the poses, with `Surface = surface(mesh=/Game/..., at=, rotate=, scale=)` or `ground(z=)`: it is checked over the whole clip, between keys too, and slides what overlaps out by the least joint turn. Frame 0 must start clear. `pose(collision=$Floor)` stops where it first touches instead. There is no margin word; ground allows no allowance | `block:"constraints"` |
| both hands on one held thing (a rifle, a sword) | no single two-hand word: `attach($Gun, to=palm_grip_l)`, the support place as a `frame(socket=$Gun.<Socket>, toward=, up=)` and `reach(arm_L, from=$Palm, to=$Guard)` | [HANDS.md](HANDS.md) ("a rifle in both hands"); `block:"places"` |
| an arm's elbow and hand where a reach puts them, but the upper arm rolled your way (which of its roll and the forearm's twist takes the turn) | `reach(sword3_r, to=$SlashHand, upper_roll=-24.3)`: the upper arm held at that roll, as `[readings]` print its twist; the forearm's twist takes the rest, or, on an elbow that twists past a quarter turn (its roll goes with where the elbow is), the elbow goes round to the swivel that gives that roll, the hand's place kept. Without it the limb chooses. The build says what it held and what the limb alone took; -180 to 180 only (else refused), and where the word cannot act (a forearm held at a written angle, a chain that is not a three-joint limb) the build says it is not used | `block:"constraints"` |
| a palm that faces a direction, or a forearm that rolls | a hand's facing is a frame, `frame(socket=..., toward=, up=)` or `frame(at=, toward=, up=)`, reached with `reach(<chain>, to=$Frame)`; a roll is `swing(<bone>, axis=twist, by=<degrees>)`. There is no pronate or supinate word | `block:"places"`; [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) |
| follow-through, overlap or lag along a chain | `follow=auto` (or `follow=proximal`) on `swing(...)`: larger contributions lead smaller ones and the delays spread over a share of the duration. A counter-swing and a lead or lag are `[share=]` and `[phase=]` | `block:"constraints"`; [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md) |
| a cut or strike whose tip is fastest at the contact | `Tip = point(bone=, at=(x, y, z))`, then `$Cut [accel=66%, point=$Tip, strike=$Contact]` on a timed pose travel; the build notes say when and how fast the point is fastest and how far the limb is extended then | [POSES.md](POSES.md#a-strike-name-the-point-and-the-pose-it-lands-on); [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md) |
| a pose-based human action (strike, slash, wave, reach, gesture) from the shipped presets | the `presets:/GraphCodeMove/Presets/Human/HumanBody` header, two or three looked-at poses, one effort across, the graph as the gate, then the pose tab | [HUMAN-PRESET-ANIMATION.md](HUMAN-PRESET-ANIMATION.md); [MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md) |
| a hand or an aim that follows a moving thing | name a point or frame on that thing's bone or socket, `point(socket=$Gun.Trigger)` or `frame(socket=...)`; a named point follows its animated pose at every sample | `block:"places"` |
| a sword slash: a wind-up, a cut through a target, an end | a keys file of two one-frame keys (the hand's place, `[share=]`, plants, `balance(at=)`, the head on the target), then a cut file: `pose(steps)` of the built keys on `[clock=blade_r]`, `travel(blade_r, ..., path=trailing)`, the target as `reach(sword3_r, from=$Cutting, to=$Target)` with `Cutting` a point on the blade bone | `block:"constraints"`; [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade) (the short form) |

Three things the build does that look like faults and are not:

- **The clip is stored at twice the fps you wrote.** When a joint limit or a collision dip between two keys
  needs in-between samples, the build doubles the stored rate (at most twice for a dip) and says so:
  `WARNING joint limit: … bake rate increased from N to M fps`. Duration and arrival times are unchanged,
  and the build notes name both the stored and the authored frame. Do not re-time the clip to "fix" it.
- **A held limit that is released changes speed.** A joint asked on past a stop stays at the stop and does
  not ease into it, so the release shows a speed step. Fix the asked pose or the range; slower timing does
  not help ([ANIMATION.md](ANIMATION.md)).
- **A blade cannot roll 180° in the forearm.** The MetaHuman template's forearm turn (`twists=`) is
  ±90.6°, the anatomical stop ([HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md)), and a sword cut that
  reached −88 of it already read as an awkward hold. A roll beyond the stop needs a regrip part-way
  through the clip or a smaller roll; widening a stop that is already the anatomical figure breaks the
  pose rather than fixing it.

A shoulder's twist range can follow the arm's elevation, on a rig that says so with `twists_raised=` on that joint ([RIG.md](RIG.md) has the key). Move's solve, balance, collision's fit and the cut's build then read the range at the pose's own elevation, so a refusal about that joint at its stop names the stop at that elevation; a rig without the key keeps one range at every elevation.

**One designed pose at different amounts is one pose with values, not several retyped poses.** A `name=value` in `pose(...)` or
`clip(...)` that is not one of that kind's own settings is a value of yours with that default; the lines read it as `$name`, and a
caller sets it by name:

```gcmove
Back = pose(lean=0.0, round=0.0) {
  swing(spine_01, axis=fold, by=$lean * (1 - $round))
  swing(spine_03, axis=fold, by=$lean * $round)
}
Stance = clip(frames=1) {
  $Back(lean=45, round=0.8)
}
```

Arithmetic is read only in a value that a parameter was written into. The build notes name each use as the pose with its values,
`Back(lean=45, round=0.8)`. Ask the tool for the exact grammar and its limits (`gc_describe kind:"move" block:"poses"`), and read
[POSES.md](POSES.md#a-pose-with-values) before the first one.

**Three ways to make a motion, each a main way; choose by what is hard about it (user 2026-10-26: "simple movement, use normal
approach, more complex ones use video or image based").** Every way ends in the same `.gcmove`: key poses that GraphCode joins,
plants, times and keeps clear of itself. They differ in where the key poses come from.

| The motion | Make it by | Why |
|---|---|---|
| **Simple**: a joint or two moving, a look, a reach, a hand on a place, a strike at full extension, a stand or a step, anything the words say exactly, or anything the table below has a shipped example for | **Authoring** (key poses in words, then constraints and speed, below) | Exact and editable, no extra tools; the words carry the intent (where the hand goes, what the weight does) |
| **A complex POSE**: a whole body hard to describe joint by joint (a sit, a stretch, a crouch, a curl, a pounce's gather, a fall's landing), where the motion between the poses can be GraphCode's | **Still pictures** of each key pose, from an image model or an artist ([From still pictures](MOTION-FROM-VIDEO.md#from-still-pictures-key-poses-an-image-model-made)) | A picture says the whole pose at once; a few pictures, each from two sides; measured about 1-2 cm a pose from one picture, 0.7 from two |
| **A complex MOTION**: the path and timing between poses are the point (a gait's character, a combination, a tumble, a dance phrase) | **Video** from a video model or filmed ([MOTION-FROM-VIDEO.md](MOTION-FROM-VIDEO.md)) | The in-betweens are measured, not drawn; key it every few frames to keep them, at its landings to let GraphCode draw them |

Mix them freely: author the motion and take one hard pose from a picture, or key a video at its landings and author what it got
wrong. When the table below has a shipped example for this motion, follow it. When it has none, recommend one of these to the user
by the rule above, say in plain words what it needs, and ask before the first line: authoring needs the poses described; pictures need
each key pose drawn from a GraphCode render of the character as the reference, whole body in frame, from two sides; video needs a
generated video made from a first frame GraphCode renders, two camera views of one motion if the video tool allows it. Pictures and
video need the joint reader (an optional download, taught once per character).

## Every `.gcmove`

| Read | For |
|---|---|
| [ANIMATION.md](ANIMATION.md), including [natural movement](ANIMATION.md#natural-movement-is-part-of-the-performance) | what each instruction does, profiles, poses, clips, constraints, effort timing and checking a clip |
| [ASSET-QUALITY.md](ASSET-QUALITY.md) | the bar a shipped animation is held to |
| [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md) | effort timing (`accel=`, `velocity=`), the body joining an action (`[share=]`, `[clock=]`, `[phase=]`, `follow=`), so motion is not evenly interpolated keys |
| [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) and [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) | a human: real ranges, speeds and how to map an angle to a rig control |
| [VALIDATION.md](VALIDATION.md) | proving the result: renders from several sides, playback, and the build's own notes |

**Write every place the way a person describes it (user 2026-10-14: "something more intuitive to write and something
that can be learnt", "i never want to see this type of work again").** A place is a landmark and distances in the body's
own directions: `point(at=(forward=145, up=135))` for the opponent, `point(bone=upperarm_r, at=(up=25, back=5),
relative=spine_05)` for "above and behind the right shoulder, as the chest faces", `frame(..., toward=(back, up=15))`
for "pointing back, a little up", and a bone points the same way: `aim(upperarm_r, from=$UpperArm, toward=(down, left=10))`. Never compute a place, a facing or a joint angle outside the tool and paste the
numbers in: if the words cannot say what the pose needs, that is a gap in the tool to fix, not numbers to type
(`gc_describe kind:"move" block:"places"`). A hand's or foot's place does not say where the elbow or knee goes: say it with
an aim on the upper bone (`aim(thigh_r, from=$ThighR, toward=(forward, down=30))`, `ThighR = frame(bone=thigh_r,
toward=calf_r)`), and write that line from where the end is, so the two agree; the build's notes name the one that gave way.

**A place that rides a bone, and a reach with `[share=]`.** `point(bone=upperarm_r, ...,
relative=spine_05)` RIDES the body: the place goes with that bone, so when the trunk or, with `balance`, the hips move, the hand's place
moves with them. A reach with `[share=]` and `balance(...)` builds with such a place, a bone of the trunk or of the pelvis
(`relative=pelvis`) alike; balance reads the place in the same pose it moves the body to. When the hand should stay where the
character stood at rest instead, write `relative=character`:
`point(bone=upperarm_r, at=(right=43%, up=60%, back=43%), of=sword3_r, relative=character)` is measured from where that
shoulder stands at rest, in the character's directions, and rides nothing (the note says "riding no bone"); built with both
plants, `balance` and `reach(sword3_r, to=...) [share=0.4]`, the hand landed within 0.02 cm (`of=` is the chain the reach
moves; a longer chain counts more than the arm can reach). With no `relative=` at all, a place on a bone of the reaching
arm turns with the arm the reach is turning, and the hand ended 73 to 97 cm from where it was meant: the build warns
("its place ... rides 'upperarm_r', which this reach itself turns ... Write relative=character on the place"). How far the
hand goes is still the bubble's max line (next paragraph), not a share you choose.

**The order of a key's lines (measured 13 Nov 2026).** A key builds with its `reach ... [share=]` written before or after
`balance`, and the order still MEANS something: the later line wins (user-ruled 2026-10-26). A reach written after `balance`
is met exactly and the weight gives way to it, and the build then says so: "a reach written after balance wins ...: the
weight ends N cm outside its support ...; no balance certificate". Write the reach BEFORE `balance` when the pose must stand
balanced. A key that takes its shared lines from a parent (`clip(from=$Shared) { reach(...) }`) builds, and its own reach
always comes after the parent's `balance`, so it is the case above: the measured key ended 1.03 cm outside its support with
no balance certificate. Share lines through a parent only where that is what you mean.

**A hand's place is the bubble's MAX EXTENSION (user-ruled 2026-10-23: *"always use the maximum bubble approach, or else the hand
look weird"*, *"save in docs too, or else many times the hand will look awkward"*).** Draw the bubble on the key's source pose with
`key:` naming the key, ask toward the target with no `distance`, and write its max line: the angle is the bubble's `aim:` for a held
thing, and an empty hand is the hand as the forearm carries it there (`point(at=...)`). Never a place inside the max, never a facing the
bubble did not test: those leave the wrist and elbow to bend however the solver can, and the hand looks awkward
([HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade) step 2, [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md)).
*Record, 2026-11-15 (a record of the accepted cut, not a ruling of the user's):* the accepted 14 Nov cut wrote its wind-up as a
share of the arm in a direction, about three quarters out (`Content/__MCPTests/SlashCheck/ThreePhase/TP_Keys.gcmove`,
`CockHand`), and checked it with the bubble on that key; its contact and finish are the bubble's max line. That file wrote
its finish as a share past the arm's length, which is not met, and the weight did not move
([A cut with a held blade](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade), the short form).

**Poses first, then constraints and speed in the middle (user-ruled 2026-10-13: "the rule is simple, poses, then add
constraint and speed in the middle").** A swing, a cut, a step, any motion:

1. **Plan every pose in writing first, with numbers.** Its intent (what it does in the performance); where EVERY part
   is: both hands, the held thing, the head and the gaze, the chest, the hips, each knee, ankle and foot; where the
   weight is and why (it leans forward or back by intent, it is not centred by default). Measure the body first (a
   `[readings]` dry run: segment lengths, footholds, how a held thing sits in the hand) and check each pose against the
   rig's ranges before code. Code then only shows the plan; it is not where the pose is found (user, 2026-10-14).
   **A strike's contact pose is at FULL EXTENSION (user-ruled 2026-10-25: "a punch is always a fully extended arm, anything
   that is not is considered not a punch, same with sword swing"):** the arm straight at a punch's impact and through a cut,
   the knee straight at a kick's contact; the target goes where the extended limb ends
   ([HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md)) (Record 2026-11-15: the accepted cut's wind-up is
   folded; see the record under the max-extension paragraph above.) **Author it in three steps (user-ruled 2026-10-25: "set balance, to let
   the whole body shift front right, ask bubble for fullest reach in front, then author it in"):** `balance(..., at=)` for the weight,
   the bubble's max extension toward the target on that key (`key:`), its line as the place. Never `angle=0` for a straight elbow or
   knee: an angle counts from the bind pose (a MetaHuman elbow's is about 37 degrees bent).
   **Check every pose of the motion in ONE pass, before the first key is built:** list each joint angle every pose needs
   and set it beside the rig's range (the character's file over the shared template). The shared ranges are an ordinary
   adult's, measured in a clinic; a fighter, thrower or dancer needs some widened with a sourced athlete value
   ([HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md), [GCRIG.md](GCRIG.md)). Found one stop at a time, each costs a build
   and a re-plan (BlueEdge_20261013: the shoulder's outward turn, the loaded ankle, then the upper-spine twist).
2. **Build the body from the ground up, and let the hand bring the held thing.** Footholds, legs, hips, trunk,
   shoulders, then each arm written joint by joint (upper arm, elbow, forearm turn); the held thing rides in the hand
   and where it points is a CHECK, not an IK target. IK lands the hand or foot on its place, the last few centimetres;
   never `aim` a held blade through the wrist, and hold wrists near neutral in the rig. Leave no joint at its zero: a
   straight spine, square shoulders and locked elbows are what "stiff" looks like
   ([HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade), BlueEdge_20261013/BUILD.md).
   **What the solver now places for you (MOVE-ACTION1, built 2026-10-15; user: *"You declare you intent, solver do the rest.
   Never guess where hips should go, where chest should go, etc."*).** For a key placed by a reach, write the reach with
   `[share=]`, the plants and `balance`: the trunk turns toward the place, the hips rise or sink for a place past the limb's
   span, the free arm counters, and the planted legs and weight follow
   ([The body joins a reach](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach), clip `C0`). The joint-by-joint lines above are
   then the tool for an OVERRIDE, for a pose that is the author's own style (a lean the share would level), and for what the
   share does not turn (the pelvis, when planted legs hang from it). Start from the reach; add a typed line only to change
   what it gave. (The reach's frame does ask where the held blade points; what stays banned is an `aim` of the blade through
   the wrist.)
3. **When a pose needs what the rig lacks, change the rig, not the pose:** a fighter's range (a standing adult's
   shoulder cannot cock a sword over the shoulder), a chain that does not own the hips, a held wrist; cite the range
   ([GCRIG.md](GCRIG.md)).
4. **Render each pose from the front, the side and above, and read the build's notes** (a joint at its stop, a
   residual, a shortfall) before the next pose. This is a look at one pose, to see that it is right as a pose. Whether
   the motion works is decided only on its path (step 6).
5. **Then the middle.** The motion travels between those poses (`pose(source=)` keys, in-betweens where the path
   needs them); speed by effort (hips first, the hand last) and constraints (plants, contacts, collision last) are
   added there. A walk's or a swing's speed can instead be a joint's own: `travel(root, velocity=0.4m/s)` over keys with the plants
   written inside them times the poses by distance / speed ([WALK-AND-RUN.md](WALK-AND-RUN.md) 6a). **The move first, the held thing last** (user, 2026-10-16: *"don't put sword in first, just author move
   first"*, *"sword in hand, but don't care about grip first"*, *"Blade last"*): author and judge the body's travel with
   the held thing riding the hand and no blade frame, grip seating or held-part collision; add those once the move reads.
6. **Judge the motion on its trails, never frame by frame** (user, 2026-10-16: *"use the new render approach not the by
   frame approach"*, *"so you can actually see the trajectory"*): one `gc_render` of the built clip with
   `highlights:[{type:"trails", of:[blade_r, hand_r, hand_l, pelvis, head]}]` shows each bone's whole path in one
   picture, its spacing the speed, the held bone's pyramids its turning, the centre of mass as dots. A contact sheet of
   frames hides the arc, the spacing and a flip of the blade between two frames. Nothing about a motion is judged,
   ranked, criticised or reported from frames, stills, strips or sheets; a frame is only named to say where on the path
   something happens (user, 2026-11-12: *"you need to always see the flow of the sword not just multiple frames"*;
   2026-11-15: *"Always see the path, don't keep seeing frames only"*). **A jitter is read on its graph**
   (user, 2026-10-17: *"So author can really see the jitterness"*): add `graph:true` (or `"place"`, `"turn"`, `"joint"`,
   `"centre"`) to that entry, `of:` a bone or a rig chain, and `views[].graph` is each bone's velocity beside its
   acceleration over the clip, a red dot where its velocity steps; the legend names the frames.

Do not tune joint angles inside a motion to find where it should end: a one-handed sword cut took six builds that way
and still ended with the hand on the wrong side and the wrist wrong; its end pose, solved once by IK, was right first time.

## What your clip does, and what to read for it

| The clip | Read, in this order | Shipped example |
|---|---|---|
| smiles, blinks or opens a MetaHuman's jaw | [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md), [CHARACTERS.md §9](CHARACTERS.md#9-flagship-recipe-metahuman-face-plus-gcmove-body-and-macro-motion): native face ownership | [FaceExpressions](../Examples/README.md#faceexpressions-smile-blink-and-jaw-opening) |
| closes a hand: a fist, a handle, a sword, a hammer, a tool | [HANDS.md](HANDS.md) Steps 3 and 5, **[The seat is never skipped](HANDS.md#the-seat-is-never-skipped)**, [Holding any weapon](HANDS.md#holding-any-weapon-the-order-and-what-to-do-when-the-hand-cannot-reach), [the `grab()` route for a rig without skin-hull bodies](HANDS.md#the-grab-route-named-pads-on-a-rig-without-skin-hull-bodies); [POSES.md](POSES.md); [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md) | `VRHandRig_20261001/VRHammerGrip.gcmove`, `VRSwordGrip.gcmove` |
| holds a gun or other weapon, or uses one | [HUMAN-WEAPON-HANDLING.md](HUMAN-WEAPON-HANDLING.md), then the row above, then [HANDS.md Step 6](HANDS.md#step-6-hold-a-pistol-the-way-a-person-does) | [HandAndPistol](../Examples/README.md) |
| puts a hand on a place or an object, picks something up | [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md), **Key poses carry the arm; IK lands the last few centimetres**, then **[The body joins a reach](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach)** for a far place | `KitchenCup_20261011/KitchenCupKeys.gcmove`, `KitchenCup.gcmove` |
| carries, hands over, sets down or throws a prop | **[POSES.md: props that change holder](POSES.md#props-that-change-holder)** first -- what a key pose does and does not carry, which chain may weigh when, and how the game must read the holder curve; then [ANIMATION.md](ANIMATION.md) objects (`attach`, `detach`), [HUMAN-LIFT-CARRY-PUSH.md](HUMAN-LIFT-CARRY-PUSH.md), [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md) | `KitchenCup_20261011/KitchenCup.gcmove`; hand to hand: `KitchenCupPass.gcmove` with `BP_PassPlayback` |
| strikes, cuts, swings, kicks or punches | [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md) (with a held blade: [A cut with a held blade](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade); a key is a reach with `[share=]`, plants and `balance`: [The body joins a reach](HUMAN-GRASP-AND-REACH.md#the-body-joins-a-reach)), [POSES.md: travel between two built poses](POSES.md#travel-between-two-built-poses), [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md) (effort clocks, transition poses), [MOVE-BALANCE.md](MOVE-BALANCE.md) | `Balance_20261009/HighKickForward.gcmove`, `PunchGuard.gcmove`, `Kinematics_20261009/TransitionPose.gcmove` |
| stands, crouches, lunges or shifts weight on planted feet | [MOVE-BALANCE.md](MOVE-BALANCE.md), [HUMAN-BODY-SIZE-AND-BALANCE.md](HUMAN-BODY-SIZE-AND-BALANCE.md), [WALK-AND-RUN.md](WALK-AND-RUN.md) "draw the legs in place" | `Balance_20261009/GuardStanding.gcmove`, `StandingBalance.gcmove` |
| walks or runs | [WALK-AND-RUN.md](WALK-AND-RUN.md), [HUMAN-WALKING.md](HUMAN-WALKING.md), [HUMAN-RUNNING.md](HUMAN-RUNNING.md) | [WalkAndRun](../Examples/README.md#walkandrun-a-rolling-walk-and-a-first-run) |
| walks on four legs, or moves one joint at a set speed (a sword swing) | [WALK-AND-RUN.md](WALK-AND-RUN.md) section 6c first, a gait written as RULES: one leg cycle per leg timed by its phase, each pose's joints at their measured angles (`swing(stifle_L, axis=fold, to=67, give=8)`) and its paw on its foothold (`reach(leg_B_L, contact=(Sole_B_L, Pad_B_L), to=$Land)`, the foothold `relative=moving` from the girdle), the build solving each on the gait's own body at its moment, `travel(root, velocity=sweep)`; then 6a for key poses timed by `travel(<bone>, velocity=)`; [POSES.md](POSES.md#travel-between-two-built-poses) | project test content `CatMove/Intent/CatGait.gcmove` (walk, trot and gallop as rules), `Rig/Elephant/ElephantGait.gcmove`; `CatMove/Intent/CatWalkTravel.gcmove`, `BlueEdgeProbe/SwordSwingTravel.gcmove` |
| climbs stairs or a ladder | [STAIRS.md](STAIRS.md), [HUMAN-STAIRS-AND-LADDERS.md](HUMAN-STAIRS-AND-LADDERS.md); in a game [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md) | |
| jumps, lands, falls, or meets gravity and collision | [HUMAN-JUMP-AND-LAND.md](HUMAN-JUMP-AND-LAND.md), [MOVE-PHYSICS.md](MOVE-PHYSICS.md) | |
| turns, looks at something, moves the head | [HUMAN-TURNING.md](HUMAN-TURNING.md), [HUMAN-GAZE-AND-HEAD.md](HUMAN-GAZE-AND-HEAD.md) | |
| sits, stands up, gets down to the floor | [HUMAN-SIT-STAND-FLOOR.md](HUMAN-SIT-STAND-FLOOR.md) | |
| reaches, looks at or holds ANOTHER PERSON (a hand on a shoulder, a strike that lands, a handshake) | Each person is built in their own file; the other is `Him = surface(part=<his skinned component on the stage>, along=<his .gcmove>.<Clip>)`, his clip as his file BUILT it (build his first), `at=` a second of it; places on him are `point(bone=$Him.head, at=(forward=9))`, read in HIS words on HIS rest pose; `reach`/`aim` to them follow his clip, and his physics bodies stop this body in the final check, a hand resting on him let through only where it lies. **A blow from him needs no line:** where his moving body closes on this head faster than a touch, head and neck give from the base of the neck and come back, by measured human defaults the build reports; a `balance(at=)` the feet cannot hold takes one step by itself (`PartnerTakesCross.gcmove`, `StepBackProbe.gcmove`). **A strike lands with no line either:** a timed travel that turns a limb's end back where it touches him goes in with no stop and comes back from rest, the build says where it landed (`CrossLandsEffort.gcmove`). Refused by name: `from=`, `grab`, `drive=` on him | `MetaHumanSpar_20261026/PartnerListen.gcmove` + `HandOnShoulder.gcmove` (BP_TalkStage) |
| opens a door, presses a button, works a device | [HUMAN-DOORS-AND-DEVICES.md](HUMAN-DOORS-AND-DEVICES.md), then the grasp row | |
| plays on a character with hair or clothes | [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md), [GARMENT.md](GARMENT.md) | |
| is one layer of a game's runtime animation (Blend Spaces, Sync Groups, states, additive layers) | [ANIMATION-LAYERS.md](ANIMATION-LAYERS.md), [runtime composition](ANIMATION.md#build-the-runtime-performance-with-native-animation-tools), then [current terrain support](ANIMATION.md#slopes-and-stairs-current-support) | |
| comes from a video: motion measured, not authored | [MOTION-FROM-VIDEO.md](MOTION-FROM-VIDEO.md) (the measuring scripts are not in this release) | |
| needs a bone, a contact, a socket or a range the rig does not have | [GCRIG.md](GCRIG.md) first: change the rig, then come back | |

A clip that does several of these reads every row it touches. A sword combo, for example, is the hand
row (the grip), the strike row, the planted-feet row and the turning row.

## Before you call it done

Activity guides describe how to compose reusable poses and the existing controls/constraints. They do not
add `jump(height=)`, `vault()`, `pickup()` or `rise_from_floor()` keywords. Reuse or adapt the shipped examples
and verify the new body's ranges, contacts and timing; an activity name does not supply those decisions.

- Read the build's notes. A note naming a depth inside the skin, a joint at its limit, a residual or a
  refusal is part of the result; quote it, do not summarise it away.
- Read the kinematics notes line by line: the peak speed, acceleration and **peak jerk**, the **slide while
  planted** line for each plant, and each plant's **let-go** line (the speed of the limb when its plant gives
  it up). A `WARNING` on any of them has a remedy in [VALIDATION.md](VALIDATION.md#read-these-notes-every-time):
  a let-go above 300 cm/s wants the plant's `weight=$Profile` eased out over more frames, and a slide above
  0.5 cm means the plant is not holding.
- Two more warnings that mean the clip is not what you meant: `barely moves` (no bone leaves its first pose
  by 0.5 cm or 1 degree, so it plays as a still; make the amounts bigger) and `held prop ... no finger
  tracks` (a hand holds a prop and no finger joint moves past 0.5 degrees; close the fingers with a `grab()`
  or a hand pose, [HANDS.md](HANDS.md)).
- A grip is judged by each finger's reported depth, then from the palm side, the back of the hand and
  from below ([HANDS.md](HANDS.md#the-seat-is-never-skipped)).
- Look at the whole motion, not one frame: several cameras, the beats, and a playback.
- **How to read the trails and the graph — what clean and jittery measure like, the there-and-back
  spike of a planted foot, the loop seam, and which build notes to read every time:**
  [VALIDATION.md](VALIDATION.md#judge-a-clip-on-its-trails-and-its-graph).
