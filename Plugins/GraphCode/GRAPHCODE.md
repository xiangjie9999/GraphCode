# GRAPHCODE.md — read this first, every session

*For any AI (Claude, Codex or another) working in a project that has GraphCode installed. Read this
file at the start of every session and again before any GraphCode work you would otherwise do from
memory. It is short on purpose: it holds the rules and the map, and every detail lives one layer
down.*

## 1. Memory is the last authority, not the first

What you remember about GraphCode (from training, an earlier session, another project or a
transcript) was true of some other build. Grammar, verbs, defaults and limits change between
releases. **Before you write, claim or plan anything from memory, check it against the installed
build.** When the two disagree, the installed build wins and your memory is wrong.

The order of authority, highest first:

1. **The installed tool's own answer.** `unreal_info` for what is installed and running; a verb
   called with `{ "help": true }` for its parameters and values; `gc_describe` for a kind, block,
   token, program or native type. These are generated from the code you are calling.
2. **The capability registry** (`Resources/mcp-bridge/graphcode-capabilities.json`, summarised in
   [CAPABILITY-MAP.md](Docs/Guides/CAPABILITY-MAP.md)): what each product covers.
3. **These guides** (`Docs/Guides/`): methods, order of work, verification and lessons.
4. **The project's own documents**: its goal, decisions, paths and open work.
5. **Your memory**: a hint of where to look, never a source to quote.

## 2. Before you say GraphCode cannot do something

Most "missing features" reported from the field already existed under another spelling. Before you
write a workaround, a helper script or a request for new grammar:

1. Open the entry guide for the file kind (§4) and read its **"You want → write"** table.
2. Ask the tool: `gc_describe kind:"<kind>"` for the topic index, then the block you need; call the
   verb with `{ "help": "<parameter>" }`.
3. Search the guides for the effect you want (`join`, `mirror`, `fade`, `lods`...), not only the
   word you expected.
4. Read [LESSONS.md](Docs/Guides/LESSONS.md) for the failure you are seeing.

Only then call it a gap — and quote the help answer that shows it is one.

## 3. How the documents are layered

Each layer is more detailed than the one above it and links only downward. Read one route at a
time; never load the whole tree.

```text
Layer 0  GRAPHCODE.md (this file) ........ rules, authority, the map
Layer 1  routers ......................... which product, which verb, which guide
         Docs/Guides/README.md ........... route by task
         Docs/Guides/AI-COLD-START.md .... you arrived with no useful context
         Docs/Guides/CAPABILITY-MAP.md ... which product owns which kind
         Docs/Guides/TOOLS.md ............ which verb, and how to write safely
Layer 2  entry guides .................... one per file kind, read before its first line
Layer 3  method guides ................... the ones each entry guide lists for your situation
Layer 4  exact grammar ................... the tool's help and gc_describe, never copied into docs
```

A guide never restates exact grammar from layer 4; it names the verb and tells you to ask for it.
If a guide and the tool's help disagree, the help is right and the guide is stale.

## 4. Layer 2: the entry guide for each kind

**Before the first line of any of these files, open its entry guide and read every guide it lists
for what your file does, plus the shipped example it names.** A step a guide states is never
skipped because the rig or tool lacks something: fix the rig or tool, or report the refusal.

| You are writing | Entry guide |
|---|---|
| `.gcshape` — props, vehicles, buildings, weapons, creature parts | [GCSHAPE.md](Docs/Guides/GCSHAPE.md) |
| `.gcmove` — any animation | [GCMOVE.md](Docs/Guides/GCMOVE.md) |
| `.gcrig` — joints, limits, contacts, skinning | [GCRIG.md](Docs/Guides/GCRIG.md) |
| `.gcgroom` — hair, fur, brows, stubble | [GROOM.md](Docs/Guides/GROOM.md) |
| `.gcgarment` — clothing and armour | [GARMENT.md](Docs/Guides/GARMENT.md) |
| `.gchuman` — a MetaHuman identity, likeness, skin, face | [GCHUMAN.md](Docs/Guides/GCHUMAN.md) |
| a Niagara system — fire, smoke, sparks, magic, impacts | [EFFECTS.md](Docs/Guides/EFFECTS.md) |
| a Blueprint, level, widget, material or other `.gc` asset | [TOOLS.md](Docs/Guides/TOOLS.md), then [AUTHORING.md](Docs/Guides/AUTHORING.md) |
| `.gcw` / `.gcs` — terrain, water, foliage, navigation | [ENVIRONMENTS.md](Docs/Guides/ENVIRONMENTS.md) |
| `.gcp` — driving the running game, captures, tests | [VALIDATION.md](Docs/Guides/VALIDATION.md) |
| a ready-made recipe, guide or skill (damage, kits, pose libraries) | `unreal_gc_market`, then [MARKET.md](Docs/Guides/MARKET.md) |

Making game content or presentation of any kind? [ASSET-QUALITY.md](Docs/Guides/ASSET-QUALITY.md)
comes first, every time. Unfamiliar or complex work? [RESEARCH-AND-PLAN.md](Docs/Guides/RESEARCH-AND-PLAN.md)
before building.

## 5. Rules that hold on every route

- **Read before writing.** `gc_read` the asset; use `dry_run` where offered; verify the exact asset,
  graph or actor before changing it.
- **One state, one word:** `saved` is the file on disk; `live` is the editor with unsaved edits.
- **Every write is `gc_apply`**; creating any asset is `gc_apply create:true`. A long build:
  `wait:0`, keep working, collect with `unreal_info include:["tasks"]`.
- **A build's notes are part of its result.** Read every warning before rendering; a warning you
  skipped is the first thing to check when the render looks wrong.
- **Prove it the way the player sees it**: render, play back or play — [VALIDATION.md](Docs/Guides/VALIDATION.md).
- **Deleting needs explicit intent** from the person and a verified target.

## 6. Pointing your project at this file

A project's own agent instructions (`CLAUDE.md`, `AGENTS.md` or equivalent) should carry this one
line, so every session starts here instead of from memory:

```text
Before any GraphCode work, read <GraphCode plugin>/GRAPHCODE.md and follow its layers; never rely on memory for GraphCode grammar or limits.
```

[SETUP.md](SETUP.md) writes that line for you when it sets up a project. The GraphCode MCP server
also repeats it to every client that connects.
