// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"
#include "UObject/StrongObjectPtr.h"

class USkeletalMesh;

/**
 * What a native Groom or Binding remembers about the `.gcgroom` that owns its generated data.
 * Core owns only these metadata keys so a future `.gc` projector can read them without depending on
	 * HairStrands. Geometry fingerprints are produced by GraphCode Groom.
 */
struct GRAPHCODE_API FGcGroomAssetProvenance
{
	FString Source;
	FString Program;
	FString Output;
	FString Target;
	FString Groups;
	FString Generator;
	FString Engine;
	FString Role;

	bool IsSet() const
	{
		return !Source.IsEmpty() && !Program.IsEmpty() && !Output.IsEmpty() && !Role.IsEmpty();
	}

	static const TCHAR* SourceKey();
	static const TCHAR* ProgramKey();
	static const TCHAR* OutputKey();
	static const TCHAR* TargetKey();
	static const TCHAR* GroupsKey();
	static const TCHAR* GeneratorKey();
	static const TCHAR* EngineKey();
	static const TCHAR* RoleKey();

	static bool Read(const UObject* Asset, FGcGroomAssetProvenance& Out);
	static void Stamp(UObject* Asset, const FGcGroomAssetProvenance& In);
	static void Clear(UObject* Asset);
};

/** Product-neutral result for one `.gcgroom` build. HairStrands types stay in GraphCode Groom. */
struct GRAPHCODE_API FGcGroomBuildResult
{
	bool bRan = false;
	TArray<FString> BuiltAssets;
	TArray<FString> Changes;
	TArray<FString> Notes;
	FString Message;
};

using FGcGroomProgramBuilder = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	FGcGroomBuildResult& /*OutResult*/,
	FString& /*OutError*/)>;

/** Core's complete knowledge of `.gcgroom`: its header and one optional-product seam. */
class GRAPHCODE_API FGraphCodeGroomProgram
{
public:
	static bool LooksLikeGroomProgram(const FString& Text);
	static bool Register(EGcProduct Product, FGcGroomProgramBuilder Builder, FString& OutError);
	static void Unregister(EGcProduct Product);
	static bool IsAvailable();
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcGroomBuildResult& OutResult, FString& OutError);
};

using FGcGroomTargetSupports = TDelegate<bool(UObject* /*Asset*/)>;
using FGcGroomTargetResolve = TDelegate<bool(
	UObject* /*Asset*/,
	TStrongObjectPtr<USkeletalMesh>& /*OutSurface*/,
	FString& /*OutError*/)>;

/**
 * An optional product's translation from its own character asset to the skeletal surface hair
 * grows on. Groom depends only on Core; framework products register adapters into this Core-owned
 * seam instead of becoming link dependencies of Groom.
 */
struct GRAPHCODE_API FGcGroomTargetAdapter
{
	FName Name;
	FGcGroomTargetSupports Supports;
	FGcGroomTargetResolve Resolve;
};

enum class EGcGroomTargetResolution : uint8
{
	NotHandled,
	Resolved,
	Failed,
};

class GRAPHCODE_API FGraphCodeGroomTargetAdapters
{
public:
	static bool Register(FGcGroomTargetAdapter Adapter, FString& OutError);
	static void Unregister(FName Name);
	static EGcGroomTargetResolution Resolve(UObject* Asset,
		TStrongObjectPtr<USkeletalMesh>& OutSurface, FString& OutError);
};
