// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class UPhysicsAsset;
class UStaticMesh;
struct FKAggregateGeom;

/** Which of a static mesh's geometry a collision surface is made of (FGcBodyCollision::AddSurface): its drawn triangles, or its simple collision. */
enum class EGcSurfaceSolid : uint8 { Mesh, Collision };

/** Read-only collision audit shared by Rig and Move. Uses the mesh's native PhysicsAsset. */
class GRAPHCODE_API FGcBodyCollision
{
public:
	struct FContact
	{
		int32 BoneA = INDEX_NONE, BoneB = INDEX_NONE;
		int32 ShapeA = INDEX_NONE, ShapeB = INDEX_NONE;
		FVector Position = FVector::ZeroVector;
		/** Native minimum-translation direction for the second shape. */
		FVector Normal = FVector::ZeroVector;
		double Penetration = 0, ReferencePenetration = 0;
	};
	FGcBodyCollision();
	~FGcBodyCollision();
	FGcBodyCollision(const FGcBodyCollision&) = delete;
	FGcBodyCollision& operator=(const FGcBodyCollision&) = delete;
	void Initialize(USkeletalMesh* Mesh, TArray<FString>& Notes);
	/** DeclaredRest (OBJ1t v004): the rig's declared rest (`rest=`, the pose every clip starts from since RIG-REST1), local, one transform per
	 * bone of Mesh; empty = the reference pose only. A pair's allowed rest overlap is the deeper of its overlap at the reference pose and here:
	 * the rig's own rest is never refused, and motion may not deepen it. Skin points stay stored against the reference (bind) pose.
	 * Returns false (with a note saying why, and nothing set up) only when a DeclaredRest is given that is not one finite transform per bone:
	 * a malformed rest is refused, never replaced by the reference pose. With none given it is true. */
	bool Initialize(USkeletalMesh* Mesh, UPhysicsAsset* Physics, TArray<FString>& Notes, const TArray<FTransform>& DeclaredRest = {});
	bool HasCoverage() const;
	/** Whether any shape is fixed in the component frame rather than carried by the body: a ground (AddGround) or a held surface
	 * (AddSurface). Without one, every pair's depth depends only on the relative pose, and a common translation of the whole
	 * body changes none (the balance screen's sweep relies on that; MOVE-ROOT1 2026-10-12). */
	bool HasWorldGeometry() const;
	/** Posed native contacts, including reference overlap, for corrective authoring solvers. */
	bool Contacts(const TArray<FTransform>& Local, TArray<FContact>& Out, FString& Error) const;
	struct FSurfacePoint
	{
		int32 Bone = INDEX_NONE, Shape = INDEX_NONE;
		FVector Position = FVector::ZeroVector, Normal = FVector::ZeroVector;
		double Distance = 0;
	};
	struct FSeparation
	{
		FVector SourcePoint=FVector::ZeroVector, TargetPoint=FVector::ZeroVector;
		/** Direction from target toward source. Distance is negative for penetration. */
		FVector Normal=FVector::ZeroVector;
		/** NearSurfacePairs: the skin's own outward normal at SourcePoint, posed. Deep inside a held part the
		 * nearest face of the part may be its far side; the skin still says which way is out. Zero elsewhere. */
		FVector SourceNormal=FVector::ZeroVector;
		double Distance=0, ReferencePenetration=0;
		int32 SourceShape=INDEX_NONE, TargetShape=INDEX_NONE;
		int32 SourceBone=INDEX_NONE, TargetBone=INDEX_NONE;
		/** NearSurfacePairs: which of the source body's skin points this is, for PosedSkinPoint. INDEX_NONE elsewhere. */
		int32 SourceSkinPoint=INDEX_NONE;
		/** NearSurfacePairs: how far SourcePoint is from the nearest hole its surface was left open at (the hole's sphere; negative inside one), the
		 * largest move of the surface over which this Distance changes by no more than the move (CanBoundSurfaceTranslation). Max with no hole. */
		double HoleClearance=TNumericLimits<double>::Max();
	};
	/** Native signed convex separation for an enabled source-bone/target-shape pair. */
	bool Separation(const TArray<FTransform>& Global, int32 SourceBone, int32 TargetShape,
		FSeparation& Out, FString& Error) const;
	/** The middle of a bone's body in this pose: of its skin where the rig built it as a hull of the
	 * skin, of its shapes otherwise. Where a touch with no named pad starts looking. */
	bool Centre(const TArray<FTransform>& Global, int32 Bone, FVector& Out) const;	/** Nearby convex body pairs for predictive kinematic contact. External surfaces are audited separately. */
	bool NearBodyPairs(const TArray<FTransform>& Global, double Within,
		TArray<FSeparation>& Out, FString& Error) const;
	/** THE BODY PAIRS A CHECK OF THIS POSE WOULD REFUSE, by the check's own measure (the first frame's push-out; user, 2026-10-12: "the engine
	 * absorbs physics; authors never tune cm"): per live body pair past its allowance, where the skin confirms the pair, the deepest skin vertex
	 * of the body that is inside and the way out of the other's piece to its nearest surface; else the hulls' separation. Source is the body
	 * that is inside, Distance minus its depth, ReferencePenetration the allowance the check judges it by. bBothWays: where the skin confirmed
	 * it, each side's skin against the other's piece, one entry each, so a caller moving one side pushes by the skin it moves; a side
	 * whose skin is not in the other's piece at all has Distance 0 and no point (Normal zero). */
	bool RefusedBodyPairs(const TArray<FTransform>& Global, TArray<FSeparation>& Out, FString& Error, bool bBothWays = false) const;
	/** Local to component space as the checks compose it: each baked twist helper rolled from its driver first (FData::Global). A solve that
	 * measures a pose to recover it must measure THIS pose, or it pushes a helper's skin from where the check does not see it (codex 17:29Z). */
	void ComposeGlobal(const TArray<FTransform>& Local, TArray<FTransform>& Out) const;
	/** Held surfaces near each body's skin, where the surface is measured on the skin: per pair, the
	 * body's skin vertex nearest the surface (the deepest one where it is inside), its signed distance and
	 * the surface's outward normal there. Source is the body, target the surface; ReferencePenetration
	 * is how far the flesh gives. A solve keeping a body out of a held thing plans by these. */
	bool NearSurfacePairs(const TArray<FTransform>& Global, double Within,
		TArray<FSeparation>& Out, FString& Error) const;
	/** What a clearance question is about (P2c, GC-MOVE-GRIP-PLACEMENT-PLAN.md): the bones that move, and the held part. */
	struct FSurfaceClearScope
	{
		/** The bones whose skin is asked about (the chain, its descendants, the helpers they drive), collision bone order.
		 * ClearOfSurface closes it over descendants and driven helpers itself, as it owns both. */
		TSet<int32> Bones;
		/** The held part's shape, by its stored name: two parts can ride one bone. */
		FName TargetShape;
	};
	/** What a clearance question found. Eligible is what the scope asks for, Covered what the query could measure
	 * (an enabled pair, skin on record); Evaluated is how many pairs had skin near enough to be measured at all, so a
	 * scope whose skin is far away reads Evaluated 0 with Covered above 0 and is clear. */
	struct FSurfaceClearReceipt
	{
		int32 EligiblePairs = 0, CoveredPairs = 0, EligibleSkinPoints = 0, CoveredSkinPoints = 0, Evaluated = 0;
		double MaxPastCm = 0.0; FSeparation Deepest; FString Why;
	};
	enum class ESurfaceClear : uint8 { Clear, Blocked, Unavailable, Error };
	/** From a LOCAL pose (collision bone order): the conversion to component space runs here, through the same placement the final audit uses
	 * (helpers included). Coverage is resolved BEFORE any distance is read: the target shape by name, every enabled skin pair between a body whose
	 * skin moves with Scope.Bones and that shape, and that the skin is there. A required pair that is excluded or a body whose skin is missing is
	 * Unavailable; a scope whose pairs are simply far away is Clear. Past = max(0, -Distance - ReferencePenetration); Clear iff every covered pair's
	 * past <= 1e-4 cm. A non-finite measurement is Error.
	 * Required is a body that owns a bone of the scope: it must be enabled against the part and have skin. A body reached only through the
	 * bones its skin follows is measured where its pair is enabled, and an exclusion the build made for it stands (a palm that holds the part).
	 * Read-only: no AcceptOverlaps, no memo, safe on a pose a search is only trying. */
	ESurfaceClear ClearOfSurface(const TArray<FTransform>& Local, const FSurfaceClearScope& Scope, FSurfaceClearReceipt& Out) const;
	/** How near a scope's skin is to a named held surface (OBJ1 S5, Docs/GC-MOVE-OBJECT-INTERACTION-PLAN.md): coverage first, as ClearOfSurface, but
	 * read-only and INDEPENDENT of the checks' exclusions, so a hand holding the part (its pairs exempt) is measured too. Every body whose skin moves
	 * with Scope.Bones, against every shape stored under Scope.TargetShape; each posed skin vertex's RAW signed distance to the solid (negative inside),
	 * with the flesh's extra give at that vertex reported beside it and never subtracted. Measured with WithinCount 0 means nothing of the scope is
	 * within Within; Unavailable names why it could not be measured (no such surface, an unsealed solid, a body owning a scoped bone with no skin);
	 * Error a non-finite pose, range or measurement. Local pose, collision (mesh) bone order. */
	enum class ESkinToSurface : uint8 { Measured, Unavailable, Error };
	struct FSkinToSurfaceReceipt
	{
		int32 EligibleBodies = 0, CoveredBodies = 0, SkinPoints = 0, WithinCount = 0;
		/** The nearest skin vertex: the first shape of its body and that body's bone, its raw signed distance (cm), the extra give there, and where it is. */
		FName NearestShape, NearestBone;
		double NearestDistance = 0, NearestGive = 0;
		FVector NearestPoint = FVector::ZeroVector;
		FString Why;
	};
	ESkinToSurface SkinToSurface(const TArray<FTransform>& Local, const FSurfaceClearScope& Scope, double Within, FSkinToSurfaceReceipt& Out) const;
	/** One held surface against another in this pose (OBJ1 S4, a prop set down onto a table): every vertex of every shape stored under From, taken into
	 * every shape stored under To, its signed distance to that solid (negative: that deep inside). OutGap is the smallest, OutPoint where it is (component
	 * space). A vertex sample: an edge crossing a face between vertices, or To wholly inside From, is not seen. False with Error: no such surface, an
	 * unsealed To, a solid with no tree, a non-finite value. Local pose, collision bone order. */
	bool SurfaceGap(const FString& From, const FString& To, const TArray<FTransform>& Local, double& OutGap, FVector& OutPoint, FString& Error) const;
	/** The lowest vertex of a held surface in this pose (component space): its z and where it is. False with Error as SurfaceGap. */
	bool SurfaceLowest(const FString& Name, const TArray<FTransform>& Local, double& OutZ, FVector& OutPoint, FString& Error) const;
	/** Where a skin point NearSurfacePairs answered with (its SourceShape and SourceSkinPoint) is in the pose `Global`,
	 * with every influence it has. Carried rigidly on its bone instead, a point weighted to two finger bones is off by
	 * 0.0073 cm for a 1 degree turn of the joint between them (codex, Docs/Research/bounded-hand-placement-solver.md). */
	bool PosedSkinPoint(const TArray<FTransform>& Global, int32 SourceShape, int32 SkinPoint, FVector& Out) const;
	/** OBJ1t (Docs/GC-MOVE-DRAWN-BODY-PLAN.md): every skin point read from a part worn over the mesh (FGcRigBodyFit::Worn), posed from a LOCAL pose
	 * (collision bone order) exactly as every check poses skin: the bone of its body, the worn entry's index, the LOD 0 vertex of that worn mesh it
	 * came from, and where it is in component space. Read-only; what a leader-pose comparison against the engine's own skinning reads. */
	struct FWornPoint { FName Body; int32 Worn = INDEX_NONE; int32 Vertex = INDEX_NONE; FVector At = FVector::ZeroVector; };
	bool WornSkinPoints(const TArray<FTransform>& Local, TArray<FWornPoint>& Out, FString& Error) const;
	/** All surfaces NearSurfacePairs can use have continuous signed distance after capping.
	 * For a fixed pose translated through one carrier subtree, normalized skin weights and
	 * unit-scale placed solids then give a 1-Lipschitz bound in component centimeters.
	 * This does not certify rotation, changing geometry/give, or open winding surfaces.
	 * A surface open only at its holes left open (a within= hole, a declared opening, one no cap seals) counts too (MOVE-OPENRIM1, 2026-10-26;
	 * gc.Collision.HoleBound): off those holes its sign changes only across a triangle, so the bound holds for a witness point no further moved
	 * than its FSeparation::HoleClearance. */
	bool CanBoundSurfaceTranslation() const;
	/** Boundary points on selected native shapes, sorted by distance; bSearchArea includes additional area samples.
	 * Global uses mesh bone order.
	 * SourceBone restricts results to enabled collision pairs; INDEX_NONE queries the source itself. */
	bool SurfacePoints(const TArray<FTransform>& Global, const FVector& From, const TArray<int32>& Bones,
		int32 SourceBone, TArray<FSurfacePoint>& Out, FString& Error, bool bSearchArea = false) const;
	/** Takes the body's pairs with itself out of the check, all but those with a body at or below one of KeepBelow (OBJ1s P2,
	 * Docs/GC-MOVE-COLLISION-TARGETS-PLAN.md: a pose that stops on part of the body, a hand being its fingers and their helper
	 * bodies). Empty: every body pair goes, as before. Returns how many body pairs are left. */
	int32 ExcludeSelfPairs(TConstArrayView<FName> KeepBelow = {});
	/** Add actual source triangles attached to an explicit body bone, or component space. Where the
	 * physics asset carries a `.gcrig` build's record its bodies are hulls of the skin, and the surface
	 * is measured against the skin itself: the deepest posed skin vertex inside it, allowed as far as
	 * the rig says the flesh gives (GC-MOVE-GRIP-SEATING.md). Hand-made bodies keep their shapes.
	 * Its holes are sealed, so it is a solid (ruled by the user 2026-10-02, GC-COLLISION-SPEED.md), all but
	 * the hole nearest each of Within: points in the mesh's own space, as its sockets are, that a chain
	 * goes within (`reach(..., within=hole)`).
	 * Solid says which of the mesh's geometry is the surface (OBJ1r, Docs/GC-MOVE-COLLISION-SOURCE-PLAN.md): its drawn source triangles (the
	 * default), or its simple collision, separate enabled closed convexes, which read no mesh description and have no hole to go within.
	 * Closed hulls that overlap or touch are refused: their internal triangles are not the union's depth. */
	bool AddSurface(UStaticMesh* Mesh, FName AnchorBone, const FTransform& Local, const FString& Name, FString& Error,
		TArray<FString>* Notes = nullptr, const TArray<FVector>* Within = nullptr, EGcSurfaceSolid Solid = EGcSurfaceSolid::Mesh);
	/** MOVE-DUO1 S1b (Docs/GC-MOVE-PAIRED-PLAN.md): a solid made of simple physics shapes, not a mesh -- one body of another person's physics asset.
	 * Its spheres, capsules (tapered ones too), boxes and convexes are each a closed convex piece, a round one as a polyhedron round it (never
	 * inside it); a piece with its collision off is no part of it; pieces that overlap or touch are refused, as for solid=collision, so a caller adds
	 * such a piece as its own surface. Placed at Local (component space with AnchorBone None, else on that bone of this body), it is checked as a
	 * solid=collision surface is, and SetSurfaceMotion moves it. Who names it in a refusal. */
	bool AddShapeSurface(const FKAggregateGeom& Shapes, FName AnchorBone, const FTransform& Local, const FString& Name, const FString& Who,
		FString& Error, TArray<FString>* Notes = nullptr);
	/** A surface already added stops checking these bones' bodies, and only these: a pad HELD on a held
	 * thing touches it on purpose, as a body contact's limb is left out of what it touches
	 * (GC-MOVE-GRIP-SEATING.md, "The forefinger first"). Returns how many pairs it dropped. */
	int32 ExcludeSurfacePairs(const FString& Name, TConstArrayView<FName> Bones);
	/** A prop that changes holder during a clip (GraphCodeMove's object interaction, Docs/GC-MOVE-OBJECT-INTERACTION-PLAN.md S2). A surface's
	 * anchor (its bone, INDEX_NONE for component space, and its unit-scale transform there) is read and set exactly, or moved onto another anchor
	 * keeping where the LOCAL pose `Local` (this collision's bone order) puts it now, as Unreal's KeepWorldTransform keeps a component. The pairs a
	 * surface has do not depend on its anchor, so they stay; every cached placement is dropped. False: no surface of that name, or no such bone. */
	bool SurfaceAnchor(const FString& Name, int32& OutBone, FTransform& OutLocal) const;
	bool SetSurfaceAnchor(const FString& Name, int32 Bone, const FTransform& Local, FString& Error);
	bool ReanchorSurface(const FString& Name, FName AnchorBone, const TArray<FTransform>& Local, FString& Error);
	/** OBJ1 S6 (Docs/GC-MOVE-OBJECT-INTERACTION-PLAN.md, "S6, the final design" 7): a prop let go under physics has LEFT the collision. Every shape stored
	 * under Name is taken out (bEnabled false) or put back; shape and pair indices, anchors and exclusions stay. A shape out of the collision is in no
	 * check, contact, near or passing query, and a query that names it answers Unavailable or false, never "clear". Only that surface's own pairs'
	 * kept state is forgotten (their depths, overlaps accepted at a start, the contact memo), never the body's accepted overlaps. False: no surface of
	 * that name. */
	bool SetSurfaceEnabled(const FString& Name, bool bEnabled, FString& Error);
	/** DOOR1 D5 (Docs/GC-MOVE-MOVING-PARTS-PLAN.md, "Collision on fractional time"): a part that moves along its own clip while a body is checked
	 * against it -- a door a hand pulls open. PerFrame is where the surface stands at each sample a check takes (the index CheckSamples hands its
	 * Sample, the clip's frame), as a component-space transform applied after the surface's own placement; between samples it is blended, so the
	 * swept checks meet the part where it is at every fractional time they measure, and an interval is subdivided for the part's own travel too.
	 * Empty takes the motion off. Only a surface placed in component space moves so (one anchored on a bone already rides it). Its pairs are
	 * measured again whenever the time moves; nothing changes for a collision with no moving surface. False: no surface of that name, one on a
	 * bone, or a key that is not finite or not a rotation. */
	bool SetSurfaceMotion(const FString& Name, const TArray<FTransform>& PerFrame, FString& Error);
	/** Whether every shape stored under Name is in the collision; false for no such surface. */
	bool IsSurfaceEnabled(const FString& Name) const;
	/** How deep each body a surface no longer stops (ExcludeSurfacePairs) is inside it in this pose, measured
	 * as the checks measure a pair: on the skin where the rig built the bodies from it. It stops nothing; a
	 * build says with it how far a held chain passes through what it holds (GC-MOVE-GRIP-SEATING.md). One
	 * entry per left-out pair, so a bone with several shapes may come more than once. Local pose, mesh order.
	 * Allowed is how deep the checks would let that pair go were it still checked (the flesh's give). */
	struct FPassing { FName Surface, Bone; double Depth = 0, Allowed = 0; };
	bool PassingDepths(const TArray<FTransform>& Local, TArray<FPassing>& Out, FString& Error) const;
	/** Research only (`passing_witness` in the capture request): for one pair PassingDepths measures, the deepest skin
	 * vertices exactly, each with its raw depth, the flesh's extra give there, its rest and posed point, the solid-local
	 * point, the winding number, and the triangle it is measured against and whether that is a filled hole's. JSON;
	 * empty where no such pair was taken out of the checks. */
	FString PassingWitness(const TArray<FTransform>& Local, FName Surface, FName Bone) const;
	/** Infinite component-space floor; the half-space below Z is solid. */
	bool AddGround(double Z, const FString& Name, FString& Error);
	/** Penetration beyond reference, in cm, that CheckMotion/CheckSamples accept through interpolation. */
	static constexpr double InterpolationTolerance = 0.05;
	/** Whose skin a drawn vertex is (Initialize): the rigid piece that moves it most, its weights (bone, weight) summed by the
	 * piece each bone's body is in (`BodyOf` by bone, `PieceOfBody` by body), then that piece's strongest bone. INDEX_NONE where
	 * no influence has a body. */
	static int32 SkinBoneOf(const TArray<TPair<int32,double>>& Influences, const TArray<int32>& BodyOf, const TArray<int32>& PieceOfBody);
	/** What the last checked pose had past its allowance it may keep and not deepen, until ForgetAcceptedOverlaps: a
	 * start a fit was handed and accepted within its tolerance (2026-10-04), as an overlap at rest may stay. Returns how many
	 * pairs it accepted that were not accepted that deep before (LIMB-BASE1): a refusal names bodies and a depth, so two shapes
	 * of the same two bodies read alike, and only this count tells a caller re-checking one pose whether it made progress. */
	int32 AcceptOverlaps() const;
	/** The pairs AcceptOverlaps let stand and the depth each may keep, spelled as a refusal names them ("'A' in 'B' 1.234567 cm"),
	 * joined by "; " in pair order; "none" when nothing is kept. For notes that say what a search was allowed to keep. */
	FString DescribeAcceptedOverlaps() const;
	void ForgetAcceptedOverlaps() const;
	/** Whether turning these bones (their local transforms changed) can change the pair the last refusing check stopped on: a bone above some of
	 * what the pair reads (its shapes' bones, their skin's bones, its rigid piece) and not above all of it moves one part of the pair against the
	 * other; a bone above all of it carries the pair whole, and one above none of it leaves it be. True when nothing was refused, or the pair rides
	 * no bone on one side (a ground). MOVE-KIN1 2026-10-25: a form fit's step is judged only on what that step changes. */
	bool LastRefusalMovedBy(TConstArrayView<int32> Changed) const;
	/** Keep the pair the last refusing check stopped on at the depth it was refused at, for ONE step: it may stay, not deepen, until
	 * ForgetStepKeeps (a keep that outlived its step let a push-out leave a forearm 3.47 cm in the belly). False when nothing was refused or it
	 * is already kept that deep. */
	bool KeepLastRefusal() const;
	/** Excuse the pair the last refusing check stopped on for the rest of ONE step, at any depth, until ForgetStepKeeps: for a caller that has
	 * established (LastRefusalMovedBy) that its step cannot move one part of the pair against the other, so whatever depth the pair reaches
	 * anywhere along the step is the handed motion's own. Kept at its refused depth instead, each sweep found it a hair deeper further along
	 * the step: a balance candidate re-swept 17 times and was refused anyway, every hip it tried, 10 minutes on one sample (MOVE-BALSMOOTH1,
	 * 2026-10-25). False when nothing was refused or the pair is already excused. */
	bool ExcuseLastRefusal() const;
	void ForgetStepKeeps() const;
	/** Start establishing a different pose's overlap baseline. Discard accepted depths and checked-pose history so
	 * AcceptOverlaps cannot adopt another pose's unmeasured depths after an early refusal. Geometry caches remain keyed by pose. */
	void BeginOverlapBaseline() const;
	/** Dense local poses, including interpolation between them. Refuses worsening penetration. */
	bool CheckMotion(const TArray<TArray<FTransform>>& Poses, const FString& Clip, FString& Error) const;
	/** Form fitting stops at contact; the ordinary audit retains its numerical penetration tolerance. */
	bool CheckMotionToContact(const TArray<TArray<FTransform>>& Poses, const FString& Clip, FString& Error,
		double NumericalTolerance = 1.e-4) const;
	/** Acquisition spans a whole transition, not one baked frame. Keep geometric sampling without the frame-size guard. */
	bool CheckTransitionToContact(const TArray<FTransform>& From, const TArray<FTransform>& To,
		const FString& Name, FString& Error) const;
	/** SampleRate, when positive, adds seconds at the failing interpolated output frame; zero means the caller has no clock. */
	bool CheckSamples(int32 Count, TFunctionRef<void(int32,TArray<FTransform>&)> Sample,
		const FString& Clip, FString& Error, double SampleRate = 0.0) const;
	/** One pair past its allowance somewhere in an inventory: frames are interpolated output frames, as a refusal names them. */
	struct FOverAllowance
	{
		FName A, B;
		bool bSkin = false;
		double First = 0, Last = 0, DeepestAt = 0, Deepest = 0, Allowed = 0;
		int32 Samples = 0;
		/** Why Allowed is what it is, in a refusal's words ("their overlap at rest", ...). */
		FString AllowedBy;
	};
	/** A1 (Docs/Research/metahuman-animation-report-claims.md): what InventorySamples found. Budget (seconds) is the input. */
	struct FInventory
	{
		double Budget = 30.0;
		TArray<FOverAllowance> Pairs;           // in the order first found
		double Through = 0;                      // the last interpolated frame measured
		bool bTimedOut = false;
		TArray<FIntPoint> Unmeasured;            // (frame before, frame) intervals past the 512-subdivision guard
		double Seconds = 0;
	};
	/** EVERY PAIR A CHECK OF THESE SAMPLES FINDS PAST ITS ALLOWANCE, by CheckSamples' own measure, tolerance and sampling, for the report of a
	 * check that refused: it decides nothing, and it does not stop at the first pair or sample. Stops at Out.Budget seconds, and says so. */
	bool InventorySamples(int32 Count, TFunctionRef<void(int32,TArray<FTransform>&)> Sample, FInventory& Out, FString& Error) const;
	/** What the swept checks have cost since this was made, in seconds, so a build can say where its
	 * time went rather than have it guessed at: placing the shapes, measuring body pairs (with the skin
	 * confirming a rigid piece), and measuring skin against held surfaces. */
	/** SkinAsked: skin vertices tested against held parts; of them SkinEmpty cleared by the empty-cell memo, SkinGrid
	 * decided by the distance grid, SkinExact measured with the winding number (GC-COLLISION-SPEED.md). */
	/** Contact/Near: seconds in Contacts() and NearBodyPairs(), which corrective solvers ask per trial pose; of the pairs
	 * Contacts() looked at, ContactMeasured were measured and ContactReused kept their last answer (gc.Collision.ContactMemo);
	 * NearMeasured pairs were near enough to be measured (Docs/Research/body-contact-build-cost.md). */
	struct FCost { double Placing = 0, Bodies = 0, Skin = 0, Contact = 0, Near = 0; int64 Checks = 0, Poses = 0, SkinAsked = 0, SkinEmpty = 0, SkinGrid = 0, SkinExact = 0,
		ContactQueries = 0, ContactMeasured = 0, ContactReused = 0, NearQueries = 0, NearMeasured = 0; };
	FCost Cost() const;
	/** Upward root corrections measured over the same swept floor samples as the final audit. */
	bool FloorCorrections(int32 Count, TFunctionRef<void(int32,TArray<FTransform>&)> Sample,
		TArray<double>& Lift, FString& Error) const;

private:
	/** The shape just added at Surface is paired with every body of this collision (AddSurface, AddShapeSurface). */
	bool PairSurface(int32 Surface, const FString& Name, TArray<FString>* Notes, FString& Error);
	bool ShapeSeparation(const TArray<FTransform>& Global, int32 Source, int32 Target,
		FSeparation& Out, FString& Error) const;
	bool CheckSamplesWithTolerance(int32 Count, TFunctionRef<void(int32,TArray<FTransform>&)> Sample,
		const FString& Clip, FString& Error, double PenetrationTolerance, TArray<double>* FloorLift = nullptr,
		bool bWholeTransition = false, double SampleRate = 0.0, FInventory* Inventory = nullptr) const;
	struct FData;
	TUniquePtr<FData> Data;
};
