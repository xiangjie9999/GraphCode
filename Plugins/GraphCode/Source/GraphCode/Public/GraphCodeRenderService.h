// `gc_render` — the whole render, from an asset path to pixels on disk.
//
// `GC-VISION-DESIGN.md` §6.1: the implementation belongs in the GraphCode module and exposes a
// provider-neutral C++ request and result, so the MCP tool stays a thin adapter. Nothing in this
// header knows about JSON, MCP or a tool schema.
//
// It sits on three layers that were built and proved before it:
//   - `GraphCodeRender.h`      — the camera. Where to stand, headless and fixtured.
//   - `GraphCodeRenderOutput`  — where the picture goes, decided before anything renders.
//   - the phase 0 finding      — reuse the engine's thumbnail preview scene for its WORLD, and
//                                never its `CreateView`, which fixes the aspect at 1:1 and reads
//                                the camera off the asset (`GC-VISION-HANDOFF.md` phase 0).
//
// WHAT `neutral` IS, stated because a render profile that is not stated is a hidden variable: the
// engine's thumbnail preview world with its sky light kept, its floor plane and visible sky sphere
// HIDDEN, and its three directional lights RE-AIMED onto `NeutralLightRig()` below. The lighting
// stays because a metal with nothing to reflect renders black and lies about the material; the
// scenery goes because it is not the subject. Phase 0 found that both were on by default and in
// every picture, and `FMaterialThumbnailScene` does not accept `FConstructionValues`, so they are
// switched off through `FPreviewScene::Components` after the fact rather than never created — and
// the lights are re-aimed the same way, by ownership, for the same reason.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRender.h"

class AActor;
class UBlueprint;
class UMaterialInterface;
class UAnimSequenceBase;
class USkeletalMesh;
class UObject;
class UWorld;
class FSceneInterface;
class FSceneViewStateReference;
class UTextureRenderTarget2D;
class FPrimitiveComponentId;

/**
 * An Actor Blueprint's `gc_render simulate:true physics:true` world, built exactly as gc_render builds it (the physics scene
 * prepared before the spawn, an effects system, the actor at its authored place, BeginPlay, every skeletal body posing each
 * tick, a clip's wind() on a wind source), for a caller that ticks and draws it itself — one tick, then one draw, which is what
 * a hair readback needs and gc_render's many-ticks-per-picture loop does not give. `[bake]`'s Groom recorder
 * (GC-GARMENT-HAIR-BAKE.md R3). Every tick and every draw is a frame of its own; the editor's frame clock is restored after each.
 */
class GRAPHCODE_API FGraphCodeSimulatedBlueprint
{
public:
	/** Null, with the reason, when the Blueprint is not a compiled Actor Blueprint or its actor cannot be spawned. */
	static TSharedPtr<FGraphCodeSimulatedBlueprint> Open(UBlueprint& Blueprint, FString& OutError);
	~FGraphCodeSimulatedBlueprint();

	UWorld* GetWorld() const;
	/** The one actor of the Blueprint's class, spawned at its authored place. */
	AActor* GetActor() const;
	/** BeginPlay, as simulate:true begins its world. */
	void Begin();
	/** The clip whose wind() blows from now on, for a clip played after BeginPlay. */
	void UseWindOf(const UAnimSequenceBase* Clip);
	/** One tick of `Seconds`, the wind source blowing the clip's air at `WindAtClipSeconds` first. Nothing is drawn. */
	void Step(double Seconds, double WindAtClipSeconds);
	/** One draw from `Camera` into a Size x Size target the stage keeps, nothing ticked; `OutPixels` read back when given. */
	bool Draw(const FGraphCodeRenderCamera& Camera, int32 Size, bool bCameraCut, TArray<FColor>* OutPixels = nullptr);

private:
	FGraphCodeSimulatedBlueprint();
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};

/**
 * One named point a subject marks on the picture (`highlights:"landmarks"`): a dot (a ring when `bRing`) in its group's
 * colour with a number beside it, and a line in the legend under the group's heading. USER-ASKED 2026-10-19: names written
 * on the face were *"hard to see, we need something like a colour marker and legend"*.
 */
struct GRAPHCODE_API FGraphCodeRenderMark
{
	FVector World = FVector::ZeroVector;
	FString Name;
	FString Group;
	FColor Colour = FColor::White;
	bool bRing = false;
	bool bHidden = false;
};

/**
 * A preview subject supplied by an optional GraphCode product.
 *
 * Core deliberately does not know HairStrands, MetaHuman, or any other optional engine framework.
 * An extension owns the preview scene and its framework components; Core owns the camera, render
 * target, output contract, and lifetime of the shared interface. This keeps an optional renderer
 * out of GraphCode's link dependencies without making `gc_render` a second implementation in each
 * product.
 */
class GRAPHCODE_API IGraphCodeRenderSubject
{
public:
	virtual ~IGraphCodeRenderSubject() = default;

	virtual FSceneInterface* GetScene() const = 0;
	virtual UWorld* GetWorld() const = 0;
	virtual FBox GetBounds() const = 0;
	virtual FString GetKind() const = 0;
	virtual FString GetDescription() const = 0;
	virtual FTransform GetSubjectTransform() const { return FTransform::Identity; }

