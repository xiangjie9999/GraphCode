// Copyright OraStudio. All Rights Reserved.
//
// THE PARTITIONED FIXTURE, IN ONE CARRIER — `Content/__RuntimeFixture/RuntimeFixture_WP` and the
// three things every spec that opens it needs: its package name, the load, and the unwind.
//
// **IT EXISTS BECAUSE THE SECOND USER MADE IT A UNITY COLLISION, and `Tools/Sweep/anon-collisions.py`
// is what said so before the commit** ([[uncommitted-work-hides-unity-collisions]]). World Partition
// row 8's specs needed the same map `GraphCodeExternalActorTests` had been opening since row 11m,
// and copying the helpers gave one module two anonymous-namespace `LoadMapFixture`s,
// `PartitionedFixtureMap`s and `OrdinaryFixtureMap`s. Those compile only while UBT keeps the two
// files out of one unity blob — which adaptive unity does for exactly as long as both are in the
// working set, and stops doing the moment they are committed — a thing that builds today only
// because of a build-system accident. A NAMED namespace and `inline` is the fix
// `AgentGuide/BUILD-AND-TEST.md` prescribes for the identical case.
//
// **AND IT MOVED FROM `Private/Tests/` TO `Public/` FOR ITS THIRD USER, WP row 8b.** `GraphCodeWorld`
// drives the realisers, so the specs that prove a write does not duplicate an actor in an unloaded
// cell live in that module — and a private header is not reachable from there. The choice was
// between promoting one header and writing a third copy of the same map name, load and unwind, which
// is the thing the two paragraphs above exist to stop ([[misplaced-capability-costs-a-name]]).
// `GraphCodeRuntime`'s `GraphCodeTestActors.h` is the precedent: a spec-support header in Public is
// how this project already shares fixtures across modules.
//
// **AND A COPIED RULE DRIFTS, which is the reason that outlives the blob.** The two copies had
// already diverged: one said *"%s is not in this project."* and left the reader to find the
// conversion command in a different helper further down the file. The merged version names the
// command at the point of failure, because that is the moment somebody needs it.

#pragma once

#include "CoreMinimal.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace GraphCodePartitionedFixture
{
	/** The World Partition fixture: the same content as the ordinary one, converted. */
	inline const TCHAR* PartitionedMap = TEXT("/Game/__RuntimeFixture/RuntimeFixture_WP");

	/** The unpartitioned original, which is also what the conversion command below is pointed at. */
	inline const TCHAR* OrdinaryMap = TEXT("/Game/__RuntimeFixture/RuntimeFixture");

	/**
	 * Open a fixture map into the editor world context, reusing it when it is already resident.
	 *
	 * **A MISSING FIXTURE FAILS LOUDLY AND NAMES THE COMMAND**, because a silently skipped gate and
	 * a passing gate must not look the same in a log.
	 */
	inline UWorld* LoadMap(FAutomationTestBase& Test, const TCHAR* PackageName)
	{
		if (!GEditor)
		{
			Test.AddError(TEXT("No GEditor. These gates need an editor context."));
			return nullptr;
		}
		if (UWorld* Current = GEditor->GetEditorWorldContext().World();
			Current && Current->GetOutermost()->GetName() == PackageName)
		{
			return Current;
		}
		FString Filename;
		if (!FPackageName::TryConvertLongPackageNameToFilename(
				PackageName, Filename, FPackageName::GetMapPackageExtension())
			|| !FPaths::FileExists(Filename))
		{
			Test.AddError(FString::Printf(
				TEXT("%s is not in this project. Build the fixture with: UnrealEditor-Cmd.exe "
					 "<project> -run=WorldPartitionConvert %s -AllowCommandletRendering "
					 "-ConversionSuffix"),
				PackageName, OrdinaryMap));
			return nullptr;
		}
		UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(Filename);
		if (!World)
		{
			Test.AddError(FString::Printf(TEXT("Could not load %s"), *Filename));
		}
		return World;
	}

	/**
	 * **A PARTITIONED WORLD LEFT RESIDENT IS A CRASH WITH SOMEBODY ELSE'S NAME ON IT.** Measured
	 * 2026-08-10, and it took the FULL GATE down 757 specs into ~1258:
	 *
	 *     Assertion failed: InitState == EWorldPartitionInitState::Uninitialized
	 *       <- FBlueprintIRFidelitySpec::ExpectSurvivesTheDisk()
	 *
	 * A `UWorldPartition` must be uninitialised BEFORE its world is destroyed, and the engine only
	 * does that on the path that replaces the current world — so a partitioned world that is merely
	 * abandoned sits there, still initialised, until something forces a collect. The something was a
	 * `CollectGarbage(bPerformFullPurge=true)` a hundred specs and a whole other plugin away.
	 *
	 * **NEITHER FILTER CAN SEE IT ALONE**, which is why it survived: `UnrealClaude.BlueprintIR` never
	 * loads a partitioned map, and `GraphCode.ExternalActor` never full-purges. It needs both halves
	 * in one process, which is exactly what the full gate is.
	 *
	 * `NewBlankMap` is the engine's own unwind and is what makes the uninitialise happen. `LoadMap`
	 * above still reuses a resident fixture, so the cost is one blank map per spec that asked for
	 * one, not a reload of the fixture.
	 */
	struct FScopedMap
	{
		explicit FScopedMap(UWorld* InWorld) : bLoaded(InWorld != nullptr) {}

		FScopedMap(const FScopedMap&) = delete;
		FScopedMap& operator=(const FScopedMap&) = delete;

		~FScopedMap()
		{
			if (bLoaded && GEditor)
			{
				UEditorLoadingAndSavingUtils::NewBlankMap(/*bSaveExistingMap=*/false);
			}
		}

		bool bLoaded = false;
	};
}
