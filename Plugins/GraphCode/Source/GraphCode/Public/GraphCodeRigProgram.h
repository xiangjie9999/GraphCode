// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"

// What Core knows about a `.gcrig` (`Docs/GC-RIG-GRAMMAR.md`): the header word and one seam, as
// `GraphCodeShapeProgram.h`, `GraphCodeMoveProgram.h` and `GraphCodeHumanProgram.h` do for theirs.

/**
 * WHAT A SKELETAL MESH A `.gcrig` BUILT REMEMBERS ABOUT THE PROGRAM THAT MADE IT — ledger §7's
 * `rig: "…" program:<digest>`, and `FGcMoveAssetProvenance` with the nouns swapped.
 *
 * **THIS IS WHAT LETS A DOCUMENT REFUSE TO CONJURE A SKELETON AND A PROGRAM BUILD ONE ANYWAY.**
 * `kind:skeleton` and `kind:skeletal_mesh` both carry `reference_skeleton` and `geometry` on their
 * `omits:` lines, so a `.gc` may not invent them — exactly as every static mesh here carries
 * `omits: geometry` while a `.gcshape` builds its geometry and stamps what it did. The stamp is a
 * fact about the asset, so reading it is Core's; only the stamping is the product's.
 */
struct GRAPHCODE_API FGcRigAssetProvenance
{
	/** The `.gcrig` this asset was built from, as the caller addressed it. */
	FString Source;

	/** A digest of the program text that built it. */
	FString Program;

	/**
	 * A digest of the SKELETON as built — every bone's name, parent and rest transform.
	 *
	 * **Kept apart from `Program` because the two answer different questions.** A changed program
	 * with an unchanged skeleton is an edit to skin or limits; an unchanged program with a changed
	 * skeleton is somebody editing the asset by hand, which is the one this exists to catch.
	 */
	FString Skeleton;

	/** A stamp names both a source and a program; either alone is not one. */
	bool IsSet() const { return !Source.IsEmpty() && !Program.IsEmpty(); }

	static const TCHAR* SourceKey();
	static const TCHAR* ProgramKey();
	static const TCHAR* SkeletonKey();

	/** Read the stamp off an asset. False, and `Out` untouched, where there is no stamp. */
	static bool Read(const UObject* Asset, FGcRigAssetProvenance& Out);

	/** Write the stamp. It does not save: the build's own save carries it. */
	static void Stamp(UObject* Asset, const FGcRigAssetProvenance& In);
};

/** The pose a rig was AUTHORED in — grammar §3's `stance:`, user-ruled 2026-09-17 (ledger row 75). */
enum class EGcRigStance : uint8
{
	APose,
	TPose,
};

/**
 * **THE STANCE A `.gcrig` DECLARED, LEFT ON THE SKELETON SO THE OTHER SIDE OF A RETARGET CAN ASK.**
 *
 * **IT IS DECLARED RATHER THAN DERIVED, WHICH IS THE WHOLE REASON THIS EXISTS.** Two rigs authored
 * in different stances retarget wrong, and nothing on a skeleton says which is which: the engine
 * falls back to each one's rest pose and hopes. A stance guessed from where the arms point is
 * silent when it is wrong, so the fact is typed once and written down — the rule `faces:` already
 * follows for the same reason.
 *
 * **THE SAME SHAPE AND THE SAME PLACE AS `FGcRigAxisRecord`**: package metadata on the skeleton,
 * which is editor data, stripped on cook, and read only by an authoring build. The reader here is
 * the retargeter half of a LATER build, over somebody else's rig, which is why it goes on the asset
 * rather than staying in the document.
 */
struct GRAPHCODE_API FGcRigStanceRecord
{
	/** The metadata key the word lives under. */
	static const TCHAR* Key();

	/** The word a `.gcrig` types, and the one this record stores — one spelling for both. */
	static const TCHAR* Word(EGcRigStance Stance);

	/** `a_pose` or `t_pose`, case-insensitively. False, and `Out` untouched, for anything else. */
	static bool Parse(const FString& Text, EGcRigStance& Out);

	/** Read the stance off a skeleton. False where none was declared. */
	static bool Read(const UObject* Skeleton, EGcRigStance& Out);

