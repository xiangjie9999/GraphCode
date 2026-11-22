// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;

/**
 * `.gcmove`'s `drive()` of a curve channel on a face (GC-MOVE-FACE-EXPRESSIONS-PLAN.md, the curve-name check): a curve name the face
 * never reads would be written into the clip and consumed by nothing, so it is checked against what the face consumes. Move asks; the
 * product that can read a face's DNA answers (Advanced's MetaHuman module, through RigLogic). Move never links MetaHuman or RigLogic,
 * and what it gets back is a list of names.
 *
 * The domain is the DNA's RAW controls and no more. RigLogic makes each raw control `<object>.<attribute>` the curve
 * `<object>_<attribute>` (DNAIndexMapping.cpp:33-55), so a name is refused only when it begins with one of those objects
 * (`CTRL_expressions_`) and is not one of the curves. A name outside that domain (`CTRL_ML_*` neural masks, a custom curve) is not
 * refused: it is unverified, and the build says how many. Skeleton curve metadata is not the consumer and is not asked.
 */
struct GRAPHCODE_API FGcFaceControls
{
	enum class EStatus : uint8
	{
		/** No product registered a reader: nothing is known. */
		NoReader,
		/** The mesh carries no DNA (a part that is not a MetaHuman face), or there is no mesh: nothing is known and nothing is wrong. */
		NoDna,
		/** Every raw control of the DNA was read: Curves and Objects are complete. */
		Known,
		/** A DNA is there and its raw controls could not be read in full: Curves and Objects are empty and Note says why. */
		Unreadable,
	};

	struct FList
	{
		EStatus Status = EStatus::NoReader;
		/** Known: the curve each raw control becomes, `<object>_<attribute>`. FNames, as RigLogic keys them, so case does not tell two apart. */
		TSet<FName> Curves;
		/** Known: the `<object>` of every raw control, `CTRL_expressions` for `CTRL_expressions.jawOpen`. */
		TSet<FString> Objects;
		/**
		 * What this list describes, or why it says nothing, written so a sentence can end with it. After Read: Known, "<mesh>'s DNA";
		 * NoDna, "<mesh> carries no DNA" (or why there is no mesh); NoReader, why nothing could read it; Unreadable, "<mesh>'s DNA could
		 * not be read: <why>". A reader sets it only for Unreadable, to the why; Read writes the rest.
		 */
		FString Note;
	};

	/**
	 * The product's answer for one face mesh: fill Out.Status (NoDna, Known or Unreadable) and, for Known, Curves and Objects. Return
	 * false only when it cannot answer at all; Read then reports the face Unreadable. A reader must not publish a partial Known.
	 */
	using FReader = TDelegate<bool(USkeletalMesh&, FList&)>;
	static void RegisterReader(FReader Reader);
	static void UnregisterReader();

	/** What the face's DNA consumes, through the registered reader. A null Face (a `skeleton:` file selects no part) is NoDna. */
	static FList Read(USkeletalMesh* Face);

	/**
	 * The rule, with nothing read here. True when `Curve` may be written, and bOutUnverified says the DNA could not vouch for it
	 * (outside the raw-control domain, NoDna, NoReader). False with OutRefusal when it must not: inside the domain and not a raw
	 * control (the three nearest names by edit distance are offered), or Unreadable (OutRefusal is the list's Note).
	 */
	static bool Check(const FList& List, const FString& Curve, FString& OutRefusal, bool& bOutUnverified);
};
