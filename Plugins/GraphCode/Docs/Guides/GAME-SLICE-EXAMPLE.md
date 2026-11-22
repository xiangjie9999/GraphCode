# Worked example: one delivery that feels like a game

This is an instructional design, not a shipped demo, measured playtest or commitment for a
customer's game. Names, timings and rules below are example choices. Reuse the method and replace
the choices with that project's direction. Exact GraphCode syntax comes from installed help.

The purpose is to connect [design](DESIGNING.md), [mechanics](GAMEPLAY.md), [UI](UI-UX.md), art,
audio and validation. A pile of correct isolated assets does not establish that connection.

## The experience and its decision

The player is a small courier robot carrying a fragile parcel through one city courtyard. They
should feel nimble, understand danger before committing, and want another attempt after failure.
There are two routes: a longer sheltered walkway and a shorter path past a cycling steam vent.
The delivery pays a fixed reward; a clearly advertised time bonus makes the shortcut tempting.
The ordinary route remains viable. The player chooses whether the risk is worth the time saved.

The first slice contains one contract, one parcel, two route choices, one vent and one recipient.
It includes the contract menu, movement/camera, pickup, warning feedback, delivery, failure, retry,
pause/settings, and one saved completion record. No city expansion is needed to assess this loop.
Use finished representative art for a polished slice. A blockout is a separate, explicitly
authorized mechanics experiment and leaves the visual gate open.

## Write the mechanic before writing assets

| Question | Example decision |
|---|---|
| What does Accept do? | Starts one delivery attempt with a stable attempt identifier. It cannot create a second while one is active. |
| How is the parcel picked up? | An interaction action within valid reach, with an unobstructed target. Commit only after rechecking it. |
| What creates risk? | Entering the vent's damaging state while carrying the parcel reduces its integrity. A visual and audible warning precedes the burst. |
| What determines success? | Interacting with the recipient while the current parcel is carried and integrity is above zero. |
| What determines failure? | Integrity reaches zero before delivery. No completion reward is issued. |
| What resets? | Retry creates a fresh attempt, restores parcel/robot state and clears old prompts, timers and effects. |
| What persists? | Completed contract identifier, best result and awarded currency; no active-attempt resume in this example. |
| What happens on leaving? | Leaving an active attempt abandons it; explain this before the player commits to leaving. |

Example state sequence:

```text
Available -> Accepted -> Carrying -> Delivered
                           |
                         Failed -> Retry -> Accepted (new attempt)
```

Pause is an overlay on an attempt in this single-player example, not a second reward state.
Specify whether simulation time stops; here both the vent and bonus clock pause together.
Do not accidentally pause one and keep the other running. Quit and reload return to contract
selection with the persisted completed result, not a half-restored parcel.

Define terminal-event precedence too: if damage and delivery happen during the same update,
process qualifying damage first in this example, then revalidate delivery. A failed attempt
cannot also deliver. Implement one serialized owner for this decision and test the tie case.
Other games can choose another rule; leaving it to callback order is not a design.

## Assign ownership and connect the presentation

| Owner | Responsibility | Presentation observes |
|---|---|---|
| Attempt state | Attempt identity, phase, bonus clock, completion and single reward commitment | Objective, result screen and remaining bonus time. |
| Parcel state | Carried target and integrity for this attempt | Parcel appearance, integrity display and damage feedback. |
| Hazard | Warning/burst/recovery phase and valid damage events | Vent animation, sound, warning marker and effects. |
| Persistent progress | Completed contracts, best result and currency | Contract selection and saved completion feedback. |
| UI | Selection, focus, visible screen and local presentation state | Requests gameplay actions; does not independently grant rewards. |

Choose classes from the existing architecture after assigning these responsibilities. A small
implementation need not create five new managers. If an attempt owns the parcel values, keep
them there; the table separates responsibilities, not mandatory classes.

Author shared hazard behaviour plus tunable data. A second vent with a different cycle should
not require a copied logic graph. Define damage once per valid burst encounter rather than
accidentally once per overlap callback or rendered frame.

## Build the UI that makes the loop understandable

