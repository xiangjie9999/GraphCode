// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "GraphCodeGarmentProgram.h"

struct FGcGarmentBuiltRing;
struct FGcGarment;
struct FGcGarmentBuiltSurface;
class FGcGarmentBody;

/**
 * How the cloth as laid turns where it passes through one middle ring (G4, the check's G4-7, `skirt-g4-slice1-17nov.md`): the largest turn
 * of its lines at the ring beside the same lines' turns everywhere else. Numbers only - no threshold says what is a kink.
 */
struct GRAPHCODEGARMENT_API FGcGarmentRingTurn
{
	FString Ring;

	/** Lines measured at the ring; lines whose station nearest the ring is their first or last (they end there); panels drafted as a free mesh. */
	int32 Lines = 0;
	int32 EndAtRing = 0;
	int32 FreeMeshPanels = 0;

	/** The largest turn at the ring, in degrees: the angle between a line's step into its station on the ring and its step out. */
	double MostDegrees = 0.0;

	/** Where it is: round the ring from `@0` (`PlaceOf`), and the station itself. */
	double MostU = 0.0;
	FVector MostAt = FVector::ZeroVector;

	/** Which way that line turns there against the skin under it: 1 away from the body, -1 toward it, 0 neither. */
	int32 MostWay = 0;

	/** The furthest any measured line's station on the ring lies off the ring's own line: the measure's check that it is on the ring at all. */
	double OffRingCm = 0.0;

	/** The same lines' turns at every station not on a middle ring and not an end: how many, their median and their largest, in degrees. */
	int32 ElsewhereStations = 0;
	double ElsewhereMedianDegrees = 0.0;
	double ElsewhereMostDegrees = 0.0;

	/** Lines whose station nearest the ring is not on it, within one place (`FGcGarmentBody::SamePlaceCm`): lines of a panel that does not pass through it. */
	int32 NotOnRing = 0;
};

/**
 * The rings as something to look at: a thin coloured tube along each, drawn over the body by
 * `gc_render`'s `overlay:` — the rig's `DIAG_*` precedent. A thicker stretch runs from `@0` a little
 * way round, so one mark says where a ring starts and which way it counts.
 */
class GRAPHCODEGARMENT_API FGcGarmentDiagnostic
{
public:
	static constexpr double TubeCm = 0.25;
	static constexpr double StartTubeCm = 0.7;

	/** How far round the start mark reaches, as a fraction of the ring. */
	static constexpr double StartReach = 0.04;

	/** A stable colour per ring, by the order the file declares them. */
	static FLinearColor RingColour(int32 Index);

	static UE::Geometry::FDynamicMesh3 RingTubes(const TArray<FGcGarmentBuiltRing>& Rings);

	/**
	 * Write the tubes as an ordinary static mesh that shows its vertex colours, rebuilding one already
	 * there, stamped with its body and legend so `gc_render` of it draws it on the body.
	 */
	static bool Write(const UE::Geometry::FDynamicMesh3& Tubes, const FString& PackagePath,
		const FGcGarmentAssetProvenance& Stamp, FString& OutError, bool bSave = true);

	/**
	 * For each of `Garment`'s middle rings (`FGcGarment::Middle`, found by name in `Rings`), how its cloth as laid (`Surface`'s lines) turns
	 * there and elsewhere. A line's station on a ring is its station nearest that ring; its turn is between the step in and the step out at
	 * its own station spacing. A middle ring not found in `Rings` is left out. The cloth AS LAID: the hang is not seen.
	 */
	static TArray<FGcGarmentRingTurn> MiddleRingTurns(const FGcGarment& Garment, const FGcGarmentBuiltSurface& Surface,
		const TArray<FGcGarmentBuiltRing>& Rings, const FGcGarmentBody& Body);

	/** `MiddleRingTurns` in words, one note per middle ring, whatever the angle (`skirt-g4-slice1-17nov.md`). */
	static TArray<FString> MiddleRingTurnNotes(const FString& GarmentName, const TArray<FGcGarmentRingTurn>& Turns);
};
