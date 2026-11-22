# Tank top

**Status: Not rendered on the published 16 Nov build.**

## What it is

A jersey tank top: neck and armhole edges turned in three layers, hemmed, with the same jersey as the T-shirt (without its knit look).

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `TankTop.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Tank = garment($Neck, $Chest, $Hips, $Hem, loose@1=0%, loose@2=100%, loose@3=0%, fabric=$Jersey) {` | a garment with ramped looseness at three rings |
| `rim($Neck, turned=1.5cm, layers=3)` | an edge turned in 3 layers |
| `rim($Armhole_L, turned=1.5cm, layers=3)` | the armhole likewise |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/TankTop/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
