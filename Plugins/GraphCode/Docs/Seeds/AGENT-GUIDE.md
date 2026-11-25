# <Project> — agent guide

*Read this first, every session. Keep it small: it is paid for on every single task.*

## Setup

*Written before anything else, ticked as each one is actually done. A box the person declined stays
unticked and says so on the line — an unexplained empty box sends the next reader hunting for work
that was already decided against. This block outlives setup: when GraphCode is upgraded and setup is
re-run, this is what says what was already settled.*

- [ ] Looked at the project — what is here already
- [ ] Involvement level agreed
- [ ] GraphCode customer access — status: not checked
- [ ] Who drives which domain — game direction, art, mechanics, sound
- [ ] Project questions answered — what we are making, how serious, how big
- [ ] Assets: where content may come from
- [ ] Quality bar agreed, and what may be spent on generation
- [ ] `.env` prepared and gitignored, if any generation service is used
- [ ] Sidecars: on or off
- [ ] Excluded folders decided
- [ ] `.gitattributes` written
- [ ] Build mode chosen
- [ ] Production targets and first slice recorded, if this is a vertical slice or production game
- [ ] Project docs written, one design file per domain
- [ ] Permissions and hooks written
- [ ] Pointer added to the root agent file
- [ ] First thing built, rendered and played
- [ ] Handed back with the board populated

For customer access, follow `<plugin>/SETUP.md` step 2a after involvement is agreed: check installed
status/help first. Record only status, date and a non-sensitive reason. Unsupported or deferred
access stays unticked with a reason; tick only after installed status confirms the chosen access.
Do not invent activation tools or a
portal. Keep contact details, verification codes, tokens and PC secrets out of this guide.

## What this project is

<One paragraph. Genre, what the player does minute to minute, and the one thing that has to be good.>

**Build mode: <prototype | vertical slice | production>.** <One line on what that means here — how
many levels, whether the world streams, whether the art is real yet.>

**Current milestone and production targets:** <link to the project's production note, if applicable>.
For a production game or polished slice, read `<plugin>/Docs/Guides/PRODUCTION.md` before planning
the next slice. Preserve the agreed fidelity; a temporary proxy does not close a visual milestone.

## The board

`<path to the board>` is the only place open work lives. Read it before picking anything up. Close a
row by deleting its line. Never re-derive a list of work from a design document.

## Where things are

| | |
|---|---|
| Levels | `<path>` |
| Player and core gameplay | `<path>` |
| UI | `<path>` |
| Shapes and blockout geometry | `<path>` |
| Imported content | `<path>` — and its licences are in `<asset log>` |
| Project docs | `<path>` |

## How I work here

**Involvement: <observer | collaborator | director | do it all>.** <What that means in practice — what
gets built without asking, and what stops for a yes.>

**Who drives what: game direction <them|me> · art direction <them|me> · mechanics <them|me> ·
sound <them|me>.** The domains they drive are the ones I show and ask about. The domains I drive are
the ones I decide and report — and every decision lands in that domain's file in `<docs>/Design/`.

**Review cadence: <after every build | at milestones | when unsure>.** Renders and play captures go to
<name> at that cadence. Placement, proportion and whether something feels right are theirs to judge,
not mine.

## Research and plan before implementation

For any unfamiliar or complex part of the work, first follow
`<plugin>/Docs/Guides/RESEARCH-AND-PLAN.md`: research current primary sources and working examples,
verify the installed capabilities, and save a detailed implementation plan before building.
This applies to every domain, including external tools. Keep findings, dependencies, ordered
steps and acceptance checks in the owning project note. Resolve each still-ambiguous step before
spending production effort on it; record the decision, evidence and success check. Continue already
authorized implementation after planning; a research-only or plan-only request stays within that scope.

## Making game content and presentation

**Before making game content or presentation, read `<plugin>/Docs/Guides/ASSET-QUALITY.md` and follow
it.** References first, a written plan, one representative part at final quality, then the rest,
compared with the reference after every revision. Quality, never speed: a quick shape that only
resembles the reference is a failed task. A blockout exists only when <name> asked for one, labelled
as such, with its replacement on the board. Armour is Garment and Shape together; hair starts from
the Groom guide's proven coat.

Aim beyond minimum detail: surprise <name> with purposeful craft and distinctive features that fit
the whole theme, unless they explicitly request simplicity. Straight edges and simple geometry,
including a bare sphere, must be deliberate design choices. Follow the quality guide's repeated
render-inspect-repair loop for every seam, connection and detail, then check the whole asset in context.
This applies equally to animation, VFX, UI, audio and other media: plan and verify the quality of
every relevant phase, state and transition through actual playback, listening or interaction.
For realistic surfaces, follow the same guide's imperfection and bake workflow: represent believable
irregularity through geometry, material+bake or decal+bake as appropriate, protecting functional
interfaces and verifying the actual baked appearance.

For Shape work, prepare dedicated part references where the whole-object concept leaves construction
or assembly unclear. Plan interfaces before modelling, and choose each `add` join for the connection;
do not use `join=none` by habit. The quality guide contains the part-sheet and join decision workflows.

## Making a game and its interface

**For game design or mechanics, read `<plugin>/Docs/Guides/DESIGNING.md`, then `GAMEPLAY.md` in
that directory. Before building game UI, read `<plugin>/Docs/Guides/UI-UX.md`.** Design the player
journey, choices, action feedback and recovery together. UI needs a coherent visual language,
complete states, supported-device navigation and a check in the running game. Use
`<plugin>/Docs/Guides/UI-TOOLS.md`: default to HTML/CSS first with generated artwork where useful,
including authorized image-generation APIs when needed, preserving an existing suitable workflow.
Other design apps are optional. An external prototype
still needs a verified implementation in the target game.

