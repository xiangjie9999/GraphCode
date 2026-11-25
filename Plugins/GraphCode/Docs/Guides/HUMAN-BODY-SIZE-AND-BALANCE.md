# Human movement: body size and balance

[Human movement guides](HUMAN-MOVEMENT.md) · this page: scaling a gait to body size, where the mass is, and how a person stands.

This guide answers three questions for a human character. How do speed, stride and timing change for a body of another
size? Where are a person's segment lengths, segment masses and centre of mass? Where does the weight sit when a person
stands, and what happens just before the first step? Use it to scale a measured human clip to your character, to
proportion a body, and to make a standing idle or a start of a walk look like a person's.
[HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) explains the angle conventions and evidence words used below.

**GraphCode has no balance or centre-of-mass verb today.** Nothing in `.gcmove` reads a body mass or computes a centre
of mass. `balance` is listed with `release`, `carry` and automatic contact selection as a proposal
([ANIMATION.md](ANIMATION.md)), and added motion does not establish automatic balance. `plant(..., drive=Root)` moves
the body from the planted feet, not from its mass. So the balance numbers here are a target you draw to and check by
hand, not something the tool enforces. "Using it in GraphCode" at the end says what you can do.

## What a correct-looking result is

- **A character of another size moves at its own size.** Joint angles and phase percentages stay the same. Speed,
  stride and cycle time change with hip height. A taller character takes longer strides and a slower cycle; a giant
  moving at human cadence reads as small.
