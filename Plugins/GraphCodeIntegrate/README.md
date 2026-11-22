# GraphCode Integrate

GraphCode Integrate connects external authoring tools to GraphCode. It is a separate Unreal
Editor plugin alongside World and Shape. It sends selected Blender static meshes and translates
their material graphs into editable Unreal materials, baking branches when translation is unavailable.

**Version 0.3.0 beta.** Tested on Windows, Blender 5.2.1 and Unreal 5.7.4. This is a working
first integration, not a universal Blender shader or renderer conversion.

## AI-operated workflow (default)

For a new machine, missing dependency or connection, read the installed
`Plugins/GraphCode/Docs/Guides/BLENDER-SETUP.md`. It identifies official downloads, what Production
already includes, saved-file versus live-scene routes, installation/configuration and readiness checks.

Ask the AI to import the desired Blender objects into Unreal. The AI handles setup, object selection,
export preparation, material conversion, delivery, and inspection of the Unreal result. Do not hand
the user a ZIP or menu-click checklist as the completion of an authorized import task.

For a saved `.blend`, run `Resources/export-mesh.py` with explicit `--object` names, then run the
packaged importer on its `integrate.json`: plan first, then `--apply --render`. These shipped scripts
work without installing the sidebar companion. For a connected live Blender session, inspect the
scene first and call `graphcode_integrate.mesh_export.export_meshes` through Blender Python.
Inspect conversion reports and verify the resulting dimensions, material assignments and render.

The AI checks units and prepares instances on an export copy, preserving the author's original.
The current exporter requires Unit Scale 1; changing only the source scene's unit setting is not
a valid physical-scale conversion. Explain a specific uncovered case if preparation cannot preserve
its geometry/material meaning. Routine preparation is the AI's work.

If the companion is needed, the AI installs or updates it and sets its Unreal project preference.
Preserve existing preferences and any open scene; use Blender's normal installation API. Verify the
enabled version and saved project setting in a fresh process. A genuinely unavailable connection or
permission should be reported specifically, after completing the available setup steps.

## Optional manual controls: install and send a material

1. Install both `GraphCode` and `GraphCodeIntegrate` in the Unreal project's `Plugins` folder.
   Enable them and open the project. Integrate uses Core's running MCP bridge.
2. In Blender, install the supplied `GraphCode-Integrate-Blender.zip` through
   Preferences > Add-ons > Install from Disk, then enable **GraphCode Integrate**.
3. In its preferences, select the Unreal **project folder**. The packaged importer and
   Core bridge include their runtime; no Node.js installation is needed.
4. Open Blender's Shader Editor, press **N**, and select the **GraphCode** tab. Choose a
   destination such as `/Game/Integrate/Brass/M_Brass` and press **Send to Unreal**.

**Translate only** reports anything it cannot translate. **Translate + bake fallback** keeps
supported nodes editable and bakes unsupported, object-independent UV branches. The default
bake size is 2048. **Open conversion report** names every translated output, baked branch,
frozen control and unmet requirement. **Export material bundle** prepares files without importing.

**Surface mapping: Strict** remains the default. **Engine approximation** additionally allows
legacy Subsurface and thin-translucent glass mappings; the report identifies the differences.
**Bake on active mesh** evaluates Object/Generated coordinates and ambient occlusion on a copy
of the evaluated mesh. Use non-overlapping UV0 in the 0–1 tile, and transfer that same mesh to
Unreal. This checkbox does not export the mesh automatically. The report records its identity,
transform and frozen controls; these textures cannot be reused faithfully on arbitrary meshes.

The original Blender material, groups and scene are preserved. Reimport updates the material
and its owned textures. A destination changed outside Integrate is protected: choose a new
destination to keep both versions. Keep `Saved/GraphCodeIntegrate/imports` with the project if
you want to continue managed reimports; deleting that ownership history does not grant permission
to overwrite existing assets.

## Optional manual controls: send meshes and their materials

In Blender's **3D View**, select mesh objects in Object Mode, press **N**, open **GraphCode**,
choose a destination folder such as `/Game/Integrate/Meshes`, and press **Send selected meshes
to Unreal**. **Export mesh bundle** prepares the same portable files without importing.
Update an older add-on by installing the new companion ZIP; a running Blender session does not
automatically load the updated Python code.

Each object becomes a Static Mesh in its own folder, with its translated materials assigned to
the correct slots. The exporter evaluates modifiers at the current frame, preserves UV channels,
exports normals/tangents and triangulates the mesh. Rotation and scale are baked around the
object origin, which becomes the asset pivot. Scene translation is recorded, not placed in a level.
Use **Unit Scale 1**: one Blender unit becomes 100 cm in Unreal.

Repeated sends update owned meshes and materials in place. Saved mesh bytes are checked as well
as their editable settings, so an external geometry edit is protected. Renaming an object or changing
the destination creates a different asset; deselecting an object does not delete an earlier import.
Keep the ownership records and stable sources under `Saved/GraphCodeIntegrate` for reimport.

This sends static geometry, not skeletons or animation. Armature-deformed meshes are refused.
It does not author LODs, collision or level placement. Additional UV channels are preserved in the
mesh, while translated materials currently use UV0. Geometry-dependent baking automatically uses
each source object; provide non-overlapping UV0 in the 0–1 tile when those branches need baking.
Generated instances must be realized on an export copy first (Geometry Nodes: **Realize Instances**);
the exporter refuses them rather than silently dropping part of a generator's result.
Mesh import leaves Nanite disabled; enable it separately for tessellated displacement materials.

