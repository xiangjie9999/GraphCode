# Leather armour

**Status: Not rendered on the published 16 Nov build.**

## What it is

A cuirass-style leather armour: a hard leather cloth (4 mm), openings cut at the sides, a collar grown from the back of the neck, and parts from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `LeatherArmourParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/LeatherArmour/LeatherArmourParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `LeatherArmour.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `HardLeather = fabric(weight=4200, thickness=4mm, stretch=(3.46%, 8.07%), shear=1758.5, stiffness=(23, 23), friction=0.54)` | a stiff leather cloth |
| `Collar = extend($BackNeck, 6.26)` | a collar grown from an edge |
| `cut($GapFrontL, $GapBackL)` | an opening cut between two lines |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/LeatherArmour/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
