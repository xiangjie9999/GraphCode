[Human movement guides](HUMAN-MOVEMENT.md) · this page: how far each joint of a healthy adult goes, and how to type it into a rig.

# Human movement: joint ranges, how far each joint really goes

This guide gives the range of motion of every joint of a healthy adult, in degrees, with the study each number comes
from. Use it to write the `range=`, `side=` and `twists=` of a `.gcrig`, and to judge whether a pose bends a joint
further than a person can. [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) explains how to read an angle, what the evidence
words in the tables mean, and how a number maps to a rig control. Read its "Reading an angle" first.

## What a correct-looking result is

- **Every joint stops where a person's stops.** An elbow or knee straightens to zero or a few degrees past it. A
  forearm turns 80° (pronation) and 87° (supination), active. A neck turns about 73–74° to each side.
- **Ordinary motion stays inside the ranges.** A range is a limit, not a target. Normal walking uses a fraction of
  most ranges, so a build note for a walk, a reach or a grip prints no `WARNING joint limit` lines.
- **A twist or a raise is shared along the body.** A spinal twist is mostly in the upper and middle back. An arm
  raise moves the shoulder blade and collarbone as well as the upper arm.
- **A limit that is too short is visible.** The build does not fail; the foot or hand stops short and the note says
  `WARNING joint limit: '<bone>' clamped by up to … degrees`. Read those lines.

## The six most common mistakes

1. **One range for every character.** Ranges fall with age and differ by sex; women were more mobile than men in
   nearly all joints at all ages, and children are more mobile than adults (CDC study, below). Pick the group your
   character belongs to.
2. **A passive range used as a ceiling for a character that moves itself.** Passive ranges are a few degrees
   larger than active ones (elbow flexion 147 passive against 143 active, same men). Active range is the ceiling for
   a character that moves itself. Passive range bounds contact, falls and ragdoll.
3. **A range too short for the motion.** On the example character the ankle allowed −10° of pointing the toes where
   a walk needs −17° and a run −23° to −31°, hip turning was locked where a walk needs −13° to +6°, and trunk twist
   was locked where a walk needs ±13° ([WALK-AND-RUN.md](WALK-AND-RUN.md), measured 2026-10-07). Widen the rig to a person's range before animating.
4. **Comparing the build note with the rig's numbers.** A rig writes limits from straight; a build note reports from
   the rest pose. A joint that rests bent shows a shifted range in the note ([RIG.md](RIG.md), "Reading what the
   build says about joints"). Check the rest bend before deciding a limit is too tight.
5. **Ignoring the next joint.** Two-joint muscles change a range. Hip flexion with the knee straight is 48.5–57.1°,
   against 120–134° with the knee bent. Ankle dorsiflexion with the knee straight is 4°, against 20° with the knee
   bent 75°. A knee turns on its long axis only when it is bent.
6. **Putting a whole twist or raise into one joint.** The lumbar spine twists only 1–2° per level, about 9° in all;
   a rig whose clavicle never moves puts a whole arm raise into the upper arm.

## Passive and active

**Passive** means an examiner moves the joint to its end. **Active** means the person moves it. Passive ranges are a
few degrees larger (elbow flexion 147 passive against 143 active in the same men, Zwerus 2019). An animated character
moves itself, so use the active range as its ceiling. Use the passive range for contact, falls and ragdoll.

Age and sex, in one place:

| What the studies found | Source | Evidence |
|---|---|---|
| Women were more mobile in nearly all joints at all ages; range fell with age; the values differ significantly from the commonly used normative values; children aged 2–19 are more mobile | Soucie 2011 | abstract |
| Boys aged 2–8 have about 10° less hip flexion than girls (131.1 against 140.8) | Soucie 2011 tables | derived |
| Neck range falls about 2.0° per decade in flexion and 3.3° in extension; men had 6.52° less extension than women | Zárate-Tejero 2023 | cited |
| Neck: Youdas found women larger in every direction except flexion (numbers unverified); Swinkels found no sex effect and a fall in range after 50. The sources disagree | Youdas 1992; Swinkels 2014 | abstract |
| Lumbar range falls with age: flexion 73 to 40, extension 29 to 6, side bend 28 to 14 (youngest to oldest) | Troke 2005 | abstract |
| Shoulder flexion 170.8 men / 164.7 women at 20–24 y, 127.8 / 124.1 at 85+; abduction 158.4 / 156.4 at 20–24 y, 118.9 / 119.5 at 85+; external rotation 62.3 / 73.8 at 20–24 y, 51.5 / 48.6 at 85+ | Gill 2020 | cited |
| Elbow: women have more range than men, and range falls with age | Zwerus 2019 | cited |
| Hip rotation: women rotate more inward, men more outward, in both sitting and prone; sources disagree on whether position matters | Han 2015; Kouyoumdjian 2012 | cited; abstract |
| Knee rotation: women rotate 10–20 % more (n=120) | Almquist 2013 | abstract |
| Foot side to side: total inversion plus eversion fell from 53.2° at 20 y to 44.3° at 60 y; women had more, mostly eversion (100 adults) | Ball 1996 | abstract |

## Typing a range into a rig

A `.gcrig` limit is a pair, minimum to maximum, with **zero straight**. `range=` is how far the joint folds, `side=`
how far it swings sideways, `twists=` how far it rolls. `closes=` names the direction the joint closes. The anatomical
angles in this guide are also from straight, so a table value goes into the pair as it is; only the sign depends on
which way your rig's joint closes.

On the example character (RifleMan, [WALK-AND-RUN.md](WALK-AND-RUN.md)) a thigh with `closes=forward` has hip
extension negative, a calf with `closes=back` has flexion positive and a hyperextension negative, and a foot with
`closes=forward` has pointing the toes negative. Check the sign on your own rig with a one-pose render first
([HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md), "Using the numbers in GraphCode").

```gcrig
joints {
  thigh_r  [closes=forward, range=(-18.1, 133.8)]   # hip: extension 18.1, flexion 133.8
  calf_r   [closes=back,    range=(-1.6, 141.9)]    # knee: 1.6 past straight, flexion 141.9
  foot_r   [closes=forward, range=(-62.1, 13.8)]    # ankle: pointing the toes 62.1, dorsiflexion 13.8
}
```

These are the CDC means for women aged 20–44 (Soucie 2011, passive; the table below). Each is one study and one
group, which keeps the set consistent. Two cautions:

- **They are passive.** A character that moves itself should stay a few degrees inside them.
- **The hip will clamp a run.** A gait's hip angle is the thigh against the pelvis, and a run reaches −28.9° of hip
  extension in one measured take against 18.1° here. [WALK-AND-RUN.md](WALK-AND-RUN.md) says to allow about −35°.
  A range is the joint's own end; the gait guides ([HUMAN-WALKING.md](HUMAN-WALKING.md),
  [HUMAN-RUNNING.md](HUMAN-RUNNING.md)) say what a gait asks of it.