	/** Thin or temporally filtered geometry can ask Core to retain view history for one view. */
	virtual int32 GetWarmUpPasses() const { return 0; }
	/** Advance the isolated preview world before one warm-up draw. Never called for a level. */
	virtual void AdvancePreview(double DeltaSeconds) {}
	/**
	 * A subject whose framework integrates over time (hair physics) answers simulate:true itself: Core
	 * then advances it only through AdvancePreview, one draw per step, and never seeks its Niagara.
	 */
	virtual bool CanSimulate() const { return false; }
	/**
	 * Start the subject's own simulation from rest, once per simulated view after its warm-up. False when
	 * nothing in it simulates, so the view is its rest shape. Either way it adds, as `token: message`
	 * lines, what the view should say about that simulation.
	 */
	virtual bool BeginSimulation(TArray<FString>& OutWarnings) { return false; }
	/**
	 * `animation:` — play `Clip` on the subject's own skeletal mesh: held at its first frame, posed by a
	 * stated moment, and run from zero by BeginSimulation. False, with the reason, when the subject has
	 * no skeletal mesh that clip's skeleton drives.
	 */
	virtual bool PlayAnimation(UAnimSequenceBase* Clip, FString& OutError)
	{
		OutError = TEXT("this subject has no skeletal mesh to play a clip on");
		return false;
	}
	/**
	 * Pose the subject at a stated moment itself: true, and Core poses none of its components. For a
	 * subject whose parts take their pose from one another — a MetaHuman's head and clothing from its
	 * body — which a preview world, never ticked, would otherwise leave at rest.
	 */
	virtual bool PoseAt(double Seconds, TArray<FString>& OutWarnings) { return false; }
	/**
	 * The air around a simulating subject, in centimetres a second in its own space: a Move clip's `wind()`,
	 * set before every simulated step. For a framework that reads no scene wind (Unreal's hair). False when
	 * nothing in it reads air; it adds, as `token: message` lines, what the view should say about it.
	 */
	virtual bool SetAir(const FVector& Velocity, TArray<FString>& OutWarnings) { return false; }
	/** Hair and similarly thin subjects should not inherit an accidental depth-of-field pass. */
	virtual bool ShouldDisableDepthOfField() const { return false; }
	/** Optional neutral-preview grade. An explicit camera exposure always overrides this. */
	virtual TOptional<double> GetDefaultExposureCompensation() const { return {}; }
	/**
	 * Optional clear colour behind an isolated preview subject. Most assets retain gc_render's
	 * historical black plate; subjects whose authored appearance can itself be black may request a
	 * neutral contrast plate without adding geometry to their bounds or changing their materials.
	 */
	virtual TOptional<FLinearColor> GetBackgroundColor() const { return {}; }
	/** Framework-specific readiness, sampled at the same point as Core material readiness. */
	virtual bool IsReadyToRender(FString& OutReport) const
	{
		OutReport = TEXT("ready");
		return true;
	}
	/** A highlight type only this subject can draw (`landmarks` on a MetaHuman). The level's own types are Core's. */
	virtual bool DrawsHighlight(const FString& CanonicalType) const { return false; }
	/**
	 * This view's named points (`landmarks` on a MetaHuman), seen from `Camera`. Core numbers the ones in view, draws them
	 * flat on the finished picture and lists them in a legend panel at its right; `bHidden` is the subject's word that its
	 * own surface stands between the point and the camera. Each entry of `OutHighlighted` is a note for the result.
	 */
	virtual bool MarkSubjectPoints(const TArray<FString>& CanonicalTypes, const FGraphCodeRenderCamera& Camera,
		TArray<FGraphCodeRenderMark>& OutMarks, TArray<FString>& OutHighlighted, TArray<FString>& OutErrors) { return true; }
};

using FGraphCodeRenderSubjectSupports = TDelegate<bool(UObject* /*Asset*/)>;
using FGraphCodeRenderSubjectFactory = TDelegate<TSharedPtr<IGraphCodeRenderSubject>(
	UObject* /*Asset*/, const FString& /*AssetPath*/, TArray<FString>& /*OutErrors*/)>;

/** Refresh framework buffers at zero time after a Blueprint's bones finalize; returns warm-up draws. */
using FGraphCodeRenderPreviewPose = TDelegate<int32(UWorld& /*ConstructionOnlyPreview*/)>;

/**
 * The triangles a subject draws, at rest and in its own space — what `mode:"ask"` hits, so a pixel of
 * the picture becomes a point on the asset, as it does for a static or skeletal mesh's LOD 0.
 */
using FGraphCodeRenderSubjectGeometry = TDelegate<bool(UObject* /*Asset*/, TArray<FVector>& /*OutVertices*/,
	TArray<uint32>& /*OutIndices*/, FString& /*OutReason*/)>;

struct GRAPHCODE_API FGraphCodeRenderSubjectAdapter
{
	FName Name;
	FGraphCodeRenderSubjectSupports Supports;
	FGraphCodeRenderSubjectFactory Create;
	/** Optional component support for construction-only Blueprint previews, independent of asset kind. */
	FGraphCodeRenderPreviewPose RefreshPreviewPose;

	/** Optional. Unbound, a picture of the subject cannot be asked for points. */
	FGraphCodeRenderSubjectGeometry Geometry;
};

/** Registration seam used by optional products to extend the public scene renderer. */
class GRAPHCODE_API FGraphCodeRenderSubjectAdapters
{
public:
	static bool Register(FGraphCodeRenderSubjectAdapter Adapter, FString& OutError);
	static void Unregister(FName Name);
	/** Only isolated, unstarted EditorPreview worlds are eligible. Never advances world time. */
	static int32 RefreshPreviewPose(UWorld& World);

	/** Null with bOutSupported=false means no adapter claimed the asset; true means creation failed. */
	static TSharedPtr<IGraphCodeRenderSubject> Create(UObject* Asset, const FString& AssetPath,
		bool& bOutSupported, TArray<FString>& OutErrors);

	/** Whether the adapter named `Kind` — a subject's `GetKind()` — answers `Geometry`. */
	static bool HasGeometry(FName Kind);