	/**
	 * Write it, or REMOVE it where the file declares none — a rebuild adopts the skeleton standing
	 * there, so an unset stance that left the old word behind would answer for a document that
	 * stopped saying it. `FGcRigAxisRecord::Write` drops an empty record for the same reason.
	 */
	static void Write(UObject* Skeleton, TOptional<EGcRigStance> Stance);
};

/**
 * **THE THREE AXES A JOINT DECLARED, WRITTEN ONTO THE SKELETON SO A MOTION FILE NEED NOT WORK THEM
 * OUT AGAIN** — ledger §9, owed since the hand trial found out what happens when it does.
 *
 * **THE FACT ALREADY EXISTS AND WAS BEING THROWN AWAY.** `FGraphCodeRigLimits::Resolve` derives,
 * from the geometry, which of a bone's own axes its `range=` folds about, which its `side=` swings
 * about and which its `twists=` rolls about. That answer writes the ragdoll and the IK rig and was
 * then dropped, so a `.gcmove` animating the same skeleton had to derive it a second time, by hand,
 * from a different asset.
 *
 * **AND THAT SECOND DERIVATION IS THE ONE THAT BROKE** (ledger §7e). The IK rig states a bone's
 * axes **against its parent**; `.gcmove`'s `axis=` is **the bone's own**. Reading the first as the
 * second — the obvious thing to do, since both came off the same `range=` lines — turned every
 * knuckle of the trial hand into a sideways splay. Nothing refused. Both builds reported success.
 * Only a render showed it.
 *
 * **SO THIS IS A DELETION RATHER THAN A FEATURE**: it removes the step where a derived fact is
 * copied between two files by hand, which is the step that has already failed once.
 *
 * **IT RECORDS WHAT THE FILE DECLARED AND NOTHING MORE.** A joint with no `side=` gets no side axis
 * here even though one is derivable — the same rule the ragdoll takes (§7i), where a bone that says
 * nothing at all is never in the list. A `.gcmove` asking for an axis the rig never declared is
 * refused by name, which is the answer that sends its author back to the `.gcrig`.
 *
 * **PACKAGE METADATA, WHICH IS EDITOR DATA, AND THAT IS THE POINT.** The only reader is an
 * authoring build. `USkeleton` does carry an `AssetUserDataEditorOnly` array, but `AddAssetUserData`
 * only ever writes the COOKED one, so a `UAssetUserData` subclass of ours would put an editor-module
 * `UCLASS` into a cooked package — the trap that is already written down. Metadata is stripped on
 * cook and cannot reach a shipped build at all.
 */
struct GRAPHCODE_API FGcRigBoneAxes
{
	FString Bone;

	/**
	 * Each axis as a SIGNED unit vector in the bone's own space, zero where the joint declared
	 * nothing of that kind.
	 *
	 * **SIGNED, BECAUSE THE SIGN IS THE WHOLE VALUE OF THE FOLD.** Positive about `Fold` carries the
	 * joint FURTHER THE WAY IT IS ALREADY BENT, which is a measurement rather than a convention: a
	 * knee, a hock and an elbow are all slightly folded at rest and that fold is the sign. So
	 * `by=70` on a fold means seventy degrees further into the bend, and `range=(-5, 145)` is read
	 * against the same zero.
	 */
	FVector Fold = FVector::ZeroVector;
	FVector Side = FVector::ZeroVector;
	FVector Twist = FVector::ZeroVector;

	/**
	 * Degrees the joint's rest turns it about `Twist` (rest=(twist=N) or twist=forward), its innermost rest turn; 0 for none. Its
	 * `Fold` and `Side` above are said in the frame after that twist (the directions they had before it), so a swing laid on
	 * the rest turns about the anatomical axes; Move reads the joint's limits twist-first (2026-10-18, the cat's limp).
	 */
	double RestTwist = 0.0;

	bool IsSet() const { return !Bone.IsEmpty() && !(Fold.IsZero() && Side.IsZero() && Twist.IsZero()); }
};

/** Which of the three a caller is asking for — the words a `.gcmove` writes in its `axis=` slot. */
enum class EGcRigDeclaredAxis : uint8
{
	Fold,
	Side,
	Twist,
};

struct GRAPHCODE_API FGcRigAxisRecord
{
	/** One entry per joint that declared something, in the order the file declares them. */
	TArray<FGcRigBoneAxes> Bones;

