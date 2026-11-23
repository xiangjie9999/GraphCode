#pragma once

#include "CoreMinimal.h"
#include "WidgetIRTypes.h"

class UWidgetBlueprint;

namespace WidgetIR
{
	/** One size to lay the widget out at. Sizes are PIXELS; DpiScale converts them to layout units. */
	struct WIDGETIR_API FWidgetIRViewport
	{
		/** Caller's label, used for the PNG filename and to address the viewport in the result. */
		FString Name;
		int32 Width = 0;
		int32 Height = 0;

		/**
		 * 0 means "ask the project", which is the honest default: the DPI curve in
		 * UUserInterfaceSettings is what the game will actually apply at this size, and a render
		 * that silently used 1.0 would disagree with the shipped product at every size but one.
		 */
		float DpiScale = 0.0f;
	};

	/** Where one widget in the instantiated preview ended up. All values are post-paint facts. */
	struct WIDGETIR_API FWidgetIRRenderedWidget
	{
		/** The widget's name in the tree - the same token ui_read uses as an alias. */
		FString Name;
		FString ClassToken;

		/**
		 * WHAT THE AUTHOR WROTE, not what the preview is showing.
		 *
		 * `UWidget::GetVisibility()` answers from the cached Slate widget when there is one, and at
		 * design time `UWidget::SynchronizeProperties` has already overwritten that with
		 * `GetVisibilityInDesigner()` - which is `Visible` for everything except `bHiddenInDesigner`
		 * and never consults the authored property at all. Asking it therefore reported
		 * `visible:true` for a widget the asset says is `Collapsed`, which made the renderer
		 * useless for checking a show-only-when-needed UI: it always described the MAXIMAL row.
		 * Both fields below are read from the serialized `Visibility` property instead, so they say
		 * the same thing in both modes and agree with the `.gc`.
		 */
		FString Visibility;
		bool bVisible = false;

		/** Top-left in viewport pixels, and the painted size in pixels. */
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		/** What the widget asked for, in layout units, before its parent decided. */
		FVector2D DesiredSize = FVector2D::ZeroVector;

		/** Set when the widget is visible and painted but lies wholly outside the viewport rect. */
		bool bOffscreen = false;
		/** Set when a visible widget painted at zero width or height. */
		bool bZeroSize = false;
		/**
		 * Set when the picture shows a widget the author collapsed or hid.
		 *
		 * Only ever true under `bDesignTime`, and it is the engine's behaviour rather than ours -
		 * the Designer deliberately paints hidden widgets so they can be selected. The flag exists
		 * because the image cannot say it: a collapsed row and a shown one look identical in the
		 * PNG, so a caller reading the picture alone would conclude the game shows both.
		 * `design_time:false` paints what the game paints.
		 */
		bool bPaintedDespiteVisibility = false;

		TArray<FString> Notes;
	};

	/** The result of one viewport: an image, the layout that produced it, and what looked wrong. */
	struct WIDGETIR_API FWidgetIRViewportRender
	{
		FWidgetIRViewport Viewport;
		/** The DPI actually used, after the project curve resolved a 0. */
		float ResolvedDpiScale = 1.0f;

		bool bRendered = false;

		/** Absolute path of the written PNG. Empty when writing was disabled or failed. */
		FString ImagePath;
		int32 ImageBytes = 0;
		/** Populated only for the one viewport the caller asked to see inline. */
		FString ImageBase64;

		/** The root's desired size and what the viewport actually allotted it, in layout units. */
		FVector2D RootDesiredSize = FVector2D::ZeroVector;
		FVector2D RootAllottedSize = FVector2D::ZeroVector;

		TArray<FWidgetIRRenderedWidget> Widgets;
		TArray<FString> Warnings;
	};

	struct WIDGETIR_API FWidgetIRRenderOptions
	{
		/** One to MaxViewports entries. Empty means the single default viewport. */
		TArray<FWidgetIRViewport> Viewports;

