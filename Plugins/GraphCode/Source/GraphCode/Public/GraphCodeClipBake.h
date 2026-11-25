// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

class AActor;
class UActorComponent;
class UAnimSequence;

/**
 * `.gcmove`'s `[bake]` (GC-GARMENT-CLOTH.md §E2, user 2026-10-06: "we shall have ways to decide whether is it a real time
 * simulation or animation based simulation"): what a character wears that simulates, run once over a built clip and saved
 * beside it, so the clip plays back with its cloth and hair without simulating. Move asks; each product that owns a
 * simulation (Garment for Chaos cloth, Groom for hair) answers. None depends on another, and what is written is native
 * Unreal assets only.
 *
 * Every baker records first and writes nothing; only when all have recorded does each publish (GC-GARMENT-HAIR-BAKE.md R1),
 * so a recording that fails leaves every earlier bake on disk as it was.
 */
struct GRAPHCODE_API FGcClipBake
{
	struct FRequest
	{
		/** The built clip, saved, with its `[settle=]` and `wind()` on it. */
		UAnimSequence* Clip = nullptr;
		/** The `.gcmove`'s `character:` Blueprint, and the `part:` component the clip plays on. */
		UObject* Character = nullptr;
		FString Part;
		/** `[settle=]` seconds: held on the first pose before the recording starts, so the bake begins hung. */
		double SettleSeconds = 0.0;
		/** The clip's package path; what is baked is written beside it. */
		FString PackagePath;
	};

	struct FResult
	{
		TArray<FString> Written;
		TArray<FString> Notes;
	};

	/**
	 * What one baker recorded, not yet written. Three stages, each run for every baker before the next begins: record (the
	 * baker itself; changes nothing outside its own preview world), Prepare (in memory only — packages opened, loaded assets
	 * written into — so a failure saves nothing anywhere), Publish (the saves; only a save can fail there).
	 */
	struct FRecorded
	{
		/** Set, with Publish unbound, when the character wears nothing this baker records: what it looked for. */
		FString Nothing;
		/** Optional. Adds to `OutChangedInMemory` every package it changed in memory, whether it then succeeds or fails. */
		TUniqueFunction<bool(TArray<FString>& OutChangedInMemory, FString& OutError)> Prepare;
		/** Saves what was prepared, adding each file to `Out.Written` the moment it is saved. */
		TUniqueFunction<bool(FResult& Out, FString& OutError)> Publish;
	};

	using FBaker = TDelegate<bool(const FRequest&, FRecorded&, FString&)>;
	/** `Owner` names the product ("Garment", "Groom"); registering it again replaces its baker. */
	static void RegisterBaker(FName Owner, FBaker Baker);
	static void UnregisterBaker(FName Owner);
	/** Every registered baker, in registration order. */
	static bool Bake(const FRequest& Request, FResult& Out, FString& Error);
	/** The same with the bakers given, for a test that must not run the registered ones. */
	static bool BakeWith(TConstArrayView<TPair<FName, FBaker>> Bakers, const FRequest& Request, FResult& Out, FString& Error);

	/** What every baker writes with: `Asset` saved into its package's file, registered with the asset registry. */
	static bool Save(UObject& Asset, const FString& PackagePath, FString& OutError);
	/**
	 * The variable a Blueprint graph reads `Component` through — the property holding it, which for a native component can
	 * differ from its own name (a Character's `CapsuleComponent` is `CollisionCylinder`) — for the playback recipe a note
	 * prints. Empty where no property holds it.
	 */
	static FString VariableOf(const AActor& Actor, const UActorComponent& Component);
};
