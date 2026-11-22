[Human movement guides](HUMAN-MOVEMENT.md) · this page: how a person jumps and lands.

# Human movement: jumping and landing

This guide says what a body does in a standing jump: the dip, the push-off, the flight and the landing. It gives
timings, joint angles, and how hard a landing is. How to build a jump clip in GraphCode is split across
[ANIMATION.md](ANIMATION.md), [WALK-AND-RUN.md](WALK-AND-RUN.md) and [MOVE-PHYSICS.md](MOVE-PHYSICS.md). This guide
points into them and says what they leave out.

**What the numbers are.** Every number was on a page that was read. Each row says whether it came from an abstract, a
table or the results. *Cited* means it was on the page. *Derived* means arithmetic on cited rows, shown. CMJ is a
countermovement jump (a dip, then the jump). SJ is a squat jump (start from the bottom of the dip, no dip first).
BW is body weights: ground force divided by body weight. Most subjects are young athletes. Copy a whole consistent set
from one row. Never mix values from different studies into one pose.

## What a correct-looking result is

- **A jump starts with a dip.** The body drops, then pushes. The dip is slow compared with the push.
- **The push goes outward from the trunk.** The upper body speeds up first, then the thighs, then the shanks, then
  the feet.
- **The arms swing.** An arm swing raised jump height by about 8.6 cm in one study.
- **The body is in the air for a short time.** A height of 0.373 m is about 0.55 s of flight (see "Flight").
- **The landing is soft.** The knees bend a lot, and the foot arrives toe first, with the toes pointing down.
- **A stiff, straight-legged landing looks wrong.** It carried about 1.6 times the force of a soft one.

## The movement, in order

### Phases of a countermovement jump

The studies define their phases differently. Each row says how. Do not mix rows.

| Phase | Value | Population and convention | Source |
|---|---|---|---|
| Countermovement (the whole dip) | **0.701 s** self-selected (0.765 shallow, 0.950 deep) | 10 healthy men, 26 ± 8 y | cited (Table 1), McHugh et al. 2024 |
| Propulsive phase | **0.301 s** (0.269 shallow, 0.428 deep) | same | cited (Table 1), McHugh 2024 |
| Descending phase | **0.53 s** (knee-led) to **0.58 s** (hip-led) | 12 male students | cited (results), Seki et al. 2023 |
| Braking phase | **0.169 s** (F) / **0.156 s** (M) | 11 F + 11 M strength-matched soccer players, about 21 y, force-plate phases | cited (results), Thomas et al. 2022 |
| Propulsion | **0.264 s** (F) / **0.249 s** (M) | same | cited (results), Thomas 2022 |
| Time to take-off | **0.784 s** (F) / **0.788 s** (M) | same | cited (results), Thomas 2022 |
| Unweighting | about **0.35-0.38 s**: 0.784 - 0.169 - 0.264 = 0.351 (F); 0.788 - 0.156 - 0.249 = 0.383 (M) | Subtraction on the Thomas row above | *derived* (the note calls it its own subtraction, not a reported value) |
| Total movement time, unloaded CMJ | **0.45 ± 0.05 s** | 33 elite male volleyball players | cited (methods), Zhang et al. 2025 |
| Before take-off | vertical velocity peaks **0.03 s** before take-off and falls 6-7% by take-off. Peak power comes about **0.07 s** before take-off | 18 males, force plate | cited (abstract), Harman et al. 1990 |
| Push-off order | the velocity differences peak in the order **upper body, thighs, shanks, feet**. Muscle activation reaches its plateau in the order hamstrings, gluteus maximus, vasti, rectus femoris, soleus, gastrocnemius | 10 skilled jumpers. Order only, no ms | cited (abstract), Bobbert & van Ingen Schenau 1988 |
| Pre-take-off duration | the countermovement **lengthens the pre-take-off jump by 71-76%** | 18 males, all 4 combinations of arms and countermovement | cited (abstract), Harman 1990 |
| Unweighting force | force drops to **43% of body weight** (self-selected) against 58% (shallow). A deep countermovement makes the jump **38% longer** | 10 healthy men | cited (abstract, results), McHugh 2024 |