	/** The claiming adapter's `Geometry`. False with `OutReason` where none claims the asset or answers. */
	static bool ReadGeometry(UObject* Asset, TArray<FVector>& OutVertices, TArray<uint32>& OutIndices,
		FString& OutReason);
};

/**
 * ONE BONE OF THE POSE VIEW (`Docs/GC-MOVE-SEE-AND-POINT-PLAN.md` §3.7), as the product that knows its rig reads it. Every direction
 * is a unit vector in the posed component's space and every origin a point in it: core draws, the reader says.
 */
struct GRAPHCODE_API FGraphCodeRenderBoneReading
{
	FName Bone;
	FVector Origin = FVector::ZeroVector;
	/** Where the bone points, and how far to draw it (centimetres). */
	FVector Pointing = FVector::XAxisVector;
	double Length = 30.0;
	/** Its three limit axes and their names in the rig's own words (fold, side, twist), or "local X/Y/Z" where no rig speaks. */
	FVector Axes[3] = { FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector };
	FString Words[3];
	/** The legal pointing region's edge at its present twist, as SEGMENTS (each two consecutive directions one piece); empty when no
	 * range was read. Pairs rather than a loop because the edge is found by sampling, and a region need not be one loop. */
	TArray<FVector> RangeOutline;
	/** Its twist span as an arc of directions square to its length, and where it turns now. */
	TArray<FVector> TwistArc;
	FVector TwistNow = FVector::ZeroVector;
	/** The legend line: the same words and numbers as a build's `[readings]`. */
	FString Legend;
	/** On a chain's FIRST joint: how far the chain reaches laid out (its reach bubble's radius), and where its end is now. Zero elsewhere. */
	double ReachRadius = 0.0;
	FVector ReachEnd = FVector::ZeroVector;
	/**
	 * On a bone that ends a limb or rides its end: the DIRECTION SPHERE (plan §3.12), each direction it could point with the hand kept where
	 * it is, and how far past the joints' ranges that needs (degrees; 0 in range, negative where no place could be made), drawn at Radius.
	 * Directions is the reach sphere's grid (§3.18 v6): rows of up from -85 every 5 degrees (35), 72 columns round every 5.
	 */
	TArray<FVector> Directions;
	TArray<float> DirectionExcess;
	/** Per direction, why the arm (and the held part) meets the body when the joints hold it in range; empty where clear or not tried
	 * (plan §3.20 "the direction sphere and the ring try the body": bright is what the build accepts). Dark where not empty. */
	TArray<FString> DirectionBlocked;
	double DirectionRadius = 0.0;
	/**
	 * The sphere BRIGHT AND DARK, as segments (pairs of directions) drawn at DirectionRadius: its parallels and meridians every 10 degrees in
	 * 5-degree pieces, bright where both ends are held with every joint in range, dark where neither, split at the middle between (user
	 * 2026-10-16: "I thought we have ruled to use bright spots and dark spots"; §3.14's green and orange outlines are gone).
	 */
	TArray<FVector> DirectionBright;
	TArray<FVector> DirectionDark;
	/**
	 * THE REACH ENVELOPE (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.16 row 3) on a limb's first joint: per direction of a grid (rows of up from
	 * -84 every 12 degrees, ReachShellColumns round every 12), the farthest the limb places its end with every joint in range, as a point from
	 * the joint in the component's space (zero where it reaches no way that direction). Drawn as a net: inside it the end can go.
	 */
	TArray<FVector> ReachShell;
	int32 ReachShellColumns = 0;
	/** The direction sphere is an AIM sphere (a chain's facing, the gaze), read in degrees short, not a limb end's in degrees past a stop. */
	bool bAimSphere = false;
	/**
	 * THE COMBINED BUBBLE (plan §3.17 item 1) on a limb's first joint, given `aim:`: hand places inside the reach net (component space), and at
	 * each how far past a stop the held bone pointing the wanted aim needs (0 in range, negative no place). Drawn as dots, green and orange.
	 */
	TArray<FVector> AimPlaces;
	TArray<float> AimPlaceExcess;
	/** At each place, that excess split over the limb's three joints (AimJoints: upper, middle, end), as the build's note splits it (§3.17 1b). */
	TArray<FVector3f> AimPlaceShares;
	TArray<FName> AimJoints;
	/** At each place in range, why the arm there meets the body (the solve's limb clearance; §3.17 1a), empty where clear. Drawn red. */
	TArray<FString> AimPlaceBlocked;
	/** With the request's BuildLine (BUBBLE-GRIP1): for TryPlaces[0], the key dry-run with its reach pointed there: "accepted", else why the build
	 * refuses it or does not put the hand there as asked. Empty: not built. */
	TArray<FString> AimPlaceBuilt;
	/** The line the key was built with there (`frame(at=..., toward=..., up=...)`; a ring point's is the key's own at=). Empty: not built. */
	FString AimPlaceLine;
	/** On a limb end's direction sphere: the limb's first joint, the reach a ring point's ask is tried with (BUBBLE-GRIP1). None elsewhere. */
	FName LimbFirst;
	/** The hips' move the sphere stood on (the key's balance, W10 G3), and from the build check where that balance left them with this limb
	 * at TryPlaces[0] (bBuiltSettle; W10 G15), both in the component's space. */
	FVector Settled = FVector::ZeroVector;
	FVector BuiltSettle = FVector::ZeroVector;
	bool bBuiltSettle = false;
	/**
	 * THE COMBINED BUBBLE, ONE SPHERE THE AUTHOR SIZES (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.18 v6; user 2026-10-16: "just use one sphere,
	 * declared by the author on the size, brighter parts of the sphere means able to reach, darker parts means cant"): segment pairs from the
	 * joint (as ReachShell is), its parallels and meridians every 10 degrees, AimSize centimetres out: bright pieces where a hand there holds
	 * the aim with every joint in range and the arm clear, dark pieces where it does not. Drawn alone: no globe or direction sphere beside it.
	 * The places (its points, every 5 degrees) stay for the ask, whose bubble is this sphere.
	 */
	TArray<FVector> AimShellHolds;
	TArray<FVector> AimShellDark;
	double AimSize = 0.0;
	/**
	 * THE WHOLE CHAIN AT EACH POINT (plan §3.19 step 3; user 2026-10-16: "When asking about angle it should not only give the wrist angle, but
	 * the whole chain that makes it possible"): the chain's joints (the collarbone first when it has one), each axis's word as the rig names
	 * it (three per joint), and per point, per joint, the three angles as [readings] reads them (zero where the point does not hold).
	 */
	TArray<FName> AimPoseJoints;
	TArray<FString> AimPoseWords;
	TArray<FVector3f> AimPose;
	/** The wanted aim on that sphere (plan §3.13), and how far past a stop holding it from here needs; zero vector when none was asked. */
	FVector WantedAim = FVector::ZeroVector;
	float WantedAimExcess = -1.f;
	/**
	 * THE ROLL RING (plan §3.20 step 5): with the wanted aim on a limb end's direction sphere, the ups square to it (72, 5 degrees apart, in
	 * the body's words), each held as the frame's +Z (the bone's +Z, its edge) with the hand kept where it is; per up the least degrees past
	 * the joints' ranges (0 in range, negative no place) and that placement's excess per joint of RollJoints (upper, middle, end).
	 */
	TArray<FVector> RollUps;
	TArray<float> RollExcess;
	TArray<FVector3f> RollShares;
	TArray<FName> RollJoints;
	/** Per up, why the arm meets the body with the edge that way though the joints hold it; empty where clear or not tried (as DirectionBlocked). */
	TArray<FString> RollBlocked;
};

