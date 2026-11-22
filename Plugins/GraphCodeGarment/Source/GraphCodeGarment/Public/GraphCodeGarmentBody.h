// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "GraphCodeMeshSection.h"
#include "Spatial/FastWinding.h"
#include "UObject/StrongObjectPtr.h"

class USkeletalMesh;
struct FGcHumanBodyLevel;

/** The skin nearest a point, and which way is out from it there. */
struct FGcGarmentSkin
{
	FVector Point = FVector::ZeroVector;

	/** Away from the body, through the point where it is off the skin. */
	FVector Outward = FVector::UpVector;

	/** How far the point is off the skin; negative inside the body. */
	double Distance = 0.0;
};

/**
 * An open edge of the skin that goes round (G4, `skirt-g4-slice1-17nov.md`): where the body as read has no skin - the end of a neck, a
 * wrist, an ankle on a body read without head, hands or feet, or the skin an assembly cut away under clothes. Every body has some or
 * none; one is no fault, and nothing is said of one that no ring and no cloth meets (the lead's ruling, 17 Nov).
 */
struct GRAPHCODEGARMENT_API FGcGarmentOpening
{
	/** Its edge in order, on the skin as read. */
	TArray<FVector> Points;

	/** Its length round, in centimetres: the edge walked once, back to its start where it closes. */
	double LengthCm = 0.0;

	/** The lowest and highest of its points on the mesh's own Z - the up the ring lines word as "looking down". */
	double LowZ = 0.0;
	double HighZ = 0.0;

	/** The middle of its points. */
	FVector Centre = FVector::ZeroVector;

	FBox Bounds = FBox(ForceInit);

	/** False for an open run the walk could not close (a bow-tie corner of the skin): its edge ends at its last point. */
	bool bClosed = true;
};

/** The body a garment is made on, read once: the nearest skin, inside or out, and plane sections. */
class GRAPHCODEGARMENT_API FGcGarmentBody
{
public:
	/** No edge of the body as read is longer than this, in centimetres. */
	static constexpr double RefineCm = 1.5;

	/**
	 * Points that round to the same thousandth of a centimetre are one place: the weld `WeldedSkin` has always made (a value
	 * named, not changed: it was the literal 1000.0 there). `1 / WeldPerCm` is the weld's resolution, in centimetres.
	 */
	static constexpr double WeldPerCm = 1000.0;

	/** The weld's resolution, `1 / WeldPerCm` cm: two places closer than this are one place, and a move or a depth within it is the arithmetic's. */
	static constexpr double SamePlaceCm = 1.0 / WeldPerCm;

	/** How many edges of `InMesh` have one triangle: the solid body's closed-skin test (`GcGarmentSolidBodyElem`) and `OpeningsIn` share it. */
	static int32 OpenEdgeCount(const UE::Geometry::FDynamicMesh3& InMesh);

	/**
	 * Every opening of `InMesh`, largest round first: a copy welded by place (`WeldPerCm`, so an edge split at a UV seam is no opening),
	 * then its boundary loops. Empty for a closed skin. Reads nothing else and says nothing: a caller speaks only of an opening a ring or
	 * the cloth meets (`OpeningAt`).
	 */
	static TArray<FGcGarmentOpening> OpeningsIn(const UE::Geometry::FDynamicMesh3& InMesh);

	/** This body's openings: `OpeningsIn` of its skin as read. Each call reads the skin again; a build calls it once and keeps them. */
	TArray<FGcGarmentOpening> Openings() const { return OpeningsIn(Mesh); }

	/**
	 * Which of `InOpenings` the skin nearest `Point` lies on the edge of, or INDEX_NONE. A point over a hole in the skin - a ring point
	 * past the cut edge, cloth over the gap - has its nearest skin on the hole's edge; a point over skin has it on that skin. "On the
	 * edge" is within the weld's resolution (`1 / WeldPerCm` cm) of the edge's line: no reach is chosen.
	 */
	int32 OpeningAt(const FVector& Point, const TArray<FGcGarmentOpening>& InOpenings) const;

	/** How many of `Points` have their nearest skin on each of `InOpenings` (`OpeningAt`), one count per opening. */
	TArray<int32> OpeningsMet(const TArray<FVector>& Points, const TArray<FGcGarmentOpening>& InOpenings) const;

	/** An opening in words for a note: `141.2 cm round from z 61.0 to z 139.8, centre (0.3, 1.2, 101.5)`. */
	static FString Spelled(const FGcGarmentOpening& Opening);

	/** What an author can do where the body has no skin, in every note and refusal that says so (G4 slice 2). */
	static const TCHAR* NoSkinAdvice();

