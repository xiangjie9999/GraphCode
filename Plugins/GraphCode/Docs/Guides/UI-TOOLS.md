# Use the right tools to make good game UI

Read after [UI-UX.md](UI-UX.md) when layout, illustration, animation or iteration would benefit
from tools outside Unreal. Use them to improve the result. GraphCode's authoring surface is not
a reason to accept a weak interface, and an outside mockup is not the finished game UI.

## Default: apply the researched workflow, generate the art, test the interface

User direction, 2026-09-28, clarified 2026-10-03: this guide distills professional UI research into
instructions for an AI using ImageGen or an authorized online image generator. Follow the steps
below to make artwork that closely matches the game's accepted art. This supersedes both optional
generated decoration after an HTML design and a generic instruction to "go research UI". Use the
[art specification](UI-UX.md#apply-the-researched-method-to-this-games-art) and a local
HTML/CSS/JavaScript prototype to test layout, states and navigation
unless the project already has a suitable workflow. It is a place to assemble and judge the art,
not a reason to replace it with generic CSS panels or manually guessed ornament.

**Generate the complete UI mockup and its palette first; only after refining that design generate
the respective transparent-background UI elements.** User clarification, 2026-10-03: "like let ai
generate a complete ui mockup, with palette etc. then only generate respective transparent
background UI elements". The accepted mockup is the visual source for the component kit.
Further clarification: "Like, professional figma way, but made using AI" and "Text box, button,
etc". Organize the work like a component-based design file: foundations, complete screen frames,
reusable components and state variants, then assembled instances. Use AI to create and refine
the visual designs and their artwork; retain editable layout rules and live behavior.
Place those components in the prototype and judge them against the mockup. Keep labels, values,
layout and controls live. Figma
and Penpot are optional alternatives when they already fit the project or the user prefers them;
do not require an account or a design-app detour to begin. Combine the artwork loop and browser
prototype, then complete the Unreal handoff. A plain HTML/CSS interface does not need Three.js unless a
live 3D context is part of the question being tested.

## AI workflow: from game reference to working UI

Execute this sequence for the requested screen. Use existing project decisions and accepted art;
ask only for an essential missing decision. Review at the owner's established cadence, without
inventing approval stops between routine stages. The linked professional sources in UI-UX explain
the method; targeted research is needed only when a specific unresolved problem remains.

1. **Read the game and the request.** Open actual gameplay captures, accepted key art and existing
   UI assets. Identify the screen's player task, required information, primary action, input
   devices and supported displays. Reuse existing components and fonts where suitable.
   **Produce:** the screen brief from UI-UX, including real labels/data examples and failure states.
2. **Extract the visual rules.** Select a compact set of complementary references and label their
   roles. Record palette, value hierarchy, shape/motifs, material treatment, edge style, typography
   and motion. Distinguish what must stay identical from what can vary.
   **Produce:** one reference sheet and a short art specification grounded in visible examples.
3. **Resolve the reading order and interaction.** Place the real content over a gameplay capture
   using an editable layout or simple wireframe. Mark safe gameplay areas, text zones, the main
   action and controller focus order. Check the longest label and essential empty/error states.
   **Produce:** a composition reference with explicit regions; this is layout guidance for the
   generator, not manually fabricated final artwork.
4. **Generate the complete screen mockup and palette.** Give ImageGen or the selected service the game
   references, composition reference and the art specification. Request the representative screen
   as a complete composition: background/gameplay context, panels, buttons, icons, intended text
   hierarchy, spacing and selected/focused treatment. Include a companion palette/style sheet with
   color swatches, their roles, typography direction and recurring motifs. Keep it separate from
   the actual screen area. Record verified color values as editable tokens; generated hex labels
   are not authoritative. Start with one deliberate direction; an existing accepted complete mockup
   can satisfy this step. Treat generated text as concept-only. Do not generate the component kit yet.
   **Produce:** a saved full-screen mockup and palette/style sheet tied to the prompt and inputs.
5. **Critique, then edit precisely.** Compare the candidate with the game at final display size.
   Check identity, hierarchy, detail, silhouette and whether the action remains obvious. Write
   specific differences, such as "border is twice as heavy as the reference". Edit the candidate
   to fix those differences while preserving the accepted areas. Do not restart with a broad
   "make it better" prompt. **Pass:** one coherent complete mockup and palette that fit the game
   and the screen's task. This is a quality check, not an extra permission checkpoint.
6. **Generate the respective transparent elements from the accepted mockup.** Inventory its
   components, then supply the accepted full mockup, palette and the relevant component crop to
   the generator as actual references. Request one isolated element per file, preserving its
   silhouette, proportions, colors, material, lighting and ornament. Generate panel/frame art,
   button artwork, portraits, illustrated icons and ornaments with genuine transparent backgrounds;
   request transparency through the tool's option as well as the prompt when supported. Remove
   mockup labels, values, neighboring controls and background scenery. Preserve intentional surface
   fills inside panels; transparency belongs outside their silhouette and in designed openings.
   A full-screen background is a separate layer and can remain opaque. Reuse exact source logos
   and symbols. Keep padding, border widths and state alignment consistent; verify nine-slice regions.
   Inspect the alpha channel and composite over light/dark backgrounds: no baked checkerboard,
   opaque backdrop, clipped glow or edge halo. Reassemble over the original background and compare
   with the accepted mockup; repair drift before generating more elements.
   **Produce:** individually named RGBA assets, their intended sizes and a component/state mapping.
7. **Assemble live UI and test it.** Put the artwork into HTML/CSS or the existing prototype with
   real fonts, dynamic values, buttons, hit areas and navigation. Implement hover/focus/pressed,
   selected, disabled, busy and failure states where relevant. Test resize, long text, contrast,
   controller/keyboard and Back. Repair art in the generator and behaviour in the UI as needed.
   **Pass:** a working representative screen with the intended visual identity and complete states.
8. **Implement and verify in Unreal.** Import the accepted art and build the supported UMG/widget
   structure through GraphCode. Connect actual gameplay state and input. Compare runtime captures
   against the accepted prototype; test motion, fonts, alpha, cropping, scale, navigation and cost.
   Repeat the player journey with actual data. **Produce:** working Unreal assets plus captures,
   source art, prompt/edit history and a list of any differences still requiring repair.

Expand to other screens only after the representative screen passes. Reuse its art kit and style
values so the game has one interface language. A full-screen image is a design target; a functioning
UI uses its artwork in components that can respond to state, input, localization and screen size.

### Build a design system from the mockup

Keep the accepted screens, palette/style sheet and component sheet together in the project's
existing design area. This is the Figma-style workflow the user requests; using Figma itself is
optional. The component sheet previews the family together; runtime exports are separate files.

- **Foundations:** named colors by role, actual licensed font choices and type sizes, spacing,
  corner/border treatments, icon sizes and focus treatment. Turn the accepted palette into shared
  editable values rather than repeatedly guessing colors from generated pictures.
- **Screen frames:** complete AI-generated compositions at the intended viewport, including the
  real kinds of content. Refine hierarchy and visual identity before deriving component artwork.
- **Components and variants:** identify each reusable control in the accepted screen; generate its
  isolated artwork with the mockup and palette as references. Make the relevant states together,
  holding dimensions, content insets, edge treatment and visual identity consistent.
- **Instances and layout:** reuse the same accepted components in every screen. Record padding,
  gaps, alignment, minimum sizes and stretch rules so labels and content can grow. A component
  revision should update its instances rather than creating unrelated button designs per screen.

| Component | AI artwork derived from the mockup | Live implementation and relevant variants |
|---|---|---|
| Button | Transparent surround, surface, border and optional ornament/icon; no baked label | Label, hit region and action; default, hover, focus, pressed, disabled and loading where needed. |
| Text box / text input | Field surface, border and optional leading/trailing decoration on transparent surroundings | Editable text, placeholder, caret, selection and validation; empty, filled, focused, disabled and error states. Preserve clear text insets. |
| Panel, card or dialogue box | Frame and surface with clear content regions; separate ornaments where useful | Resizing layout, title/body/choices and clipping; nine-slice-safe borders or an appropriate scalable treatment. |
| Tab, checkbox, toggle or selector | Aligned base and selected/checked indicator artwork | Actual selection state, focus, hit area and navigation; distinguish selected from focused and disabled. |
| Slider or progress bar | Separate track, fill, thumb and optional end caps | Real value drives fill/thumb; labels remain live; preserve contrast and alignment across the range. |
| Tooltip or notification | Separate background/frame and relevant illustrated symbol | Content, anchor, wrapping, timing and dismissal; verify long text and screen edges. |

Generate only the controls the screen needs. For each component, save a stable name, intended
dimensions, content insets, resize rules and state-to-file mapping. Keep variants aligned on the
same canvas. Use shared live tints/overlays where they preserve the design more faithfully than
regenerating every state. A generated component sheet establishes appearance, not working inputs.
Reassemble the exported elements with real fonts and compare the resulting screen to the mockup
before extending the kit. Correct mismatches in shape, palette, spacing or state consistency.

### Prompt the image AI as an art collaborator

Use the detailed asset brief below for export requirements. For the full mockup, its elements and edits,
adapt these instructions to the actual supplied images:

```text
Design request:
Image 1 is the game's visual style. Image 2 is the layout with protected text/action regions.
Create a complete <screen> UI mockup using image 1's <specific palette, edges, motifs,
surface treatment>. Preserve image 2's information hierarchy and clear content regions.
The primary action is <action>; the player must read <information> first.
Match the established game art; add no unrelated visual theme or invented controls.
Show the whole composition at <viewport dimensions>, including its gameplay/background context.
Provide a companion style sheet with palette swatches and their roles, type hierarchy and motifs.
This is the design target; individual transparent elements come after the mockup is refined.

Component request (only after the complete mockup passes review):
Image 1 is the accepted full mockup; image 2 is its palette; image 3 identifies <component>.
Generate only that element as a separate <dimensions> RGBA image on a transparent background.
Match its exact design, proportions, palette, border weight, lighting and ornament in the mockup.
Remove labels, numbers, surrounding UI and background scenery. Preserve <intentional panel fill>.
Keep <padding> for edges/glow and <clear region> for live text. Do not redesign or add decoration.

Revision request:
Use this accepted candidate as the edit target. Change only <specific visible defect> so it
matches <named property of the reference>. Preserve <layout, silhouette, palette, motif
positions and all already accepted regions>. Return the corrected asset at the same dimensions.
```

Before each call, supply the actual references through the tool's supported image inputs. A file
name mentioned in prompt text alone does not give a generator the image. Inspect the returned pixels
after every call; do not assume the requested preservation, transparency or proportions succeeded.

## What to learn from Three.js games

Three.js can render a game's 3D world while HTML and CSS supply a separate interface. Its official
[HTML overlay example](https://threejs.org/manual/pages/align-html-elements-to-3d.html) demonstrates
styled HTML elements positioned against a 3D view. That is one available approach, not proof of
how every impressive web game was made. Canvas and other UI approaches are also possible.

The useful lesson is to treat layout, typography, assets, transitions and feedback as designed
work. Browser tools can make those quick to inspect and revise. Bring that discipline into Unreal;
do not infer that changing renderer would fix an undesigned menu. Three.js's own
[game-making manual](https://threejs.org/manual/pages/game.html) also distinguishes its rendering
role from the other systems a game needs.

For a browser game the web UI may be the final runtime. For an Unreal game, an HTML/CSS prototype
is usually a design artifact that will be rebuilt in UMG. Keeping a web renderer inside the game
is a separate architecture choice with input, packaging, performance, platform and accessibility
consequences. Prove that choice in a packaged slice before depending on it. Do not switch a user's
engine or add a runtime browser merely because a reference looks good.

## Choose by the job

| Job | Useful tool or medium | Deliverable that survives the handoff |
|---|---|---|
| Compare compositions and visual directions | Reference-based generation of a complete UI mockup and companion palette/style sheet | Accepted screen composition and palette before transparent component generation. |
| Test menu flow and transitions | Local HTML/CSS/JavaScript; optional Figma/Penpot prototype | Navigable flow plus explicit behaviour and state definitions. |
| Explore responsive HUD over moving 3D | Local web prototype, adding Three.js only if the 3D context matters | Reproducible source, reference captures, aspect-ratio and interaction findings. |
| Make illustrated icons, frames and ornament matching the game | Reference-based image generation; vector cleanup where the runtime needs scalable geometry | A consistent art family, separate exports and preserved source. |
| Extend exact symbols or an existing vector icon system | Existing vector source/editor or design app | Precise geometry, consistent strokes and separate runtime exports. |
| Make painterly panels, key art and textures | ImageGen or an authorized online generator using accepted references; reuse suitable game art | Source output, clean exports and provenance. |
| Implement final Unreal interaction and data | UMG, the project's input system, optional CommonUI/MVVM where justified | Real widgets connected to game state and verified in play. |

Availability changes by session. Discover installed skills, connectors, browser tools and local
applications before promising a tool-specific path. Read their current help or skill instructions.
Use an existing appropriate tool; do not silently buy a subscription or introduce a new service.
If a connector is unavailable, investigate the authorized online/API route below. Continue layout
work where independent, but do not label placeholder art as finished or quietly substitute generic
hand-crafted decoration. Generation budgets and content rights follow [CONTENT.md](CONTENT.md).

## Assemble and test in a local browser prototype

After the mockup and component stages, use this to judge layout, transitions or state changes.
An earlier wireframe may test information order; the accepted generated mockup guides the final art.
Keep it in the project's design/prototype area, outside Unreal's cooked content, with launch
instructions. Use local dependencies or the project's established workflow so the result remains
reproducible. A prototype should not need a public deployment to be reviewed.

1. State the question: for example, can the contract screen remain readable over the moving city
   view, and can a player find Accept without coaching?
2. Build a small HTML/CSS component system with named style values, real text and deliberate
   focus styles. Use semantic controls. Add a 3D scene only when it answers that question; an
   existing gameplay capture can be a sufficient background for layout exploration.
3. Feed the UI explicit sample states: no contracts, one selected, unavailable, waiting, failed,
   completed. Label this as sample data. Provide a simple way for the reviewer to reach each state.
4. Implement enter, confirm, cancel and Back. If testing gamepad behaviour, implement and actually
   exercise it; keyboard operation does not establish controller support. Keep pointer events
   from leaking through menus to the scene beneath.
5. Test resizing, text scaling, long strings, focus and interruption. In a browser, respect
   [prefers-reduced-motion](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference/At-rules/@media/prefers-reduced-motion)
   and provide a deliberate reduced-motion presentation. Test both modes.
6. Capture the accepted states and record the layout rules, timings and behaviours. State which
   tests used a browser and which remain owed in Unreal. Use the same state cases for the port.

A browser UI can prove visual direction and prototype behaviour. It cannot prove cooked fonts,
Unreal input routing, game saves, GPU cost in the final scene, or a packaged console build.

## Make generated artwork serve the interface

Use the available image tool and its current instructions; the product names above are examples,
not a claim that a particular service is connected. The user's chosen service, existing budgets
and permissions govern. Use an authorized equivalent when appropriate, and preserve provenance
through [CONTENT.md](CONTENT.md). Do not silently introduce a paid generation service.

1. **Start from the accepted complete mockup and palette.** Finish steps 1–5 above before generating
   individual elements. Open the mockup and actual game references; inventory its background,
   frames, icons, portraits, ornaments and state overlays. Do not invent a new design per asset.
2. **Specify each asset's job.** Record final display size/range, aspect ratio, crop, clear areas
   for live content, transparent regions and resizable borders. Distinguish a style reference from
   an edit target whose geometry must be preserved. Request separate files for independently
   moving/resizing parts; do not assume a generated contact sheet is an aligned production atlas.
3. **Generate from the accepted mockup and palette.** Supply the full design and component crop.
   Use reference-conditioned generation or editing with
   explicit palette, shape, surface and edge constraints. Use ImageGen when available, or the
   user's authorized online generator. Check that the chosen service actually accepts the needed
   reference/edit inputs. Ask for genuine transparency for cutouts and preserve alpha. Do not
   accept a painted checkerboard as transparency.
4. **Finish one representative family.** Produce one screen's necessary art and assemble it with
   real labels, numbers, hit areas and focus. Compare to the accepted game art at the intended
   display size and over actual gameplay backgrounds. Inspect every generated edge and motif;
   repair or regenerate malformed details before extending the family.
5. **Derive variants from accepted assets.** Reuse the accepted source as an edit/reference input.
   State what stays fixed: silhouette, margins, scale, perspective, light direction and motif
   placement. Change only the requested property. For hover/pressed/focus states, preserve alignment;
   a live tint, overlay or animation may be more consistent than independent full-image generations.
   Verify each variation against the base. Similar prompts or the same seed do not prove consistency.
6. **Keep exact content live.** Use licensed fonts for localized text, numbers and prompts. Reuse
   exact existing logos/symbols from their source; preserve the established vector system where
   applicable. Illustrated item icons can be generated. Keep their meaning recognizable at small
   size and their interaction states separate from the illustration.
7. **Export, import and inspect.** Retain originals, references, prompt/edit history, provider/model,
   dimensions and export settings. Prepare clean runtime files and test alpha edges, nine-slice
   corners, cropping, compression and color in Unreal. Check the family side by side and in motion.

Example art brief (replace the style with the project's own):

```text
Purpose: <specific frame, background, portrait or illustrated icon> for <screen/action>.
Input image 1: accepted game art; match its palette, material treatment and edge language.
Input image 2: layout reference; preserve the marked content area and component proportions.
Input image 3, if editing: accepted asset; keep its silhouette, margins and motif placement.
Style constraints: <observable properties from the art specification, not a genre label>.
Composition: <ornament placement>; quiet space behind <live content>; <crop/resize behaviour>.
Output: <dimensions/aspect>; <transparent or opaque>; separate asset, no baked UI text or values.
Edit: <one intended change>; preserve <explicit invariants>.
Review: <actual display size>, longest label, controller focus, bright/dark gameplay backgrounds.
```

A generated image of a complete UI can establish the visual target; label it as a concept.
Extract its design intent into real components before calling the screen
interactive. An image can be excellent final artwork without being the whole interface.

### Reject generic output and prove fidelity

Compare the reference, accepted art and runtime screen side by side. For layout/edge alignment,
use an overlay at matching size where useful. Judge these independently:

| Review | Reject and repair |
|---|---|
| Identity | A palette, motif, material, lighting style or decorative language unrelated to the game. |
| Craft | Melted ornament, broken symmetry where symmetry is intended, muddy silhouettes, halos or fake text. |
| Family consistency | Different bevel widths, perspective, light direction, border thickness or icon scale across the set. |
| Hierarchy | Artwork that competes with actions, obscures gameplay or leaves no quiet area for readable labels. |
| States | Focus/selected/pressed variants that shift shape, are indistinguishable, or rely only on color. |
| Adaptation | Stretched corners, clipped decoration, blurred small icons or failure with long translations/larger text. |

Repair with targeted edits to the accepted source, then repeat the same in-context check. Do not
expand to all screens while the representative family still drifts from the game's art. Record
intentional adaptations; no generator promises an exact match simply because references were supplied.
Use the [contrast and readability checks](UI-UX.md#6-make-layout-and-accessibility-part-of-the-design)
throughout. Neither a beautiful image nor a working greybox alone closes the UI task.

## API-based image generation is a supported route

An agent can call a suitable image-generation service directly when that is the available or
better workflow. An absent image-generation connector does not make outside generation impossible.
Use the existing authorized provider and configured credentials where available. API-based
generation is an intended authoring option. When the current project's user has already authorized
it, do not ask again merely because the transport is an API. Missing credentials, a new unapproved
spend or an external upload outside the existing scope are separate decisions. Continue the HTML
and component work while resolving any genuinely missing requirement.

Read the provider's current official API documentation before choosing an endpoint or model.
Examples of documented routes include
[Google's Imagen guide](https://docs.cloud.google.com/vertex-ai/generative-ai/docs/image/generate-images),
[Black Forest Labs' API quickstart](https://help.bfl.ai/articles/8446125349-quickstart-guide), and
[fal's queue API](https://fal.ai/docs/documentation/model-apis/inference/queue).
These are alternatives to investigate, not a requirement to open three accounts. Google documents
model migrations, and queued services have their own job/result protocol; never freeze a model ID
or assume one provider's request schema works on another. Verify current capability, price and
usage terms for the chosen route at execution time.

For matching established art, choose a route with reference-image/editing support. Provider examples
include [Imagen subject references](https://cloud.google.com/vertex-ai/generative-ai/docs/image/subject-customization)
and [BFL multi-reference editing](https://help.bfl.ai/articles/6546682167-what-is-multi-reference-editing).
Label each image's role and inspect preservation after every edit. These capabilities assist style
matching; they do not replace visual review or guarantee a production-ready family.

The integration should be small and reproducible:

```text
Art brief + output requirements + chosen provider/model
  -> load credentials through the existing secret/environment mechanism
  -> submit one deliberate request; save non-secret request metadata
  -> if queued, retain the job identifier and poll its documented status
  -> obtain the completed image through the documented response/download path
  -> save the original and inspect the actual image
  -> put it into the HTML prototype, review and revise
  -> import the accepted asset, compare in Unreal and record provenance
```

Handle the provider's actual response form: image bytes, encoded image data, a result URL or a
queued job are not interchangeable. Keep service keys out of browser code, HTML, logs and the
packaged game; generation belongs in a local authoring script or existing backend. The running
UI uses the saved artwork. Follow [CONTENT.md](CONTENT.md) for credential handling and asset logs.

For queued jobs, distinguish accepted, running, completed and failed states using that API's
documented values. Preserve the job ID after a timeout so the next attempt checks the existing
job instead of blindly submitting another paid generation. Use bounded waits, provider-appropriate
backoff and rate-limit handling. A failed status or filtered result is not an image. Check output
type, dimensions and decodability, then visually inspect it; do not import an error document saved
with a `.png` extension. Download accepted output before a temporary result link expires, subject
to the tool/provider's download rules.

Record provider/model, generation date, prompt and authorized reference inputs, relevant settings,
request ID when appropriate, source file, accepted export and Unreal destination. Preserve the
actual file: a prompt and seed alone may not reproduce the same result after a provider update.
Revise one representative asset before spending on a whole set. Reuse already accepted artwork
when it fits rather than regenerating the same panel for every screen.

## Optional: a visual design app and clickable prototype

Use this when it fits the user's tools or an existing design better than the default browser route.

1. Create a representative screen at the target display shape with realistic content. Use
   layout relationships, not merely a group of fixed rectangles. Figma's
   [auto layout](https://help.figma.com/hc/en-us/articles/360040451373-Guide-to-auto-layout)
   can express resizing, gaps and padding; test longer labels and narrower frames.
2. Create shared components and the states listed in [UI-UX.md](UI-UX.md). Name colours, text
   styles and spacing by purpose. Make the focused and unavailable variants now.
3. Connect the actual journey, including Back and failure. Penpot supports linked boards and
   interactions for [prototyping](https://help.penpot.dev/user-guide/prototyping-testing/prototyping/).
   A clickable prototype can establish flow, but it does not demonstrate Unreal controller support.
4. Review the representative screen against the visual references. Iterate the design itself
   before exporting more art. Follow the owner's existing review cadence.
5. Export individual runtime assets and preserve the editable source. Figma supports formats
   including PNG and SVG; choose by content and destination using its
   [export guidance](https://help.figma.com/hc/en-us/articles/13402894554519-Export-formats-and-settings-for-static-designs).
   Preserve vector originals even when the runtime needs raster textures.

Do not export a whole menu as one image and call it implemented. Text, values, focus, hit regions
and layout need to remain live. Do not assume exported CSS, SVG or generated code imports as a
working UMG widget. Prototype interactions, font metrics and effects must be checked again in
the chosen runtime.

## Export a small, complete handoff

Keep this beside the design source, extending the project's existing asset log where possible:

```text
Source file and tool/version:
Screen/component and variant:
Exported file -> Unreal destination:
Dimensions, intended display range and transparency:
Font family/weight, language coverage and license:
Colour representation and any export conversion:
Resizable border/margin or aspect-ratio behaviour:
Layer order, clipping and hit area:
Motion: trigger, end state, duration/easing, interruption, reduced mode:
Data field, owner and update event:
Evidence: reference capture, runtime capture, differences still open:
```

Export icons consistently and inspect them at their actual displayed size. A giant clean export
can become a blurry or uneven small icon. Check transparent edges over both bright and dark
backgrounds. Keep panel corners undistorted with a suitable resizable border treatment. Retain
the original asset and import settings so later exports update a known destination.

## Translate intent into Unreal

These are conceptual counterparts, not a code converter or a promise of GraphCode coverage:

| Web/design concept | Unreal implementation to consider | Check after translation |
|---|---|---|
| Row, column, grid | Horizontal/Vertical/Grid containers and reusable widgets | Resize, long labels, alignment and overflow. |
| Screen-relative overlay | Anchors, suitable overlay/container and safe area | Supported aspect ratios and input hit testing. |
| Shared style tokens | Existing style assets, widget bases or suitable shared data | One change updates all intended instances. |
| Button/component states | Widget styles and explicit state presentation | Focus, pressed, selected, unavailable and busy remain distinct. |
| Resizable illustrated panel | UMG brush border/box treatment or a suitable material | Corners, seams and border widths at multiple sizes. |
| CSS transition | Widget animation, explicit tween or UI material as appropriate | Same meaning, interruption behaviour and measured cost. |
| UI data model | Gameplay owner -> events or a suitable viewmodel -> widget | Opening reads current state; later changes update correctly. |
| HTML/SVG illustration | Supported import path or intentional rasterization | No missing features, colour shifts or unexpected blur. |

Read the existing asset and ask the installed tools for help before authoring. Use `gc_describe`
to inspect the actual widget kind/blocks and `omits:` to identify unsupported facets. Author
through the public path described in [TOOLS.md](TOOLS.md), preserve current signatures and
read back the result. Consult the narrow grammar guide before implementation source. If the
surface lacks a needed operation, investigate an available native workflow within authorization
and document the gap; never invent a `ui_*` verb or quietly omit the behaviour.

For reactive values, an existing event-based design may suffice. Unreal's
[UMG Viewmodel](https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-viewmodel-for-unreal-engine?application_version=5.7)
provides another approach using field notifications. Check its installed status and limitations
before adopting it. Ordinary polling property bindings and notification-driven updates are
different mechanisms; do not treat all uses of the word "binding" as equivalent.

## Verify fidelity and cost in the target runtime

Port one representative component and screen first. Compare the design and in-game captures at
the same viewport and state: typography, padding, icon edges, colours, focus and motion. Record
intentional adaptations. A browser's blur, shadow or font rendering may not transfer identically.
Choose an explicit equivalent and inspect it rather than silently accepting a weaker result.

Epic's [UMG optimization guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/optimization-guidelines-for-umg-in-unreal-engine?application_version=5.7)
recommends event-driven updates, restrained Tick use, and avoiding deeply nested Canvas Panels.
Use containers for ordinary flow layout. Invalidation and retention solve particular rendering
costs; they are not universal switches to turn on without measurement. Profile the actual menu,
scrolling list and animations over the actual game. Check load/construction cost and inactive
widgets as well as steady-state frame time.

Complete the runtime review in [UI-UX.md](UI-UX.md), then the relevant package checks in
[VALIDATION.md](VALIDATION.md). A beautiful external prototype is a useful intermediate result;
completion of an Unreal UI task requires the functioning, inspected Unreal interface.

Core sources checked 2026-09-28; reference-based generation and studio research updated 2026-10-03.
Tool access, export support and engine integrations must be checked
again when used. These guides do not authorize purchases, uploads or publication beyond the
user's existing instructions.