/**
 * Reads a posed skeleton's bones for the pose view. A reader that does not know the asset leaves bOutClaimed false and the next
 * one is asked; a reader that claims it and cannot answer returns false with the reason.
 */
using FGraphCodeRenderBoneReader = TDelegate<bool(UObject* /*Asset*/, const USkeletalMesh* /*Mesh*/, const TArray<FName>& /*BoneNames*/,
	const TArray<int32>& /*ParentIndices*/, const TArray<FTransform>& /*RestLocal*/, const TArray<FTransform>& /*ComponentSpace*/,
	const FGraphCodeRenderView::FBones& /*Request*/,
	TArray<FGraphCodeRenderBoneReading>& /*Out*/, bool& /*bOutClaimed*/, FString& /*OutReason*/)>;

/** The pose view's registration seam, the twin of the subject adapters: Move registers the reader that speaks its rigs' words. */
class GRAPHCODE_API FGraphCodeRenderBoneReaders
{
public:
	static bool Register(FName Name, FGraphCodeRenderBoneReader Reader, FString& OutError);
	static void Unregister(FName Name);
	/** The first registered reader that claims the asset. False with bOutClaimed=false when none does. */
	static bool Read(UObject* Asset, const USkeletalMesh* Mesh, const TArray<FName>& BoneNames, const TArray<int32>& ParentIndices,
		const TArray<FTransform>& RestLocal, const TArray<FTransform>& ComponentSpace, const FGraphCodeRenderView::FBones& Request,
		TArray<FGraphCodeRenderBoneReading>& Out, bool& bOutClaimed, FString& OutReason);
	/**
	 * POINT AND ASK (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.15): how the registered reader's grammar writes a direction in the character's
	 * space (Move: `(down, forward=35)`), so a direction picked on a bubble answers as words a file takes. Empty when none is registered.
	 */
	using FSpeller = TDelegate<FString(const FVector& /*Direction*/)>;
	static void RegisterSpeller(FSpeller Speller);
	static FString Spell(const FVector& Direction);
	/** The same reader's spelling of an OFFSET in the character's space as its grammar writes a place's distances (Move: `(forward=38, down=40)`);
	 * a point inside a reach bubble answers with it. Empty when none is registered. */
	static void RegisterPlaceSpeller(FSpeller Speller);
	static FString SpellPlace(const FVector& Offset);
	/**
	 * THE SPELLER BACKWARDS (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.23): a direction written in the reader's grammar (`(down, forward=37)`) as a
	 * unit vector in the character's space, so an ask can name a bubble's direction as the answer before it wrote it. False with the reason
	 * where the words are not a direction (no reader registered: false).
	 */
	using FDirectionReader = TDelegate<bool(const FString& /*Words*/, FVector& /*Out*/, FString& /*OutReason*/)>;
	static void RegisterDirectionReader(FDirectionReader Reader);
	static bool ReadDirection(const FString& Words, FVector& Out, FString& OutReason);
	/**
	 * TRAILS' FLOOR MARK (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.16 row 5): the body's centre of mass at each pose (component space), by the mass
	 * model the registered reader's builds balance with. False with the reason when it has none for this asset (no reader registered: false).
	 */
	using FCentres = TDelegate<bool(UObject* /*Asset*/, const USkeletalMesh* /*Mesh*/, const TArray<FName>& /*BoneNames*/,
		const TArray<TArray<FTransform>>& /*ComponentSpacePoses*/, TArray<FVector>& /*Out*/, FString& /*OutReason*/)>;
	static void RegisterCentres(FCentres Centres);
	static bool Centres(UObject* Asset, const USkeletalMesh* Mesh, const TArray<FName>& BoneNames, const TArray<TArray<FTransform>>& Poses,
		TArray<FVector>& Out, FString& OutReason);
	/**
	 * TRAILS' GRAPH (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.22): each asked bone's three joint angles at each pose (component space), in the
	 * registered reader's words and numbers (Move: the rig's fold, side and twist as `[readings]` reads them). Per bone three words in
	 * OutWords and a series in OutAngles (degrees, one per pose; empty where its rig has no limit for it). False with the reason when it
	 * reads no rig for this asset (no reader registered, or not its asset: false with no reason).
	 */
	using FJointAngles = TDelegate<bool(UObject* /*Asset*/, const TArray<FName>& /*BoneNames*/, const TArray<int32>& /*ParentIndices*/,
		const TArray<FTransform>& /*RestLocal*/, const TArray<int32>& /*Bones*/, const TArray<TArray<FTransform>>& /*ComponentSpacePoses*/,
		const FString& /*Rig*/, TArray<FString>& /*OutWords*/, TArray<TArray<FVector>>& /*OutAngles*/, FString& /*OutReason*/)>;
	static void RegisterJointAngles(FJointAngles JointAngles);
	static bool JointAngles(UObject* Asset, const TArray<FName>& BoneNames, const TArray<int32>& ParentIndices, const TArray<FTransform>& RestLocal,
		const TArray<int32>& Bones, const TArray<TArray<FTransform>>& Poses, const FString& Rig, TArray<FString>& OutWords,
		TArray<TArray<FVector>>& OutAngles, FString& OutReason);
};

