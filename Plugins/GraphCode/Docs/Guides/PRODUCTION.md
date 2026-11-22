# Build a game that can ship

Read this when the customer asks for a production game, a polished vertical slice, or release
planning. Start here before choosing assets or architecture. Exact tool syntax stays in tool help;
the [authoring guide](AUTHORING.md) explains each product's workflow.

**Production quality is a set of demonstrated outcomes.** The player can understand and complete
the intended loop; the art, motion, UI and audio meet the agreed direction; progress survives the
required transitions; the game runs within its budget on its target hardware; and the delivered
package works without the development environment. A large level or a successful apply proves none
of those by itself.

For the design method, use [DESIGNING.md](DESIGNING.md); for the mechanic's rules and feel, use
[GAMEPLAY.md](GAMEPLAY.md). Read [UI-UX.md](UI-UX.md) before authoring the slice's interface and
[UI-TOOLS.md](UI-TOOLS.md) when outside tools would improve it. The
[worked delivery slice](GAME-SLICE-EXAMPLE.md) connects those decisions to one complete journey.
That example teaches the design method; it is not a runnable sample or evidence for this project.

Use this order, starting at the first unmet condition in the existing project:

```text
player outcome and target
  -> resolve the dependencies that could invalidate the slice
  -> build one connected journey at the intended quality
  -> save, restart and run an early package
  -> measure at representative density
  -> expand in useful batches
  -> validate and hand over the exact release candidate
```

Keep implementation details in the linked subject guides, current syntax in installed help, and
this project's decisions and observations in its production note. A production guide should let
the next AI choose the next useful action without loading every product's documentation.

## The first session

Apply [RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md) to every unfamiliar or complex part of the
slice. Save the research findings and detailed implementation stages before building that part;
link the plan from the production note instead of duplicating it. Resolve critical dependencies
with planned small experiments and continue authorized implementation when they are understood.

1. Read the project's own agent guide, board and relevant design decisions. Inspect the existing
   game and confirm the connected project and installed products through `unreal_info` and help.
   Preserve existing work and permissions. Run the existing loop before deciding it needs replacing.
2. Write one sentence describing the player loop and the intended audience. Identify the current
   milestone and the largest uncertainty: feel, appearance, a system interaction, performance or
   deployment. Build the next slice to answer that uncertainty.
3. Record the target platform, representative minimum hardware, resolution and quality settings,
   input devices, frame-time target, memory and loading limits, and single-player or network model.
   Use known project answers; ask only for missing decisions that change the work. Label provisional
   budgets as assumptions. An unmeasured budget is a target, never a result.
4. Establish the visual and audio bar using references and one representative in-game view.
   Every visible asset then follows [the asset quality workflow](ASSET-QUALITY.md).
   Read [content sourcing](CONTENT.md) before choosing the source. Reduce the slice's size to fit
   the budget before lowering a quality bar the customer already chose.
5. Put the smallest complete slice on the board, with a visible outcome and verification method.
   Begin implementation within the existing authorization. Keep an unknown capability as a named
   dependency to investigate, rather than inventing syntax or quietly removing the requirement.

Keep these project-specific answers in an existing production note, or adapt the
[production plan seed](../Seeds/PRODUCTION-PLAN.md). Link to design decisions rather than copying
them. Keep open tasks on the board; the production note holds targets and evidence.

## Prove the dependencies before multiplying content

Trace the journey through the systems that must cooperate. For a pickup, follow input and target
selection into the authoritative inventory change, then UI, animation, sound, persistence and
reset. Name the asset or component that owns each fact and how the next system observes it.
Creating those assets individually does not establish that the connection works.

Record only the consequential dependencies in the production note. For each, name the required
outcome, owning source, installed authoring route, runtime consumer and smallest acceptance check.
Use [CAPABILITY-MAP.md](CAPABILITY-MAP.md) to find the relevant product, then inspect current help
and the actual asset's coverage. Distinguish engine support, GraphCode authoring support and a
result verified in this project. Keep an untested path marked **not run**, even if its assets
compile. A missing optional product, unsupported operation and unfinished implementation need
different responses; investigate before treating one as another.

Use the following questions where the journey needs them:

