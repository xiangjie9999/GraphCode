# Design a game around the player's experience

*A GraphCode guide. Read it when the task needs it, not before — the router is [`README.md`](README.md), and the setup run that points here is [`SETUP.md`](../../SETUP.md).*

The loop is: **write it, look at it, play it, change it.** Use the installed authoring, rendering
and play tools for the parts they support; check help before promising an automated path.
For a production game or polished slice, start with [PRODUCTION.md](PRODUCTION.md): it sets the
quality bar, build order and evidence needed before expanding content.

- **Write it** — the mechanic, the level, the widget, as text.
- **Look at it** — render it and show them.
- **Play it** — run the game, drive it, capture what the world actually did.
- **Change it** — the diff is readable, the history is real, and going back is a call rather than an
  apology.

What to lean on, before anyone tells you otherwise:

- **Build one complete player journey before expanding the world.** Include input, consequence,
  feedback and retry. For an existing game, start from the loop already present.
- **Match fidelity to the task.** Blockout is useful for an approved mechanics experiment. When the
  customer requests production art, build a smaller representative area at that quality first.
  A temporary proxy needs a replacement task; it does not satisfy the visual milestone. Every
  visible asset follows [ASSET-QUALITY.md](ASSET-QUALITY.md): quality, never speed.
- **Expand after the slice works.** More levels do not fix a thin loop. If travel or streaming is a
  core requirement, include a representative boundary in the first slice.
- **Keep the smallest thing playable at every moment.** Never leave the project in a state where
  nobody can press play.
- **Architecture follows requirements.** Choose state ownership, persistence and any networking
  model early. Add streaming, data layers and reusable components where the game needs them.
- **Decide against something you can see**, not against a description of it.
- **Let them steer what only a person can judge** — how it feels, how it looks, whether it is fun. You
  can measure that a wall is four metres from the door. Only they can say it is too close.

## Make each loop a proof

State the intended player action and one visible success before writing assets. Build the smallest
complete slice: input, world response, feedback and a failure path. Read the created assets back,
render them from the intended camera, then drive the path with `gc_play` when installed, or an
available Unreal play workflow. A clean compile establishes
neither appearance nor behaviour. If the claim is about a packaged game, run the packaged game.

Show the owner the render or Play capture at the cadence chosen in setup. Record their judgment
about placement, proportions, motion and appeal in the relevant design note, then revise the
artifact. An automated measurement can help locate a visual problem; it cannot vote it away.
Keep the board for open work and move completed work off it once the result has been demonstrated.

## Start with an experience, then choose rules that can create it

When the requested work or a proposed technique is unfamiliar or complex, follow
[RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md). Research first and save the task-specific findings
and implementation plan before building; the same method applies to every area of the game.

Write a short design brief in the project's game-direction note before expanding the feature list:

```text
Player and context: who is playing, on what device, for roughly how long?
Fantasy: who am I and what do I get to do?
Desired experience: what should this play feel like?
Core loop: observe -> choose -> act -> consequence -> next decision.
Tension: what makes the choice matter; what can I gain or lose?
Success, failure and recovery: what ends a run, and why try again?
Distinctive promise: what should a player remember?
Pillars: a few priorities that decide tradeoffs, with observable consequences.
Scope: the smallest complete journey and what is outside this milestone.
Unknowns: which assumptions need play evidence or the owner's judgment?
```

Use known project decisions rather than reopening them. If the user asked only for design or
research, deliver that work; do not treat this workflow as implementation authorization. For an
authorized build, make reasonable reversible choices, mark assumptions and keep moving.