- **The body is in proportion.** The hip joint is at about 0.530 of standing height, the knee at 0.285, the shoulder at
  0.818 (one standard table; measure your character's own mesh when you can).
- **The mass is where a person's is.** The trunk is 42.57 % (women) and 43.46 % (men) of body mass, one thigh 14.78 % and
  14.16 %, the head and neck 6.68 % and 6.94 % (de Leva). A whole-body centre of mass in quiet standing sits at about 56.6 % of height.
- **A standing idle barely moves.** The weight sits a little behind mid-foot (about 45 % of foot length from the heel),
  never on the heels, and sways by millimetres over seconds. On one foot it moves to mid-foot (50 %).
- **A first step starts with a weight shift.** The pressure moves back and toward the swing leg, which pushes the body
  forward and over the standing leg. Only then does the swing heel lift.

## The six most common mistakes

1. **Scaling stride by a body-height ratio.** Scale length by hip height. For heights 1.70–1.80 m a stride law
   for animals predicts a 1.30–1.35 m stride at 1.36 m/s against a measured 1.47 m, so scaling a measured human clip
   by the rules below is better than that law for a human.
2. **Scaling speed in proportion to size.** Only length scales with the hip-height ratio itself. Speed and cycle
   time scale with its square root, and cadence divides by it (see the rules). A 3.0 m giant's stride time is 1.33
   times a human's.
3. **Using two segment tables at once.** Two standard models disagree by up to 40 % on some segments (thigh, trunk).
   Pick one table and keep it for a whole character.
4. **Mixing two forms of the Froude number.** Hof's size-free speed `v / √(g·l)` is the **square root** of the Froude
   number `v² / (g·h)`. Check which form a paper uses before reusing its number.
5. **A rigid or swaying idle.** The weight sits about 45 % of foot length from the heel and sway is a few
   millimetres per second. Moving the hips centimetres in an idle is not a person's sway (derived).
6. **Lifting the stepping foot first.** Before a first step the body shifts its pressure back and onto the leg that will
   swing, so the hips move forward and over the standing leg; the stepping foot unloads and lifts only after that.

## 1. Scaling speed, stride and timing to leg length

Two bodies of different size move alike when they move at the same **Froude number**, `Fr = v² / (g·h)`: `v` is speed,
`g` is 9.81 m/s², and `h` is leg length taken as **hip height**, "the perpendicular distance from greater trochanter to
the ground" (Raichlen 2013, cited; Raichlen takes g = 9.8 m/s²). Alexander and Jayes 1983 give `Fr = u²/gh` with h the
hip-joint height in standing (search snippet only, so treat it as unconfirmed). Kram 1997 uses "leg length" L in
`v²/(g·L)` (abstract). Rose and Arellano 2021 (cited) use the square-root form `v/√(gl)`, with leg length measured from
the front of the hip bone (anterior superior iliac spine) to the inner ankle bone (medial malleolus).

Size-free quantities (Hof 1996, via Baker; secondary). `l` is lower-limb length, hip joint to floor.

| Quantity | Divide by (to make it size-free) |
|---|---|
| Speed | √(g·l) |
| Cadence (steps per **second**) | √(g/l) |
| Length (step, stride, excursion) | l |
| Time (stride time, contact time) | √(l/g) |
| Force / moment / power | m·g / m·g·l / m·g^1.5·l^0.5 |

Reference Froude numbers:

| Reference value | Froude number | Source | Evidence |
|---|---|---|---|
| Energy-optimal / preferred walking | about 0.25 in earlier studies (on 40 cm stilts it fell to about 0.17) | Leurs 2011 | abstract |
| Walk–run transition | about 0.5 | Kram 1997 | abstract |

Alexander's cross-species stride law (Alexander 1976, secondary via Vila 2013 and Prescott 2025): **stride / h ≈
2.3 (v²/(g·h))^0.3**, h the hip height. Solved for speed, v ≈ 0.25 g^0.5 λ^1.67 h^−1.17 with λ the stride (derived: the
two forms are the same law, 2.3^(−1/0.6) = 0.2495). A refit on elite human athletes changes 0.25 to 0.226 (Prescott
2025). In that law h is the hip-socket (acetabulum) height (Vila 2013); in the speed form λ is the stride in m and h
the hip height in m, or 4 × the track length (Prescott 2025); a stride is two steps. The original (Alexander 1976,
*Nature*, "Estimates of speeds of dinosaurs") could not be obtained.

### Rules for a character of another size

Derived from the size-free quantities above, with `h_new / h_ref` the ratio of hip heights:

| Quantity | Multiply by |
|---|---|
| Speed | √(h_new / h_ref) |
| Stride and step length | h_new / h_ref |
| Cycle time | √(h_new / h_ref) |
| Cadence | divide by √(h_new / h_ref) |
| Joint angles and phase percentages | 1 (unchanged) |

**Worked examples** (derived; hip height = 0.530 × standing height, from the segment table below):

| Character | Hip height h | Walk at Fr 0.25 | Walk–run switch at Fr 0.5 | Stride time against the 1.70 m person |
|---|---|---|---|---|
| 1.70 m person | 0.90 m | 1.49 m/s | 2.10 m/s (measured switch: 1.99 ± 0.20 m/s, Hreljac 2005) | 1 |
| 3.0 m giant | 1.59 m | 1.97 m/s | 2.79 m/s | longer by √(1.59/0.90) = 1.33 |
| 1.20 m child-sized figure | 0.64 m | 1.25 m/s | 1.77 m/s | — |

## 2. Segment lengths as fractions of standing height H

Drillis and Contini 1966, as drawn in Winter's Fig. 4.1 (secondary, read from the figure). Winter's own caveat: a good
approximation **in the absence of better data**; measure the individual (the character's mesh) when you can.

| Height of (from the floor) | × H | Length or width | × H |
|---|---|---|---|
| Eye | 0.936 | Head, vertex to chin | 0.130 |
| Chin | 0.870 | Shoulder width (joint to joint) | 0.259 |
| Shoulder joint | 0.818 | Chest width | 0.174 |
| Chest-width level | 0.720 | Hip width (joint to joint) | 0.191 |
| Elbow | 0.630 | Upper arm | 0.186 |
| Hip joint (greater trochanter) | 0.530 | Forearm | 0.146 |
| Wrist | 0.485 | Hand | 0.108 |
| Fingertip | 0.377 | Foot length | 0.152 |
| Knee | 0.285 | Foot breadth | 0.055 |
| Ankle | 0.039 | Thigh = 0.530 − 0.285 (derived) | 0.245 |
| | | Shank = 0.285 − 0.039 (derived) | 0.246 |

The same lengths from de Leva's joint-centre table divided by stature (derived from de Leva 1996; men 1.741 m, women
1.735 m), men / women: upper arm 0.162 / 0.159, forearm 0.154 / 0.152, hand (wrist to 3rd knuckle) 0.050 / 0.045, thigh
0.243 / 0.212, shank (knee to lateral malleolus) 0.249 / 0.249, foot 0.148 / 0.132, head 0.117 / 0.115. The upper arm is
shorter than Drillis and Contini's 0.186 because de Leva measures joint centre to joint centre; the female thigh is
shorter because of de Leva's hip-centre definition for women. Pick one table and keep it for a whole character.

## 3. Segment masses and centre-of-mass positions

Two standard models disagree by up to 40 % on some segments (thigh, trunk) (Duarte, BMClab notebook, secondary);
both are given. Mass is % of body mass. The centre-of-mass (COM) position is % of the segment length **from the first
named endpoint**. F = female, M = male.

| Segment | de Leva endpoints | Mass F / M (%) | COM from origin F / M (%) | Dempster via Winter endpoints | Mass (%) | COM from first (%) |
|---|---|---|---|---|---|---|
| Head and neck | vertex → gonion midpoint | 6.68 / 6.94 | 58.94 / 59.76 | C7–T1 → ear canal | 8.1 | 100 (at the ear canal) |
| Trunk | suprasternale → hip-centre midpoint | 42.57 / 43.46 | 41.51 / 44.86 | greater trochanter ↔ shoulder joint | 49.7 | 50 |
| Upper arm | shoulder → elbow centre | 2.55 / 2.71 | 57.54 / 57.72 | shoulder → elbow axis | 2.8 | 43.6 |
| Forearm | elbow → wrist centre | 1.38 / 1.62 | 45.59 / 45.74 | elbow axis → ulnar styloid | 1.6 | 43.0 |
| Hand | wrist centre → 3rd metacarpale | 0.56 / 0.61 | 74.74 / 79.00 | wrist axis → knuckle II of middle finger | 0.6 | 50.6 |
| Thigh | hip → knee centre | 14.78 / 14.16 | 36.12 / 40.95 | greater trochanter → femoral condyles | 10.0 | 43.3 |
| Shank | knee centre → lateral malleolus | 4.81 / 4.33 | 44.16 / 44.59 | femoral condyles → medial malleolus | 4.65 | 43.3 |
| Foot | heel → tip of longest toe | 1.29 / 1.37 | 40.14 / 44.15 | lateral malleolus → 2nd metatarsal head | 1.45 | 50 |
| Head + arms + trunk (HAT) | — | — | — | greater trochanter ↔ shoulder joint | 67.8 | 62.6 |

- **de Leva 1996** (cited): Zatsiorsky's gamma-ray scans of 100 male and 15 female Caucasian students (mean ages 24 and
  19 y), re-referenced to joint centres; reference subjects 73.0 kg / 1.741 m (men) and 61.9 kg / 1.735 m (women). The
  "head" keeps the same mass when its lower endpoint is moved to C7, so it includes the neck.
