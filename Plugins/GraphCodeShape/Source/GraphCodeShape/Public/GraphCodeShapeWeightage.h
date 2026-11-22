// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "GraphCodeShapeDocument.h"
#include "GraphCodeWeight.h"
#include "GraphCodeShapeCurvature.h"
#include "Templates/Function.h"

/** An ordered weight recipe. Surface-dependent distances are bound by its selection. */
struct GRAPHCODESHAPE_API FGcShapeWeightage
{
	enum class EMeasure : uint8 { Constant, Points, Lines, Radial, Edge, X, Y, Z, Angle, Noise, Texture, Contact };
	enum class ENoise : uint8 { Perlin, Ridged, Cellular };
	struct FImage
	{
		FIntPoint Size = FIntPoint::ZeroValue;
		TArray<float> Pixels;
		FVector2d Tile = FVector2d(1,1), Offset = FVector2d::ZeroVector;
		uint8 AddressX = 0, AddressY = 0;
		double Sample(const FVector2d& UV) const;
	};
	struct FRule
	{
		EMeasure Measure = EMeasure::Constant;
		FVector3d From = FVector3d::ZeroVector;
		FVector3d To = FVector3d::ZeroVector;
		FVector3d FromEnd = FVector3d::ZeroVector;
		FVector3d ToEnd = FVector3d::ZeroVector;
		FVector3d Facing = FVector3d::ZeroVector;
		FVector3d Direction = FVector3d::ZeroVector;
		double Start = 0.0;
		double End = 1.0;
		int32 Seed = 0;
		FVector3d NoiseOffset = FVector3d::ZeroVector;
		ENoise Noise = ENoise::Perlin;
		double Scale = 10;
		int32 Octaves = 1;
		TSharedPtr<const FImage> Image;
		bool bProjected = false;
		GraphCodeShapeProject::FFrame Projector;
		FGcShapeProfile Profile;
	};
	struct FNode
	{
		int32 Rule = INDEX_NONE;
		int32 Left = INDEX_NONE;
		int32 Right = INDEX_NONE;
		GraphCodeWeight::EOperation Operation = GraphCodeWeight::EOperation::Add;
		double Amount = 0.5;
		bool bUsed = false;
	};
	struct FSample
	{
		FVector3d Position = FVector3d::ZeroVector;
		FVector3d Normal = FVector3d::UnitZ();
		FVector2d UV = FVector2d::ZeroVector;
		double EdgeDistance = TNumericLimits<double>::Max();
		/**
		 * `condition=contact`: how far this point is from the nearest surface of ANOTHER part of the
		 * shape (another connected shell). Round 2 item 4 — the field report's river rocks on tiles.
		 */
		double ContactDistance = TNumericLimits<double>::Max();
		/** Distance from a radial rule's point, indexed by Rules. */
		TArrayView<const double> PointDistances;
	};

	TArray<FRule> Rules;
	TArray<FNode> Nodes;
	int32 Result = INDEX_NONE;
	bool NeedsEdgeDistance = false;
	bool NeedsContactDistance = false;

	using FPointResolver = TFunction<bool(const FString&, FVector3d&, FString&)>;
	static bool Read(const FGcShapeDocument& Document, const FGcShapeEntry& Entry,
		FGcShapeWeightage& Out, FString& OutError, const FPointResolver& ResolvePoint = {});
	double Evaluate(const FSample& Sample) const;
};
