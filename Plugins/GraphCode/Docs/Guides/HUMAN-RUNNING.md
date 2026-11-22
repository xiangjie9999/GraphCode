[Human movement guides](HUMAN-MOVEMENT.md) · this page: what a normal run looks like, in numbers.

# Human movement: running, what a normal run looks like

This guide states what a healthy adult's run does: stride, cadence and how long each foot is down and how long the
body is in the air at each speed, the joint angles and when they peak, how the foot lands, how far the trunk leans and
how the arms are carried. Use it to decide what your run should look like and to check a clip against a person. **How
to build the clip** (feet that roll, a body the feet carry, a flight that keeps its speed) is in
[WALK-AND-RUN.md](WALK-AND-RUN.md); this guide does not repeat it. [HUMAN-WALKING.md](HUMAN-WALKING.md) is the same
for a walk, and [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) explains the angle conventions and evidence words used below.

## What a correct-looking run is

All numbers are in the tables further down, each with its source.

- **The body leaves the ground.** Above a walk's pace both feet are off the floor for part of every stride. At 3.96
  m/s one take has the right foot down 0–37.3 % of the stride and both feet up 37.3–49.9 % and 86.0–100 %. Flight
  time stays about 0.13 s at every speed (0.137 s at 4.1 m/s, 0.128 s at 7.7 m/s). Slow runners may have no flight
  (30 % of the runners at 2.36 m/s had none).
- **Going faster is a longer stride and a shorter contact.** Cadence is about 162 to 183 steps/min at 2.5 to 4.5 m/s
  and contact falls from 0.36 s (2.36 m/s) to 0.096 s (top speed of faster men).
- **The legs.** At 3.5 m/s the hip reaches 52.76° of flexion and the knee 108.68° in swing; the ankle points the toes
  20.47° (mean of individual peaks). A run's joint angles are larger than a walk's.
- **The landing.** Most runners land heel first (rearfoot strike 68 % of lab runners at 3.5 m/s).
- **The trunk and arms.** The trunk leans forward about 7° (7.3 ± 3.6°). The elbow stays bent: mean 91° in the one
  full-body take, and it swings through about 40°.
- **The bounce.** The centre of mass rises and falls about 10.8 cm per step at 10 km/h; the pelvis of the 3.96 m/s take
  moves 7.9 cm peak to peak.

## The six most common mistakes

1. **A sped-up walk.** A run's stance is shorter, its knee and ankle angles are larger and its arms are bent. Draw a
   run from run data ([WALK-AND-RUN.md](WALK-AND-RUN.md)). The switch from walking is a change of pattern, not a
   change of speed: the duty factor drops below 50 % and a flight phase appears.
2. **No flight, or contacts too long for the speed.** Contact times are short (0.158 s at 4.1 m/s). Build a run at 60
   fps (`fps:60` in the header), since its contacts are short ([WALK-AND-RUN.md](WALK-AND-RUN.md)).
3. **A forefoot strike on every runner.** Most runners land heel first at ordinary speeds. Drawing every run as a
   forefoot strike is a style choice, not the norm (derived).
4. **A fixed 90° elbow.** The common "elbow at 90°" is otherwise untraced. The one full-body take runs 71–130° with a
   mean of 91°, and a study of distance runners found a range of 38.8 ± 12.6° at 4.4 m/s (secondary).
5. **The wrong lean.** Runners at 3.4 m/s lean 7.3 ± 3.6° when they choose their own posture (14.1 ± 4.8° when told to
   lean, 4.0 ± 3.9° when told to stay upright); across 97 runners lean varied by about 20°.
6. **Mixing hip-extension sources.** Hip extension is −8° in one marker model and −29° in another, because the second
   measures the thigh against a pelvis that tilts 8.6°. Use one source consistently for one character, and give the rig
   a hip range that allows it (the example rig allowed −24° where a run needs −29°).

## 1. Stride, cadence and ground contact

