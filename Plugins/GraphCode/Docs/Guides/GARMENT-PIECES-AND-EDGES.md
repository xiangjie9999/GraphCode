# Garment: sewn-on pieces, edge finishes and presses

*A GraphCode guide. Read it when a `.gcgarment` needs a pocket, a belt loop or a label as its own
piece of cloth, a faced or double-turned edge, or a seam allowance pressed one way or the other.
[GARMENT.md](GARMENT.md) is the entry guide for clothing; this guide holds one topic of it.*

Like the entry guide, this one does not replace the tool. Start with `gc_describe kind:"garment"`
and read the branch you are about to write. **If the installed description does not list a word
below, the installed build does not have it**, and the word is not yours to use. Every example here is
copied from a file or a parser test that exists; the comment above it says which. Measured numbers in
an example belong to that example's body.

## What the status lines mean

The Garment plugin installed as Production was built on 10 Nov 2026. Everything below marked *added
13 Nov* or later is **after that build**: check the installed description before using it. A build of
16 Nov compiles, but none of its new words has yet been rendered or run through the project's tests.
Each construct names how far it got, in these words:

| Status | It means |
|---|---|
| *parsed* | the document reads it, and a wrong use is refused by name; nothing is built from it yet |
| *meshed as X until the mesher takes it* | it is declared and checked; the build makes the nearest thing it already knows (X) and says so in a build note |
| *coded, not yet shown in a render* | the code exists and nothing has looked at its result |
| *experimental* | the words may change or go; do not build work on them |

A build's notes are part of its result: read every one after building a piece or an edge.

## A piece of cloth sewn on: `patch()`

*Added 13 Nov 2026.* A `patch()` is **a separate piece of cloth**, cut to an outline and laid on the
garment's own cloth, sewn down along its sides. It is not a shape or a mark drawn on the base cloth.
Use it for a pocket, a label, a belt loop, a knee patch. It is written below `construct()`, after the
sides that close it have been drawn as `line()`s.

The sides are three or more drawn lines, each named once, in order round the outline. The mouth of a
pocket is its `open=` side: the piece is turned under and sewn along every other side.

```gcgarment
# A pocket on a tube round the test body: its five sides drawn as lines, then the piece.
# Source: GraphCodeGarmentPatchTests.cpp, the jeans' back pockets block (EdgeR1..5, PocketR).
EdgeR1 = line($Back@6.78cm offset=-4.05cm, $Back@6.78cm offset=-19.83cm)
EdgeR2 = line($Back@6.78cm offset=-19.83cm, $Back@19.38cm offset=-19.04cm)
EdgeR3 = line($Back@19.38cm offset=-19.04cm, $Back@20.64cm offset=-11.94cm)
EdgeR4 = line($Back@20.64cm offset=-11.94cm, $Back@19.38cm offset=-4.84cm)
EdgeR5 = line($Back@19.38cm offset=-4.84cm, $Back@6.78cm offset=-4.05cm)
PocketR = patch($EdgeR1, $EdgeR2, $EdgeR3, $EdgeR4, $EdgeR5, open=$EdgeR1, turn=1cm, hem=1.27cm)
```

(The `line()`s sit inside the garment's braces, below `construct()`; `$Back` is a line drawn down the
back of the garment.)

