# <Project> — the board

*One line per open piece of work. Close a row by deleting its line. This is the only place progress is
tracked — never re-derive it from a design document.*

Name a player-visible outcome and the check that closes each row. For example: “collect once,
reject a second collection, retain the quantity after restart — runtime capture and reload check.”
Link to the relevant design or production note for detail. An apply or compile success alone does
not close a gameplay row. A temporary art proxy leaves its replacement and visual review open.

## Now

| What | State |
|---|---|
| <the one thing being worked on> | <where it actually is, not where it should be> |

## Next

| What | Why it is next |
|---|---|
| <row> | <the reason, in a few words> |
| <row> | <…> |

## Blocked

| What | On what |
|---|---|
| <row> | <the thing that has to happen first, and who owns it> |

## Done

*Newest first. A closed row keeps its date and loses everything else — one line.*

- **<YYYY-MM-DD>** <what shipped, in one line, including how it was verified>

---

**When this file passes roughly 800 lines, split it**: the open work stays here, the finished work
moves to its own file verbatim, and anything learned along the way goes to a third. Finished items
*leave* the open file — a board is only useful if the whole of it can be read.
