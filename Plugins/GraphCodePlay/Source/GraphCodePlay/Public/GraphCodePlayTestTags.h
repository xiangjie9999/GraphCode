// Fixture gameplay tags — R3.
//
// **THESE LIVE IN THE RUNTIME MODULE FOR AN ENGINE REASON, NOT A TIDINESS ONE.**
// `FNativeGameplayTag`'s constructor is `ensureAlwaysMsgf(false, "… The module type is '%s' but
// needs to be 'Runtime' or 'RuntimeAndProgram'. Client and Server tags must match.")` for anything
// declared outside a Runtime module — native tags have to exist identically on both ends, and a
// dedicated server never loads an Editor module. Declaring them in `GraphCodePlayEditor`, where
// the gates that use them live, therefore fired an ensure per run and left a `Saved/Crashes`
// dump behind: the specs still PASSED, so nothing red ever said so, and the only visible symptom
// was crash directories appearing under a suite that reported green — including another agent's.
//
// **AND THEY ARE GUARDED TWICE, so no packaged game carries a tag named `GcTest`.**
// `WITH_GRAPHCODE_ABILITIES` is the optional-plugin gate; `WITH_DEV_AUTOMATION_TESTS` is what keeps
// a fixture out of a shipping build. The alternative — declaring them in the host project's own
// `DefaultGameplayTags.ini` — was rejected because a spec that needs project CONFIG compiles fine
// for a Core-only install and then fails, which is the one outcome the gating recipe exists to
// prevent (`PLAY-SESSION-DESIGN.md` §3.1i, recipe part 4).

#pragma once

#include "CoreMinimal.h"

#if WITH_GRAPHCODE_ABILITIES && WITH_DEV_AUTOMATION_TESTS

#include "NativeGameplayTags.h"

/**
 * Three tags whose NAMES are deliberately out of alphabetical order against their declaration.
 *
 * `GcTest.State.Guarding` is declared first and sorts last, so the writer's promise to sort by name
 * cannot be met by accident — a fixture already in order could not tell a sort from a coincidence.
 */
GRAPHCODEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GcTestGuarding);
GRAPHCODEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GcTestAttack);
GRAPHCODEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GcTestAlerted);

#endif
