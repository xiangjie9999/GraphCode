// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegionFile.h"

class UWorld;

namespace GcsApplyToken
{
	inline constexpr const TCHAR* BadDocument = TEXT("gcs_bad_document");
	inline constexpr const TCHAR* StaleSource = TEXT("gcs_stale_source");
	inline constexpr const TCHAR* ReceiptMismatch = TEXT("gcs_receipt_mismatch");
	inline constexpr const TCHAR* SnapshotRefused = TEXT("gcs_snapshot_refused");
	inline constexpr const TCHAR* FieldOutsideCoverage = TEXT("gcs_field_outside_coverage");
	inline constexpr const TCHAR* CreationClassRequired = TEXT("gcs_creation_class_required");

	/**
	 * A staged geometry edit that does not say a place — §22.14.
	 *
	 * `move $Nothing by (…)`, a `move` naming a corner instead of a region or a course, or a
	 * `regions { }` override whose text the `.gcw` grammar refuses.
	 */
	inline constexpr const TCHAR* BadGeometry = TEXT("gcs_bad_geometry");

	/**
	 * The staged edit is well formed and the `.gcw` would not take it — the write refused, or the
	 * file moved between the plan and the commit.
	 *
	 * **ITS OWN TOKEN, because the two failures are not one.** `BadGeometry` is *this text is wrong*
	 * and the author fixes it; this one is *the document underneath you changed*, and the answer is
	 * to read it again. A caller branching on the reason needs to tell those apart.
	 */
	inline constexpr const TCHAR* GeometryNotStaged = TEXT("gcs_geometry_not_staged");
}

struct GRAPHCODE_API FGcsApplyOptions
{
	bool bDryRun = false;
	bool bSave = true;
	bool bSnapshot = false;
};

struct GRAPHCODE_API FGcsApplyResult
{
	FString Level;
	FString Entry;
	TArray<FString> Changes;
	TArray<FString> Placements;

	/** Entries of this slice that ended up inside one another — §22.9e. It warns and never refuses. */
	TArray<FString> Overlaps;

	/**
	 * What the staged geometry did to the `.gcw`, one sentence per edit — §22.14.
	 *
	 * **SEPARATE FROM `Changes`, WHICH IS ABOUT ACTORS.** One apply now writes two documents, and a
	 * caller reading a flat list could not tell *the street moved* from *a house moved*. That
	 * distinction is the whole of what §22.14 rules: the geometry edit is the cause and the actor
	 * moves are its consequence.
	 */
	TArray<FString> Geometry;

	/** The `.gcw` the staged edit landed in. Empty when the slice carried no geometry. */
	FString GeometryPath;

	bool bSaved = false;
};

/** Parse and apply one complete `.gcs` as a patch to its loaded source level. */
class GRAPHCODE_API FGraphCodeSliceApply
{
public:
	static bool LooksLikeSlice(const FString& Text);

	static bool Apply(UWorld* World, const FGcwDocument& Definition, const FString& Text,
		const FGcsApplyOptions& Options, FGcsApplyResult& OutResult, FString& OutRefusal);
};
