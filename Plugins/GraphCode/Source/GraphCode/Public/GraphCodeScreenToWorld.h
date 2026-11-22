// Retained `gc_render` queries — image pixels back to observed geometry.
// `GC-SPATIAL-AUTHORING-DESIGN.md` §4b is the level contract; GC Rig extends it to mesh-local
// entry/middle/exit answers for static and skeletal mesh previews.
//
// The premise is that a vision model can pick out a wall, a tabletop or a patch of floor in a
// picture far more reliably than it can infer that surface's transform. This closes the loop: it
// deprojects through the EXACT camera that produced the image and queries the photographed subject.
//
// It observes and never writes. No actor, selection, viewport, dirty flag or file changes here.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeMeshSource.h"
#include "GraphCodeRender.h"
#include "GraphCodeSceneMeshQuery.h"

class UWorld;
class UObject;
struct FGcMeshUVQueryContext;

/**
 * One retained capture's one view, read back off its manifest: the exact camera, the raster it was
 * taken at, and what it left out.
 *
 * IT IS SHARED WITH `mode:"legend"` ON PURPOSE. A legend mask is an assertion about the picture a
 * caller is looking at, and `GC-VISION-DESIGN.md` §4.2 rules that it must therefore come off the
 * CAPTURE rather than re-solve a camera request. Two readers of one manifest would agree until one
 * of them learned about a field — a cut plane, an ignore list — that the other did not, and the
 * disagreement would be silent: a mask painted through a slightly different camera looks exactly
 * like a mask.
 */
struct GRAPHCODE_API FGcCaptureView
{
	FString CaptureId;
	FString ViewName;
	FString ManifestPath;
	FString ImagePath;

	/** What was photographed. A level answers in world space; a mesh preview in mesh-local space. */
	FString Scene;
	FString AssetPath;
	FString Kind;
	FString AssetToken;
	int32 UvChannel = INDEX_NONE;
	FString UvSignature;
	bool bUvLayout = false;
	FVector2D UvMin = FVector2D::ZeroVector;
	FVector2D UvMax = FVector2D(1, 1);

	/** The thumbnail scene recentres preview components. Recorded so that shift can be undone. */
	FTransform SubjectTransform = FTransform::Identity;
	bool bHasSubjectTransform = false;

	/** The level package as RECORDED, which is not necessarily the one open now. */
	FString LevelPackage;
	FString LevelToken;
	FString SceneMeshToken;

	/** Reproduced exactly, cut plane included — never re-derived from the original request. */
	FGraphCodeRenderCamera Camera;
	int32 Width = 0;
	int32 Height = 0;

	/** R8's list: the actors this picture was taken WITHOUT, by internal name. */
	TArray<FString> Ignored;

	/** Point and ask (plan §3.15): the direction spheres the picture drew (FGraphCodeRenderViewResult::FBubble, as the manifest keeps them). */
	TArray<FGraphCodeRenderViewResult::FBubble> Bubbles;
};

/**
 * The two collision modes are mutually exclusive, and the reason is a real confusion this refuses
 * to inherit: `Visibility` and `Camera` are **trace channels**, while `WorldStatic` is an **object
 * type**. Putting `WorldStatic` in `TraceChannel` is the mistake the design calls out by name, so
 * the request cannot express it — a caller who wants only solid architecture fills `ObjectTypes`.
 */
struct GRAPHCODE_API FGcScreenToWorldRequest
{
	FGcSceneMeshSelector Mesh;
	TSharedPtr<FGcSceneMeshQueryContext> MeshContext;
	TSharedPtr<FGcMeshUVQueryContext> UvContext;
	FGcSceneMeshSelector RelativeTo;
	FString Snap = TEXT("none");
	/** The `gc_render` capture this pixel came from. Required: a pixel with no picture is a guess. */
	FString CaptureId;
	FString ViewName = TEXT("main");

	/** Normalized to `[0,1]`, `(0,0)` at the image's TOP-LEFT — image convention, not UV. */
	double X = 0.5;
	double Y = 0.5;

	/** V7/gcw_ask: integer image pixel. When set, X/Y are derived from its centre. */
	bool bUsePixel = false;
	int32 PixelX = 0;
	int32 PixelY = 0;

	/**
	 * Mesh preview only: 0 is the entry surface, 0.5 the middle of the entered part, and 1 the
	 * corresponding exit surface. A level has no durable notion of "the same part" and accepts 0.
	 */
	double Depth = 0.0;

