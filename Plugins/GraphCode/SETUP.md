# Set up GraphCode in this project

*Customer setup seed. This file ships with the plugin and is read in the customer's project.*

`setup-revision: 16`

<sub>That number changes only when this document changes in a way that makes a project's existing docs
wrong — a new product, a renamed verb, a changed default. A project records the revision it was set up
against, and the tools say one line, once, when the two disagree (§9).</sub>

**You are an AI standing in someone's Unreal project.** They installed GraphCode and said something
like *"set GraphCode up for me."* This file is the only one you have to read to start. It teaches you
how to operate the tools, then walks you through a short setup, at the end of which **this project is
configured, has its own documentation written by you, and has one thing in it you have built, looked
at and played.**

One rule above the rest: **you do not stamp out a fixed set of files.** What you write depends on what
you find here and on what the person tells you. This file gives you the method, the guides and our
leanings. The output is yours.

**For unfamiliar or complex work in any area, follow
[RESEARCH-AND-PLAN.md](Docs/Guides/RESEARCH-AND-PLAN.md).** Research the subject thoroughly using
current primary sources and working examples, verify installed capabilities, and write a detailed
implementation plan before building. Carry this universal rule and its generic plan template
route into the project's agent guide. Resolve each still-ambiguous step and record its approach
and success check before spending production effort on it. Continue already authorized
implementation after planning.

For anything a player will see or hear, carry the [asset quality guide](Docs/Guides/ASSET-QUALITY.md) into
the project's agent instructions: exceed minimum detail with purposeful craft that fits the theme (unless
the owner asks for simplicity), choose each Shape `add` join deliberately (`join=none` is not a default),
and render, inspect, repair and re-render every seam and detail. Verify animation, VFX, UI, audio and
cinematics the same way, by repeated playback, listening and interaction.

**If the task is a production game or polished vertical slice, read
[the production workflow](Docs/Guides/PRODUCTION.md) before choosing the first build.** Preserve
the customer's quality bar and existing decisions. Setup creates a route to a shippable game;
finishing setup does not establish that the game is ready to ship.

**For game design or mechanics, read [DESIGNING.md](Docs/Guides/DESIGNING.md), then
[GAMEPLAY.md](Docs/Guides/GAMEPLAY.md). Before building game UI, read
[UI-UX.md](Docs/Guides/UI-UX.md).** Teach the project's agent to design the whole player journey
and verify its interface in play. [UI-TOOLS.md](Docs/Guides/UI-TOOLS.md) covers outside design and
browser tools: generate a complete UI mockup and palette from the game's references through ImageGen
or authorized online services, refine it, then generate its transparent elements. Assemble live
HTML/CSS interaction prototypes, then complete the Unreal handoff.
Preserve an existing suitable workflow. Carry these task routes into its agent guide.

If this is your first GraphCode task rather than a request to configure the project, start with
[the AI cold-start route](Docs/Guides/AI-COLD-START.md). It teaches the smallest safe loop and sends
you here only when project setup is actually needed.

---

# Part A — before you ask anything

## 1. What GraphCode is, in one paragraph

Unreal's content is binary. GraphCode writes deterministic **text** beside it — a `.gc` sidecar per
asset — and that text can be read, searched, diffed, reviewed, merged and **written back**. That is
the whole idea: you author a game by writing and reading text, and the editor is where it lands, not
where you work.

## 2. What is installed, and what each part is for

GraphCode 0.1.15 supports Unreal Engine 5.7 and 5.8 (one download per engine). It is a product line.
**Core is required; the rest are optional and may not be here.** Core, Shape and Integrate are free for
personal use; the trial of every product lasts 21 days. What each product can and cannot do, with the
workaround for each gap, is one page: [CAPABILITY-MAP.md](Docs/Guides/CAPABILITY-MAP.md). Ask
`unreal_info` for the installed environment and `gc_describe` for the kind you intend to use. Never
promise a capability from this table without checking the installed package and its tool help.

