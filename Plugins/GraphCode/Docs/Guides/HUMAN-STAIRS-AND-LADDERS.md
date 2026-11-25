# Human movement: stairs and ladders

[Human movement guides](HUMAN-MOVEMENT.md) · this page: how a person climbs and descends stairs and uses a ladder.

This guide says how stairs are built, how fast people climb them, what the leg does, where the foot lands, how the
trunk leans and how the handrail is used. It also gives the design rules for ladders. How to author a climb is
[STAIRS.md](STAIRS.md) (a baked step-up on typed tread heights) and [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md) (a flat
walk fitted to stairs at runtime). This guide supplies the human numbers to check those against, and says plainly what
neither guide does.

**What the numbers are.** Most gait rows come from journal abstracts, so the row says what was read. Labels:

- **C**: full text read.
- **Ca**: abstract only.
- **Cs**: through a named secondary page.
- **D**: derived here by arithmetic, which is shown.
- **M**: measured in this project from a cited public dataset.

Most rows name their staircase. Read a number against its stair. The staircases the cited gait studies used were:
18 cm riser / 28.5 cm tread (Protopapadaki 2007); 22 / 28 cm (McFadyen and Winter 1988, three men); 18 / 25.5 cm, seven
steps (Miyasike-daSilva 2016); riser conditions 85, 170 and 255 mm (Foster 2014, listing only). Copy a whole
consistent set from one row. Never mix values from different rows into one pose.

**This guide also uses two further research notes.** The stairs note defers the paired joint angles at foot contact and
the pelvis position to a note on stair climbing, and it points to a note on swing-toe clearance for the trailing leg.
Rows from them say "second note".

## What a correct-looking result is

- **Each step is a lift and a forward move at once**, not a lift and then a move. The knee extensors and the ankle's
  plantar flexors do the work in both climbing and descending.
- **The foot that lands on a tread lands forefoot first** in ascent. In descent, on low steps, about half the
  landings are forefoot first. On higher steps nearly all are.
- **The trailing leg folds its knee early in its swing,** before the hip carries the foot forward. That is how the toe
  clears the next riser's nose.
- **Up and down are different.** In the one lab that measured both, cadence was 75.8 steps/min going up and 117.4
  going down.
- **Hands use the rail some of the time,** more as people get older. The natural trunk lean is not in the sources; only
  the effect of a deliberate lean is (see "Trunk").
- **A ladder is climbed facing it, with a hand on it.** Those are rules. What a ladder climb looks like is not
  measured in the sources.

## The movement, in order

### One stair step, ascent

| What | Value | Population / stair | Source |
|---|---|---|---|
| Progression | each step is a "simultaneous lifting and horizontal translation of the body" with specific phases. Knee extensors with ankle plantar flexors do the work in both ascent and descent | 3 men, 22 / 28 cm stair | Ca, McFadyen and Winter 1988 |
| Phases, ascent | stance **50-60%**, swing **40-50%** | 15 women, three staircases of different dimensions | Ca, Livingston 1991 |
| Phases, descent | stance **19-64%**, swing **36-81%**, varying a lot with stair dimensions | same | Ca, Livingston 1991 |
| Scaling | cycle time, swing and stance durations, cadence and velocity were "systematically related to subject height" | same | Ca, Livingston 1991 |
| Contact part | forefoot first, either direction, any inclination | 10 people, 5-step stair (24°, 30°, 42°) | Ca, Riener 2002 |
| Inclination | temporal gait-cycle parameters and ground reactions "not significantly affected" by 24°, 30°, 42°. Joint angles and moments showed a low but significant dependency. Joint powers a large one | 10 people, 5-step stair | Ca, Riener 2002 |
| Against level walking | longer cycle, shorter share of stance | 11 healthy adults over 40 | Ca, Nadeau 2003 (no numbers in the abstract) |
| Trailing leg, swing | folds the knee strongly early in swing before the hip carries the foot forward | Camargo sample, as cited in the swing-toe clearance note | cited there, not re-measured |

### Cadence and cycle time