| Speed | Stride length | Cadence | Contact / flight | Population | Source | Evidence |
|---|---|---|---|---|---|---|
| 2.36 ± 0.14 m/s | 1.75 ± 0.12 m | 1.34 ± 0.06 strides/s (stride time 0.75 ± 0.03 s) | contact 0.36 ± 0.03 s; duty factor 49.46 ± 3.63 %; 30 % of runners had **no** flight | 20–23 slow race runners (83 % women, age 45.3 ± 10.3 y, height 1.71 ± 0.09 m), events from high-speed video at 100–125 Hz | Bonnaerens 2021 | cited |
| 2.5 m/s | 1.86 ± 0.11 m | 80.82 ± 4.63 strides/min (≈162 steps/min, derived) | — | 28 runners ≥20 km/week, treadmill | Fukuchi 2017 | cited |
| 2.78 m/s | "stride length" 1.95 ± 0.14 m | "stride frequency" 2.87 ± 0.21 Hz (as printed; 1.95 m × 2.87 Hz is 5.6 m/s, so evidently steps per second) | contact 0.27 ± 0.03 s, flight 0.09 ± 0.03 s (duty ≈ 37 %, derived) | 15 recreational runners (6 M, 9 W), leg length 0.90 ± 0.04 m, 120 Hz video | Peyré-Tartaruga 2024 | cited |
| 3.33 m/s | — | — | duty factor 27.2–30.8 % (group means) | 89 runners (65 M, 24 W), motion capture 179 Hz | Patoz 2019 | cited |
| 3.5 m/s | 2.46 ± 0.15 m | 85.68 ± 5.27 strides/min (≈171 steps/min, derived) | — | 28 runners ≥20 km/week, treadmill | Fukuchi 2017 | cited |
| 3.96 m/s | one take: 0.700 s stride | — | right stance 0–37.3 %, flight 37.3–49.9 % and 86.0–100 % of the cycle | OpenSim full-body running model take | OpenSim running take (below) | measured |
| 4.1 m/s | step 1.28 m | 3.23 steps/s | contact 0.158 s, flight 0.137 s (duty ≈ 27 %, derived) | 40 adults (20 M, 20 W, about 22 y), mean ± s.e.m.; events picked by eye from heel and ball-of-foot marker heights | Clark 2020 | cited |
| 4.5 m/s | 2.96 ± 0.20 m | 91.74 ± 6.69 strides/min (≈183 steps/min, derived) | — | 28 runners ≥20 km/week, treadmill | Fukuchi 2017 | cited |
| 6.4 m/s | step 1.72 m | 3.70 steps/s | contact 0.125 s, flight 0.136 s | as Clark 2020 | Clark 2020 | cited |
| 7.7 m/s | step 1.93 m | 3.97 steps/s | contact 0.110 s, flight 0.128 s | as Clark 2020 | Clark 2020 | cited |
| Top speed, faster men 9.4 ± 0.3 m/s | step 2.31 m | 4.08 steps/s | contact 0.096 s (duty ≈ 21 %, derived) | as Clark 2020 | Clark 2020 | cited |
| Top speed, slower women 6.9 m/s | — | — | contact 0.127 s | as Clark 2020 | Clark 2020 | cited |

- **Duty factor** (contact time ÷ stride time) is how much of a stride one foot is down. Above 50 % there is no flight
  (Bonnaerens 2021). Measured values at 2.8–3.3 m/s run from about 28–31 % (motion capture) to about 37 % (video):
  the event-detection method changes the answer, so compare within one method. Fukuchi 2017 defines stance by vertical
  force above 20 N.
- **Steps per minute from strides per minute** is a doubling (derived): 80.82 strides/min × 2 ≈ 162 steps/min.
- **How runners go faster.** Up to about 7 m/s mainly by longer strides (pushing harder on the ground), above that
  mainly by a higher stride rate (swinging the legs faster) (Dorn 2012, 9 subjects running overground at 3.5, 5.2, 7.0
  and 9.0 m/s, abstract; the speeds are from the SimTK project page, secondary; the per-speed numbers were not
  reachable). Flight time stays about 0.13 s at every speed (Clark 2020). Between runners of different top speed
  (6.2–11.1 m/s) swing time does not differ (P = 0.18); the faster ones push harder, with a support force 1.26 times
  higher (Weyand 2000, 33 subjects, abstract; contact times are not in the abstract). This compares runners, while
  Dorn follows one runner speeding up, so the two do not contradict.
