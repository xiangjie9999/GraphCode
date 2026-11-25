// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GraphCodeSvgSource.h"

/**
 * Shape's SVG slot. The plain types and the slot itself live in GraphCode Core (`GraphCodeSvgSource.h`), shared with Garment's
 * `cut('/Game/Art/Tears')` (GC-GARMENT-GRAMMAR.md §5); only the GraphCodeShapeSvg adapter touches Epic's SVG Importer types, so
 * GraphCode Shape loads in a project without that plugin and outline('...svg') refuses by naming it (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md).
 */
namespace GraphCodeShapeSVG
{
	/** Loads ObjectPath as SVG Data; runs on the game thread. False when it is not one. OutAsset is the build input. */
	using FLoader = GraphCodeSvg::FLoader;

	/** The adapter's slot: set at its startup, cleared at its shutdown - Core's, which every product reads. */
	GRAPHCODESHAPE_API void SetLoader(FLoader Loader);
}
