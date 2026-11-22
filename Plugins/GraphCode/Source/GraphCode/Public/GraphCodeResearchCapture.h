// Copyright OraStudio. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

/**
 * A capture another agent asked for, written by a build on request only (GC-MOVE-ARM-LADDER.md, "Captures for the
 * research"; Docs/Research/README.md). `Saved/Research/capture-request.json` names what to write out:
 *
 *   { "out": "Saved/Research/arm-ladder-20261005", "key_bones": ["index_01_r"], "solid_meshes": ["SM_VRPistol"] }
 *
 * With no such file a build captures nothing and does no more than look for it. A request file rather than a
 * console variable because the editor's tool bridge refuses `gc.` commands, and a request needs no restart.
 */
namespace GcResearchCapture
{
	/** The folder to write into when the request's List names Name (case-insensitive), else empty. */
	GRAPHCODE_API FString Wants(const TCHAR* List, const FString& Name);
	/** Writes Text to Folder/File, making the folder, and says where in the log. */
	GRAPHCODE_API bool Write(const FString& Folder, const FString& File, const FString& Text);
	/** A double at full precision, for a capture someone replays offline. */
	GRAPHCODE_API FString Exact(double Value);
}
