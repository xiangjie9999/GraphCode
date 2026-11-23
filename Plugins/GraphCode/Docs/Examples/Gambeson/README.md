# Gambeson

**Status: Not rendered on the published 16 Nov build.**

## What it is

A quilted gambeson: a thick, stiff cloth (`thickness=` as a profile that varies down the body), sleeves, a front opening cut and sewn back, and a collar grown from the back of the neck.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `GambesonParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/Gambeson/GambesonParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `Gambeson.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Quilted = fabric(weight=1635, stretch=(1.4%, 3.25%), shear=58, stiffness=(14, 12), friction=0.49)` | a heavy quilted cloth |
| `Wall = profile(0=1.9, 0.209=2.5, 0.5=1.3, 1=2.5)` | wall thickness as a profile down the body |
| `LeftFront = cut($Front)` | a front cut open |
| `sew($Front, $LeftFront)` | and sewn shut |
| `Collar = extend($BackNeck, 5.0)` | a collar grown from an edge |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/Gambeson/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
