// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeCodecs.h"

/**
 * `kind:control_rig` — GRAPHCODE RIG'S FIRST WHOLE KIND, AND IT LIVES HERE RATHER THAN IN CORE.
 *
 * `GC-RIG-LEDGER.md` §12.10. The obvious place for this was a `ControlRig` branch in
 * `GraphCodeProjector.cpp`'s `BuildDocument`, beside the Material ones, and that would have been
 * wrong in a way no spec of it would have caught:
 *
 * **CORE HARD-LINKS `MaterialIR`, WHICH IS SAFE BECAUSE MaterialIR NEEDS ONLY ALWAYS-PRESENT ENGINE
 * MODULES.** `ControlRigIR` needs the OPTIONAL `ControlRig` plugin, and Core linking that would
 * break the one thing the product model exists for — a build without the plugin still has to
 * classify a `UControlRigBlueprint` and write an honest `omits: all@rig` stub for it. So Core keeps
 * the token, the class path and the three registrations a kind needs, and the CODEC sits on the
 * product seam `FGraphCodeCodecs::RegisterKindCodec` already provides. Core Extension's MetaSound
 * codec is the precedent, one optional plugin over.
 *
 * **AND RIG IS ALSO THE PRODUCT THAT WRITES THESE ASSETS**, which is not the argument but is what
 * getting it wrong would have cost: a customer with Rig generates a Control Rig from `bulge=`, and
 * a codec living in a product they had not bought would make the one thing their own build produced
 * the one thing it could not read back.
 */
namespace GraphCodeRig
{
	/**
	 * True for an exact `UControlRigBlueprint`.
	 *
	 * **EXACT, MATCHING `GcControlRigClassPath()`'s OWN EXACTNESS**, and here the exactness has
	 * teeth: a `UControlRigBlueprint` IS a `UBlueprint`, so a subclass test would let this projector
	 * claim assets Core classifies as `kind:blueprint`, and the two disagreeing is a file that never
	 * gets written with no error anybody sees.
	 */
	GRAPHCODERIG_API bool IsProjectableControlRig(const UObject* Asset);

	/** Everything Rig supplies for `kind:control_rig`: four blocks, one projector, no factory. */
	GRAPHCODERIG_API FGcProductKindCodec ControlRigCodec();
}

/**
 * **`program { override = true }` — A PERSON HAS TAKEN THIS GENERATED RIG OVER, SO A REBUILD LEAVES
 * IT STANDING.** `GC-RIG-LEDGER.md` §13 item 3, user-raised: *a rebuild replaces what a `.gcrig`
 * made, so a hand-tweak to a generated rig is lost.*
 *
 * A `.gcrig` build renames the Control Rig standing at its output name into `/Engine/Transient` and
 * writes a fresh one over it. That is right for an asset the last run wrote and nobody has touched,
 * and it is silent destruction for one a rigger opened and added a control to — the same failure
 * `MayBuildOver` already refuses for an asset no `.gcrig` made, one step further along: here the
 * stamp says yes, and the answer still has to be no.
 *
 * **THE ASSET CARRIES IT AND THE DOCUMENT REFLECTS IT, which is the way round that survives.** The
 * `.gc` is a projection — `gc_sync` rewrites it from the asset whenever the asset moves on — so a
 * marker that lived only in the file would be erased by the next sync with nothing to show for it,
 * and a protection that disappears in silence is worse than none. So the fact sits in package
 * metadata, the projector reflects it into `program { }`, and `gc_apply` on that block is where a
 * person writes it. Same place and shape as `FGcRigStanceRecord`: editor data, stripped on cook,
 * read by an authoring build and nothing else.
 *
 * **AND IT ONLY MEANS ANYTHING ONCE THE PACKAGE IS SAVED — measured 2026-09-17, and it cost a
 * morning.** A marker written and left in memory was read back true on the object that had just
 * been handed it, and was GONE from the package's metadata map when the next build asked for it
 * moments later — while the provenance keys beside it, which a previous build had saved, were still
 * there. So an unsaved metadata write does not survive a build in the same process, whatever
 * refreshes the map. `gc_apply` saves after every product write, which is the route a person takes;
 * anything that calls `Write` directly and does not save has written nothing that lasts, and the
 * spec that proves this saves for exactly that reason.
 *
 * **ABSENT IS FALSE, AND CLEARING REMOVES THE KEY** rather than writing `false` — a rebuild may
 * replace this rig is the default state of every rig a `.gcrig` writes, and `FGcRigStanceRecord`
 * drops an unset record for the same reason: a key left behind answers for a document that has
 * stopped saying it.
 */
struct GRAPHCODERIG_API FGcControlRigOverrideRecord
{
	/** The metadata key the marker lives under. */
	static const TCHAR* Key();

	/** True where a person has taken this rig over. False for every rig that has no marker. */
	static bool Read(const UObject* Rig);

	/** Set it, or REMOVE it where the document says `override = false`. It does not save. */
	static void Write(UObject* Rig, bool bOverridden);
};
