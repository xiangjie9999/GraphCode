// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

/**
 * What an SVG Data asset holds, as plain data, for every GraphCode product that reads one: Shape's `outline()` and Garment's
 * `cut('/Game/Art/Tears')` (GC-GARMENT-GRAMMAR.md §5). Only the GraphCodeShapeSvg adapter touches Epic's SVG Importer types, so
 * each product loads in a project without that plugin and refuses by naming it (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md).
 */
struct FGcSvgPolygon
{
	/** The persisted sampled vertices (X unused; SVG x is +Y, SVG y is -Z). */
	TArray<FVector> Vertices;
	bool bDrawn = true;
};

struct FGcSvgShape
{
	FString Id;
	bool bFill = false;
	bool bVisible = false;
	/** Drawn with a line (`stroke`), and whether that line closes on itself: GraphCode's converter's `--mode lines` writes each ink stroke
	 *  as one, `fill="none"` - a garment reads it as a rip. */
	bool bStroke = false;
	bool bClosed = false;
	TArray<FGcSvgPolygon> Polygons;
};

struct FGcSvgSource
{
	FString FileContent;
	TArray<FGcSvgShape> Shapes;
	/** How far a vertex lies for each unit of the SVG's own coordinates: Epic's importer scales them (`FSVGImporterUtils::SVGScaleFactor`,
	 *  0.1 in UE 5.7). A reader placing shapes on the SVG's canvas (its viewBox, in SVG units) scales the canvas by it; Shape's `outline()`
	 *  sizes every outline to the author's width and needs none. */
	double UnitScale = 1.0;
};

namespace GraphCodeSvg
{
	/** Loads ObjectPath as SVG Data; runs on the game thread. False when it is not one. OutAsset is the build input. */
	using FLoader = TFunction<bool(const FString& ObjectPath, FGcSvgSource& Out, UObject*& OutAsset)>;

	/** The adapter's slot: set at its startup, cleared at its shutdown. */
	GRAPHCODE_API void SetLoader(FLoader Loader);

	/** What the slot holds now: empty when no adapter is loaded (SVG Importer off, or GraphCode Shape not installed). */
	GRAPHCODE_API FLoader Loader();
}
