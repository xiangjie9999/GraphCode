# Lessons from real GraphCode work

*A GraphCode guide. Each lesson below is something that went wrong on a real project, why it went
wrong, and what to do instead. Read the section for the work in front of you. The tool's own help
still owns exact parameters; [AI-COLD-START.md](AI-COLD-START.md) and [TOOLS.md](TOOLS.md) own the
read-apply-verify loop.*

## 1. Quality and planning

**Rushing to geometry produces things that only resemble the reference.** An AI asked for a warrior
went straight to shapes and invented absolute positions. The result was recognisable at a glance and
wrong everywhere else: no plan for the layers, no armour construction, hair that stood out from the
head. *Do instead:* follow [ASSET-QUALITY.md](ASSET-QUALITY.md) in order: references, a written plan,
one representative part at final quality, then the rest.

**A clean build is not a good-looking result.** A clock tower passed 3,203 part-pair fit checks and was
still rejected as flat and office-like. A car's saved bonnet, cowl and mudguards passed every check
and were rejected as "simple curves". *Do instead:* judge the render against the reference, silhouette
first. Checks find faults; they never approve appearance.

**Comparing from memory drifts.** Each revision looked closer to the remembered concept than it was to
the real image. *Do instead:* open the actual reference image beside a render from the same angle
every time you compare.

**Detail postponed to a polish pass never arrives.** Parts built as "shape now, detail later" stayed
plain. *Do instead:* build each part to final quality when you build it. The finish pass is for
materials and wear, not for missing construction.

**Rivets round the edge are not detail.** A perimeter row of rivets on a smooth panel was rejected, and
so was a high rivet count spread evenly. *Do instead:* break large surfaces with construction that
reads from the whole-object distance: straps, seams with backing, ribs, layered plates, access covers.
Put each fastener on a joint it actually holds.

**More detail can make it worse.** A tower concept covered in small exposed gears was rejected as
"messy". *Do instead:* a few large purposeful features, quiet areas between them, and small parts
inside housings.

**Decoration with no job is noticed.** Disconnected service fittings on a steam alarm were called
out, and "decoration" is not accepted as a reason for a part. *Do instead:* give every part a function
and a physical connection.

**Speed is not a quality level.** When time is short, build fewer assets to the bar. Never build more
of them below it.

## 2. Shape authoring

