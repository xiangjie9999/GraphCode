// Copyright OraStudio. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

class USkeleton;
struct FReferenceSkeleton;

/** Editor-only recipe shared by Rig's native graph and Move's native animation bake. */
struct GRAPHCODE_API FGcRigTwist
{
	FName Driver;
	FName Helper;
	double Share = 0.0;

	static void Write(USkeleton& Skeleton, const TArray<FGcRigTwist>& Twists);
	static bool Read(const USkeleton& Skeleton, TArray<FGcRigTwist>& Out, FString& Error);
	bool Resolve(const FReferenceSkeleton& Ref, int32& DriverIndex, int32& HelperIndex,
		FQuat& DriverToParent, FString& Error) const;
	/** Assign rotation only; incoming helper translation and scale are independent. */
	static FQuat Rotation(const FQuat& DriverLocal, const FQuat& DriverRest,
		const FQuat& DriverToParent, const FQuat& HelperRest, double Share);
};
