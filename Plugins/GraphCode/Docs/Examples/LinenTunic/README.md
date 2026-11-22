# Linen tunic

**Status: Not rendered on the published 16 Nov build.**

## What it is

A linen tunic: a neck slit and neck rim, side slits with their own edge finish, sleeves grown from the armhole rings, and two ties from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `LinenTunicParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/LinenTunic/LinenTunicParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `LinenTunic.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Linen = fabric(weight=199.7, thickness=0.51mm, stretch=(2.8%, 6.5%), shear=29, stiffness=(2.68, 2.10), friction=0.49)` | a linen fabric line |
| `rim($NeckSlit, turned=0.4cm)` | an edge finished by name |
| `rim($Neck, turned=1cm, layers=3)` | a neck turned in three layers |
| `SleeveL = extend($Armhole_L, 47.52, 55.6%)` | a sleeve grown from an armhole ring |
| `settle(time=5)` | build-time hang |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/LinenTunic/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