- **Bounce.** The centre of mass rises and falls 10.8 ± 2.9 cm per step at 10 km/h, estimated from contact and
  flight times with a spring-mass model, not markers (Peyré-Tartaruga 2024, cited). The pelvis of the 3.96 m/s take
  moves 7.9 cm peak to peak (measured). A higher step rate at the same speed lowers the bounce (45 recreational
  runners, treadmill at preferred speed, Heiderscheit 2011, abstract; the full text was blocked).

## 2. Joint angles in running

Peaks, 28-participant means, Cardan angles of the distal segment relative to the proximal one (Fukuchi 2017, cited):

| Angle | 2.5 m/s | 3.5 m/s | 4.5 m/s |
|---|---|---|---|
| Hip flexion (peak) | 43.75 ± 6.06° | 52.76 ± 5.75° | 60.50 ± 6.06° |
| Hip extension (peak) | −3.58 ± 4.85° | −7.95 ± 4.58° | −11.75 ± 4.78° |
| Knee flexion (peak, in swing) | 93.52 ± 10.36° | 108.68 ± 10.65° | 119.12 ± 10.37° |
| Ankle dorsiflexion (peak) | 26.36° | 26.54° | 26.79° |
| Ankle plantarflexion (peak) | −16.62° | −20.47° | −23.17° |

**When they happen**, % of the cycle from the same foot's strike. Measured from curves: the 3.5 m/s mean curve of the
Fukuchi dataset, and a 3.96 m/s full-body take of the OpenSim running model (Hamner model).

| Joint | 3.5 m/s mean curve (Fukuchi) | 3.96 m/s full-body take (Hamner model) |
|---|---|---|
| Hip | flexion 52.5° at 82 %; least flexion −7.2° at 39 % | flexion 45.2° at 83 %; extension −28.9° at 45 % |
| Knee in stance | 43.5° at 15 % | 53.0° at 16 % |
| Knee in swing | 108.2° at 68 % | 106.1° at 69 % |
| Ankle | dorsiflexion 22.8° at 20 % (mid-stance); plantarflexion −22.6° at 40 % (just after toe-off) | dorsiflexion 24.0° at 23 %; plantarflexion −23.0° at 45 % |
| Pelvis tilt / list / turn ranges | — | 8.6° / 10.8° / 4.5° |
| Trunk on pelvis | — | flexed 6.2° on average, up to 13.5°; twist range 36.7° |
| Elbow | — | 71–130° flexion, mean 91° |
| Shoulder flexion–extension range | — | 40.7° (about an arms-down frame) |

The mean curve's ankle peaks differ from the paper's mean-of-individual-peaks by 2–4°. Hip extension differs by model:
−8° in Fukuchi's marker model against −29° in the Hamner OpenSim model (the second measures the thigh against a pelvis
that tilts 8.6°). Use one source consistently for one character.

## 3. Foot strike

| Quantity | Value | Population | Source | Evidence |
|---|---|---|---|---|
| Foot strike, lab runners | rearfoot 68 / 68 / 61 %, midfoot 14 / 18 / 21 %, forefoot 18 / 14 / 18 % at 2.5 / 3.5 / 4.5 m/s; unaffected by speed | 28 runners, strike index from centre of pressure | Fukuchi 2017 | cited |
| Foot strike, elite half marathon (15 km) | rearfoot 74.9 %, midfoot 23.7 %, forefoot 1.4 %; top 50: 62.0 / 36.0 / 2.0 % | 283 elite runners | Hasegawa 2007 | abstract |
| Foot strike, road race (10 km) | rearfoot 88.9 %, midfoot 3.4 %, forefoot 1.8 %, different per foot 5.9 %; more heel strike by 32 km | 936 mostly recreational | Larson 2011 | abstract |
| Foot strike, marathon (8.1 km) | heel strike 93.67 % | 1,991 marathoners | Kasmer 2013 | abstract |
| Contact time by strike | rearfoot 200.0 ± 21.3 ms against mid/forefoot 183.0 ± 16 ms | as Hasegawa 2007 | Hasegawa 2007 | abstract |

