// Copyright OraStudio. All Rights Reserved.

// A spline's curve, as a level document records it — `GC-WORLD-GENERATION-DESIGN.md` §12.10.
//
// **THIS IS `GraphCodeVolumeShape` FOR SPLINES, and that is the user's ruling rather than an
// analogy** (2026-08-20): *"make spline just like volume. it will be a reference. because it's so
// large in size, it should be at bottom, together with volumes{}, called splines{}, then each spline
// has a name, refer those names in the params."* Same problem, same answer: a large geometric value
// inlined into an actor's property list drowns every other field in it, so it moves to a named
// file-scope block and the property carries the NAME.
//
// **WHAT IT COSTS TODAY, MEASURED 2026-08-20** (`WhatASplineComponentCostsALevelDocument`, a plain
// 20-point `USplineComponent`): the actors block is **14,055 characters**, of which
// `SplineCurves.ReparamTable` alone is **5,547 (39.5%)**. `USplineComponent::SplineCurves` is
// `UPROPERTY(EditAnywhere, Replicated)` (`SplineComponent.h:242`), so `CPF_Edit` admits the whole
// struct and the reflective walk writes every byte of it.
//
// **AND THE SECOND HALF OF THE USER'S RULE — *"for engine recompute datas, should not be stored by
// us"* — REACHES FURTHER THAN THE REPARAM TABLE.** Two kinds of derived data ride inside that struct:
//
//  1. `ReparamTable` — a distance->parameter lookup, rebuilt wholesale by `UpdateSpline`. Nobody
//     authored an entry of it and nobody could.
//  2. **`ArriveTangent`/`LeaveTangent` on an AUTO point.** `UpdateSpline` calls `AutoSetTangents`,
//     which RECOMPUTES both for every `CIM_CurveAuto` and `CIM_CurveAutoClamped` point. A document
//     that stores them is storing an output, and one that stores a STALE one is worse: applying it
//     writes tangents the curve is about to overwrite, so the file and the world disagree for
//     exactly as long as it takes something to call `UpdateSpline`.
//
// So `ModeDerivesTangents` is not an optimisation. It is the same rule as `ReparamTable`, applied at
// the level the engine actually draws the line ([[decoration-is-not-authored-state]]).
//
// **WHAT IS AUTHORED, and therefore what this carries:** the point positions, the interpolation mode
// per point, the tangents ONLY where the mode makes them an input, the input keys where they are not
// simply ordinal, and the per-point rotation and scale where they deviate from identity. That is the
// whole of what a person can set, and rebuilding from it plus one `UpdateSpline()` reproduces the
// rest ([[validate-a-write-by-reprojecting-it]] is the gate).

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"

class USplineComponent;

namespace GraphCodeSplineCurve
{
	/**
	 * True for a mode whose tangents `UpdateSpline` recomputes, so the document must NOT state them.
	 *
	 * `CIM_CurveAuto` and `CIM_CurveAutoClamped` are the two `AutoSetTangents` rewrites. Everything
	 * else either takes an authored tangent (`CIM_CurveUser`, `CIM_CurveBreak`) or ignores tangents
	 * entirely (`CIM_Linear`, `CIM_Constant`) — and for the ignoring pair, writing a tangent is
	 * noise rather than a lie, so they are grouped with "do not write" for the same reason.
	 */
	GRAPHCODE_API bool ModeDerivesTangents(EInterpCurveMode Mode);

	/** The grammar's word for a mode, and its inverse. Unknown text is a named parse failure. */
	GRAPHCODE_API const TCHAR* WordForMode(EInterpCurveMode Mode);
	GRAPHCODE_API bool ModeFromWord(const FString& Word, EInterpCurveMode& OutMode);

	/** One authored point. Every optional half says whether it is present rather than guessing. */
	struct FPoint
	{
		FVector Position = FVector::ZeroVector;
		EInterpCurveMode Mode = CIM_CurveAuto;

		/** Set only when `ModeDerivesTangents(Mode)` is false. */
		FVector Arrive = FVector::ZeroVector;
		FVector Leave = FVector::ZeroVector;

