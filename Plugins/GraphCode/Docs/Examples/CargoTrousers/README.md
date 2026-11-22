# Cargo trousers

**Status: Not rendered on the published 16 Nov build.**

## What it is

Twill cargo trousers: two long legs of many rings each, a waistband, turned hems, and a button and belt loops from a parts file, hung from the hips for the build.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `CargoParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/CargoTrousers/CargoParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `CargoTrousers.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Twill = fabric(weight=281.88, thickness=0.70mm, stretch=(3.2%, 2.3%), shear=148, stiffness=(2.24, 1.84), friction=0.53)` | a woven twill's fabric line |
| `construct(rim=(turned=3.2cm, layers=2))` | hems turned 3.2 cm, two layers |
| `band($Waist, width=4.13cm, fabric=$Twill, cut=100%)` | a waistband as a band |
| `hang(from=$Hip)` | the held ring for the hang |
| `add($Kit.CargoPocket, at=$PocketAt_R@0, hold=0)` | a part placed from the parts file |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/CargoTrousers/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
