// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"

class USkeletalMesh;
namespace GraphCodeMove::PoseAccess { struct FBuild; }

/**
 * MOVE-PARAM1 SLICE 0 (Docs/GC-MOVE-POSE-PANEL-INTEGRATION.md, sections 0 and 4): THE POSE SESSION.
 *
 * One `.gcmove` and one of its clips (a pose: a one-frame clip such as a key or a stance). Each Pose() takes the file's text as it now stands (the
 * panel writes the person's values into it at the call) and answers the clip's pose AS WRITTEN: its lines solved as the build solves them, with no
 * body clearance, no collision, no balance, no timing and nothing written. Game thread only in slice 0.
 *
 * The first ask prepares the build in full; every later ask with the same header reads only the text again (N1: the character, rig, skeleton and
 * physics stay bound). The IK rig's solver is still made afresh inside every solve (GcMoveSolve::Run), so every ask's solve starts from nothing
 * ("reset per ask", ruling 8); FGcMovePoseResult times its parts so the warm solver (N2) is built only if they are where the milliseconds are.
 */

/** A joint held at its stop, or a chain left short of what its line asked: the build's own note, read into numbers. */
struct GRAPHCODEMOVE_API FGcMovePoseStop
{
	/** A bone (`spine_04`) for a joint at its stop; the line's chain (`$LookH`) for an aim or a reach left short. */
	FString Name;
	/** Degrees past the stop for a joint; degrees or centimetres short for a chain. */
	double Amount = 0.0;
	bool bJoint = true;
	bool bCentimetres = false;
	/** The line of the file for a chain (0 for a joint). */
	int32 Line = 0;
	/** The build's sentence, whole. */
	FString Said;
};

struct GRAPHCODEMOVE_API FGcMovePoseResult
{
	/** The body's bones (its reference skeleton), each with its parent's index (INDEX_NONE for a root) and its LOCAL transform in this pose. */
	TArray<FName> Bones;
	TArray<int32> Parents;
	TArray<FTransform> Local;
	/** RULING 106 SLICE 3: the pose's control curves (a face's drive() lines), name and native value, for the live preview; empty for a body. */
	TArray<FName> Curves;
	TArray<float> CurveValues;
	TArray<FGcMovePoseStop> Stops;
	/** What the pose as written leaves to the whole animation (`balance(...)`, `collision(...)`, a physics bake), one sentence each. */
	TArray<FString> LeftOut;
	/** The preparation's and the solve's notes, as a build prints them. */
	TArray<FString> Notes;
	double PrepareMs = 0.0, SolveMs = 0.0, TotalMs = 0.0;
	/** True where this ask prepared the build afresh (the first ask, a changed header); false where it read only the text again (N1). */
	bool bFullPrepare = false;
	/** Inside SolveMs: GcMoveSolve::Run's calls, its preparation of the transient solve rig and the IK rig processor's initialisation (ms). */
	int32 SolveRuns = 0;
	double SolveRigMs = 0.0, SolveInitializeMs = 0.0;
	/** Inside PrepareMs when only the text was read again (N1): reading it with the parameter pass, binding, the resolve, the checks (ms). */
	double ParseMs = 0.0, BindMs = 0.0, ResolveMs = 0.0, CheckMs = 0.0;

	/** Every bone's transform in the component's space (parents before children, as a reference skeleton orders them). */
	void ComponentSpace(TArray<FTransform>& Out) const;
};

/** RULING 106 SLICE 1: one reading of a joint as [readings] prints it (the IK rig's limit reader, the rig's word and sign), and its stops in that word. */
struct GRAPHCODEMOVE_API FGcMovePoseReading
{
	/** fold, side or twist. */
	FString Word;
	double Value = 0.0;
	/** The rig's stops on this reading; a free axis has none (bRange false, -180..180). */
	bool bRange = false;
	double Min = -180.0, Max = 180.0;
};

class GRAPHCODEMOVE_API FGcMovePoseSession
{
public:
	/** Use GraphCodeMove::OpenPoseSession, which also prepares the text once. */
	FGcMovePoseSession(const FString& InDocumentPath, const FString& InClip) : Path(InDocumentPath), ClipName(InClip), AskedClip(InClip) {}