	/** The metadata key the whole record lives under. */
	static const TCHAR* Key();

	/** The bone's entry, matched exactly as a bone name is, or null. */
	const FGcRigBoneAxes* Find(const FString& Bone) const;

	/** The word a `.gcmove` writes, and the one a refusal prints. */
	static const TCHAR* NameOf(EGcRigDeclaredAxis Which);

	/** That axis off one entry — zero where the joint did not declare it. */
	static FVector AxisOf(const FGcRigBoneAxes& Entry, EGcRigDeclaredAxis Which);

	/**
	 * Degrees past which a recorded direction is not really the bone axis it is nearest to. Rig reports
	 * a limit written that far off as an approximation, and Move then clamps a hinge about the recorded
	 * direction instead of the axis, so the two cannot disagree about where the line is.
	 */
	static double TooFarOffAnAxis() { return 15.0; }

	/**
	 * **ONE KEY HOLDING THE WHOLE RECORD, NOT ONE PER BONE, AND THAT IS WHAT MAKES A REBUILD
	 * CORRECT.** A rebuild that declares fewer joints than the run before must not leave the
	 * dropped ones standing; a single value is replaced whole, where a key per bone would have to
	 * remember what to delete and would quietly keep an axis for a joint the file no longer has.
	 *
	 * **SPELLED THE WAY THE GRAMMAR SPELLS AN AXIS**, so the value reads as the thing it means:
	 * `knee_B_L fold=+Z side=+Y twist=+X; hip_B_L fold=-Y`.
	 */
	static FString ToText(const FGcRigAxisRecord& In);
	static bool FromText(const FString& Text, FGcRigAxisRecord& Out);

	/** Read the record off a skeleton. False, and `Out` untouched, where there is none. */
	static bool Read(const UObject* Skeleton, FGcRigAxisRecord& Out);

	/** Write it. It does not save: the build's own save carries it. */
	static void Write(UObject* Skeleton, const FGcRigAxisRecord& In);
};

/**
 * **A RECORD FOR A SKELETON THE BUILD MAY NOT WRITE, ANSWERED BY `FGcRigAxisRecord::Read` FOR AS LONG AS THIS
 * LIVES** (GC-MOVE-METAHUMAN-BODY-ADAPTER-PLAN.md item 3, user-ruled 2026-10-09). A `controls:` adapter's joints are
 * worked out at build time over a skeleton outside /Game — MetaHuman's, Epic's and shared by every body of its type —
 * so nothing is on the asset to read. Every reader already asks `Read`: `axis=` on a swing, a `bone.fold` control, a
 * hinge's limit and the body's rigid pieces. One door is what keeps the four agreeing, where a record threaded through
 * each would be four chances to forget one (the second-writer lesson). Made and dropped on the same thread; a second
 * one for the same skeleton on that thread stands in front of the first until it goes.
 */
struct GRAPHCODE_API FGcRigAdapterAxes
{
	FGcRigAdapterAxes(const UObject* Skeleton, const FGcRigAxisRecord& Record);
	~FGcRigAdapterAxes();
	FGcRigAdapterAxes(const FGcRigAdapterAxes&) = delete;
	FGcRigAdapterAxes& operator=(const FGcRigAdapterAxes&) = delete;

	/** The record standing for this skeleton now, or null where no build holds one. */
	static const FGcRigAxisRecord* Find(const UObject* Skeleton);

private:
	const UObject* Skeleton = nullptr;
	uint32 ThreadId = 0;
	TOptional<FGcRigAxisRecord> Previous;
};

/**
 * What a `.gcrig` build says about the hull bodies it made (GC-RIG-IMPORTED-MESH.md,
 * GC-MOVE-GRIP-SEATING.md). Its presence on a physics asset means the bodies are hulls of the skin, so
 * a `.gcmove` measures a held thing against the skin itself and not against the hulls.
 */
struct GRAPHCODE_API FGcRigBodyFit
{
	/** How far each body's hull stands off its bone's skin, in cm, as the build measured it. */
	TMap<FName, double> StandsOff;

	/** `[gives=]`: how far, in cm, a held thing may press into the skin. Zero is a rigid hand. */
	double Gives = 0.0;

	/** `skin { <region> [gives=] }`: skin that gives other than `Gives`, per vertex: its rest position in mesh
	 * space (XYZ) and how far a held thing may press into it there (W, cm). */
	TArray<FVector4> Soft;