The [MDA paper by Hunicke, LeBlanc and Zubek](https://www.cs.northwestern.edu/~hunicke/MDA.pdf)
distinguishes rules and systems, the behaviour they produce during play, and the experience the
player has. Use it as a reasoning tool: work backward from the experience, then observe whether
the chosen rules actually produce it. A list of mechanics alone cannot establish that result.

Example: "tense but fair delivery" suggests a visible hazard, a safer alternative, a tempting
bonus and a short recovery path. A hidden random failure would undermine that promise. A pillar
such as "danger is readable" should lead to warning cues, useful camera framing and a test in
which players can explain why they failed. "Fun and immersive" cannot decide those tradeoffs.

## Design decisions, not just activities

A useful mechanic offers an understandable action with a consequence. Many games also rely on
interesting tradeoffs: speed versus safety, immediate power versus future resources, exploration
versus time. Other games intentionally emphasize expression or relaxation; do not force combat,
scarcity or punishment into a brief that does not want them.

For each choice, identify what the player knows, what is uncertain, the available options, and
why more than one option could make sense. If one action always dominates, inspect the rules,
information or costs. If the outcome is arbitrary, inspect what information or control is missing.
If repetition adds no new judgment, variation or pleasure, more content may only lengthen boredom.

Separate three timescales when the game needs them:

| Scale | Question | Example |
|---|---|---|
| Immediate action | Is performing and reading the action satisfying? | Steering around a vent and recognizing its warning. |
| Encounter or run | Do decisions combine into a meaningful arc? | Choosing a delivery route, managing damage, completing or retrying. |
| Longer progression | Does continued play offer worthwhile possibilities? | New routes or tools that change decisions, not only larger numbers. |

Prove the short loop before funding the long one with dozens of unlocks. Progression should
support the game's promise. Avoid adding currencies, crafting, skill trees or daily rewards just
because other games have them.

## Turn mechanics into a coherent level and learning sequence

Teach an action where it is safe to notice and try, then introduce a consequence, then combine
it with a known action. For a puzzle this may be discover -> use -> recombine; for a race, learn
steering -> read a corner -> choose a risky line. The exact sequence follows the intended player,
not a mandatory tutorial template.

Choose sightlines, landmarks, collision and camera framing around the player's decisions. Put
useful information before the commitment point. Offer room to recover when the design promises
fair recovery. Test the actual pawn and camera, not just an overhead map. Art, lighting and sound
can direct attention, but critical instructions should remain readable when effects overlap.

Plan changes in intensity: anticipation, action, consequence and a chance to understand or recover.
Constant maximum noise makes important events harder to distinguish. Test how long traversal,
menus, failure and retry actually take. A short encounter can feel slow if restarting it requires
several unnecessary screens.

Write narrative through the player's actions where appropriate: a recipient reacts to the parcel's
condition; a repaired machine changes the route. Dialogue and lore should support the playable
experience rather than excuse a missing interaction. Keep story promises consistent with game rules.

## Design feedback, art, UI and audio together

Every key action needs a readable beginning, change and result. Decide what the world, animation,
sound, camera and UI each contribute. Feedback should expose a real rule: damage corresponds to
a valid hit; a reward display corresponds to a committed reward. Choose sound roles and priorities
so navigation sounds, warnings and music do not mask one another. Exercise mute and subtitle paths.

Read [UI-UX.md](UI-UX.md) before making screens. HUD placement changes what hazards can be seen;
the camera changes how far a warning needs to be readable; the retry flow changes the cost of
failure. These are design dependencies, not a finishing pass. [UI-TOOLS.md](UI-TOOLS.md) explains
when an external visual or browser prototype can help settle them.

## Balance through hypotheses and observation

Keep tunable values in the mechanic's data and record their meaning and units. Derive related
values when appropriate: route time from measured distance and movement, capacity from intended
choices, reward cadence from actual encounter duration. Chosen constants are legitimate design
decisions; unexplained precision and duplicated values make tuning unreliable.

For a resource economy, list sources, sinks, storage limits and the expected opportunities to
spend. Simulate a few representative sessions in a small script or spreadsheet when useful, then
compare with play. An arithmetic balance can still feel tedious, and a simulation using assumed
player behaviour is not playtest evidence. Test hoarding, repeated rewards and unusually efficient
play before expanding progression.

Use a short experiment log rather than accumulating guesses:

```text
Observation: players wait out every vent cycle and ignore the shortcut.
Hypothesis: the time bonus is not understood before the fork.
Change: show the bonus condition on the contract and HUD before departure.
Scenario: first delivery, same route and hazard settings.
Observe: whether players notice the bonus, explain the choice and try either route.
Result: observed behaviour and quotes, or not yet tested.
Decision: keep, revise or restore; why; next uncertainty.
```

Change one meaningful cause at a time when testing a hypothesis. Never invent observations or
player quotes. Automated input can prove a rule and expose exploits; it cannot stand in for a
new player's comprehension or enjoyment.

## Run a playtest that can change the design

Choose the question and a reproducible starting state. Let a player attempt the task without
coaching unless help is the thing under test. Observe hesitation, missed information, intended
versus accidental actions, repeated failures and recovery. Afterwards ask what they thought
the objective and rules were, what felt good or frustrating, and what they wanted to do next.

Keep observation separate from interpretation: "opened inventory four times looking for the
parcel" is evidence; "inventory is confusing" is a hypothesis. Combine behaviour, technical
measurements and the owner's judgment. A tiny playtest can reveal a serious usability problem;
it does not establish a population-wide retention rate or prove the game is universally fun.

When a problem appears, classify it before adding systems:

- **Comprehension:** the player cannot tell what to do or why something happened.
- **Control/feel:** they understand the action but cannot execute or predict it comfortably.
- **Decision quality:** choices lack consequence, variety or a plausible alternative.
- **Pacing:** the experience spends too much time waiting, repeating or recovering.
- **Presentation:** the visual/audio hierarchy fails to express the intended state or direction.
- **Reliability:** the designed rule is not what the running game actually does.

Fix the largest demonstrated issue, replay it and its nearby failure paths, then expand. For the
connected design-to-runtime exercise, read [GAME-SLICE-EXAMPLE.md](GAME-SLICE-EXAMPLE.md). For
mechanic specifications and tuning at the implementation boundary, read [GAMEPLAY.md](GAMEPLAY.md).

Research checked 2026-09-28. The MDA link supports the conceptual model; the workflow and courier
examples here are original guidance, not experimental findings or a recipe that guarantees fun.

---
