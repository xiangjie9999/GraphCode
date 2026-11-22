// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGcShapeDocument;

/**
 * **BIND A PARSED `.gcshape` TO THE FILES IT IMPORTS** — `GC-SHAPE-GRAMMAR.md` §3.1.
 *
 * A parse is a property of ONE file's text and knows nothing of disk; a `Pillar = shape('/…')`
 * leaves a `Module` entry naming a path and nothing loaded. Linking is the second half: it reads
 * each named `.gcshape`, parses it, links IT, and hangs the result on `Document.Imports` so the
 * evaluator can reach `$Pillar.Column`.
 *
 * **THE DOCUMENT GRAPH MAY BE CYCLIC AND LOADING STILL TERMINATES.** Two files that import each
 * other are legal — a shared library and a caller often do — so the cycle a customer must be
 * protected from is an ENTRY cycle (`Pillar.A` builds `Base.B` builds `Pillar.A`), and that is the
 * evaluator's cross-file cycle stack, not this. Here a path already being loaded links to the SAME
 * in-progress node rather than reading it twice, which is what makes a mutual import terminate.
 *
 * **AND WHAT A CUSTOMER'S AI ACTUALLY HITS IS A MISSING FILE OR A TYPO**, so every failure names the
 * alias, the path and the file it was reached through — never a bare *could not load*.
 *
 * **AN IMPORT MAY CARRY ARGUMENTS, AND THIS IS WHERE THEY MEAN SOMETHING.** `shape('/…', R = 0.5,
 * Steel = $Brass)` replaces a top-level entry of the file it names, on a PRIVATE copy of that
 * document, before its own imports resolve — which is what lets a middle file forward a knob its
 * importer overrode.
 *
 * **A NUMBER IS SETTLED IN THE CALLER'S SCOPE AND EVERY OTHER KIND IS FORWARDED.** A number's whole
 * value is its literal; a material, profile, draw, plane or mesh is a name for something written
 * somewhere, so the entry points back at where it was written and keeps its own file's `$names`.
 *
 * A document is therefore keyed by path AND arguments — and by the forwarded entry's IDENTITY, not
 * by the text `$Brass`, or two importers each passing their own would share one node. An import
 * that passes nothing shares one node per path exactly as it always did.
 */
namespace GraphCodeShapeLink
{
	/** Reads the `.gcshape` text for a content path. False with `OutError` when it is not there. */
	using FReadFile = TFunctionRef<bool(const FString& ContentPath, FString& OutText, FString& OutError)>;

	/**
	 * Resolve `Document`'s imports (recursively), filling `Imports`, `SelfPath` and `SourceText` on
	 * it and every file it reaches. `SelfPath` is the document's own content path — the caller's
	 * `asset_path` for the top file, empty where there is none. `SelfText` is this document's own raw
	 * text, stored for the digest fold. False with `OutError` naming a missing import, one that did
	 * not parse, or a `build { }` line that names a module.
	 */
	GRAPHCODESHAPE_API bool Resolve(FGcShapeDocument& Document, const FString& SelfPath,
		const FString& SelfText, FReadFile Read, FString& OutError);

	/** The disk reader `Resolve` uses in a real build: a content path -> `Content/…/<name>.gcshape`. */
	GRAPHCODESHAPE_API bool ReadFromDisk(const FString& ContentPath, FString& OutText,
		FString& OutError);

	/**
	 * The text whose digest a dependent records — this file's own text, then every file it imports,
	 * transitively, each named by its path and sorted so the fold is stable. An upstream edit changes
	 * one of those texts and so changes the digest, which is the whole point: the dependent's asset
	 * stops reading *the program still describes me* the moment a file it is built from moves.
	 */
	GRAPHCODESHAPE_API FString FoldedText(const FGcShapeDocument& Document);
}
