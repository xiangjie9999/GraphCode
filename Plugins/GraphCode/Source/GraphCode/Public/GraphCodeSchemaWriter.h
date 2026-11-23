#pragma once

#include "AuthoringCoreTypes.h"
#include "AuthoringPropertyCodec.h"
#include "CoreMinimal.h"
#include "GraphCodeSidecarWriter.h"

struct GRAPHCODE_API FGraphCodeSchemaWriteRequest
{
	EGraphCodeAssetKind Kind = EGraphCodeAssetKind::UserStruct;
	FString AssetPath;
	FString BlockName;
	FString CurrentBody;
	FString DesiredBody;
	bool bDryRun = false;
	bool bAllowBreakingChanges = false;

	/**
	 * `rename <Old> -> <New>`, already split off the block body. C23 §6 step 4.
	 *
	 * **SPLIT AT THE SEAM AND NOT IN THE ADAPTER, because for one of the three routes the body no
	 * longer exists by then.** A Blueprint `properties` block is handed to the adapter as a PARSER
	 * DOCUMENT — `DeclarationParserDocument` wraps it in a fake function and prefixes every line
	 * with `local `, so a directive line arrives as `local rename A -> B` at a depth that is not the
	 * block's. Splitting there found nothing and would have silently ignored every token, which is
	 * the exact failure mode this row exists to remove. The seam still holds the real block text.
	 */
	TArray<AuthoringCore::FAuthoringRename> Renames;

	/**
	 * What an omitted PROPERTY means, for the blocks behind this seam that are property bags.
	 * C23 §3, 2026-08-10 — `settings { }`'s route 3, `material` and `material_function`.
	 *
	 * **DEFAULTED TO PATCH, WHICH IS THE OPPOSITE OF THE TYPE'S OWN DEFAULT, AND THAT FLIP IS THE
	 * POINT.** `FAuthoringImportMode` defaults to snapshot because most of its forty-odd callers
	 * reach it through a block that decides omission one level up. This is a `gc_apply` request, and
	 * `gc_apply`'s unstated omission is patch — a request that silently meant snapshot would make
	 * the same payload mean two things depending on which door it came through.
	 *
	 * Filled at the seam rather than in the adapter, because the seam is where the real block text
	 * still exists and `reset <Path>` has to come off it before a `Path = Value` parser reads the
	 * line and reports it as a malformed property. Ignored by the blocks here that are not bags:
	 * `properties`, `entries` and `parameters` decide omission their own way, and `settings` on
	 * `user_struct`/`user_enum` is one mandatory value with no entry to omit at all.
	 */
	AuthoringCore::FAuthoringImportMode Import = AuthoringCore::FAuthoringImportMode{ false };

	/**
	 * What an omitted ENTRY means on the two named-entity blocks behind this seam: a Blueprint's
	 * `properties` and a `user_struct`'s `properties`. K2 of the 13 Nov gap report (P232), C23 §3.
	 *
	 * True is snapshot — the payload is the whole list and an omitted entry is removed. False is
	 * patch — an omitted entry is left alone and `Deletes` below is the only removal.
	 *
	 * **DEFAULTED TO TRUE, THE BEHAVIOUR EVERY CALLER HAD BEFORE THE CONVERSION**, so a caller that
	 * does not know this field (a spec driving the adapter directly) still means what it meant. The
	 * one caller that decides — `gc_apply`'s seam — always sets it from the call's mode. Appended at
	 * the end of the struct because this is a core public header.
	 */
	bool bOmissionIsDeletion = true;

	/** `delete <Name>` lines, already split off the block body. Patch only; the seam refuses them under snapshot. */
	TArray<FString> Deletes;
};