- **Dempster via Winter** (cited, Winter's Table 4.1; Dempster's data are 8 older male cadavers): the trunk rows are
  fractions of the trochanter-to-shoulder length. The table names a proximal and a distal fraction but not the end;
  pelvis 0.105 and thorax 0.82 only make sense measured **up from the trochanter** (derived, an inference).
- **Whole-body COM in quiet standing: 56.6 ± 0.5 % of height** from the floor, measured with a reaction board on 6 young
  men (172.5 ± 5.9 cm) (Hakamada 2009, cited). Small sample; treat it as a check, not a norm.

### Placing a centre of mass on a character

`COM = Σ mᵢ · pᵢ / Σ mᵢ`, with `pᵢ = originᵢ + cᵢ · (endᵢ − originᵢ)` for each segment's two endpoint positions from the
posed skeleton, mass fraction `mᵢ` and COM fraction `cᵢ` from the table above.

**Worked check** (derived): de Leva male masses and COM fractions placed on the Drillis and Contini heights of section
2, arms hanging, **assuming** the suprasternale sits at shoulder-joint height (0.818 H) and the foot COM at half ankle
height, give a standing COM at **0.563 H**, close to the measured 0.566 H. A character's own joint positions replace the
section 2 heights; the masses stay.

## 4. Standing and shifting weight

