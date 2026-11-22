// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class UAnimSequence;
class USkeletalMesh;
class USoundWave;

/**
 * `.gcmove`'s `clip(speech=)` (GC-MOVE-LIP-SYNC.md L1): a face clip solved from a sound. Move asks; the product that owns the
 * native solver answers (Advanced's MetaHuman module, with Epic's MetaHuman Animator). Move never links MetaHuman, and what it
 * gets back is a native AnimSequence of float curves on the face's own skeleton, which it reads as it reads `clip(source=)`.
 */
struct GRAPHCODE_API FGcClipSpeech
{
	struct FRequest
	{
		USoundWave* Sound = nullptr;
		/** The `part:` the clip plays on: the face whose controls the solver writes. */
		USkeletalMesh* Face = nullptr;
		/** `face=mouth`: the mouth only, the rest of the face left to whatever else animates it. */
		bool bMouthOnly = false;
	};

	struct FResult
	{
		/** Transient, on Face's skeleton, float curves only; nothing is written to disk. */
		TStrongObjectPtr<UAnimSequence> Solved;
		/** How long the sound lasts, and how long the solved keys last (they can end up to a frame short of it). */
		double SoundSeconds = 0.0;
		double KeySeconds = 0.0;
		TArray<FString> Notes;
	};

	using FSolver = TDelegate<bool(const FRequest&, FResult&, FString&)>;
	static void RegisterSolver(FSolver Solver);
	static void UnregisterSolver();
	static bool Solve(const FRequest& Request, FResult& Out, FString& OutError);
};