	/**
	 * `Depth` READ AS CENTIMETRES IN FROM THE ENTRY SURFACE instead of as a fraction of the chord.
	 *
	 * `camera.offset`'s pair, for `camera.offset`'s reason: the relative reading is the one that
	 * behaves the same on a doorknob and on a building, and the absolute one is the only one that
	 * can say *1 cm under the skin* - a tenth of the way through a 4 cm limb and a tenth of the
	 * way through a 90 cm torso are not the same statement about depth. Resolved against the
	 * MEASURED thickness, so unlike the fraction it can only be range-checked after the ray hits.
	 */
	bool bDepthInCentimetres = false;

	/** Exactly one of these two. Empty channel + empty types defaults to `Visibility`. */
	FString TraceChannel;
	TArray<FString> ObjectTypes;

	/** Bounded on purpose; an unbounded trace through a streamed world is not a query. */
	double MaxDistance = 100000.0;

	/** Actor labels/names the trace steps over — the prop between the camera and the wall. */
	TArray<FString> IgnoreActors;

	/** Point and ask on a bubble (plan §3.15-3.16): only the bubbles of this bone are met; none named, the nearest bubble the ray meets. */
	FName Bubble;

	/**
	 * ASK AGAIN BY DIRECTION AND DISTANCE (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.23; user 2026-10-22: "so next query can query about the
	 * relative angle + distance without needing to render again"): instead of a pixel, the direction from Bubble's centre in the body's words
	 * (`(down, forward=37)`, as a point's answer spells it) and how far out (cm); a Distance under 0 asks its max extension. Needs Bubble, a
	 * reach bubble the capture kept its pose for.
	 */
	FString Toward;
	double Distance = -1.0;
};

/** Region-relative facts for one pixel hit inside an authored `.gcw` region. */
struct GRAPHCODE_API FGcScreenRegionResult
{
	FString Region;
	double Area = 0.0;
	FVector Offset = FVector::ZeroVector;
	FVector Percentage = FVector::ZeroVector;
	/** A wall has one name since §21.1b, so the alias that sat beside this is gone. */
	FString NearestEdge;

	double EdgeParameter = 0.0;
	double EdgePercentage = 0.0;
	double EdgeDistance = 0.0;
	FVector ClosestEdgePoint = FVector::ZeroVector;
};

/**
 * Enough to place with, and enough to argue with afterwards.
 *
 * A miss is a normal result with `bHit=false` and the ray still filled in. It is never `(0,0,0)`
 * and never a guessed fallback: the design says so, and a fallback coordinate is exactly the
 * invented number this whole feature exists to stop.
 */
struct GRAPHCODE_API FGcScreenToWorldResult
{
	bool bUv = false;
	bool bUvLayout = false;
	int32 UvChannel = INDEX_NONE;
	int32 UvIsland = INDEX_NONE;
	FVector2D UV = FVector2D::ZeroVector;
	TArray<int32> UvCandidates;
	FString UvColor;
	FString UvSelector;
	bool bSceneMesh = false;
	FString MeshAsset;
	FString Geometry;
	int32 Instance = INDEX_NONE;
	int32 Triangle = INDEX_NONE;
	FVector Barycentric = FVector::ZeroVector;
	FVector SurfacePosition = FVector::ZeroVector;
	FVector MeshPosition = FVector::ZeroVector;
	FVector MeshNormal = FVector::ZeroVector;
	TArray<FVector> TrianglePoints;
	FString CoordinateActor;
	FString CoordinateComponent;
	int32 CoordinateInstance = INDEX_NONE;
	FVector ShapePosition = FVector::ZeroVector;
	FVector ShapeNormal = FVector::ZeroVector;
	FTransform MeshTransform = FTransform::Identity;
	FTransform CoordinateTransform = FTransform::Identity;
	bool bHit = false;
	FString Space;
	FString AssetPath;
	/** The RESOLVED fraction, whichever unit it was asked in, beside the same depth in cm. */
	double Depth = 0.0;
	double DepthCentimetres = 0.0;

	FVector Location = FVector::ZeroVector;
	FVector Normal = FVector::ZeroVector;
	FVector EntryLocation = FVector::ZeroVector;
	FVector ExitLocation = FVector::ZeroVector;
	bool bHasExit = false;
	double Thickness = 0.0;
	FString ActorName;
	FString ActorLabel;
	FString ComponentName;
	double Distance = 0.0;
	FVector ActorOrigin = FVector::ZeroVector;
	FVector ActorCentre = FVector::ZeroVector;
	FVector ActorForward = FVector::ForwardVector;
	FVector ActorRight = FVector::RightVector;
	FVector ActorUp = FVector::UpVector;

