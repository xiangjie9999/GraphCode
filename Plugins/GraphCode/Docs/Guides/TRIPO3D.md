# Superseded: Tripo Smart Mesh with GraphCode composition

**Retired by the user, 8 October 2026:** “and discard the working with tripo or 3d generative
model approach, since that one is very unreliable”. This applies to the authoring approach
generally, extending the earlier rifle-body replacement. Do not start or resume this route.
Follow [the current artistic-shape workflow](ARTISTIC-SHAPES.md), including iterative brush
sculpting for real depth, and [content intake](CONTENT.md).

Everything below is the earlier procedure retained as history. Its recommendations and API
snapshot are superseded for current production authoring; existing files are retained.

## Reference first

1. Define one semantic part and its boundary with neighbouring parts. List what
   must remain independently adjustable or movable, and what belongs to this mesh.
2. Make an isolated reference from the governing design. Show one complete object,
   clear compound curvature, a clean background and little occlusion. Neutral clay
   is useful for a geometry-only source. Inspect the image against the original.
3. Save the reference, exact prompt, version and assumptions before generation.
   A generated view is an interpretation; it does not certify hidden dimensions.
4. **Split into as many meaningful parts as the construction supports.** Never
   generate an entire object silhouette merely because the generator can. One
   source mesh has one material kind; separate steel, bronze, copper, wood,
   leather and glass. Same-material parts may remain separate to preserve their
   assembly interfaces. Keep handles and other controllable curved profiles SVG
   based; simple structured parts stay native. Use Tripo only for the particular
   compound surface that warrants it. Plan actual mating surfaces between parts.
5. **Generate the silhouette and curved volume only.** Do not ask Tripo to size
   dial pockets, muzzle bores, mounting holes, slots or other precision cuts.
   Remove those features from the reference. First import and size the resulting
   mesh, then construct the interfaces natively against the actual parts.

These were OraStudio production decisions from the Aether rifle brief (2026-10-08), superseded above.

## Credentials

Use an environment variable or a user-selected local secrets file outside the
repository. Never print the key, put it into a prompt, save authorization headers
in request manifests, or pass it to an unrelated service. Read it directly inside
the API process. Send bearer authentication only to the verified Tripo API host;
do not forward it to model-download URLs. [Official authentication](https://developers.tripo3d.ai/en/docs/authentication).

## Current API, verified 2026-10-08

Use v3 at `https://openapi.tripo3d.ai/v3`. Do not mix legacy v2 payloads with v3.
[Introduction](https://developers.tripo3d.ai/en/docs/introduction).

Upload the PNG/JPEG via multipart `POST /files`; use its returned `file_token`
as generation `input`. The documented image limit is 20 MB.
[File upload](https://developers.tripo3d.ai/en/docs/files).

For Smart Mesh, use the **P Series** generation route. P2 supports 48–50,000
triangles or 48–25,000 quads; P1 supports 50–20,000 faces. Pin the selected model.
For geometry-only work, explicitly set **both** `texture:false` and `pbr:false`:
PBR otherwise forces textures on. `orientation:align_image` has no effect with
textures off. Keep UV export if later materials need it. Avoid relying on automatic
size for mechanical fits. [P-series image API](https://developers.tripo3d.ai/en/docs/generation-image-to-model/p).

Example request for one detailed body candidate, not a universal budget:

```json
{
  "input": "<returned file_token>",
  "model": "P2-20260801",
  "model_seed": 3201,
  "face_limit": 20000,
  "quad": true,
  "texture": false,
  "pbr": false,
  "export_uv": true,
  "auto_size": false,
  "enable_image_autofix": false
}
```

Submit to `POST /generation/image-to-model`. Review limits/pricing before changing
models or starting a batch. P2 is labelled Preview as of the verification date.
[P-series image API](https://developers.tripo3d.ai/en/docs/generation-image-to-model/p).

Read `GET /account/balance` to validate credentials without generating an asset.
[Account API](https://developers.tripo3d.ai/en/docs/account).

Save `task_id` immediately, then query `GET /tasks/{task_id}` until success or
failure. A lost HTTP response is not proof that no paid task was created; reconcile
before resubmitting. Record credits consumed and failures. Download the returned
model promptly; result links can expire. Never treat a successful task as art
acceptance. [Task query](https://developers.tripo3d.ai/en/docs/task-query),
[quick start](https://developers.tripo3d.ai/en/docs/quick-start).

## Inspect and integrate

These are local production gates, not promises made by Tripo:

- Preserve the original returned model outside Content with request, source hash,
  model version, task ID and actual cost. Work on a copy.
- Inspect every side in neutral shading. Compare the original reference, input
  image and actual mesh. Check silhouette, thickness, holes, disconnected pieces,
  folded surfaces, normals and triangulated count. Inspect GLB images/textures to
  verify geometry-only delivery rather than assuming the request was honoured.
- Establish centimetres, axes and origin from the target assembly. Record the
  transformation. Avoid unrecorded nonuniform stretching to fake a precise fit.
- Plan real mating surfaces: bores, seats, tangs, mounting flats and pipe bosses.
  Generate the sculpted form; use native parametric construction to finish precise
  interfaces where the generated source is insufficient.
- Import through reviewed `gc_import` plans. Read and write through `gc_read` /
  `gc_apply`. Consult current shape grammar before composing an imported mesh.
  Assign existing authored materials; preserve the separate normal/relief work.
- A generated mesh has no GC procedural history for its internal topology.
  Expose meaningful assembly dimensions, placements and fitting parameters, and
  accurately describe which edits require regenerating or reshaping the source.
- Complete one semantic part and its connections before the next. A whole-object
  beauty render never excuses a failed individual joint. Keep the rejected source
  isolated; promote only after rendered and numerical checks.

The task-specific request helper, if used, belongs under `Tools/Research/Tripo`,
not the shipped plugin Tools junction. It is not the public authoring API.

### Measured Aether intake lesson

The 2026-10-08 P2 request for 20,000 quads returned mixed polygons (16,434 quads
plus 5,752 triangles) and nonmanifold regions. A successful generation and `quad:true`
are not a closure or topology guarantee. The user instead selected their own FBX.
Its small bad triangle patches were repaired locally on a derivative with zero
vertex movement, then checked again after FBX export/reimport. Prefer that narrow
repair when sufficient; do not remesh an accepted form by default. Evidence and
hashes: `Docs/Research/tripo-smart-mesh-intake.md`.
