// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigPose.h"

class UDynamicMesh;

/**
 * WHERE A REGION ACTUALLY IS — the question three different jobs have been waiting on.
 *
 * A `.gcrig` says `knee_back_L = region($popliteal_L, long=6, wide=40%)`. That is a centre, a reach
 * along the limb and a reach around it — **and until this file nothing turned those words into a set
 * of vertices**, which is why `crease=`, `folds=` and `crowd=` reached no geometry at all. Three
 * things need the same answer and would otherwise each invent one:
 *
 * - **the weighting**, which has to know where a joint's hand-over happens;
 * - **the ring cut**, which cuts inside the span and nowhere else;
 * - **the picture**, which colours a region's own edges so a reader can see what it covers.
 *
 * **ONE ANSWER, SO THE THREE CANNOT DRIFT.** A region drawn in one place and weighted in another is
 * the worst possible failure here, because the picture would show the cure working.
 *
 * **IT READS GEOMETRY, WHICH IS WHY IT IS NOT IN THE LINKER.** The linker refuses what the text alone
 * can settle; a region's centre sitting on the skin, and which vertices fall inside it, need the mesh
 * open (`GraphCodeRigLink.h`).
 */

/** A region's placement, worked out from the file and the pose. Independent of any mesh. */
struct GRAPHCODERIG_API FGcRigRegionSpan
{
	FString Name;

	/** The centre point's position, in the mesh's own centimetres. */
	FVector Centre = FVector::ZeroVector;

	/** Down the limb — the direction `long=` is measured along, and a twist turns about. */
	FVector Axis = FVector::ForwardVector;

	/**
	 * A point ON the limb's line — the bone's head.
	 *
	 * **NOT THE SAME AS `Centre`, AND THE DIFFERENCE IS THE WHOLE OF `wide=`.** A patch's centre sits
	 * OFF the line, out on the skin; *round the limb* has to be measured from the line itself. Measured
	 * from the centre instead, a band across the back of a limb counts its own neighbours as 90° away
	 * and takes nothing — which is exactly what the spec caught.
	 */
	FVector AxisOrigin = FVector::ZeroVector;

	/** Centimetres along `Axis`, the WHOLE length rather than a half-extent. */
	double Long = 0.0;

	/**
	 * **THE DIRECTION THE REGION SITS ON, and it is what makes `wide=` mean anything.** A band across
	 * the back of a knee faces backwards; that facing is the centre's offset from the bone's own line,
	 * and it is what the *side that closes* is read from (grammar §4, five things `turns_in=` carries).
	 */
	FVector Facing = FVector::ZeroVector;

	/** `wide=40%` -> 0.4 of the way round. Unset is a full band and `bFullBand` says so. */
	double Wide = 1.0;
	bool bWidePercent = true;

	/**
	 * **A FULL BAND WRAPS ALL THE WAY ROUND, so it has no side and `Facing` is meaningless for it.**
	 * That is the one case where `closes=` is needed on its own — the grammar says so, and this flag
	 * is where the build finds out.
	 */
	bool bFullBand = true;

	/**
	 * **A FULL BAND HOLDS ITS OWN LIMB'S SKIN** (MOVE-CAT1, 2026-10-19; Docs/GC-MOVE-CAT-PLAN.md 1y; RIG.md step 5's known limit): the bones it
	 * lies between (the limb it turns, the bone that limb turns from, and helpers riding either), each head to tail, and every other bone. A
	 * full band takes only skin nearer its own bones than any other: round a cat's thigh, which lies inside its flank, the slab across the
	 * thigh's line ran through the whole body to the head and re-laid 57,000 triangles. Empty: the slab alone (a sided band, which wide= bounds).
	 */
	TArray<TPair<FVector, FVector>> OwnBones, OtherBones;

	/** Whether the skin at `At` is this band's own: true for a band with no bones of its own. */
	bool HoldsSkinAt(const FVector& At) const;

	/** `tilt=` in degrees, carried through for the ring search. */
	double Tilt = 0.0;

	/**
	 * **TRUE WHERE `long=` WAS NOT STATED AND A LENGTH HAD TO BE INVENTED.** The grammar says an unset
	 * `long=` takes its length from the ring on the mesh, and there is no ring until something cuts
	 * one — so the fallback is the turning bone's own length, and it is FLAGGED rather than hidden,
	 * because every count derived from it is then a guess too.
	 */
	bool bLongWasDerived = false;

	/** The bone whose `turns_in=` named this region, or empty for a region no joint turns in. */
	FString TurnedInBy;