| What | Value | Population / stair | Source |
|---|---|---|---|
| Ascent cadence | **75.8 ± 5.8** steps/min without a phone, **65.6 ± 4.4** talking on one | 8 young adults, instrumented 7-step stair | Ca, Di Giulio 2020 |
| Descent cadence | **117.4 ± 4.2** steps/min without a phone, **108.6 ± 6.0** with one | same | Ca, Di Giulio 2020 |
| Body velocity, ascent | 0.534 (0.026) m/s without, 0.511 (0.024) m/s with the phone. Descent without 0.642 (0.026) m/s | same | Ca, Di Giulio 2020. The abstract prints the descent-with-phone speed as 0.511 (0.024), identical to the ascent figure. Probably a misprint, not used |
| With the phone | participants "moved slowly but more smoothly". Pelvis and foot jerk mostly fell. Foot clearance did not change | same | Ca, Di Giulio 2020 |
| Cadences other studies imposed (not preferred cadence) | 63, 88 and 116 steps/min. About 90 steps/min. 96 steps/min | 10 young men, 4 steps (Spanjaard 2009). 13 young men, 3 steps (Mian 2007). 20 women (Atkins 2019, 2022) | Ca, the cadence-listing fetch and the two Atkins abstracts |
| Descent cycle | **0.97 ± 0.18 s**. Self-selected speed **0.79 ± 0.16 m/s** | 101 unimpaired people 18-35, unconstrained 13-step staircase outside a lab | Ca, Sie 2022 |

