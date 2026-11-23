#pragma once

#include "CoreMinimal.h"
#include "Editor.h"
#include "Containers/Ticker.h"

namespace GraphCodeRender
{
	/** Temporarily opts active rendering out of editor CPU throttling; never changes preferences. */
	class FScopedRenderActivity
	{
	public:
		FScopedRenderActivity()
		{
			check(IsInGameThread());
			if (GEditor)
			{
				Editor = GEditor;
				UEditorEngine::FShouldDisableCPUThrottling Delegate =
					UEditorEngine::FShouldDisableCPUThrottling::CreateLambda([] { return true; });
				Handle = Delegate.GetHandle();
				GEditor->ShouldDisableCPUThrottlingDelegates.Add(MoveTemp(Delegate));
			}
		}
		~FScopedRenderActivity()
		{
			check(IsInGameThread());
			if (UEditorEngine* Owner = Editor.Get())
			{
				Owner->ShouldDisableCPUThrottlingDelegates.RemoveAll(
					[this](const UEditorEngine::FShouldDisableCPUThrottling& Delegate) { return Delegate.GetHandle() == Handle; });
			}
		}
		FScopedRenderActivity(const FScopedRenderActivity&) = delete;
		FScopedRenderActivity& operator=(const FScopedRenderActivity&) = delete;

	private:
		TWeakObjectPtr<UEditorEngine> Editor;
		FDelegateHandle Handle;
	};

	/** Game-thread request lifetime, including time queued before the capture callback starts. */
	inline void KeepEditorResponsiveWhile(TFunction<bool()> IsActive)
	{
		check(IsInGameThread());
		if (!IsActive()) { return; }
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[Activity = MakeShared<FScopedRenderActivity>(), IsActive = MoveTemp(IsActive)](float) { return IsActive(); }));
	}
}
