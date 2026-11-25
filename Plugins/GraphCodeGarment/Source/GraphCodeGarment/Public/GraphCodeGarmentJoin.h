// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeGarmentRing.h"

class FGcGarmentBody;
struct FGcGarment;
struct FGcGarmentStep;

/**
 * What a `join($Crotch@0.25, $Crotch@0.75, $Top_L@0.5, $Top_R@0.5)` made of the rings it names
 * (`GC-GARMENT-GRAMMAR.md` §10). The places meet at their centre. The ring named twice — the
 * garment's second — is pulled into an 8 through it, and each ring named once becomes one of the 8's
 * two halves: where two rings become one they lie on their mean path, as Groom's `clump` puts a lock
 * where its members meet. The changed rings are this garment's alone.
 */
struct GRAPHCODEGARMENT_API FGcGarmentJoined
{
	/** Every ring the garment sees: as built, with the ones the join touched as it left them. */
	TArray<FGcGarmentBuiltRing> Rings;

	/** Where the places met. */
	FVector At = FVector::ZeroVector;

	/**
	 * The ring pulled into an 8, and its two named places as they were before, in the order written - each where the ring crosses the plane
	 * between the 8's halves, nearest the place written (front or back), since that is where the halves meet (2 Nov).
	 * Its first half runs from the first of them round to the second, and its second half back.
	 */
	FString Eight;
	FVector Before[2] = { FVector::ZeroVector, FVector::ZeroVector };

	/**
	 * Where the garment's first ring crosses the plane between the 8's halves, over each of the 8's places (the front and the back): where
	 * the cloth from the first ring runs down to them, its two halves parting there. Zero where no plane was found (2 Nov).
	 */
	FVector OnFirst[2] = { FVector::ZeroVector, FVector::ZeroVector };

	/** The ring named once that became each half of the 8; empty for a half no ring became. */
	FString HalfOf[2];

	const FGcGarmentBuiltRing* Find(const FString& Name) const;
};

class GRAPHCODEGARMENT_API FGraphCodeGarmentJoin
{
public:
	/** `Join`, one of `Declared`'s steps, on `Rings` as built. False with a refusal naming the line and its fix. */
	static bool Apply(const FGcGarment& Declared, const FGcGarmentStep& Join, const TArray<FGcGarmentBuiltRing>& Rings,
		const FGcGarmentBody& Body, FGcGarmentJoined& Out, FString& OutRefusal);
};