namespace GraphCodeRender
{
	/** One written picture, and the facts about it a caller can act on. */
	struct GRAPHCODE_API FImage
	{
		FString ViewName;
		/** Absolute path of the PNG. */
		FString Path;
		int32 Bytes = 0;

		/** Populated only for the single-view case, per §3. */
		FString Base64;

		/** SHA-1 of the PNG bytes. Provenance, NOT a portable equality predicate — §7. */
		FString Digest;

		/**
		 * The subject's own shaders had not finished compiling when this was read back.
		 *
		 * Sampled AFTER the scene was built, because phase 0 measured that building it queues
		 * shader work of its own and a pre-flight check therefore says nothing. Reported rather
		 * than treated as a failure: the same probe run produced a correct picture while this was
		 * set, so it means "do not trust these pixels yet", not "these pixels are wrong".
		 */
		bool bIncomplete = false;

		/** §11: which frame of its view, and the moment it is of. Both zero for a still. */
		int32 FrameIndex = 0;
		double Seconds = 0.0;
		/** A6: where this frame's camera stood, set only when it followed its anchor (`CameraFollowed`). */
		TOptional<FVector> Camera;

		/**
		 * §11's derived images, which were ASSEMBLED rather than rendered.
		 *
		 * They are in this list beside the frames rather than in lists of their own because every
		 * caller of a render result already knows how to read an `FImage` — a path, a digest, a byte
		 * count and optional inline content are the same four facts whether the pixels came off a
		 * render target or out of a memcpy. What a caller needs on top of that is to be able to tell
		 * them apart, which is what these say.
		 *
		 * **THE ORDER IS LOAD BEARING:** every view's frames come first, in frame order, then its
		 * sheet, then its video, then its graph. So `Images[0]` is still view 0's first picture for every request
		 * that existed before §11, which is what keeps the probe suite honest.
		 */
		bool bSheet = false;
		bool bVideo = false;
		/** Trails' graph (`{type:"trails", graph:...}`, plan §3.22): velocity and acceleration over the clip, drawn, not rendered. */
		bool bGraph = false;

		/** How many frames were tiled into a sheet, and the grid. Zero for anything else. */
		int32 Columns = 0;
		int32 Rows = 0;
	};

	struct GRAPHCODE_API FResult
	{
		FString AssetPath;
		/**
		 * What the subject turned out to be: `material`, `material_instance`, `material_function`,
		 * `static_mesh`, `groom`, `blueprint` or `level`.
		 */
		FString Kind;
		FString CaptureId;
		FString ManifestPath;

		/** What the preview scene actually built — phase 0 found the engine picks this per asset. */
		FString PreviewMesh;
		FBox SubjectBox = FBox(ForceInit);

		/** Cameras and poses, one per requested view, in request order. */
		TArray<FGraphCodeRenderViewResult> Views;
		/** Images, parallel to `Views`. */
		TArray<FImage> Images;

		FString ReadinessReport;
		TArray<FString> Warnings;
		double ElapsedSeconds = 0.0;
		/** How long the capture waited for the subject's shaders before drawing; 0 when they were ready. */
		double ShaderWaitSeconds = 0.0;
		/** The meter reading every draw after the first kept one was exposed at; 0 when nothing metered. */
		double PinnedExposure = 0.0;
		/** Local video memory over the RHI's budget after the last draw; 0 within budget (field report E2/H1). */
		uint64 GpuOverBudgetBytes = 0;
	};

	// ---- `mode:"legend"` — the identification image, `GC-VISION-DESIGN.md` §4.2 ------------------
	//
	// IT TAKES A CAPTURE AND NO CAMERA, which is the whole of §4.2's ruling: a mask is an assertion
	// about a picture somebody is looking at, and one that re-solved a smart camera would be an
	// assertion about a picture nobody took. Rendering through the stored matrices makes the identity
	// true by construction instead of by a gate that has to catch the drift.
	//
	// THE MECHANISM IS HIT PROXIES (§4.2b, measured): the buffer a viewport click reads is already
	// per-pixel object identity. What ships here is the half that was owed — the value decoded back to
	// its actor, through `GetHitProxyById`, and then REPAINTED in the legend's own colours. The repaint
	// is what makes the promise exact: the engine's ids are the engine's, and no arithmetic on them
	// could be made to equal `FGcFrameLegendEntry::Color`.

