# Design the interface as part of the game

Read this before building a HUD, menu, inventory, dialogue, settings screen or onboarding flow.
The interface is part of the player's experience from the first playable slice. A working graph
with a hard-to-find action is unfinished. Use [UI-TOOLS.md](UI-TOOLS.md) for outside design tools
and the Unreal handoff; use [GAMEPLAY.md](GAMEPLAY.md) for the state the interface displays.

The quality principle in [ASSET-QUALITY.md](ASSET-QUALITY.md) applies: references, deliberate
design, one finished representative part, inspection and revision. Its physical construction
details are for physical assets. A clean interface can need very little ornament. Extra borders,
textures, gradients and animation do not make an unclear screen better.
Go beyond the minimum with theme-consistent craft: distinctive typography, composition, artwork,
state treatment and interaction polish unless simplicity is explicitly requested. Apply the
[shared quality loop](ASSET-QUALITY.md#apply-the-quality-bar-to-every-medium) to every screen,
state and transition, including actual interaction; decoration alone cannot satisfy it.

These are reusable methods, not a compulsory visual style. Preserve the game's existing design
language. An editor's compact properties panel, a couch-play adventure and a mobile puzzle need
different density, typography and navigation.

## 1. Start from the player's question

For each moment, answer: **What do I need to know? What can I do? What happened? What next?**
Design those answers before selecting widget classes. Make a screen list and connect the paths:

```text
Launch -> Title -> New game / Continue -> Play -> Outcome -> Retry / Next / Title
           |                              |
        Settings                    Pause -> Settings -> Back -> Resume
```

Adapt this to the game. Include first launch without a save, loading, empty content, failure,
cancellation, and returning from a submenu. A mocked-up happy-path screen leaves much of the UI
undesigned. Record the initial focus, Back action and input owner at every transition.

Use a small screen brief in the project's existing UI design note:

```text
Screen and entry condition:
Player's immediate question:
Primary action and secondary actions:
Data owner and update event:
Normal / empty / busy / unavailable / error states:
Initial focus, navigation, Back, and focus restored on exit:
Whether gameplay continues, pauses or is input-blocked:
Supported displays, input devices, languages and text scaling:
Reference image, accepted direction and runtime evidence:
```

A pause overlay that leaves online play running must communicate that. A Continue button needs
a real save check. An unavailable action needs either a useful explanation or to be absent when
it has no relevance; decide based on what the player needs to understand.

## 2. Establish a visual direction before multiplying screens

Collect a few relevant references, including one in motion. Annotate what each teaches:
information order, text treatment, spacing, navigation, feedback or integration with the game
world. Record source links. Borrow principles and make original assets; a reference screenshot
is not permission to extract its icons or font.

Make one representative screen with the actual kinds of data it will carry. Include a long
label, an unavailable action, keyboard/controller focus, and a busy gameplay background. This
screen should settle the design language before building ten variants. Compare the reference,
the design and the runtime capture at the same useful display size.

For a new interface, follow the researched workflow below and establish the game's art direction. Use ImageGen
or an authorized online image generator with the game's actual references to produce a complete
UI mockup plus palette/style sheet first. Refine the complete design, then generate its individual
transparent-background elements using that mockup and palette as references, following
[UI-TOOLS.md](UI-TOOLS.md#ai-workflow-from-game-reference-to-working-ui). HTML/CSS
remains the default interaction prototype when no suitable workflow already exists. Keep controls
and text live; judge the generated artwork inside that composition and then in Unreal.

### Apply the researched method to this game's art

User direction, clarified 2026-10-03: the guide author researches professional UI methods and turns
the findings into concrete steps for an AI making UI with image generation. **The research is
distilled here; execute [the AI workflow](UI-TOOLS.md#ai-workflow-from-game-reference-to-working-ui)
instead of starting another general UI research assignment.** Use ImageGen or authorized online
generation to closely match the game's artwork. Do not settle for a generic
AI-looking interface or manually approximate distinctive art with convenient primitive shapes.
Reuse accepted game assets and components when they already fit. Generated artwork is the preferred
route for new visual assets; existing precise vector symbols and live layout still have their jobs.

Apply these researched decisions to the actual player task, such as combat HUD legibility,
inventory comparison or controller navigation. The sources explain why the steps exist; reading
all of them again is not a prerequisite for routine UI work. Investigate only unresolved technical
or design questions under RESEARCH-AND-PLAN.md. Research basis and the resulting instructions:

- [Riot's UI design overview](https://www.riotgames.com/en/artedu/user-interface-design) connects
  audience needs, timely information, visual design and motion into a cohesive game experience.
  Decide what needs attention and when before decorating the screen.
- [VALORANT's interface redesign](https://playvalorant.com/en-us/news/game-updates/preview-the-future-of-valorant-s-interface/)
  describes auditing key flows, exploring style, incorporating game key art and removing decoration
  that weakened readability. Audit the target screen, carry over this game's art, and remove
  competing ornament before extending the component family.
- [Ubisoft's information-design discussion](https://news.ubisoft.com/en-us/article/3FpgxppxEUmS0dciV2vsY7/create-the-unknown-communicating-information-in-games-is-more-complicated-than-you-think)
  puts understandable, actionable information at the center of UI decisions. Give each visible
  element a player-facing purpose and test whether players can act on it.

These sources support the design principles. The image-generation workflow is GraphCode's
recommended application of the user's direction, not a claim that those studios used image AI.

Inspect the game's accepted gameplay captures, key art, characters, props and existing screens.
Create a compact art specification and reference sheet before generation:

| Visual property | Record from the actual game |
|---|---|
| Palette and values | Dominant/supporting colors, accent use, quiet backgrounds and contrast for content. |
| Shape and motifs | Silhouettes, curves/angles, border construction, recurring symbols and deliberate asymmetry. |
| Surface and rendering | Painterly or graphic treatment, edge softness, grain, material finish, lighting and depth cues. |
| Typography and icons | Licensed fonts by role, numeric clarity, symbol silhouettes and detail at final size. |
| Density and hierarchy | Where ornament belongs, where it must stay quiet, and which information leads. |
| Motion and sound | Weight, timing, transitions and feedback that belong to this game. |

Assign reference images clear roles: visual style, composition or an exact asset to preserve.
Feed them into a reference-capable generator rather than describing the project only as "fantasy"
or "sci-fi". When the art direction is unresolved, develop one representative direction before
expanding the asset family. Match established art closely and inspect the result; a prompt cannot
guarantee fidelity. Preserve important content areas and readability while carrying the style.

Reject unrelated glass cards, neon glows, repeated ornamental borders or template dashboards unless
the game's references call for them. Reject inconsistent bevels, lighting, icon detail, muddy
symbols, fake lettering and uncontrolled ornament. A visually restrained game still needs precise
art direction; richer does not mean busier. Use the generated-art review in UI-TOOLS for fixes.

Choose a small set of named design tokens, meaning shared values with a job:

| Token family | Decide once, then reuse |
|---|---|
| Typography | Display, heading, body, label and numeric roles; font assets, fallback characters, weights and size hierarchy. |
| Colour | Surface, primary text, secondary text, primary action, focus, warning and danger; never assign meaning by colour alone. |
| Spacing | A short scale for gaps and padding; relationship between a label, its value and the next group. |
| Shape | Corner treatment, panel borders, icon stroke/filled style and selected-state treatment. |
| Motion | A small set of durations/easing choices, what can interrupt them, and reduced-motion equivalents. |
| Layers | HUD, contextual prompt, menu, modal and notification order, including who receives input. |

Numeric values must fit the actual game and display target. Prefer named shared values over
unrelated per-widget guesses. Keep editable tokens in the existing style system or one suitable
shared asset. Document their meaning, not a second conflicting copy of every value.

## 3. Compose a screen with a clear order of attention

Place the most useful information where the player naturally looks during this task. Keep related
values together and align them consistently. Let whitespace separate groups. One primary action
should be obvious; secondary actions should remain findable without shouting equally loudly.

For a HUD, first draw the area the player needs for aiming, movement and hazards. Fit information
around it. Persistent HUD elements earn their space by supporting a recurring decision; temporary
messages should disappear or move to a log when their useful moment ends. A large reward banner
must not cover the next hazard. Test legibility during camera movement, particles and flashes.

For menus and inventory, establish a predictable reading and navigation order. Use real content
and show capacity, cost, selection and consequences where the decision is made. Keep tooltips
available through focus as well as hover. Avoid mysterious icon-only actions until their meaning
is established. Write labels as player actions: "Resume", "Equip", "Retry route".

Game identity comes through considered type, illustration, sound, shapes and motion. Do not paste
a generic website dashboard over a game or decorate every surface equally. A restrained survival
HUD and an expressive arcade results screen can both be excellent.

## 4. Build components with their whole behaviour

Make one reusable button, selectable row, prompt and relevant value display before cloning a
screen. Their states should be distinguishable without depending solely on colour:

| State | What the player needs |
|---|---|
| Rest | A readable action and a clear hit area. |
| Hover | Pointer acknowledgement without shifting surrounding layout. |
| Focus | An obvious keyboard/controller location, distinct from selection. |
| Pressed | Immediate acknowledgement; do not wait for a screen transition to show receipt. |
| Selected | A persistent indication of the chosen item or option. |
| Unavailable | A useful reason when discovery matters; prevent the action. |
| Busy | Honest progress or waiting feedback; handle repeat input deliberately. |
| Failed | What happened and a useful recovery action, without losing the player's context. |

An action awaiting a gameplay result is not already successful. Give immediate input feedback,
then show the confirmed outcome. The widget requests a purchase; the game's owner checks the
cost and grants it; the UI displays the result. Never deduct currency only inside a button graph.

## 5. Design navigation, focus and modality explicitly

Write the route for mouse, keyboard and every supported controller. Include the first focused
control, directional neighbours in irregular layouts, scrolling the focused item into view,
and return to the invoking control after Back. When an item disappears, choose a valid neighbour
or fallback. Do not leave focus on a hidden or destroyed widget.

Opening a modal must prevent accidental actions beneath it. Closing it must restore the previous
input state. Verify rapid open/close, holding an action while opening, losing application focus,
changing device and disconnecting a controller. Keep prompts tied to the active mappings.

CommonUI can help organize layered menus and input routing when it is already appropriate to
the project; enabling it alone does not establish correct navigation. Follow its version-matched
[input guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/commonui-input-technical-guide-for-unreal-engine?application_version=5.7)
and verify behaviour in play. A simpler existing UMG flow may already meet the need.

## 6. Make layout and accessibility part of the design

Use content-driven layout for lists and forms, anchors for relationships to the viewport, and a
deliberate DPI rule. Do not recreate an entire screenshot with unrelated absolute coordinates.
Check the smallest supported view and contrasting aspect ratios, plus safe areas where relevant.
Unreal provides [DPI scaling](https://dev.epicgames.com/documentation/en-us/unreal-engine/dpi-scaling-in-unreal-engine?application_version=5.7)
and [Safe Zone widgets](https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-safe-zones-in-unreal-engine?application_version=5.7);
neither removes the need to test the resulting composition.

Use Microsoft's game-specific guidance as a baseline, then inspect the actual presentation:

- [XAG 101: text](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/101)
  recommends at least 26 px for console text and 18 px for PC text at 1080p, with scaling up to
  200% of the minimum. These are rendered display targets, not numbers to paste into Unreal font
  properties. Test at the intended viewing distance with the chosen font and DPI configuration.
- [XAG 102: contrast](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/102)
  recommends 4.5:1 for standard important text/visuals and 3:1 for large ones. Measure against the
  lowest-contrast part of a variable background. Give translucent HUD text a dependable backing
  when necessary. Check focus, unavailable text and high-contrast mode as separate cases.
- [XAG 107: input](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/107)
  covers remapping and alternatives to difficult holds or combinations. Map actions, update their
  prompts, and prove the menu can be completed without a mouse when that input path is supported.

Use text, shape or icons alongside colour. Keep subtitles and critical cues clear of other HUD
content; give essential audio information a visual equivalent. Support the agreed reduced-motion,
text, subtitle, audio and control settings. Test long translations and missing glyphs early; keep
player-facing text localizable and outside flattened images. These checks are evidence of specific
behaviour, not a blanket accessibility certification.

## 7. Use motion and sound to explain changes

For each effect, name what it communicates: focus moved, a command was accepted, a cost was paid,
danger increased, a goal completed. Give higher-priority events stronger treatment. Repeated
navigation sounds should remain comfortable after many uses, and UI audio must respect settings.

Animate from the actual state and handle interruption. Pressing Back midway through an opening
transition must not strand an invisible input blocker. Rapid focus changes must not queue seconds
of movement. Avoid effects that delay a frequent action or make the interface drift while reading.
Reduced motion should preserve meaning through static emphasis or simpler transitions.

## 8. Finish by using it in the game

| Review | Evidence to keep |
|---|---|
| Visual | Viewed captures of the representative screen and its important states over real game backgrounds. |
| Navigation | A complete supported-device journey: enter, choose, confirm, Back, reopen and resume. |
| Adaptation | Narrow/wide supported views, larger text, long strings and safe-area cases. |
| State | Empty, full, busy, failed, unavailable, repeated and interrupted actions with correct feedback. |
| Integration | HUD reflects real state after respawn, travel, reopening and save/load where applicable. |
| Cost | Profile the busiest real screen and its transitions; record the target and measured result. |

Do not declare UI finished from a Widget Designer preview or a static mockup. Run its actual
input and data flow. Keep screenshots and observed problems in the project's design note and
remaining work on its board. Use [VALIDATION.md](VALIDATION.md) for release checks and
[GAME-SLICE-EXAMPLE.md](GAME-SLICE-EXAMPLE.md) for a connected example.

Core sources checked 2026-09-28; studio design research and art workflow updated 2026-10-03.
Epic links select UE 5.7 for this repository; choose the installed
engine version in another project. The workflow and examples are project guidance, not claims
that those sources prescribe a particular art style or guarantee fun.
