# Zip jacket

**Status: Not rendered on the published 16 Nov build.**

Pictures rendered 16 Nov on an earlier build (not this file on the published build):

![front](images/front.png)

![back](images/back.png)

![three-quarter](images/threequarter.png)

## What it is

A fleece zip jacket: rib cuffs, hem and a 7 cm collar band, flap pockets with snaps, a sleeve pocket, and a coil zip made of a chain, a slider and open halves placed from a parts file.

## What you need

- The T-posed body: build `../GarmentBodies/TPoseBody.gcshape` first (it makes `/Game/GraphCodeExamples/Garment/SM_TPoseBody`). Every ring in this file is measured on that body.
- `ZipJacketParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/ZipJacket/ZipJacketParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `ZipJacket.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `Fleece = fabric(weight=288, thickness=1.69mm, stretch=(7.27%, 29.93%), shear=26.6, stiffness=(0.80, 0.79), friction=1.92)` | a fleece |
| `band($NeckEdge, width=7cm, fabric=$Rib, cut=75%)` | a tall rib collar band |
| `cut($ZipEdgeR, $ZipEdgeL)` | the front cut open between two lines |
| `add($Zip.Chain, at=$Hem@0, repeat=47, step=(0, 0, 1))` | a part repeated down a line |
| `add($Zip.Slider, at=$ZipEdgeR@1)` | a slider part at a line's end |
| `add($Zip.OpenChain, at=$ZipEdgeR@0.983, rotate=(0, 0, -26.9))` | open zip teeth turned to the edge |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/ZipJacket/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
