# Configuring GraphCode

*A GraphCode guide. Read it when the task needs it, not before — the router is [`README.md`](README.md), and the setup run that points here is [`SETUP.md`](../../SETUP.md).*

**Where the settings live.** `Config/DefaultGraphCode.ini`, and nothing else is in that file — it is
named after the plugin so it can be diffed, reviewed, ignored or deleted as one thing. The door a
person uses is **Project Settings → Plugins → GraphCode**.

### Sidecars, on or off

Ships **on**, and the rule is *everything gets a `.gc` unless told otherwise, by folder*.

Turning it off does not stop reads. It **does move existing `.gc` files out of `Content/` after a
confirmation**; they are recoverable under `Saved/GraphCode/RemovedSidecars/<timestamp>/`. Reads still work: the projection
happens in memory, you get the same text back, and the status says `Suppressed` instead of claiming a
file. What stops is files appearing beside assets. The two whole-project sweeps — `Sync all` in the
panel, and the check commandlet — refuse while it is off and say which toggle to flip, rather than
quietly treating every existing sidecar as an orphan.

Leave it on unless they ask otherwise. Text beside the asset is the entire point of the product; a
project with it off is a project where review, history and merge stop at the content boundary.

### Which folders to exclude

Ships **empty**. This is an opt-out list, and it takes **folders, not prefixes** — `/Game/Textures`
does not catch `/Game/TexturesOld`.

**Exclude:** vendored and marketplace packs nobody will ever author or review, and bulk imported
libraries whose contents are not part of the game's design. The test is *would anyone ever read a diff
of this?*

**Do not exclude:** the project's own content. That is the thing you are here to make reviewable.

Two facts worth telling them plainly, because both are easy to assume wrong:

- **An excluded folder is still fully readable.** The list says *do not keep text beside these*; it
  does not say *pretend these do not exist*. You can still read, search and write assets in an
  excluded folder — you just do not get a file on disk.
- **Adding a folder removes the `.gc` already under it**, and that is recoverable: the files are moved
  to `Saved/GraphCode/RemovedSidecars/<timestamp>/`, never unlinked. A `.gcw` region file is never
  touched. Removing a folder from the list offers to bring the sidecars back.

Folders the tooling itself owns — scratch and trash roots — are not on this list and cannot be added.

### The rest of the settings, briefly

- **Local history** — on, with a budget for the whole project rather than a count per asset. Setting
  the budget to `0` stops new entries and deletes nothing. It covers the last few saves; git covers
  what you committed.
- **Recording terrain sculpts and water splines on save** — on, and both only ever act on a level
  whose region file already declares that program. Every other level saves exactly as it did before.

**Before changing either capture setting**, inspect the affected `.gcw`. If terrain sculpt
recording is off, a hand sculpt remains in the binary level but is not expressed by its program.
If water spline capture is off, a later water rebuild can put the authored course back over a
hand-edited spline. Keep the level binary and its authored program under the project's chosen
version-control policy.

Review `git status` after changing exclusions or the master switch. A project that ignores
`Content/` may have relied on `.gc` as its only tracked record of an asset. Do not mistake
recoverable local removal for a committed backup. Turning sidecars back on, or lifting an exclusion,
offers to sync the files again.