	/** One name in the mask, and what it is worth looking for. */
	struct GRAPHCODE_API FLegendMaskEntry
	{
		/**
		 * `actor`, `region`, `edge`, `uv_island`, `overlap`, `unnamed` or `background`.
		 *
		 * THE LAST TWO ARE NOT NAMES AND ARE ROWS ANYWAY, because a caller sampling a pixel has to be
		 * able to tell three different things apart: this actor, nothing at all, and **something is
		 * here and this legend cannot name it**. The third is not hypothetical — the legend's actor
		 * set is the one a level's `.gc` names, and a transient actor is in the picture and outside
		 * it. Painting those as background would report solid geometry as empty space, which is the
		 * confident wrong answer this whole surface exists to remove.
		 */
		FString Kind;
		FString Name;
		/** `#RRGGBB`, and this exact value is in the image wherever this thing is. */
		FString Color;

		/**
		 * How many pixels it actually covers.
		 *
		 * ZERO IS A REAL ANSWER and it is the useful one: an actor listed in the frame legend that is
		 * wholly behind a wall covers nothing, and a caller who points at its aim pixel gets the wall.
		 * The text legend measures that with a nine-sample trace; this counts it.
		 */
		int64 Pixels = 0;
		int32 Island = INDEX_NONE;
		int32 Triangles = 0;
		FVector2D UvSeed = FVector2D::ZeroVector;
		FString Selector;
	};

	struct GRAPHCODE_API FLegendMaskRequest
	{
		FString By;
		int32 UvChannel = 0;
		/** The capture to paint. Required — a mask with no picture is a picture of its own. */
		FString CaptureId;
		FString ViewName = TEXT("main");
		/** Return the PNG bytes inline as well as writing them. */
		bool bInlineImage = false;
	};

	struct GRAPHCODE_API FLegendMaskResult
	{
		FString SourceCapture;
		FString AtlasPath;
		FString LegendPath;
		int32 UvChannel = INDEX_NONE;
		FVector2D UvMin = FVector2D::ZeroVector;
		FVector2D UvMax = FVector2D(1, 1);
		FString CaptureId;
		FString ViewName;
		FString Path;
		FString Base64;
		FString Digest;
		int32 Bytes = 0;
		int32 Width = 0;
		int32 Height = 0;

		/** The colour that means *no object here*, stated rather than left to be inferred from black. */
		FString Background;
		/** And the colour that means *something is here that this legend cannot name*. */
		FString Unnamed;

		/** Actors first, then regions and their edges, then the two colours that are not names. */
		TArray<FLegendMaskEntry> Entries;
		TArray<FString> Warnings;
	};

	/**
	 * Paint a retained capture's identification image and legend.
	 *
	 * Level identification writes one PNG beside the original manifest. By=uv_island creates a new
	 * retained source-LOD capture with main/uv views and a swatch legend. Neither changes packages,
	 * actors, materials, editor selection or the source capture.
	 */
	GRAPHCODE_API bool RenderLegendMask(const FLegendMaskRequest& Request,
		FLegendMaskResult& OutResult, TArray<FString>& OutErrors);

	/**
	 * Whether this process can render a scene at all, and why not when it cannot.
	 *
	 * Two gates, Play first. While the editor is in Play or a Play request is queued it refuses,
	 * before any preview world or RHI work (GAP-1113 V1, P269: a render during Play asserted the
	 * renderer); `Render`, `RenderAssetThumbnail` and `RenderLegendMask` all open on it. Then
	 * `FApp::CanEverRender()`. `ui_render` additionally needs an initialised Slate application; a
	 * scene capture does not, which is why `gc_render` runs in a headless `UnrealEditor-Cmd` and its
	 * GPU gates can be automated.
	 */
	GRAPHCODE_API bool CanRender(FString& OutReason);

	/**
	 * Render one asset at every requested view. Writes PNGs and a manifest; changes no package.
	 *
	 * Validation, the output plan and the kind gate all run BEFORE the scene is built, so a request
	 * that was never going to work costs no GPU.
	 */
	GRAPHCODE_API bool Render(const FGraphCodeRenderRequest& Request, FResult& OutResult,
		TArray<FString>& OutErrors);

	/**
	 * A POINT RIDING A BONE for a trails highlight (Docs/GC-MOVE-CUT-PLAN.md item 1(a), 15 Nov 2026): its path is drawn beside the bones'
	 * trails of view Request.Views[View]: the bone's component-space transform applied to At, centimetres in the bone's own axes (the
	 * blade's cutting point is blade_r at (85, 0, 0)). CARRIED BESIDE THE REQUEST, never in it: FGraphCodeRenderView is shared by every
	 * separately published GraphCode module and keeps its layout (static_asserts in GraphCodeRenderService.cpp).
	 */
	struct FTrailPoint
	{
		int32 View = 0;
		FString Name;
		FString Bone;
		FVector At = FVector::ZeroVector;
	};
	/** Render, with trail points beside the request. Empty TrailPoints is exactly Render(Request, OutResult, OutErrors). */
	GRAPHCODE_API bool Render(const FGraphCodeRenderRequest& Request, FResult& OutResult,
		TArray<FString>& OutErrors, const TArray<FTrailPoint>& TrailPoints);

