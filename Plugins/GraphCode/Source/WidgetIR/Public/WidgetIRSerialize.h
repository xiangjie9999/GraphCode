#pragma once

#include "CoreMinimal.h"
#include "WidgetIRTypes.h"

class FJsonObject;

namespace WidgetIR
{
	WIDGETIR_API FString DocumentToText(const FWidgetIRDocument& Document, bool bIncludeSignature = true);

	/**
	 * The same tree as a `widgets { }` block, with no header of its own.
	 *
	 * For GraphCode's merged `.gc`, where the envelope already names the asset, its kind and
	 * its parent class, and one document holds both this and the Blueprint graph.
	 */
	WIDGETIR_API FString WidgetsBlock(const FWidgetIRDocument& Document);
	WIDGETIR_API TSharedPtr<FJsonObject> DocumentToJson(const FWidgetIRDocument& Document);

	WIDGETIR_API bool DocumentFromText(const FString& Text, FWidgetIRDocument& OutDocument, FString& OutError);

	/**
	 * Parse the inside of a `widgets { }` block — the exact inverse of WidgetsBlock.
	 *
	 * The block states no asset path, no ir version and no parent class, because GraphCode's
	 * envelope states all three once. So this fills only what the block actually carries, and a
	 * caller that needs the identity fields takes them from the envelope rather than from here.
	 * Same statement grammar and same parser as a document; only the header differs, which is
	 * the one thing a merged document deliberately removed.
	 */
	WIDGETIR_API bool DocumentFromBlock(const FString& BlockBody, FWidgetIRDocument& OutDocument,
		FString& OutError);

	/**
	 * Parse one or more headerless top-level statements - the hypothetical form ui_describe takes.
	 *
	 * Deliberately the same grammar and the same parser as a document, so a fragment that describes
	 * cleanly is spelled exactly like the lines that will later be applied.
	 */
	WIDGETIR_API bool FragmentFromText(const FString& Text, TArray<FWidgetIRNode>& OutStatements,
		FString& OutError);

	/** Render one statement and its subtree as the indented lines a document would carry. */
	WIDGETIR_API FString StatementToText(const FWidgetIRNode& Statement, int32 Depth = 0);
	WIDGETIR_API bool DocumentFromJson(const TSharedPtr<FJsonObject>& Json,
		FWidgetIRDocument& OutDocument, FString& OutError);

	WIDGETIR_API FString ComputeContentSignature(const FWidgetIRDocument& Document);
	WIDGETIR_API bool DocumentsEquivalent(const FWidgetIRDocument& A, const FWidgetIRDocument& B,
		FString& OutDifference);
}
