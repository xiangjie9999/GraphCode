// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeShapeProgram.h"

/**
 * **WHAT IS IN A FOLDER OF `.gcshape` FILES, WITHOUT BUILDING ANY OF THEM.**
 * `GC-SHAPE-PRESET-LIBRARY.md` §7 — the reader behind `gc_search kind:"preset"` and
 * `gc_describe preset:`.
 *
 * **IT COSTS A PARSE, NOT A MESH, AND THAT IS THE WHOLE DESIGN.** A parse is a property of ONE
 * file's text — no editor, no asset registry, no evaluation — so answering *what does the library
 * have for a worn brick* is a directory walk and N parses. The alternative, building each entry to
 * see what it is, is the mistake `FGcShapeRecipeResolver` already refused to make for the same
 * question one document at a time.
 *
 * **AND IT IS NOT LINKED.** `GraphCodeShapeLink::Resolve` reads every file an entry imports; an
 * index that linked would read the whole transitive graph to answer a question about names. A
 * `Module` entry is reported as what it is — this file imports that one — and nothing is followed.
 * ~~*Not linked*~~ — **except a file with a bare `$Entry` clause in a parameter list (K1, 2026-11-19)**:
 * what that clause takes may be in an imported file, so that file alone is linked, and its rows carry the
 * whole list with where each parameter came from (`Inherits`, `ParameterFrom`, `Overrides`).
 *
 * **A DESCRIPTION IS THE `#` BLOCK ABOVE THE ENTRY, user-ruled 2026-09-11**, lifted by the parser
 * itself so this file re-reads nothing. See `FGcShapeEntry::Description`.
 */
class GRAPHCODESHAPE_API FGraphCodeShapeLibrary
{
public:
	/**
	 * Every entry of every `.gcshape` under `Roots`, in file order within each file.
	 *
	 * `Roots` are CONTENT paths — `/GraphCodeShape/Presets`, `/Game/Steampunk/Recipes` — and a root
	 * that is not under a mounted point is reported in `OutSkipped` rather than failing the call: a
	 * caller asking about two folders should not lose the one that is there because the other is
	 * not. A file that does not PARSE is likewise skipped with its refusal, for the same reason and
	 * a sharper one — a library with one broken file is still a library, and the refusal is the most
	 * useful thing to say about that file.
	 *
	 * False only where nothing could be read at all, which is a caller error rather than a corpus
	 * one.
	 */
	static bool Index(const TArray<FString>& Roots, TArray<FGcShapeLibraryEntry>& Out,
		TArray<FString>& OutSkipped, FString& OutError);

	/** Every entry of ONE file. The single-file half of `Index`, and what it is built from. */
	static bool IndexFile(const FString& ContentPath, TArray<FGcShapeLibraryEntry>& Out,
		FString& OutError);

	/**
	 * **WHERE THE LIBRARY LIVES WHEN NOBODY SAYS.** One answer, so a search and a describe cannot
	 * disagree about what *the presets* means, and so the shipped mount is reachable without the
	 * caller knowing the plugin's content path.
	 */
	static const TArray<FString>& DefaultRoots();
};
