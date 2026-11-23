# Joggers

**Status: Not rendered on the published 16 Nov build.**

## What it is

Fleece joggers: rib ankle bands, a gathered, elasticated waist, and pockets with pressed allowances. A drawcord's ends and eyelets come from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `JoggersParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/Joggers/JoggersParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `Joggers.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Elastic = fabric(weight=324, thickness=1.46mm, stretch=(122.47%, 119%), shear=11.74, stiffness=(1.18, 0.99), friction=1.75)` | an elastic waist fabric |
| `band($Ankle_R, width=5cm, fabric=$Rib, cut=60.2%)` | a rib ankle band |
| `match($Waist, 135.4%, gather, falloff=$Gathered)` | a waist gathered to 135.4 % of its length |
| `band($Waist, width=5.08cm, fabric=$Elastic, thickness=0.242cm, cut=93.2%)` | an elastic band |
| `press($PocketR, -1cm)` | a pocket allowance pressed |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/Joggers/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