So (derived): most runners land heel first at ordinary speeds. Drawing every run as a forefoot strike is a style choice,
not the norm.

## 4. Trunk and arms

| Quantity | Value | Population | Source | Evidence |
|---|---|---|---|---|
| Trunk forward lean | 7.3 ± 3.6° self-selected (14.1 ± 4.8° told to lean; 4.0 ± 3.9° told to stay upright) | 24 recreational runners, 3.4 m/s | Teng 2014 | abstract |
| Trunk lean spread | forward-lean group 10.8 ± 2.2°, upright group 3.6 ± 2.8°; across 97 runners lean varied by about 20° | Teng 2015 (n=40); Folland 2017 (n=97) | Teng 2015; Folland 2017 | abstract; cited |
| Elbow range while running | 38.8 ± 12.6° | 16 male distance runners, 4.4 m/s | Tartaruga 2012 via Wilk 2025 | secondary |
| Elbows bent in running, nearly straight in walking | direction only, no angle in the abstract | — | Yegian 2019 | abstract |
| Elbow in the 3.96 m/s take | 71–130° flexion, mean 91° | OpenSim running take | OpenSim running take | measured |

So (derived): a runner leans forward a few degrees (about 7°). The elbow stays bent through the stride: mean 91° in the
one measured take and a range of about 40°. The common "elbow at 90°" is otherwise untraced.

## 5. From walking to running

| Quantity | Value | Source | Evidence |
|---|---|---|---|
| Preferred walk–run transition speed | 1.99 ± 0.20 m/s forward (12 adults: 7 M, 5 W, 26.2 ± 4.1 y); earlier studies 1.89–2.16 m/s (Thorstensson and Roberthson 1987, Kram 1997, Minetti 1994 and others, as cited in Hreljac 2005) | Hreljac 2005 | cited |
| Same, walking backward | 1.58 ± 0.16 m/s | Hreljac 2005 | cited |
| Froude number at the transition | about 0.5, for humans and other bipeds; under reduced gravity the speed falls but the Froude number stays about 0.5 | Kram 1997 | abstract |
| Froude number at the gait change, other sources | "around 0.5" for birds, humans and humans in altered gravity (Usherwood 2005); about 0.35 across horse sizes at the walk-to-trot change (Griffin 2004) | both via a science.gov abstract listing | secondary |
| What changes | duty factor drops below 50 % and a flight phase appears (Bonnaerens 2021); stance mechanics differ at the joints, so a sped-up walk is not a run | Bonnaerens 2021 | cited |

One paper states that the transition "occurs at a Froude number > 1" (Dececchi 2020). That conflicts with the measured
0.5: 1 is the theoretical ceiling for walking, not the observed switch. The Froude number is explained in
[HUMAN-BODY-SIZE-AND-BALANCE.md](HUMAN-BODY-SIZE-AND-BALANCE.md).

## Putting the numbers into a clip

GraphCode does not invent a gait: you draw the legs (or map measured curves) and the solver keeps the feet honest and
moves the body. A run is a walk with an air phase: both support weights are 0 for part of the stride, and nothing
extra is written ([WALK-AND-RUN.md](WALK-AND-RUN.md), "A run: let the body fly").

| A number in this guide | Where it goes |
|---|---|
| Contact and flight timing (right stance 0–37.3 %, left stance after the 49.9 % flight to 86.0 %) | the two plants' `weight=` profiles; WALK-AND-RUN's run example comments them `0 to 0.373` and `0.499 to 0.860` |
| Stride time 0.700 s | a one-stride clip: 42 frames at 60 fps |
| The air phase | while no planted foot weighs, the root keeps the horizontal speed its feet gave it over the last 0.05 s before takeoff; the height in the air is your pelvis-rise curve, not gravity |
| Hip, knee, ankle, trunk and arm curves | `swing` with looping profiles; the anatomical-angle-to-control table is in [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) |
| Bounce (7.9 cm peak to peak in the take) | `shift(Root, axis=<up>, by=$Rise)`, a measured pelvis rise |

