// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UGroomBindingAsset;

namespace GraphCodeGroom
{
	/** Rebuild a binding for its current settings and wait; safe for an already-built binding. */
	GRAPHCODEGROOM_API void RebuildNativeBinding(UGroomBindingAsset* Binding);
}
