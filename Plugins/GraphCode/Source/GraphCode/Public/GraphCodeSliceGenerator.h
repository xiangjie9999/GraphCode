// Copyright GraphCode Sdn Bhd. All Rights Reserved.

// W0's pure generation half: selected actors become one deterministic `.gcs` answer.
// `GC-SPATIAL-AUTHORING-DESIGN.md` sections 18.3i, 18.4, 18.5 and 19.13.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeSliceMembership.h"

namespace GcsGenerationToken
{
	/** Membership returned a name for which generation received no candidate description. */
	inline constexpr const TCHAR* CandidateMissing = TEXT("gcs_candidate_missing");
}

/** Pure renderer for one declaration's generated `.gcs` document. */
class GRAPHCODE_API FGraphCodeSliceGenerator
{
public:
	/** Bumped whenever generated syntax/semantics require old listening outputs to refresh. */
	static constexpr int32 GeneratorRevision = 2;

	/**
	 * The portion protected by `sig:`. Advisory `errors { }` is deliberately outside it, matching
	 * ordinary `.gc`: refreshing a diagnosis must not turn generated text into an authored proposal.
	 */
	static FString SignedBody(const FString& Body);

	/**
	 * Measure, factor and render one `.gcw` declaration.
	 *
	 * `Candidates` and `Targets` are the same snapshot membership consumed. A caller with a live world
	 * builds them once; this function performs no actor walk, file write or editor operation.
	 *
	 * `FromSignature` is the canonical live level projection's signature. It is provenance rather
	 * than the generated file's own edit guard; `sig:` is computed here over the canonical body.
	 */
	static bool Generate(const FGcwDocument& Definition, const FGcwDeclaration& Declaration,
		const TArray<FGcsCandidate>& Candidates,
		const TMap<FString, FGcPlacementTarget>& Targets,
		const FString& LevelPath, const FString& FromSignature,
		FString& OutText, FString& OutRefusal);
};
