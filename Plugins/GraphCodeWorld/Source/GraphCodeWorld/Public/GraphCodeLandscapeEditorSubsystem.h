// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "GraphCodeLandscapeEditorSubsystem.generated.h"

class ALandscape;
class ALandscapeBlueprintBrushBase;

/** Native Landscape operations which Unreal exposes to C++ but not to Blueprint authoring. */
UCLASS()
class GRAPHCODEWORLD_API UGraphCodeLandscapeEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Attach a placed native/Blueprint brush to a named Landscape edit layer. Repeating the
	 * same binding preserves brush order. A missing layer is created only when requested.
	 * Moves preserve both landscapes' other brushes and painted/sculpted data.
	 * Invoke through gc_invoke with asset_path:/Script/GraphCodeWorld.GraphCodeLandscapeEditorSubsystem.
	 * Success means the binding landed; rendering and collision complete on subsequent editor ticks.
	 */
	UFUNCTION(BlueprintCallable, Category="GraphCode|Landscape")
	bool BindLandscapeBrush(ALandscapeBlueprintBrushBase* Brush, ALandscape* Landscape,
		FName LayerName, bool bCreateLayer, FString& Error);

	/** Remove this brush from its current layer without deleting the brush, layer or terrain data. */
	UFUNCTION(BlueprintCallable, Category="GraphCode|Landscape")
	bool UnbindLandscapeBrush(ALandscapeBlueprintBrushBase* Brush, FString& Error);
};
