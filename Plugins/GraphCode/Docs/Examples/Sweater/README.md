# Sweater

**Status: Not rendered on the published 16 Nov build.**

## What it is

A knit sweater: a thick knit with rib bands at the cuffs, hem and neck, laid on the shoulders and chest at 0 % and spanning below.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `Sweater.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Knit = fabric(weight=480, thickness=3.40mm, stretch=(20.75%, 20.75%), shear=25.17, stiffness=(2.47, 1.34), friction=2.22)` | a thick knit |
| `band($Cuff_L, width=4cm, fabric=$Rib, cut=85%)` | a rib cuff |
| `band($Hem, width=4cm, fabric=$Rib, cut=85%)` | a rib hem band |
| `band($Neck, width=2cm, fabric=$Rib, cut=75%)` | a rib neckband, cut 75 % |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/Sweater/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
