# Human movement: how real bodies move, with numbers

This is the index to a family of guides on how a healthy adult's body really moves: joint ranges, walking, running,
body size and balance, and the everyday tasks (holding a weapon, grasping, lifting, sitting, stairs, jumping, turning,
throwing, looking, using a door or a phone). Each guide gives **numbers with their sources**, so an author or an agent
can draw a movement the way people make it, and never has to research it again.

The guides state the movement. They do not build it. How to build a walk or a run, a grip or a stair climb is in the
build guides ([WALK-AND-RUN.md](WALK-AND-RUN.md), [HANDS.md](HANDS.md), [STAIRS.md](STAIRS.md),
[ANIMATION.md](ANIMATION.md)); each human guide points to the one it needs and says plainly where GraphCode cannot do
something yet. The numbers come from published studies, public datasets and clinical references, and every row names
its source. This page and every guide in the family carry their own numbers and references; none depends on a file
outside the guides.

## What a correct-looking result is

- The numbers in a clip come from one consistent source for one character, not from the best-looking row of each table.
- Every rig range allows the motion the clip asks for, and the build note prints no `WARNING joint limit` lines for it.
- The sign of every control was checked with a one-pose render before a curve was mapped onto it.
- The finished motion was looked at: over a floor, from the side, at the speed it will play.

## The five most common mistakes

1. **Mapping a curve before checking the sign.** A positive number can swing a joint either way on your rig. Render a
   one-pose test first (see "Using the numbers in GraphCode").
2. **Treating a range as a target.** A range is a limit. Normal walking uses a fraction of most ranges.
3. **Mixing angle conventions or sources.** Clinical and laboratory gait angles differ (hip at contact 25° against
   27.7°). Use one source for one character. Values from one lab or one take are examples, not norms.
4. **Reading the build note as if it were the rig.** The rig's limits are from straight; the build note reports from the
   rest pose ("Reading an angle").
5. **A rig that stops short of a person.** The example character's ankle, hip and trunk ranges were too short for a
   walk and a run. Widen the rig to a person's range before animating.

## How to use the family