| Dependency | Resolve before production expansion |
|---|---|
| Character assembly | Do the actual body, face, hair, clothing and held objects work together through the required motion and camera distances? Follow [CHARACTERS.md](CHARACTERS.md), then only the relevant motion/clothing/hair route. |
| Cinematic or scripted event | Follow [CINEMATICS.md](CINEMATICS.md) and save the [director's packet](CINEMATICS-PREPRODUCTION.md) before production: intent/treatment, beats/focus, boards, planned view-study results, timing, shot rationale and dependencies. Use [CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md) for authoring. Does each track affect its intended runtime actor/component? Verify timing, skip/interruption and return of player control; review the complete cut and mix. |
| Audio and VFX | What starts, updates and stops each effect? Observe onset, sustained phase and cancellation in play; listen to the combined mix. Follow [CONTENT.md](CONTENT.md) for sources and installed help for the chosen asset kinds. |
| Persistence or networking | Which state must survive, who owns it, and what reconstructs it? Exercise restart or the actual server/client path from [GAMEPLAY.md](GAMEPLAY.md) before duplicating the mechanic. |
| Import, export or delivery | Does one representative result survive the complete source-to-destination path with its materials, motion and required dependencies? Verify in the destination; an exporter success is only one step. |
| Package and platform | Can the small slice be built, launched and played using the intended runtime dependencies and target configuration? Resolve missing SDKs, runtime plugins or cooked content before expanding the level. |

When a path is uncertain, run the bounded experiment in [RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md).
Use a supported native workflow within the existing authorization when it meets the requirement;
record the route so another AI can repeat it. If no verified route meets the requirement, name
the blocked outcome and continue independent work. Changing the promised outcome is a design
decision, not an automatic response to a tool refusal.

## Choose the next milestone by its exit condition

| Milestone | Build | Evidence needed before expanding |
|---|---|---|
| Feasibility, only where uncertain | One small experiment for the hardest dependency | The required operation works in the installed tools and engine, with an observed result and known limits. |
| Playable loop | Input, action, consequence, feedback, failure and retry | A repeatable play session; no manual editor action needed to keep the loop going. |
| Representative vertical slice | A small complete experience with the intended art, motion, sound, UI and systems | In-game review, persistence where required, measured performance and a first packaged run. |
| Production expansion | More content using the proven slice's components, asset standards and authoring process | Each batch meets the same quality bar; regression scenarios and performance remain within budget. |
| Release candidate | A specific package built from a known revision | The [validation guide](VALIDATION.md) gates pass on the intended environment, with unresolved issues visible. |

These are exit conditions, not a requirement to restart an existing game. An approved prototype
can answer a mechanics question with blocks. A request for a realistic forest, finished character
or polished environment calls for representative assets from the first reviewed slice. Do not
present a blockout as that milestone's completed art. If a temporary proxy is needed, record its
purpose and replacement task and keep the visual gate open.

## Build one complete player journey

For a survival game, a useful first slice could be: start a new game, find a resource, collect it,
craft one useful item, use it against a threat, survive or fail, then retry or resume. Give that
route representative finished surroundings, readable prompts, interaction animation, sound and feedback
to judge the intended experience. Add a second biome only after this journey works.

For a robot game, make one robot accept a destination, travel, react when a route is blocked,
then arrive or clearly fail. Verify a blocker present before departure and one raised while
the robot is moving; the second requires runtime navigation support and its own play evidence.
Use the [AI navigation guide](AI-NAVIGATION.md) for StateTree, EQS and path-following roles.

For other genres, change the nouns: select and finish a race; solve and reset a puzzle; enter and
finish an encounter; build and operate a small production chain. Pick a journey with dependencies
that future content will share. A showcase camera shot is useful art evidence, but it does not
exercise input, collision, progression or recovery.

Make the slice reproducible: identify its start map, player, scenario, initial state and completion
condition. A new contributor should be able to launch it without recovering setup steps from chat.

### Implement the journey in connected steps

Adapt this sequence to the project's existing systems. Each step produces something the next
can use; reuse a completed step instead of rebuilding it. Keep the agreed presentation standard
throughout and follow [ASSET-QUALITY.md](ASSET-QUALITY.md) for construction and review.

1. **Establish the real entry path.** Use the intended start map, player, camera and input route.
   Reach the slice as a player would. Record any temporary launch setup and its removal task;
   spawning an actor manually in the editor cannot close the normal-start requirement.
2. **Complete one state change.** Connect an input to a real target, validate the action, commit
   its result once and expose the reason when it cannot proceed. For the pickup example, both
   inventory quantity and the collectible's availability must agree after the action.
3. **Connect the presentation to that state.** UI, motion, sound and VFX must respond to the same
   accepted or rejected action. Judge them together at the gameplay camera, including timing,
   readability and mix. Keep editable art/audio sources and the asset provenance with the work.
4. **Complete failure and recovery.** Try repeat input, interruption, target loss, failure and
   retry where applicable. Check input focus, camera ownership, subscriptions, timers and looping
   effects after reset. Run a second attempt without restarting the editor.
5. **Prove durability and deployment.** Save and reload authored work. If the game promises saved
   progress, save through the game, quit, relaunch and load it. Run an early package through the
   same journey using [VALIDATION.md](VALIDATION.md); record editor and packaged results separately.
6. **Add a meaningful variation.** Use a second item, encounter or route with different data to
   expose hard-coded assumptions. Check the shared behaviour and the differences the design
   intends. More copies of the first asset do not exercise variation.
7. **Measure the intended workload.** Exercise the expected simultaneous actors, effects, UI and
   streaming along this route. Record first-use and repeated performance, loading and memory.
   Correct measured bottlenecks and recheck appearance before expanding production.

For example, a collection journey is ready to expand when a player can collect once, see and
hear the accepted result, receive no duplicate reward on repeat input, get useful feedback when
full, and retain the intended result after restart in a package. Record the actual observations.
This describes an acceptance scenario; it does not claim a GraphCode fixture has already passed it.

## Make deliberate architecture choices

- Reuse the project's working systems and Unreal's supported frameworks where they fit. Before
  adding a manager, component or plugin, name the responsibility it owns and its lifetime. The
  [gameplay guide](GAMEPLAY.md) covers state ownership and system boundaries.
- Choose streaming from the size, traversal, memory and collaboration needs of the game. A small
  production game may need neither World Partition nor data layers. Investigate large-world or
  network requirements early enough to prove them in the slice.
- Keep tunable content separate from behaviour when multiple items use the same rules. Add one
  item or enemy through data and shared behaviour to prove expansion does not require copying graphs.
- Define the source of each built asset. Edit a recipe when it owns the result, preserve imported
  sources and settings, and retain binaries for facets that `.gc` omits. Generated output alone
  is not a reproducible production pipeline.
- Use the actual camera and expected density early. A hero prop's close-up, a crowd and a forest
  crossing have different costs; measuring one does not establish the others.

## Keep the quality bar concrete

| Area | What the slice must demonstrate |
|---|---|
| Art | Coherent scale, silhouette, material response, lighting and detail at the intended camera distances. Inspect seams and repetition on representative surfaces; use the [texture variation guide](TEXTURE-VARIATION.md) to consider UV variation, near/far blending and macro masks, then prove transitions in motion and within budget. |
| Motion and interaction | Grounded movement, useful collision, reachable interactions and transitions that survive interruption. |
| UI and accessibility | A coherent visual language and designed normal/empty/busy/error states; readable prompts, visible focus, supported input paths, responsive layout, useful feedback and the agreed accessibility settings. Prove the real journey using [UI-UX.md](UI-UX.md). |
| Sound | Audible action and consequence, suitable levels, transitions and no missing or stuck loops on retry. |
| Reliability | Failure and restart, repeated use, level transitions and save/load where the design requires them. |
| Performance | Recorded frame times, memory and loading behaviour for representative and expensive scenarios on the target configuration. |

Art style is a project decision: realistic and stylized games both need a coherent, demonstrated
bar. Do not equate detail or triangle count with quality. Do not silently substitute primitive
geometry when suitable authored or imported assets are needed to meet the chosen direction.

For water, use [WATER.md](WATER.md) to finish one representative moving section, especially its
material: depth, reflections, roughness, flow, foam and shore transitions. Prove it in playback
and within the target budget before expanding across the level.

## Expand without losing the proven result

Choose the next batch by what it adds to the player journey: another encounter, a complete room,
a character variation or a new route. Keep it small enough to inspect at the intended quality.
Reuse the representative result's sources and conventions; give each new variant a reason and
verify that it works in context. Recheck shared systems when a batch changes their assumptions.

Before marking that batch complete, close its open proxy/replacement tasks, confirm its sources
and dependencies are recoverable, exercise its failure/retry path, and measure any increased
density. Keep a failing required check attached to the affected milestone. A missing target device
or unrun package stays **not run**; an art or feel decision awaiting the agreed review stays
**review pending**. Neither becomes a pass because other checks are green.

Rerun the affected scenario after a change and the broader release checks when cutting a
candidate or changing a shared dependency. Record which evidence the change invalidated. Avoid
repeating unrelated tests while the next required part remains unbuilt; the
[validation guide](VALIDATION.md) owns the full release matrix.

## Finish a slice so another AI can continue

Re-read authored assets, inspect the result, exercise the player path and record the evidence
appropriate to the claim. Use [validation](VALIDATION.md) to distinguish an editor check from a
package check. Report what changed, what was observed, any remaining failure and the next board
item. Keep captures and logs linked to the tested revision, map and configuration.

Leave a short continuation record in the existing production note: the current candidate, how
to launch and safely reset it, the last observed result, the next executable board item and any
specific dependency that prevents it. Link the owning sources and evidence. Another AI should
be able to resume from those facts without recreating the environment or trusting an old green
status after the assets have changed.

For a release handoff, identify the exact delivered artifact, tested platforms/configurations,
runtime dependencies, launch instructions, accepted limits and recovery/update requirements.
Verify the delivered copy through the native packaging workflow. If delivery promises an editable
project or reusable character/asset, also reopen it in the intended destination with only the
declared dependencies and check its sources, references and runtime behaviour there.

Follow the customer's review cadence for direction and feel. Continue authorized implementation
between those reviews. Report a missing product, unverified target or rejected dependency clearly;
use an available native workflow when authorized and supported, and identify what that workflow
proved. Never label an untested requirement complete to make the milestone read green.