		/**
		 * Design-time construction, matching what the Designer shows: preview text on bound
		 * properties, no runtime bindings firing. false instantiates the widget the way a game
		 * would, which is a different picture and occasionally the one being debugged.
		 *
		 * MATCHING THE DESIGNER INCLUDES ITS HIDDEN WIDGETS. `UWidget::SynchronizeProperties`
		 * binds a design-time widget's Slate visibility to `GetVisibilityInDesigner()`, so a
		 * `Collapsed` or `Hidden` widget is painted here exactly as the Designer paints it. Each
		 * one comes back with `bPaintedDespiteVisibility` and a note; `false` is the mode that
		 * answers "what will the player see".
		 */
		bool bDesignTime = true;

		/** Absolute directory for the PNGs. Empty uses Saved/UnrealAIBridge/renders/<Asset>. */
		FString OutputDirectory;
		bool bWriteFiles = true;

		/**
		 * Index into Viewports whose PNG comes back base64 in the result, or INDEX_NONE for none.
		 * One image, deliberately: eight of them is megabytes of response for a caller who wanted
		 * geometry, and the files are on disk either way.
		 */
		int32 InlineImageIndex = INDEX_NONE;

		/** Seconds handed to the paint pass. Fixed by default so two runs produce one picture. */
		float DeltaTime = 0.0f;

		static constexpr int32 MaxViewports = 8;
		static constexpr int32 MaxDimension = 7680;
		static constexpr int32 MinDimension = 1;
	};

	struct WIDGETIR_API FWidgetIRRenderResult
	{
		FString AssetPath;
		FString ClassPath;
		TArray<FWidgetIRViewportRender> Viewports;
		TArray<FAuthoringDiagnostic> Diagnostics;
	};

	/**
	 * Instantiate one Widget Blueprint off-screen, paint it at each viewport, and report the
	 * picture plus the layout that produced it.
	 *
	 * NOTHING IS DUPLICATED. The preview is an instance of the already-compiled generated class,
	 * not a copy of the asset, so this never enters `StaticDuplicateObjectEx` and never opens the
	 * window UI-AUTHORING-DESIGN.md section 23 documents - the one where a Blueprint duplication
	 * runs a garbage collection from inside itself while holding an annotation map keyed on raw
	 * object pointers. That is the whole reason ui_render can sweep a corpus that ui_apply cannot.
	 *
	 * Read-only in the sense that matters: no compile, no dirty, no save. The asset's package dirty
	 * flag is checked before and after and a change is reported as a diagnostic rather than
	 * shrugged off.
	 *
	 * Requires a real RHI and an initialised Slate application. Under -nullrhi - which is how the
	 * automation suite runs - this refuses with a message saying so instead of asserting inside the
	 * engine's renderer.
	 */
	WIDGETIR_API bool RenderWidgetBlueprint(const FString& AssetPath,
		const FWidgetIRRenderOptions& Options, FWidgetIRRenderResult& OutResult,
		TArray<FString>& OutErrors);

	/** The same render against an already-resolved asset, for callers that already have one. */
	WIDGETIR_API bool RenderWidgetBlueprintObject(UWidgetBlueprint* Blueprint,
		const FWidgetIRRenderOptions& Options, FWidgetIRRenderResult& OutResult,
		TArray<FString>& OutErrors);

	/**
	 * Whether this process can paint at all, and why not when it cannot.
	 *
	 * Separated from the render so the refusal is testable without a GPU: the headless suite runs
	 * with -nullrhi and asserts that this returns false with a message, which is the only part of
	 * the render path a machine without a display can prove.
	 */
	WIDGETIR_API bool CanRenderWidgets(FString& OutReason);

	/** Validate viewports the way RenderWidgetBlueprint will, without touching an asset. */
	WIDGETIR_API bool ValidateViewports(const TArray<FWidgetIRViewport>& Viewports,
		TArray<FString>& OutErrors);

	/**
	 * The shorthand spelling: "1920x1080", or "phone:390x844" to name it.
	 *
	 * Lives here rather than in the MCP adapter so it can be tested without a JSON document. The
	 * object spelling stays the adapter's, because it is a JSON shape and this module has no
	 * opinion about JSON. Dimensions only - a shorthand cannot carry a DPI scale, and inventing a
	 * third field's syntax would buy nothing over passing an object.
	 */
	WIDGETIR_API bool ParseViewportSpec(const FString& Spec, FWidgetIRViewport& Out,
		FString& OutError);
}
