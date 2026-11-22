# Hooded cloak

**Status: Not rendered on the published 16 Nov build.**

## What it is

A wool cloak with a hood grown from each half of the neck and sewn together at the back, a face opening cut in it, and a front cut, settled for five seconds.

## What you need

- A MetaHuman character's complete body mesh. **No MetaHuman ships with the plugin.** The file's `mesh:` is the placeholder `/Game/YourProject/YourMetaHumanBody`: point it at your own body, then re-measure every ring on it.
- `CloakParts.gcshape` (in this folder): the hard parts the garment places. Its program path is `/Game/GraphCodeExamples/Garment/HoodedCloak/CloakParts`, which the garment's `shape(...)` line names.
- The plugin's Garment guides: [GARMENT.md](../../Guides/GARMENT.md) and [GARMENT-PIECES-AND-EDGES.md](../../Guides/GARMENT-PIECES-AND-EDGES.md).

## What it shows

The exact lines, from `HoodedCloak.gcgarment`:

| Line | What it demonstrates |
|---|---|
| `WoolCoat = fabric(weight=398, thickness=2.28mm, stretch=(6.0%, 6.2%), shear=36.0, stiffness=(2.42, 2.02), friction=0.82)` | a wool cloth |
| `HoodL = extend($HalfL, $HoodRise, 149%)` | half a hood grown from an edge |
| `sew($HoodL, $HoodR, match=ease)` | the halves sewn, the longer eased onto the shorter |
| `cut($Face)` | the face opening |
| `settle(time=5)` | build-time hang |

## Build it

1. Copy this folder to `Content/GraphCodeExamples/Garment/HoodedCloak/` in your project, and copy `../GarmentBodies/` to `Content/GraphCodeExamples/Garment/`. The paths inside the files use those places; change them together if you move the files.
2. Ask the tool for its current form first: `gc_describe` with `kind:"garment"`, then `gc_apply` and `gc_render` with `{ "help": true }`.
3. `gc_apply` the body, then the parts file (if any), then the garment, in that order. A `.gcgarment` is a whole program: apply it whole.
4. Read every note the build prints, then render it lit from the front, back, both sides and at a grazing angle ([GARMENT.md](../../Guides/GARMENT.md), "Verify the garment as a garment").

## Limits

- The long working notes that carried each number's source were removed from this copy; the numbers are those the garment was built with on the example body. Re-measure the rings on any other body.
- The paths in this copy (`/Game/GraphCodeExamples/Garment/...`) were rewritten from the project's test paths and have not been built in this form.