		/**
		 * `InVal`. Written only when it is not this point's ORDINAL, which is the shape
		 * `AddSplinePoint` produces and therefore the shape almost every spline has.
		 */
		bool bHasKey = false;
		float Key = 0.0f;

		/** Per-point rotation and scale, written only where they deviate from identity. */
		bool bHasRotation = false;
		FQuat Rotation = FQuat::Identity;
		bool bHasScale = false;
		FVector Scale = FVector::OneVector;
	};

	/** One spline component's authored curve. */
	struct FCurve
	{
		TArray<FPoint> Points;

		/**
		 * `bClosedLoop` is NOT here, and the omission is deliberate: it is a plain `bool` `UPROPERTY`
		 * on the component, so the ordinary property walk already carries it on the component's own
		 * line. Moving it would take a one-word field out of the place a reader looks for fields.
		 */
		bool IsEmpty() const { return Points.Num() == 0; }
	};

	/**
	 * Read a spline component's authored curve.
	 *
	 * Non-destructive ([[reading-must-not-write]]): it reads `SplineCurves` and never calls
	 * `UpdateSpline`, so a component whose tangents are stale reads as stale rather than being
	 * quietly fixed by the act of being looked at.
	 */
	GRAPHCODE_API FCurve Extract(const USplineComponent& Spline);

	/**
	 * Write an authored curve onto a spline component, replacing its points.
	 *
	 * **Ends with `UpdateSpline()`, and that call is the whole reason the derived halves are safe to
	 * omit**: it regenerates `ReparamTable` and re-runs `AutoSetTangents` for every auto point. A
	 * caller that skipped it would leave a component whose curve is right and whose length queries,
	 * distance lookups and drawn shape are all wrong.
	 *
	 * Returns the number of points written.
	 */
	GRAPHCODE_API int32 Rebuild(USplineComponent& Spline, const FCurve& Curve);

	// ---- The text spelling, kept beside the model so the two cannot drift ------------------------
	// [[a-reader-that-spells-owes-the-writer-an-inverse]]: one file owns both directions.

	/**
	 * One spline's entry — **ONE LINE, whole**, on `volumes { }`'s ruling and for its reasons.
	 *
	 *   <Actor>.<Component> = points(<x> <y> <z>, …) modes(<word>, …) [arrive(…) leave(…)]
	 *                         [keys(…)] [rotations(…)] [scales(…)]
	 *
	 * **PARALLEL ARRAYS, not a record per point**, which is `points(…) faces(…)`'s shape one subject
	 * over. It also makes the omissions legible: a spline whose points are all auto has no
	 * `arrive(…)` at all, and the absence says *these are computed* rather than *these are zero*.
	 *
	 * **`modes(<word>)` WITH A SINGLE WORD MEANS EVERY POINT**, which is not a special case so much
	 * as the common one: a spline drawn in the editor is `curveauto` throughout, and spelling that
	 * twenty times states one fact twenty times. Any other count must equal the point count.
	 *
	 * The name is `<Actor>.<Component>` because ONE ACTOR MAY HOLD SEVERAL SPLINES — unlike a brush,
	 * where the actor IS the shape and `volumes { }` can key on the actor's document name alone. `.`
	 * is already the grammar's qualified-member separator (`GC-GRAMMAR-REFERENCE.md` §4), so this
	 * spells a nested thing the way everything else nested is spelled.
	 */
	GRAPHCODE_API FString FormatEntry(const FString& Name, const FCurve& Curve);

	/** One parsed `splines { }` entry — the qualified name and its curve. */
	struct FParsedSpline
	{
		FString Name;
		FCurve Curve;
	};

	/**
	 * Parse a `splines { }` block body (without the outer `splines { }`) into its entries.
	 *
	 * The exact inverse of `FormatEntry`, and it validates as it reads: a clause whose length is
	 * neither 1 nor the point count, a mode word the grammar does not know, a point without three
	 * numbers, a missing `points(…)` or a duplicate name is a NAMED failure and never a silently
	 * dropped line ([[a-writer-that-validates-as-it-writes]]).
	 */
	GRAPHCODE_API bool ParseBlock(const FString& BlockBody, TArray<FParsedSpline>& OutSplines,
		FString& OutError);
}
