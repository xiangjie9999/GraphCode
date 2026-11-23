#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"

class UClass;

namespace WidgetIR
{
	/**
	 * What a widget class offers a document, read from reflection rather than from a curated table.
	 *
	 * Nothing here is a list this project maintains: the palette category, the tooltip, the keywords
	 * and the containment endpoints all come from the class itself, so a plugin's widget is indexed
	 * the day it is loaded and no tool code changes to admit it.
	 */
	struct WIDGETIR_API FWidgetIRClassInfo
	{
		FString ClassPath;
		/** The short spelling when it is unique in this editor, the full path otherwise. */
		FString ClassToken;
		FString DisplayName;
		FString PaletteCategory;
		FString ToolTip;
		FString Keywords;
		/** Owning module, and the plugin that module belongs to when it is not engine or game code. */
		FString Module;
		FString Plugin;

		bool bPanel = false;
		bool bMultipleChildren = false;
		bool bContentWidget = false;
		bool bUserWidget = false;
		/** Backed by a Widget Blueprint asset rather than by C++. */
		bool bAssetBacked = false;
		bool bLoaded = true;

		bool bAbstract = false;
		bool bDeprecated = false;
		bool bExperimental = false;
		bool bEarlyAccess = false;
		bool bEditorOnly = false;

		/** Named slots the class declares. Only knowable for a loaded class. */
		TArray<FString> NamedSlots;

		/** Empty when the class can be constructed here; otherwise why it cannot. */
		FString UnusableReason;
		bool IsUsable() const { return UnusableReason.IsEmpty(); }
	};

	struct WIDGETIR_API FWidgetIRSearchMatch
	{
		FWidgetIRClassInfo Class;
		/** A paste-ready declaration, with the containment braces the class actually supports. */
		FString Skeleton;
		/** One statement, ready for ui_describe. */
		FString DescribeIr;
		TArray<FString> MatchedTerms;
		int32 Score = 0;
	};

	struct WIDGETIR_API FWidgetIRSearchQuery
	{
		/** Free text over class name, display name, keywords, palette category and tooltip. */
		FString Text;

		/**
		 * The asset this widget would be added to. Supplies the project's palette settings, whether
		 * editor-only widgets are admissible, and the asset's own class - which cannot contain
		 * itself.
		 */
		FString AssetPath;

		/** A prospective parent, by class token or path. Reported, and used to refuse impossible asks. */
		FString ParentClass;

		bool bPanelsOnly = false;
		/** Report classes that cannot be constructed here, with the reason, instead of filtering them. */
		bool bIncludeUnusable = false;
		/** Asset-backed User Widgets from the asset registry, whether or not they are loaded. */
		bool bIncludeProjectWidgets = true;

		int32 Limit = 10;
		int32 Offset = 0;
	};

	/** What the prospective parent can hold, so a caller knows which endpoint a child would use. */
	struct WIDGETIR_API FWidgetIRParentContext
	{
		bool bResolved = false;
		FString ClassPath;
		FString ClassToken;
		bool bAcceptsChildren = false;
		bool bMultipleChildren = false;
		TArray<FString> NamedSlots;
	};

	struct WIDGETIR_API FWidgetIRSearchResult
	{
		TArray<FWidgetIRSearchMatch> Matches;
		int32 TotalMatches = 0;
		int32 Offset = 0;
		int32 NextOffset = INDEX_NONE;
		/** Excluded because they cannot be constructed here. Counted, never dropped in silence. */
		int32 FilteredUnusable = 0;
		TArray<FString> FilterReasons;
		TArray<FString> UnmatchedTerms;
		TArray<FString> Notes;
		FWidgetIRParentContext Parent;
	};

	/**
	 * Index the widget classes this editor can construct and rank them against one query.
	 *
	 * No result is ever chosen for the caller: an ambiguous short class name is reported as the full
	 * path, and a term that matched nothing is named rather than quietly dropped.
	 */
	WIDGETIR_API bool SearchWidgetClasses(const FWidgetIRSearchQuery& Query,
		FWidgetIRSearchResult& OutResult, FString& OutError);

	/** Everything the index knows about one class, for a caller that already has its identity. */
	WIDGETIR_API bool DescribeWidgetClass(const UClass* Class, FWidgetIRClassInfo& OutInfo);

	/**
	 * Why this class cannot be constructed into that asset, or empty when it can.
	 *
	 * An empty asset path answers only the questions the class itself settles - abstract, hidden,
	 * experimental - because the rest are the project's, not the class's.
	 */
	WIDGETIR_API FString WidgetClassUnusableReason(const UClass* Class, const FString& TargetAssetPath);
}