	/** Where the span starts — the end nearest the parent. */
	FVector Start() const { return Centre - Axis * (Long * 0.5); }
};

class GRAPHCODERIG_API FGraphCodeRigRegion
{
public:
	/**
	 * Work out one region's placement from the file and the pose.
	 *
	 * **THE AXIS COMES FROM THE FILE, NEVER FROM AN AXIS LETTER** (ledger row 28): `along=($a, $b)`
	 * where it is stated, otherwise the line between the neighbouring joints of whatever turns in this
	 * region (rows 34, 36), and the turning bone's own direction as a last resort.
	 */
	static bool Resolve(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose,
		const FString& RegionName, FGcRigRegionSpan& Out, FString& OutRefusal);

	/** Every region in the rig, in declaration order — what a whole-rig picture needs. */
	static bool ResolveAll(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose,
		TArray<FGcRigRegionSpan>& Out, FString& OutRefusal);

	/**
	 * Which vertices fall inside the span — along the limb AND around it — **and on the same skin
	 * as its centre.**
	 *
	 * **A VERTEX IS IN OR OUT; THE FADE IS SOMEBODY ELSE'S JOB.** Ledger row 43 rules that a region's
	 * settings fade across its edge, because a hard edge puts a kink exactly where the skin bends —
	 * but a fade is an AMOUNT per vertex and this is the set the amount is computed over. Confusing
	 * the two would make the ring cut fade, and half a cut edge is not a thing.
	 *
	 * **A REGION IS A PATCH OF SKIN, AND SKIN IS WHAT CAN BE WALKED TO — 2026-09-22.** The slab and
	 * the sector are geometry, and geometry cannot tell an ear flap hanging in front of a shoulder
	 * from the shoulder: the shoulder's band held the flap's lower half and shaped it for the chest.
	 * A region's centre is a measured hit on the skin it means, so the set is what that hit reaches
	 * along the mesh's own edges without leaving the slab — the flap from a point on the flap, the
	 * neck under it from a point on the neck, never both from either.
	 */
	static void SelectVertices(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span,
		TSet<int32>& OutVertices);

	/** Every edge with both ends inside — what a region-coloured wireframe draws. */
	static void SelectEdges(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span, TSet<int32>& OutEdges);

	/**
	 * How far INSIDE the span a point sits, 1 at the middle falling to 0 at the edge — row 43's fade,
	 * as one number a caller can multiply an amount by.
	 *
	 * Outside the span it is 0, so a caller never has to test membership separately.
	 */
	static double Falloff(const FGcRigRegionSpan& Span, const FVector& At);

	/**
	 * WHERE in the span a point sits, as two fractions: `OutAlong` 0 at the parent's end and 1 at the
	 * far end, `OutRound` 0 dead on the facing and 1 at the sector's edge (0 for a full band, which
	 * has no side). False where the point is outside the span at all.
	 *
	 * **THE FADE ANSWERS *HOW MUCH* AND THIS ANSWERS *WHICH EDGE*, which a single bell cannot.** The
	 * claim needs to know which of a patch's three edges a vertex is near, because only some of them
	 * are borders the skin crosses — `FGraphCodeRigWeight::MeasureClaim`.
	 */
	static bool Places(const FGcRigRegionSpan& Span, const FVector& At,
		double& OutAlong, double& OutRound);

	/**
	 * **HOW MUCH OF THIS SKIN THE REGION OWNS is a different question from how strongly it SHAPES it,
	 * and it is NOT answered here.** `Falloff` is the shape of an AMOUNT — `crease=`, `slide=`, a
	 * profile — and row 43 is right about those: a step in an amount puts a kink exactly where the
	 * skin bends, so they fade from the middle out. Ownership is not an amount, and a claim scaled by
	 * this fade takes about a quarter of what it should everywhere, because the fade is a product of
	 * two linear tapers. The claim's own law is `FGcRigClaim::Claimed` in `GraphCodeRigWeight.cpp`:
	 * distance from the patch's real border, along the mesh, which is a question about the SKIN and
	 * not about the span, and so does not belong to this file.
	 */

	/**
	 * **WHERE THE FOLD IS, as a fraction along the span** — 0 at the parent's end, 1 at the child's.
	 *
	 * A fold sits at a bone's HEAD, between it and its parent (grammar section 11.3aa): `knee_B_L` is
	 * the shin and the knee's crease is at its top. The span already carries that point as
	 * `AxisOrigin`, so this is a projection and not a second opinion — and 0.5 is the ordinary answer,
	 * because a band is usually centred on the joint it belongs to.
	 */
	static double CreaseAlong(const FGcRigRegionSpan& Span);

