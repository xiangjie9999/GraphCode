#pragma once

#include "CoreMinimal.h"

/**
 * The resolution surface over an unresolved `.gc` merge. GC-VCS-MERGE-DESIGN.md M1b.8, §4.9.
 *
 * WHAT THIS IS FOR, and what it deliberately is not (§4.9.1 assessed a broader tool and cut most of
 * it): enumerating conflicted files project-wide is `GcCheck`, which CI already runs, and rendering
 * what a merge changed is `git diff`, on text this format exists to make diffable. The one thing
 * nothing else does is turn a `conflict { }` construct into `(id, entity, property, base, ours,
 * theirs)` that a person or a model can reason about, and take a decision back. That is all this is.
 *
 * IT READS THE `.gc` ITSELF, never a sibling manifest. §4.9's `<asset>.gc.conflicts` file was
 * rejected in §4.12: a second file is a second thing to keep in sync, against a format whose whole
 * premise is that the `.gc` is the unit.
 *
 * AND IT PROPOSES RATHER THAN COMMITTING. Resolving a merge conflict is a semantic decision about
 * code the resolver did not write and cannot run, so `Pick` edits the file and stops — it does not
 * apply the result to an asset, and it does not commit. The apply is `gc_apply`, deliberately a
 * separate step with its own gates.
 */
struct GRAPHCODE_API FGcConflictRecord
{
	/** 1-based, positional within the file. Stable for as long as the file is unmodified. */
	int32 Id = 0;

	FString Kind;
	FString Entity;
	FString Property;
	FString Detail;

	FString Base;
	FString Ours;
	FString Theirs;
	bool bHasBase = false;

	/** 1-based line of the construct's opening brace, so a message is navigable. */
	int32 Line = 0;
};

class GRAPHCODE_API FGcResolve
{
public:
	/** Every unresolved conflict in a `.gc`, in file order. */
	static bool List(const FString& Text, TArray<FGcConflictRecord>& Out, FString& OutError);

	/** What a resolution may take. */
	enum class EChoice : uint8
	{
		Ours,
		Theirs,
		Custom,
	};

	/**
	 * Resolve one conflict and remove its construct.
	 *
	 * `Ours` is already what the merge left in the text - every ruling keeps ours - so taking ours
	 * only removes the construct and touches nothing else. Taking theirs, or a custom value, edits
	 * the site.
	 *
	 * THE EDIT IS EXACT OR IT IS REFUSED. The site is located by walking the entity path inside the
	 * named block, and the conflict's recorded `ours` text must occur EXACTLY ONCE on the statement
	 * found there. Zero occurrences or several means the file moved under the construct, and a
	 * resolver that guessed in that state would write a value into a line nobody chose - which is
	 * the one failure a merge tool may not have. It says so instead.
	 */
	static bool Pick(const FString& Text, int32 Id, EChoice Choice, const FString& CustomValue,
		FString& OutText, FString& OutError);
};
