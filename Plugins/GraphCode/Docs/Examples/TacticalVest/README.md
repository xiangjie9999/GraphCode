# Tactical vest

**Status: Not rendered on the published 16 Nov build.**

## What it is

A tactical vest in a Cordura nylon cloth: a sharp-cornered neckline ring, a thick shell, and bags and pouches placed from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `Tactical.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/TacticalVest/Tactical`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `TacticalVest.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Cordura = fabric(weight=241, stretch=(1.11%, 1.09%), shear=1558, stiffness=(5.10, 5.14), friction=0.27)` | a stiff nylon cloth |
| `Neck = ring((0.000, 10.259, 141.397), (7.300, 9.277, 141.397)@sharp, (7.300, -0.600, 147.485), (7.300, -9.277, 141.397)@sharp, (0.000, -10.259, 141...` | a ring with `@sharp` corners |
| `add($Kit.FrontBag, at=$PlateLine@0)` | a bag part placed at a ring |
| `add($Kit.MagPouch, at=$MagLine@0-7.62cm, gap=3.292, hold=0)` | a pouch held at its own place (`hold=0`) |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/TacticalVest/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