	/** OBJ1t (Docs/GC-MOVE-DRAWN-BODY-PLAN.md): a skeletal mesh worn over the record's mesh and the bodies made of ITS skin, by bone name. Written
	 * only on a build's transient copy of the asset (GcMoveCollisionCoverage::Drawn); FGcBodyCollision then reads that mesh's skin for those
	 * bodies, each vertex in its own mesh's rest, so posing it by the part's bones is leader-pose skinning. */
	/** Gives (OBJ1v): how far, in cm, a body made of that mesh gives in a pair against another body, cloth over flesh; never less than `Gives`.
	 * Zero where none was written (a record from before OBJ1v): the record's own `Gives` then. */
	struct FWorn { FString Mesh; TArray<FName> Bodies; double Gives = 0.0; };
	TArray<FWorn> Worn;

	/** One key holding the whole record, replaced whole, as the axis record is. */
	static const TCHAR* Key();

	/** Read the record off a physics asset. False, and `Out` untouched, where there is none. */
	static bool Read(const UObject* PhysicsAsset, FGcRigBodyFit& Out);

	/** Write it, or remove it when empty. It does not save: the build's own save carries it. */
	static void Write(UObject* PhysicsAsset, const FGcRigBodyFit& In);
};

/**
 * **A `USkeleton` CARRIES A SECOND ARRAY BESIDE ITS REFERENCE SKELETON, AND THE ENGINE ASSERTS THE
 * TWO ARE THE SAME LENGTH.** `BoneTree` holds one `FBoneNode` per raw bone, and the only thing left
 * on it is the per-bone translation retargeting mode. `USkeleton::MergeBonesToBoneTree` checks
 * `NumBones == ReferenceSkeleton.GetRawRefBoneInfo().Num()` outright, so a skeleton whose tree is
 * short is malformed by the engine's own definition rather than merely unfinished.
 *
 * **AND THE SETTER IS UNGUARDED WHERE EVERY READER IS GUARDED.**
 * `USkeleton::GetBoneTranslationRetargetingMode` asks `BoneTree.IsValidIndex` first;
 * `SetBoneTranslationRetargetingMode` is `BoneTree[BoneIndex] = …` (`Skeleton.cpp:1073`). A caller
 * that validated its bone against the REFERENCE skeleton — the obvious, and wrong, place to validate
 * it — therefore holds an index that is valid for one array and out of bounds for the other, and the
 * write lands in whatever follows. It killed a measurement process before it could report anything
 * (ledger §11), which is how it was found.
 *
 * **REFLECTION, BECAUSE THE ENGINE OFFERS NO OTHER DOOR.** `BoneTree` is `protected` and every
 * public route to it — `MergeAllBonesToBoneTree`, `RecreateBoneTree` — takes a `USkinnedAsset`, and
 * a `.gcrig` may build a skeleton with no mesh anywhere near it. What goes in is what the engine's
 * own mesh path puts there: `CreateReferenceSkeletonFromMesh` does `BoneTree.AddZeroed(NumBones)`
 * and a zeroed `FBoneNode` is `EBoneTranslationRetargetingMode::Animation`, the default a bone that
 * has never been ruled about should have.
 *
 * It is Core's for the reason [[FGcRigAxisRecord]] is: **Rig writes it and Move reads it**, and
 * neither product should have to depend on the other to agree about an engine fact.
 */
struct GRAPHCODE_API FGcSkeletonBoneTree
{
	/** How many nodes stand today, or `INDEX_NONE` where the object is not a skeleton at all. */
	static int32 Num(const UObject* Skeleton);

	/** Whether the tree covers every raw reference bone — the engine's own invariant, asked. */
	static bool Covers(const UObject* Skeleton);

	/**
	 * Size the tree to the reference skeleton, adding default nodes and dropping any past the end.
	 * True where it changed something. It does not save, and it does not mark anything dirty: the
	 * build that just wrote the bones carries both.
	 */
	static bool Cover(UObject* Skeleton);
};

/** What building a rig did, or would do. Filled by GraphCode Rig; Core only carries it. */
struct GRAPHCODE_API FGcRigBuildResult
{
	bool bRan = false;

	/** Every asset a `build { }` line put on disk, by package path, in declaration order. */
	TArray<FString> BuiltAssets;

