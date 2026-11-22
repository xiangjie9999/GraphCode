#pragma once

#include "CoreMinimal.h"
#include "WidgetIRTypes.h"

class UUserWidget;

namespace WidgetIR
{
	/**
	 * One already-constructed instance of a Widget Blueprint that Slate is still holding.
	 *
	 * The difference from `ui_render`'s preview is the whole point of this file. A preview is a
	 * FRESH instance of the compiled class, so it shows what the asset WOULD look like; an instance
	 * listed here is the one somebody is actually looking at, with whatever state it has accumulated
	 * since it was opened. `EUW_GraphCodeVcs` rendered as `not refreshed` while the open tab said
	 * `1 file(s) need attention` - same asset, two states, and until now only one of them reachable.
	 */
	struct WIDGETIR_API FWidgetIRLiveInstance
	{
		/**
		 * `UUserWidget::GetName()`. The handle a caller passes back when it is unambiguous, which
		 * is most of the time and is not always: no coordinates, no Slate path, no minted id.
		 */
		FString InstanceName;
		FString ClassPath;
		/** Where the instance lives. Two instances of one class are told apart by this. */
		FString OuterPath;

		/**
		 * `UUserWidget::GetPathName()` - the handle that is ALWAYS unique, and the second one a
		 * caller may pass as `instance`.
		 *
		 * A BARE NAME IS NOT AN ADDRESS. Measured 2026-08-27 driving a real app: eight live
		 * `UW_ExtrudeProperties` and thirteen `BP_CabinetProperties` shared one name each, so the
		 * name refused and the object was visible and unreachable. Every list here prints this, and
		 * every list here accepts it back.
		 */
		FString Path;

		/** True when the instance holds a Slate widget, which is what "constructed" means here. */
		bool bHasSlateWidget = false;
		bool bInViewport = false;

		/** The window the instance is in. Empty when Slate is not showing it anywhere. */
		FString WindowTitle;

		/**
		 * Last painted position, in DESKTOP space - where a person would point at it.
		 *
		 * Converted, not passed through. Slate's "absolute" position for a widget inside a window is
		 * WINDOW space, which equals desktop space only when the window sits at the origin; a caller
		 * reading it as a screen position is right by accident. When there is no window there is
		 * nothing to convert against and this stays in the space the engine gave it.
		 */
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;

		/** Where the window itself is on the desktop, so the conversion above can be undone. */
		FVector2D WindowPosition = FVector2D::ZeroVector;

		bool bVisible = false;
		bool bEnabled = false;
		bool bHitTestable = false;
		bool bHasFocusedDescendant = false;

		/**
		 * Why this instance cannot be photographed, when it cannot. Empty means it can.
		 *
		 * Reported rather than filtered: an instance that exists but is not on screen is a different
		 * answer from no instance at all, and the caller's next move differs.
		 */
		FString NotOnScreenReason;
	};

	/** One widget inside a live instance, as it stands right now rather than as authored. */
	struct WIDGETIR_API FWidgetIRLiveWidgetState
	{
		FString Name;
		FString ClassToken;

		/** The authored enum spelling - Visible, Collapsed, HitTestInvisible and so on. */
		FString Visibility;
		bool bVisible = false;
		bool bEnabled = false;
		bool bHitTestable = false;
		bool bFocused = false;

		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;

		/**
		 * What the widget currently DISPLAYS, for the widgets that display text.
		 *
		 * Found through the engine's own `GetText` reflection rather than a list of classes this file
		 * maintains: a hand-written list is additive and cannot notice a class it has never heard of,
		 * and every text-bearing UMG widget already exposes a Blueprint-callable getter.
		 */
		bool bHasText = false;
		FString Text;

		TArray<FString> Notes;
	};

	struct WIDGETIR_API FWidgetIRLiveOptions
	{
		/** Which instance, when more than one is open. Empty picks the only one, or refuses. */
		FString InstanceName;

		/** Off returns state only, which is the mode that works over a slow link. */
		bool bCapture = true;

		/** Absolute directory for the PNG. Empty uses Saved/UnrealAIBridge/renders/<Asset>/live. */
		FString OutputDirectory;
		bool bWriteFile = true;
		bool bInlineImage = true;

		/** Cap on returned widget rows. 0 omits them. */
		int32 MaxWidgets = 200;
	};

	struct WIDGETIR_API FWidgetIRLiveObservation
	{
		FString AssetPath;
		FString ClassPath;
		FWidgetIRLiveInstance Instance;

		TArray<FWidgetIRLiveWidgetState> Widgets;
		int32 TotalWidgets = 0;

		bool bCaptured = false;
		FString ImagePath;
		FString ImageBase64;
		int32 ImageBytes = 0;
		int32 ImageWidth = 0;
		int32 ImageHeight = 0;

		TArray<FString> Warnings;
		TArray<FAuthoringDiagnostic> Diagnostics;
	};

	/**
	 * Whether a real mouse click on this widget would do anything, asked one condition at a time.
	 *
	 * Reported rather than reduced to a single bool, because "it did not activate" and "it activated
	 * something nobody could have clicked" are the two failures worth telling apart, and a caller
	 * gating on this needs to know WHICH condition a widget failed to know what to fix.
	 */
	struct WIDGETIR_API FWidgetIRClickPreconditions
	{
		/** The authored name exists in this instance's widget tree. */
		bool bResolved = false;
		/** It holds a Slate widget - the branch of the tree it is in was actually realised. */
		bool bRealised = false;
		/** Its instance is in a Slate window, so there is somewhere for a click to come from. */
		bool bInWindow = false;
		bool bVisible = false;