## Operating GraphCode

- **Read `<plugin>/GRAPHCODE.md` before any GraphCode work, and follow its layers.** Never rely on
  memory for GraphCode grammar, verbs or limits: the installed build is the authority.
- **Help is a parameter, not a tool.** `{help: true}` on the verb, then the parameter, then the value.
  Never guess a schema from memory.
- **`gc_apply` is every write, including creating an asset.** `gc_read` is every read — `saved` is the
  file on disk, `live` is the editor with unsaved edits. Say which one you mean.
- **`gc_describe kind:"<kind>"`** is the real grammar for a kind, and it answers without an editor.
- **Read before writing. Deletion needs explicit intent and a verified target** — never inferred from
  *"clean up"*, never widened with a wildcard.
- **`omits:` is the coverage contract.** Only apply canonical text with a current `sig`.
- **Verify by looking.** Re-read what you wrote, and render anything with a shape.

## When you need a subject, go and read it

*Routes only. Nothing on this page explains a subject — that is what keeps this page cheap, and it is
paid for on every single task. The plugin's guides are the plugin's; they upgrade when it does, so
they are linked, never copied.*

| When you need to | Read |
|---|---|
| check customer access, sign out or remove this PC | `<plugin>/SETUP.md`, step 2a — check installed status and help first |
| learn any unfamiliar or complex subject and plan its implementation before building | `<plugin>/Docs/Guides/RESEARCH-AND-PLAN.md` |
| make game content or presentation — models, animation, VFX, UI, audio and other media | `<plugin>/Docs/Guides/ASSET-QUALITY.md`, **first, every time** |
| arrive with no useful GraphCode context | `<plugin>/Docs/Guides/AI-COLD-START.md` |
| choose the owning product, source document or asset kind | `<plugin>/Docs/Guides/CAPABILITY-MAP.md` |
| start or expand a production game or polished slice | `<plugin>/Docs/Guides/PRODUCTION.md`, then the project's production targets |
| connect gameplay systems, save state or networked interactions | `<plugin>/Docs/Guides/GAMEPLAY.md` |
| design game UI, HUD, menus, focus, responsive layout and accessibility | `<plugin>/Docs/Guides/UI-UX.md` |
| use outside design/browser tools and bring the interface into Unreal | `<plugin>/Docs/Guides/UI-TOOLS.md` |
| learn from one complete design-to-playable example | `<plugin>/Docs/Guides/GAME-SLICE-EXAMPLE.md` |
| make an AI character choose a destination, detour or recover | `<plugin>/Docs/Guides/AI-NAVIGATION.md` |
| verify a milestone, profile performance or hand over a package | `<plugin>/Docs/Guides/VALIDATION.md` |
| know where an asset may come from, or generate one | `<plugin>/Docs/Guides/CONTENT.md` — then log it in `<asset log>` |
| change a GraphCode setting, sidecars, excluded folders | `<plugin>/Docs/Guides/SETTINGS.md` |
| touch git, `.gitattributes`, or a conflicted file | `<plugin>/Docs/Guides/GIT.md` |
| know how to design with these tools rather than around them | `<plugin>/Docs/Guides/DESIGNING.md` |
| work while somebody else is working here | `<plugin>/Docs/Guides/TEAM.md` |
| choose the right verb and verify a write | `<plugin>/Docs/Guides/TOOLS.md`, then `{help:true}` on the verb |
| work with Shape, World, Move, Play, Rig, Groom, Garment or Human | `<plugin>/Docs/Guides/AUTHORING.md`, then the focused guide for that product |
| generate a model by part and combine prepared anatomical connections | `<plugin>/Docs/Guides/ORGANIC-PART-ASSEMBLY.md` |
| combine Human or MetaHuman identity, Rig, body motion and facial performance | `<plugin>/Docs/Guides/CHARACTERS.md` |
| fit clothing or armour, or choose render-only versus simulated garment construction | `<plugin>/Docs/Guides/GARMENT.md` |
| make hair | `<plugin>/Docs/Guides/GROOM.md` |
| diagnose a recurring authoring, visual, runtime or persistence failure | `<plugin>/Docs/Guides/LESSONS.md` |
| learn a grammar, a kind, or a native type | `gc_describe` for supported kinds and programs; the Groom guide covers the current `.gcgroom` help gap |
| decide anything about the look, the loop, a mechanic or the sound | `<docs>/Design/<the one domain>.md` — and nowhere else |
| show them what something looks like before deciding | `<docs>/Design/MoodBoards/` — images live there, never in `Content/` |
| <a subject only this project has> | `<path>` |

## What may be spent

**Quality bar: every visible asset to `<plugin>/Docs/Guides/ASSET-QUALITY.md`, scaled to how close the
player gets.** <One line on what that means here. Blockouts only on request, labelled, with a
replacement task.>

**Generation services: <none | Meshy 3D · ElevenLabs API audio · 3D AI Studio 2D — and free or paid
tier>.** A generation costs <name>'s money and the free and paid tiers usually carry different
licences, so it is theirs to authorise. Never fan out variations nobody asked for.

**Keys: `.env` at the project root, gitignored, filled in by <name>, not me.** A blank line means no
key yet — stop and ask rather than guessing or switching service. I never type, paste or read a key.

## Decisions that are already made

<A short list, each one line: what was decided and why. This is the section that saves the most time —
it is where "we tried that and it did not work" lives. Mark a reversed decision as superseded rather
than deleting it.>

- <decision> — <why>
