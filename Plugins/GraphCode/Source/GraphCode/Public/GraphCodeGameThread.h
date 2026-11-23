// Copyright OraStudio. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

namespace GraphCodeGameThread
{
	GRAPHCODE_API void Run(TFunctionRef<void()> Work);
	GRAPHCODE_API void KeepInput(UObject* Object);
	template<class T> T* Load(UObject* Outer, const TCHAR* Name)
	{
		T* Result = nullptr;
		Run([&] { Result = LoadObject<T>(Outer, Name); KeepInput(Result); });
		return Result;
	}
	/** Serves only this dispatcher, never arbitrary tickers, Slate or editor tasks. */
	GRAPHCODE_API void WaitServing(TFunctionRef<bool()> IsDone);
	class GRAPHCODE_API FCarry
	{
	public:
		explicit FCarry(TFunction<void(TFunctionRef<void()>)> Around, TFunction<void(UObject*)> Input = {});
		~FCarry();
		FCarry(const FCarry&) = delete;
		FCarry& operator=(const FCarry&) = delete;
	private:
		FCarry* Previous;
		TFunction<void(TFunctionRef<void()>)> Callback;
		TFunction<void(UObject*)> Input;
		uint32 ThreadId;
		friend GRAPHCODE_API void KeepInput(UObject* Object);
		friend GRAPHCODE_API void Run(TFunctionRef<void()> Work);
	};
}
