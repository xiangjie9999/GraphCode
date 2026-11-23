# Button-up shirt

**Status: Not rendered on the published 16 Nov build.**

**May fail to build on the 16 Nov build: under investigation.** Fixes were coded but are unverified.

## What it is

A woven button-up shirt in cotton shirting: banded cuffs with two buttons each, a front placket with eight buttons, and a collar folded to stand. The placket and collar are grown in a second, stiffer fabric (`ShirtingFused`) declared beside the shirting.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `ShirtButton.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/ButtonShirt/ShirtButton`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `ButtonShirt.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Shirting = fabric(weight=135.1, thickness=0.38mm, stretch=(1.5%, 7.6%), shear=106.52, stiffness=(1.97, 1.83), friction=0.49)` | a woven fabric stated in the fabric test's own units |
| `ShirtingFused = fabric(weight=154.56, thickness=0.70mm, stretch=(1.5%, 7.6%), shear=294, stiffness=(3.13, 3.06), friction=0.49)` | a stiffer, fused cloth for the placket and collar |
| `Placket = extend($PlacketSeam, 3.184, fabric=$ShirtingFused)` | the placket, grown from a seam line in its own fabric |
| `band($Wrist_L, width=5.83cm, fabric=$Shirting, cut=78.9%)` | a cuff as a band on the sleeve's edge |
| `fold($Collar, stand=3.18cm, gap=0.25cm)` | a collar folded to stand |
| `add($Btn.Button, at=$Placket@3.11cm offset=1.592cm)` | a Shape part (the button) placed by distance along a line |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/ButtonShirt/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
