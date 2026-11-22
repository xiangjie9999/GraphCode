# Long coat

**Status: Not rendered on the published 16 Nov build.**

Pictures rendered 16 Nov on an earlier build (not this file on the published build):

![front](images/front.png)

![back](images/back.png)

![three-quarter](images/threequarter.png)

## What it is

A wool coat: lapels and a collar grown and folded, pressed cuffs and pocket flaps, and buttons from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `LongCoatParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/LongCoat/LongCoatParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `LongCoat.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `WoolCoat = fabric(weight=398, thickness=2.28mm, stretch=(6.0%, 6.2%), shear=36.0, stiffness=(2.42, 2.02), friction=0.82, crease=81)` | a coat wool with a set crease (`crease=`) |
| `LapelR = extend($RightLap, $LapelWidth)` | a lapel grown past an edge |
| `fold($LapelR)` | and turned back |
| `press($LapelR)` | and pressed |
| `fold($Collar, stand=$CollarStand)` | a collar on a stand |
| `add($Parts.Button, at=$BeltL@3.23cm offset=3.23cm)` | buttons placed in a lattice |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/LongCoat/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
