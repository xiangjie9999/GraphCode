# Garment: cloth that hangs — solver words, a lifted hem, curtains

*A GraphCode guide. Read it when a `.gcgarment` is simulated in the game (`[as=cloth]`), when a hem should
be held up while the garment hangs, or when the file is a curtain. [GARMENT.md](GARMENT.md) is the entry
guide for clothing; this guide holds one topic of it.*

Like the entry guide, this one does not replace the tool. Start with `gc_describe kind:"garment"` and
read the branch you are about to write. **If the installed description does not list a word below, the
installed build does not have it**, and the word is not yours to use. Every example is copied from a file
that exists; the comment above it says which. Everything here was added 19 Nov 2026 and is built by the
20 Nov publish.

## `hang()`'s solver words: the game cloth, said in the recipe

`hang(from=$Ring)` says where an `[as=cloth]` garment is held ([where to hold the cloth](GARMENT.md#fit-skinning-and-collision-are-different-claims)).
It now also takes the words of the cloth's solver. They set the simulated asset the game plays, not the
build's own `settle()`. A word you leave out keeps the value the build or the engine chose; the build
prints one line saying what your words set.

```gcgarment
# Every word at once. Source: Content/__MCPTests/Garment/Proposals/HangWordsSkirt.gcgarment, line 66.
  hang(from=$Hips, damping=5%, local_damping=30%, iterations=12, substeps=4, ccd, self=0.6cm, repair)
```

| Word | Takes | Left out | Refused |
|---|---|---|---|
| `damping=` | a percent, 0% to 100%: how fast the whole cloth's motion dies | the engine's 1% | not a percent (`damping=0.05`), or outside 0 to 100% |
| `local_damping=` | a percent, 0% to 100%: how strongly each point's motion is pulled toward the whole cloth's | the engine's 0% | as `damping=` |
| `iterations=` | a whole number, 1 to 100: solver passes a 60 Hz step; the cap on a frame is twice it, at most 100 | the build's 8, cap 16 | outside 1 to 100, or not whole |
| `substeps=` | a whole number, 1 to 100: steps a frame, each finding contacts again | the build's 2 | as `iterations=` |
| `ccd` | nothing (a word alone): each point is swept against the body, so fast motion cannot pass through it | off | `ccd=...` |
| `self=` | a length above 0, up to 1000 cm: how far the cloth keeps from itself round each point; or `self=off`, the cloth never meets itself | the larger of half the cloth's thickness and a fifth of the spacing (0.5 cm for thin cloth) | 0 or less, over 1000 cm, or no value |
| `repair` | nothing (a word alone): cloth that has passed through itself is put back | off | `repair=...`; `repair` together with `self=off` |
| `spacing=` | a length, 1 cm to 2.5 cm: about how far apart the simulated points are | 2.5 cm | outside 1 to 2.5 cm |

<!-- Source: GraphCodeGarmentDocument.cpp ReadHangWords, lines 3077-3190 (spellings, ranges, refusals); GraphCodeGarmentClothWords.h lines 18-96 (ranges, what each leaves out). -->

A word said twice is refused ("the cloth runs with one"). A misspelt word is named: `'dampin' is not a word of
hang()`, with the list it reads. `hang()` without `from=` is refused as it always was.

**`spacing=` and the edge you draw.** The drawn garment keeps its own detail, each drawn point carried by
one simulated triangle, so a drawn edge bends only where one triangle hands over to the next. At the
default 2.5 cm that is every 2.5 cm; `spacing=1.25cm` makes it every 1.25 cm, for a wrap skirt's free edge.
It costs points (about four times as many at half the spacing) and the build's own self-collision, a fifth
of the spacing, follows it. Below 1 cm the cloth has no more of the garment to carry; above 2.5 cm it could
not carry the folds the build's own hang made.

```gcgarment
# Source: Content/__MCPTests/Garment/Proposals/HangSpacingSkirt.gcgarment, line 66.
  hang(from=$Hips, spacing=1.25cm)
```

## A tee pulled up a little, with creases

The hang lets a garment fall under its own weight. To hold part of it up while it falls, `settle()` takes a
hand, and a hand can be a whole ring: the garment's hem taken up to a ring you place and kept there, the
spare cloth gathering into creases above it by itself. (A hand that takes one place along a side of a cut
is the older form, in the [entry guide](GARMENT.md#plan-the-layers-from-the-skin-out); the two live in the same braces.)

Two lines make it. The ring, declared with the file's other rings, a level ring about 4 cm above the
tee's hem:

```gcgarment
# Source: Content/__MCPTests/Garment/Probe/HemGuideTee.gcgarment, lines 120-122. The tee is Common/TShirt.gcgarment as shipped.
HemUp = ring((0.000, 10.156, 94.5), level) {
  expand(0.259)
}
```

and, last in the tee's garment block, the guide:

```gcgarment
# Source: Content/__MCPTests/Garment/Probe/HemGuideTee.gcgarment, line 253.
  settle() { guide($Hem, $HemUp) }   # the hem taken up to HemUp while it hangs, and held there to the end
```

**What it does.** Each held point of `$Hem` is driven, while the cloth hangs, to its own fraction round
`$HemUp` (`@0` to `@0`: a twist is never corrected, so give both rings their first points at the same
place) and stays there to the end. Held are the points the simulated copy keeps, about one every 2 cm
(about 3.7 cm apart on this tee); the row between them is free, and the garment's own hold at the neck
stays. It moves cloth; it does not tighten it (that is `pull`). The fold above the hem is made by the hang,
not drawn: on the tee, the folds a 0.3 cm dent deep, counted at heights 97, 100 and 105 cm (the hem is at 90.5), number 17, 12 and 6,
against 2, 2 and 2 for the same tee hung with a bare `settle()`; the held points land on the ring to
0.000 cm.

**What to change.**

- *More or less lift:* the height in the ring's one point, `94.5` here, the tee's hem being at 90.5. Take
  the point's other numbers from the body at the new height (here 10.156, the front of the skin at 94.5;
  `gc_render` reads it), and keep the point on the skin, with `level` to make the ring flat.
- *Stood off the skin:* `expand(0.259)` is the contact the hang prints for this tee ("start within its
  0.259 cm contact off the body"); copy the number the build prints for yours.
- *Which rings:* both names are rings the file declares (`$Hem` is the tee's own last ring, `$HemUp` the ring above). A line or a
  `constraint()` as the place is refused, as is a name the garment makes below the guide.

**Refused by name:** a whole ring outside `settle() { }` ("a whole ring goes to a ring only while the cloth
hangs"); an unknown ring or place; the place a `constraint()`, a line, or `$Place@u` (no place along a
whole ring); a name the garment makes below.

**The honest limit.** The hem is held all round, on a ring about 9% shorter than the hem is. The extra
cloth has to go somewhere, and it piles into the hem as a crushed band rather than as a few soft folds. A
lift at only a few points (a hitch, `guide($Hem@0.1, $HemUp@0.1)`) is **not shipped**. Smaller residuals
the build does not hide: a small saw along the held edge (about 40 teeth over 0.2 cm on this tee), a
0.4 cm shelf at the back corners, and the row between held points about 4% short at its tenth
percentile. Use it for a tee that is pulled up and bunched; do not use it for a hem that is gathered at a
few places.

<!-- Source: GraphCodeGarmentDocument.cpp lines 4917-4936 (the settle braces), 6446-6470 (rings, place not a constraint), 6325 (outside-settle sentence); reports/hem-guide-slice-19nov.md, close-out (folds 17/12/6 against 2/2/2, 26 of 26 held at 0.000, the residuals, the hitch parked). -->

## A curtain

*Built, as far as this section says. The design is larger than the code (pinch pleats, a pole through
more than two points, a rim turned, a runtime curtain); this section lists only what builds.*

A curtain is its own kind of file: a `.gcgarment` whose header's `mesh:` is the **room** (a static mesh
holding the pole, the wall and the floor), and whose body holds only fabrics, tracks, panels and `build`
lines. It builds one static mesh a panel. The cloth hangs fresh in a Chaos run against the room, and what
you see is the folds the hang made, none drawn.

```gcgarment
# A small curtain on hooks. Source: Content/__MCPTests/Curtain/CurtainHooks.gcgarment, lines 8-19.
Linen = fabric(weight=239, thickness=0.77mm, stretch=(2.8%, 6.5%), shear=29, stiffness=(2.68, 2.10), friction=0.49)

Pole = track((-20, 16, 263.4), (20, 16, 263.4))   # a 40 cm stretch of the pole, centred

Small = panel(width=80, drop=60, fabric=$Linen) {
  heading(hooks, on=$Pole, every=10cm)
  settle(time=4, slide=2)
}

build {
  SM_CurtainHooks = $Small
}
```

| Word | What it says | Refused |
|---|---|---|
| `Pole = track((x, y, z), (x, y, z))` | the pole, two points typed on the room, in the room mesh's own centimetres | not two points; the same place twice; any body; `@sharp` |
| `Left = panel(width=250, drop=230, fabric=$Linen) { }` | a flat cut of that width and length from top to hem, in a `fabric()` of the file | a missing `width=`, `drop=` or `fabric=`; 0 or less; a fabric the file does not declare |
| `heading(hooks, on=$Pole, every=12.5cm)` | one hook every 12.5 cm along the pole, the cloth between them free | no `on=`, or neither `every=` nor `hooks=`; both; a second heading in a panel |
| `hooks=11` in place of `every=` | the count of hooks, 2 or more | under 2 |
| `settle(time=6, slide=2)` | how the curtain is hung; see below | any other word; a second `settle()` |
| `$Left.Top`, `.Hem`, `.Left`, `.Right` | the panel's four edges, as seen from the room | any other edge word |

What the heading works out for you (the build's note prints every number): the stretch is the pole's
length; **fullness** is the cut's `width=` over the stretch (2.00 on the example), and under 1 is refused
("the cloth cannot reach both ends"); the hooks sit a whole number of pitches apart, so an `every=` that
does not divide the stretch is not refused, the nearest whole number of pitches is built and the note says
so; and a pitch under 5 cm on the pole (two of the build's 2.5 cm steps) is refused, a hook that close has
no cloth to fold between it and the next. The cloth between two hooks, along the top, is the cut over the
pitches.

**How it hangs.** Three runs, each up to `time=` seconds: the cloth hangs flat from its hooks, the hooks
are drawn along the pole to their places, and the cloth settles. `time=` is more than 0 and at most 10,
**6 unwritten**; `slide=` is the seconds the hooks take (`slide=2` or `slide=2s`) or their speed
(`slide=15cm/s`), **15 cm/s unwritten**. Write no `settle()` and the curtain still hangs, on those
defaults, and the note prints them as defaults. `settle(off)` lays the curtain flat and runs no Chaos; it
takes nothing else. A dry run builds nothing and says "not hung: a dry run of a panel runs no Chaos".

**What a hang needs.** A hung panel's fabric must say `weight=`, `thickness=`, `stretch=`, `shear=`,
`stiffness=` and `friction=` (the missing ones are named); `settle(off)` reads none. A hang that does not
finish inside **300 seconds** is refused by name ("did not hang within the 300 s a curtain build may
take"), with the frames and seconds each run used; mesh it coarser, cut it smaller, or give it less
`time=`. The 250 x 230 cm curtain below built in 208 s on a busy machine (the same at 6 s a run, 265 s); the
80 x 60 one in about 10 s.

**A crease across the drop.** Draw a line straight across the panel and press it:

```gcgarment
# A pressed ridge at 76 cm. Source: Content/__MCPTests/Curtain/CurtainA0.gcgarment, lines 9-24 (the fixture's source comments elided).
Linen = fabric(weight=239, thickness=0.77mm, stretch=(2.8%, 6.5%), shear=29, stiffness=(2.68, 2.10), friction=0.49, crease=86)

Pole = track((-62.5, 16, 263.4), (62.5, 16, 263.4))   # the pole's 125 cm stretch, typed on the set

Left = panel(width=250, drop=230, fabric=$Linen) {
  heading(hooks, on=$Pole, every=12.5cm)
  Pack1 = line($Left.Left@76cm, $Left.Right@76cm)
  press($Pack1)
  settle(time=3, slide=2)
}

build {
  SM_CurtainA0 = $Left
}
```

The line runs from the panel's `.Left` edge to its `.Right` edge at one height (`@76cm` down from the top,
or a fraction such as `@0.33`), and `press($Pack1)` creases it. Refused by name: a line that is not
straight across at one height, a crease within 2.5 cm of the top, the hem or the heading's band, a
`press()` of anything but a line drawn above it in the panel, and a fabric with no `crease=` (the angle a
strip reopens to after a steam press, measured; `crease=86` in the example is a cotton shirting's figure
standing in for a linen's, which was not found: state your own), or a `crease=` under 35. As hung, the
pressed crease folds about 52 degrees (a median over its hinges) and the cloth below it keeps swinging
when the time is up; read the folds as a moving cloth's.

**Written, read, not built (say so in your file's comments; none is a result):**

- `heading(pinch(3), ...)` is read and then **refused at the build** ("its pleats are sewn before the
  hang ... hang it on `heading(hooks, ...)` for now"). So are `wave`, eyelets, pencil and rod pocket,
  which are not words at all.
- `depth=` on a heading makes one row of the cut at that depth; the band is **not stiffened**, and
  a heading's `fabric=` (its stiffening, written only with `depth=`) is read and not built. The build's note says so.
- `rim($Left.Hem, ...)` is read; the cut hangs **raw** there ("not yet turned").
- A track through more than two points, a panel built `[as=cloth]` for the game, and any ring or garment in
  the same file are refused.

<!-- Source: GraphCodeGarmentCurtainParse.cpp lines 199-211 (spellings), 212-410 (heading), 412-470 (settle), 500-560 (panel), 596-612 (track), 725-810 (the rule's refusals), 876-990 (fabric, crease); GraphCodeGarmentCurtainBuild.cpp lines 220-240 (what is read and not built), 406-420 (pinch refused), 626-632 (dry run); GraphCodeGarmentCurtainRun.h line 25 and CurtainRun.cpp 359-381 (300 s); Public/GraphCodeGarmentCurtain.h (the defaults 6 s, 15 cm/s, 10 s). Times: Docs/GC-CLOTH-CURTAIN-DESIGN.md section 16. -->
