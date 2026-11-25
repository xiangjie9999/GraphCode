// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * One resolved circle, rectangle or arc before any product-specific projection.
 *
 * These three constructors are shared language vocabulary: Shape turns them into mesh outlines,
 * while Groom projects them onto a growth surface. Keeping their tessellation and transform in
 * Core lets either product ship without depending on the other.
 */
struct GRAPHCODE_API FGcPrimitiveOutline
{
	TArray<FVector3d> Points;
	bool bClosed = false;
};

struct GRAPHCODE_API FGcPrimitiveOutlineTransform
{
	FVector3d Translation = FVector3d::ZeroVector;
	FVector3d RotationDegrees = FVector3d::ZeroVector;
	FVector3d Scale = FVector3d::OneVector;
};

namespace GraphCodePrimitiveOutline
{
	/** Smallest span count whose chord stays within the stated tolerance. */
	GRAPHCODE_API int32 SegmentsForArc(double Radius, double Tolerance, double SweepDegrees);

	GRAPHCODE_API FGcPrimitiveOutline Circle(double Radius, int32 Segments);
	GRAPHCODE_API FGcPrimitiveOutline Rectangle(double Width, double Height);
	GRAPHCODE_API FGcPrimitiveOutline Arc(double Radius, double FromDegrees, double ToDegrees,
		int32 Segments);

	/** Scale, then Unreal's ordinary Rotator composition, then translate. */
	GRAPHCODE_API void Transform(FGcPrimitiveOutline& Outline,
		const FGcPrimitiveOutlineTransform& Placement);
}
