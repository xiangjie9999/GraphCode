// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeGarmentProgram.h"

class FGcGarmentBody;
struct FGcGarmentBuiltRing;
struct FGcGarmentDocument;

class GRAPHCODEGARMENT_API FGraphCodeGarmentBuilder
{
public:
	/**
	 * Core's seam. Reads the file, finds the body, builds the rings and every garment `build { }`
	 * names, and writes the ring overlay and one mesh per build line — static, or skeletal on the body's
	 * skeleton for `[as=skeletal]`. A dry run builds all of it and writes nothing.
	 */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcGarmentBuildResult& OutResult, FString& OutError);

	/**
	 * The same, and `bSave` false (`gc_apply save:false`) leaves every asset it writes dirty rather than saved. A line that goes
	 * into a MetaHuman's wardrobe is refused with it by name, before anything is written: the wardrobe is written by Core
	 * Extension's writer, which saves as it writes.
	 */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun, bool bSave,
		FGcGarmentBuildResult& OutResult, FString& OutError);

	/** Every ring the file declares, in order, on this body. */
	static bool BuildRings(const FGcGarmentDocument& Document, const FGcGarmentBody& Body,
		TArray<FGcGarmentBuiltRing>& OutRings, TArray<FString>& OutNotes, FString& OutError);

	/** Where the ring overlay goes: `DIAG_<file>_Rings` in `into:`, or beside the file. */
	static FString OverlayPath(const FGcGarmentDocument& Document, const FString& DocumentPath);
};
