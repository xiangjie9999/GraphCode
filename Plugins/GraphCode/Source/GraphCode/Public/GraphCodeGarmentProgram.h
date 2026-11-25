// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"

// What Core knows about a `.gcgarment` (`Docs/GC-GARMENT-GRAMMAR.md`): the header word and one seam,
// as `GraphCodeGroomProgram.h` and `GraphCodeRigProgram.h` do for theirs.

/**
 * What a mesh a `.gcgarment` built remembers about the program that made it: the source file and a
 * digest of its text. Package metadata, so it is editor data, stripped on cook, and a built garment
 * runs with GraphCode uninstalled. Reading it is Core's; stamping it is the product's.
 */
struct GRAPHCODE_API FGcGarmentAssetProvenance
{
	FString Source;
	FString Program;

	/**
	 * A ring overlay's body and legend. With a body, `gc_render` of the overlay draws it on that
	 * body and answers with the legend — one line per ring: name, colour, length, direction, `@0`.
	 */
	FString Body;
	FString Legend;

	bool IsSet() const { return !Source.IsEmpty() && !Program.IsEmpty(); }

	static const TCHAR* SourceKey();
	static const TCHAR* ProgramKey();
	static const TCHAR* BodyKey();
	static const TCHAR* LegendKey();

	/** False, and `Out` untouched, where there is no stamp. */
	static bool Read(const UObject* Asset, FGcGarmentAssetProvenance& Out);

	/** It does not save: the build's own save carries it. */
	static void Stamp(UObject* Asset, const FGcGarmentAssetProvenance& In);
};

/** What building a garment program did, or would do. Filled by GraphCode Garment; Core only carries it. */
struct GRAPHCODE_API FGcGarmentBuildResult
{
	bool bRan = false;
	TArray<FString> BuiltAssets;
	TArray<FString> Changes;
	TArray<FString> Notes;
	FString Message;
};

using FGcGarmentProgramBuilder = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	bool /*bSave*/,
	FGcGarmentBuildResult& /*OutResult*/,
	FString& /*OutError*/)>;

/** Core's complete knowledge of `.gcgarment`: its header and one product seam. */
class GRAPHCODE_API FGraphCodeGarmentProgram
{
public:
	/** `gcgarment 1` on the first line that is not a comment. */
	static bool LooksLikeGarmentProgram(const FString& Text);
	static bool Register(EGcProduct Product, FGcGarmentProgramBuilder Builder, FString& OutError);
	static void Unregister(EGcProduct Product);

	/** Installed AND bound. */
	static bool IsAvailable();

	/**
	 * Build the program, or refuse naming GraphCode Garment as the fix. `bSave` false is `gc_apply save:false`: what
	 * the build writes is left dirty in the editor rather than saved.
	 */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun, bool bSave,
		FGcGarmentBuildResult& OutResult, FString& OutError);

	/** The same, saving what it writes. */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcGarmentBuildResult& OutResult, FString& OutError);
};