	/**
	 * POINT AND ASK ON A BUBBLE (plan §3.15): the ray met a direction sphere first. The bone, the hit direction as the rig's grammar writes
	 * it (empty where no reader spells it), the same direction in the body's words as numbers, and how far past a stop holding it needs,
	 * read between the four grid samples round it (-1 where no place could be made; 0.05 or under is in range).
	 */
	bool bBubble = false;
	FName BubbleBone;
	FString BubbleToward;
	FVector BubbleDirection = FVector::ZeroVector;
	double BubblePastStop = -1.0;
	/**
	 * Which bubble (FGraphCodeRenderViewResult::FBubble::Kind). A point in a REACH bubble is a place: BubblePlace spells it from the bone as a
	 * .gcmove writes a place's distances, BubbleDistance is how far it is from the joint, BubbleReach how far the end goes that way with every
	 * joint in range (between the net's samples; -1 without a net). Depth runs through the bubble as through a mesh: 0 where the ray enters,
	 * 1 where it leaves.
	 */
	FName BubbleKind;
	FString BubblePlace;
	double BubbleDistance = 0.0;
	double BubbleReach = -1.0;
	/**
	 * THE READY LINE (plan §3.17 item 2): what the key writes for this point, measured from the character's origin as a .gcmove frame is,
	 * `frame(at=(forward=38, up=70), toward=(forward, down=55))` with the combined bubble's wanted aim, else `point(at=(...))`; and on a
	 * combined bubble the nearest hand place tried, how far it is, and how far past a stop the aim needs there (-1 none tried).
	 */
	FString BubbleLine;
	double BubbleAimPast = -1.0;
	double BubbleAimNear = -1.0;
	/** At that place, which joints are past and by how much (`hand_r 12.3, lowerarm_r 2.0`; empty in range), and why an arm in range there meets
	 * the body (empty: clear, or not tried). On a direction sphere BubbleAimBody is why the arm meets the body pointing that way. */
	FString BubbleAimStops;
	FString BubbleAimBody;
	/** At that place, where it holds, the whole chain's pose that makes it (plan §3.19 step 3): each joint's three angles in the rig's words,
	 * `clavicle_r twist 3.0, fold 12.5, side -2.0; upperarm_r ...`; empty where it does not hold. */
	FString BubblePose;
	/** ON A ROLL RING (plan §3.20 step 5): the point is within 4 degrees of the ring square to the wanted aim; the nearest up's degrees past
	 * holding the edge that way (0.05 or under in range; -1 no place) and which joints are past (`hand_r 12.3`); -2 off the ring. */
	double BubbleRollPast = -2.0;
	FString BubbleRollStops;
	/** On the ring, why the arm meets the body with the edge that way though the joints hold it (empty: clear, or not tried). */
	FString BubbleRollBody;
	/** The ring point tried exactly (BUBBLE-GRIP1): with the reach bubble's key:, where the key puts the grip (its own at=), against the body
	 * and the held part, and the key built so (BubbleBuild; BubbleLine the line built). False: the picture's ring, the joints only. */
	bool bBubbleRollExact = false;
	/**
	 * THE MAX EXTENSION (§3.23; user 2026-10-22: "when querying about the point, it should also show max extend"): on a reach bubble whose
	 * capture kept its pose, the place was tried exactly (BubbleAimNear 0) and the line from the bubble's centre through it was searched from
	 * the chain laid out inward: the farthest distance (cm) where the hand holds what the sphere tests, with that place's ready line and
	 * chain. BubbleMaxExtension -1: not searched; 0: nowhere on that line holds.
	 */
	bool bBubbleExact = false;
	/** With the bubble's key: (BUBBLE-GRIP1) the answered place built as the key builds it, a dry run: "accepted", else why not. Empty: not built. */
	FString BubbleBuild;
	/** Where the key's build accepts the place asked and the view's own arm (which the key's balance does not lean) did not hold it: what that
	 * arm met. The build's verdict is the answer (BubbleAimPast 0, BubbleAimBody empty); this is said beside it. Empty: they agree. */
	FString BubbleViewArm;
	double BubbleMaxExtension = -1.0;
	double BubbleMaxSearched = 0.0;
	FString BubbleMaxLine;
	FString BubbleMaxPose;
	/** With the bubble's key (BUBBLE-GRIP1, the lead's ruling 2026-10-26): the max the view's own arm found, built as the key builds it -
	 * "accepted", or how the walk inward went (at most three more builds, 1, 3 and 7 cm in). Empty: no key, or no max. */
	FString BubbleMaxBuild;
	/** With the bubble's key (W10 G15): where its balance leaves the hips with the limb at the max extension, and how many times the line
	 * was searched again from there. Empty: no key, or no max extension asked. */
	FString BubbleSettle;

	/** The ray as traced, so a surprising hit can be re-derived by hand. */
	FVector RayOrigin = FVector::ZeroVector;
	FVector RayDirection = FVector::ZeroVector;
	double RayLength = 0.0;