**Where the sources disagree.** The whole dip is 0.701 s (McHugh), the descending phase 0.53-0.58 s (Seki), the
time to take-off 0.784-0.788 s (Thomas) and the total movement time 0.45 s (Zhang). They define the start and end
differently, and the sources do not reconcile them. Pick one study and keep its set.

### Flight

None of the rows gives flight time. Constant gravity gives it. *Derived* here, not from the research: flight time =
2 x sqrt(2 x h / g), with g = 9.81 m/s^2. For the self-selected jump height of 0.373 m that is 2 x sqrt(2 x 0.373 /
9.81) = 0.55 s. [ANIMATION.md](ANIMATION.md#compose-movement-in-stages) says to take a jump's height curve from
physics (push, free flight at g, landing) rather than easing it by eye.

### The landing

| Value | Population and convention | Source |
|---|---|---|
| Landing told "toes then heels" after a CMJ: **toe peak 2.58 ± 0.85 BW at 15 ± 5 ms**, **heel peak 9.92 ± 3.02 BW at 45 ± 13 ms**, impact absorption phase **144 ± 33 ms** | 30 semi-pro football players. BW; ms after toe contact. The definition of the 144 ms phase was not checked. Knee angle was not controlled | cited (results), Rojano Ortega et al. 2010, *JSSM* |
| Heel-first landing: **first force peak 3.4 times higher**, second peak 1.4 times lower than forefoot-first | 10 male students, 0.4 m drop jumps. Ratio | cited (abstract), Kovács et al. 1999 |
| Moderate fatigue makes landings softer: peak knee about **+23%** and ground force down | 9 M + 9 F college athletes | cited (abstract, results), Zhang Q et al. 2021 |

At 60 frames a second one frame is 16.7 ms. The toe-to-heel peak gap of 30 ms is about 2 frames. Build a landing at 60 fps or
more ([WALK-AND-RUN.md](WALK-AND-RUN.md) says the same for a run's short contacts).

## Angles and distances

### The dip

| Value | Population and convention | Source |
|---|---|---|
| Self-selected countermovement **depth 0.364 m**. Shallow 0.269 m. Deep 0.467 m. Jump height **0.373 m** self-selected, 0.345 shallow, 0.378 deep | 10 healthy men, 26 ± 8 y. Centre-of-mass drop (m) | cited (Table 1), McHugh 2024 |
| Countermovement depth **0.32 m** self-selected, 0.43 deep, 0.23 shallow. Jump height **48 / 50 / 45 cm** | 29 male basketball and soccer athletes, 22.7 y. Centre-of-mass drop | cited (results), Sánchez-Sixto, Harrison & Floría 2018 |
| Bottom of an unloaded CMJ: **hip 85 ± 4°, knee 100 ± 3°, ankle 50 ± 2°**. Self-chosen depth falls in a **80-100° knee-flexion** half squat (citing Pérez-Castilla 2020) | 33 elite male volleyball players (Chinese national team). Convention not stated. The ankle value of 50° suggests an included angle | cited (methods; 0%-load results), Zhang et al. 2025. Use with caution |
| CMJ is **3.4 cm higher** than SJ from the same start posture. Cause given: muscle force built up before shortening. Stored elastic energy is ruled out | 6 male volleyball players. Jump height | cited (abstract), Bobbert, Gerritsen, Litjens & Van Soest 1996 |
| Arm swing adds **+0.086 m** to jump height | 20 males. Jump height | cited (abstract), Lees, Vanrenterghem & De Clercq 2004 |
| Arm swing raised jump height more than the countermovement did. With arms there is less unweighting and a slower, shallower centre-of-mass drop | 18 males. Qualitative | cited (abstract; no % given), Harman 1990 |

**Where the sources disagree.** Going deeper raised jump height in Sánchez-Sixto (48 to 50 cm). McHugh found no
difference between deep and self-selected.

### Take-off angles of a long jump

The angle is the centre-of-mass projection angle from horizontal. These are for jumps for distance, not a vertical
jump.

| Value | Population | Source |
|---|---|---|
| Standing long jump: optimum take-off angle **19-27°**, but people prefer **31-39°**. The distance lost by using the preferred angle is small | 5 active males | cited (abstract), Wakai & Linthorne 2005 |
| Running long jump: optimum **about 23°**, matching the athletes' competition angles | 3 experienced male long jumpers | cited (abstract), Linthorne (ISBS) |
| World Championships 2017 men's final: take-off angle **20.2-26.3°**. Board contact **0.120-0.130 s**. Second-last step about **+6%** longer, last step about **-9%** shorter | Finalists | cited (abstract), Tucker et al. 2018, IAAF biomechanical report |

**Where the sources disagree.** The standing-jump optimum is 19-27° but people choose 31-39°. For the running jump,
about 23° (Linthorne, n = 3) against 20.2-26.3° (the 2017 final).

### Landing angles and force

| Value | Population and convention | Source |
|---|---|---|
| Knee flexion: **soft landing 117°, stiff landing 77°**. A stiff landing gives higher ground force, and the ankle absorbs more energy. A soft landing loads the hip and knee more | 8 female athletes, **59 cm** drop. The abstract classifies soft and stiff by a 90° split and does not make the convention explicit (see below) | cited (abstract), Devita & Skelly 1992 |
| Peak vertical ground force **soft 1.63 ± 0.73 BW against stiff 2.60 ± 1.32**. Knee flexion from contact to peak: **55.4° soft against 36.8° stiff** | 16 adults (6 M, 10 F), **40 cm**. Body weights, degrees | cited (abstract, results), Myers et al. 2011 |
| No fatigue, at initial contact: **knee 14.0° (F) / 17.9° (M)**, **ankle plantarflexion 33.2° / 21.2°**, hip flexion 16.4° / 22.6°. **Peak knee 66.0° / 76.6°**. Peak ground force **2.82 / 2.40 BW** | 9 M + 9 F college athletes, **40 cm**, two-foot landing. 0° = anatomical standing posture | cited (Tables 1 and 3), Zhang Q et al. 2021 |
| Mean peak force landing from a 0.3 m box: **4.5 BW (SD 1.7)** | 234 adolescents, about 16 y | cited (abstract), McNair & Prapavessis 1999 |
| Knee extensors absorb energy at every height. The ankle contributes more in stiff landings, the hip more in soft ones. The strategy shifts **from ankle to hip** as height rises (0.32 / 0.62 / 1.03 m; contact speeds 2.5 / 3.5 / 4.5 m/s) | 9 males, step-offs | cited (abstract), Zhang SN, Bates & Dufek 2000 |
| Females land **more upright**. The knee is the main shock absorber in both sexes | 12 M + 9 F, 60 cm | cited (abstract), Decker et al. 2003 |

**Where the sources disagree.**

- **Devita 1992's convention.** The abstract labels soft and stiff as "less than / greater than 90°" yet reports soft
  117° and stiff 77° of knee flexion. Read 117° against 77° as peak flexion from a straight knee. The 90° split probably
  uses the included angle. Check the full text before relying on it.
- **Landing force varies a lot with the task:** 1.6-2.6 BW for 40 cm soft and stiff landings (Myers); 4.5 BW for a 0.3 m
  drop in adolescents (McNair); 9.9 BW for the heel peak of a CMJ landing told to go toe then heel (Rojano Ortega).

## The most common mistakes

All are *derived* from the rows above.

1. **A stiff-legged landing.** Peak knee flexion well under about 60-70° raises ground force by about 1.6 times (Myers:
   2.60 against 1.63 BW).
2. **A heel-first landing.** It raises the first impact peak by about 3.4 times (Kovács).
3. **A foot that arrives flat or with the toes up.** The foot should arrive plantarflexed by about 20-33° with the
   knee at about 14-18° (Zhang Q 2021).
4. **Every joint extending at once in the push-off.** The sources give an outward order: upper body, thighs, shanks,
   feet.
5. **No dip, or a dip far outside the measured range.** Measured dips are 0.23-0.47 m of centre-of-mass drop. The
   descending phase or countermovement is 0.53-0.95 s long, depending on the study and the instruction.
6. **A landing that ignores the drop height.** The strategy moves from ankle to hip as the drop grows (0.32 to 1.03 m),
   and force changes a lot with the task.

## How to author it in GraphCode

There is no jump verb. POSES.md says there is no built-in crouch activity. These pieces exist.

| You want | Use | Source guide |
|---|---|---|
| Feet that stay put while the hips drop, then leave and land | `plant(leg_R, on=$Floor, contact=Sole_R, weight=...)`. Weight 1 on the floor, 0 in the air. Replant at landing by raising the weight | [ANIMATION.md](ANIMATION.md#compose-movement-in-stages): "A crouch, a jump or a landing keeps the feet where they stand." [WALK-AND-RUN.md](WALK-AND-RUN.md#4-a-run-let-the-body-fly) for weights that go to 0 |
| A deep dip that needs more ankle than the rig has | `shift(Root, ...)` to move the hips back, and lean the trunk forward over them | ANIMATION.md: do not push the ankle past its range |
| The height in the air | `shift(Root, axis=<up>, by=$Rise)` with your own curve: "a measured pelvis rise, or a ballistic arc for a jump" | WALK-AND-RUN.md: the vertical comes from your curve, not from gravity |
| A body that keeps its speed in the air | `plant(..., drive=Root)` | WALK-AND-RUN.md: while no planted foot weighs, the root keeps the horizontal speed it had. Horizontal only. A standing jump has almost none |
| A landing that lands where the body has been carried | `plant(..., drive=Root)` and `blend=` | WALK-AND-RUN.md: a landing foot lands where the body has carried it. `blend=` fades a jump of the root |
| The arm swing, the push-off order | `swing` on each segment with its own `profile`, started in the order above | Joint mapping: [WALK-AND-RUN.md](WALK-AND-RUN.md#2-draw-the-legs-in-place) |
| A passive fall or floor contact | `clip(gravity=true, collision=$Floor)`, `contact(...)` | [MOVE-PHYSICS.md](MOVE-PHYSICS.md): there are no built-in human strength values, so physics will not make a person push off |

The knee, hip and ankle map like this: hip flexion is `thigh` fold, knee flexion is `calf` fold, ankle dorsiflexion
is `foot` fold with pointing the toes negative (WALK-AND-RUN.md). A landing needs the ankle to point the toes about
20-33° at contact. The guide's rig example allows only -10° on RifleMan and writes `foot_r [..., range=(-50, 85)]` to
fix that for a gait. Check your rig's `WARNING joint limit` lines.

**Check each pose with the character's own checks.** CHARACTERS.md section 9.2 records a standing jump on a
MetaHuman: the audit refused the thighs meeting as soon as the hips flexed (a hip-width stance and knees tracking out
over the toes fixed it) and the hands meeting overhead at a 100-degree arm swing (75 fixed it). Fix the source motion,
not the check. Check simulated cloth and hair in a jump and a landing, which are the fastest motions they will wear
([GARMENT.md](GARMENT.md#check-simulated-cloth-in-the-fastest-motion-it-will-wear)).

**Watch the frame rate.** The push-off events are tens of milliseconds long. `fps:60` gives 16.7 ms per frame. The
body-collision audit is "not continuous collision detection" and fast motion needs an appropriate fps
([ANIMATION.md](ANIMATION.md#body-collision-coverage)).

**Judge it by eye.** `balance()` keeps the weight over standing feet ([MOVE-BALANCE.md](MOVE-BALANCE.md)), but nothing
times a body through the air or catches a landing's momentum: a person landing from a high drop leans and reaches, and
nothing in the guides authors that for you.

## What is not known yet

The note lists these as not confirmed. Do not plan with a number for them.

- **CMJ bottom and take-off joint angles.** Some knee, hip and ankle angles appeared only in a search summary. The best
  candidate source (McErlain-Naylor, King & Pain 2014, *J Sports Sci*) returned 403 or could not be read. They are not
  reproduced here.
- **Arm swing as a percentage of jump height.** Neither the Harman 1990 nor the Lees 2004 abstract gives a percentage.
- **Joint timings in the push-off** (hip, knee and ankle peaks in ms before toe-off). The Bobbert 1988 abstract gives
  the order only.
- **Time to peak knee flexion in landing** and **forefoot against flat-foot landing knee angle at contact** came from
  search snippets only. A PDF at par.nsf.gov could not be read.
- **Arm counterbalance when landing:** nothing was found.
- **Not in GraphCode:** no jump or landing recipe, no balance through flight or landing (`balance()` is for standing feet), no flight under gravity from a verb that gives
  human push-off.

## Sources

Europe PMC forms: search = `https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=<q>&resultType=core&format=json`;
full text = `https://www.ebi.ac.uk/europepmc/webservices/rest/<PMCID>/fullTextXML`. Entries marked "candidate" were named but
not read.

- Bobbert, Gerritsen, Litjens & Van Soest 1996, why is countermovement jump height greater than squat jump height: https://research.vu.nl/en/publications/why-is-countermovement-jump-height-greater-than-squat-jump-height/
- Harman et al. 1990, effects of arms and countermovement on vertical jumping: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:2287261&resultType=core&format=json
- Lees, Vanrenterghem & De Clercq 2004, how an arm swing enhances performance in the vertical jump: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=TITLE:%22arm%20swing%20enhances%20performance%20in%20the%20vertical%20jump%22&resultType=core&format=json
- McHugh, Cohen, Orishimo & Kremenic 2024, effect of countermovement depth on neuromechanics: https://pmc.ncbi.nlm.nih.gov/articles/PMC11208864
- Sánchez-Sixto, Harrison & Floría 2018, larger countermovement increases CMJ height: https://pmc.ncbi.nlm.nih.gov/articles/PMC6316300
- Thomas, Jones & Dos'Santos 2022, CMJ force-time in strength-matched male and female soccer players: https://www.ebi.ac.uk/europepmc/webservices/rest/PMC8949185/fullTextXML
- Seki et al. 2023, SJ and CMJ with knee- or hip-dominant strategies: https://pmc.ncbi.nlm.nih.gov/articles/PMC10170540
- Zhang et al. 2025, joint contributions during loaded CMJs: https://www.ebi.ac.uk/europepmc/webservices/rest/PMC12126539/fullTextXML
- Bobbert & van Ingen Schenau 1988, coordination in vertical jumping: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=TITLE:%22coordination%20in%20vertical%20jumping%22%20AND%20AUTH:%22Bobbert%22&resultType=core&format=json
- Wakai & Linthorne 2005, optimum take-off angle in the standing long jump: https://bura.brunel.ac.uk/handle/2438/537
- Linthorne (ISBS), the optimum takeoff angle in the long jump: https://ojs.ub.uni-konstanz.de/cpa/article/view/653
- Tucker et al. 2018, IAAF biomechanical report: https://lida.sport-iat.de/dlv/Record/4049026
- Devita & Skelly 1992, effect of landing stiffness: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:1548984%20AND%20SRC:MED&resultType=core&format=json
- Myers et al. 2011, tibiofemoral kinematics during soft and stiff drop landings: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:21602566%20AND%20SRC:MED&resultType=core&format=json
- Zhang Q et al. 2021, progression of fatigue in drop landing: https://www.ebi.ac.uk/europepmc/webservices/rest/PMC7877277/fullTextXML
- Kovács et al. 1999, foot placement modifies kinematics and kinetics during drop jumping: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=TITLE:%22foot%20placement%20modifies%20kinematics%20and%20kinetics%20during%20drop%20jumping%22&resultType=core&format=json
- Rojano Ortega et al. 2010, *JSSM*: https://jssm.org/jssm-09-282.xml-Fulltext
- McNair & Prapavessis 1999: https://sponet.de/sponet/Record/4004409?lng=en
- Zhang SN, Bates & Dufek 2000: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:10776901%20AND%20SRC:MED&resultType=core&format=json
- Decker et al. 2003: https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=EXT_ID:12880714%20AND%20SRC:MED&resultType=core&format=json
- *Candidates, not read:* McErlain-Naylor, King & Pain 2014, *J Sports Sci*; par.nsf.gov/servlets/purl/10498700 (could not be read).