| Quantity | Value | Population, condition | Source | Evidence |
|---|---|---|---|---|
| Mean centre of pressure (COP), front to back | **44.5 ± 5.5 % of foot length from the heel**, wide-base stance | 60 healthy young adults | Teranishi 2013 | abstract |
| Same, standing on one foot | 50.2 ± 4.4 % (dominant), 50.4 ± 4.3 % (non-dominant) | same | Teranishi 2013 | abstract |
| Same, tandem stance | 41.0 ± 7.0 % of two foot lengths (dominant foot in front) | same | Teranishi 2013 | abstract |
| COP mean speed of sway, eyes open | 9.19 ± 1.6 mm/s (F), 9.24 ± 2 mm/s (M); front to back 6.39 ± 1.0 / 6.5 ± 1.35 mm/s; side to side 5.2 ± 1.2 / 5.2 ± 1.4 mm/s | 100 young adults (50 F, 50 M, 19–28 y), five 60 s trials, habitual stance | Błaszczyk 2023 | cited |
| Same, eyes closed | 11.2 ± 2.45 (F), 12.38 ± 3.48 (M) mm/s | same | Błaszczyk 2023 | cited |
| Frequency of body sway | COM sway lives below about 0.5 Hz; the faster COP flicker (0.5–5 Hz) is the ankles correcting it | as stated in Błaszczyk 2023, citing Blaszczyk 2008 | Błaszczyk 2023 | cited |
| Sway model | the body sways front to back as one inverted pendulum about the ankles; side to side it does not behave as a single pendulum | review | Duarte 2010 | cited |
| Standing COM height | 56.6 ± 0.5 % of height | section 3 | Hakamada 2009 | cited |
| Weight shift before a first step | COP moves **backward and toward the swing leg**, which pushes the COM forward and **toward the stance leg**; only then does the swing heel lift | review of gait initiation | Yiou 2017 | cited |

What these mean for a pose (derived): in a relaxed stand the weight sits a little behind mid-foot (about 45 % of foot
length from the heel), never on the heels. Sway is a few millimetres per second, so a believable idle moves the hips by
millimetres over seconds, not centimetres. Before the first step, the body first shifts its pressure back and onto the
leg that will swing, so the hips move forward and over the standing leg; the stepping foot unloads and lifts only after
that. Standing on one foot, the COP moves to mid-foot (50 %).

## Using it in GraphCode

- **No verb checks balance.** There is no `balance()` and no centre-of-mass verb. A pose that looks off-balance is not
  refused. Judge it in a render, and compute the centre of mass by hand with the formula above when it matters.
- **Scaling a clip.** The stride of a built clip comes from the character's own legs and the planted feet
  ([WALK-AND-RUN.md](WALK-AND-RUN.md)); you do not type a speed. Compare the clip's cycle time and the distance its
  root travels per cycle with the rules in section 1. Read your character's hip height from its rig
  (`gc_read kind:"rig"`, [RIG.md](RIG.md)), or use 0.530 × height.
- **An idle.** Author the sway as a small `shift(Root, ...)` (centimetres: a millimetre is 0.1 cm) or a small torso
  `swing`, over seconds, and keep planted feet planted. A sway amplitude is not sourced (see below), so do not present
  a number for it as a measurement.
- **A first step.** Draw the order: pelvis `shift` toward the standing leg first, then the stepping foot's support ends
  and it lifts. The size and duration of that shift are not sourced.
