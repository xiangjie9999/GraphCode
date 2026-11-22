#pragma once

#include "CoreMinimal.h"
#include "ControlRigIRTypes.h"

namespace ControlRigIR
{
	/**
	 * Canonical named blocks, ready for `FGraphCodeDocument::Blocks`.
	 *
	 * **THE `hierarchy` BLOCK IS ONE LINE PER ELEMENT** — `<type> <name> [parent=…, location=…]` —
	 * with the transform components stated only where they deviate from identity, the same
	 * subtraction every other block in this language makes.
	 *
	 * **THE `graph` BLOCK NESTS, and it nests the way a Blueprint's does.** `solve <Name> { … }` for
	 * the rig's own model and `function <Name> { … }` for each of its functions, so a reader that
	 * knows one `.gc` knows this one. Inside, every statement is
	 * `<alias> = <Type>(<pin>=<node>.<pin>, …) [<pin>=<value>, …]`: links in the parentheses at the
	 * pin that CONSUMES them, deviating defaults in the brackets.
	 */
	CONTROLRIGIR_API FString SettingsBlock(const FControlRigIRDocument& Document);
	CONTROLRIGIR_API FString HierarchyBlock(const FControlRigIRDocument& Document);
	CONTROLRIGIR_API FString PropertiesBlock(const FControlRigIRDocument& Document);
	CONTROLRIGIR_API FString GraphBlock(const FControlRigIRDocument& Document);

	/**
	 * One statement, exposed so a test can assert a line rather than a whole block.
	 *
	 * It is the serializer's own function and not a second spelling: `GraphBlock` calls this for
	 * every node, for the reason `ControlRigIRNodeLess` is shared — a format rendered by two pieces
	 * of code eventually disagrees with itself, and the only symptom is a sidecar that stops
	 * matching.
	 */
	CONTROLRIGIR_API FString NodeStatement(const FControlRigIRNode& Node);

	/** One hierarchy line, exposed for the same reason `NodeStatement` is. */
	CONTROLRIGIR_API FString ElementStatement(const FControlRigIRElement& Element);
}
