#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;

namespace WidgetIR
{
	/**
	 * One place that resolves a widget by its authored name.
	 *
	 * UMG has no widget GUID a document could carry instead. Generated variables, designer bindings,
	 * navigation, animation bindings, the desired-focus setting, named-slot fills from derived
	 * assets and native `BindWidget` contracts all key on the FName - which is why the name is the
	 * IR's identity, and why changing one is a question about references rather than a refusal.
	 */
	struct WIDGETIR_API FWidgetIRNameReference
	{
		/** The widget name this reference resolves. */
		FString Name;

		/** Machine token: graph, desired_focus, binding, animation, navigation, bind_widget,
		 *  named_slot_filler. */
		FString Kind;

		/** The graph, animation, asset or class that holds the reference. */
		FString Where;

		/** What in there names it - a node title, a driven property, a direction. */
		FString Detail;

		/**
		 * True when renaming the widget carries this reference across with it.
		 *
		 * A reference this write path cannot repair is one it does not own: a C++ `BindWidget`
		 * property needs the header changed, and another asset's named-slot fill needs that asset
		 * written. Deleting a widget repairs nothing by definition - there is no new name to point
		 * at - so this flag only distinguishes renames.
		 */
		bool bRenameRepairs = true;

		/** One human sentence, for a refusal that has to be a work list. */
		FString Describe() const;
	};

	/**
	 * Every name-keyed reference in this asset, plus the two kinds that live outside it: a native
	 * parent's BindWidget properties, and derived Widget Blueprints that fill this asset's named
	 * slots.
	 *
	 * Graph references are asked of the engine - `UK2Node::ReferencesVariable` - rather than
	 * enumerated by node class, because that is the same predicate
	 * `FBlueprintEditorUtils::ReplaceVariableReferences` repairs with. A node type this build has
	 * never heard of answers for itself, and detection cannot drift from repair.
	 */
	WIDGETIR_API void CollectNameReferences(UWidgetBlueprint* Blueprint,
		TArray<FWidgetIRNameReference>& OutReferences);

	/**
	 * Carry every repairable reference from OldName to NewName, in the order the engine's own
	 * `FWidgetBlueprintEditorUtils::RenameWidget` uses.
	 *
	 * The widget object itself is renamed by the caller, between phases: `OnVariableRenamed` looks
	 * the OLD name up, so renaming the object first strands the variable GUID map and the generated
	 * variable is reissued with a fresh GUID - destroying the identity that makes graph references
	 * survive at all.
	 */
	WIDGETIR_API void BeginRename(UWidgetBlueprint* Blueprint, FName OldName, FName NewName);
	WIDGETIR_API void FinishRename(UWidgetBlueprint* Blueprint, FName OldName, FName NewName,
		TArray<FString>& OutRepaired);
}
