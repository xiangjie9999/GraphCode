#pragma once

#include "CoreMinimal.h"

struct FGcShapeDocument;
struct FGcShapeInstruction;
namespace UE::Geometry { class FDynamicMesh3; }

/**
 * GROUP 1 — SKELETAL SCULPT: `ball()`, `limb()` and `sculpt()` (SHAPE-ANIMAL1, 2026-11-22).
 *
 * An organic body is written as the masses a sculptor blocks in — balls (ellipsoids) and limbs
 * (round cones, or a chain of them through points) — each melting into what came before it by its
 * own `blend=` distance. Nothing is meshed per line: the lines collect into ONE analytic distance
 * field, and the field is meshed once, by marching cubes followed by a remesh that projects every
 * vertex back onto the field. So the joins (shoulder, hip, neck root, tail root) are one surface by
 * construction, never a weld between two meshes, and every position is the author's arithmetic
 * over named proportions: lengthen the neck and the head moves with it because its `at=` is
 * computed from the neck's end.
 *
 * `ball(at=(x,y,z), r=<cm>|(rx,ry,rz), rotate=(roll,pitch,yaw), blend=<cm>, op=add|carve|raise, by=<cm>, mirror=X|Y|Z)`
 * `limb(from=(..), to=(..), r=(r0, r1), blend=, op=, mirror=, flat=<ratio>, up=<direction>)`
 * `limb(through=((..), (..), ...), r=(r0, ..., rn) | (r0, rn) | r, blend=, op=, mirror=, flat=, up=)`
 * `sculpt(cell=<cm>, edge=<cm>, passes=<n>)` — optional; meshes the field now, with these settings.
 *
 * The field is meshed at the first line that is not a sculpt primitive, or where the body ends.
 */
namespace GraphCodeShapeSculpt
{
	/** One round cone: the convex hull of a sphere of RadiusA at A and of RadiusB at B. */
	struct FSegment
	{
		FVector3d A = FVector3d::Zero();
		FVector3d B = FVector3d::Zero();
		double RadiusA = 1.0;
		double RadiusB = 1.0;
	};

	struct GRAPHCODESHAPE_API FPrimitive
	{
		enum class EKind : uint8 { Ball, Limb };
		EKind Kind = EKind::Ball;
		bool bCarve = false;
		/** `op=raise`: lifts (or with a negative `by=`, grooves) the skin already there, by Raise cm at the core, fading to 0 at the radius. */
		bool bRaise = false;
		double Raise = 0.0;

		/** Ball: centre, radii, and the rotation that takes local axes to world (inverse applied to a point). */
		FVector3d Centre = FVector3d::Zero();
		FVector3d Radii = FVector3d::One();
		FQuat4d Rotation = FQuat4d::Identity;

		/** Limb: consecutive round cones, unioned exactly among themselves. */
		TArray<FSegment> Segments;
		/** Limb cross-section width over depth across `Side` (1 = round). */
		double Flat = 1.0;
		FVector3d Up = FVector3d::UnitZ();

		/** How far this primitive melts into the field before it, in cm (polynomial smooth min). */
		double Blend = 0.0;

		/** -1 none, else 0/1/2: a mirrored copy across the plane through the origin normal to X/Y/Z. */
		int32 MirrorAxis = INDEX_NONE;

		int32 Line = 0;

		/** Signed distance (negative inside), before blending, of this primitive alone. */
		double Distance(const FVector3d& Point) const;

		/** 0 at the core, 1 at the radius: how far a point is into this primitive's reach, for `op=raise`. */
		double Influence(const FVector3d& Point) const;

		/** World-space box that contains the primitive (and its mirror), grown by nothing. */
		FBox3d Bounds() const;

		/** The smallest radius it carries, for the automatic cell size. */
		double SmallestRadius() const;

		/** Points well inside the primitive from which a surface search may start. */
		void Cores(TArray<FVector3d>& Out) const;
	};

	struct GRAPHCODESHAPE_API FOptions
	{
		/** Marching-cubes cell in cm; 0 = automatic (a third of the smallest added radius, bounded). */
		double Cell = 0.0;
		/** Remesh target edge in cm; 0 = 1.5 cells. */
		double Edge = 0.0;
		/** Remesh passes that also project onto the field; 0 keeps the raw marching-cubes surface. */
		int32 Passes = 6;
	};

	struct GRAPHCODESHAPE_API FField
	{
		TArray<FPrimitive> Primitives;
		FOptions Options;
		int32 FirstLine = 0;

		bool IsEmpty() const { return Primitives.IsEmpty(); }
		void Reset() { Primitives.Reset(); Options = FOptions(); FirstLine = 0; }

		/** The blended signed distance at a point, every primitive in order. */
		double Evaluate(const FVector3d& Point) const;
	};

	/** `ball`, `limb` — the lines that collect into the pending field. */
	GRAPHCODESHAPE_API bool IsPrimitiveVerb(const FString& Verb);

	/** `sculpt` — the line that meshes the field now, with settings. */
	GRAPHCODESHAPE_API bool IsMeshVerb(const FString& Verb);

	/** Reads one `ball()`/`limb()` line into the field, or a `sculpt()` line's settings. */
	GRAPHCODESHAPE_API bool Read(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		FField& Field, FString& OutError);

	/** Meshes the field into a closed, outward-facing surface with UVs and normals. */
	GRAPHCODESHAPE_API bool Mesh(const FField& Field, UE::Geometry::FDynamicMesh3& OutMesh,
		FString& OutError, TArray<FString>* OutNotes);
}