**Agents fall back to boxes and cylinders.** Recipes built only from `rect`, `circle`, `extrude` and
`add` produced primitive stacks. *Do instead:* use precedents and high-level intent: project recipes,
the shipped presets (`gc_search kind:"preset" target:"<word>"`, more than 500 entries), and the
operation that fits the form: `revolve` for turned parts, `sweep` and `loft` for curved panels and
pipes, `round` for edges, `subtract` for recesses, `shell` for walls. See
[ASSET-QUALITY.md §6](ASSET-QUALITY.md#6-shape-precedents-and-high-level-intent-never-low-level-execution).

**Numbers nobody chose.** Values like `$Width * .784377367` come from working backwards from an
absolute measurement. They cannot be adjusted, and nobody can tell what they mean. The shipped
`HandAnatomy` and `AnimeBody` presets were written this way. *Do instead:* name the measurement
(`PalmWidth = 7.1`) and derive the rest from it.

**Listing a pattern by hand.** A car grille was written as 82 near-identical `add` lines. Unioning 82
solid bars instead stalled the editor for five minutes. *Do instead:* one line with `repeat=` and
`step=`/`spread=`/`around=`/`along=`. For a fine grille, use a geometric frame and bars with a masked
material for the mesh.

**Relations measure bounding boxes.** A post set `on()` a roughened or curved top floated above the
real surface and built as two pieces (`parts 2`). *Do instead:* read `parts` and the "NOT TOUCHING"
note, and sink the part with a negative `gap=`.

**Presets can be wrong too.** Measured 2026-09-28: `Solids.Dome` with a height different from its
radius sat its base at z = (r − h) / 2 instead of 0, so the shipped `Mechanical.Rivet` built as two
loose pieces (fixed the same day). *Do instead:* build a preset part alone first and read its `parts`,
`open`, `folds` and bounds before repeating it.

**A thin open plate is harder than it looks.** Measured 2026-09-28: `revolve` closes a profile to the
axis by default, and `shell` on the result gives a sealed hollow body. A profile widened into a band
then revolved came out inside-out with square ends, and folded with rounded ends at a fine `smooth:`.
*Do instead:* revolve the open profile as a surface and give it thickness,
`curve(...) revolve(capped=false) shell(0.25)`, which builds as one clean closed part, and weld a bead
round the rim for a rolled edge. Check `volume` (negative means inside-out) and render it.

**A rewrite that "meant the same" builds differently (measured 2026-11-19/20, the kit rewrites).** *Symptom:* a hand rewrite of a generated preset adds or loses a few triangles, with the same formulas. *Cause:* a set once stored `%.10g`, so naming a sub-expression rounded it and flipped a near-degenerate boolean. *Do instead:* in a current build a set keeps full precision (G28), so name freely; and prove a rewrite by building old and new under the same package path, comparing triangles, bounds, per-slot triangles and area on every build line plus extra lines at other seeds and knobs.

**A seedless draw re-rolls when you edit around it (measured).** *Symptom:* debris or a crack changes shape after you renamed an entry, merged two entries into one with a flag, or built the same text from another path, though the text is the same. *Cause:* a seedless `random()` is keyed by the entry's name and file path plus what its line is (verb, part, argument names, `if`/`loop` arm, pass), not by its line number. Old kits padded files with up to 1,537 blank lines to pin line offsets; that is no longer needed, and a comment or a moved block no longer moves a draw. *Do instead:* put `seed=` on every draw that must survive a rename or merge; compare old and new under one path.

**A kit written as a generator script (measured: `StoneKit` 16,056 lines, 13,811 of them unrolled names).** *Symptom:* a preset of thousands of lines of `$x1_3`-style names, `eq()` sums that pick "the block at drawn index", pad lines and sine-hash pairs, regenerated by Python. *Cause:* the grammar had no value per index, no list of points, no named formula. *Do instead:* `$x[i]` over a loop, a points list, `formula()` and `random()` in arithmetic; edit the one `.gcshape` by hand ([A kit is a recipe, not a generator](GCSHAPE.md#a-kit-is-a-recipe-not-a-generator)). The earlier spellings still build, but nothing new should be written that way.

**A name chosen by an index does not exist.** *Symptom:* a block repeated four times that differs only in a name suffix (`$k1ah0`, `$k1bh0`) stays unrolled. *Do instead:* set the values into one list (`$zl[0] = ...`) and read `$zl[$i]`; mind that a list keeps one kind and a set boundary.

## 3. Overlapping parts and flicker

**Two surfaces in the same place flicker.** Parts appended with `join=none`, or separate components
whose faces coincide, shimmer in stripes as the camera moves. *Do instead:* let `add` weld (the
default); seat parts with depth, never flush; leave a small gap between separate assets. See
[ASSET-QUALITY.md §7](ASSET-QUALITY.md#7-parts-must-not-share-a-surface).

**The overlap count cannot tell harmful from harmless.** Measured 2026-09-28: a plate buried flush in a
wall (flickers) reported `crossings 19`, and a plate 1 cm proud (fine) reported 22. The note on both
said overlap is how parts attach. *Do instead:* read `coincident`, which a build line now prints only
when two separate surfaces lie on each other facing the same way, and a Blueprint write now reports
for its components. Where neither reaches (C++ components, socket attachment, parts placed in a
level), inspect a close render at a grazing angle.

**Overlap-free is not connected.** Parts that passed an intersection check still floated. *Do
instead:* run a support check as well: every part and every island inside a mesh must connect to what
holds it.

## 4. Clothing and armour

**Armour is a garment.** Loose Shape meshes placed by hand did not fit the body or follow the skeleton.
*Do instead:* Garment fits and skins the layers, and Shape makes the hard parts, which the garment
seats on its rings. See [GARMENT.md, armour](GARMENT.md#armour-is-a-garment).

**Folded, pinched or twisted cloth usually starts at a ring.** Measured 2026-09-28: a chest ring given
front and back points near the armpits tilted 29°. The plate built between it and a level waist ring
had no warning in the build, and rendered covered in black-and-white speckle with its top rising to a
point over one shoulder. The same plate with a four-point chest ring rendered clean. Two rings whose
`@0` points sit at different places round the body make cloth that spirals, and the build does not
correct it. *Do instead:* read every ring's line in the dry run (its facing should read `looking
down` round a torso or leg, and its length should match the body), give torso rings front, side, back
and side points, and line up `@0` on both of a garment's rings. Then read the ring notes and each
build line's `folds`, `crossings` and `inward` (added the same day), and render the static result from
every side before going further. See [GARMENT.md step 3](GARMENT.md#3-inspect-and-measure-the-real-body).

**Speckle is not always two surfaces.** The tilted plate's speckle looked like faces fighting. It was
the checker texture on UVs laid at many times the cloth's size: a `lighting_only` render was clean and
an `unlit` one speckled. *Do instead:* render `lighting_only` and `unlit` before blaming geometry. Clean
lighting and a noisy texture point at the UVs; read the "laid flat" note's range.

**Layers are geometry, not draw order.** Every garment is fitted to the body, not to the garment under
it. *Do instead:* stand each outer layer's rings further off the body than the inner layer's clearance
plus its thickness. Hidden-body masks hide skin; they do not separate garments.

**A part on a garment cannot be sunk into the cloth.** A negative `gap=` on a rivet was overridden and
the rivet lifted onto the cloth. That is the intended behaviour: parts sit on the outermost cloth.

**Render-only is not simulated.** A MetaHuman outfit follows the bones; it does not swing or collide.
*Do instead:* for cloth that swings and collides, build `[as=cloth]` with `hang(from=$Ring)`; see
[GARMENT.md](GARMENT.md#fit-skinning-and-collision-are-different-claims).

## 5. Hair

**No `depth=` makes a porcupine.** Hair left along the skin normal and stood out in quills. Stiffness 4
made a nest, 16 a mane, and nothing about stiffness fixed it. *Do instead:* `depth=0.5` to `1` on the
layer, starting from [the proven coat](GROOM.md#the-proven-recipe).

**A flow can open a bald patch.** A single flow direction over a curved scalp parts the hair round the
point where that direction aims into the head. A later whole-scalp flow also silently took every
follicle from an earlier one. *Do instead:* one direction per zone, never pointing into the head
inside its own zone; general flows first.

**Close-ups hide the worst faults.** A crown close-up looked clean three builds in a row while the
quills stood out past the head, visible only from the front. *Do instead:* judge from the front,
three-quarter, side, back and top.

**Double weighting leaves loose hair.** A braid called on a region that already faded at its edges
selected by that fade twice and left 1,641 loose hairs under the coat. *Do instead:* give a feature an
unweighted twin region.

## 6. Characters

**Build the clean base first.** Hair, brows, clothing or accessories fused into the body cannot be
fitted, masked, swapped or repaired on their own. *Do instead:* keep each layer separate on the same
scale, pose and skeleton, and plan all of them before building any. See [CHARACTERS.md](CHARACTERS.md).

**A control that exists is not a control that moves.** A face control name can be present while the
rig exposes no reachable control, or no animation drives it. *Do instead:* prove motion in a render or
play session, not in the asset's text.

**A motion tuned to find its own end goes in circles** (owner's ruling, 2026-10-13: "the rule is simple,
poses, then add constraint and speed in the middle"). A one-handed sword cut was rebuilt six times by
changing shoulder, elbow and wrist angles inside the swing; the hand kept landing beside the wrong hip and
the wrist was wrong. Its end pose, built once on its own with IK placing and pointing the sword, was right
first time, and the travel from the start pose to it was the cut. *Do instead:* build and render the start
and end poses first, then the motion between them ([GCMOVE.md](GCMOVE.md),
[HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md#a-cut-with-a-held-blade)).

## 7. State, targets and proof

- **Correct operation, wrong target** is the most common dangerous mistake. Use full paths; a short
  name can match several objects.
- **After a refused or failed write, read the live asset again.** A refusal does not prove that every
  intermediate change was rolled back.
- **A read straight after a write can be reading memory.** Save, reload and read again to prove it
  persisted.
- **An unchanged document can round-trip through a fast path** and prove nothing about the parser or
  writer. Change something real.
- **A value that survives a reload may reach nothing.** Find the runtime consumer and prove it reads the
  value.
- **A whole-mesh average hides a local fault.** Look at the region: the face, a hand, a joint, a lock of
  hair.
- **A navigation rebuild for one agent says nothing about another.** Check the pawn's own agent at both
  ends of its route.
- **An exit code is not a test result.** Read the log for errors beside a passing assertion.
- **"fetch failed" is usually a busy editor, not a crash.** Check the editor's log for whether the call
  arrived before resending anything, and read the live asset: a write that timed out behind another
  client's long task may never have run (a character kept playing the previous clip, and a whole render was
  judged against the wrong motion).

## 8. Rigging and walking an animal (2026-10-19, the British Shorthair cat; measured)

- **Typed numbers from someone else's animal make yours wrong.** A lab cat's 1 s cycle at 0.4 m/s, typed on a smaller
  cat, made it stride 40 cm, crouch 2.4 cm and fold its hind knees to 67 degrees in swing (a cat's least is ~82).
  *Do instead:* write the speed and leave `frames=` out; the walk takes its cycle from its legs
  ([WALK-AND-RUN.md](WALK-AND-RUN.md) 6b). Papers check the result; they are not inputs.
- **A region's point is a joint's fold axis, not just its skin.** Points measured straight ahead of a limb, or a few
  centimetres off the back's midline, folded the cat's wrist 33 degrees off its paw and tipped its chest on every sway.
  *Do instead:* each region point is the skin hit from its joint toward where the joint closes ([RIG.md](RIG.md) Route B 4).
- **A changed axis changes the meaning of every `axis=` already written for that joint.** The sway lines said fold and
  swayed only while the back's fold lay near a side bend; once the back folded up and down, they bent it up and down.
  *Do instead:* after a rig change, read the walk from above and from the front, not only from the side.
- **"The body is still" is a weights question.** Check which bone each patch follows with the build's
  `DIAG_<Mesh>_Weights` overlay or by reading the weights, before moving joints or bands. The cat's flank followed helper
  bones hung on its pelvis; when those were moved, their skin by nearness stepped its back.
- **A fix that touches more than its target is a patch.** A weight rule meant for the back over the hips re-weighted
  4,000 vertices at the head and knees; a full band meant for the thigh re-laid half the body. Both were taken back.
  *Do instead:* the smallest change in the mechanism that decides it, then read every view.
- **A leg's pose found by a hand-run solver is a second copy of the rig, and it drifts** (measured, 2026-10-31). The cat's and the elephant's gait
  pictures were folds Python solved on its own copy of the rig: its axes read off a rebuilt picture came out 0.72 long, and a box round its start
  stopped a leg its range did not. *Do instead:* write the rule and let the build solve it: each joint at its measured angle with `give=`, the paw
  on its place with a contact pair (`swing(stifle_L, axis=fold, to=67, give=8)`, `reach(leg_B_L, contact=(Sole_B_L, Pad_B_L), to=$Land)`), solved at
  the moment the gait reaches the pose, on the body as it plays then ([WALK-AND-RUN.md](WALK-AND-RUN.md) 6c).
- **A swing turns from the rig's declared rest, not the skeleton's reference pose** (measured). The cat's stifle is 43.9 degrees folded on the
  reference pose and 60.5 at its `.gcrig`'s `rest=(fold=)`, which the build installs; a fold read on the reference ended a knee at 96.5 where 113
  was asked. *Do instead:* read a joint's fold on the rest the build animates from; `to=` does.
- **A cycle's last pose is its first again, at moment 1, not 0** (measured). Read at the clip time of moment 0 its step window was shut, so each
  gait landing at the seam kept its unsolved drawing and the paw came in 3.9 cm high. *Do instead:* read a pose of a list with the list's windows
  pinned at that pose's own moment, as the cycle's join reads it.
- **Reading a built clip in the shared editor holds it** (measured): the next headless spec could not save over it and failed "could not be
  saved". *Do instead:* `Tools/GaitCheck/gaitcheck.py release` after reading, `reload` before (its README, `Tools/GaitCheck/README.md` in the OraStudio checkout: a link from here would leave the plugin).
- **A green spec is not a pose within its give** (measured, 2026-11-01). The cat's rule walk passed every gait spec while its right foreleg landed with
  the wrist 34 degrees and the shoulder 19 past their give: the specs read footfalls and timing, the give is in the build's notes. *Do instead:*
  read every `PAST ITS GIVE` and `at its stop` note of a gait after each build, all four legs.
- **A foothold read off a built clip is read at the contact the reach places, not at a joint** (measured, 2026-11-01). The picture gallop's
  landings measured at the toe JOINT stood 1.0-1.3 cm ahead of its toe pads; drawn there, the rules gallop landed its knee 6 degrees straighter and
  yielded 38 in a frame. Read at the pads they were the walk's own footholds. *Do instead:* measure the rig contact the line names.
- **A paw whose two pads do not lie as its joints put them stands on the lower one** (measured, 2026-11-01). The cat sculpt's right forepaw has its
  palm pad 2.3 cm under the metacarpus against the left's 0.9: held exactly flat, its wrist opened 34 past its give; let off flat either way
  (`flat=1`, before the fix), its palm sank 1.5 cm into the floor. *Do instead:* `flat=1` on a standing pair, which stands it on its lower contact
  as a plant rolls a paw; a big remaining tilt is the sculpt's, for the rig, not the words ([WALK-AND-RUN.md](WALK-AND-RUN.md) 6c).

## Adding a lesson

Put the full lesson in the narrow guide that owns the decision, and add a line here only when a cold
AI would not otherwise find it. Say what happened, why, and what to do instead, with the real example.
Mark whether it is an owner's ruling, a measured result or an open question. Do not add a session
diary.