**Ranges a run needs.** On the example character a run needs −23° to −31° of ankle plantarflexion, ±13° of hip turn,
±21° of trunk counter-turn and about −29° of hip extension, against ankle −10°, locked hip turn and trunk, and −24° of
hip extension as built; WALK-AND-RUN says to allow about −35° at the hip ([WALK-AND-RUN.md](WALK-AND-RUN.md), measured
2026-10-07). Read the `WARNING joint limit` lines.

**What GraphCode does not do here.** The flight carries horizontal speed only; the vertical is your curve. The
full-body run is a first pass: pelvis tilt, list and turn, hip and trunk twist and side bend are not mapped in the
example's rig yet (the MetaHuman template, [Examples/MetaHumanBody.gcrig](Examples/MetaHumanBody.gcrig), declares the
pelvis and the trunk twist) ([WALK-AND-RUN.md](WALK-AND-RUN.md), "What it does not do yet").

## Not yet sourced

Do not plan with a number for these until someone reads the source:

- Novacheck 1998 stance and swing percentages and peak timing; Hamner and Delp 2013 (10 male runners at 2, 3, 4 and 5
  m/s per the SimTK project page, secondary) and Dorn 2012 per-speed stride tables; Heiderscheit 2011's bounce and
  stance-knee numbers (a search snippet gave 8.7 → 7.3 cm and 46.3 → 42.8°, unverified); Cavanagh and Kram on bounce; a
  measured mean running elbow angle beyond one take.
- Any whole-cycle toe curve for running stance (the first MTP at 3 m/s peaks at 44.7° shod and 52.6° barefoot; see
  [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md)).
- Foot-strike shares in a 50-km trail race and a 161-km ultra, and Hreljac 1993's preferred and energy-optimal
  transition speeds: their read status was not recorded, so they are left out.

## Sources

Read 2026-10-07; "read" says what was actually read. PMC links open the full text.

