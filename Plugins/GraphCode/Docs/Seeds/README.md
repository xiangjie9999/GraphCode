# Seeds — starter documents to copy into a project

*V1 draft, 2026-08-24.*

These are the floor for step 6 of [`SETUP.md`](../../SETUP.md), not a format. Copy the ones that fit
into the project's own docs folder, fill in what setup told you, and delete every line that does not
apply. A project that needs a fourth document should have one; a prototype that needs only two should
only have two.

| Seed | Copy it when | Becomes |
|---|---|---|
| `AGENT-GUIDE.md` | **first, before anything else** | the routing-and-rules doc read at the start of every session — and it carries the setup checklist, so an interrupted setup does not look like a finished one |
| `BOARD.md` | always | the one place open work is tracked |
| [`PRODUCTION-PLAN.md`](PRODUCTION-PLAN.md) | a vertical slice or production game | project-specific quality targets, reproducible player journey and evidence for the current candidate; open tasks stay on the board |
| `ASSET-LOG.md` | anything is imported or downloaded | the record of where content came from, under what terms, and — for generated content — the recipe to make it again |
| `DESIGN-NOTE.md` | **once per design domain** — game direction, art direction, mechanics, sound | the single home for that domain's decisions, read only when the task is about it |

Two rules that travel with them:

- **Fill them in, do not ship the placeholders.** A seed left with `<angle brackets>` in it is worse
  than no document, because the next reader cannot tell what was decided from what was skipped.
- **Rename them to suit the project.** The names here are ours. The shape is what transfers.
