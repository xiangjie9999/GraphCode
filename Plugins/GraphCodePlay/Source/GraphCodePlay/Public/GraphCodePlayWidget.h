// Pressing the game's own UI — `RUNTIME-DRIVE-DESIGN.md` §12 R7, §2.7's `widget_activate` row.
//
// **THE HALF OF LIVE UI A PICTURE CANNOT DO.** A shot already shows the player's HUD exactly as they
// see it (`GraphCodePlayShot.h` — `bShowUI` is true, and the engine composites the game's own Slate
// window), so an agent can LOOK at a menu. It could not press one. That is this file.
//
// ---------------------------------------------------------------------------
// WHY IT IS NOT `WidgetIRLive`, WHICH ALREADY DOES THIS
// ---------------------------------------------------------------------------
//
// `WidgetIR` is `Type: "Editor"` and `GraphCodePlay` is `Type: "Runtime"` — a Runtime module may not
// depend on an Editor one, and Play's whole premise is the half that survives into a packaged game
// (`GraphCodePlay.Build.cs`). So the logic is here rather than borrowed, and the borrowing that DID
// happen is of the measured facts rather than the code: the precondition order, the two-pass hit
// test that tells *disabled ancestor* from *something on top*, and the refusal to broadcast a
// parameterised event are all `WidgetIRLive.cpp`'s findings, re-stated where a run can reach them.
//
// ---------------------------------------------------------------------------
// THE ONE PLACE THIS DELIBERATELY DISAGREES WITH THE EDITOR'S VERSION
// ---------------------------------------------------------------------------
//
// **A WINDOW ON TOP IS A WARNING HERE, NOT A REFUSAL.** `WidgetIRLive` asks Slate whether a real
// click at the widget's centre would land on it, across every top-level window, and refuses when it
// would not. That is right for the editor, where the windows over a widget are the ones a person is
// actually looking at. **In PIE the window stack contains the EDITOR** — its tabs, its notifications,
// a content browser somebody left floating — and none of those exist for the player whose HUD this
// is. Refusing on them would report the editor's furniture as a fault in the game.
//
// So the split is by whose fact it is. Conditions that belong to the GAME'S OWN UI refuse:
// unrealised, invisible, not hit-testable, zero area, disabled — including a disabled ANCESTOR,
// which only the second hit test can see. Conditions that belong to the window stack warn.

#pragma once

#include "CoreMinimal.h"

class UWidget;
class UUserWidget;
class UWorld;

/** What one press resolved to, tried, and reached. */
struct FGcPlayWidgetPress
{
	/** The `UUserWidget` holding the control — its object name, which is what tells two HUDs apart. */
	FString InstanceName;

	/** That widget's class, leaf only: `WBP_HUD`. */
	FString ClassToken;

	/** The authored name of the control itself — `StartButton`. */
	FString WidgetName;
	FString WidgetClassToken;

	/** The multicast delegate that was broadcast. `OnClicked` unless the caller named another. */
	FString EventName;

	/** True when the caller did not name an event and `OnClicked` was assumed. */
	bool bEventDefaulted = false;

	/** How many handlers the broadcast reached. **Zero is a real answer** — a button nobody wired. */
	int32 HandlersRun = 0;

	/** True only after the delegate actually fired. */
	bool bPressed = false;

	/**
	 * Whether a real player click could have landed on it, from the window stack's point of view.
	 *
	 * False does NOT mean the press was refused — see the header. It means something was over the
	 * control when this ran, which in PIE is usually the editor rather than the game.
	 */
	bool bReachable = false;

	/** Why a real click would not have reached it. Empty when one would have. */
	FString UnreachableReason;
};

/**
 * Resolve and press a control in the running game's UI.
 *
 * Static and world-scoped: every lookup is filtered to the world being played, so an editor preview
 * of the same Widget Blueprint — which lives in the same process and answers the same class test —
 * can never be the thing that gets pressed.
 */
class GRAPHCODEPLAY_API FGraphCodePlayWidget
{
public:
	/**
	 * Split `WBP_HUD.StartButton` into its two halves, or accept a bare `StartButton`.
	 *
	 * The host half is optional because most games have one HUD and making a caller name it would be
	 * ceremony; when it is omitted and two widgets answer to the name, `Plan` refuses with both
	 * spelled out rather than picking one.
	 */
	static bool ParseTarget(const FString& Target, FString& OutHost, FString& OutWidget,
		FString& OutError);

	/**
	 * Resolve the control and read its state, WITHOUT firing anything.
	 *
	 * Same division of labour as `FGraphCodePlayCall::Plan`: a caller that can still be answered
	 * plans first, so a target that does not exist is refused in the reply to the call that named it
	 * rather than as a finding in a log minutes later.
	 */
	static bool Plan(UWorld* World, const FString& Target, const FString& EventName,
		FGcPlayWidgetPress& Out, TArray<FString>& OutErrors);

	/** Plan, then broadcast. Refuses on everything `Plan` refuses on, and on nothing else. */
	static bool Press(UWorld* World, const FString& Target, const FString& EventName,
		FGcPlayWidgetPress& Out, TArray<FString>& OutErrors);

	/**
	 * `/Game/UI/WBP_HUD.WBP_HUD_C` -> `WBP_HUD` — the name a person types, and this verb takes.
	 *
	 * Published rather than file-local because the `.gcui` sidecar names the same things this verb
	 * addresses, and a second copy would drift: the day a class suffix rule changes, a target that
	 * resolves would stop matching the row that advertised it ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	static FString ClassToken(const UObject* Object);

	/** Every `UUserWidget` alive in this world, in name order — what a refusal lists. */
	static void FindHosts(UWorld* World, TArray<UUserWidget*>& Out);
};