*D:* seconds per step = 60 / (steps per minute). 75.8 steps/min is 0.79 s per step. 117.4 steps/min is 0.51 s per step.
At `fps:30` those are about 24 and 15 frames ([ANIMATION.md](ANIMATION.md#compose-movement-in-stages): frames = seconds x
fps).

## Angles and distances

### Stair geometry people actually build

| Code or study | Riser | Tread | Other | Source |
|---|---|---|---|---|
| US IRC 2021, R311.7.5 (residential) | max 7 3/4 in | min 10 in | riser and tread may differ by at most 3/8 in within a flight. Nosing not needed when tread is 11 in or more. Nosing projects 3/4 to 1 1/4 in, radius at most 9/16 in | Cs, Viewrail summary of IRC 2021 (the code site itself returned 403) |
| Michigan R 408.30519, amends R311.7.5.1/.2 | max 8-1/4 in (210 mm) | min 9 in (229 mm) | | C, Cornell LII text |
| US IBC 1011.5.2 as printed in the California Fire Code | 4 in (102 mm) min, 7 in (178 mm) max, measured between nosings | min 11 in (279 mm) | headroom 80 in (2032 mm), spiral 78 in (1981 mm), section 1011.3 | C, gocodebook CFC page |
| UK, private stair, quoted from Approved Document K and BS 5395 | 155-220 mm | 245-260 mm | max pitch 42°. 2 x rise + going between 550 and 700 mm | Cs, BWF page (the Approved Document PDF could not be read) |
| UK, institutional / assembly | 135-180 mm | 280-340 mm | same 2R+G rule | Cs, BWF |
| UK, other buildings | 150-190 mm | 250-320 mm | same 2R+G rule | Cs, BWF |
| ADA 504.2 (covered stairs) | 4 to 7 in | at least 11 in | a jurisdiction- and scope-specific requirement, not a universal domestic range | cited, US Access Board |

*D, pitch at the code limits, atan(riser/tread):* IRC 7.75/10 gives 37.8°. IBC 7/11 gives 32.5°. Michigan 8.25/9 gives
42.5°. Riener's three test inclinations were 24°, 30° and 42° (Ca). A search snippet from a flashcard site gave
different UK private-stair ranges (rise 150-220, going 220-300 mm); it was not opened and is not used.

Camargo's open dataset used stair heights **10.16, 12.7, 15.24 and 17.78 cm**. Bergmann et al. 2009 studied 14 healthy
participants on **15 cm** risers with **23 cm** tread length.

### Joint ranges over the whole cycle (sagittal)

| What | Value | Population / stair | Source |
|---|---|---|---|
| Maximum motion, hip / knee / ankle | **42° / 88° / 27°** (mean of the maxima; the abstract says "going up and down") | 10 men | Ca, Andriacchi 1980 |
| Knee flexion needed | **83° to 105°**, depending on the staircase. People adjusted by changing knee flexion/extension, not hip or ankle | 15 women, 3 staircases | Ca, Livingston 1991 |
| Descent ranges | hip flexion 30°, extension 8°. Knee flexion 85°, extension -11°. Ankle dorsiflexion 31°, plantarflexion 28° | 101 people, descent only | Ca, Sie 2022 |
| Ascent minus descent | hip larger in ascent by 28.10° (SD 4.08), knee by 3.39° (SD 7.20). Ankle dorsiflexion larger in descent by 9.90° (SD 3.80), plantarflexion by 8.78° (SD 4.80). These are mean differences, not peaks | 33 young (16 M, 17 F, 18-39), 18 / 28.5 cm | Ca, Protopapadaki 2007 |
| Repeatability | coefficient of variation 2.35-17.53% for joint angles | same | Ca, Protopapadaki 2007 |
| Weight acceptance on ascent | "the knee joint extended only 3 degrees" (quotation truncated in the fetch, stair size not given) | 33 healthy college women | Ca, Chleboun 2008 |

The knee figures 88°, 83-105° and 85° come from different stairs and people. They are listed, not blended.

### The leg at foot contact, ascent (paired, from primary data)

*M*: means in degrees from two retained subjects of Camargo's public dataset, at the dataset's labelled contact, for the
exact condition `stairascent`. `n` counts repeated contacts. There are only two retained subjects. The trailing leg is
read at the same timestamp, so each row is one simultaneous pose. Hip is `hip_flexion`. Knee flexion is minus the
`knee_angle` coordinate. Positive ankle is dorsiflexion. Zero is the model's static posture, **not** anatomical neutral
and **not** a GraphCode `fold=` value.

| Subject | Riser cm | n | Leading hip | Leading knee | Leading ankle | Trailing hip | Trailing knee | Trailing ankle |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| AB06 | 15.24 | 30 | 51.7 | 61.0 | 16.0 | 3.0 | 9.9 | 9.2 |
| AB06 | 17.78 | 28 | 54.9 | 64.6 | 17.2 | 4.0 | 9.1 | 8.1 |
| AB10 | 15.24 | 36 | 45.9 | 62.2 | 15.7 | -1.6 | 12.4 | 4.9 |
| AB10 | 17.78 | 30 | 47.8 | 67.3 | 17.9 | -0.6 | 13.4 | 4.7 |

- Across those rows, leading knee ranges **57.44-70.28° at 15.24 cm** and **59.54-71.40° at 17.78 cm**. Trailing knee
  ranges 0.51-18.39° / 0.43-19.20°. Trailing ankle -4.93-15.45° / -8.16-15.41°. These extrema are independent
  summaries, not one compatible six-angle pose. **Copy an actual paired row** when using a reference.
- Entry onto the flight (`walk-stairascent`) has a lower leading-knee minimum, **50.84° / 54.07°**. Mixing entry or
  top-of-flight contacts with steady climbing changes the answer.
- **The label is not exact first physical contact.** On one subject's force plate, the label came 5-70 ms later than
  the last 25 N upward crossing before 100 N. At the 25 N load-onset point that subject's mean leading hip / knee /
  ankle was **53.4 / 61.7 / 14.1°** on 15.24 cm (20 events) and **56.4 / 65.6 / 15.6°** on 17.78 cm (18 events).
  Trailing hip / knee / ankle then was 3.1 / 10.1 / 10.6° and 3.7 / 9.4 / 9.9°.
- The field is called HeelStrike. Riener et al. reports forefoot-first contact. Do not read the name as proof of a
  heel-first landing or a flat whole sole.
- **A 50° leading knee** is below the observed steady-ascent range, though close to the entry minimum. This is a
  two-subject analysis. It cannot say 50° is impossible for a person or for a long-thighed character.

**Where the body is over the leading foot (M, same data).** The model pelvis origin is approximately over the heel. The
hip midpoint is about **8-9 cm behind** it. Negative means behind the heel.

| Subject | Riser cm | n | Pelvis origin forward cm | Pelvis origin above cm | Hip midpoint forward cm | Hip midpoint above cm |
|---|---:|---:|---:|---:|---:|---:|
| AB06 | 15.24 | 30 | -0.11 | 77.94 | -8.00 | 71.67 |
| AB06 | 17.78 | 28 | -0.49 | 75.82 | -8.44 | 69.62 |
| AB10 | 15.24 | 33 | -2.05 | 74.55 | -8.88 | 68.25 |
| AB10 | 17.78 | 30 | -1.31 | 72.28 | -8.11 | 65.97 |

Heel is a marker on the foot, not the sole plane, exact physical contact, ankle centre or tread edge. A measured ASIS
midpoint instead sits **+2.51, +2.13, +1.53, +2.27 cm** forward of the heel, in table order. Naming the pelvis point can
reverse the sign. Individual hip-midpoint offsets span roughly **-18.8 to -2.1 cm**. A mean is not a universal landing
target.

**Compare with the authored fixture.** [STAIRS.md](STAIRS.md) lands the right foot with hip **39°**, knee **63°** and
ankle **24.5°** (its profiles at 0.4-0.45). The guide says its angles are "values chosen for this fixture, not
universal human gait measurements". The dataset's leading-leg values are hip 45.9-54.9°, knee 61.0-67.3°, ankle
15.7-17.9°. They are not directly comparable: the fixture's angles are `fold` changes on one rig, and the dataset's are
joint coordinates about a model's static posture. Both are shown. Do not average them, and do not copy dataset angles
into `by=`.

### Foot placement and landing

| What | Value | Source |
|---|---|---|
| Descent, foot overhang | mean **17.07 ± 6.66%** of foot length. Foot size explained 6% of heel placement variation but 79% of toe placement variation | Ca, Sie 2022 (101 people) |
| Descent, landing part | forefoot first: **50%** (10 of 20) at a 5 cm step, **90%** (18 of 20) at 15 cm, all 20 at 25 cm and at 35 cm | C, Wang 2025 (20 healthy young women, age 21.30 ± 0.98) |
| Ascent contact part | forefoot first, either direction, any inclination | Ca, Riener 2002 |

### Trunk

| What | Value | Source |
|---|---|---|
| Forward lean | a deliberately leaned trunk raised peak forward trunk lean by a mean **34.7°** over the self-selected posture (printed as -34.7°, 95% CI -39.1 to -30.3, the sign is the paper's convention). It shortened stance by 0.02 s and lowered peak hip adduction 2.8° and internal rotation 1.1°. The self-selected lean itself is not in the abstract | Ca, Atkins 2022 (20 women, 96 steps/min) |

### Handrail

| What | Value | Source |
|---|---|---|
| Height, US and IBC | **34 to 38 in (864-965 mm)** measured vertically from the sloped plane through the tread nosings (IRC R311.7.8.1, 2021) or from the line joining the nosings (IBC section 1014.2) | Cs, Engineering Express wiki quoting IRC. C, gocodebook CFC page (IBC) |
| Other rail rules in that page | continuous along the flight, graspable, 1 1/2 in minimum from the wall, extends 12 in horizontally at the top and one tread depth plus 12 in at the bottom. Alternating-tread devices and ship's ladders 30-34 in | C, gocodebook (CFC section 1014) |
| Guard | at least 42 in above the walking surface or nosing line. Where the guard top is also the handrail the 34-38 in rule applies | C, Alameda (CRC R312) |
| Preferred height, spiral stair | most preferred for descent **105 cm**. 95% of participants found **95-100 cm** satisfactory (both as printed) | Ca, Sinisammal 2010 |
| Use, young people | "approximately one-third of stair users" (Cohen and Cohen 2001). In a lab dual-task descent, 26% of trials (3 participants) in one condition, 34.7% (4 participants) in the other | Cs, via Miyasike-daSilva and McIlroy 2016 (n = 10); C |
| Use, with age | **23.1%** at 70 (103 of 446), **41.0%** at 78 (368 of 897), **86.7%** at 85 (903 of 1041). Asked at home | Ca, Stessman 2017 |
| Use, observed | in two houses with a depth camera (6 people): the two in their 70s held the rail about 80% of the time going up. The man in his 20s had the smallest grasp ratio | C, Miyazaki 2023 (tiny sample) |

### A ladder

These are design limits and rules, not measured body postures.

| What | Value | Source |
|---|---|---|
| Rung spacing | not less than **10 in (25 cm)** and not more than **14 in (36 cm)** apart | C, OSHA 1926.1053(a)(3)(i) and 1910.23(b)(2) |
| Clear width | at least **11.5 in (29 cm)** on portable ladders. "and 16 inches (41 cm)" for another type that the fetch did not name | C, OSHA 1910.23(b)(4) |
| Set-up angle | foot of a non-self-supporting ladder about one quarter of the working length from the top support. The 1910.23 figure shows the base width as x/4. *D:* a foot 1/4 of the length out stands at arccos(0.25) = **75.5°** from horizontal | C, OSHA 1926.1053(b)(5)(i), 1910.23 figure D-1 |
| Overlap | side rails extend at least **3 ft (0.9 m)** above the upper landing | C, OSHA 1926.1053(b)(1) |
| Facing | the user faces the ladder going up and down | C, OSHA 1926.1053(b)(20), 1910.23(b)(11) |
| Hands | at least one hand grasps the ladder while progressing | C, OSHA 1926.1053(b)(21), 1910.23(b)(12) |
| Carrying | nothing carried that could make the climber lose balance | C, OSHA 1926.1053(b)(22), 1910.23(b)(13) |

OSHA's "at least one hand" is a rule's minimum, not a description of how people climb.

## The most common mistakes

1. **A stair steeper than about 42°, or a riser over 7 3/4 in,** for an ordinary building. The limits above stop there
   (*derived*). Keep to the code table for the setting you are building.
2. **A descent onto 15 cm steps with a flat-footed landing on every one.** 90% of 20 women landed forefoot first at 15
   cm (*derived*).
3. **One cadence for up and down.** 75.8 against 117.4 steps/min in one lab (*derived*).
4. **A trailing leg swung forward before the knee folds.** Real people fold the knee early in swing. In the authored
   fixture, a trailing foot swung forward as it let go (hip 30°, knee 60° at the end) put its toe 7.3 cm into the next
   tread's nose ([STAIRS.md](STAIRS.md): "The trailing foot lifts before it swings"). That figure is a measurement of
   the fixture, not of a person.
5. **A ladder climber facing away from the rungs, or carrying a load that unbalances them** (*derived* from the OSHA
   rules).
6. **Landing angles read as `fold=` values.** The dataset's angles are about a model's static posture. Copying
   hip 51.7 / knee 61.0 / ankle 16.0 into `by=` does not put a rig in that pose.

## How to author it in GraphCode

[STAIRS.md](STAIRS.md) owns the mechanics of a baked climb. [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md) owns the runtime
walk on stairs. Use them for what they do, and use this guide for numbers to check the result against.

| You want | Where | What this guide adds |
|---|---|---|
| One step up, each foot landing flat on a tread | [STAIRS.md section 2](STAIRS.md#2-one-step-up): `Step = ground(z=18)`, `plant(leg_R, on=$Step, contact=Sole_R, drive=Root, weight=$OnStepR)` | The riser comes from the code table for your setting. The step takes about 0.5-0.8 s at the cadences above (*derived*) |
| A rolling foot on a tread | `plant(..., roll=(Heel_R, Ball_R, Toe_R))` on `ground()` planes ([WALK-AND-RUN.md](WALK-AND-RUN.md)) | STAIRS.md says its stair clips have not been rebuilt or measured with `roll=` |
| The next step, or a flight | [STAIRS.md sections 3 and 3b](STAIRS.md#3-the-next-step-the-next-clip-or-the-same-one) | Check each clip's last frame against the next riser |
| Toe clearing the riser's nose | The swing: hip held back, then knee folded, then ankle lifts the toes (STAIRS.md section 2) | Matches the human reference that the knee folds early in swing |
| The trunk lean | `swing(spine_0N, axis=fold, by=...)`. On a rig built like RifleMan a positive fold leans forward ([WALK-AND-RUN.md](WALK-AND-RUN.md#2-draw-the-legs-in-place)) | The natural lean is not in the sources. See "What is not known yet" |
| A walk on stairs in a game | [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md) | Use the cadence rows above to check the walk's step rate against the stairs' treads |
| A descent | Not authored in any guide | See below |

**What GraphCode does not do on stairs.** From [STAIRS.md](STAIRS.md) and [ANIMATION.md](ANIMATION.md#slopes-and-stairs-current-support):

- It does not find the tread. The plant projects the foot onto the height you type, wherever the swings put it.
- It does not know a riser's nose. A `ground()` is a plane, so a swinging toe can pass through a nose and nothing
  refuses it. Look at the side view over boxes at every frame of each swing.
- A baked clip does not adapt to a level's stairs at runtime.
- **Descent is not demonstrated.** The guides' stair examples are ascent. The descent rows above are for checking a
  descent you build yourself.
- Do not turn the missing planner into per-stair angle tables for a request that needs reusable adaptation.

**A handrail, and a ladder.** The guides show no recipe for either. The nearest tools are `reach(arm_R, to=$Point)` to a
point on a rail (`point(bone=..., at=(...))`, [POSES.md](POSES.md#hold-an-object-while-moving)) and `grab` on a surface.
Neither has been built for a rail or a rung here. A rung is not a plane, so `ground(z=...)` as a tread would hold a foot
at a height but says nothing about where on the rung it stands. Do not claim a ladder climb works until it has been
built and looked at.

**Where the eyes go on stairs** is in [HUMAN-GAZE-AND-HEAD.md](HUMAN-GAZE-AND-HEAD.md).

## What is not known yet

From the research notes' own gaps. Do not fill these with guesses.

- **Stairs:** the hand and forearm position on the rail, when the hand reaches for it, trunk angle in natural ascent
  (only the effect of a deliberate lean is given), arm swing, the swing leg's path (only that the knee folds early),
  and any ascent foot-placement fraction against the tread.
- **The paired contact angles come from two retained subjects** and explicit event definitions. Exact first physical
  contact, the excluded files' processing history, a population normal range and the mapping to a native rig's axes are
  unverified. 41 of 64 screened trials passed the provenance check; the rest are excluded for processing differences,
  not because those subjects moved wrongly.
- **Ladder: the "three points of contact" rule, the hand-foot sequence, climbing rhythm and the body's distance from
  the rungs are in none of the sources read.** The OSHA quick-card PDF was unreadable and the NIOSH page returned 404.
  There is no measured ladder-climbing posture here. Do not invent a hand-foot sequence.
- **Handrail height for a ladder-type device** is only the 30-34 in alternating-tread rule.
- **Not in GraphCode:** descent, ladders, a handrail, finding a tread or a nose, and a balance control.

## Sources

All read 2026-10-07 through web fetches, unless the entry says otherwise. Rows name author and year.

**Stairs: geometry and codes**

- IRC 2021 R311.7.5 summary (Viewrail): https://resources.viewrail.com/code-compliance/stair-code/tread-depth-and-risers
- Michigan R 408.30519 (Cornell LII): https://www.law.cornell.edu/regulations/michigan/Mich-Admin-Code-R-408.30519
- California Fire Code 1011 and 1014 (gocodebook): https://gocodebook.com/us/california/california-fire-code/means-of-egress/doors-stairs-ramps-and-guards/stairway-dimensions-headroom-and-handrails
- CRC handrail and guard heights (City of Alameda): https://www.alamedaca.gov/Departments/Planning-Building-and-Transportation/Permit-Center/FormsHandoutsVer2/Guardrails-Handrail-Requirements
- IRC R311.7.8.1 handrail (Engineering Express): https://engineeringexpress.com/wiki/20062012-irc-railing-guidelines
- UK rise and going (British Woodworking Federation): https://www.bwf.org.uk/question-of-the-week/what-are-the-current-recommendations-for-the-rise-and-going-of-stairs/
- US Access Board, ADA section 504.2: https://www.access-board.gov/ada/chapter/ch05/

**Stairs: gait studies**

- Riener, Rabuffetti, Frigo 2002 (PMID 11809579): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:11809579%20AND%20SRC:MED&resultType=core&format=json. Primary abstract: https://pubmed.ncbi.nlm.nih.gov/11809579/
- McFadyen and Winter 1988 (PMID 3182877): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=AUTH:%22McFadyen%20BJ%22%20AND%20TITLE:%22stair%20ascent%20and%20descent%22&resultType=core&format=json
- Protopapadaki 2007 and dissertation 2008: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=AUTH:%22Protopapadaki%20A%22%20AND%20TITLE:%22stair%22&resultType=core&format=json
- Nadeau 2003, Andriacchi 1980, Livingston 1991 listing: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=(TITLE:%22stairclimbing%20kinematics%22%20OR%20TITLE:%22lower-limb%20mechanics%20during%20stair-climbing%22%20OR%20TITLE:%22stair%20climbing%20task%20in%20healthy%20adults%20aged%20over%2040%22)&resultType=core&format=json
- Livingston, Stevenson, Olney 1991 (PMID 2059107): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:2059107%20AND%20SRC:MED&resultType=core&format=json
- Andriacchi 1980 (PMID 7391098): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:7391098%20AND%20SRC:MED&resultType=core&format=json
- Chleboun 2008 and Baldon 2013 listing: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=TITLE:%22stair%22%20AND%20ABSTRACT:%22knee%20flexion%22%20AND%20ABSTRACT:%22descent%22%20AND%20ABSTRACT:%22healthy%22&resultType=core&format=json&pageSize=25
- Cadence listing (Di Giulio 2020, Spanjaard 2009, Foster 2014, Mian 2007): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=TITLE:(stair*)%20AND%20(ABSTRACT:%22cadence%22%20OR%20ABSTRACT:%22steps%2Fmin%22%20OR%20ABSTRACT:%22velocity%22)%20AND%20ABSTRACT:%22ascent%22%20AND%20ABSTRACT:%22descent%22%20AND%20(ABSTRACT:%22young%22%20OR%20ABSTRACT:%22healthy%22)&resultType=core&format=json&pageSize=40
- Di Giulio 2020 (PMID 31987506): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:31987506%20AND%20SRC:MED&resultType=core&format=json
- Sie 2022 descent dataset (PMID 34933229): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:34933229%20AND%20SRC:MED&resultType=core&format=json
- Wang 2025 landing strategy (PMC11814434): https://www.ebi.ac.uk/europepmc/webservices/rest/PMC11814434/fullTextXML
- Atkins 2022 trunk lean (PMID 35961131): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:35961131%20AND%20SRC:MED&resultType=core&format=json
- Atkins 2019 trunk posture (PMID 31499406): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:31499406%20AND%20SRC:MED&resultType=core&format=json
- Handrail listing including Sinisammal 2010 and Stessman 2017 snippets: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=TITLE:(handrail%20AND%20stair*)&resultType=core&format=json&pageSize=30
- Sinisammal 2010 (PMID 20828488): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:20828488%20AND%20SRC:MED&resultType=core&format=json
- Stessman 2017 (PMID 28112776): https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:28112776%20AND%20SRC:MED&resultType=core&format=json
- Miyazaki 2023 handrail use (PMC9967829): https://www.ebi.ac.uk/europepmc/webservices/rest/PMC9967829/fullTextXML
- Miyasike-daSilva and McIlroy 2016 (PMC5071366): https://www.ebi.ac.uk/europepmc/webservices/rest/PMC5071366/fullTextXML

**Paired contact angles and pelvis position (second note), and the trailing swing (swing-toe clearance note)**

- Camargo et al., open biomechanics dataset (22 healthy participants, stair heights 10.16, 12.7, 15.24 and 17.78 cm): https://www.epic.gatech.edu/opensource-biomechanics-camargo-et-al/. Only subjects AB06 and AB10 passed the provenance screen. The angles and positions in this guide were computed from that dataset by an earlier research agent (marked M above), not published by Camargo et al.
- Bergmann et al. 2009 (14 healthy participants, 15 cm risers, 23 cm treads): https://pmc.ncbi.nlm.nih.gov/articles/PMC2684094/
- Riener et al. primary abstract, forefoot-first stair contact: https://pubmed.ncbi.nlm.nih.gov/11809579/

**Ladders**

- OSHA 1926.1053 ladders: https://www.osha.gov/laws-regs/regulations/standardnumber/1926/1926.1053
- OSHA 1910.23 ladders: https://www.osha.gov/laws-regs/regulations/standardnumber/1910/1910.23

Results that were fetched but gave no usable numbers, or errored (403, 404, 500, reCAPTCHA, unreadable PDF), were not used.