- **Bonnaerens 2021** — Bonnaerens S et al. 2021. Relationship between duty factor and external forces in slow recreational runners. *BMJ Open Sport Exerc Med* 7(1):e000996. PMC7931753, https://pmc.ncbi.nlm.nih.gov/articles/PMC7931753/. Read: full text.
- **Clark 2020** — Clark KP, Meng CR, Stearne DJ. 2020. 'Whip from the hip': thigh angular motion, ground contact mechanics, and running speed. *Biol Open* 9(10):bio053546. PMC7595692, https://pmc.ncbi.nlm.nih.gov/articles/PMC7595692/. Read: full text.
- **Dececchi 2020** — Dececchi TA et al. 2020. The fast and the frugal: divergent locomotory strategies drive limb lengthening in theropod dinosaurs. *PLoS One* 15(5):e0223698. PMC7220109, https://pmc.ncbi.nlm.nih.gov/articles/PMC7220109/.
- **Dorn 2012** — Dorn TW, Schache AG, Pandy MG. 2012. Muscular strategy shift in human running: dependence of running speed on hip and ankle muscle performance. *J Exp Biol* 215(11):1944–1956. Read: abstract.
- **Folland 2017** — Folland JP et al. 2017. Running technique is an important component of running economy and performance. *Med Sci Sports Exerc* 49(7):1412–1423. PMC5473370, https://pmc.ncbi.nlm.nih.gov/articles/PMC5473370/. Read: full text.
- **Fukuchi 2017** — Fukuchi RK, Fukuchi CA, Duarte M. 2017. A public dataset of running biomechanics and the effects of running speed on lower extremity kinematics and kinetics. *PeerJ* 5:e3298. https://doi.org/10.7717/peerj.3298; PMC5426356, https://pmc.ncbi.nlm.nih.gov/articles/PMC5426356/. Dataset (CC BY 4.0): https://doi.org/10.6084/m9.figshare.4543435. Read: full text. The timing rows are measured from the 28 original participants' 3.5 m/s mean curves.
- **Griffin 2004** — Griffin et al. 2004 (horses, walk to trot). Read: secondary, via a science.gov abstract listing; no URL recorded.
- **Hasegawa 2007** — Hasegawa H, Yamauchi T, Kraemer WJ. 2007. Foot strike patterns of runners at the 15-km point during an elite-level half marathon. *J Strength Cond Res* 21(3):888–893. Read: abstract.
- **Heiderscheit 2011** — Heiderscheit BC et al. 2011. Effects of step rate manipulation on joint mechanics during running. *Med Sci Sports Exerc* 43(2):296–302. Read: abstract.
- **Hreljac 2005** — Hreljac A et al. 2005. Preferred and energetically optimal transition speeds during backward human locomotion. *J Sports Sci Med* 4(4):446–454. https://jssm.org/jssm-04-446.xml-Fulltext. Read: full text (forward and backward conditions).
- **Kasmer 2013** — Kasmer ME et al. 2013. Foot-strike pattern and performance in a marathon. *Int J Sports Physiol Perform* 8(3):286–292. Read: abstract.
- **Kram 1997** — Kram R, Domingo A, Ferris DP. 1997. Effect of reduced gravity on the preferred walk-run transition speed. *J Exp Biol* 200(4):821–826. Read: abstract.
- **Larson 2011** — Larson P et al. 2011. Foot strike patterns of recreational and sub-elite runners in a long-distance road race. *J Sports Sci* 29(15):1665–1673. Read: abstract.
- **OpenSim running take** — OpenSim full-body running model (Hamner 2010 model), example labelled 3.96 m/s: https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53088554/Full%20Body%20Running%20Model; model https://github.com/opensim-org/opensim-models/blob/d9b05d470b1a481c222372c85b75772faf8f7792/Pipelines/Hamner/FullBodyModel_Hamner2010_v2_0.osim. A separate speed reference, not an observed 3.5 m/s performance. Peaks and timing measured from its curves 2026-10-07 (stride 0.700 s, right strike to right strike); the knee is exported with its flexion sign made positive.
- **Patoz 2019** — Patoz A et al. 2019. Duty factor is a viable measure to classify spontaneous running forms. *Sports* 7(11):233. PMC6915645, https://pmc.ncbi.nlm.nih.gov/articles/PMC6915645/. Read: full text.
- **Peyré-Tartaruga 2024** — Peyré-Tartaruga LA et al. 2024. Biomechanical, physiological and anthropometrical predictors of performance in recreational runners. *PeerJ* 12:e16940. PMC10903338, https://pmc.ncbi.nlm.nih.gov/articles/PMC10903338/. Read: full text (its printed "stride" frequency is evidently steps per second).
- **Teng 2014** — Teng HL, Powers CM. 2014. Sagittal plane trunk posture influences patellofemoral joint stress during running. *J Orthop Sports Phys Ther* 44(10):785–792. Read: abstract (overground, 3.4 m/s).
- **Teng 2015** — Teng HL, Powers CM. 2015. Influence of trunk posture on lower extremity energetics during running. *Med Sci Sports Exerc* 47(3):625–630. Read: abstract.
- **Usherwood 2005** — Usherwood 2005 (surname and year only recorded). Read: secondary, via a science.gov abstract listing; no URL recorded.
- **Weyand 2000** — Weyand PG et al. 2000. Faster top running speeds are achieved with greater ground forces not more rapid leg movements. *J Appl Physiol* 89(5):1991–1999. Read: abstract.
- **Wilk 2025** — Wilk C et al. 2025 (online 2024). Impact of elbow stiffness on running economy in trained athletes. *Shoulder Elbow* 17(6):825–830. PMC11656458, https://pmc.ncbi.nlm.nih.gov/articles/PMC11656458/. Read: full text (quotes Tartaruga et al. 2012).
- **Yegian 2019** — Yegian AK et al. 2019. Straight arm walking, bent arm running: gait-specific elbow angles. *J Exp Biol* 222:jeb197228. Read: abstract.