| Word | What it does |
|---|---|
| `open=$Side` | the side left unsewn (the mouth); written once for each open side; at least one side stays sewn |
| `turn=1cm` | how far the sewn edges are turned under; required, with no default (about a centimetre is a pocket's usual allowance) |
| `hem=2cm`, `hem=(1.27cm, once)` | how deep and how often the open side is turned; unwritten, twice, as deep as `turn=` |
| `fabric=$Denim` | the piece's own cloth; unwritten, the cloth under it |
| `ply` | the piece is a layer of its own: it hangs as its own cloth and is tied to the cloth under it at the needle holes of the stitch rows you write over it. Without `ply` it is carried by the cloth under it and never simulated |
| `turn=0`, `layers=n` | a piece cut raw, or a strip folded to n plies; see the belt loop below |
| `ease=`, `gap=` | length in hand and clear space under the piece; see below |
| `under`, `window=` | a piece laid beneath the cloth; *experimental*, see the last part of this section |

Refused by name (the parser's own texts name each one): fewer than three sides; a side named twice,
or one that is not a `line()` drawn above the patch; `open=` naming a line that is not one of its
sides, or every side; no `turn=`; a turn or hem too wide for the piece; a patch written with no name to the
left of it; a word a patch does not take (`colour=` is refused).

### A belt loop: a raw strip folded to plies

*Added 15 Nov 2026.* `turn=0` cuts the piece **raw**: no turned allowance, its edge the cloth's own cut
edge, its face at one height. It is held only by the stitch rows and tacks you write over it, so it
must be a `ply`. `layers=3` stacks three plies of its fabric and sews them as one, so its stack is
three times the fabric's thickness: a belt loop is a strip folded in three.

```gcgarment
# Loop 1 of seven belt loops: a 1 cm strip on the waistband, its four sides, the piece, then its rows
# and the bar tacks across its ends. BandR is a line drawn down the waistband.
# Source: Content/__MCPTests/Garment/Probe/RealRender/JeansLoops8/Jeans-made-loops.gcgarment, lines 620-628 and 669-674.
BandR = line($Waist@0.245, $LoopTop@0.245)
L1Out = line($BandR@0.3cm offset=-12.13cm, $BandR@3.7cm offset=-12.13cm)
L1Top = line($BandR@3.7cm offset=-12.13cm, $BandR@3.7cm offset=-11.13cm)
L1In = line($BandR@3.7cm offset=-11.13cm, $BandR@0.3cm offset=-11.13cm)
L1Foot = line($BandR@0.3cm offset=-11.13cm, $BandR@0.3cm offset=-12.13cm)
Loop1 = patch($L1Out, $L1Top, $L1In, $L1Foot, turn=0, ply)
stitch($L1Out)
stitch($L1In)
stitch($L1Top)
stitch($L1Foot)
tack($L1Top@0.5, length=0.95cm)
tack($L1Foot@0.5, length=0.95cm)
```

A three-ply version, from the patch tests (the strip has its own fabric and three layers):
`Strap = patch($SOut, $SFoot, $SIn, $STop, turn=0, layers=3, ply)`.

- `layers=n` is the same word `rim()` uses for the plies at an edge; unwritten, 1.
- A **row written on one of a patch's own sides** (`stitch($L1Top)`) runs along the patch's side,
  between its two corners, wherever the patch now lies. With no `offset=` it is sewn 0.2 cm in from the
  edge, as the piece's own edge rows are. A row with `offset=`, or on a line that is no patch's side, is
  found on the cloth as before. *Added 15 Nov 2026.*
- `tack($L1Top@0.5, length=0.95cm)` needs the row stitched first; see the bar tack row of
  [GARMENT.md](GARMENT.md#plan-the-layers-from-the-skin-out).
- Every `patch()` has its edge rounded inward: the piece is exactly as wide as its lines say, widest at
  mid-thickness on the outline, and the round of its face and underside starts one radius inside.
  *Added 15 Nov 2026; coded, not yet shown in a render of a finished garment.*

Refused by name: `turn=0` without `ply` (a raw piece is held by its rows and tacks); `turn=0` with
`hem=` (a hem is a turn); `layers=` that is not a whole number of 1 or more; `layers=` above 1 with
`turn=` above 0 (a folded strip's edges are its folds: write `turn=0`).

### Length in hand: `ease=`

*Added 15 Nov 2026; coded; no example garment uses it.* `ease=0.3cm` cuts the raw piece
that much longer than the span between its sewn ends, along its long way, and lays it as the arch of
that length: a belt loop's room for the belt. The height of the arch comes from the length; nothing
is picked.

```gcgarment
Strap = patch($SOut, $SFoot, $SIn, $STop, turn=0, layers=3, ply, ease=0.3cm)
```

<!-- Source: GraphCodeGarmentPatchTests.cpp (the eased strap spec). -->

Refused by name: `ease=` of 0 or less; `ease=` with `turn=` above 0 or without `ply` (a turned piece
is sewn all round, so its middle has no free length).

### Clear space under the piece: `gap=`

*Coded 16 Nov 2026, not yet shown in a render.* `gap=0.1cm` adds clear space between the piece's
underside and the base cloth, on top of the rest the plies stand at. You place it; the tie and stack
springs at the needle holes rest with the gap, so the hang does not pull the piece back down.

```gcgarment
Strap = patch($SOut, $SFoot, $SIn, $STop, turn=0, layers=3, ply, gap=0.1cm)
```

<!-- Source: GraphCodeGarmentPatchTests.cpp, the author's-gap spec. The jeans' seven loops write gap=0.1cm. -->

Refused by name: `gap=` of 0 or less ("more than 0"; with none, drop `gap=`). Limits the build keeps:
the needle reach for holes grows by the largest `gap=` in the garment, so one piece's gap is also given
to a `ply` patch written without one; it is exact where all are equal. How far apart the piece and the
cloth are after the hang has not yet been measured. Read a note that says the rest and the author's gap
that were laid under it.

### A piece laid beneath the cloth: `under` and `window=` (experimental)

*Experimental, 16 Nov 2026: written and wired; the cutter is being finished and **no run has yet cut a
window successfully**. Do not build work on it.* `under` lays the piece beneath the cloth it is drawn on,
on the body's side, at its rest plus `gap=`: a pocket's facing. It is not simulated and is carried under
the finished cloth. `window=($W1, $W2, $W3, ...)` is a closed outline of drawn `line()`s, in order round
it (three or more, each once, the last ending where the first starts), independent of the piece's own
larger outline. The cloth inside it is cut out of the finished cloth above the under piece, exactly
along the lines, and the opening gets its own rim. A window line lying on the cloth's own edge is not
cut; the cut runs out to the edge there.

```gcgarment
# Source: Content/__MCPTests/Garment/Probe/RealRender/JeansPocket10/Jeans-made-pocketlayer.gcgarment, line 553
# (the window's eighteen lines WR3..WR17 are elided here).
FacingR = patch($FROut, $FRTop, $FRIn, $FRFoot, turn=0, ply, under, gap=0.1cm, window=($WR1, $WR2, ..., $WR18))
```

Refused by name: `under` without `ply`; `under` without `window=`; `window=` without `under`; `under` with
`turn=` above 0 or with `ease=`; a window line that is not a `line()` drawn above it, or named twice;
fewer than three window lines; an outline that does not close, leaks, or lies round a tube.

## Cloth laid over a cut: `extend(..., over)` and a band that follows it

*Added 19 Nov 2026. Built by the 20 Nov publish.* A wrap skirt is made as it is sewn: the skirt cut open
down the front, then the over-front grown on from one side of the cut and laid round over the cloth past
it. The word is a bare `over` inside `extend()`:

```gcgarment
# A wrap skirt's over-front. Source: Content/__MCPTests/Garment/Proposals/WrapSkirtOver.gcgarment, lines 29-32 (`Overlap = profile(0=14, 1=14)` is declared above it).
  Wrap = line($Waist@0.06, $KneeHem@0.16)   # where the under-front ends
  Under = cut($Wrap)                        # the skirt opened down it: sides $Wrap and $Under
  Over = extend($Wrap, $Overlap, over)      # grown on from $Wrap, laid over the cloth past the cut, welded at its root, free below
  rim($Over, turned=1cm)                    # its free edge is finished as any edge is
```

The over-front is the garment's own cloth: its root is a weld, one ply of the garment's fabric, and the
two fronts part below the waist. It is the mirror of `patch(..., under)`. Where a hand-drawn lap on the
cloth (`Overlap = extend($Lap, 14)`, [the entry guide](GARMENT.md#a-wrap-skirt-today)) rides the skirt,
this one is cloth of the cut itself.

What `over` takes, and what it refuses, by name:

| Written | Result |
|---|---|
| `extend($Wrap, $Overlap, over)` | one ply, no clear space |
| `over, gap=0.1cm` | clear space you place under the grown cloth, more than 0 (the same meaning as `patch(gap=)`); `gap=` with no `over` is refused, "on other grown cloth it is not built" |
| `over, layers=2` | that many plies stacked; `layers=1` is refused ("one ply unwritten; drop it") |
| a garment drafted flat | refused: "'over' is not built on it"; `over` needs the garment laid as one piece, as a cut skirt is |
| `over` twice, `over=...`, `gap=` twice | refused; `over` is a word alone |
| `over, turn=` | refused: finish the free edge with `rim($Over, turned=1cm)` |

<!-- Source: GraphCodeGarmentDocument.cpp lines 2944-2975 (over, over=, gap=) and 3030-3050 (turn=, layers=1, gap= without over); GraphCodeGarmentCloth.cpp line 1349 (drafted flat). -->

**A waistband that follows it.** `band($Waist, width=3.2cm, fabric=$Cotton, cut=100%, ends=$Wrap)`
written under the over-front runs from the under-front's top round the back and on along the over-front's
top row. The over-front already says which end is on top and how far it laps, so `ends=` stands alone:
there is no `over=` and no `lap=` beside it.

```gcgarment
# Source: Content/__MCPTests/Garment/Proposals/WrapSkirtOver.gcgarment, line 48 (the band of the wrap skirt above).
  band($Waist, width=3.2cm, fabric=$Cotton, cut=100%, ends=$Wrap)
```

Refused by name: `ply` beside `ends=` alone ("its own cloth already: drop ply"); `ends=` naming a line no
`cut()` opened above the band; a cut with no `extend(..., over)` grown from one of its sides above the band
(then the band names all three, `ends=`, `over=` and `lap=`, as a fly's does in the
[entry guide](GARMENT.md#plan-the-layers-from-the-skin-out)); and `over=` and `lap=` written beside
`ends=` where the cloth grown over already says them.

<!-- Source: GraphCodeGarmentDocument.cpp lines 3447-3452 (ply refused), 6547-6575 (the cut and the grown-over cloth checked), 3365 (over=). -->

## How an open edge is finished

The first guide covers the edge's measurements: `construct(rim=(turned=, layers=, falloff=))` for the
whole garment and `rim($Edge, ...)` for one edge (how deep the cloth is turned in, how many layers
stand there, how far across the edge the stack rounds off). A garment's edges are cut raw unless you
say otherwise.

A turned or faced edge is the garment's own cloth turned to the inside: its plies are drawn on the
inside face and the outside does not move, so from outside the edge reads as the plain cloth, not as a
band. A waistband that shows is a `band()`, cloth of its own: see
[a band shows; a turned edge does not](GARMENT.md#a-band-shows-a-turned-edge-does-not).

### Declare the kind of the edge

*Added 16 Nov 2026; parsed and meshed. The mesher builds each kind as its own section; where it cannot tell
two edges' kinds apart (a `turned(2cm)` and a `faced(2cm, ...)` both read as 2 cm, 2 layers) that edge keeps
the nearest plain edge, and the build's line for the garment says so edge by edge. The "Meshed today as"
column is that plain edge, not always what you get.*

An open edge has one **kind**, written at two levels. The garment's default goes in `construct(edge=...)`,
beside `rim=(...)` (each written once, in either order). One edge's own kind is written bare in
`rim($Edge, <kind>)`. The edge's own kind always wins. With neither, the edge is raw. Everything a kind
takes goes inside its own brackets, so one kind reads the same at both levels.

| Kind | Write | Meshed today as |
|---|---|---|
| raw | `raw` | a cut edge |
| overlocked | `overlocked`, `overlocked(0.4cm)` (the thread's width) | a cut edge (no thread drawn) |
| turned | `turned(2cm)`, `turned(2cm, twice)`, `turned(2cm, twice, first=6mm)` | as written: once is 2 plies, twice is 3. `first=` is read but not carried: 3 plies over the whole depth |
| faced | `faced(6cm, $Lining)`; inside the brackets `roll=` (default 0.16 cm, a turn of cloth) and `grade=(under, upper)` (default 0.3 cm, 0.6 cm) | turned once, 2 plies of the garment's own cloth: no facing fabric, roll or grade |
| bound | `bound(1.2cm, $Tape)` | turned once, 3 plies of the garment's own cloth |
| banded | never written: it is derived where a `band()` is joined to the edge | as `band()` builds it |

`faced` and `bound` require a declared fabric and are refused without one, or when the name is not a
fabric. An override is read only on a **real open edge** at that line: a ring the garment ends at, or an
edge made by a `cut()`, an `extend()` or a `patch()` mouth written above it. A drawn line, a middle ring,
something made below, or an edge a `sew()` or `zip()` joins fully shut is refused.

The older spellings read exactly as before: `turned=2.54cm`, `turned=(2.54cm, once)`,
`rim(..., turned=, layers=, falloff=, ply)`.

```gcgarment
# The garment's default, then one edge finished differently.
# Source: GraphCodeGarmentEdgeKindTests.cpp (Resolve.DefaultAndOverride), written into the StripOnTube garment.
construct(edge=turned(2.54cm, twice), rim=(falloff=0.1cm))
...
rim($Low, raw)
```

```gcgarment
# A hem turned twice with a narrow first turn.
# Source: GraphCodeGarmentEdgeKindTests.cpp (Parse.Kinds).
rim($Low, turned(2cm, twice, first=6mm))
```

```gcgarment
# A faced edge: the facing is its own declared fabric. $Lining is declared above the garment.
# Source: GraphCodeGarmentEdgeKindTests.cpp (Parse.Kinds).
Lining = fabric(weight=120, thickness=0.30mm)
...
rim($Low, faced(6cm, $Lining))
# or, the same edge with its roll and grade written instead of defaulted (one rim() for an edge, not both):
# rim($Low, faced(6cm, $Lining, roll=0.2cm, grade=(0.4cm, 0.8cm)))
```

Refused by name (the text names what was written): an unknown kind; `banded` written; a kind missing its
depth; a depth of 0 or less; `first=` on a hem turned once, `first=` of 0 or as deep as the hem;
`once|twice` misspelt; a setting a kind does not take; `roll=` or `grade=` as deep as the facing; two
kinds in one `rim()`; a kind together with `turned=`; `layers=` on raw, overlocked, faced or bound;
`roll=`, `grade=` or `first=` outside their kind's brackets; `edge=` inside `rim()`; a kind inside
`construct(rim=(...))`; `edge=` written twice; two `rim()` overrides on one edge when either writes a
kind; a kind on a banded edge.

For a turned kind, `layers=` must agree with its turns: `rim($Low, turned(2cm, twice), layers=3)` reads,
`rim($Low, turned(2cm), layers=4)` is refused ("stands 2 layers"), `layers=1` is refused (2 or more).

### Rows where the machine sews them: `stitch($Edge, at=turn, ...)`

*Added 22 Nov 2026; parsed and built.* A hem's stitching sits where its turn puts it, so write the rows from
the edge's own turn, not as numbers you place by hand. `at=turn` sews the first needle at the edge's
declared turned depth: its own `rim($Edge, turned=...)`, else the garment's `construct(rim=(turned=...))`.
`inset=` moves that needle inside the turn's inner edge. `needles=n` with `gauge=` sews a machine's
parallel rows, each one gauge nearer the edge. Change the turn and the rows follow.

```gcgarment
# A tee's coverstitch: the left needle even with the turn's raw edge, a twin needle 4.8 mm apart
# (Docs/Research/knit-hem-coverstitch.md). On a 2.54 cm turn: rows at 2.54 and 2.06 cm.
stitch($Hem, at=turn, needles=2, gauge=4.8mm, every=1.9mm)
# A jeans hem edge-stitched 2 mm inside a 1.27 cm turn: one row at 1.07 cm.
stitch($Ankle_R, at=turn, inset=2mm)
```

Each row is built as if written with its own `offset=`, and the build's line says where each landed.
Refused by name: `at=` other than `turn`; `at=turn` with `offset=`; `inset=` without `at=turn`;
`needles=` 2 or more without `gauge=`; `gauge=` with one needle; an edge with no turn; an inset as deep
as the turn; and a needle that would sew past the edge.

### A hem turned as real cloth: `ply` on an edge

*Added 15 Nov 2026; parsed; builds as the relief of two layers until the edge slices land.* A turned edge
can be a real second ply that hangs: `rim($Cuff_L, turned(2.54cm), ply)`, the same words as
`patch(..., hem=(d, once), ply)`. It needs a fabric that states `crease=` (the fold rests at its pressed
angle), and only a hem turned **once**. The sleeve of a knit tee, hemmed once with a coverstitch, is the
fixture:

```gcgarment
# Source: Content/__MCPTests/Garment/Common/TeeSleeveHem.gcgarment, line 34 (the older spelling of the same rim).
rim($Cuff_L, turned=(2.54cm, once), ply)
stitch($Cuff_L, offset=2.54cm, every=1.9mm)
stitch($Cuff_L, offset=2.06cm, every=1.9mm)
```

Refused by name: `construct(rim=(... ply))` (it would turn every edge; write the per-edge form), `twice` with
`ply`, `ply` on a faced or bound edge, a fabric with no `crease=`, and a `layers=` that disagrees with
the turns. The stitching is the file's own `stitch` rows.

### How a turned edge is drawn and hung

*Coded 16 Nov 2026, not yet shown in a render.* The mesh of a turned edge steps its inside face at the
declared depth (the plies end where you said, rather than at the last cell), and rounds corners sharper
than 60 degrees to a radius derived from the plies' stack. The hang can also weigh and stiffen a turned
zone by its plies, behind a switch that ships off: see [the switches](#the-switches).

## Which way a pressed seam stands: plain and `follow=`

`press($Seam, 1cm)` folds a seam's allowance to the side its sign names and presses it there (it
was in the 10 Nov build; see the seam row of the table in the entry guide). **Which way its
thickness goes** is decided as follows:

- A **plain press** is a seam allowance, or a turned hem, pressed flat against the cloth's inside, with
  the garment's cloth over it. On the outside that cloth stands proud by the allowance's layers: the
  pressed ridge a real seam shows.
- A **`follow=` press** is a **facing**: a wide piece held only by its stitch row, hanging free inside,
  so in the garment build its stack hangs inward as far as the room to the body allows.

*The `follow=` and `margin=` words were added 15 Nov 2026; parsed; the stack-direction rule is coded
16 Nov 2026, not yet shown in a render.* No new word is needed: `follow=` is the word that makes it a
facing.

```gcgarment
# A plain press: the sleeve's armhole allowance pressed toward the sleeve.
# Source: Content/__MCPTests/Garment/Common/TShirt.gcgarment, lines 229-230.
press($Armhole_L, -0.635cm)
press($Armhole_R, -0.635cm)
```

A facing cut to follow its own topstitching: the jeans' fly facing curves in with the J to the foot.
Draw the stitched rows themselves as ordinary two-place lines above the press, one or several in
brackets, in any order; the allowance's free edge lies `margin=` outside them.

```gcgarment
# The fly facing follows the J's own stitched rows. J1..J7 are drawn above the press (a press names only lines above it)
# and stitched below it.
# Source: Content/__MCPTests/Garment/Probe/RealRender/JeansLoops8/Jeans-made-loops.gcgarment, lines 287-294.
J1 = line($Front@0cm offset=-2.515cm, $Front@5.855cm offset=-2.515cm)
J2 = line($Front@5.855cm offset=-2.515cm, $Front@6.52cm offset=-2.48cm)
J3 = line($Front@6.52cm offset=-2.48cm, $Front@7.18cm offset=-2.35cm)
J4 = line($Front@7.18cm offset=-2.35cm, $Front@7.77cm offset=-2.04cm)
J5 = line($Front@7.77cm offset=-2.04cm, $Front@8.14cm offset=-1.50cm)
J6 = line($Front@8.14cm offset=-1.50cm, $Front@8.326cm offset=-0.761cm)
J7 = line($Front@8.326cm offset=-0.761cm, $Front@8.37cm offset=0cm)
press($FlyL, -3.195cm, follow=($J1, $J2, $J3, $J4, $J5, $J6, $J7), margin=0.68cm)
```

`follow=$J1` names one row. The allowance's free edge lies `margin=` outside the rows (a true offset of
all of them, pooled), never wider than the width: a row at the width less the margin is capped, with its
narrower margin named in the note, and one outside the width is refused. It rounds off over the margin
where the rows meet the pressed line; if that rounded end would close past the line's last point it is
shortened onto it, to 0 there. Write both settings or neither. Each row must lie on the pressed side and
meet the line only at its end. A gap between rows is named in the note. Not built yet on a declared seam's
chain or a dart (refused by name); the hang still carries the declared width, and the note says so.

Refused by name: `follow=` without `margin=` and `margin=` without `follow=`; `margin=` of 0 or less; a
margin as wide as the allowance; a press pressed `open` with `follow=`; `margin=` written twice; another
setting (`taper=` is not a setting of a press); a `follow=` row that is no line above the press, named
twice, or an empty list.

A crease down a trouser leg or a sleeve is the same word with no width: `CreaseR = line(...)` then
`press($CreaseR)`; see the crease row in the entry guide.

## The switches

Seven console variables change how the build treats an edge, a cut or a part. Set one before a build; where it
says something, the build prints a line saying what it did. All but `TurnedPliesCarry` and `RemeshTurnsBack` ship
**on** (19, 22 and 23 Nov) and need nothing written; the switch is there for comparing.

| Switch | Values | What it does |
|---|---|---|
| `gc.Garment.TurnedPliesCarry` | `0` (default), `1` | `0`: a turned edge hangs as one ply, exactly as before 16 Nov, and nothing is measured or said. `1`: a turned edge's zone hangs with its plies' weight and bending (n plies weigh n times, bend n squared times a single ply), and each hang prints one `GcGarmentEdgeHang` line giving what was added. *Coded 16 Nov 2026, not yet run*; the multipliers for bending are a starting value |
| `gc.Garment.RemeshTurnsBack` | `1` (default), `0`, `2` | Only in an editor built with the automation tests, and only for comparing. `1`: the remesh keeps split-only the two tips of a closed slit whose lips lie on each other (a cut across as laid). `0`: that rule off, as before 15 Nov. `2`: every point where an open edge turns back on itself, the 15 Nov form of the rule. In any other build the rule is always `1`. *Coded 16 Nov 2026, not yet run* |
| `gc.Garment.BandWindLevel0` | `1` (default), `0` | `1`: the nose band of a fold (the faces that join its two layers round a grown round, as at a cuff or collar) is wound as the skin face it joins, so it shows as cloth and not as a dark ring. `0`: wound as builds before 19 Nov did. Level 2 of `gc.Garment.OneFold` and up always use the new rule. *Default on 19 Nov* |
| `gc.Garment.CutLips` | `1` (default), `0` | `1`: the two lips of a `cut()` (the sides that stand a hairline apart) are the same cloth's neighbours when the hang starts, and are not pushed apart again. `0`: every such pair is laid apart, as before. The LinenTunic's 30 pairs became 0. *Default on 19 Nov* |
| `gc.Garment.OverLipUnder` | `1` (default), `0` | Reaches only `extend(..., over)`. `1`: the cut's other lip, under the over-front's root, is set in under it by the over-front's laid distance (never nearer the skin than the hang's pair contact), as an under front's edge lies a stack under the front laid on it; the build's note gives the count. `0`: that lip stays on the cut's line, as before. On the double-breasted coat probe the laid crossings fell from 81 to 26. *Default on 22 Nov* |
| `gc.Garment.HoldNotThrough` | `1` (default since 23 Nov), `0` | `1`: a part's hold never presses a piece sewn on the cloth (a lap, a placket) through the cloth it lies on; a point whose press would reach that cloth stays where the hang left it, and the build says how many and by how much. On the long coat its outer button column had pressed the lap 0.34-0.47 cm through the front - 55 crossings and a sunk button; with it the coat is clean. `0`: unchecked, as before. |
| `gc.Garment.PartsFollowHang` | `1` (default since 23 Nov), `0` | `1`: a part placed by a ring in space (`add($Kit.BeltLoop, at=$LoopRow@0.125)`) is placed where the hang carried the cloth laid at its place, so it stays on the cloth it was sewn to: belt loops end at the band's top edge instead of standing up to 1 cm above it, a back label stays inside the band. Parts placed on a cloth edge, a part's point or over the cloth were already on the hung cloth and are unchanged. `0`: cast from the place as laid, as before. |

## Experimental, unrendered: woven fabric looks

*Experimental. Added 16 Nov 2026; parsed. The build draws the weave into a mask and a normal map and
gives the material a second yarn colour. None of it has yet been shown in a render, and material work
stopped on 16 Nov, so the words below may change or go. Keep to a `fabric()` without them for any work
that must last.*

[GARMENT.md](GARMENT.md#materials-uvs-and-visible-stitching) describes a knit fabric's look (`knit=`
and `gauge=`). A woven fabric adds these words to the same `fabric()` line. All are refused by name
where they do not apply:

| Write | Means | Refused |
|---|---|---|
| `weave=plain` | over one, under one, drawn from `gauge=(ends, picks)` | without `gauge=` |
| `weave=twill(3/1, z)` | u over, w under (each 1 to 8, together 3 or more); direction `z` or `s`; 3/1 z is denim | `right` or `left`, no direction, u + w under 3, bare `weave=twill` |
| `weave=satin(5, 2)` | warp-faced satin: shafts n (5 to 16), move s sharing no factor with n | s of 1 or n - 1, or a common factor |
| `weave=basket(2/2)` | the plain rule on groups of 2 to 4 ends and picks | unequal counts, or outside 2 to 4 |
| `weave=herringbone(2/2, 8, z)` | a twill reversing every 8 ends (2 to 64), first stripe `z` or `s` | a missing part |
| `colour=(#31343A, #EAE0CE)` | a yarn-dyed woven: warp, then weft; `colour=#RRGGBB` stays one colour for both | on a knit, or not two swatches |
| `yarn=(100tex, 80tex)` | warp and weft linear density; `Ne 30` also reads; one value for both | on a knit, no weave, or no unit |
| `crimp=(12%, 6%)` | warp and weft crimp, as percentages | not percents, 0 or less or over 100 %, on a knit |
| `fibre=cotton` | cotton, linen, viscose, wool, silk, polyester, nylon or acrylic | another word, on a knit |

```gcgarment
# A denim: weight, thickness and the measured numbers the hang needs, then the woven words.
# Source: GraphCodeGarmentWeaveTests.cpp (the denim words read) and the jeans proposal Jeans-made-loops-denim.gcgarment, line 54.
Denim = fabric(weight=426.7, thickness=1.10mm, stretch=(3.2%, 2.3%), shear=314, stiffness=(4.02, 2.37), friction=0.53, crease=86, weave=twill(3/1, z), gauge=(26, 16), yarn=(100tex, 80tex), crimp=(12%, 6%), fibre=cotton, colour=(#31343A, #EAE0CE))
```

Known: the build's note prints a predicted face colour beside the warp's own, and for this denim the
mix is expected to read far too light against a measured face; the visible share of each yarn is not yet
drawn from the geometry. A crimp that cannot be derived is assumed at 10 % and the note says so; every
fibre density, assumed crimp or colour that is not sourced is marked in the note as assumed. A woven
under `uv(...)` is scaled as a knit is today, and a colour for the stitching thread is not read yet.
