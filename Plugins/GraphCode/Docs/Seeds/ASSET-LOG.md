# <Project> — where the content came from

*Written at the moment of import, never later. Later, nobody remembers which pack the tree was in.*

Every row is one thing that came from outside this project. If a row cannot be filled in, the asset
should not ship.

| Asset in project | Source | Licence | Attribution required | Date | Notes |
|---|---|---|---|---|---|
| `/Game/<path>` | <site or pack name, and the item> | <CC0 / free for UE / per-item — quote the term> | <no / yes — and the exact text> | <YYYY-MM-DD> | <what it was modified into, if anything> |

## Rules

- **The licence is checked before the import, not after.** An asset already in the project is an
  argument for keeping it.
- **"Free" is not a licence.** Free to download, free for personal use, free with attribution and
  public domain are four different things, and only the last one asks nothing of a shipped game.
- **Attribution that is required goes into the game's credits at the same time as this row**, not at
  the end of the project.
- **Generated content is a row too, and it carries the recipe.** The service, the model, and the
  prompt — plus what its terms say about commercial use, which on most services differs between the
  free and the paid tier. The sidecar already writes down how the file came *in* (`import {
  source_hash }` and the `pipelines { }` block); the prompt is the other half. With both written
  down the asset is reproducible from text instead of being a download nobody can make again.
- **A custom ElevenLabs sound records the API capability and accepted output.** Keep the prompt,
  current model, duration and loop intent, output format, generation date and original accepted
  audio file. For dialogue, also record the authorized voice source and consent/licence basis.
- **Anything with an unclear term gets replaced now**, while it is one asset, rather than after the
  level is built around it.
