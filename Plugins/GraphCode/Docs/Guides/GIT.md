# Git, and the one attribute that matters

*A GraphCode guide. Read it when the task needs it, not before — the router is [`README.md`](README.md), and the setup run that points here is [`SETUP.md`](../../SETUP.md).*

If this is a repository, write these lines into `.gitattributes`:

```gitattributes
# Unreal packages are binary and stay conflicted on purpose.
*.uasset binary
*.umap   binary

# GraphCode projections are canonical LF text and must never be auto-merged.
*.gc  text eol=lf merge=binary
*.gcw text eol=lf merge=binary
```

**A merge driver** is the command git runs to combine two edits of the same file during a merge or
rebase. Left to itself git uses a line-by-line text merge; `.gitattributes` can point a path at a
different one by name (`merge=<name>`), and `merge=binary` is the built-in that says *do not merge at
all — mark a conflict and let a person decide.* That built-in is what the lines above use, and it is
the safe choice here.

**Do not point `.gc` at a *custom* merge driver by name.** It is tempting — a `.gc` is text, so a
grammar-aware driver could merge two edits cleanly — and it is the one setup mistake with teeth: git executes a driver command, so it refuses to take one from a committed file — the command
has to be registered per clone, in local config, which cannot be shared. An attribute that names a
driver **nobody registered falls back to git's default line merge**, and that merge interleaves both
sides of a document into a file that parses, describes neither, and reports success with exit 0 and no
warning. `merge=binary` is the safe floor: a collision stays a conflict, and a person decides.

Resolving one is not a choice between two binaries. Take the asset from one side, apply the merged
`.gc` over it, save once — the binary is regenerated rather than merged, and the conflict is what said
that step existed. `gc_versions` resolves the text half.

Treat the proposal as unfinished until `gc_apply` succeeds, a fresh `gc_read` agrees with the
result, and the affected visual or gameplay check passes. `omits:` tells you which binary facets
the text cannot settle. An authored `.gcshape`, `.gcw`, `.gcrig` or `.gcgroom` recipe also has its
own file history; restoring a built Unreal asset does not undo the recipe that can build it again.

**When a project is ready for more than the floor, the driver ships in the box.** GraphCode's own
three-way `.gc` merge — *one dev moved a wall, another rotated it* resolves instead of colliding — is
wired by the two scripts in [`../../Tools/`](../../Tools/), with the full safe order in their
[`README`](../../Tools/README.md). It stays **off by default on purpose**, and the order is the whole
point: every clone runs `setup-merge-driver.ps1` *first* (dormant while the attribute is `binary`),
and only once the whole team has, does the `merge=binary` line above flip to `merge=graphcode`.
Flipping it before a clone has registered is the fail-open trap all over again — so if you are solo,
not sure, or cannot confirm every collaborator is set up, **leave it at `binary`**; it costs a
conflict marker at worst, never a wrong merge.

If a hosted Git service performs a merge without that local driver, its result also needs review.
Follow the repository policy in the [tool README](../../Tools/README.md) before enabling the driver
for a team.

**If this project gitignores `Content/`** — many do, for size — then say so out loud during setup,
because it changes what a sidecar is. The `.gc` files become the *only* versioned record of the assets
they describe. They must not be ignored too, and a binary-only change that never reaches a `.gc` is a
change that is nowhere.

**The one file that must be ignored, always: `.env`.** If setup put generation-service API keys in a
`.env`, that file is a secret and a committed secret is a leaked secret — git history keeps it long
after you delete the line. `.env` goes into `.gitignore` before the first key is written. A keyless
template (`.env.example`, every value blank) may be committed so a teammate knows what to obtain; the
real file never is.
