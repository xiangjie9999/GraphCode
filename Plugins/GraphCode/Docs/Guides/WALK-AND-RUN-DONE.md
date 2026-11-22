# Walk and run: closed and superseded sections

Moved here verbatim from [WALK-AND-RUN.md](WALK-AND-RUN.md) on 2026-11-04 when the guide passed ~800 lines
(Docs/AgentGuide/DOC-STRUCTURE.md: a closed item leaves the open file, moved verbatim). Nothing here is current practice.

## 6b. ~~A four-legged walk, written as intent~~ (2026-10-19, the cat; history, superseded by 6a on 2026-10-20)

> **SUPERSEDED 2026-10-20 (MOVE-TRAVEL1, `Docs/GC-MOVE-TRAVEL-PLAN.md`): write a four-legged walk as 6a.** The stride-and-foothold way
> this section described is gone. What stays below still holds: the places as shares of the leg on the floor, the swing clearance,
> the rig fact about joints, and the sway. The rest is in the record linked below.

Say where each paw goes and what the leg is for; GraphCode bends the joints. No joint angle and no line converted by hand
on an assumed hip height (user: "intent based ... Not hard cold numbers").

Moved verbatim to [WALK-AND-RUN-DONE.md](../Historical/WALK-AND-RUN-DONE.md) on 2026-10-20 (closed records, struck with their dates and reasons where superseded): "Say where each paw lands and lifts; the walk keeps it" (with its still-picture paragraph); "Write no foothold: the stride places landing and lift-off itself"; "The pictures' body"; "Write the speed, not the cycle: each stride times itself"; "Write the speed, not the cycle"; "For another animal (a dog)" (the long form); "The leg lands and lifts moving with the ground by itself"; "The leg bends like an animal's by itself"; "Each joint keeps its own pace in the swing"; "Where angles still help"; "A near-straight leg bends to the side its joints allow"; "Still drawn, and why".

- **Footholds as a share of the leg, on the floor:** `point(bone=hip_L, at=(back=44%), of=leg_B_L, relative=character,
  on=$Floor)` and **the hind paw lands where the fore paw lifted** (`reach` to `point(bone=toe_F_L)`): REINSTATED 2026-10-19
  evening (they had been marked superseded by GraphCode's own footholds, which are gone). A point the leg cannot reach is said
  ("ends N cm short"); on the cat, the hind landing on the fore print stands 23-26 cm from the hip, past its 22.7 cm leg. Places as shares (`at=(up=11.5%), of=leg_B_L`) and `on=` still work wherever a pose places a paw on purpose.
  Under the hip is `point(bone=hip_L, relative=character, on=$Floor)`.
  In the walk's four keys the fore paw has already curled when its hind lands, so its toe is no print there: the key draws the hind paw 25% of its
  leg ahead of the hip, which is where it walks onto the print its fore paw left (6a, step 1).
- **Swing clearance as a share too:** `Pass = point(bone=hip_L, at=(up=11.5%), of=leg_B_L, relative=character, on=$Floor)` is
  the paw under the hip, that far over the floor (`on=` measures `up=` from the ground). Say where the number comes from:
  no cat clearance is published, so it is marked an assumption.
- **For another animal (a dog):** copy the cat's two files (the keys and the walk), rename the chains to the new rig's, and change
  only what was drawn or measured: the places (shares of the leg), each key's body and the travel's speed. An author never researches
  papers for them (user, 2026-10-19): pick each number by eye, write it once, and look at the render. The long form of this bullet,
  with the strides it once described, is in [WALK-AND-RUN-DONE.md](../Historical/WALK-AND-RUN-DONE.md).
- **Rig the joints where the limb bends:** a hock on the heel's point instead of the ankle's crease skewed the cat's fold
  axis 21 degrees from its knee's, and no parallel leg could hold it. Measure the mesh's outline before placing a joint.
- **The body sways to the side as `axis=side`** on the spine, chest, neck and head (and the tail's own side the way the
  curve runs). Each spine joint folds about the axis its back band gives it, which is the up-down bend; a sway written as
  fold bends the back up and down. Check a sway from above: swayed left, the head and the tail tip both go left (a C).

