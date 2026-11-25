# GraphCode

GraphCode makes Unreal's authored content readable and writable as deterministic text. It keeps the
Unreal asset as the engine artifact and gives people and AIs a reviewable source beside it: assets,
procedural programs, runtime captures and the lessons needed to use them safely.

GraphCode 0.1.15 supports Unreal Engine 5.7 and 5.8 (one download per engine). Core, Shape and Integrate
are free for personal use; the trial of every product lasts 21 days. Contact: admin@graphcode.dev.

## Start here

**Any AI working with GraphCode reads [GRAPHCODE.md](GRAPHCODE.md) first, every session**: the rules,
the order of authority (the installed tools over memory) and the map of the documents, layer by layer.

```text
new installation
├─ connect an AI client .............. Docs/CONNECTING.md
├─ configure the project ............. SETUP.md
├─ what it can and cannot do ......... Docs/Guides/CAPABILITY-MAP.md
└─ begin normal work ................. Docs/Guides/AI-COLD-START.md

normal work
├─ what it can/cannot do, workarounds  Docs/Guides/CAPABILITY-MAP.md
├─ choose a verb and write safely .... Docs/Guides/TOOLS.md
├─ choose a focused workflow ......... Docs/Guides/README.md
└─ diagnose a surprising result ...... Docs/Guides/LESSONS.md
```

- [Connect the client](Docs/CONNECTING.md)
- [Set up a new project](SETUP.md)
- [Cold-start an AI](Docs/Guides/AI-COLD-START.md)
- [Browse the guide tree](Docs/Guides/README.md)
- [What GraphCode can and cannot do, and the workaround for each gap](Docs/Guides/CAPABILITY-MAP.md)

The installed build is the authority. Ask `unreal_info` what is present, call a verb with
`{ "help": true }`, and use `gc_describe` for the exact kind, block, program, token or native type.
The generated `Resources/mcp-bridge/graphcode-capabilities.json` is the development inventory.
Never infer current coverage from an old kind count, design proposal or example.

## The product family

Core is required. Every other product is optional and must be checked in the installed environment.
Some products are **program-first** and therefore have useful authoring surfaces even when they own
no reflective `.gc` asset kind.

| Product | Owns | Main text |
|---|---|---|
| **Core** | ordinary Unreal assets, Blueprints, UI, materials, data, input, imports and level contents | `.gc` |
| **Play** | the running game, actions, captures, comparisons and bounded restore | `.gcp` and `gc_play` |
| **Move** | animation, sequences, effects and decision assets | `.gc`, `.gcmove` |
| **World** | named space, terrain, water, foliage, navigation and PCG | `.gc`, `.gcw`, `.gcs` |
| **Core Extension / Advanced** | optional engine frameworks and Human/MetaHuman identity | `.gc`, `.gchuman` |
| **Shape** | procedural geometry intent | `.gcshape` |
| **Rig** | joints, topology, skinning and control intent | `.gcrig`, `.gc` |
| **Groom** | hair growth, styling and native Groom output | `.gcgroom`, `.gc` |
| **Garment** | clothing construction and fitted garment output | `.gcgarment` |
| **Integrate** | supported external-source translation and import | source files and import records |

The [capability map](Docs/Guides/CAPABILITY-MAP.md) says per product what you can make, what it does
not do and the workaround (for organic objects such as creatures: generate the mesh, then clean and
rig it with GraphCode). It also lists the registry's kinds and the status words: readable, writable,
creatable, existing-only, conditional, unsupported and planned.

## One source, several kinds of proof

```text
authoring text
  -> apply or build
    -> Unreal asset
      -> canonical readback
        -> render or layout
          -> running behaviour
            -> save + reload
              -> packaged result
```

- `.gc` reflects supported authored state in an Unreal asset. Its `omits:` line names facets the
  projection does not carry.
- `.gcshape`, `.gcrig`, `.gcmove`, `.gchuman`, `.gcgarment`, `.gcgroom` and `.gcw` are upstream
  intent. Their built assets may have `.gc` projections, but the projection does not replace the
  source program.
- `.gcp` records selected live runtime state. It is evidence and a bounded restore surface, not a
  replacement for a game's durable save design.

Text parsing proves syntax. A canonical read proves authored state. A render proves appearance. A
play run proves observed behaviour. A reload proves persistence. A packaged run proves deployment.
Use the [validation guide](Docs/Guides/VALIDATION.md) to stop at the evidence the actual claim needs.

## The normal safe loop

1. Identify the project, installed product and exact asset or program.
2. Read the state you intend to change: `saved` for disk, `live` for unsaved editor state.
3. Ask the verb and `gc_describe` for the smallest relevant help branch.
4. Inspect `sig:`, aliases, provenance, `omits:` and the block's omission/deletion rule.
5. Edit the source that owns the decision; plan or dry-run when the route supports it.
6. Apply once, then read the live result back and inspect asset-level errors.
7. Render visual work, drive behavioural work, save and reload when durability matters.

Deletion needs explicit intent and a verified target. Learn risky identity or destructive operations
on a new fixture, not the owner's irreplaceable asset. Use `gc_versions` for GraphCode history and
recovery, and never empty GraphCode's recovery trash for the user.

## Focused guides

- [Generate a model by part and assemble it with Shape organic join](Docs/Guides/ORGANIC-PART-ASSEMBLY.md)
- [Existing MetaHuman or 3D-AI mesh into MetaHuman, then Rig, body and face](Docs/Guides/CHARACTERS.md)
- [Garment construction, fitting and simulation decisions](Docs/Guides/GARMENT.md)
- [Groom recipes and visual verification](Docs/Guides/GROOM.md)
- [Environment and World workflows](Docs/Guides/ENVIRONMENTS.md)
- [Gameplay ownership and runtime systems](Docs/Guides/GAMEPLAY.md)
- [Production and release gates](Docs/Guides/PRODUCTION.md)
- [Content sourcing, modular models and ElevenLabs sound](Docs/Guides/CONTENT.md)

The complete lazy-loaded index is [Docs/Guides/README.md](Docs/Guides/README.md).

## Historical material

The former long-form README is preserved as
[Docs/Historical/README-LONG-FORM-2026-07.md](Docs/Historical/README-LONG-FORM-2026-07.md). It is a
design and positioning record, not current operating guidance. Its old counts, status statements
and Git instructions are intentionally kept out of this entrypoint so a cold AI does not mistake
history for installed capability.
