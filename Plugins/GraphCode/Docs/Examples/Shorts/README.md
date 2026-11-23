# Shorts

**Status: Not rendered on the published 16 Nov build.**

## What it is

Twill shorts: short legs added to a waist-and-crotch garment, turned hems, a waistband and belt loops; the tack button is the jeans' part, so `JeansParts.gcshape` is included.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `ShortsParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/Shorts/ShortsParts`, which the garment's `shape(...)` line names.
- `JeansParts.gcshape` (in this folder): the tack button.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `Shorts.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Shorts = garment($Waist, $Crotch, loose=100%, clearance=$Seat, fabric=$Twill) {` | legs added to a waist and crotch |
| `construct(rim=(turned=3cm, layers=3))` | turned hems |
| `band($Waist, width=4.2cm, fabric=$Twill, cut=100%)` | a waistband |
| `Own = shape('/Game/GraphCodeExamples/Garment/Shorts/JeansParts')` | a second parts file |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/Shorts/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