	// ---- `kind:"thumbnail"` — the engine's own renderer, `GC-VISION-DESIGN.md` §6.5 -------------
	//
	// **A TEXTURE CASE WOULD HAVE BEEN THE WRONG FIX, which is the whole of §6.5's ruling.** The
	// refusal a `UTexture2D` used to land on named five classes, and every class after it would have
	// been one more line — the shape `gc-capability-registry` exists to stop. The engine already
	// keeps the answer per class: `UThumbnailRenderer` is registered against a type and
	// `ThumbnailTools::RenderThumbnail` draws any of them off-screen at a requested size. One route
	// covers everything the Content Browser can draw, and what stays per-class is the REFUSAL — a
	// class with no registered renderer says so by name, which is a list that SHRINKS as the engine
	// grows rather than one we maintain.
	//
	// **IT IS ITS OWN KIND AND NEVER A FALLBACK INSIDE THE SCENE ROUTE.** A thumbnail renderer picks
	// its own view; there is no camera, no anchor, no zoom, no post-process bracket and no view
	// matrix. Falling through to it when `BuildSubject` refuses would leave `camera`, `views`,
	// `azimuth` and every other framing parameter accepted and silently ignored, which is the
	// `impact` defect this project has paid for three times. `mode:"screen_to_world"` does not reach
	// it either, for the same reason: there is no matrix to invert.

	/** What one thumbnail turned out to be, beside the picture of it. */
	struct GRAPHCODE_API FThumbnailResult
	{
		FString AssetPath;
		/** The subject's own class name — `Texture2D`, `NiagaraSystem`, `Font`. Not a `.gc` kind. */
		FString Kind;
		/** The `UThumbnailRenderer` subclass that drew it, so the picture names its own author. */
		FString Renderer;

		FString CaptureId;
		FString ManifestPath;

		/** What was asked for. A BOX on this route — see `ThumbnailSizeAdjusted`. */
		int32 RequestedWidth = 0;
		int32 RequestedHeight = 0;
		/** What was actually drawn, which is what the PNG measures. */
		int32 Width = 0;
		int32 Height = 0;

		FImage Image;
		FString ReadinessReport;
		TArray<FString> Warnings;
		double ElapsedSeconds = 0.0;
	};

	/**
	 * Draw one asset through whatever thumbnail renderer the engine has registered for its class.
	 *
	 * Reads `AssetPath`, `Output` and the single view's `Width`/`Height` off the request and ignores
	 * nothing else, because the caller that builds it accepts nothing else. Writes one PNG and a
	 * manifest; loads no map, compiles nothing, dirties no package.
	 *
	 * FRESH, NEVER THE CACHED `FObjectThumbnail`. `ThumbnailTools::RenderThumbnail` runs the
	 * registered renderer; `FindCachedThumbnail` and `GetThumbnailForObject` are the ones that would
	 * hand back the picture in the package, which is an answer about the asset as it was last saved.
	 * A stale thumbnail answering a question about the asset as it is NOW is
	 * `a-lossy-comparison-reports-no-change` in picture form.
	 */
	GRAPHCODE_API bool RenderAssetThumbnail(const FGraphCodeRenderRequest& Request,
		FThumbnailResult& OutResult, TArray<FString>& OutErrors);

	/**
	 * An Actor Blueprint's post-construction preview bounds, and whether previewing dirtied it.
	 *
	 * Phase 3 is what turns this into an adapter. It is here now because phase 0 needed it and the
	 * answer — five project Blueprints previewed, none dirtied — is a standing gate worth keeping.
	 */
	GRAPHCODE_API bool MeasureBlueprint(UBlueprint* Blueprint, FBox& OutBounds,
		bool& bOutPackageDirtied, TArray<FString>& OutErrors);

	/**
	 * The box an anchored actor selection is framed by, in the given basis, with the warnings that
	 * explain how it was arrived at.
	 *
	 * THREE RULES IN ORDER, and each is a fallback for the one before: the engine's own framing
	 * whitelist, then every visible non-editor primitive (`bounds_widened`), then the actors
	 * ATTACHED BELOW the anchor (`bounds_from_children`, T-6). Only when all three find nothing does
	 * the 100 cm cube arrive, at the selection's own location rather than at the basis origin.
	 *
	 * **EXPORTED SO IT CAN BE MEASURED WITHOUT A GPU, which is the whole reason it is a function.**
	 * `Render` refuses on `CanRender` before any view resolves, so every gate over this arithmetic
	 * SKIPPED under `-nullrhi` while reporting green — and none of it needs pixels. `MeasureBlueprint`
	 * above is here for the same reason.
	 */
	GRAPHCODE_API void MeasureActors(TArrayView<AActor* const> Actors, const FTransform& ToBasis,
		FBox& OutBox, TArray<FString>& OutWarnings);

	/**
	 * What a LEVEL capture shows unadvanced in this camera's frame, said rather than left to be
	 * mistaken for the game (GAP-1113 V1, P190 and P274).
	 *
	 * A capture does not move the level (`GC-VISION-DESIGN.md` §8), so Niagara components in the
	 * open level are never simulated and placed skeletal meshes driven by an Anim Blueprint stand in
	 * their last evaluated editor pose. This only READS: components, their cached bounds and the
	 * camera's frustum. Nothing is ticked, posed, advanced or marked dirty. Adds one `time_not_seekable`
	 * line per kind found, naming the count and the route that does show it (the asset with
	 * `simulate:true`); `Hidden` (the view's ignore list) is left out. Returns how many were found.
	 * Exported for the same reason `MeasureActors` is: it needs no GPU, and `Render` refuses without one.
	 * `bEffectsSimulated`: the view asked `simulate:true`, so its Niagara components WERE advanced (N14)
	 * and only the posed characters are counted and said.
	 */
	GRAPHCODE_API int32 WarnUnadvancedInView(const UWorld* World, const FGraphCodeRenderCamera& Camera,
		int32 Width, int32 Height, const TSet<FPrimitiveComponentId>* Hidden, TArray<FString>& OutWarnings,
		bool bEffectsSimulated = false);