	/**
	 * **WHICH WAY THE JOINT CLOSES, as a direction in the mesh's own frame.**
	 *
	 * Two things can answer, and the grammar already ordered them: the side the region SITS on is the
	 * side that closes, and `closes=` is kept so a file can state it too — so a declaration beats the
	 * derivation, which is the same order the mirror plane takes. A full band has no side at all,
	 * which is the one case the grammar says needs `closes=` on its own, and that is a refusal rather
	 * than a guess.
	 *
	 * **AND IT IS WHERE THE CHECK THE LINKER COULD NOT MAKE FINALLY HAPPENS** (order step 2 left it
	 * here): a stated `closes=` that does not agree with the side its region sits on is reported
	 * through `OutDisagreement`, because both are authored facts about one thing and only one of them
	 * can be right.
	 */
	/** Whether one of the six words names a direction at a place, and if not, why not. */
	enum class EWordSide : uint8
	{
		/** It does (zero for `Unset`, which names none). */
		Named,
		/** `in`/`out` on a rig with no symmetry plane: neither `faces:` nor `mirror_axis:`. */
		NoMidline,
		/** `in`/`out` at a place ON the midline, which is neither side. */
		OnMidline,
	};

	/**
	 * **THE ONE READING OF THE RIG'S SIX DIRECTION WORDS**, at `At` in the mesh's own frame: `forward`/`back`/`up`/`down`
	 * against `faces:`, `in`/`out` toward and away from the midline from the side `At` stands on. A joint's `closes=` and a
	 * morph line's `along=` (GC-MORPH-DESIGN.md §3.2) both read through here, so the two cannot drift; each words its own
	 * refusal from the answer.
	 */
	static EWordSide WordDirection(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose, EGcRigCloses Word,
		const FVector& At, FVector& Out);

	/**
	 * The direction a `closes=` word names for a joint standing at `At`, zero where none is stated.
	 * `in` and `out` need the creature's midline and are refused by name without one. Shared by
	 * `ClosingSide` and by a joint that names no region and so has only the word.
	 */
	static bool StatedSide(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose, const FString& JointName,
		EGcRigCloses Stated, const FVector& At, FVector& Declared, FString& OutRefusal);

	static bool ClosingSide(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose,
		const FGcRigRegionSpan& Span, FVector& Out, FString& OutDisagreement, FString& OutRefusal);

	/**
	 * Every triangle with all three corners inside — the region EXACTLY as `long=` and `wide=` place
	 * it, which is what a re-lay rebuilds and what a region-coloured picture paints.
	 *
	 * **A CUT USES `SelectRing` INSTEAD, and the difference is the whole of what `wide=` is for.** A
	 * plane has no edges and must be told where to stop, but stopping it at the sector stops the ring
	 * too; the sector is a statement about BEHAVIOUR, and the thing a ring must not run off the end of
	 * is the limb.
	 */
	static void SelectTriangles(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span,
		TSet<int32>& OutTriangles);

	/**
	 * **THE WHOLE WAY ROUND THE LIMB, WHATEVER `wide=` SAYS — the triangles a RING may be cut across.**
	 *
	 * `wide=` says which skin BEHAVES differently; it was never a statement about where the mesh may
	 * be cut. A ring that stops at the edge of a 45% band is not a ring: the sides and the far face of
	 * the limb get no loop at all, and a fold has nothing to bend on everywhere the band is not. So
	 * the cut takes the band ALONG the limb and leaves the sector to the weighting, and `wedge=` stays
	 * what it always was — the extra half-rings on the closing side, which is now genuinely a half.
	 *
	 * **AND GOING ROUND IS NOT THE SAME AS DROPPING THE FILTER.** The span's reach along the limb is a
	 * SLAB, and a slab at knee height also holds the other leg, the belly and anything else standing
	 * there — the very bug `RestrictTo` exists to fix, re-made one stage earlier. So the slab is grown
	 * from the skin the author pointed at and stops where the surface stops being connected INSIDE it:
	 * at a knee's height the two legs are separate pieces, and no distance constant has to be chosen
	 * for that to be true.
	 *
	 * A region that is already a full band has no patch to start from, so it starts at the triangle
	 * nearest its own centre — the same sentence, for a region whose author pointed at all of it.
	 */
	static void SelectRing(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span,
		TSet<int32>& OutTriangles);
};