	/** One line per create and per rebuild. */
	TArray<FString> Changes;

	/**
	 * Worth saying and not worth refusing — a bone that twists with no band to spread the roll
	 * along, a socket the mesh still owes a surface point, a region nothing uses.
	 */
	TArray<FString> Notes;

	FString Message;
};

using FGcRigProgramBuilder = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	FGcRigBuildResult& /*OutResult*/,
	FString& /*OutError*/)>;

/**
 * Does this program still hold the recipe for one asset it built? TEXT ONLY, nothing evaluated —
 * `FGcShapeRecipeResolver` with the nouns swapped, asked by the corpus sweep once per `rig:` line.
 * True means a `build { }` line produces `PackagePath`; a `.gcrig` declares ONE rig, so unlike
 * Shape's there is no entry name to check as well.
 */
/**
 * **A RIG READ BACK OUT OF A BUILT ASSET — AND IT RUNS THE PIPE THE OTHER WAY, WHICH NOTHING ELSE
 * HERE DOES.** Every other authored format in this product flows one way: a declaration builds an
 * asset, and the asset's `.gc` sidecar REFLECTS what it ended up saying. This turns a built skeleton
 * back into a DECLARATION — upstream text a person then edits.
 *
 * **SO IT IS A SEED AND NEVER A PROJECTION, and the difference is the whole hazard.** A sidecar may
 * be regenerated at any time because it owns nothing; this text, once a person has written `range=`,
 * regions and skin onto it, holds intent no skeleton contains. Re-reading the asset would hand back
 * the bare spine and silently drop all of it. Nothing may therefore treat this as refreshable, and
 * the readout's own first note says so in the text it produces.
 */
struct GRAPHCODE_API FGcRigReadResult
{
	/** The `.gcrig` text — a declaration that would rebuild the skeleton it was read from. */
	FString Text;

	int32 Bones = 0;

	/** Bones with no child: where a chain ends. */
	int32 Leaves = 0;

	/** What the asset could not say, and what was found in it worth knowing. */
	TArray<FString> Notes;
};

/**
 * An asset path in, `.gcrig` text out. Filled by GraphCode Rig; Core only carries it, exactly as it
 * carries the builder — a Core-only install must be able to say *install GraphCode Rig* without
 * this module's code.
 */
using FGcRigAssetReader = TDelegate<bool(
	const FString& /*AssetPath*/,
	FGcRigReadResult& /*OutResult*/,
	FString& /*OutError*/)>;

using FGcRigRecipeResolver = TDelegate<bool(
	const FString& /*ProgramText*/,
	const FString& /*PackagePath*/,
	FString& /*OutReason*/)>;

class GRAPHCODE_API FGraphCodeRigProgram
{
public:
	/** `gcrig 1` on the first line that is not a comment. */
	static bool LooksLikeRigProgram(const FString& Text);

	/** The whole seam or none of it: a half-bound one answers `IsAvailable()` and then refuses. */
	static bool Register(EGcProduct Product, FGcRigProgramBuilder Builder,
		FGcRigRecipeResolver Resolver, FGcRigAssetReader Reader, FString& OutError);
	static void Unregister(EGcProduct Product);

	/** Installed AND bound. A folder on disk whose module never started is not an installation. */
	static bool IsAvailable();

	/** Build the program, or refuse naming GraphCode Rig as the fix. */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcRigBuildResult& OutResult, FString& OutError);

	/**
	 * The sweep's question. False with `OutReason` is *the recipe is gone*; false with `OutError` is
	 * *no product here to ask*, which a Core-only install must not report as every rig-built asset
	 * being broken.
	 */
	static bool HoldsRecipeFor(const FString& ProgramText, const FString& PackagePath,
		FString& OutReason, FString& OutError);

	/**
	 * Read a built skeleton or skeletal mesh back out as `.gcrig` text, or refuse naming GraphCode
	 * Rig as the fix.
	 *
	 * **THE RESULT IS A STARTING DECLARATION, NOT THIS ASSET'S TEXT.** See `FGcRigReadResult`: a
	 * caller that stores it beside the asset and refreshes it has built a second carrier for intent
	 * the asset does not hold.
	 */
	static bool Read(const FString& AssetPath, FGcRigReadResult& OutResult, FString& OutError);
};