		/**
		 * `SWidget::IsEnabled()`, which is the widget's OWN flag and nothing more.
		 *
		 * Named `self` on purpose. Slate has no stored inherited-enabled state - a disabled parent
		 * disables its subtree at hit-test and paint time, not by writing to the children - so a
		 * button inside a disabled panel answers true here. `bReachable` is the chain-aware answer.
		 */
		bool bSelfEnabled = false;
		bool bHitTestVisible = false;
		/** It painted at a non-zero size, so there is a point to click. */
		bool bHasArea = false;

		/**
		 * A hit test at the widget's centre reaches it - the same call the engine's own mouse input
		 * makes, with the same enabled-status rule.
		 *
		 * This is the condition that subsumes the ancestor chain and every widget on top: Slate's
		 * hit grid truncates a bubble path at the first disabled ancestor, so a widget under a
		 * disabled panel is absent from the path even though its own flag says enabled. Asking the
		 * engine beats re-deriving the rule here, where a second copy could disagree with it.
		 */
		bool bReachable = false;

		/** What a click at that point would reach instead, when it is not this widget. */
		FString BlockedBy;

		/** The first unmet condition, as a sentence. Empty exactly when all of them are met. */
		FString UnmetReason;

		bool AllMet() const { return bResolved && UnmetReason.IsEmpty(); }
	};

	/** One multicast delegate a widget declares - what an authored event binds to. */
	struct WIDGETIR_API FWidgetIRInvocableEvent
	{
		FString Name;
		/** True when the signature takes nothing, which is the only shape this can fire. */
		bool bInvocable = false;
		/** The parameters, when it has them. This is why a parameterised event is refused. */
		FString Signature;
		/** How many handlers are listening. Zero is legal and invoking it does nothing. */
		int32 BoundHandlers = 0;
		/** `Object.Function` for each, so a caller can see whose code is about to run. */
		TArray<FString> Handlers;
	};

	struct WIDGETIR_API FWidgetIRInvokeOptions
	{
		/** Which instance, when more than one is open. Empty picks the only one, or refuses. */
		FString InstanceName;

		/** The authored widget name - the same handle the `.gc` document carries. Required. */
		FString WidgetName;

		/** Which event. Empty means the one a click fires, and refuses when there is no such thing. */
		FString EventName;

		/**
		 * Refuse when a real click could not have reached the widget.
		 *
		 * ON by default, and that default is the point: an activation that works on a collapsed or
		 * covered control is a gate that passes on a button nobody can press.
		 */
		bool bRequireClickable = true;

		/** Report what would happen and fire nothing. */
		bool bDryRun = false;
	};

	struct WIDGETIR_API FWidgetIRInvokeResult
	{
		FString AssetPath;
		FWidgetIRLiveInstance Instance;

		FString WidgetName;
		FString WidgetClassToken;

		FWidgetIRClickPreconditions Preconditions;
		/** Every multicast delegate the widget declares, invocable or not. */
		TArray<FWidgetIRInvocableEvent> Events;

		FString EventName;
		/** True when the event was chosen by the click rule rather than named by the caller. */
		bool bEventDefaulted = false;

		bool bInvoked = false;
		/**
		 * How many handlers the broadcast reached. Zero with `bInvoked` means nothing was bound.
		 * On a dry run it is how many a real call WOULD have reached, beside `bInvoked` false.
		 */
		int32 HandlersRun = 0;

		TArray<FString> Warnings;
	};

	/**
	 * Every constructed instance of this Widget Blueprint, whether or not it is on screen.
	 *
	 * Returns true when the asset resolved, even with no instances - "none are open" is an answer,
	 * not a failure. False means the path named nothing that could have instances.
	 */
	WIDGETIR_API bool FindLiveWidgetInstances(const FString& AssetPath,
		TArray<FWidgetIRLiveInstance>& OutInstances, TArray<FString>& OutErrors);

	/**
	 * Activate one widget in a live instance by its authored name.
	 *
	 * IT BROADCASTS THE BOUND DELEGATE AND SYNTHESISES NO INPUT. `IAutomationDriverModule::Enable()`
	 * takes the user's real mouse and keyboard away until `Disable()` (`RUNTIME-DRIVE-DESIGN.md` §2);
	 * this fires the same delegate a click would fire and leaves the machine alone, which is the
	 * entire reason it exists. It is also why the preconditions above are checked SEPARATELY: a
	 * broadcast does not care whether the widget is on screen, and a person does.
	 *
	 * WHAT RUNS IS THE WIDGET'S HANDLER, which can do anything a Blueprint can do - an Editor
	 * Utility Widget's button may delete assets or run a commandlet. The caller decides that, not
	 * this function; it reports what it is about to reach before reaching it.
	 *
	 * Returns false when nothing was invoked and says why. `bDryRun` returns true having fired
	 * nothing, with the preconditions and event list filled in.
	 */
	WIDGETIR_API bool InvokeLiveWidget(const FString& AssetPath, const FWidgetIRInvokeOptions& Options,
		FWidgetIRInvokeResult& OutResult, TArray<FString>& OutErrors);

	/**
	 * Observe one live instance: its current widget state, and a photograph of the pixels that are
	 * genuinely on screen.
	 *
	 * READ-ONLY, and more strictly than the preview render is. Nothing is constructed, nothing is
	 * ticked, no handler is invoked and no input is synthesised - `FSlateApplication::TakeScreenshot`
	 * redraws the widget's own window into a target and hands back its pixels. The user's mouse and
	 * keyboard are never taken; that is why this is not built on the Automation Driver.
	 *
	 * Needs an editor with a GPU AND the widget actually shown. Both refusals name themselves.
	 */
	WIDGETIR_API bool ObserveLiveWidget(const FString& AssetPath, const FWidgetIRLiveOptions& Options,
		FWidgetIRLiveObservation& OutObservation, TArray<FString>& OutErrors);
}