1. Open the guide for the movement ([the table below](#the-guides)).
2. Read its "What a correct-looking result is" and "most common mistakes" first.
3. Take the numbers from one row or one consistent set. Note the **evidence word** beside each number.
4. Check the angle convention (below) and the sign on your rig; widen the rig's ranges first if the guide says to.
5. Build with the build guide it names, then look at the result and compare it with the numbers.

## Reading an angle

- **Anatomical angles are measured from the anatomical position:** standing straight, arms at the sides, palms forward,
  feet forward. Flexion, abduction, external rotation, dorsiflexion and extension of the toe are the names. Unless a
  row says otherwise, an angle is flexion from that zero. The sign a rig uses is a separate question (below).
- **A rig writes ranges from straight; a build note reports them from the rest pose.** In a `.gcrig`, zero is straight
  and a range is a pair, minimum to maximum. The build note's angles are measured from the rest pose. The VR hand's
  forefinger middle joint rests bent 14.9°, so its `range=(0, 110)` appears in a motion's note as *"fold at 95° (-15 to
  95)"* ([RIG.md](RIG.md), "Reading what the build says about joints"). Check the rest bend before concluding that a
  limit is too tight. Write the anatomical range into `range=`; expect the note to shift it by the rest bend.
- **Two angle conventions run through the gait literature** and explain most of its disagreements. *Clinical
  observational angles* (Rancho Los Amigos and Perry) measure the thigh against vertical. *Laboratory model angles*
  (Davis, Plug-in-Gait, OpenSim) measure it against the pelvis, so pelvic tilt is inside the hip angle. The walking and
  running guides say which each row uses. Do not mix them in one character.
- **A range is a limit, not a target.** A rig must allow at least the motion it will be asked for. Normal walking uses a
  fraction of most ranges.
- **Passive and active ranges differ.** An examiner moves a joint to its passive end; the person reaches the active
  end. Passive is a few degrees larger. A character that moves itself is bounded by the active range
  (see [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md)).

## The evidence words

Every number in these guides carries one of these words, in an "Evidence" column or beside the figure. Some guides print
a short label instead; the table gives both.

| Word | Label some guides print | Meaning |
|---|---|---|
| **cited** | C, "full text", "table" | A named published source whose full text (or the table in it) was read. The row says which study and which population. |
| **abstract** | Ca | A named published source whose abstract only was read. The number is as the abstract, or a table quoted with it, gave it. Where the abstract gave no number the row says so. |
| **secondary** (also written "second-hand") | Cs | A textbook table, a handout or a review that quotes the study. Treat it as one step further from the measurement. |
| **measured** | M | Taken from the curves of a public dataset (named in the row) by running a tool on its files. It describes that dataset's subject or take, not people in general. |
| **derived** | D | Our arithmetic on cited rows. The formula and the assumptions are shown beside it. |
| **unsourced** | U | A common figure nobody could trace. **These guides never use one.** Where a number is missing, the guide lists it under "Not yet sourced"; do not fill the gap with a guess. |

A row marked "numbers unverified" or "title not recorded" is kept so you know the study exists, but plan with it only
after reading the source.

## Using the numbers in GraphCode

**Map the angle to the rig control.** The mapping below is from [WALK-AND-RUN.md](WALK-AND-RUN.md) (step 2, "Take the
curves from measured gait"), written for a rig built like RifleMan, the example character. It is not re-derived here.

| Anatomical angle | Rig control | Note |
|---|---|---|
| hip flexion | `thigh` fold | + swings the thigh forward |
| hip adduction | `thigh` side, negated | the rig's + side is outward (abduction +) |
| hip rotation | `thigh` twist | needs `twists=` in the rig; check the sign |
| knee flexion | `calf` fold | |
| ankle dorsiflexion | `foot` fold | pointing the toes (plantarflexion) is negative |
| toe (first MTP) extension | `ball` fold | stance data only in most sets |
| lumbar extension | `spine_01..03` fold, negated, shared between them | the rig's spine folds forward (trunk flexion +) |
| pelvis rise | `shift(Root, axis=<up>)` | centimetres |
| shoulder flexion | `upperarm` fold, after lowering the arm | |
| shoulder adduction | `upperarm` side = 90 + adduction | on a T-pose rig, side 90 brings the arm down |
| elbow flexion | `lowerarm` fold | |

**Sign-check every control with a one-pose render before you map a curve.** WALK-AND-RUN's own test: a two-frame clip
with `swing(upperarm_r, axis=side, by=80)` then `swing(upperarm_r, axis=fold, by=40)`, rendered from the front and the
side, showed on RifleMan that side +80 brings the T-pose arm down and fold + then swings it forward.

**Widen the rig to a person's range before animating.** Measured on RifleMan ([WALK-AND-RUN.md](WALK-AND-RUN.md),
2026-10-07):

| Joint | A walk needs | A run needs | RifleMan allowed | Write in the rig |
|---|---|---|---|---|
| ankle, pointing the toes (plantarflexion) | −17° | −23° to −31° | −10° | `foot_r [..., range=(-50, 85)]` |
| hip, turning in and out | −13° to +6° | ±13° | locked | `thigh_r [..., twists=(-40, 40)]` |
| trunk counter-turn (over the spine) | ±13° | ±21° | locked | `spine_0N [..., twists=(-15, 15)]` each |
| hip extension (leg behind) | −16° | −29° | −24° in the IK rig | allow about −35° |

A range that is too short does not fail the build. It prints `WARNING joint limit: '<bone>' clamped by up to …
degrees` and the limb stops short. Read those lines.

**Joint ranges are the ceiling; the movement guides give the target.** Write the range from
[HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) into `range=` (from straight). Draw the walking, running and task
curves inside it. Scale timing and stride by [HUMAN-BODY-SIZE-AND-BALANCE.md](HUMAN-BODY-SIZE-AND-BALANCE.md) for a
character of another size.

**What GraphCode does not do yet** (stated where it affects these guides):

- **Balance is static.** `balance()` keeps the body's mass over the planted soles and moves the weight before a foot
  lifts ([MOVE-BALANCE.md](MOVE-BALANCE.md)); it does not model momentum, a push or a fall. Added motion does not
  balance itself: write the line. The dynamic balance numbers are a target you draw to and check yourself.
- **No gait is made for you.** You draw the legs (or map measured curves); the solver keeps the feet and moves the body.
  Rolling feet plant on flat floors only ([WALK-AND-RUN.md](WALK-AND-RUN.md), "What it does not do yet").
- **A swinging chain does not swing on its own yet.** `[dynamic=]` on a rig chain reaches no asset; key follow-through
  yourself ([ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md)).
- **Breathing and other natural motion are authored,** not generated ([ANIMATION.md](ANIMATION.md)).

## The guides

**The body and its gaits**

| Guide | What is in it |
|---|---|
| [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) | the range of motion of every joint, passive and active, by age and sex; how a spinal twist and an arm raise are shared along the body; a `joints { }` example |
| [HUMAN-WALKING.md](HUMAN-WALKING.md) | a normal walk: speed, cadence, stride, support timing, the eight phases and joint angles, foot roll, pelvis, trunk, arms and head |
| [HUMAN-RUNNING.md](HUMAN-RUNNING.md) | a normal run: stride, cadence, contact and flight by speed, joint peaks, foot strike, trunk lean, arm carriage, the switch from walking |
| [HUMAN-BODY-SIZE-AND-BALANCE.md](HUMAN-BODY-SIZE-AND-BALANCE.md) | scaling a gait to body size (Froude number, hip height), segment lengths and masses, the centre of mass, standing balance and the weight shift before a step |

**Everyday tasks**

| Guide | What is in it |
|---|---|
| [HUMAN-WEAPON-HANDLING.md](HUMAN-WEAPON-HANDLING.md) | rifle and pistol grip, support hand, stance, carry and ready positions, draw and reload |
| [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md) | grasp types, finger joint angles in a grip, wrist posture, grip force; the timing and path of a reach, reach envelope and trunk recruitment |
| [HUMAN-LIFT-CARRY-PUSH.md](HUMAN-LIFT-CARRY-PUSH.md) | squat, stoop and semi-squat lifts, order and timing, carries, pushing and pulling |
| [HUMAN-SIT-STAND-FLOOR.md](HUMAN-SIT-STAND-FLOOR.md) | sit to stand and stand to sit, squat, kneel, and getting up from the floor |
| [HUMAN-STAIRS-AND-LADDERS.md](HUMAN-STAIRS-AND-LADDERS.md) | stair dimensions, cadence, leg angles through a climb, descent landing, trunk lean, handrails, ladders |
| [HUMAN-JUMP-AND-LAND.md](HUMAN-JUMP-AND-LAND.md) | countermovement and squat jumps, take-off angles, landing angles and force |
| [HUMAN-TURNING.md](HUMAN-TURNING.md) | spin and step turns while walking, how often people turn, the order the body's segments follow |
| [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md) | the overarm throw and the punch: angles, timing and the order from legs to hand |
| [HUMAN-GAZE-AND-HEAD.md](HUMAN-GAZE-AND-HEAD.md) | eye-only range, the order and timing of a gaze shift, saccade and head speed, the head while walking, blinking |
| [HUMAN-DOORS-AND-DEVICES.md](HUMAN-DOORS-AND-DEVICES.md) | door hardware height and force, phone neck and head flexion, keyboard and monitor set-up |

## Sources

This page quotes no study of its own. The angle-to-control mapping and the rig-range gaps are from
[WALK-AND-RUN.md](WALK-AND-RUN.md) and [RIG.md](RIG.md), measured on the example character on 2026-10-07. The rule
that rig ranges are written from straight is from [RIG.md](RIG.md). The two gait angle conventions are as described by these clinical references, read 2026-10-07
(secondary):

- **AAPM&R** — Kim, Gallagher, Speckman. Biomechanics of Normal Gait. AAPM&R PM&R KnowledgeNow, updated 2025-03-20. https://now.aapmr.org/biomechanics-normal-gait/.
- **Godges (Kaiser)** — Godges J. Normal Gait Mechanics (Kaiser Permanente residency handout, after Rancho Los Amigos 1993 and Perry 1992). https://scal-pt-residencyfellowship.kaiserpermanente.org/wp-content/uploads/2008-Resources/foot-region-docs/03GaitMechanics&DeviationsSummary.pdf.

Every other study is listed, with its full reference and URL, in the "Sources" section of the guide that uses its
numbers.