	/**
	 * One note for each of `InOpenings` that `Points` meet (`OpeningsMet`), and none for an opening they do not meet - a neck, wrist or ankle
	 * end nothing reaches is never mentioned (the lead's ruling, 17 Nov). `Who` starts each note (`ring 'Waist' (line 9)`, `Skirt`); `What`
	 * names the points (`points`, `cloth points`).
	 */
	TArray<FString> NoSkinNotes(const FString& Who, const TCHAR* What, const TArray<FVector>& Points, const TArray<FGcGarmentOpening>& InOpenings) const;

	/**
	 * The header's `mesh:` — a static mesh, a skeletal mesh, or a MetaHuman character read as the head and
	 * body `gc_render` draws — or null with a refusal naming the path.
	 */
	static TUniquePtr<FGcGarmentBody> Load(const FString& MeshPath, FString& OutError);

	/**
	 * The same `mesh:` as a skinned mesh a garment's weights are copied from: a skeletal mesh is its own, a
	 * character is its head and body merged on the body's skeleton. Null with a refusal for a static mesh.
	 */
	static TStrongObjectPtr<USkeletalMesh> LoadSkinned(const FString& MeshPath, FString& OutError);

	/** The same, with the body's detail levels — a skeletal mesh's own, a character's body's. */
	static TStrongObjectPtr<USkeletalMesh> LoadSkinned(const FString& MeshPath, TArray<FGcHumanBodyLevel>& OutLevels, FString& OutError);

	/** The character `mesh:` names, whose wardrobe a skeletal garment goes into; null for a static or skeletal mesh. */
	static UObject* LoadCharacter(const FString& MeshPath);

	/**
	 * Triangles read off a render — split at every UV seam, a head and a body apart — made one skin: points
	 * that share a place welded, and only the pieces as big as a tenth of the largest kept. `OutPieces` says
	 * what was kept and what was left out, one line each.
	 */
	static UE::Geometry::FDynamicMesh3 WeldedSkin(const TArray<FVector>& Positions, const TArray<uint32>& Indices,
		TArray<FString>& OutPieces);

	/** What `WeldedSkin` kept and left out, for the build's notes; empty for a static or skeletal mesh. */
	const TArray<FString>& PiecesRead() const { return Pieces; }

	/** Built from a surface directly — how a spec makes a body without an asset. */
	static TUniquePtr<FGcGarmentBody> FromMesh(const UE::Geometry::FDynamicMesh3& Mesh);

	FVector Nearest(const FVector& Point) const;

	/** The outward normal of the skin nearest `Point`. */
	FVector NormalAt(const FVector& Point) const;

	bool IsInside(const FVector& Point) const;

	/** How far along `Direction` (unit) from `From` a ray first meets the skin; false where it meets none. */
	bool FirstHit(const FVector& From, const FVector& Direction, double& OutDistance) const;

	/**
	 * The nearest skin in one query, with the side read off that triangle's normal — cheaper than
	 * `IsInside`, and what a line of cloth is held off the body by thousands of times.
	 */
	FGcGarmentSkin SkinAt(const FVector& Point) const;

	/**
	 * How far each vertex is from `From` over the skin, along the mesh's edges, never walking an edge
	 * `Blocked` names. A vertex it cannot reach is `TNumericLimits<double>::Max()`.
	 */
	TArray<double> DistancesOverSkin(const FVector& From,
		TFunctionRef<bool(const FVector&, const FVector&)> Blocked) const;

	/** The vertex of the skin nearest `Point`. */
	int32 NearestVertex(const FVector& Point) const;

	/** Where skin vertex `Index` is — an index `DistancesOverSkin` answers for. */
	FVector Vertex(const int32 Index) const { return FVector(Mesh.GetVertex(Index)); }

	const FGcSectionSurface& Surface() const { return Section; }

	/** The skin as read and welded: what a knot's body contact is audited on before it is held as a solid (GC-STRIP-SOLVE-PLAN.md §10 S3). */
	const UE::Geometry::FDynamicMesh3& Skin() const { return Mesh; }

	/** The skin's own triangle tree, for a test the carrier's gate makes face to face (`PutOffSkin`). */
	const UE::Geometry::FDynamicMeshAABBTree3& SkinTree() const { return Tree; }

private:
	FGcGarmentBody() = default;
	void Prepare();

	UE::Geometry::FDynamicMesh3 Mesh;
	TArray<FString> Pieces;
	UE::Geometry::FDynamicMeshAABBTree3 Tree;
	TUniquePtr<UE::Geometry::TFastWindingTree<UE::Geometry::FDynamicMesh3>> Winding;
	FGcSectionSurface Section;
};
