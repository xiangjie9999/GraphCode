// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/** Shared field arithmetic. Callers choose bounded coverage or unbounded influence. */
namespace GraphCodeWeight
{
	enum class EOperation : uint8 { Add, Multiply, Union, Cap, Subtract, Exclude, Blend };

	inline double Apply(EOperation Operation, double A, double B, double Amount = 0.5)
	{
		switch (Operation)
		{
		case EOperation::Add: return A + B;
		case EOperation::Multiply: return A * B;
		case EOperation::Union: return FMath::Max(A, B);
		case EOperation::Cap: return FMath::Min(A, B);
		case EOperation::Subtract: return FMath::Max(0.0, A - B);
		case EOperation::Exclude: return A * (1.0 - FMath::Clamp(B, 0.0, 1.0));
		case EOperation::Blend: return FMath::Lerp(A, B, Amount);
		}
		checkNoEntry();
		return 0.0;
	}

	inline double ApplyCoverage(EOperation Operation, double A, double B, double Amount = 0.5)
	{
		return FMath::Clamp(Apply(Operation, FMath::Clamp(A, 0.0, 1.0),
			FMath::Clamp(B, 0.0, 1.0), Amount), 0.0, 1.0);
	}
}
