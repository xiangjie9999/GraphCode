# Walk and run: the first four-legged walk, closed records (moved verbatim from WALK-AND-RUN.md section 6b, 2026-10-20)

Moved out of [../Guides/WALK-AND-RUN.md](../Guides/WALK-AND-RUN.md) when that guide passed 800 lines (the way a long document splits
into an open file and a record of what closed: open / DONE). These are the bullets of its old section 6b, "A four-legged walk,
written as intent" (2026-10-19), that describe machinery GraphCode no longer has: the stride rule, its own footholds, paw paths, body
height and tilt, the least-effort swing and the parallel-leg pass (MOVE-TRAVEL1, removed 2026-10-19 and 2026-10-20). They are word
for word as they stood, struck with the date and reason where they were superseded (the first holds in part: its kernel, that the
author says where each paw lands and lifts, is step 1 of 6a). **A four-legged walk is written as section 6a of the
guide.** Nothing here is written from; it is kept so a ruling or a measurement is not re-derived.

- **Say where each paw lands and lifts; the walk keeps it** (2026-10-19 evening; user: "why cant author says how each leg plant
  its feet?", "a good fix to the bugs, not a convinient button"). In the landing, mid-stance and lift-off pictures each leg reaches
  its toe pad to a point on the floor, ahead of, under and behind its top joint, as shares of the leg:
  `reach(leg_F_L, contact=Pad_F_L, to=$LandFL)` with `LandFL = point(bone=shoulder_L, at=(forward=20%), of=leg_F_L,
  relative=character, on=$Floor)` (`back=` behind). No joint angle, no paper: pick the shares by eye. ~~The walk drives each leg by~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B6, `Docs/GC-MOVE-TRAVEL-PLANS/PLAN.md`: GraphCode no longer drives a leg by its paw on a path of its own nor stands the body at a height of its own; `GcMoveProgramLegPaths`, `gc.Move.LegPaths` and the parallel leg are removed, and a walk is its pictures joined, the travel timing them.)
  ~~its paw through exactly those places (only where the leg cannot reach a point within its ranges does the whole stance sit
  further forward, and the note says by how much); the stance is as long as they are apart.~~
  ~~In a STILL PICTURE (a clip one frame long) a reached or planted paw stands as the rig stands it~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B6,
  `Docs/GC-MOVE-TRAVEL-PLANS/PLAN.md`: the solve's parallel-leg pass and the stood paw inside it are removed, stills and walks; a paw stands
  as its picture draws it and the plant holds its place; `gc.Move.PlantKeepsDrawnPaw` is gone. Then the paw rule of 2026-10-20, MOVE-TRAVEL1 B9 (6a step 7): a reach or plant moves
  only the leg above the paw, which stands flat on a ground as its contact points say. The rest of this paragraph is history:)
  In a STILL PICTURE (a clip one frame long) a reached or planted paw stands as the rig stands it, or as the picture's own wrist
  and paw lines turn it, and the leg is formed to put its pad on the point. It is ONE solve: the pad's place is the goal, the
  paw's drawn turn (as its own joint can hold it) a preference weighed like each joint's posture, every joint inside its range,
  the leg settling toward its parallel form, a planted paw's roll points kept over its ground; a leg that cannot reach the paw as
  drawn is first laid toward it as far as it spans. The note says how far the paw ends from its drawn turn ("its paw stands N
  degrees from its drawn turn, as near as its leg holds it on its place"). `gc.Move.PlantKeepsDrawnPaw 0` shows the old folded
  paw. (A first version held the drawn turn hard and added a fallback for each way it failed; each fixed one picture and broke another.)
  Measured on the cat: all six foreleg pictures at 30-46 degrees from upright, wrists 3.9-4.7 cm up, pads exactly on their points
  (before: folded flat, 52-61 degrees, the paw joint under the floor). Pictures drawn alike, each only "paw on the floor", say the
  paw never moves back, and the walk refuses them ("its paw ... is never further back under the body than at its first pose").
  (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B3: that refusal came from the stride rule, which is removed. The same drawing is now
  refused where a stretch's deciding paw is drawn no further back in the later key: 6a step 5.)
- ~~**Write no foothold: the stride places landing and lift-off itself**~~ (SUPERSEDED the same evening by the bullet above: it
  replaced the author's places with GraphCode's own, a convenience, not a fix; `gc.Move.StrideFootholds` is gone; AND REMOVED 2026-10-20, MOVE-TRAVEL1 B3 and B6: the stride rule, `GcMoveProgramLegPaths` and
  `StandTheBody` are deleted, and what follows is history) (2026-10-19; user: "We don't assume every author research
  for scientific papers"). A `[clock=root]` stride that holds its plant over a whole cycle puts its paw down two thirds of its
  cycle (a walk's, at any size), centred under its leg's top joint (the hip, or the shoulder blade's pivot), from the travel's
  speed and the clip's cycle. ~~It turns the leg's top joint to get there, and the joint that most changes the leg's length
  (knee, elbow) where that would sink the paw under its drawn height.~~ ~~Each such leg is then DRIVEN BY ITS PAW~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B6, `Docs/GC-MOVE-TRAVEL-PLANS/PLAN.md`: GraphCode no longer drives a leg by its paw on a path of its own nor stands the body at a height of its own; `GcMoveProgramLegPaths`, `gc.Move.LegPaths` and the parallel leg are removed, and a walk is its pictures joined, the travel timing them.): the paw's place
  along and across the body and its height over the ground are planned through the poses, and the leg is solved at every point
  (`Docs/GC-MOVE-CAT-PLAN.md` 1l, 1n). Where the leg cannot reach a point of its stance inside its joints' ranges (the cat's
  shoulder blade at the end of its swing back), the whole stance sits that much further forward; the note says "its stance sits
  N cm further forward". No part of the paw goes through the floor, and the body comes down to the height its legs carry it
  ("its body stands as high as its legs carry it"). The landing and lift-off pictures only stand the toe
  pad on the floor (`plant(leg_B_L, on=$Floor, contact=Pad_B_L)` in the picture), and the stride's lift-off is its last
  picture on the ground before one in the air. The note says where each paw lands and lifts, how much its rolling hold adds,
  and what turned; `gc.Move.StrideFootholds 0` keeps the drawn places to compare. Measured on the cat: all four legs down 67%
  (the drawn shares had 62-74%), no paw under the floor.
- ~~**The pictures' body**~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B6, `Docs/GC-MOVE-TRAVEL-PLANS/PLAN.md`: GraphCode no longer drives a leg by its paw on a path of its own nor stands the body at a height of its own; `GcMoveProgramLegPaths`, `gc.Move.LegPaths` and the parallel leg are removed, and a walk is its pictures joined, the travel timing them.) (2026-10-19 evening; now each key writes its own body, user W38 "author says it", 6a step 2): the one-frame pictures that start from one clip share one body, and GraphCode brings it
  down and tips it as far as their legs on a ground (planted, or reached to a point on the floor) need, with a bend to spare (note: "the
  body its N pictures are drawn on stands as high as their legs on a ground carry it"). Known limit tonight: from a sculpt's own standing
  height it may not settle (the test cat's did not), so write the shared clip's `shift(pelvis, ...)` / `swing(pelvis, ...)` by eye and
  GraphCode adjusts from there; the note says how far it moved and what is left.
- ~~**Write the speed, not the cycle: each stride times itself**~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B2 and B3: the stride's
  stance and its least-effort swing are removed; the travel times the poses by distance / speed, 6a step 5; the rest of this
  bullet is history) (2026-10-19 evening; user: "And I thought author declare how it
  curls? And when it lifts? And the [travel] decides how to time those together"). With no `frames=`, each `[clock=root]` stride's
  stance lasts as long as the travel takes to carry its paw from its landing place to its lift-off place (how far apart they are
  over the speed), and its swing as long as its joints take to go from lift-off through the swing picture to landing at the least
  effort that joins the two (the least share of the joints' accel references that can; `[accel=60%]` on the stride's line sets
  it instead). The walk is the longest stride's stance plus swing; every other stride swings in the rest of it at the least effort
  that plays it. The note: "no frames= written, so its cycle is its strides' own: 1.167 s ... its stance 0.653 s as its travel
  carries its paw 26.1 cm back, and its swing 0.504 s with its joints at 7% of their accel references". Measured on the cat at
  0.4 m/s: swings 0.39-0.50 s, paws down 56-63%, the joints' acceleration peaks 4-8.5 thousand deg/s2 (7-16 thousand before).
  Taken back the same day: the paw down two thirds of a cycle set by GraphCode, the cycle as 90% of the leg (next bullet), and the
  swing at all of each joint's reference (0.13-0.15 s, three times the accelerations).
- ~~**Write the speed, not the cycle**~~ (SUPERSEDED by the bullet above) (2026-10-19; user: "do cat bend the leg so much to move?"): `Walk = clip() { ... travel(root,
  velocity=0.4m/s) }` with its `[clock=root]` strides and no `frames=`. GraphCode takes the cycle from the legs: the stance is 90%
  of the shortest plant-held leg laid straight (top joint to paw; a leg sweeping about 30 degrees either side), the paw down two
  thirds of the cycle, at the travel's speed, and the note says what it chose ("no frames= written, so its cycle is its legs'").
  A cycle typed from someone else's animal is a stride sized for their legs: the cat at a lab cat's 1 s crouched 2.4 cm and folded
  its hind knees to 67 degrees in swing; at its own (0.77 s) they fold as a cat's do (80).
- ~~**For another animal (a dog):**~~ (SUPERSEDED IN PART 2026-10-20, MOVE-TRAVEL1: for the current way read 6a. A dog's author
  writes the rig, the keys (a whole-body still for each paw's moment, the body written in each), the plants inside the walk's
  poses and `travel(root, velocity=)`. The strides' `[phase=]`, GraphCode's own landings, paw paths, body height and tilt, and
  the shared `$Walking` body named below are gone. The ruling AN AUTHOR NEVER RESEARCHES PAPERS stands.) Copy the cat's two files, rename the chains to the new rig's, and change only the measured
  facts: the shares, the line directions, the travel's speed, each with its source. What is still the cat's own: the
  forelegs' measured joint angles, the footholds' numbers and the body's height (`shift(pelvis, ...)` in `$Walking`).
  ~~the reach before landing~~ GraphCode's now (next bullet). AN AUTHOR NEVER RESEARCHES PAPERS (user 2026-10-19): those
  numbers are becoming GraphCode's own defaults from the rig and the speed (`Docs/GC-MOVE-CAT-PLAN.md` 1k).
  AS OF 2026-10-19 (plan 1k-1o), a dog's author writes: the rig; the walk's `travel(root, velocity=)` and its strides'
  `[phase=]` (the footfall order); and the pictures, each a drawing of the leg's shape (landing, mid-stance, lift-off,
  mid-swing). ~~GraphCode then places each landing and lift-off (two thirds of the cycle, where the leg reaches within its
  ranges)~~ (SUPERSEDED the same evening: the pictures place them, first bullet), drives each leg by its paw over the ground, lifts the paw to its mid-swing picture's clearance, and sets the
  walking body's height and tilt from the legs. ~~A picture's places (`at=(back=44%)`, the fore print) are only drawings now:
  drawn at 40% the cat walked the same.~~ (SUPERSEDED: they are where the paws go.) STILL OWED: the pictures share one body, `$Walking`'s `shift`/`swing` (any height
  that lets every picture's planted legs reach the floor; the walk sets its own), until GraphCode derives it (plan 1o);
  and the rig's joint `velocity=`/`accel=`/`ramp=`, which a walk's swing reads only relative to each other.
- ~~**The leg lands and lifts moving with the ground by itself:**~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B3: `gc.Move.StanceJoins`
  is removed with the stride rule; the rest of this bullet is history) a stride that holds its plant over a whole cycle joins its
  landing and lift-off poses at the stance's own rate, so the leg reaches a little past its landing pose and draws back as
  it comes down, and swings back past its lift-off pose as it lifts (only for a moment: the ground's push ends there, so the swing
  turns toward its next pose at once). Draw no pose ahead of landing: write the swing as
  lift-off, mid-swing, landing (`{ $Contact, $MidStance, $Lift, $MidSwing, $Contact }`). The cat's paws now touch down at
  6-20 cm/s over the ground where a drawn reach left them sliding in at 22-38 and stopping in a frame. Its note says "its
  leg lands and lifts moving as its stance moves it"; `gc.Move.StanceJoins 0` switches it off to compare.
- ~~**The leg bends like an animal's by itself:**~~ (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B6: the solve no longer lays a held leg parallel, stills
  and walks; `GcMoveProgramLegPaths` and `ParallelLeg` were deleted the same day (B6 widened), so nothing forms a leg that way any more.) a chain whose joints are a free hip over two pure hinges (a hind leg), or a
  hinged shoulder blade, a free shoulder and an elbow (a foreleg), is placed with its first and third segments parallel
  (Fischer & Blickhan's pantograph; `Docs/Research/tri-segmented-limb-pantograph.md`), and its paw is then landed exactly by
  the plant solver, which settles toward that form inside every range. The note says how near parallel each leg came. A
  person's legs and arms are left alone (an ankle that turns sideways, a free clavicle).
- ~~**Each joint keeps its own pace in the swing**~~ (POSES.md "Each joint of a walking leg keeps its own time";
  SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B3: the per-joint swing retime is removed with the stride rule, and POSES.md strikes that bullet).
- ~~**Where angles still help:**~~ (SUPERSEDED 2026-10-20: a bare line and a stride's stance share are gone; a foreleg's place is
  a foothold in a key, 6a step 1; the rest of this bullet is history) a foreleg's measured elbow fixes where along its measured line the paw lands; with the line
  alone the leg straightened and stepped 3 cm too far (paw down 78% against a cat's 74%). Give a foreleg a measured
  foothold, not a bare line, before taking its angles out.
- ~~**A near-straight leg bends to the side its joints allow**~~ (built 2026-10-19; SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B6: the
  parallel leg is removed; the rest of this bullet is history): the parallel leg has two mirror
  solutions, and a foreleg standing almost straight has its shoulder almost on its top-to-paw line; GraphCode solves
  both and keeps the one its joints' ranges move least, the drawn side on a tie, and the path keeps that side. (Read off
  the drawing, a 1.7-degree rest twist on the cat's right elbow flipped its foreleg: blade asked 93 degrees against a
  38 stop, the paw 22 cm off its path.)
- ~~**Still drawn, and why** (2026-10-19, open): the forepaw's attitude in its contact, mid-stance and lift-off pictures.~~
  (SUPERSEDED 2026-10-20, MOVE-TRAVEL1 B9: a standing paw now stands flat as its contact points say, 6a step 7, so its joints need
  no drawing there, and no stride reads its stance from how far the pictures sweep the paw. The rest of this bullet is history.)
  A picture that says only `plant(leg_F_L, on=$Floor, roll=(Palm_F_L, Pad_F_L))` has its paw folded flat by the
  solve (metacarpus 77 degrees from upright where the rig stands it at 30), and `contact=Palm_F_L` alone is refused
  (an inner bone is planted only through `roll=`). Until a planted paw stands as its rig stands it in a picture, draw
  the paw's joints there. The pictures must also place the paw ahead, under and behind the leg's top joint: a stride
  reads its stance from how far its pictures sweep the paw.

