#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"
#include "AuthoringPropertyCodec.h"
#include "StructUtils/PropertyBag.h"

/**
 * The `FInstancedPropertyBag` codec — the one genuinely new codec piece the World and Move product
 * kinds both need, and therefore Core's rather than either product's. `kind:pcg_graph`'s
 * `parameters { }` block IS a bag (`UPCGGraph::UserParameters`), and `kind:state_tree` declares one
 * the same way, so the vocabulary lives here on the standing rule *"if anything shares a base, put
 * in Core"* (`GC-PRODUCT-SPLIT.md` §7). The kinds supply the block name and wrap the lines; the
 * shape of a typed, author-declared parameter set is one thing spelled once.
 *
 * **What is new, and what is not.** Nothing else in `gc` writes a parameter set whose TYPES are
 * authored data rather than a compiled class — that is the new part, and it is narrow: the
 * declaration grammar `Name: CppType [= Value]` and the reverse of `FProperty::GetCPPType()` that
 * rebuilds the bag's layout on apply. The VALUES themselves are ordinary reflected leaves once the
 * layout exists, so both halves delegate to `AuthoringCore::ExportStructDeviations` /
 * `ImportStructDeviations` — the same code every `values { }` and `settings { }` block already runs.
 * A second value walker here would eventually disagree with that one about which leaves exist.
 *
 * **The declaration line, fixed by the shipped G2 reader** (`GraphCodeWorldPcgGraph.cpp`):
 *
 *     Density: float = 40.0        // a deviation from the freshly-constructed baseline
 *     Seed: int32                  // equal to the baseline, so no value is written
 *     Mesh: TObjectPtr<UStaticMesh> = obj'/Game/Meshes/SM_Pine.SM_Pine'
 *
 * The type is the property's own C++ type asked of the reflected property, NOT a name mapped from
 * `EPropertyBagPropertyType` — the reader chose that so it never has to grow a map as the engine
 * adds bag types, and so a struct or object parameter reads as the struct or class it actually is.
 *
 * **The inverse is bounded, and refuses rather than guesses.** A C++ type string resolves totally
 * for the scalar keywords and for any decorated engine type (`TObjectPtr<…>`, `TSoftObjectPtr<…>`,
 * `TSubclassOf<…>`, `TEnumAsByte<…>`, `TArray<…>`). A bare struct, class or enum name is resolved
 * through the reflection registry, which can be ambiguous or miss for a project-specific type — and
 * there the apply RETURNS A NAMED ERROR rather than inventing a layout the reader never described.
 * [[inverting-a-lossy-projection-can-be-impossible]] is why the boundary is drawn out loud.
 */
namespace GraphCode
{
	/**
	 * Emit a bag's members as `Name: CppType [= Value]` declaration lines — no block wrapper, so the
	 * caller adds its own `parameters { … }` or `tasks { … }`. Values are compacted against a freshly
	 * INITIALISED baseline of the same layout, never a CDO: a property bag's per-instance defaults
	 * (a `Seed` seeded from a class-name hash, anything a struct's constructor sets) are the fixed
	 * point, and a CDO baseline would print them as deviations nobody typed
	 * ([[a-created-node-carries-constructor-defaults]]).
	 *
	 * `Owner` resolves relative object references during export and may be null. Diagnostics carry
	 * anything a value would not survive its own round trip through; they are never silently dropped.
	 */
	GRAPHCODE_API void ProjectPropertyBag(const FInstancedPropertyBag& Bag, const UObject* Owner,
		TArray<FString>& OutLines, TArray<FAuthoringDiagnostic>& OutDiagnostics);

	/**
	 * Rebuild a bag from declaration lines: parse each `Name: CppType [= Value]`, reconstruct the
	 * layout, then import the values in SNAPSHOT (a declaration block is the complete authored set,
	 * so an unmentioned value ends at its baseline, not at whatever `OutBag` held).
	 *
	 * `OutBag` is REPLACED, not patched — the declaration block owns the whole layout. On any error
	 * `OutBag` is left untouched and every independent failure is collected, so one unresolvable
	 * type does not hide a second one below it. The instance-override case (a `kind:pcg_graph_instance`
	 * writing values against a parent's layout) is a different path and does not come through here:
	 * it keeps the parent's bag and imports in PATCH, because omission there PRESERVES.
	 *
	 * **A MEMBER THAT KEEPS ITS NAME KEEPS ITS `ID`, WHICH IS THE ONE THING THE REPLACEMENT DOES NOT
	 * REPLACE.** A declaration line cannot state a GUID, so leaving it blank makes the engine mint a
	 * fresh one — and everything that cites a bag member cites the GUID, not the name. What that
	 * costs is measured rather than feared: PCG deletes a `GetGraphParameter` node whose GUID it
	 * cannot find, at the next unrelated edit rather than at the apply. So the ID is taken from
	 * `OutBag` by name, and only a genuinely new name gets a new one.
	 *
	 * **A RENAME IS THEREFORE A REMOVAL PLUS AN ADDITION, and is a declared gap rather than a bug.**
	 * The block has no rename direction to read, so nothing here can tell the two apart; a caller who
	 * renames a parameter should expect whatever cited it to lose the reference.
	 *
	 * `BlockBody` may arrive with or without its `name { … }` wrapper. `Owner` resolves relative
	 * object references during import and may be null.
	 */
	GRAPHCODE_API bool ApplyPropertyBag(const FString& BlockBody, FInstancedPropertyBag& OutBag,
		UObject* Owner, TArray<FString>& OutErrors);
}
