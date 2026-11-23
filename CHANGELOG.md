# Changelog

Each release is published on the [Releases](https://github.com/xiangjie9999/GraphCode/releases) page with its
signed manifest, signature and archive. The editor offers new releases itself. Longer notes with pictures:
[graphcode.dev/whats-new](https://graphcode.dev/whats-new). All releases are for Unreal Engine 5.7.4, Win64.

## 0.1.14 (23 November 2026)

- **Move:** hold a sword or any handle firmly from three posed values (new guide HOLDING-A-SWORD and a SwordGrip example); faster balance builds and quicker collision checks; the motion-from-video tools now ship inside Move.
- **Shape:** new presets for animals (cats, dogs, heads, paws, eyes, noses) and nature (vegetation, understory, dead wood, rocks and cliffs, stone, concrete), plus rope and weather.
- **Market:** "Get in Unreal" on a pack's web page opens it straight in your editor.
- **Garment** and **MCP** updates.
- Carries our new hardware release key, `release-tpm-2`, so later releases can be signed with it.

Archive SHA-256: `88529c88e3ba4e17b9c6ac330d262e5a2f7350d4cedcaccab71754335ae29485` (131,838,784 bytes),
signed by `release-backup-1`.

## 0.1.13 (20 November 2026)

- **Garment:** collar and cuff bands wind cleanly; cut edges lie flat at the lay; new `hang()` solver words and
  `hang(spacing=)`; `extend(..., over)` lays a strip over a cut; `band(..., ends=)`; `settle() { guide($Hem,
  $HemUp) }` lifts a hem while it hangs; curtains (track, hooks, heading fullness, hang, slide, settle, a
  pressed crease); faster garment builds; new guide GARMENT-HANG-AND-CURTAINS.
- **Move, Shape and Core** updates.
- Shape presets now come from the Market; built preset meshes are no longer bundled, so the download is smaller.

Archive SHA-256: `213830bee5c08de3214113aca55642385c848e95f7adf9a2168c2596045f0076` (130,544,448 bytes),
signed by `release-backup-1`.

## 0.1.9 (17 November 2026)

- **Garment:** live-cloth (`[as=cloth]`) builds report every solver value written, layer pairs that sit too
  close to collide, which material slots keep an earlier material, and the exact command when the stitch
  material needs creating.

## 0.1.8 (16 November 2026)

- **Garment:** sewn-on pieces (pockets, belt loops), declared edge kinds, rounded turned corners and hem layer
  zones, pressed seams with their ridge, coat lapel and zip and hoodie fixes; a new garment guide and 20
  example garments.

## 0.1.7 (12 November 2026)

- **Hair cards:** a cleaner anime look by default: one soft, broken highlight ring across the locks, and
  outlines at the rim and around big masses. Existing toon materials rebuild from the new recipes on the next build.

## 0.1.6 (12 November 2026)

- **Move:** strikes by intent (a pose's travel names its point, when it is fastest and whether the limb is
  extended; `pace=`), relaxed parts that sway, whole-body human walk steps, door and kick work, cancelling a long build.
- **Garment:** sewn by the hang, physical folds, the common game garments, skinned accessories, a hooded cloak.
- **Groom:** anime hair cards with painted tiles, bands, clumps, combing and solvers.
- **Shape:** organic patches, lofts, outlines and SVG.
- **Rig and morph:** ragdoll limits saved to the asset, morph targets.
- **Effects:** the stacked-timed-layers method for spells and fire.

## 0.1.5 (2 November 2026)

- **Garment:** the common garments (T-shirt, jeans, hoodie, joggers, zip jacket, button shirt, cargo trousers,
  tactical vest, long coat, tank top, gambeson, leather armour, linen tunic, robe, dress) built from cited
  patterns, a measured body and measured fabrics; hangs two to four times faster; mirror-symmetric trousers;
  folded collars and lapels, stitches across pieces, part seating.

## 0.1.1 (20 October 2026)

- The first signed GraphCode release: all ten products ship together.
