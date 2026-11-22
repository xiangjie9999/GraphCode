# Working where somebody else is working

*A GraphCode guide. Read it when the task needs it, not before — the router is [`README.md`](README.md), and the setup run that points here is [`SETUP.md`](../../SETUP.md).*

If the project is solo, none of this applies. Write that down in the project's docs and move on. It
starts applying the moment a second person — or a second AI — touches the same checkout or the same
machine, and the first sign is usually a compile error in a file you never opened.

## Three things collide, and they are three different problems

| What collides | Why | What it costs |
|---|---|---|
| **The editor** | one editor owns the project's files and its `Saved/`. A second one on the same project is a fight over locks | somebody's unsaved work, with no warning |
| **The `.uasset`** | it is binary. Git cannot merge it, and neither can you | one side's change disappears, silently, at exit 0 |
| **The build** | two builds writing the same `Binaries/` produce a DLL neither of them describes | an error that blames a file nobody touched |

**The `.gc` is the one that does not collide**, and that is a large part of what it is for: canonical
LF text that merges like code, with `gc_versions mode:"resolve"` to settle what git leaves
conflicted. Set the attribute in [`GIT.md`](GIT.md) before you rely on that — an attribute naming a
merge driver nobody registered falls back to git's line merge and interleaves both sides at exit 0,
which is worse than a conflict, because a conflict at least stops.

## The rule with teeth

**Never close an editor you did not open, and never close one a person owns without asking them in
chat first.** An editor holds unsaved work by design; closing it is not a restart, it is a deletion
of everything nobody saved. An editor you launched yourself is yours to close. If you cannot tell
whose it is, it is not yours.

## What to agree before a second agent starts

- **Who owns the editor right now**, and where that is said out loud.
- **One build at a time.** Say when you start and when you finish.
- **The board is the shared state.** Read it before picking anything up, and close a row the moment
  it is done — an open row somebody already finished is how two agents do the same work twice.
- **Stage your own paths, never everything.** In a shared checkout, staging all changes commits
  somebody else's half-finished edit along with yours.
- **A compile error in a file you did not touch is usually not a bug.** Check what changed on disk
  before you start fixing it.
- **Read `live` before a shared-asset write.** The saved sidecar may predate another worker's
  unsaved editor changes; identify the actual editor and project before applying text.
- **Do not use path-wide stashes or hard checkouts to isolate your work.** A path can contain edits
  from two authors. Inspect and stage only your own changes.

## GraphCode ships no coordinator — build one only if this project needs it

There is deliberately no coordinator in the box, because most projects should not carry one. A
convention that only lives in a document is one somebody forgets at 2am, but a coordination script in
a solo project is pure overhead. So the rule is: **solo, or one agent at a time — build nothing.** The
moment a project genuinely has two agents or two people touching the same tree, build the smallest
script that makes the three collisions above checkable rather than hoped for. Fit it to the project;
do not port someone else's.

Three verbs are usually enough — *who is live right now*, *may I do this* (build / take the editor /
commit), and *here is who I am and what I am doing*. If you write it, these are the traps that are
easy to miss and expensive to learn, so start from them:

- **The shared state must live outside every checkout.** A file inside the repo is invisible to a
  second clone or a git worktree — the very case you are coordinating. Put it somewhere machine-scoped
  (a per-user app-data path) so siblings can see each other.
- **Heartbeats expire; locks do not.** A crashed session that holds a lock forever deadlocks the
  project. Record a timestamp and treat a row older than a few minutes as stale, so a dead agent
  releases what it held by simply going quiet.
- **The editor is the sharp edge, and it has an owner.** Exactly one editor owns the project's files.
  Never close one you did not open, and never close a person's without asking — it may hold unsaved
  work. Detect the headless `UnrealEditor-Cmd` too; it is an editor for every rule here.
- **Fail safe and say why.** A check that blocks should say what it is blocking on; a check that only
  advises should still print, so *advise* and *stop* never look the same. Silence is the one thing a
  coordinator must never do.
- **Wrap long work so a death is traceable.** Key a running build or suite on the *work's* process,
  not your shell, and record when it starts and ends. A session that dies mid-build then leaves an
  orphan somebody can see and clear, not a mystery lock nobody dares touch.

Where the agent is Claude Code, a hook is the enforceable version of all this — a check that runs
before a build or an editor launch without the AI having to remember — and step 7 of `SETUP.md` is
where it gets written. This is the seed's own habit applied to coordination: the method travels, the
artifact is built to fit the project rather than shipped one-size-fits-all.
