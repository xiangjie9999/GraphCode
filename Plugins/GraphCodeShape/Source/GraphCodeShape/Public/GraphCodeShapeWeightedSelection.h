// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once
#include "GraphCodeShapeWeightage.h"

namespace UE::Geometry { class FDynamicMesh3; }

/** Hard face support and a weight expression are separate, including after composition. */
struct GRAPHCODESHAPE_API FGcShapeWeightedSelection
{
	int32 RegionLayer = INDEX_NONE;
	FGcShapeWeightage Recipe;
	bool bAutomatic = false;
	TSharedPtr<const FGcShapeWeightedSelection> Left, Right;
	GraphCodeWeight::EOperation Operation = GraphCodeWeight::EOperation::Union;
	double Amount = 0.5;

	bool Contains(const UE::Geometry::FDynamicMesh3& Mesh, int32 Triangle) const;
	using FSampler = TFunction<double(const FVector3d&, const FVector3d&, const FVector2d&)>;
	/** Bind to the finished mesh. Translation maps published positions back to authored space. */
	bool Prepare(const UE::Geometry::FDynamicMesh3& Mesh, const FVector3d& Translation,
		FSampler& Out, FString& Error) const;
	/** Geometry samples known triangle corners before moving; no nearest-face ambiguity. */
	bool VertexWeights(const UE::Geometry::FDynamicMesh3& Mesh, const TArray<int32>& Triangles,
		TArray<double>& Out, FString& Error) const;
	/** Smallest spatial feature the active recipe asks geometry to carry; zero for constants. */
	bool FeatureSize(const UE::Geometry::FDynamicMesh3& Mesh, double& Out, FString& Error) const;
	/**
	 * **REFINE ONLY WHERE A CONTACT RULE CAN LEAVE WEIGHT** (2026-10-13). When the recipe is provably
	 * zero farther than some distance from every other part (a `condition=contact` rule whose profile
	 * ends at 0%, under multiply/cap/subtract/exclude, or on both sides of add/union/blend), a triangle
	 * is split only while it is coarser than the feature size AND close enough to another part to
	 * carry weight. `Handled` is false for any other recipe, which keeps the ordinary refinement.
	 */
	bool RefineNearContact(UE::Geometry::FDynamicMesh3& Mesh, const TArray<int32>& Where, bool& Handled,
		const FString& Subject, FString& Error) const;
	static bool SupportsDeformation(const FString& Verb);
};