/**
 * One graph node left dangling because the declaration it read was removed.
 *
 * Flat strings rather than a JSON value on purpose: GraphCode's rule is that it never takes a
 * dependency on the editor integration's types, and this has to survive the trip back out to
 * whatever is driving the apply - MCP today, the `GcApply` commandlet with no MCP at all.
 *
 * It carries the removed declaration alongside the node so a caller holding only this result can
 * put the variable back, or repoint the node, without re-reading anything. After the apply the
 * node no longer resolves to a variable, so nothing downstream can reconstruct this.
 */
struct GRAPHCODE_API FGraphCodeOrphanedReference
{
	FString VariableName;
	FString VariableType;
	FString DefaultValue;
	FString BlueprintPath;
	FString GraphName;
	FString NodeGuid;
	FString NodeClass;
	FString NodeTitle;
};

/**
 * One entity an apply would REMOVE, in the shape the deletion pre-check can price. S7b.
 *
 * The check used to run off `FGraphCodeApplyResult::Deleted`, which is the omission list and
 * nothing else, so an explicit `delete X` and a removed declaration were both invisible to it. An
 * unpriced deletion looks exactly like a deletion that costs nothing, which is why the input is a
 * union rather than one field. `GC-SEARCH-DESIGN.md` §8.1.
 *
 * STRUCTURED RATHER THAN A WIDER STRING LIST, because those strings are not names: the component
 * writer reports `delete component Child` and `reset inherited override Root` into `Deleted`, so
 * widening the input would have searched for a member called "delete component Child" and answered
 * "nothing cites it" about it.
 */
struct GRAPHCODE_API FGraphCodeRemoval
{
	/** The block that held it. Filled at the apply seam, where the block name is known. */
	FString Block;

	/** The bare name a citation would use. Never a sentence, never a phrase. */
	FString Name;

	/** How the payload asked for it: `omission`, `delete` or `declaration`. */
	FString Source;

	/**
	 * Non-empty when no project-wide search can answer for this one, and why.
	 *
	 * A material expression's alias is the case it exists for. It is minted per graph, so a search
	 * would report "nothing cites it" — true, and read as a clean bill of health for a removal that
	 * may still break every Material Instance overriding the parameter that expression carried.
	 * Saying the search cannot answer is the only honest third option.
	 */
	FString IntraAssetReason;
};

struct GRAPHCODE_API FGraphCodeSchemaWriteResult
{
	bool bSuccess = false;
	bool bAssetDirty = false;
	FString Message;
	TArray<FString> Changes;
	TArray<FString> References;
	TArray<FString> UnknownRiskReferencers;
	/** Every node a removal orphaned. Populated on refusal and on dry runs too, not just success. */
	TArray<FGraphCodeOrphanedReference> OrphanedReferences;

	/**
	 * Every declaration this write removes, named. S7b.
	 *
	 * `Changes` already says `remove Health` in prose and `OrphanedReferences` answers the
	 * intra-asset half — it is empty in exactly the case the caller is about to act on, "nothing in
	 * this asset points at it". Neither can drive a project-wide check, so this carries the names.
	 * Filled on refusals and dry runs as well as on success, for the reason the other two are.
	 */
	TArray<FGraphCodeRemoval> Removed;
};

/**
 * Adapter seam for non-Blueprint block mutation.
 *
 * GraphCode owns projection, parsing, sync and routing. The current editor integration owns the
 * mature enum/struct mutation tools and Material writers, so it registers one composed handler
 * here. The dependency remains one-way: GraphCode never includes an MCP type and can ship with a
 * different writer without changing its language.
 */
class GRAPHCODE_API FGraphCodeSchemaWriter
{
public:
	using FHandler = TFunction<bool(
		const FGraphCodeSchemaWriteRequest&,
		FGraphCodeSchemaWriteResult&,
		FString&)>;

	static void Register(FHandler Handler);
	static void Unregister();
	static bool IsRegistered();
	static bool Apply(
		const FGraphCodeSchemaWriteRequest& Request,
		FGraphCodeSchemaWriteResult& OutResult,
		FString& OutError);
};