	/** What was asked, echoed back beside what was found. */
	FString CaptureId;
	FString ViewName;
	double X = 0.0;
	double Y = 0.0;
	int32 PixelX = 0;
	int32 PixelY = 0;
	int32 ViewWidth = 0;
	int32 ViewHeight = 0;
	FString Filter;
	FString LevelPackage;

	TArray<FString> Warnings;
	TArray<FGcScreenRegionResult> Regions;

	/** Optional product metadata for a surface result at depth 0 or 1. */
	bool bHasSource = false;
	FGcMeshSourceResult Source;

	/**
	 * A point between entry and exit is not a surface anchor. Preserve both endpoint answers so a
	 * caller can see whether the ray crossed one source instance or two without inventing a centre.
	 */
	bool bInteriorSource = false;
	bool bHasEntrySource = false;
	FGcMeshSourceResult EntrySource;
	bool bHasExitSource = false;
	FGcMeshSourceResult ExitSource;
};

class GRAPHCODE_API FGcScreenToWorld
{
public:
	/** Integer top-left image pixel to its normalized centre; false outside the raster. */
	static bool PixelCenterNormalized(int32 PixelX, int32 PixelY, int32 Width, int32 Height,
		double& OutX, double& OutY);

	/**
	 * What the trace depends on, hashed — the capture's "is this still the world I photographed".
	 *
	 * It covers every collision-enabled primitive in the persistent level: name, transform, collision
	 * enablement and object type. That is deliberately the TRACE's dependency set rather than the
	 * picture's — moving a light changes the image and not the geometry, and refusing a deprojection
	 * because somebody re-lit the scene would make the token useless in a week.
	 *
	 * **What it cannot see:** a change inside a static mesh asset's own collision setup, since that
	 * lives on the asset rather than the component. Stated rather than papered over; a token that
	 * hashed every referenced asset would cost more than the trace it guards.
	 */
	static FString LevelObservationToken(UWorld* World);

	/**
	 * The LOD 0 positions and triangle indices a mesh preview query depends on, hashed. Empty means
	 * the asset is not a static/skeletal mesh or its CPU geometry is unavailable.
	 */
	static FString MeshObservationToken(UObject* Asset);

	/**
	 * The LOD 0 positions and triangle indices a mesh preview is asked on — a static or skeletal mesh's
	 * render data, or what a registered render subject hands back. The one reader, so an optional
	 * product drawing several meshes hands back exactly what `ask` would have read off each.
	 */
	static bool ReadMeshTriangles(UObject* Asset, TArray<FVector>& OutVertices, TArray<uint32>& OutIndices,
		FString& OutReason);

	/** `Saved/GraphCode/renders/index.tsv` — capture id, manifest path, UTC. Append only. */
	static FString CaptureIndexPath();

	/**
	 * One line per render, so a capture is findable by ID wherever its pictures were written.
	 *
	 * A transient capture could be found by path alone, because its directory IS its ID. A capture
	 * sent somewhere by `output.path` could not, and refusing to deproject those would make the
	 * feature depend on which destination the caller happened to pick.
	 */
	static void RecordCapture(const FString& CaptureId, const FString& ManifestPath);

	/** The manifest path for a capture id, or empty. Index first, transient layout as the fallback. */
	static FString FindManifest(const FString& CaptureId);

	/**
	 * A capture id and a view name to the exact camera that produced that picture. Touches no world.
	 *
	 * Every refusal here is about the RECORD — no such capture, unsupported subject, no such view,
	 * no readable pose — so it can be answered with nothing loaded, which is what lets a caller find
	 * out they named the wrong capture before anything expensive happens.
	 */
	static bool ResolveCaptureView(const FString& CaptureId, const FString& ViewName,
		FGcCaptureView& OutView, TArray<FString>& OutErrors);

	/**
	 * Whether `World` is still the world that capture was taken of.
	 *
	 * SEPARATE FROM THE RESOLVE, because the two answer different questions and a caller may want the
	 * first without the second — and because the token is about COLLISION geometry, so what counts as
	 * stale differs by what the caller is about to do. A trace through a moved world returns a
	 * confident wrong coordinate; a mask painted over a moved world names the wrong pixels. Both are
	 * refusals, and neither is inferable from the other.
	 */
	static bool CheckCaptureWorld(const FGcCaptureView& View, UWorld* World,
		TArray<FString>& OutWarnings, TArray<FString>& OutErrors);

	/**
	 * The whole operation: resolve the capture, refuse everything refusable, then query once.
	 *
	 * Every refusal happens BEFORE querying geometry, because a ray through a stale subject still
	 * returns a confident number. `false` with `OutErrors` filled is a refusal; `true` with
	 * `bHit=false` is an honest miss, and those are different answers.
	 */
	static bool Query(const FGcScreenToWorldRequest& Request, FGcScreenToWorldResult& OutResult,
		TArray<FString>& OutErrors);
};
