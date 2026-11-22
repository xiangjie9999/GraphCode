# Suit trousers

**Status: Not rendered on the published 16 Nov build.**

## What it is

Wool suit trousers: long legs of eleven rings, a waistband, and a pressed crease down each leg (a line drawn on the cloth, pressed with no width).

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `SuitTrousers.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `WoolSuit = fabric(weight=232, thickness=0.83mm, stretch=(2.96%, 4.68%), shear=32.25, stiffness=(1.83, 1.61), friction=0.815, crease=108)` | a wool with `crease=` stated |
| `CreaseR = line($Top_R@0.75, $Ankle_R@0.75)` | a crease line drawn on the cloth |
| `press($CreaseR)` | pressed with no width |
| `band($Waist, width=4cm, fabric=$WoolSuit, cut=100%)` | a waistband |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/SuitTrousers/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