	const FString& DocumentPath() const { return Path; }
	const FString& Clip() const { return ClipName; }
	/** The clips the file's build lines make, in build order, as the last preparation read them. */
	const TArray<FString>& Clips() const { return ClipNames; }
	/** The header's character:, part: and worn: as the last preparation read them. */
	const FString& CharacterPath() const { return Character; }
	const FString& PartName() const { return Part; }
	const FString& Worn() const { return WornText; }
	/** RULING 106 SLICE 3: the header's body: (a face file's body clip), as the last preparation read it; empty for a body file. */
	const FString& BodyClip() const { return BodyClipPath; }
	USkeletalMesh* BodyMesh() const { return Body.Get(); }

	/** The package the build line of a clip writes, empty when none (or before a preparation). */
	FString PackageOf(const FString& InClip) const;

	/** Choose another clip of the file (a pose of the strip), named as a build line shows it: `KZ6`, `$KZ6` or the asset `A_T_KZ6`. */
	bool SetClip(const FString& Clip, FString& OutError);

	/** The chosen clip's pose as written, from this text. False with the build's own refusal. */
	bool Pose(const FString& ProgramText, FGcMovePoseResult& Out, FString& OutError);

	/** Only read the text: the clips, the character, which asset each build line writes; no solve. THE OWNER, 19 Nov ("it should be loading the
	 * animation, not rebuilding it every time"; Saved/Research/MOVE-PARAM1/REVIEW-LOAD-NOT-BUILD.md): this is the header and the build lines read
	 * from the text alone (the program read, no build preparation); the build's preparation runs when something needs it (Pose, PoseAsBuilt,
	 * Anatomy, Readings, Prepare). A file the build would refuse is refused there, not here. */
	bool Read(const FString& ProgramText, FString& OutError);
	/** The build's preparation of the text Read last read, when there is none yet (a pose solved, converted, a joint's readings). False with its
	 * refusal. */
	bool Prepare(FString& OutError);
	/** Whether the build's preparation has run on this session (Read alone does not). */
	bool IsPrepared() const { return Build.IsValid(); }

	/** RULING 106 SLICE 1 ("Convert to presets"): the clip's pose AS BUILT at Sample (PoseAccess::EvaluateAsBuilt, the whole evaluation a build makes before writing). */
	bool PoseAsBuilt(const FString& ProgramText, int32 Sample, FGcMovePoseResult& Out, FString& OutError);
	/** The body the last preparation solved on (null before one). */
	const struct FReferenceSkeleton* Anatomy() const;
	/** A joint's readings (fold, side, twist where the rig declares them) on Local, its turn as a local rotation, and their stops: the bone override
	 * sliders. False where the IK rig declares no joint on Bone (or there is no preparation). */
	bool Readings(FName Bone, const FQuat& Local, TArray<FGcMovePoseReading>& Out) const;

private:
	FString Path, ClipName, Character, Part, WornText, BodyClipPath;
	TArray<FString> ClipNames;
	TWeakObjectPtr<USkeletalMesh> Body;
	/** The text Read read last (the preparation's text when one is asked for later). */
	FString ReadText;
	/** The prepared build the asks reuse (N1); reset after a refusal, so the next ask prepares afresh. */
	TSharedPtr<GraphCodeMove::PoseAccess::FBuild> Build;
	/** The clip as it was asked for, until a preparation says which entry it names. */
	FString AskedClip;
	/** Each clip's built asset, as the last preparation that succeeded read it (Play needs it even after a refused solve). */
	TMap<FString, FString> Packages;
	/** The preparation half of Pose (Read and Pose share it). */
	bool PrepareText(const FString& ProgramText, FGcMovePoseResult& Out, FString& OutError);
};

namespace GraphCodeMove
{
	/** Open a pose session on a `.gcmove` (its package path, as gc_apply addresses it) and its text, for one clip: the text is prepared once here,
	 * so a file the build would refuse, or a clip no build line makes, is refused here by the build's own words. */
	GRAPHCODEMOVE_API bool OpenPoseSession(const FString& DocumentPath, const FString& ProgramText, const FString& Clip,
		TSharedPtr<FGcMovePoseSession>& OutSession, FString& OutError);
}