- **Breathing and other natural motion** are authored, not generated ([ANIMATION.md](ANIMATION.md), "Natural movement
  is part of the performance").

## Not yet sourced

Do not plan with a number for these until someone reads the source:

- Alexander 1976, 1984 and 1989 and Hof 1996 originals (only through Vila 2013, Prescott 2025 and Baker); the
  leg-length measure in Kram 1997.
- Standing: sway amplitude in millimetres (only sway speed is cited); the line of gravity or COP in centimetres in
  front of the ankle; preferred quiet-stance foot spacing and toe-out (McIlroy and Maki 1997 not reachable); the duration
  and size of the weight shift before a first step.
- Leads for later: a library copy of AAOS 1965, Perry and Burnfield 2nd ed., Winter's appendix gait data and Novacheck
  1998; Prieto et al. 1996 (IEEE TBME) for sway amplitude; the BMClab public balance dataset (Santos and Duarte 2016) for
  standing COP position and sway.
- The Manoonpong 2007 statement that preferred human walking is Fr 0.24 ("corresponds to 1.5 m/s"): how much of the
  source was read was not recorded, so it is left out of the tables.

## Sources

Read 2026-10-07; "read" says what was actually read. PMC links open the full text.

- **Alexander and Jayes 1983** — Alexander RMcN, Jayes AS. 1983. *J Zool* 201:135 (title not recorded). Read: search snippet only, so unconfirmed.
- **Błaszczyk 2023** — Błaszczyk JW, Beck M. 2023. Posturographic standards for optimal control of human standing posture. *J Hum Kinet* 86:7–15. doi:10.5114/jhk/159452. https://jhk.termedia.pl/pdf-159452-87600. Read: full text.
- **Duarte (BMClab)** — Duarte M, BMClab "Body segment parameters" notebook. https://raw.githubusercontent.com/BMClab/BMC/master/notebooks/BodySegmentParameters.ipynb. Read: secondary (model comparison).
- **Duarte 2010** — Duarte M, Freitas SMSF. 2010. Revision of posturography based on force plate for balance evaluation. *Rev Bras Fisioter* 14(3):183–192. https://www.scielo.br/j/rbfis/a/hFQTppgw4q3jGBCDKV9fdCH/?lang=en. Read: full text.
- **de Leva 1996** — de Leva P. 1996. Adjustments to Zatsiorsky-Seluyanov's segment inertia parameters. *J Biomech* 29(9):1223–1230. https://ebm.ufabc.edu.br/wp-content/uploads/2013/12/Leva-1996.pdf. Read: full text (Table 4).
- **Hakamada 2009** — Hakamada N, Funato K. 2009. Determination of center of mass using 3D anthropometry method during standing posture in humans. ISB 2009 abstract. https://media.isbweb.org/images/conf/2009/data/pdf/504.pdf. Read: full abstract.
- **Hof 1996 (via Baker)** — Hof AL. 1996. Scaling gait data to body size. *Gait Posture* 4:222. Read: secondary, via R. Baker, https://clinicalgaitanalysis.com/faq/normalisation.html.
- **Hreljac 2005** — Hreljac A et al. 2005. Preferred and energetically optimal transition speeds during backward human locomotion. *J Sports Sci Med* 4(4):446–454. https://jssm.org/jssm-04-446.xml-Fulltext. Read: full text.
- **Kram 1997** — Kram R, Domingo A, Ferris DP. 1997. Effect of reduced gravity on the preferred walk-run transition speed. *J Exp Biol* 200(4):821–826. Read: abstract.
- **Leurs 2011** — Leurs F et al. 2011. Optimal walking speed following changes in limb geometry. *J Exp Biol* 214(13):2276–2282. Read: abstract.
- **Pietraszewski 2012** — Pietraszewski B, Winiarski S, Jaroszczuk S. 2012. Three-dimensional human gait pattern — reference data for normal men. *Acta Bioeng Biomech* 14(3). https://actabio.pwr.edu.pl/fcp/aGBUKOQtTKlQhbx08SlkTUARAUWRuHQwFDBoIVURNWHtQFVZpCFghUHcKVj0GQUw/302/public/publikacje/v14-3-2012/42.pdf. Read: full text (the measured 1.47 m stride at 1.36 m/s).
- **Prescott 2025** — Prescott TL et al. 2025. Speed from fossil trackways: calculations not validated by extant birds on compliant substrates. *Biol Lett* 21(6):20250191. PMC12187409, https://pmc.ncbi.nlm.nih.gov/articles/PMC12187409/. Read: secondary for Alexander 1976 and the 0.226 refit.
- **Raichlen 2013** — Raichlen DA, Pontzer H, Shapiro LJ. 2013. A new look at the Dynamic Similarity Hypothesis: the importance of swing phase. *Biol Open* 2(10):1032–1036. PMC3798186, https://pmc.ncbi.nlm.nih.gov/articles/PMC3798186/. Read: full text.
- **Rose and Arellano 2021** — Rose & Arellano 2021, bioRxiv (title not recorded). Read: full text.
- **Teranishi 2013** — Teranishi T et al. 2013. Relationship between feet position and anteriorposterior center of pressure (COP) location. *J Phys Ther Sci* 25(1):41–43. doi:10.1589/jpts.25.41. https://pure.fujita-hu.ac.jp/en/publications/relationship-between-feet-position-and-anteriorposterior-center-o/. Read: abstract.
- **Vila 2013** — Vila B et al. 2013. Dynamic similarity in titanosaur sauropods. *PLoS One* 8(2):e57408. PMC3581443, https://pmc.ncbi.nlm.nih.gov/articles/PMC3581443/. Read: secondary for Alexander 1976.
- **Winter** — Winter DA. *Biomechanics and Motor Control of Human Movement*, ch. 4: p. 83 Fig. 4.1 (after Drillis and Contini 1966) and p. 86 Table 4.1 (Dempster via Miller and Nelson and Plagenhoef). https://courses.grainger.illinois.edu/me481/sp2021/Anthro-Winter.pdf. Read: full pages (edition not printed on the excerpt; the figure was read from its embedded image).
- **Yiou 2017** — Yiou E et al. 2017. Balance control during gait initiation: state-of-the-art and research perspectives. *World J Orthop* 8(11):815–828. https://www.wjgnet.com/2218-5836/full/v8/i11/815.htm. Read: full text.