The project's own finger limits, in rig spelling, are in [HANDS.md](HANDS.md).

**On an imported skeleton, straight is not always neutral.** A range counts from straight: the bone continuing the
line from its parent's joint. On a rig built from points that is the anatomical zero. On an imported skeleton such as
a MetaHuman it is only for the knee, elbow, wrist, fingers and thumb. The hip is a special case: its line in runs
sideways from the pelvis, almost along the axis the hip folds about. Where under 5 % of a line in lies across the
axis it says nothing about the fold, so the build takes the joint as it stands for straight and says so in its notes;
from 15 % the line is measured as on any rig, and between it fades in, so nearly equal rigs never get limits far apart.
On a MetaHuman the study's hip pair therefore goes in as it is: `thigh_l [closes=forward, range=(-35, 133.8)]`.

At the ankle, spine, neck and collarbone the standing rest is the anatomical zero, and it rests off straight: add the
rest bend to the pair. A MetaHuman's ankle rests 62.4° bent, so dorsiflexion 13.8 and plantarflexion 62.1 become
`range=(0.3, 76.2)`. The build reports a joint's rest bend when a range does not hold it, so a deliberately tiny range
is a way to ask. Two MetaHuman bodies rest within about 5° of each other at these joints and at the hip, so a
shared file was that close on both. Nothing checks it on a third: the build refuses only a rest bend outside the
declared range, and a body resting further off inside it moves both ends by that much. `side=` and `twists=` already count from rest. The worked file is
[Examples/MetaHumanBody.gcrig](Examples/MetaHumanBody.gcrig) ([CHARACTERS.md §7.4](CHARACTERS.md#74-animate-a-metahuman-body-with-gcmove-an-adapter-gcrig)).

## Hip, knee, ankle and foot

### The CDC study: passive ranges by age and sex

Soucie et al. 2011 measured 674 healthy people aged 2–69 (53.6 % female) with universal goniometers, passive, both
sides, nine physical therapists. Mean (95 % confidence interval), degrees. Method details from the CDC data
description: hip flexion supine with the knee allowed to bend near the end, so the hamstrings do not stop it; hip
extension side-lying with the knee straight; knee and elbow extension recorded as 0 if the joint passes straight,
with hyperextension recorded separately as a positive number; ankle with the knee bent at least 30°; shoulder
flexion supine with the shoulder blade held still (so mostly the ball-and-socket joint); forearm seated, elbow 90°,
thumb up = 0. Evidence: cited (abstract, tables and methods).

| Motion | Women 20–44 | Men 20–44 | Women 45–69 | Men 45–69 |
|---|---|---|---|---|
| Hip flexion | 133.8 (132.5–135.1) | 130.4 (129.0–131.8) | 130.8 (129.2–132.4) | 127.2 (125.7–128.7) |
| Hip extension | 18.1 (17.0–19.2) | 17.4 (16.3–18.5) | 16.7 (15.5–17.9) | 13.5 (12.5–14.5) |
| Knee flexion | 141.9 (140.9–142.9) | 137.7 (136.5–138.9) | 137.8 (136.5–139.1) | 132.9 (131.6–134.2) |
| Knee "extension" | 1.6 (1.1–2.1) | 1.0 (0.6–1.4) | 1.2 (0.7–1.7) | 0.5 (0.1–0.9) |
| Ankle dorsiflexion | 13.8 (12.9–14.7) | 12.7 (11.6–13.8) | 11.6 (10.6–12.6) | 11.9 (10.9–12.9) |
| Ankle plantarflexion | 62.1 (60.6–63.6) | 54.6 (53.2–56.0) | 56.5 (55.0–58.0) | 49.4 (47.7–51.1) |
| Shoulder flexion | 172.0 (170.9–173.1) | 168.8 (167.3–170.3) | 168.1 (166.7–169.5) | 164.0 (162.3–165.7) |
| Elbow flexion | 150.0 (149.1–150.9) | 144.6 (143.6–145.6) | 148.3 (147.3–149.3) | 143.5 (142.3–144.7) |
| Elbow "extension" | 4.7 (3.9–5.5) | 0.8 (0.1–1.5) | 3.6 (2.6–4.6) | −0.7 (−1.5–0.1) |
| Forearm pronation | 82.0 (81.0–83.0) | 76.9 (75.6–78.2) | 80.8 (79.7–81.9) | 77.7 (76.5–78.9) |
| Forearm supination | 90.6 (89.2–92.0) | 85.0 (83.8–86.2) | 87.2 (86.0–88.4) | 82.4 (80.9–83.9) |

The same study's two younger groups (the four groups are 2–8, 9–19, 20–44 and 45–69), same method, from the CDC tables
page:

| Motion | Female 2–8 | Female 9–19 | Male 2–8 | Male 9–19 |
|---|---|---|---|---|
| Hip flexion | 140.8 (139.2–142.4) | 134.9 (133.0–136.8) | 131.1 (129.4–132.8) | 135.2 (133.0–137.4) |
| Hip extension | 26.2 (23.9–28.5) | 20.5 (18.6–22.4) | 28.3 (27.2–29.4) | 18.2 (16.6–19.8) |
| Knee flexion | 152.6 (151.2–154.0) | 142.3 (140.8–143.8) | 147.8 (146.6–149.0) | 142.2 (140.4–144.0) |
| Knee "extension" | 5.4 (3.9–6.9) | 2.4 (1.5–3.3) | 1.6 (0.9–2.3) | 1.8 (0.9–2.7) |
| Ankle dorsiflexion | 24.8 (22.5–27.1) | 17.3 (15.6–19.0) | 22.8 (21.3–24.3) | 16.3 (14.9–17.7) |
| Ankle plantarflexion | 67.1 (64.8–69.4) | 57.3 (54.8–59.8) | 55.8 (54.4–57.2) | 52.8 (50.8–54.8) |
| Shoulder flexion | 178.6 (176.9–180.3) | 171.8 (169.8–173.8) | 177.8 (176.7–178.9) | 170.9 (169.1–172.7) |
| Elbow flexion | 152.9 (151.5–154.3) | 149.7 (148.5–150.9) | 151.4 (150.8–152.0) | 148.3 (146.8–149.8) |
| Elbow "extension" | 6.8 (5.2–8.4) | 6.4 (4.7–8.1) | 2.2 (0.9–3.5) | 5.3 (3.6–7.0) |
| Forearm pronation | 84.6 (82.8–86.4) | 81.2 (79.6–82.8) | 79.6 (78.8–80.4) | 79.8 (77.8–81.8) |
| Forearm supination | 93.7 (91.4–96.0) | 90.0 (88.0–92.0) | 86.4 (85.3–87.5) | 87.8 (85.7–89.9) |

The web table labels the rows "Knee extension" and "Elbow extension" with no sign note. Read with the methods, its
positive means look like degrees **past straight** and the one negative like a little short of straight. That reading
is an inference (derived). The study did not measure the wrist, hip abduction, adduction or rotation, foot
inversion or eversion, or the toes.

### Hip, knee, ankle and foot: three other studies side by side

Passive, men 30–40 (Roaas and Andersson 1982) against active, men 20–54 (Boone and Azen 1979), against the AAOS 1965
clinical table, degrees, mean ± SD.

| Motion | Roaas & Andersson 1982, passive, men 30–40 (right side) | Boone & Azen 1979, active, men 20–54 | AAOS 1965 | Other measured norms | Evidence |
|---|---|---|---|---|---|
| Hip flexion | 120.3 ± 8.3 | 121.3 ± 6.4 | 113 | CDC adults 127–134 (knee allowed to bend) | cited; B&A and AAOS secondary |
| Hip extension | 9.4 ± 5.3 (prone, knee bent) | 12.1 ± 5.4 | 28 | CDC adults 13.5–18.1 (side-lying, knee straight) | cited |
| Hip abduction | 38.8 ± 7.0 (supine, knee straight) | 40.5 ± 6.0 | 48 | — | cited |
| Hip adduction | 30.5 ± 7.3 | 25.6 ± 3.6 | 31 | — | cited |
| Hip internal rotation | 32.6 ± 8.2 (prone: hip straight, knee bent) | 44.4 ± 4.3 (sitting: hip bent) | 35 | sitting 42.1 ± 7.0 M / 48.2 ± 9.5 W; prone 45.6 ± 8.5 M / 53.5 ± 11.6 W (Han 2015) | cited |
| Hip external rotation | 33.6 ± 6.8 | 44.2 ± 4.8 | 48 | sitting 43.1 ± 9.1 M / 39.0 ± 7.6 W; prone 51.7 ± 10.9 M / 46.8 ± 10.0 W (Han 2015) | cited |
| Knee flexion | 143.8 ± 6.4 | 141 ± 5.3 | 134 | CDC 133–142 | cited |
| Knee hyperextension | 1.6 ± 2.8 (recorded −1.6; range 0 to 10) | — | 10 | CDC 0.5–1.6 | cited |
| Ankle dorsiflexion | 15.3 ± 5.8 (knee bent about 45°) | 12.2 ± 4.1 | 18 | CDC 11.6–13.8 | cited |
| Ankle plantarflexion | 39.7 ± 7.5 | 54.3 ± 5.9 | 48 | CDC 49–62 | cited |
| Foot eversion (valgus) | 27.6 ± 4.6 | 19.2 ± 4.9 | 18 forefoot + 5 hindfoot | — | cited |
| Foot inversion (varus) | 27.7 ± 6.9 | 36.2 ± 4.2 | 33 forefoot + 5 hindfoot | — | cited |

Populations and methods:

- **Roaas and Andersson 1982:** 210 hips, 180 knees and 192 ankles of a random sample of Göteborg men, one
  investigator, AAOS technique, nearest 5°, no left–right difference. The Boone and Azen and AAOS columns are that
  paper's own comparison columns. The authors note that the spread between individuals is large (total hip motion
  150–390°). Rounded means with individual ranges, from a search-result snippet of the same paper: flexion 120 (range
  90–150), extension 9.5 (0–35), abduction 38.5 (15–55), adduction 30.5 (15–45), internal rotation 32.5 (20–50). The
  table above was read from the PDF text and is the one to use.
- **Boone and Azen 1979:** 109 men, 18 months to 54 years, **active** range, groups 1–19 and 20–54. Abstract only; it
  gives no values, so the column is as Roaas and Andersson reprint it.
- **Han 2015:** 151 adults aged 21.8 ± 4.2 (117 men, 34 women), passive goniometer.
- **Kouyoumdjian 2012** (abstract; 120 adults 20–60, 71 women and 49 men, photographic, two observers): supine
  internal rotation 29.6 and external 38.5; prone internal 35.2 and external 41.8; no significant difference between
  positions; 4.7° less rotation in men. Seated with the hip flexed: total rotation 78.5 and internal rotation 37.9.
  The abstract prints external rotation as 78.5°, which equals the total and is an error in the abstract itself.
- **Thitilertdecha 1999** (abstract; the numbers are from a fetch tool's summary, not a verbatim quote): active
  range, 500 Thai adults aged 25–69, mean flexion 111.91, extension 18.01, abduction 38.16, adduction 24.88, medial
  rotation 34.73, lateral rotation 34.94.

### The next joint changes the range

| Joint and condition | Value | Population, method | Source | Evidence |
|---|---|---|---|---|
| Hip flexion, knee **straight** (passive straight-leg raise) | 48.5–57.1°; ankle plantarflexed 57.1 (SD 16.8) left, 56.7 right; ankle dorsiflexed 48.5 (SD 16.1) left, 48.9 right | 40 adults, inclinometer; a nerve tension test whose end point the abstract does not state | Boyd 2012 | abstract |
| Hip flexion, knee bent, for comparison | 120–134° | tables above | — | — |
| Ankle dorsiflexion, non-weight-bearing | 4° [1–8] knee straight against 20° [16–24] knee bent 75° | 20 adults 18–40 | Baumbach 2014 | abstract |
| Ankle dorsiflexion, weight-bearing lunge | 25° [22–28] against 39° [36–42]; bending the knee 20° removes the calf limit | same | Baumbach 2014 | abstract |
| Hip rotation, sitting (hip bent) against prone (hip straight) | larger prone in one study, no difference in another; the sources disagree | see the table above | Han 2015; Kouyoumdjian 2012 | cited; abstract |

**The knee turns on its long axis only when it is bent.** Total active rotation is 35°, 40° and 44° at 70°, 90° and
110° of knee flexion, with external rotation about twice internal (85 women, seated; Mossberg 1983, abstract). So at
90° it is about 27° outward and 13° inward (derived). Device-based passive values are higher, but the device
overestimates by about 100 % against bone tracking (Almquist 2011). Its values for total passive rotation at 90° of
knee flexion, 10 subjects: about 56 ± 5–9 at 6 Nm and 74–76 ± 7–10 at 9 Nm.

### The big toe (first MTP joint, extension)

| Condition | Value | Population, method | Source | Evidence |
|---|---|---|---|---|
| Active range | 44° mean | 33 adults, electromagnetic tracking of the hallux against the first metatarsal | Nawoczenski 1999 | abstract |
| Used in walking | 42° mean (heel-rise test 58°) | same | Nawoczenski 1999 | abstract |
| Needed for normal walking | about 65° | 20 adults 21–43, walking video | Hopson 1995 | abstract |
| Rancho clinical, pre-swing | 65° | handout after Perry | Godges (Kaiser) | secondary |
| Walking stance, relative to standing | 1.9 → 29.8°, peak in pre-swing | 30 adults | Sanchis-Sales 2024 | measured |
| Running at 3 m/s, maximum | 44.7 ± 7.9° shod, 52.6 ± 6.0° barefoot; flexion/extension range 27.68 ± 5.48 shod / 41.36 ± 6.77 barefoot | 15 men, dual fluoroscopy, zero = supine CT neutral | Zhang 2022 | cited |

The walking figures disagree (42° against 65°) because the reference segments and methods differ. The measured stance
curve is smaller again because it subtracts the standing angle. Nawoczenski 1999 also found the clinical tests
differed from each other by up to 21°.

## Neck and trunk

VA = US 38 CFR §4.71a rating normals (0 = anatomical position). DSHS = Washington State form 13-585A normals (no source
given). Both are clinical reference values, not measurements.

| Motion | Measured norm, mean ± SD | Clinical reference values | Evidence |
|---|---|---|---|
| Neck flexion | 39.9 ± 9.8 (women 41.6, men 38.3), Zárate-Tejero 2023; 55.7 ± 8.1, Wolan-Nieroda 2020 | VA 45; DSHS 50 | cited |
| Neck extension | 60.5 ± 10.3 (women 63.8, men 57.2), Zárate-Tejero 2023; 84.2 ± 11.1, Wolan-Nieroda 2020 | VA 45; DSHS 60 | cited |
| Neck side bend, each side | left 50.2 ± 7.0, right 47.0 ± 5.9, Wolan-Nieroda 2020 | VA 45; DSHS 45 | cited |
| Neck rotation, each side | left 73.0 ± 14.7, right 74.0 ± 9.9, Wolan-Nieroda 2020 | VA 80; DSHS 80 | cited |
| Trunk flexion | lumbar 73 falling to 40 from youngest to oldest, Troke 2005 | VA 90; DSHS 90 | abstract / cited |
| Trunk extension | lumbar 29 falling to 6, Troke 2005 | VA 30; DSHS 25 | abstract / cited |
| Trunk side bend, each side | lumbar 28 falling to 14, Troke 2005 | VA 30; DSHS 25 | abstract / cited |
| Trunk rotation, each side | lumbar median 7 at every age, Troke 2005; whole trunk 56.1 ± 7.5 passive, supine, pelvis held, Fujii 2007 | VA 30 | abstract / cited |

Populations and methods: **Zárate-Tejero 2023**, 122 adults 18–75 (61 men, 61 women), CROM device, seated.
**Wolan-Nieroda 2020**, 95 students 20–24 (31 men, 64 women), CROM, seated, active. **Troke 2005**, 405 people 16–90
(196 women, 209 men), lumbar only, CA6000 analyser, standing. The same Zárate-Tejero paper cites Chen 1999 for
inclinometer values of 61° flexion and 77° extension (ages 20–50), about −4° per decade, women greater (secondary).

### How a trunk twist is shared along the spine

| Level | Axial rotation | Population, method | Source | Evidence |
|---|---|---|---|---|
| T1 relative to L1, maximum to one side | 24.9 ± 4.9° | 13 adults, CT at neutral and maximum rotation | Fujimori 2012 | abstract |
| Each thoracic level at maximum | T1-2 1.2, T2-3 1.6, T3-4 1.4, T4-5 1.6, T5-6 1.8, T6-7 1.9, T7-8 2.3, T8-9 2.5, T9-10 2.7, T10-11 2.6, T11-12 1.3, T12-L1 0.5° | same | Fujimori 2012 | abstract |
| Each lumbar level at 45° trunk rotation | T12/L1 1.2 ± 0.4, L1/2 1.3 ± 0.5, L2/3 1.4 ± 0.3, L3/4 1.7 ± 0.6, L4/5 1.7 ± 0.6, L5/S1 1.6 ± 0.7° (sum ≈ 8.9°, derived) | 10 adults 23–31, 3D MRI, passive | Fujii 2007 | cited |
| Walking, by region | T7–L1 and L1–L3 each about 9° (see [HUMAN-WALKING.md](HUMAN-WALKING.md)) | 21 men, treadmill | Yamaguchi 2025 | cited |

So (derived): the lumbar spine twists only 1–2° per level, about 9° in total. The thoracic spine gives about 25° to
each side, most of it from T6 to T11. About three quarters of a spinal twist therefore belongs in the upper and
middle spine controls. White and Panjabi's per-level values run up to three times other literature at some thoracic
levels (Wilke 2025); do not take segment values from that one table. One figure disagrees: the discussion of Fujii
2007 says the thoracic region gives about 45° and the lumbar about 10°, against 24.9° for T1 relative to L1 to one
side in Fujimori 2012. The attribution of the 45° figure to Panjabi et al. 1989 was not checked.

## Shoulder, shoulder girdle, elbow, wrist, hand

### Shoulder

| Motion | Measured norm | Clinical reference values | Evidence |
|---|---|---|---|
| Shoulder flexion | 161.5 ± 18.7 men, 158.5 ± 19.1 women (right, active, standing); 170.8 M / 164.7 W at 20–24 y; 127.8 / 124.1 at 85+ (Gill 2020) | AAOS 180 (via Gill 2020); DSHS 150; CDC passive 172.0 W / 168.8 M | cited |
| Shoulder abduction | 151.5 ± 20.1 M, 149.7 ± 20.3 W; 158.4 / 156.4 at 20–24 y; 118.9 / 119.5 at 85+ (Gill 2020) | AAOS 180; DSHS 150 | cited |
| Shoulder external rotation | 56.8 ± 17.6 M, 60.9 ± 17.9 W; 62.3 / 73.8 at 20–24 y; 51.5 / 48.6 at 85+ (Gill 2020) | AAOS 90 | cited |
| Shoulder extension / adduction | — | DSHS 50 / 30 | cited |
| Shoulder internal rotation, horizontal ab/adduction | not verified | — | not used |

**Gill 2020:** 2,404 adults aged 20–91 (North West Adelaide Health Study); inclinometer for flexion and abduction,
external rotation estimated by eye to the nearest 5°. The authors conclude that the AAOS 180/180/90 is generally not
reached. Internal rotation, extension and adduction were not measured. Two other shoulder studies, abstract only, with
no numbers verified: Barnes 2001 (280 people aged 4–70; women exceed men in all motions and internal rotation
increases with age) and Günal 1996 (1000 men aged 18–22; the right side has less range than the left).

### Shoulder girdle during an arm raise

Ludewig et al. 2009 (cited): 12 adults (29.3 ± 6.8 y), bone-pin sensors, raising the arm to 120° relative to the
thorax, averaged over three planes.

| Quantity | Change over the raise |
|---|---|
| Clavicle retraction (swings back) | 23 → 39° (+16) |
| Clavicle elevation | 11 → 17° (+6) |
| Clavicle posterior axial rotation | +31° |
| Scapula upward rotation | 11 → 50° (+39) |
| Scapula internal rotation | 37 → 35° |
| Glenohumeral (ball-and-socket) elevation | +85° |
| Scapulohumeral rhythm, glenohumeral : scapular | 2.1:1 abduction, 2.4:1 flexion, 2.2:1 scapular plane (Inman 1944: 2:1, secondary) |

So (derived): a 120° arm raise is about 85° at the ball-and-socket plus about 40° of shoulder-blade rotation, with the
collarbone lifting about 6° and swinging back about 16°. A rig whose clavicle never moves puts all of it into the upper
arm.

### Elbow and forearm

Zwerus 2019 (cited): 352 adults 18–79 (47.2 % men), goniometer, dominant side. Negative extension means
hyperextension. Standard deviations were not captured.

| | Flexion | Extension | Pronation | Supination |
|---|---|---|---|---|
| Active, men | 143 | 0 | 78 | 85 |
| Active, women | 148 | −5 | 82 | 88 |
| Active, all | 146 | −2 | 80 | 87 |
| Passive, men | 147 | −2 | 82 | 89 |
| Passive, women | 153 | −8 | 86 | 93 |

Women have more range than men and range falls with age. Clinical reference: elbow 0–150, pronation 80, supination 80
(DSHS). Daily tasks mostly need 30–130° of flexion and 50° each of pronation and supination (Morrey 1981, 33 adults,
abstract).

### Wrist

No large active normative study was verified. Clinical reference (DSHS): flexion 60, extension 60, radial deviation
20, ulnar deviation 30. Functional use (Ryu 1991, 40 adults, biaxial electrogoniometer, abstract): all tasks together
reached 60 extension, 54 flexion, 40 ulnar and 17 radial deviation; most tasks fit in 40 flexion, 40 extension and 40
combined side-to-side. The functional 40° ulnar deviation exceeds the DSHS 30°.

### Fingers and thumb

| Joint | Measured norm | Clinical reference | Source | Evidence |
|---|---|---|---|---|
| Finger MCP | flexion 84.2–86.6; hyperextension 22.3–26.3 | VA 0–90 | Bhat 2024 (Indian sample) | abstract |
| Finger PIP | flexion 91.8–97.2; past straight 13.2–16.2 | VA 0–100 | Bhat 2024 | abstract |
| Finger DIP | flexion 81.6–84.6; past straight 6.0–8.4 | VA 0–70 or 80 | Bhat 2024 | abstract |
| Thumb IP | flexion 88 ± 2.3; extension 12 ± 9.2 (0–45) | DSHS 80 | Barakat 2013 | cited |
| Thumb MCP | flexion 60 ± 5.5; extension 8.1 ± 4.4 | DSHS 60 | Barakat 2013 | cited |
| Thumb CMC | palmar abduction 61.2 ± 4.4; radial abduction 62.9 ± 4.3; retroposition 31 ± 4; adduction 10.2 ± 4 | — | Barakat 2013 | cited |
| Functional postures | average MCP 61, PIP 60, DIP 39; thumb MCP 21, IP 18 (Hume 1990); task ranges MCP 19–71, PIP 23–87, DIP 10–64 = 48 / 59 / 60 % of active range (Bain 2015) | — | Hume 1990; Bain 2015 | abstract |

Bhat 2024: 390 hands, active, goniometer; its total active motion is 307.8–315.9. Barakat 2013: 100 adults 18–37 (62
women, 38 men); the method is poorly described.

## Ranges a gait needs

A joint's own range is not what a gait asks of the rig. [WALK-AND-RUN.md](WALK-AND-RUN.md) lists the ankle, hip and
trunk ranges a walk and a run need on the example character, and the `WARNING joint limit` lines that show a range
is too short. [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) repeats those figures under "Using the numbers in GraphCode".

## Not yet sourced

These are not verified, so do not plan with a number for them until someone reads the source:

- The AAOS 1965 table beyond the columns Roaas and Gill reproduce: shoulder internal rotation, extension and
  horizontal ab/adduction; wrist 80/70/20/30; thoracolumbar and hand values.
- Norkin and White; Kapandji; the numbers of Youdas 1992, Barnes 2001 and Günal 1996; a large active-wrist
  normative study; separate hindfoot (subtalar) inversion and eversion; knee tibial rotation measured by bone
  tracking.
- Leads found but not read for values: the 1000 Norms Project (PMC5200854, full text failed to load); Hallaçeli
  2014, Turkish population, n=987 (abstract only, no numbers).
- The passive non-weight-bearing first-MTP range in degrees, and any whole-cycle toe curve for walking swing or
  running stance.

## Sources

Every number above comes from the source named in its row. "Cited", "abstract", "secondary", "measured" and
"derived" are defined in [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md). PMC links open the full text. All read 2026-10-07;
"read" says what was actually read.

- **Almquist 2011** — Almquist PO et al. 2011. Measurements of knee rotation — reliability of an external device in vivo. *BMC Musculoskelet Disord* 12:291. PMC3264542, https://pmc.ncbi.nlm.nih.gov/articles/PMC3264542/. Read: full text. The overestimate against bone tracking is from Almquist 2002 (PMID 12038614, abstract).
- **Almquist 2013** — Almquist PO et al. 2013. Knee rotation in healthy individuals related to age and gender. *J Orthop Res* 31(1):23–28. Read: abstract.
- **Bain 2015** — Bain GI et al. 2015. The functional range of motion of the finger joints. *J Hand Surg Eur* 40(4):406–411. Read: abstract.
- **Ball 1996** — Ball P, Johnson GR. 1996. Technique for the measurement of hindfoot inversion and eversion and its use to study a normal population. *Clin Biomech* 11(3):165–169. Read: abstract.
- **Barakat 2013** — Barakat MJ, Field J, Taylor J. 2013. The range of movement of the thumb. *Hand* 8(2):179–182. PMC3653006, https://pmc.ncbi.nlm.nih.gov/articles/PMC3653006/. Read: full text.
- **Barnes 2001** — Barnes CJ et al. 2001. *J Shoulder Elbow Surg* (title and volume not recorded). Read: abstract only, numbers unverified.
- **Baumbach 2014** — Baumbach SF et al. 2014. The influence of knee position on ankle dorsiflexion — a biometric study. *BMC Musculoskelet Disord* 15:246. Read: abstract.
- **Bhat 2024** — B K MI et al. 2024. The normal active range of motion of the index, middle, ring, and little fingers in a sample of Indian population. *Indian J Plast Surg* 57(4):248–255. Read: abstract.
- **Boone and Azen 1979** — Boone & Azen 1979 (PMID 457719; title and journal not recorded). Read: abstract only, gives no values; the column here is as Roaas and Andersson 1982 reprint it.
- **Boyd 2012** — Boyd BS, Villa PS. 2012. Normal inter-limb differences during the straight leg raise neurodynamic test. *BMC Musculoskelet Disord* 13:245. Read: abstract.
- **DSHS** — Washington State DSHS form 13-585A, Range of Joint Motion Evaluation Chart (rev. 03/2014). https://www.dshs.wa.gov/sites/default/files/forms/pdf/13-585a.pdf. Read: the full form; it gives no source for its normals.
- **Fujii 2007** — Fujii R et al. 2007. Kinematics of the lumbar spine in trunk rotation: in vivo three-dimensional analysis using MRI. *Eur Spine J* 16(11):1867–1874. PMC2223353, https://pmc.ncbi.nlm.nih.gov/articles/PMC2223353/. Read: full text.
- **Fujimori 2012** — Fujimori T et al. 2012. Kinematics of the thoracic spine in trunk rotation: in vivo 3-dimensional analysis. *Spine* 37(21):E1318–1328. Read: abstract.
- **Gill 2020** — Gill TK et al. 2020. Shoulder range of movement in the general population: age and gender stratified normative data. *BMC Musculoskelet Disord* 21:676. PMC7549223, https://pmc.ncbi.nlm.nih.gov/articles/PMC7549223/. Read: full text.
- **Godges (Kaiser)** — Godges J. Normal Gait Mechanics (Kaiser Permanente residency handout, after Rancho Los Amigos 1993 and Perry 1992). https://scal-pt-residencyfellowship.kaiserpermanente.org/wp-content/uploads/2008-Resources/foot-region-docs/03GaitMechanics&DeviationsSummary.pdf. Read: secondary.
- **Günal 1996** — Günal et al. 1996 (title and journal not recorded). Read: abstract only, no numbers.
- **Han 2015** — Han H et al. 2015. Hip rotation range of motion in sitting and prone positions in healthy Japanese adults. *J Phys Ther Sci* 27(2):441–445. PMC4339156, https://pmc.ncbi.nlm.nih.gov/articles/PMC4339156/. Read: full text.
- **Hopson 1995** — Hopson MM, McPoil TG, Cornwall MW. 1995. Motion of the first metatarsophalangeal joint: reliability and validity of four measurement techniques. *J Am Podiatr Med Assoc* 85(4):198–204. Read: abstract.
- **Hume 1990** — Hume MC et al. 1990. Functional range of motion of the joints of the hand. *J Hand Surg Am* 15(2):240–243. Read: abstract.
- **Inman 1944** — Inman VT et al. 1944, as described in Kawashima I et al. 2026, *JSES Int* 10(3):101654, PMC13022622. Read: secondary.
- **Kouyoumdjian 2012** — Kouyoumdjian P et al. 2012. Clinical evaluation of hip joint rotation range of motion in adults. *Orthop Traumatol Surg Res* 98(1):17–23. Read: abstract.
- **Ludewig 2009** — Ludewig PM et al. 2009. Motion of the shoulder complex during multiplanar humeral elevation. *J Bone Joint Surg Am* 91(2):378–389. PMC2657311, https://pmc.ncbi.nlm.nih.gov/articles/PMC2657311/. Read: full text.
- **Morrey 1981** — Morrey BF, Askew LJ, Chao EY. 1981. A biomechanical study of normal functional elbow motion. *J Bone Joint Surg Am* 63(6):872–877. Read: abstract.
- **Mossberg 1983** — Mossberg KA, Smith LK. 1983. Axial rotation of the knee in women. *J Orthop Sports Phys Ther* 4(4):236–240. Read: abstract.
- **Nawoczenski 1999** — Nawoczenski DA, Baumhauer JF, Umberger BR. 1999. Relationship between clinical measurements and motion of the first metatarsophalangeal joint during gait. *J Bone Joint Surg Am* 81(3):370–376. Read: abstract.
- **Roaas and Andersson 1982** — Roaas A, Andersson GBJ. 1982. Normal range of motion of the hip, knee and ankle joints in male subjects, 30–40 years of age. *Acta Orthop Scand* 53(2):205–208. https://actaorthop.org/actao/article/download/28654/33535/82106. Read: full text (Table 1 includes the AAOS 1965 and Boone and Azen 1979 comparison columns).
- **Ryu 1991** — Ryu JY et al. 1991. Functional ranges of motion of the wrist joint. *J Hand Surg Am* 16(3):409–419. Read: abstract.
- **Sanchis-Sales 2024** — Sanchis-Sales et al. 2024, walking foot data (30 participants), https://www.nature.com/articles/s41597-024-04166-3; data CC BY 4.0, https://zenodo.org/records/10838067. Right-foot stance angles, dorsiflexion positive, relative to the standing posture; first-MTP mean 1.8927–29.8235°; phase 0–1 is stance only. Measured from the dataset's curves, 2026-10-07.
- **Soucie 2011** — Soucie JM et al. 2011. Range of motion measurements: reference values and a database for comparison studies. *Haemophilia* 17(3):500–507. doi:10.1111/j.1365-2516.2010.02399.x. Tables: https://archive.cdc.gov/www_cdc_gov/ncbddd/jointrom/index.html. Methods: CDC, "Public use dataset for normal joint range of motion — data description", https://stacks.cdc.gov/view/cdc/153156. Read: abstract; tables and methods in full.
- **Swinkels 2014** — Swinkels RA, Swinkels-Meewisse IE. 2014. Normal values for cervical range of motion. *Spine* 39(5):362–367. Read: abstract.
- **Thitilertdecha 1999** — Thitilertdecha 1999. *Siriraj Med J* 51(6):348–357 (title not recorded). https://he02.tci-thaijo.org/index.php/sirirajmedj/article/view/247002. Read: abstract only; numbers from a fetch tool's summary.
- **Troke 2005** — Troke M et al. 2005. A normative database of lumbar spine ranges of motion. *Man Ther* 10(3):198–206. Read: abstract.
- **VA** — US 38 CFR §4.71a, normal ranges for rating (§4.71 sets the anatomical position as 0). https://www.law.cornell.edu/cfr/text/38/4.71a. Read: full text.
- **Wilke 2025** — Wilke HJ et al. 2025. Range of motion and neutral zone of all human spinal motion segments (30 years of in vitro experiments). *JOR Spine* 8(1):e70052. PMC11881816, https://pmc.ncbi.nlm.nih.gov/articles/PMC11881816/. Read: full text.
- **Wolan-Nieroda 2020** — Wolan-Nieroda A et al. 2020. Assessment of interrater and intrarater reliability of cervical range of motion (CROM) goniometer. *Biomed Res Int* 2020:8908035. PMC7306852, https://pmc.ncbi.nlm.nih.gov/articles/PMC7306852/. Read: full text.
- **Yamaguchi 2025** — Yamaguchi T, Suzuki E, Katoh H. 2025. Relationship between arm swing angles and intervertebral spinal rotation angles during treadmill walking. *Cureus* 17(3):e81210. PMC12023011, https://pmc.ncbi.nlm.nih.gov/articles/PMC12023011/. Read: full text.
- **Youdas 1992** — Youdas JW et al. 1992. Normal range of motion of the cervical spine: an initial goniometric study. *Phys Ther* 72(11):770–780. Read: abstract.
- **Zárate-Tejero 2023** — Zárate-Tejero CA et al. 2023. Association between age, sex and cervical spine sagittal plane motion. *Life* 13(2):461. PMC9967952, https://pmc.ncbi.nlm.nih.gov/articles/PMC9967952/. Read: full text.
- **Zhang 2022** — Zhang F et al. 2022. Influence of shod and barefoot running on the in vivo kinematics of the first metatarsophalangeal joint. *Front Bioeng Biotechnol* 10:892760. Read: full text.
- **Zwerus 2019** — Zwerus EL et al. 2019 (online 2017). Normative values and affecting factors for the elbow range of motion. *Shoulder Elbow* 11(3):215–224. PMC6555111, https://pmc.ncbi.nlm.nih.gov/articles/PMC6555111/. Read: full text.
