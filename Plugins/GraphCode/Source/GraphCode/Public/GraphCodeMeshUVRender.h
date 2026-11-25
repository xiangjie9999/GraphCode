// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

struct FGcCaptureView;
struct FGcMeshUVQueryContext;
struct FGcScreenToWorldRequest;
struct FGcScreenToWorldResult;
namespace GraphCodeRender { struct FLegendMaskRequest; struct FLegendMaskResult; }

/** Retained source-LOD diagnostics. Never changes asset materials or the editor viewport. */
namespace GraphCodeMeshUVRender
{
GRAPHCODE_API bool Prepare(const FGcCaptureView& Capture, TSharedPtr<FGcMeshUVQueryContext>& Out,
    TArray<FString>& Errors);
GRAPHCODE_API bool Query(const FGcCaptureView& Capture, const FGcScreenToWorldRequest& Request,
    double X, double Y, FGcScreenToWorldResult& Out, TArray<FString>& Errors);
GRAPHCODE_API bool Legend(const GraphCodeRender::FLegendMaskRequest& Request,
    GraphCodeRender::FLegendMaskResult& Out, TArray<FString>& Errors);
}
