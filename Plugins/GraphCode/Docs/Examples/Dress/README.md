# Dress

**Status: Not rendered on the published 16 Nov build.**

**May fail to build on the 16 Nov build: under investigation.** Fixes were coded but are unverified.

## What it is

A poplin dress: a bodice with a skirt added, neck and armholes finished with bands, and a five-second settle.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `Dress.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Skirt = garment($Waist, $Hem, loose=100%, fabric=$Poplin)` | a skirt as its own garment |
| `add($Skirt)` | added to the bodice |
| `band($Neck, width=(1.25cm, 2.5cm), fabric=$Poplin, cut=100%)` | a band on the neckline |
| `band($Armhole_L, width=(1.25cm, 2.5cm), fabric=$Poplin, cut=100%)` | a band on the armhole |
| `settle(time=5)` | build-time hang |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/Dress/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
