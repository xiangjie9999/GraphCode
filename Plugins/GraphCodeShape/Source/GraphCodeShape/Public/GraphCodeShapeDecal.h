// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGcShapeDocument;
struct FGcShapeInstruction;
struct FGcShapeWeightedSelection;

/**
 * **MB52 — `decal()`. A mark, a scatter of marks, or wear the FORM places.**
 * `GC-SHAPE-DECAL-DESIGN.md`.
 *
 * **ONE VERB, BECAUSE ALL THREE ARE ONE OPERATION** — apply a material through a mask over a
 * region — and only the mask's SOURCE differs. This was first designed as `decal()` plus `wear()`
 * and the user cut it: *"aren't they the same thing"*. They are. Splitting a grammar because the
 * IMPLEMENTATION splits is a leak ([[spline-pull-is-clump-with-an-authored-spine]]).
 *
 * **`wear` WOULD ALSO HAVE SHADOWED A NAME AUTHORS ALREADY USE** — `Brick = mesh(w, wear)` and
 * `add($Brick, wear=random(...))` are in the corpus today.
 *
 * **THE SOURCE IS A MATERIAL AND NOT A TEXTURE, AND THAT IS LOAD-BEARING.** A stamped number is
 * embossed and its paint is worn, so it has a normal and a roughness. A material source carries
 * every channel through the evaluator the bake already runs; a texture source would carry colour
 * and need the rest bolted on. It is also why there is no authored channel list.
 */
struct GRAPHCODESHAPE_API FGcShapeDecal
{
	/** How the mask that places this decal is produced. */
	enum class EMask : uint8
	{
		/** The source image's own alpha, at `At`. The ordinary placed mark. */
		Image,
		/**
		 * Convex curvature — the shape's outer edges. Paint comes off where a thing gets knocked.
		 *
		 * **`edges` AND `crevices` ARE THE RULED PAIR** over `convex`/`concave` and
		 * `curvature`/`cavity`: they would be the only mathematical vocabulary in a grammar that
		 * otherwise says `base`, `corner`, `rim`, `border`, `facing` and `apart`.
		 */
		Edges,
		/** Concave curvature and occlusion — the recesses grime collects in. */
		Crevices,
		Selection,
	};

	/** The `$Name` of the material being applied, without its `$`. */
	FString Source;

	EMask Mask = EMask::Image;

	/** `amount=70%` — how strongly a derived mask applies. Unread when `Mask` is `Image`. */
	double Amount = 1.0;

	/** `size=(18,18)` in centimetres, or zero when the placement does not state one. */
	UE::Math::TVector2<double> Size = UE::Math::TVector2<double>::ZeroVector;

	/** `in=$Region`, empty for the whole mesh. */
	FString Region;
	FString SelectionName;
	TSharedPtr<const FGcShapeWeightedSelection> Selection;

	/** `at=(x,y,z)` — the footprint's centre, for a placed mark. */
	UE::Math::TVector<double> At = UE::Math::TVector<double>::ZeroVector;
	bool bHasAt = false;

	/**
	 * `project=-Y` — the direction the mark is cast ALONG, pointing into the surface.
	 *
	 * Defaults to nothing: a placed mark with no direction is refused rather than guessed, because
	 * a cast that picked its own axis would stamp a face the author never named.
	 */
	UE::Math::TVector<double> Project = UE::Math::TVector<double>::ZeroVector;
	bool bHasProject = false;

	/** `turn=` about the cast axis, in degrees. */
	double Turn = 0.0;
	bool bHasTurn = false;

	/** Image placement is optional for a weighted mask; any placement word requests it. */
	bool IsProjected() const
	{
		return Mask == EMask::Image || (Mask == EMask::Selection
			&& (bHasAt || bHasProject || bHasTurn || Size != UE::Math::TVector2<double>::ZeroVector));
	}

	/**
	 * `bake=Chest` — the target that receives this decal, empty for the sheet.
	 *
	 * **A DECAL WITH NOWHERE TO GO IS REFUSED, NOT DROPPED.** The ruling is that an unreceived
	 * decal is *thrown away*; this grammar refuses a silent no-op on principle — a `[pivot=corner]`
	 * that reported success and did nothing is the defect row R20 exists for — so the discard is
	 * stated at build time rather than discovered by looking at the result.
	 */
	FString BakeTarget;

	/** Read one `decal(...)` instruction. An unknown argument is refused rather than ignored. */
	static bool Read(const FGcShapeDocument& Document, const FGcShapeInstruction& Instruction,
		FGcShapeDecal& Out, FString& OutError);

	/** `edges` / `crevices` back to a word, for refusals and reports. */
	static const TCHAR* MaskWord(EMask Mask);
};