	/**
	 * N15 (GC-MOVE-NIAGARA-FIELD-1116-PLAN.md): whether a Niagara system draws Niagara Fluids, so its asset render
	 * takes the Niagara editor viewport's scene (lit sky, its lights, metered) instead of the thumbnail world. True when
	 * one of the system's own objects (emitters, renderers, data interfaces) is a `NiagaraVolumeRendererProperties` or a
	 * `NiagaraDataInterfaceGrid3DCollection`; found by class path, so Core links no Niagara. Sprite, ribbon and
	 * mesh-only effects answer false and keep the thumbnail route byte for byte. Exported for its GPU-free spec.
	 */
	GRAPHCODE_API bool IsNiagaraFluidsSystem(const UObject* System);

	/**
	 * A `simulate:true` world run with `physics` off holds any body that asks to simulate in its last
	 * pose (a ragdoll on death freezes). Adds one `time_not_seekable` line, once, naming how many
	 * components in `World` have `bSimulatePhysics` set and that `physics:true` runs them (GAP-1113
	 * V1, P271). Reads only. Returns the count.
	 */
	GRAPHCODE_API int32 WarnPhysicsAskedWhileOff(const UWorld* World, TArray<FString>& OutWarnings);

	/**
	 * R1 (RENDER-READY1, GC-GAP-REPORT-1117-PLAN.md): the slots of the visible mesh components in `World` that draw the
	 * engine's default material, because their own has no complete shader map (the draw's own test,
	 * `FMaterialRenderProxy::GetMaterialWithFallback`). Asked after a kept frame. One line per slot into `OutSlots`:
	 * `component 'C' slot N ('Name'): M has no finished shader map`. Reads only: nothing is submitted or drawn.
	 * Returns the count. Exported for its spec, as `WarnPhysicsAskedWhileOff` is.
	 */
	GRAPHCODE_API int32 FallbackMaterialSlots(const UWorld* World, TArray<FString>& OutSlots);

	/** What is not ready yet, by name and count — never a sleep. */
	GRAPHCODE_API bool IsReadyToRender(UMaterialInterface* Material, FString& OutReport);

	/** A square render target for DrawScene, cleared black. */
	GRAPHCODE_API UTextureRenderTarget2D* MakeDrawTarget(int32 Size);

	/**
	 * One draw of a scene from a camera into `Target` (Size x Size), flushed: game show flags, no motion blur or depth of
	 * field, the caller's view state kept from draw to draw so temporal history (hair coverage) carries over. `OutPixels`, when
	 * given, is read back. The draw every hair readback needs: the engine copies a requested sample only inside a draw of the
	 * component's own scene (GC-GARMENT-HAIR-BAKE.md R3).
	 */
	GRAPHCODE_API bool DrawScene(FSceneInterface* Scene, const FGraphCodeRenderCamera& Camera, int32 Size,
		FSceneViewStateReference& ViewState, bool bCameraCut, UTextureRenderTarget2D& Target, TArray<FColor>* OutPixels = nullptr);

	/** One directional light of the `neutral` rig: the direction it TRAVELS, and its intensity. */
	struct GRAPHCODE_API FRigLight
	{
		/** Unit, and the direction of travel — a surface is lit by `-Direction`. */
		FVector Direction = FVector::ZeroVector;
		float Intensity = 0.0f;
	};

	/**
	 * The `neutral` profile's three directional lights, and they are MUTUALLY ORTHOGONAL on purpose.
	 *
	 * **WHY A TRIAD RATHER THAN A BRIGHTER KEY.** The engine's thumbnail rig is three lights at
	 * 5.0 / 1.0 / 0.2, which is one light with two hints of fill, and a rig with one light has a
	 * BLIND AXIS: a cylinder whose axis lies along that light has every surface normal
	 * perpendicular to it, so the whole visible surface sits at grazing incidence and shades black.
	 * What survives is a rim highlight and the end cap, which reads as a thin shaft with a disc
	 * floating past it. That photographed a correct mesh into a defect and cost GraphCode Shape a
	 * filed row (MS1) that took a rebuild and four probes to disprove. Adding a fourth light in
	 * some other direction only MOVES the blind axis; orthogonality removes it.
	 *
	 * **THE BOUND, and it is why the directions are exactly these.** Three orthonormal directions
	 * satisfy `sum_i (A . L_i)^2 == 1` for every unit axis `A`, so the smallest of the three is at
	 * most `1/sqrt(3)`, and THAT light's component perpendicular to `A` is at least
	 * `sqrt(1 - 1/3) == 0.8165`. No axis can hide from all three. `GraphCodeRenderLightTests.cpp`
	 * asserts it over a sampled sphere rather than trusting the arithmetic here.
	 *
	 * The intensities are close together for the same reason — a guarantee carried by a light
	 * nobody can see is not a guarantee — and every direction travels DOWNWARD, so the rig still
	 * lights a subject from above and an underside still reads as an underside.
	 *
	 * **EXPORTED SO IT CAN BE MEASURED WITHOUT A GPU**, exactly like `MeasureActors` above: the rig
	 * is arithmetic, and a gate over it must not skip under `-nullrhi`.
	 */
	GRAPHCODE_API TArray<FRigLight> NeutralLightRig();

	/**
	 * A6 (field report 2026-10-28): "renders put the reference side by side automatically" — the Roadster's 30 rounds of
	 * hand-made sheets. Writes `OutPath`: the render at `RenderPath`, a white gap, and the picture at `PicturePath` scaled to
	 * the render's height. Pictures only, so it is measured without a GPU like `NeutralLightRig`.
	 */
	GRAPHCODE_API bool BesideReference(const FString& RenderPath, const FString& PicturePath, const FString& OutPath,
		FString& OutError);
}