| Moment | Information and action | Behaviour to verify |
|---|---|---|
| Title | New delivery; Settings; Continue only if its meaning is implemented | Focus starts on the intended action. No dead button. |
| Contract | Recipient, reward, fragile cargo, bonus condition; Accept | Real state controls availability; repeat activation creates one attempt. |
| Courtyard | Current objective, parcel integrity, bonus condition; contextual interaction | Prompts reflect current mappings and reachable targets. |
| Vent warning | Danger source and upcoming burst | Readable over the real scene and understandable without sound alone. |
| Pause | Resume, Settings, Leave attempt | Gameplay input is blocked; clock behaviour matches the design. |
| Failure | Why the attempt ended; Retry; Contract list | Focus on Retry, no duplicate reward, clean reset. |
| Delivery | Reward breakdown, save state, Next/Contract list | Values come from the committed result; save failure remains recoverable. |

Make one contract screen and one HUD state at final visual quality using [UI-UX.md](UI-UX.md).
Use the game's art direction for type, illustration and motion. Keep the route readable through
the HUD, and avoid covering the vent with the objective panel. A warning needs a readable shape
and timing, not just a red flash.

Prototype Contract -> HUD -> Pause -> Result in HTML/CSS using [UI-TOOLS.md](UI-TOOLS.md), unless
an existing suitable workflow already covers it. Generate artwork where it improves the chosen
direction. Create normal, long-text, unavailable and failed states. Bring the design back to real
widgets; a browser screen changing a sample number has not delivered this mechanic.

## Tune relationships, then play them

Start with a measurable question: does the shortcut create an understandable risk/reward choice?
For an illustrative baseline, suppose the safe route takes 18 seconds and the shortcut takes
12 seconds at ordinary movement speed. A bonus threshold between them could reward the shortcut,
but pickup, hesitation and delivery time must be included before selecting it. These are
hypotheses, not measured route times or universal recommendations.

Measure the real traversal with the real camera and collision. Try waiting for the vent, rushing,
turning back, and carrying damaged cargo. If everyone takes the safe route, inspect whether the
bonus is visible, valuable and achievable before increasing it. If everyone takes the shortcut,
inspect warning, damage and timing. Changing all of them at once makes the result hard to explain.

The world teaches the choice: show the destination and steam before the fork, provide a safe
place to watch a cycle, and make the sheltered path visually distinct. Add an interaction prompt
at the point of use. Observe a new player's first attempt before adding more tutorial text.

Give the action a complete feedback sequence: input acknowledgement, pickup motion and sound,
carried silhouette, vent anticipation, impact response, integrity update, and delivery release.
Check sync at the gameplay event and clean cancellation. Extra camera shake does not fix an
unclear hazard or a delayed interaction.

## The build order and proof

1. Confirm the required input, widget, state, motion and save authoring paths in the installed
   tools. Record any genuine gap and prove the chosen fallback on a disposable example.
2. Make Accept -> Pickup -> Delivery -> Result -> New attempt work, with real ownership and UI.
3. Add hazard, damage, failure and retry; include action interruption and the tie case.
4. Finish representative surroundings, motion, sound and UI together at the gameplay camera.
5. Add the promised saved result, settings and launch/menu paths. Package the small slice early.
6. Observe first-time play, fix the largest issue, repeat the affected scenarios and assess
   whether the intended experience is present before adding another contract or street.

| Scenario | Expected observation |
|---|---|
| Normal delivery, then repeated activation | One result and one reward for the attempt. |
| Parcel disappears or becomes unreachable during interaction | No false pickup or stale carried state; understandable feedback. |
| Several damage callbacks in one burst | Only the damage allowed by the rule, never frame-rate-dependent multiplication. |
| Damage and delivery coincide | The declared precedence produces one terminal outcome. |
| Pause/resume during warning, and Back during a UI transition | Intended timers, effects, focus and input recover correctly. |
| Fail, retry, deliver, return and retry again | No stale parcel, subscription, timer or result overlay. |
| Save completed result, quit application and reload | One persisted completion/reward; no resumed active attempt. |
| Simulated save failure using disposable data | Honest save feedback and safe retry without granting a second reward. |
| Controller-only route, larger text and a narrow supported view | Every required action reachable and all important information readable. |
| Packaged first launch and repeat launch | Real startup route and content work without the editor. |

Record expected versus observed results, capture locations and configuration following
[VALIDATION.md](VALIDATION.md). Ask the playtester what they thought caused failure and what
they wanted to try next. Correct state is one result; understandable and appealing play is
another. Do not label either as proven without its corresponding evidence.
