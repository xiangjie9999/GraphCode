// What is in the picture, as text beside the picture — R6, reshaped and widened.
//
// **[USER, 2026-08-19]:** *"maybe live ui, but much more like having a sidecar beside? include all
// positions etc. so you can ask, what ui is this"*, then *"and maybe save all the actors pixels in
// the sidecar also?"*
//
// ---------------------------------------------------------------------------
// WHY THIS IS NOT §5.7'S `widgets { }` BLOCK — WHICH IT REPLACES
// ---------------------------------------------------------------------------
//
// §5.7 designed R6 as a property block inside a `.gcp`: `ScoreText.Text = "1250"`, answering *what
// does it say* and nothing else. That was written before `shot` existed. **A picture and a property
// list are two halves of one question and neither is useful alone** — the picture shows a button and
// cannot tell you its name, the property list names a button and cannot tell you it is behind a
// modal. What joins them is GEOMETRY, and §5.7 has no field for it.
//
// So the artifact is a sidecar named like the picture it belongs to — `boss_01.png` beside
// `boss_01.gcui` — and everything in it carries its rectangle in that picture's own pixels.
//
// ---------------------------------------------------------------------------
// TWO BLOCKS, BECAUSE A SCREENSHOT IS MOSTLY NOT UI
// ---------------------------------------------------------------------------
//
//   ui { }     — the widget tree: which widgets are up, where, what they say, what can be pressed
//   world { }  — the actors that project into the frame: where they are on screen and in the world
//
// **THE FRAME IS THE FILTER, and that is what makes the world half writable at all.** §5's ruling is
// that a capture with no selector is a file whose diff cannot be read — true, and it is about a
// capture, which has no frame. This file HAS one: an actor is in it if its bounds land in the
// picture. Nothing has to be selected because the photograph already selected.
//
// ---------------------------------------------------------------------------
// THE FOUR QUESTIONS, AND THE FOURTH IS WHY THIS LIVES IN PLAY
// ---------------------------------------------------------------------------
//
//   1. WHAT is on screen        — instance name and class, widgets and actors alike
//   2. WHERE                    — `rect=(x, y, w, h)` in the screenshot's own pixels
//   3. WHAT it says or holds    — text and scalar values, read live where the engine allows
//   4. WHAT COULD BE PRESSED    — `press=` names exactly what `activate` would accept
//
// **Point 4 is the seam and it is deliberate.** `GraphCodePlayWidget` refuses a press no player
// could have made, and before this file the only way to discover what WAS pressable was to try one
// and read the refusal. `press=` is computed by that verb's own rule, so the sidecar is the verb's
// index rather than a second opinion about it ([[a-read-answers-through-the-write-path]]).
//
// ---------------------------------------------------------------------------
// PIXELS, NOT SLATE UNITS — the conversion that makes the file worth having
// ---------------------------------------------------------------------------
//
// A widget's geometry is in WINDOW space at Slate's scale; a screenshot is in VIEWPORT pixels at the
// game's resolution. They differ by the window's position, by DPI, and by whatever the viewport is
// stretched to. `USlateBlueprintLibrary::LocalToViewport` divides by the viewport widget's local
// size and multiplies by `GetViewportSize()` — which IS the screenshot's resolution — so a rect
// written here can be cropped straight out of the PNG. An actor's rect goes through
// `UGameplayStatics::ProjectWorldToScreen`, which lands in the same space.
//
// A rect is therefore only meaningful under this file's `viewport:` line, and both are written or
// neither is.

#pragma once

#include "CoreMinimal.h"

class AActor;
class APlayerController;
class UWidget;
class UUserWidget;
class UWorld;

/** The text, and enough counts to see an empty answer for what it is. */
struct GRAPHCODEPLAY_API FGcPlayUiResult
{
	FString Text;

	/** Absolute path written, empty when the caller only asked for the text. */
	FString Path;

	/** Screen-level `UUserWidget` instances — nested ones are counted as elements of their owner. */
	int32 WidgetCount = 0;

	/** Every element inside them, the hosts themselves excluded. */
	int32 ElementCount = 0;

	/** How many carry a `press=` — what an `activate` could address right now. */
	int32 PressableCount = 0;

	/** Actors whose bounds project into the frame. */
	int32 ActorCount = 0;

	/** Of those, how many did not draw — projected into frame while occluded or culled. */
	int32 HiddenActorCount = 0;

	/** The viewport resolution the rects are in. Zero when there is no game viewport. */
	FIntPoint Viewport = FIntPoint::ZeroValue;
};

/**
 * Project what the player is looking at to text.
 *
 * Static and world-scoped for `GraphCodePlayWidget`'s reason: an editor preview of the same Widget
 * Blueprint is a live `UUserWidget` in this process, and a sidecar listing it would describe a
 * screen the player is not looking at.
 */
class GRAPHCODEPLAY_API FGraphCodePlayUi
{
public:
	/**
	 * The document. `ShotLeaf` names the picture this belongs to and may be empty.
	 *
	 * Never fails. A world with no UI and no camera is a real answer and writes a file saying so —
	 * the states that could be missing are named (`viewport: none`, an absent `world { }`) rather
	 * than left as an empty file a reader would take for a broken writer.
	 */
	static void Project(UWorld* World, const FString& ShotLeaf, FGcPlayUiResult& Out);

	/**
	 * Project and write beside a picture: `<stem>.gcui` next to `<stem>.png`.
	 *
	 * Called when the screenshot is REQUESTED rather than when it lands, because the engine reads
	 * the surface at the end of the frame it was asked in — so the layout and the projection this
	 * reads are the frame the picture shows. Reading it on the landed delegate would describe a
	 * later frame and quietly disagree with the image beside it.
	 */
	static bool WriteBeside(UWorld* World, const FString& ShotAbsolutePath, FGcPlayUiResult& Out,
		FString& OutError);

	/**
	 * The multicast delegates on this widget that take no parameters, sorted.
	 *
	 * **THE `press=` RULE, AND IT IS `GraphCodePlayWidget`'S OWN.** A parameterised UMG event
	 * reports a change the widget has already made, so `activate` refuses to broadcast one; listing
	 * it here would advertise a press guaranteed to be refused.
	 */
	static void PressableEvents(const UWidget& Widget, TArray<FString>& Out);

	/**
	 * An actor's bounds as a screen rectangle, or false when none of it is in front of the camera.
	 *
	 * **EIGHT CORNERS, NOT THE ORIGIN AND NOT FOUR.** A single projected point has no size; four
	 * bound only a quad that happens to face the camera, and an actor's box is rotated relative to
	 * the view in general. Eight corners with a 2-D min/max over the ones that projected is the
	 * cheapest thing that is actually true.
	 *
	 * `bOutClipped` is set when SOME corners fell behind the near plane. Their projections are
	 * garbage — `ProjectWorldToScreen` reports failure rather than a coordinate — so they are
	 * dropped, and the rect that survives is a real bound on a part of the actor rather than all of
	 * it. Saying so is the difference between a partial answer and a wrong one.
	 */
	static bool ScreenBounds(APlayerController* Controller, const AActor& Actor, FBox2D& OutRect,
		bool& bOutClipped);
};