## What the material adapter translates

- One active Principled BSDF: base colour, metallic, roughness, emission, IOR/specular and supported normals.
  Specular is `25 * IOR Level * ((IOR - 1) / (IOR + 1))²`; Unreal's scalar range limits F0 to 8%.
- Value/RGB controls and group inputs become stable Unreal material parameters. Node groups
  are flattened per instance, preserving separate input values.
- UV0 coordinates, image textures, basic scalar/vector math, mixes, RGB colour ramps,
  channel split/combine, Invert, Point mapping without rotation, and procedural Checker.
- Tangent-space Normal Map with editable strength, using UV0. The source and Unreal mesh
  must use matching UVs and tangent frames; Integrate does not guess a green-channel flip.
- Bump nodes, including a tangent normal beneath the bump, use editable world-space surface gradients.
  Height amplitude and strength are retained; Blender's finite-difference Filter Width is not reproduced.
- Scalar Object-space Displacement supports **Bump Only**, **Displacement Only** and **Both**.
- Alpha cutouts use Masked blending at 0.5. Fractional alpha is thresholded, not continuous transparency.
- Optional approximations: Subsurface colour/weight; Principled transmission and Glass/Principled
  mixtures as thin-translucent sheets. Subsurface radius/scale/IOR, independent mixed lobes,
  thick refraction and caustics are reported limitations. Translucent displacement becomes bump,
  since that surface cannot use Nanite tessellation; export displaced geometry for its silhouette.

Noise, Voronoi, Wave, Brick and other supported UV-dependent branches can be selectively baked
to linear EXR. Controls inside a baked branch are frozen at export and listed in the report;
controls outside that branch remain editable. EXR data preserves negative/HDR values but costs
more memory than a compressed colour texture. Imported source images retain their resolution.

## Real displacement in Unreal

Unreal supports actual geometry displacement through Nanite tessellation. Integrate maps
Blender Height to Unreal Displacement, Midlevel to Center, and Scale to Magnitude in centimetres.
This first version assumes **1 Blender unit = 1 metre**. Negative Scale reverses height and Center.
The material receives both tessellation and Nanite usage flags.

Use a Nanite-enabled mesh. Epic's documented project setup enables these console variables in
`DefaultEngine.ini`; the importer reports the requirement and does not change global renderer settings:

```ini
[ConsoleVariables]
r.Nanite.AllowTessellation=1
r.Nanite.Tessellation=1
```

Restart Unreal after changing startup settings. Height should stay within 0–1 for the declared
Nanite bounds. Scale and Midlevel must be constant, Midlevel within 0–1. Changing material
Magnitude remains supported through reimport. Geometry silhouette, collision and shading normals
are different concerns: tessellation does not bake new collision, and renderer normals are not
guaranteed identical to Blender. See [Epic's Nanite documentation](https://dev.epicgames.com/documentation/unreal-engine/working-with-naniteenabled-content).

## Explicit boundaries

Arbitrary shader mixtures, coat/sheen lobes, animated graphs, shader mapping of additional UV sets, UDIMs and
vector/world-space displacement need further mappings. Unsupported features produce a report.
Material-only mesh-dependent baking requires the explicit mesh option; mesh bundles use each source object.
UV baking covers the 0–1 tile; it does not make a nonseamless material
tile seamlessly. Blender and Unreal lighting and BRDFs differ even with identical surface inputs.

## Command line

Export from a separate Blender process, leaving the open scene alone:

```text
blender source.blend --background --python-exit-code 1 --python Resources/export-material.py -- --material "Brass Metal" --output <bundle-folder> --destination /Game/Integrate/Brass/M_Brass --mode hybrid
blender source.blend --background --python-exit-code 1 --python Resources/export-mesh.py -- --object "Chair" --object "Table" --output <bundle-folder> --destination /Game/Integrate/Furniture --mode hybrid
```

From the Unreal project folder, plan or import:

```text
Plugins/GraphCodeIntegrate/Resources/graphcode-integrate.exe --project . --bundle <bundle-folder>/integrate.json
Plugins/GraphCodeIntegrate/Resources/graphcode-integrate.exe --project . --bundle <bundle-folder>/integrate.json --apply --render
```

Material bundles contain `material.gc`, `integrate.json`, `report.json` and hashed textures.
Mesh bundles contain hashed FBXs, nested material bundles, slot assignments and a root manifest/report.
Work files, reimport sources, ownership records and previews live under
`Saved/GraphCodeIntegrate`. Unreal owns the canonical `.gc` sidecar written after applying the proposal.

## Development checks

`Tests/blender_tests.py` runs inside a factory-startup Blender session. `Tests/bundle.test.mjs`
runs with `node --test`. `Tests/unreal-integration.mjs` uses the running Unreal test editor and
the exported fixtures, followed by `Tests/compare_previews.py` inside Blender. The separate
`Tests/unreal-displacement.mjs` needs a Nanite tessellation-enabled test host.
The native product gate is `GraphCode.Integrate.Product`.
Mesh checks are `Tests/mesh_blender_tests.py`, `Tests/mesh-bundle.test.mjs` and
`Tests/unreal-mesh.mjs`; the last requires the exported fixtures and a running Unreal editor.

`Resources/vendor-client.mjs` rebuilds the distributable MCP client from Core's locked development
dependencies. Its bundled licences and versions are in `Resources/vendor`. Source integration
architecture and remaining coverage are recorded in `Docs/GC-INTEGRATE.md` in the repository.
