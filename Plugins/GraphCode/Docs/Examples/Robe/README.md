# Robe

**Status: Not rendered on the published 16 Nov build.**

## What it is

A twill robe: a hood grown and sewn, a face opening, a banded hem and sleeves, and a rope belt with a knot placed along the waist from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `RobeParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/Robe/RobeParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `Robe.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Twill = fabric(weight=281.88, thickness=0.70mm, stretch=(3.2%, 2.3%), shear=148, stiffness=(2.24, 1.84), friction=0.53)` | a twill |
| `sew($HoodL, $HoodR, match=ease)` | the hood sewn |
| `band($Hem, width=10.16cm, fabric=$Twill)` | a wide hem band |
| `add($Kit.CordLength, along=$Waist, repeat=48)` | a part repeated along a ring |
| `add($Kit.Knot, at=$Waist@0.25, hold=0)` | a knot part |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/Robe/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
