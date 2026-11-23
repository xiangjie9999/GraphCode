# Jeans

**Status: Not rendered on the published 16 Nov build.**

## What it is

Jeans in denim: two long legs of eleven rings added to a waist-and-crotch garment, a waistband, pressed seams, and belt loops, rivets, a tack button and a label patch placed from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `JeansParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/Jeans/JeansParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `Jeans.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Denim = fabric(weight=426.7, thickness=1.10mm, stretch=(3.2%, 2.3%), shear=314, stiffness=(4.02, 2.37), friction=0.53)` | a denim fabric line |
| `Jeans = garment($Waist, $Crotch, loose=100%, clearance=$Seat, fabric=$Denim) {` | a garment that adds its legs |
| `construct(rim=(turned=1.27cm, layers=3))` | turned hems, three layers |
| `band($Waist, width=4cm, fabric=$Denim, cut=100%)` | a waistband as a band |
| `press($Waist, -1cm)` | a pressed allowance |
| `add($Own.Rivet, at=$RivetRow@0.124)` | a rivet part |
| `add($Own.BeltLoop, at=$LoopRow@0.1225)` | a belt loop part |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/Jeans/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