| Product | Reach for it when you need to | Learn it by asking |
|---|---|---|
| **GraphCode** (Core, required) | author ordinary assets — Blueprints (including Gameplay Ability System assets), widgets, materials, data tables, input, structs and enums, the level and the actors in it | `gc_describe kind:` lists every kind it covers; `{help:true}` on each verb |
| **GraphCode Shape** | make geometry from nothing. A `.gcshape` declares the intent — a wall, a column, a stair — and builds a real mesh asset from it. This is how you block out a level without sourcing a single model. Sculpted organic forms (creatures, characters) are not its strength: generate and clean them instead (step 2b) | the `.gcshape` grammar through `gc_describe`; `gc_apply` builds it |
| **GraphCode World** | levels at scale — the places somebody named, terrain, water, foliage, PCG, navigation. Owns the `.gcw` region file beside a level and the `.gcs` spatial file | `gcw_ask {help:true}` for spatial questions; `gc_describe` for the world kinds |
| **GraphCode Move** | anything that happens over time — animation, state machines, sequences, Niagara, and the AI graphs that decide and drive motion | `gc_describe kind:` for its kinds |
| **GraphCode Play** | the game while it is running. Capture the live world to a `.gcp`, start and drive a session, watch what it actually did. **This is how you play-test what you built** | `gc_play {help:true}` |
| **GraphCode Core Extension** (formerly Advanced) | framework-backed assets — MVVM, MetaSound, CommonUI, EQS and MetaHuman identity through `.gchuman` | `gc_describe` for the installed kind or human program; the [authoring guide](Docs/Guides/AUTHORING.md) for workflow |
| **GraphCode Rig** | describe joints, topology and skinning intent over a Shape or imported mesh with `.gcrig` | `gc_describe` for the installed rig grammar; `gc_render` for joint/mesh proof |
| **GraphCode Groom** | author native grooms and bindings from `.gcgroom`, with regions, flow and hairstyle constraints | the [Groom guide](Docs/Guides/GROOM.md) for the recipe; `gc_apply {help:true}` for the write; `gc_render` for appearance |
| **GraphCode Garment** | fit authored clothing to a body and build the supported garment or mesh result from `.gcgarment` | the [Garment guide](Docs/Guides/GARMENT.md) for ownership and verification; installed help for the exact program |
| **GraphCode Integrate** | import Blender static meshes and materials, including Blendkit sources, and manage reimports | [Blender setup](Docs/Guides/BLENDER-SETUP.md), then the [import workflow](Docs/Guides/AUTHORING.md#integrate-bring-an-external-source-across-a-boundary) |

So: **geometry is Shape, level placement is Core, spatial authoring is World, motion and AI are
Move, play-testing is Play, Rig fits
a skeleton to a mesh, Groom grows and styles hair, Garment fits clothing, and Human lives in Core Extension.** A level document
and ordinary assets are Core; World adds spatial and environmental authoring. Ask the installed
tools which parts of each product are available before making a promise. The [authoring
guide](Docs/Guides/AUTHORING.md) explains the workflow and visual checks for each product.

**For Blender imports, use Integrate** for static meshes, their materials and managed reimport; assembly
belongs in GraphCode, never in Blender ([organic part assembly](Docs/Guides/ORGANIC-PART-ASSEMBLY.md)).
The AI does the downloads, installation, configuration and verification itself with
[Blender setup](Docs/Guides/BLENDER-SETUP.md); handing the user a ZIP or a menu checklist does not
complete the task. Carry that link and the [import route](Docs/Guides/AUTHORING.md#integrate-bring-an-external-source-across-a-boundary)
into the project's agent guide, BlenderKit sources included.

For native Landscape brushes, see [the native terrain guide](Docs/Guides/NATIVE-TERRAIN.md).
Water, PCG, MetaHuman (including Chaos Cloth and Chaos Outfit), MVVM and MetaSound are optional
integrations. Do not enable them all during setup. Ask which workflow the project needs and
enable only its framework through Edit > Plugins, accepting Unreal's dependency prompts.
Restart after a plugin change. The binary bundle already contains the adapters; no source or
GraphCode rebuild is required. A feature used while its framework is off reports what to enable.
If the chosen workflow uses Water, also check its collision profile as described in the
[native terrain guide](Docs/Guides/NATIVE-TERRAIN.md#water-project-setup); enabling the plugin
alone does not add that project setting.
Enable the Unreal plugin supplying the chosen brush only when that workflow needs it. The guide
distinguishes verified GraphCode binding from experimental erosion and remaining quality tests.

## 3. The verbs, by the question each one answers

The names below are canonical — a bridge may prefix them (`unreal_gc_read`) or fold two into one.

| You want to | Call |
|---|---|
| See an asset as text | `gc_read` — `saved` is the file on disk, `live` is the editor with unsaved edits |
| Write an asset, or create one | `gc_apply` — **every write, including creation, is this one verb** |
| Learn a grammar, a type, or an asset's shape without an editor | `gc_describe` |
| Find something in the project | `gc_search` — one surface, several kinds of question |
| Look at what you made | `gc_render` — appearance, widgets and graph layout through its own modes |
| Make a sidecar exist and be current | `gc_sync` |
| Bring in a file from outside | `gc_import` |
| Trigger something the document names, live | `gc_invoke` |
| Rename, move or copy the package itself | `gc_asset` |
| Show a level to the person | `gc_open` |
| Go back, compare versions, resolve a merge | `gc_versions` |
| Run the game and drive it | `gc_play` |
| Ask a spatial question about the world | `gcw_ask` |
| Ask about the environment itself | `unreal_info` |

The [tool workflow guide](Docs/Guides/TOOLS.md) explains when to choose each verb and what evidence
finishes its job. The tool's own `{help:true}` remains the authority for parameters and modes.

## 4. Never guess a schema — help is a parameter

Help is not a separate tool. It is an argument on the verb, and it walks a tree: the tool, then a
parameter, then a value. No editor needed.

```json
{ "help": true }                                  // the verb: one line per parameter
{ "help": "kind" }                                // what that parameter takes
{ "help": "kind", "kind": "blueprint" }           // what that value does, and what it brings
{ "help": ["mode", "kind"], "expanded": true }    // several branches, full depth
```

If you catch yourself recalling a parameter name from memory, stop and ask the tool instead. A
retired name still answers — with its replacement.

## 5. What a `.gc` looks like

```gc
gc 1  kind:blueprint  asset:/Game/BP_Example  sig:<digest>  bin:<digest>
parent: /Script/Engine.Actor
omits: <declared-uncovered-facets>
properties {
  Health: float = 100.0 [instance_editable]
}
graph {
  eventgraph EventGraph { n1 = Event ReceiveBeginPlay() : ->_ }
}
```

`gc_describe kind:"<kind>"` gives you the real grammar for any kind, including how that kind decides
what counts as the same thing and what deleting means for it. Every kind is different; do not
generalise from one.

## 6. The rules that never change

These hold in every project, on every job, at every autonomy level. Carry them into whatever docs you
write.

- **Research unfamiliar or complex work and plan it before building.** Follow
  [RESEARCH-AND-PLAN.md](Docs/Guides/RESEARCH-AND-PLAN.md) across all systems. Keep the detailed
  findings, approach, dependencies, stages, risks and acceptance checks in the owning project note.
  Resolve each ambiguous step before production effort; use a defined small experiment when
  research alone cannot answer it, and update the plan before dependent work.
- **Read the file kind's entry guide before the first line of it.** Before writing a `.gcmove`, `.gcrig` or `.gcshape`,
  open [GCMOVE.md](Docs/Guides/GCMOVE.md), [GCRIG.md](Docs/Guides/GCRIG.md) or [GCSHAPE.md](Docs/Guides/GCSHAPE.md) and
  read every guide and shipped example it lists for what the file does (a grip, a strike, planted feet, a held prop).
  The guides hold methods that were paid for with failed builds; writing from memory repeats the failures.
- **Never skip a step a guide states because the rig or tool lacks something.** A missing bone, contact, socket or
  range is added in the `.gcrig` ([GCRIG.md](Docs/Guides/GCRIG.md)); a fallback that drops the step is not a fix. If a
  tool refuses what the method needs, say so to the owner with the refusal's text instead of working round it.
- **Read before you write.** Project the asset first. Use dry-run or impact modes where they exist.
- **Deletion needs explicit intent and a verified target.** Never infer it from *"clean up"* or
  *"simplify"*, and never widen it with a wildcard.
- **`omits:` is the coverage contract.** Apply only canonical text carrying a current `sig`. Never
  apply back something a diagnostic read handed you with `sig:none` or `non_canonical`.
- **Omission means different things in different blocks.** Some blocks are whole values that a
  shorter payload truncates; some delete only through an explicit line. Read the block's own help
  before writing a whole block.
- **`saved` and `live` are not the same project.** Say which one you mean, every time.
- **Verify by looking.** A write is not done because the call returned. Re-read it, and for anything
  with a shape, render it. Text can be right and the thing still be wrong.
- **Build on a new, disposable design before touching an owner's asset.** Prove save, reload and
  runtime behaviour where the claim depends on them. A green parser result does not prove a visible
  or playable result.
- **Quality over speed, for anything a player will see.** Before making an asset, read
  [ASSET-QUALITY.md](Docs/Guides/ASSET-QUALITY.md) and follow it: references first, a written plan,
  one representative part at final quality, then the rest, compared with the reference after every
  revision. Never ship a quick shape that only resembles the reference. A blockout exists only when
  the owner asked for one, labelled as such, with a task to replace it.

---

# Part B — the setup run

Nine main steps, with GraphCode activation at step 2a and the capability briefing at 2b. Steps 1 and 2 always happen. How heavy
the rest are depends on the answer to step 2.

**How to ask, throughout: one question at a time.** Where your harness has a native multiple-choice
question control — Claude Code does — use it, with the options lettered and a free-text escape. Where
it does not, one question per message in plain text. Never open with a wall of ten questions; people
answer the first three and guess at the rest.

## Step 0 — Write the checklist before you write anything else

**The very first thing you put on disk is a setup checklist, and you tick it as you go.** Setup is a
conversation that can be interrupted — the person walks away, the session ends, a week passes, someone
else picks it up. Without a checklist, a half-finished setup looks exactly like a finished one, and
nobody finds the thing that was never done until it bites.

Put it at the top of the project's own routing doc, where it is read at the start of every session.
`Docs/Seeds/AGENT-GUIDE.md` ships with this block in it:

```markdown
## Setup

- [ ] Looked at the project — what is here already
- [ ] Involvement level agreed
- [ ] GraphCode activated on this PC — status: not checked
- [ ] Told what GraphCode can and cannot do (CAPABILITY-MAP.md)
- [ ] Project questions answered — what we are making, how serious, how big
- [ ] Assets: where content may come from
- [ ] Sidecars: on or off
- [ ] Excluded folders decided
- [ ] `.gitattributes` written
- [ ] Build mode chosen
- [ ] Production targets and first slice recorded, if applicable
- [ ] Project docs written
- [ ] Permissions and hooks written
- [ ] Pointer added to the root agent file
- [ ] First thing built, rendered and played
- [ ] Handed back with the board populated
- [ ] Setup stamp written to `Config/GraphCodeSetup.ini`
```

Three rules that make it worth having:

- **Tick a box when it is done, not when it is started.** A ticked box is a claim someone will rely on
  without re-checking.
- **A box the person declined is not a box you forgot.** Leave it unticked and say so on the line —
  `- [ ] .gitattributes — declined 2026-08-24, not a git repository`. An unexplained empty box sends
  the next reader hunting for work that was already decided against.
- **The checklist outlives setup.** Do not delete it when you finish. When someone re-runs setup after
  a plugin upgrade (§8), this is the file that says what was already settled and what never was.

On rung (a), propose the checklist like anything else rather than writing it unasked.

## Step 1 — Look first, and never ask what the project already says

Before a single question, read the project. At minimum:

- What is in `Content/` — is this blank, or is there a game here already?
- Which plugins are enabled, and which GraphCode products are among them (`info`).
- Any existing agent docs (`CLAUDE.md`, `AGENTS.md`, a `Docs/` folder), and any `.gc` sidecars.
- Git: is this a repository, is there history, is anyone else committing to it, and is `Content/`
  ignored?
- The default map, and the project settings that follow from it.

**Whatever the project already answers, do not ask about.** A project whose `Content/` holds a
half-built third-person shooter should never be asked what genre it is. Say what you found, and ask
them to correct you if you read it wrong.

## Step 2 — Ask how involved they want to be. This one question forks everything.

Ask it first, and ask it always, even when everything else was inferable. It sets both how deep the
rest of setup goes and how much you may do without asking, from now on.

> **How involved do you want to be while I work?**
> **(a)** I approve everything — you propose, I write.
> **(b)** You draft, I approve each thing, and we talk through the art together.
> **(c)** You decide and build; show me at milestones.
> **(d)** Just do it all — pick sensible defaults and go.

| Rung | Setup depth | What you may do afterwards without asking |
|---|---|---|
| **(a) Observer** | Every doc proposed, nothing written unapproved | Write `.gc` text only; never touch the editor |
| **(b) Collaborator** | You draft, they approve each doc; art discussed | Build; ask before the editor and anything irreversible |
| **(c) Director** | You decide and draft; they review at milestones | Build and apply; check in at milestones |
| **(d) Do it all** | You infer, fill from house style, confirm once | Everything inside §6's rules |

**If nobody ever answers — a non-interactive run, or they walked away — assume (b).** It builds, so
the setup completes, and it stops at every door that cannot be undone.

If they answer with a sentence rather than a letter, take the sentence. *"I want to talk through the
art carefully and see pictures"* is (b), whatever letter they typed.

### The rung is not one answer for the whole game

Ask this immediately after, because people are rarely at the same rung across the whole project.
*"You do the art, I decide how it plays"* is one of the most common answers there is, and a single
rung cannot say it.

> **Which of these do you want to drive, and which should I?**
> **Game direction** — what this is, the loop, the scope, what gets built next.
> **Art direction** — the look, the palette, the mood, what a room feels like.
> **Mechanics and detail** — the systems, the numbers, how a thing actually works.
> **Sound** — the effects, the music, and what the silence is doing.
>
> *[default: the rung above, applied to all four]*

**The domain they own is the domain you show and ask. The domain you own is the domain you decide and
report.** It changes what you do hour to hour: someone who owns art direction wants a render before
you place the second wall; someone who handed it to you does not want to be asked about a palette
again. Someone who owns mechanics wants the damage numbers in front of them; someone who does not
wants to be told what you chose, once.

Record all four, even the ones they hand over — *"you decide"* is an answer worth writing down, and
in a month it is the only thing that says why nobody was asked. Each domain gets its own document in
step 6, which is where the decision goes once it is made, so it is made once.

## Step 2a — Activate GraphCode on this PC

GraphCode 0.1.15 runs on Unreal Engine 5.7 and 5.8. Core, Shape and Integrate are free for personal use;
the trial of every product lasts 21 days. Before it writes, `unreal_gc_apply` checks one file for this
Windows user: `%USERPROFILE%\.graphcode\activation.txt`, outside every project (Apply creates it when it
is missing). Not under `AppData`: Windows gives a packaged AI app its own private copy of `AppData`, which
the editor cannot see. Until it is filled in, Apply does not run, and its reply names the next step. You
fill it in for the person:

**A licence comes with this file already filled in.** Ask the person for it first — it is usually in
their downloads as `graphcode-activation-<their-email>.txt`, sent with the GraphCode folder. Save it as
`%USERPROFILE%\.graphcode\activation.txt`, replacing whatever is there, and activation is done: there is
nothing to type and nothing to guess. Then run the Apply again.

Only if they cannot find it, fill the lines in yourself. **They come from the licence or the person, and
none of them can be worked out** — never guess any from OS accounts, repositories or other files:

1. **Email.** The address the licence was sold to, or the one for the trial: `email = <their email>`.
2. **Activation code.** A paid licence's own code, four groups of four like `XXXX-XXXX-XXXX-XXXX`:
   `code = <their code>`. It is used once, to register this PC. **A trial starts with the line empty:**
   the first Apply is held while GraphCode emails a code to the address (from `noreply@graphcode.dev`);
   the person gives you that code, you put it in the `code =` line, and the trial starts. It works once,
   on this PC, for 15 minutes; if it ran out, run the Apply again for a new one.
3. **Licence server.** The address the licence came with: `server = <https address>`. **This is the one an
   AI most often leaves out, and it cannot be derived from anything on the machine.** Without it Apply
   refuses with "there is no licence server to ask", however correct the other lines are. For a trial it
   is `https://rlzbzhcrwvdxoriwgajn.supabase.co/functions/v1/gc-license-device`.
4. **Run the Apply again.** Apply's reply is the only explanation GraphCode gives for an activation
   refusal; follow what it says.

In the checklist, record `activated` and the date. Keep the email and code out of project docs and
commits: the file belongs to this Windows user, not to the project.

**Activation happens once.** The first Apply after the file is filled in activates this PC online; from
then on GraphCode works offline and sends nothing over the network. A paid licence stays active; a trial
ends 21 days after it starts, and then Core, Shape and Integrate keep working free for personal use while
the other products need a licence. One trial per email and one per PC.

**One PC at a time. To move the licence to another PC,** deactivate it on this one first — only at the
person's explicit word, because this PC then stops writing: add the line `deactivate = yes` to
`activation.txt` and run any Apply. That Apply does not run; it signs this PC out (it needs the network
once), deletes this PC's GraphCode key so that activation can never write again, and its reply says
whether the licence was freed. Then activate the other PC with the same lines. While `deactivate = yes`
stays, this PC writes nothing and never re-activates by itself; removing the line activates it again,
which needs the licence to be free. If the old PC cannot be reached (dead, lost, sold), the licence's
activation code, or a one-time code GraphCode mails to the licence's email, moves it to the new PC
instead. Never delete identity data to reset a trial; if a refusal is unclear, use the support route it
names, or write to admin@graphcode.dev.

## Step 2b — Say what GraphCode can and cannot do

Before the questions, give the person the honest shape of the product, in a few lines of their own
project's terms, from [CAPABILITY-MAP.md](Docs/Guides/CAPABILITY-MAP.md) — the one page for what each
product makes, what it does not do, and the workaround. Read it now; do not summarise it from memory.
The gaps people meet most:

- **Organic objects** (creatures, characters, sculpted organic forms) are not Shape's strength. The
  workaround: generate the mesh with an AI image or text-to-3D model, then clean it with the Market guide
  **GraphCode/CleanGeneratedMesh**, rig it with `.gcrig` and animate it with `.gcmove`.
- **Textures, voices and music** come from outside (an image generator, ElevenLabs, recordings); GraphCode
  imports and wires them.
- **Blender** sends static meshes and materials; skeletons and animation are rebuilt or imported another way.
- **Caves and overhangs** are geometry, not terrain height.
- **Whether a game is fun** is theirs to judge; Play supplies the evidence.

Record any gap that matters to this project in the design docs of step 6, with its workaround, so nobody
rediscovers it.


## Step 3 — Ask the gaps, one at a time

Take the list below, **delete every line step 1 already answered**, and ask what is left, one question
per turn, each carrying a default so *"whatever you think"* is a complete answer. On rung (d), ask
nothing here — state what you assumed and move on.

The defaults below fill missing answers only. “Production grade,” “realistic,” or a supplied art
direction overrides prototype and placeholder defaults. If budget or asset access cannot meet
that request, name the constraint and build a smaller representative slice; do not silently
lower the fidelity. For a vertical slice or production game, also establish the targets in
[PRODUCTION.md](Docs/Guides/PRODUCTION.md): hardware, input, performance, persistence and any
network model. Record provisional answers without presenting them as tested support.

**About the project:**

1. **What are we making?** One sentence — the genre, and what the player does minute to minute.
2. **How serious is this build?** Prototype to feel the idea / vertical slice to show people /
   production you intend to ship. *[default: prototype]*
3. **What is the smallest version of it that would be fun?** That is what I build first.
4. **What should it look like?** Name a game or a look — or say *blockout* and I will build it in grey
   shapes and we will decide later. *[default: blockout]*
5. **How big is the playable space?** One room / one level / bigger than a level.
   *[default: one level]*
6. **What are you targeting?** PC / console / mobile / VR — it changes input, lighting and budgets.
   *[default: PC]*
7. **Anything already decided that I should keep?** An existing character, a plugin you rely on, a
   system you want used.

**About content:**

8. **Have you got art and audio already, or should I make it?** *[default: I make it, to the bar in
   [ASSET-QUALITY.md](Docs/Guides/ASSET-QUALITY.md); labelled blockouts only for a mechanics test you ask for]*
9. **Where may I get assets from?** I generate geometry with Shapes by default. Beyond that, tell me
   which of the sources in [`Docs/Guides/CONTENT.md`](Docs/Guides/CONTENT.md) you want used — some are free and clearly licensed, some are not, and
   the licence follows the asset into your project. *[default: Shapes and engine content only]*
9b. **How good does it have to be, and what are you paying for?** Anything a player will see is built
   to [ASSET-QUALITY.md](Docs/Guides/ASSET-QUALITY.md); what changes per asset is how much of it
   there is — a hero character gets close-up detail everywhere, a background crate gets what reads
   at its distance. A throwaway blockout for a mechanics test is also possible, labelled as one.
   And tell me which generation services you have accounts on and whether they are free or paid
   tiers: that decides both what I can ask for and what licence the output carries. For custom
   project audio, the preferred service is the ElevenLabs API; record its available tier and budget
   rather than choosing another generator silently.
   *[default: full quality to ASSET-QUALITY.md, scaled to viewing distance; ElevenLabs for custom
   audio when authorized; free tiers only; ask before paid generation]*

9b is a standing answer, not a one-off. Write it into the project's docs and follow it: the quality
bar decides how much you build before you show them, and **a generation spends their money, so it is
theirs to authorise.** Never burn credits on variations nobody asked for. If they ask for a blockout
to test a mechanic, build a labelled grey box and put its replacement on the board: it answers the
mechanics question and never counts as finished art. Fast and approximate is not a quality level.

**About configuration** — the answers are theirs, but every one of these defaults to **yes**, and
[`Docs/Guides/SETTINGS.md`](Docs/Guides/SETTINGS.md) is the guide behind them:

10. **Keep `.gc` sidecars on?** They are on out of the box, and everything gets one unless you exclude
    a folder. *[default: yes]*
11. **Any folders that should not get sidecars?** Vendored or marketplace packs you will never edit
    are the usual answer. Nothing is excluded out of the box. *[default: none yet]*
12. **Shall I set up `.gitattributes` for this repository?** It stops git from line-merging your
    content into nonsense. *[default: yes]*

**About working together:**

13. **Is anyone else working in this project?** Another person, or another AI in the same checkout.
    It changes the editor rule and how commits are staged —
    [`Docs/Guides/TEAM.md`](Docs/Guides/TEAM.md) is the guide. *[default: solo]*
14. **How do you want to check my work?** After every build / at milestones / only when I am unsure.
    Renders and play captures are how I show you; you are the one who can tell me the door is in the
    wrong place. *[default: at milestones]*

Question 14 is not a formality. Some things — placement, proportion, whether a room feels right —
cannot be settled by any test you can write. Whatever they answer becomes a standing cadence that you
write into the project's docs and then actually follow.

## Step 4 — Configure GraphCode

Act on the answers to questions 10–12, using [`Docs/Guides/SETTINGS.md`](Docs/Guides/SETTINGS.md). Say what you changed and where it was
written, in one line each — a setting nobody knew was set is a setting that will surprise them later.

**If they named a generation service** (question 9 / 9b), prepare **one** `.env` at the project root
now, with a blank line per service and nothing else, and add `.env` to `.gitignore` in the same act.
**You write the key names; they write the values.** A key is a secret — never type, paste or read one
back; leave every value empty and ask them to fill it in. [`Docs/Guides/CONTENT.md`](Docs/Guides/CONTENT.md) has the shape and
the rules. If no service was named, skip this and say so — an empty secrets file is one more thing to
explain later.

## Step 5 — Settle the build mode

Their answer to question 2 selects one of three profiles. These are **leanings you adapt**, not laws.

**Prototype** — the smallest experiment that answers the current design uncertainty. Use blockout
where that fidelity was agreed. Make the player loop work and report what the experiment proved.

**Vertical slice** — a deliberately small, complete player journey at the intended quality.
Representative art, motion, audio, input and UI; failure and retry; persistence where required;
measured performance and an early packaged run. Choose its duration from the loop, not a fixed quota.

**Production** — expand a proven slice with reproducible content, reliable systems and measured
budgets. Use streaming, data layers or components when the game's requirements justify them.
Settings, persistence and accessibility follow the agreed product requirements. Release requires
evidence from the actual packaged candidate; follow [VALIDATION.md](Docs/Guides/VALIDATION.md).

Say which one you picked and why, in one line. If they disagree they will say so now, rather than
after you have built a streaming world for a game jam.

## Step 6 — Write this project's own docs

This is the point of the whole exercise. Our own project, after months of work, settled on three
anchors, and that shape transfers even though the filenames should not:

- **A routing-and-rules doc** — where things live, which verb answers which question, the rules from
  §6, one grammar example. Deliberately small, because it is read at the start of every session.
  Measure its size; do not estimate it.
- **A board** — one line per open piece of work, closed lines deleted, a date on anything that
  finished. It is the only place progress is tracked, and it is never re-derived from a design doc.
- **Detail, opened only when needed** — a folder of narrow documents, **one concern each**, none of
  them read unless the task is about them. Assets in one, the world in another, the team convention
  in a third. The test is simple: if opening a file to answer one question also loads three subjects
  you are not working on, it is two files.

**Start from the seeds, do not start from a blank page.** This plugin ships starter documents beside
this file, in `Docs/Seeds/`. Copy the ones that fit into the project, fill in what setup just told
you, and delete what does not apply. They are a floor, not a format — a project that needs a fourth
document should have one.

The routing doc already exists by now — you created it at step 0 to hold the checklist. This is where
the rest of it gets filled in; do not start a second one.

**How many files that is, and what they are called, is yours to decide.** A small prototype may want
two. A production project with three collaborators will want more. Fit it to what you found in step 1
and to the rung from step 2.

For a vertical slice or production game, adapt
[`PRODUCTION-PLAN.md`](Docs/Seeds/PRODUCTION-PLAN.md), or add its missing answers to an existing
production note. Record targets, a reproducible player journey and evidence locations. Link it from
the routing doc alongside [PRODUCTION.md](Docs/Guides/PRODUCTION.md). Open work stays on the board.

Record the game's experience and choice structure using [DESIGNING.md](Docs/Guides/DESIGNING.md),
the mechanic contracts using [GAMEPLAY.md](Docs/Guides/GAMEPLAY.md), and the UI screen flow, visual
direction and interaction states using [UI-UX.md](Docs/Guides/UI-UX.md). Keep project-specific UI
decisions in the existing relevant design note, or a focused UI note if substantial; link it from
art and mechanics rather than duplicating it. Add the routes below even when setup is already done.
The [worked slice](Docs/Guides/GAME-SLICE-EXAMPLE.md) is a teaching example, not the customer's design.

**The design docs — one per domain, plus one folder for what cannot be written down.** The four
domains from step 2 are four documents, because a decision about the palette and a decision about the
damage formula are never read in the same minute:

| | |
|---|---|
| `Docs/Design/GAME-DIRECTION.md` | what this is, the loop, the scope, what gets built next |
| `Docs/Design/ART-DIRECTION.md` | the look, the palette, the materials, what a room should feel like |
| `Docs/Design/MECHANICS.md` | the systems and the numbers, one section per mechanic |
| `Docs/Design/SOUND.md` | the effects, the music, and what the silence is doing |
| `Docs/Design/MoodBoards/` | **the images.** Never `Content/` — nothing imports them and nothing ships them; they are what the two of you look at while deciding |

Copy `Docs/Seeds/DESIGN-NOTE.md` once per file, and write who drives it at the top. **A domain nobody
has an opinion about yet still gets a file, with one line saying exactly that** — an empty file tells
the next session the question exists and is open, where a missing file tells it nothing at all.

Fold a decision into its file the moment it is made, not at the end of the session. The value of
these four is that a settled thing stops being re-litigated; a decision still sitting in a chat log
is not settled, it is just recent.

**Route to our guides; never copy them.** Part C is a set of files that ship with the plugin and
upgrade with it. The routing doc should say *"where an asset comes from:
`<plugin>/Docs/Guides/CONTENT.md`"* and stop there. Pasting the content into their project makes a
second copy that goes wrong the first time GraphCode changes, in a file nobody thinks to re-read.
What belongs in their doc is what is only true there — where things are, what was decided, what may
be spent, who else is working.

Rules to carry into whatever you write:

- Every durable fact has exactly one home. When something changes, update its home rather than
  writing it twice — two copies of a rule become two different rules.
- A document that grows past roughly 800 lines splits: the open work, the finished work, the things
  you learned. Finished items **leave** the open file.
- Write what was decided and why, not what happened. A log of your session is not documentation.
- Mark superseded claims as superseded rather than deleting them. A rule that was reversed is worth
  more than a rule that vanished.

## Step 7 — Make it enforceable, where the agent supports that

Markdown is advice. Where the agent has a real settings layer, write the rules into that as well, so
*"never delete without asking"* is enforced rather than hoped for. In Claude Code that means
permission rules in `settings.json` matched to the rung from step 2 — the deny list is the important
half — and hooks for anything that must happen every time rather than when you remember.

Add a pointer where the next agent will find it — the project's root agent file (`CLAUDE.md`,
`AGENTS.md`, or its equivalent) should say GraphCode is installed and route to the project's own
docs, and carry the line [GRAPHCODE.md](GRAPHCODE.md) §6 gives, so every session reads GraphCode's
own entry file and searches the Marketplace for precedent before authoring. The pointer also
tells agents to adapt a useful pack as needed for the project. Read and preserve the existing
file first. If the owner limited setup to new files, record
the pointer as an open setup item instead of silently changing an existing policy file.

## Step 8 — Build one thing, look at it, play it, show it

Do not hand back a folder of documents. Read what you just wrote, build the first thing the identity
and the build mode call for, and then:

- **Build it to the bar.** A visible first thing follows [ASSET-QUALITY.md](Docs/Guides/ASSET-QUALITY.md)
  from references onward. If the owner wants a quick mechanics test instead, build it as a labelled
  blockout and put its replacement on the board.

- **Look at it.** `gc_render` it. This is the step people skip, and it is the step that catches the
  wrong thing being built.
- **Play it.** If Play is installed, start a session, drive it, and confirm the thing you built
  actually does what it claims. Otherwise use an available Unreal play workflow; if none is
  available, mark runtime verification as not run and leave its task open. A build that has never
  run is a guess.
- **Show it.** Put the picture and what happened in front of them. On rungs (b) and (c) that is the
  conversation.

Then hand back with the board populated and the next line of work named.

**Go back to the checklist from step 0.** Every box is either ticked or explicitly declined/not
applicable with a reason; activation is done once Apply runs, as step 2a
describes. Required work that could not run stays open. If any required item remains, setup is not
finished — name it rather than handing back a project that looks done.

**Then stamp it**, so the tools stop asking. Write `Config/GraphCodeSetup.ini`:

```ini
[GraphCodeSetup]
Revision=16
Date=<YYYY-MM-DD>
```

The number is the `setup-revision:` from the top of this file. Commit it — a teammate cloning the
repository has the same docs and should not be told to set up a project that already is. **Write it
last, and only if setup actually finished**, because the stamp is what silences the reminder; stamping
a half-finished setup buys silence and loses the only thing that was going to point at the gap.
This stamp records project setup documentation; it never activates a trial or grants product rights.

Setup is finished the moment the project can be worked on. Everything after this is the work.

---

# Part C — the guides, one concern per file

**Nothing below is inlined here, and that is the point.** A guide is read when a task needs it and
never otherwise, so a subject you are not working on costs you nothing. The same rule applies to the
docs you are about to write for this project in step 6: routes in the file read every session,
content in the file read when the subject comes up.

| When you need to | Go to |
|---|---|
| **make game content or presentation — models, animation, VFX, UI, audio and other media** | [`Docs/Guides/ASSET-QUALITY.md`](Docs/Guides/ASSET-QUALITY.md) **first, every time**; then the product guide below |
| research any unfamiliar or complex work and write its detailed plan before implementation | [`Docs/Guides/RESEARCH-AND-PLAN.md`](Docs/Guides/RESEARCH-AND-PLAN.md) — universal workflow and reusable plan template |
| make clothing or armour | [`Docs/Guides/GARMENT.md`](Docs/Guides/GARMENT.md) — armour is Garment and Shape together |
| make hair | [`Docs/Guides/GROOM.md`](Docs/Guides/GROOM.md) — start from its proven coat |
| start or expand a production game or polished slice | [`Docs/Guides/PRODUCTION.md`](Docs/Guides/PRODUCTION.md) |
| make a cinematic from story and staging through performance, edit, light, sound and delivery | [`Docs/Guides/CINEMATICS.md`](Docs/Guides/CINEMATICS.md); save the [director's packet](Docs/Guides/CINEMATICS-PREPRODUCTION.md) before production, then use [`Docs/Guides/CINEMATICS-GRAPHCODE.md`](Docs/Guides/CINEMATICS-GRAPHCODE.md) for authoring and integration |
| define a cinematic's presentation intent, storyboard it, compare rough views and plan audience focus | [Intent](Docs/Guides/CINEMATICS-INTENT.md), [storyboards/view studies](Docs/Guides/CINEMATICS-STORYBOARDS.md) and [attention](Docs/Guides/CINEMATICS-ATTENTION.md); record selections and necessary dependencies before final authoring |
| choose techniques for an expressive cinematic presentation and review frames before final recording | [Technique selection](Docs/Guides/CINEMATICS-TECHNIQUE-SELECTION.md), then [key frames, frame ledger and timed review](Docs/Guides/CINEMATICS-STORYBOARDS.md#review-key-frames-before-final-recording) |
| study cinematic artistic direction, shot choices, pan/tracking moves or editorial transitions in depth | Focused [direction](Docs/Guides/CINEMATICS-DIRECTION.md), [shots](Docs/Guides/CINEMATICS-SHOTS.md), [movement](Docs/Guides/CINEMATICS-CAMERA-MOVEMENT.md) and [editing/transitions](Docs/Guides/CINEMATICS-EDITING-AND-TRANSITIONS.md) chapters |
| connect gameplay, UI, AI, persistence or networked interactions | [`Docs/Guides/GAMEPLAY.md`](Docs/Guides/GAMEPLAY.md) |
| design a polished HUD, menu, inventory, settings or onboarding flow | [`Docs/Guides/UI-UX.md`](Docs/Guides/UI-UX.md) **before authoring widgets** |
| use outside design, art or browser tools for UI and bring the result into Unreal | [`Docs/Guides/UI-TOOLS.md`](Docs/Guides/UI-TOOLS.md) |
| learn one connected design, mechanics, UI and playtest workflow | [`Docs/Guides/GAME-SLICE-EXAMPLE.md`](Docs/Guides/GAME-SLICE-EXAMPLE.md) |
| give a robot or creature goal selection, detours and recovery | [`Docs/Guides/AI-NAVIGATION.md`](Docs/Guides/AI-NAVIGATION.md) |
| verify a milestone, profile performance or hand over a package | [`Docs/Guides/VALIDATION.md`](Docs/Guides/VALIDATION.md) |
| turn sidecars on or off, exclude a folder, change any setting | [`Docs/Guides/SETTINGS.md`](Docs/Guides/SETTINGS.md) |
| stop git from merging content into nonsense | [`Docs/Guides/GIT.md`](Docs/Guides/GIT.md) |
| decide where an asset comes from — libraries, imports, generation | [`Docs/Guides/CONTENT.md`](Docs/Guides/CONTENT.md) |
| know how a game actually gets designed with these tools | [`Docs/Guides/DESIGNING.md`](Docs/Guides/DESIGNING.md) |
| work where somebody else is working in the same project | [`Docs/Guides/TEAM.md`](Docs/Guides/TEAM.md) |
| choose the right tool and finish its verification loop | [`Docs/Guides/TOOLS.md`](Docs/Guides/TOOLS.md) — then `{help:true}` on the chosen verb |
| arrive cold and find the smallest route through the product | [`Docs/Guides/AI-COLD-START.md`](Docs/Guides/AI-COLD-START.md) |
| know what GraphCode can and cannot do, the workaround for each gap, and which product owns which document | [`Docs/Guides/CAPABILITY-MAP.md`](Docs/Guides/CAPABILITY-MAP.md) — read it at step 2b |
| build with Shape, World, Move, Play, Rig, Groom, Garment or Human | [`Docs/Guides/AUTHORING.md`](Docs/Guides/AUTHORING.md); then read the matching focused guide |
| clean a generated 3D model (organic objects: creatures, characters) | Market guide `GraphCode/CleanGeneratedMesh` (`unreal_gc_market`), then [`Docs/Guides/CAPABILITY-MAP.md`](Docs/Guides/CAPABILITY-MAP.md#organic-objects-and-characters-the-generated-model-route) |
| generate a 3D model by part and combine prepared organic connections | [`Docs/Guides/ORGANIC-PART-ASSEMBLY.md`](Docs/Guides/ORGANIC-PART-ASSEMBLY.md) |
| combine Human, MetaHuman, Rig, body animation and facial animation | [`Docs/Guides/CHARACTERS.md`](Docs/Guides/CHARACTERS.md) |
| learn from recurring authoring, visual, runtime and recovery failures | [`Docs/Guides/LESSONS.md`](Docs/Guides/LESSONS.md) |
| learn a grammar, a kind, or a native type | `gc_describe` for supported kinds and programs; use the matching focused guide only where installed help names a gap |

The last three rows separate stable practice from changing syntax. These guides explain the
workflow, the judgment calls and the failure modes. **The tools answer exact grammar and parameters**
on the day they are called; never copy those into a project document as permanent truth.

**These guides live in the plugin and are never copied into a project.** They upgrade when GraphCode
upgrades. A copy would go wrong the first time anything changed, in a file nobody thinks to re-check
— which is the whole failure §9 exists to catch. What a project's own docs carry is the *route*,
plus the answers that are only true here: where things are, what was decided, and what may be spent.

# Part D — the edges

## 7. What this file is not

- **Not the install guide.** If the plugin will not load or the bridge will not connect, that is
  installation, and it lives in [`Docs/CONNECTING.md`](Docs/CONNECTING.md) — one page, and the short
  version is that the editor writes the configuration to `Saved/UnrealAIBridge/mcp-config.json`
  and the person pastes it into their client. The production bridge includes its runtime; Node.js
  and npm are not prerequisites. Source checkouts use the Node command in their generated config.
- **Not a tool reference.** The tool surface is computed and answers for itself through `help`.
  Anything you learn about a specific parameter from this file rather than from the tool is a fact
  that will eventually be wrong.
- **Not the workflow.** How the person and the AI work together day to day — the design conversations,
  the images, the iteration — is the practice that begins after setup ends.

## 8. Running this again later

Setup is re-runnable, and worth re-reading after a plugin upgrade — a new product may have been
installed since, and §2 is how you find out. When you run it again: read the docs that already exist,
keep what is still true, and change what is not. Do not overwrite a document someone has been editing
— say what you would change, and let them decide. The checklist from step 0 is what tells you what was
already settled; it is never deleted for exactly this reason.

When you finish, raise the stamp in `Config/GraphCodeSetup.ini` to this file's current
`setup-revision:`.

What each revision changed, so a re-run knows what the project's docs are missing:

- **16 (2026-11-25): release 0.1.15 facts, and the capability page.** GraphCode 0.1.15 supports Unreal
  Engine 5.7 and 5.8 (one download per engine). The trial lasts 21 days; Core, Shape and Integrate are
  free for personal use. Contact is admin@graphcode.dev. New step 2b: tell the person what GraphCode can
  and cannot do, and the workaround for each gap, from `Docs/Guides/CAPABILITY-MAP.md`. Add its route to
  the project's agent guide and the matching checklist line. Activation: a trial needs only an email; the
  first change emails a code that goes in the `code =` line (revision 15's `code = 000000` is gone).
- **15 (2026-10-20): activation is a file Apply checks.** Step 2a fills
  `%USERPROFILE%\.graphcode\activation.txt` for the person (first shipped as `%LOCALAPPDATA%\GraphCode\`,
  superseded the same day; projects never recorded the path) and follows Apply's reply when it refuses.
  Replace any recorded "unsupported — activation unavailable" status; keep email and code out of
  project docs.
- **14 (2026-10-18): customer-access status without collecting credentials.** Add the step 2a
  checklist entry and route to existing project agent docs. Check installed status and capabilities
  before offering trial/purchase activation; keep browser verification with the person and secrets
  out of chat/docs. Record unsupported or deferred access honestly. Follow installed help for later
  sign-out or PC removal without resetting trial history or releasing a fifteen-minute reservation
  early. This guidance does not ship customer activation or native licence enforcement.
- **13 (2026-10-03): game-specific generated UI art and finished water materials.** Apply the
  professional methods distilled in `UI-UX.md` and the step-by-step AI workflow in `UI-TOOLS.md`:
  inspect the game's art, define the screen/layout, generate a complete mockup and palette, refine
  it, then generate its transparent elements, assemble live controls and verify in Unreal. Use ImageGen or authorized online
  generators; do not replace these instructions with a general research assignment. Prefer generated
  artwork over generic hand-crafted approximations; keep text, layout and interaction live and
  verify art fidelity in the running game. This refines revision 8's optional-artwork guidance.
  Route water work to `WATER.md`, especially optical depth, reflections, flowing normals and foam;
  finish one moving section before expansion. Use `TEXTURE-VARIATION.md` for repetition and surface
  imperfections, and `CONTENT.md` for Blendkit generators/materials delivered through Integrate.
  Update existing agent routes without copying these guides or replacing the project's art direction.

- **11 (2026-09-28): exceptional craft, coherent theme and complete inspection across media.** Follow
  `ASSET-QUALITY.md`: go beyond minimum detail with purposeful craft unless simplicity is explicitly
  requested; simple forms and straight edges need an explicit design reason. Plan theme-consistent
  enhancements and render coverage for every seam, connection and detail. Inspect and repair
  throughout production, re-render changes, and judge the whole asset alongside its environment.
  Apply the same quality bar to animation, VFX, UI, audio and other presentation, validating
  phases, states and transitions through actual playback, listening and interaction.
  Carry the rule and canonical guide link into existing project instructions.
- **10 (2026-09-28): resolve each step before production effort.** Strengthen the research rule
  in the project's agent guide: record the decision, evidence and success check for every
  still-ambiguous step before executing it. `ASSET-QUALITY.md` now requires dedicated part
  references where construction or assembly is unclear, and deliberate `add` join selection
  per connection. Do not teach `join=none` as the default for adding geometry. Link to the
  canonical guides instead of copying their decision tables into the project's instructions.
- **9 (2026-09-28): universal research and planning.** Add `RESEARCH-AND-PLAN.md` to the project's
  agent guide with the rule: extensively research any unfamiliar or complex work, check current
  capabilities, and save a detailed implementation plan before building. The generic template
  applies across every subject. Plans cover sources/findings, approach, dependencies, stages,
  integration, risks and acceptance evidence. Continue authorized implementation after planning;
  research-only and plan-only requests remain within their scope.
- **8 (2026-09-28): game-making and UI guidance.** Add task routes to `DESIGNING.md` and
  `GAMEPLAY.md` for the player experience, choices, mechanics and playtests; require `UI-UX.md`
  before game UI work; route outside design/browser work through `UI-TOOLS.md`. Agents should
  design full screen states and navigation, default to HTML/CSS with generated artwork through
  tools or authorized service APIs where useful, preserve an existing suitable workflow, and
  prove the result in the target game. Add `GAME-SLICE-EXAMPLE.md` as an optional worked example.
  Preserve the project's chosen style, engine and existing decisions; this is not a request to redesign them.
- **7 (2026-09-28): the asset quality gate.** Add to the project's routing doc, where it is read every
  session: *before making anything a player will see, read `<plugin>/Docs/Guides/ASSET-QUALITY.md`
  and follow it — references, plan, one representative part at final quality, then the rest.* Add
  routes to `ASSET-QUALITY.md`, and to `GARMENT.md` for clothing and armour and `GROOM.md` for hair.
  If the project's quality bar reads *placeholder*, check with the owner whether that is still meant;
  a placeholder is now a labelled blockout with a replacement task, never finished art.

## 9. The reminder, and how to switch it off

You may see one line in a tool reply saying this project has never been set up, or that its docs were
written against an older revision. That is the same stamp, read once per session:

- It appears **once per session, on the first reply**, never on every call.
- It **stops the moment the stamp matches** — there is no confirmation line when things are current.
  Silence is what being set up sounds like.
- `Revision=-1` silences it permanently, for a project that keeps its documentation some other way.
- Setting `GRAPHCODE_NO_SETUP_NOTICE=1` in the environment silences it for one process, which is what
  test suites and unattended runs want.

**Do not silence it instead of acting on it.** It fires on exactly one condition — this project's docs
and this plugin disagree about what setup means — and that condition is worth a few minutes.
